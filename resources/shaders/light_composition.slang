/**
 * -------------------------------------------------------
 * Scenery Editor X
 * -------------------------------------------------------
 * Copyright (c) 2026 Thomas Ray
 * Copyright (c) 2026 Coalition of Freeware Developers
 * -------------------------------------------------------
 * light_composition.slang
 * -------------------------------------------------------
 * Combines deferred diffuse/specular lighting into frame output.
 * -------------------------------------------------------
 */
import common;
import resources;
import constants;
import position;
import fallback_lighting;

// The placeholder sky and sun (fallback_lighting.slang) light the scene while it has no directional light of its own.

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

	// reverse-z: the depth buffer is cleared to 0, so 0 means nothing was drawn here
	if (depth <= 0.0f)
	{
		// the transparent pass runs after the opaque one, which already wrote the sky
		if (pass_is_transparent())
			return;

		float3 view_direction = normalize(get_position(0.5f, uv) - get_camera_position());
		tex_uav[thread_id.xy] = validate_output(float4(fallback_sky_color(view_direction), 1.0f));
		return;
	}

	float4 gbuffer_albedo = tex_albedo[thread_id.xy];
	float4 gbuffer_normal = tex_normal[thread_id.xy];
	float4 gbuffer_mat    = tex_material[thread_id.xy]; // roughness, metalness, emission, occlusion

	float4 light_diffuse  = tex[thread_id.xy];
	float4 light_specular = tex2[thread_id.xy];

	float metallic  = saturate(gbuffer_mat.g);
	float occlusion = saturate(gbuffer_mat.a);

	float3 diffuse_term  = gbuffer_albedo.rgb * light_diffuse.rgb * (1.0f - metallic);
	float3 specular_term = light_specular.rgb;
	float3 result        = diffuse_term + specular_term;

	if (fallback_lighting_active())
	{
		float3 n       = normalize(gbuffer_normal.xyz);
		float n_dot_l  = saturate(dot(n, fallback_sun_direction()));
		float3 ambient = fallback_ambient(n);
		result += gbuffer_albedo.rgb * (1.0f - metallic) * (fallback_sun_color() * n_dot_l * INV_PI + ambient) * occlusion;
	}

	// gbuffer.slang adds emissive texels (LIT textures) to albedo and stores their luminance in material.b: let them
	// glow without light, otherwise lit windows would go dark with the sun
	result += gbuffer_albedo.rgb * saturate(gbuffer_mat.b * 2.0f);

	tex_uav[thread_id.xy] = validate_output(float4(result, 1.0f));
}
