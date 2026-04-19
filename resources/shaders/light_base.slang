/**
 * -------------------------------------------------------
 * Scenery Editor X
 * -------------------------------------------------------
 * Copyright (c) 2026 Thomas Ray
 * Copyright (c) 2026 Coalition of Freeware Developers
 * -------------------------------------------------------
 * light_base.slang
 * -------------------------------------------------------
 * Image-based lighting pass (skysphere + BRDF LUT).
 * -------------------------------------------------------
 */
import common;
import resources;
import constants;
import structs;

[shader("compute")]
[numthreads(THREAD_GROUP_COUNT_X, THREAD_GROUP_COUNT_Y, 1)]
void main_comp(uint3 thread_id : SV_DispatchThreadID)
{
	float2 resolution_out;
	tex_uav.GetDimensions(resolution_out.x, resolution_out.y);
	if (any(int2(thread_id.xy) >= int2(resolution_out)))
		return;

	Surface surface;
	surface.Build(thread_id.xy, resolution_out, true, false);

	float3 N = normalize(surface.normal);
	float roughness = saturate(surface.roughness);
	float metallic = saturate(surface.metallic);
	float3 albedo = surface.albedo;
	float3 F0 = surface.F0;

	float2 sky_uv = direction_sphere_uv(N);

	float2 sky_dims;
	tex.GetDimensions(sky_dims.x, sky_dims.y);
	float mip_count = max(floor(log2(max(sky_dims.x, sky_dims.y))) + 1.0f, 1.0f);
	float mip_level = roughness * roughness * (mip_count - 1.0f);

	float3 ibl_sample = tex.SampleLevel(samplers[sampler_trilinear_clamp], sky_uv, mip_level).rgb;

	float3 v = normalize(-surface.camera_to_pixel);
	float n_dot_v = saturate(dot(N, v));
	float3 F_ibl = F0 + (max(1.0f - roughness, F0) - F0) * pow(1.0f - n_dot_v, 5.0f);

	float3 ibl_diffuse = albedo * ibl_sample * (1.0f - metallic) * (1.0f - F_ibl) * 0.3f;
	float3 ibl_specular = ibl_sample * F_ibl * 0.2f;

	float4 current = tex_uav[thread_id.xy];
	float3 result = current.rgb + ibl_diffuse + ibl_specular;
	tex_uav[thread_id.xy] = validate_output(float4(result, current.a));
}
