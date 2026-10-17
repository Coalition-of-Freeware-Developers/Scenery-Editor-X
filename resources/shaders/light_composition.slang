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

// Placeholder sky and sun used while the scene has no lights of its own (bootstrap renderer).
// Pass_Light_Composition pushes:
//   f3_value  (values[0].xyz) - unit direction towards the fallback sun
//   f2_value.x (values[0].w)  - 1 when the fallback lighting is active (no scene lights), else 0
static const float3 sky_zenith    = float3(0.20f, 0.40f, 0.75f);
static const float3 sky_horizon   = float3(0.62f, 0.74f, 0.88f);
static const float3 ground_color  = float3(0.30f, 0.29f, 0.27f);
static const float3 sun_color     = float3(1.00f, 0.96f, 0.90f) * 2.5f;
static const float ambient_amount = 0.35f;

float3 sky_color(float3 direction)
{
    float up = direction.y;
    float3 sky    = lerp(sky_horizon, sky_zenith, pow(saturate(up), 0.5f));
    float3 ground = lerp(sky_horizon, ground_color, saturate(-up * 6.0f));
    return up >= 0.0f ? sky : ground;
}

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
		tex_uav[thread_id.xy] = validate_output(float4(sky_color(view_direction), 1.0f));
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

	if (pass_get_f2_value().x > 0.0f)
	{
		float3 n       = normalize(gbuffer_normal.xyz);
		float3 l       = normalize(pass_get_f3_value());
		float n_dot_l  = saturate(dot(n, l));
		float3 ambient = lerp(ground_color, sky_zenith, n.y * 0.5f + 0.5f) * ambient_amount;
		result += gbuffer_albedo.rgb * (1.0f - metallic) * (sun_color * n_dot_l * INV_PI + ambient) * occlusion;
	}

	tex_uav[thread_id.xy] = validate_output(float4(result, 1.0f));
}
