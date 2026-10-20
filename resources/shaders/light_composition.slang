/**
 * -------------------------------------------------------
 * Scenery Editor X
 * -------------------------------------------------------
 * Copyright (c) 2026 Thomas Ray
 * Copyright (c) 2026 Coalition of Freeware Developers
 * -------------------------------------------------------
 * light_composition.slang
 * -------------------------------------------------------
 * Composes the deferred frame in pre-exposed radiance (nits x get_pre_exposure()): the scene's own lights
 * (light.slang), the environment's sun, moon, sky and ground (environment_lighting.slang), emissive (LIT) light, then
 * aerial perspective. Where nothing was drawn: the interim sky with the atmosphere off (sky.slang draws it otherwise).
 *   tex       light_diffuse   (scene lights, pre-exposed irradiance x BRDF, without the albedo)
 *   tex2      light_specular  (scene lights, pre-exposed)
 *   tex3      lut_brdf_specular;  the atmosphere LUTs and probes (atmosphere_common.slang)
 *   pass_get_f4_value(): x 1 when the environment's sun and moon light the scene (no scene directional light)
 * -------------------------------------------------------
 */
import common;
import resources;
import constants;
import position;
import xp_material;
import environment_lighting;

[shader("compute")]
[numthreads(THREAD_GROUP_COUNT_X, THREAD_GROUP_COUNT_Y, 1)]
void main_comp(uint3 thread_id : SV_DispatchThreadID)
{
	float2 resolution_out;
	tex_uav.GetDimensions(resolution_out.x, resolution_out.y);
	if (any(int2(thread_id.xy) >= int2(resolution_out)))
		return;

	const float2 uv   = (thread_id.xy + 0.5f) / resolution_out;
	const float depth = tex_depth[thread_id.xy].r;
	const float pre_exposure = get_pre_exposure();

	// reverse-z: the depth buffer is cleared to 0, so 0 means nothing was drawn here
	if (depth <= 0.0f)
	{
		// the transparent pass runs after the opaque one, which already wrote the sky; with the atmosphere on the sky
		// pass draws it
		if (pass_is_transparent() || is_feature_enabled(FEATURE_ATMOSPHERE))
			return;

		float3 view_direction = normalize(get_position(0.5f, uv) - get_camera_position());
		float3 sky = xp_sky_radiance(view_direction) + xp_sun_moon_discs(view_direction);
		tex_uav[thread_id.xy] = validate_output(float4(sky * pre_exposure, 1.0f));
		return;
	}

	float4 gbuffer_albedo = tex_albedo[thread_id.xy];
	float4 gbuffer_normal = tex_normal[thread_id.xy];
	float4 gbuffer_mat    = tex_material[thread_id.xy]; // lighting factor, roughness, metalness / F0, occlusion

	float4 light_diffuse  = tex[thread_id.xy];
	float4 light_specular = tex2[thread_id.xy];

	XpLitSurface surface;
	surface.albedo    = gbuffer_albedo.rgb;
	surface.normal    = normalize(gbuffer_normal.xyz);
	surface.roughness = saturate(gbuffer_mat.g);
	surface.v         = saturate(gbuffer_mat.b);
	surface.occlusion = saturate(gbuffer_mat.a);
	const float3 position = get_position(depth, uv);
	surface.view = normalize(get_camera_position() - position);
	const float n_dot_v = saturate(dot(surface.normal, surface.view));
	surface.env_brdf = tex3.SampleLevel(samplers[sampler_bilinear_clamp], float2(n_dot_v, surface.roughness), 0.0f).rg;
	const float3 f0 = xp_f0(surface.albedo, surface.v);

	// The scene's own lights (already pre-exposed)
	float3 result = surface.albedo * light_diffuse.rgb * (1.0f - surface.v) + light_specular.rgb;

	// The environment
	const bool sun_and_moon = pass_get_f4_value().x > 0.5f;
	result += xp_environment_lighting(surface, f0, sun_and_moon, position) * pre_exposure;

	// Emissive luminance (X-Plane LIT textures), already pre-exposed: lights are never dimmed by daylight, exposure
	// alone hides them in the day
	result += tex_emissive[thread_id.xy].rgb;

	// The air between the camera and the surface
	result = xp_apply_aerial_perspective(result, uv, length(position - get_camera_position()), 1.0f);

	tex_uav[thread_id.xy] = validate_output(float4(result, 1.0f));
}
