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

[shader("compute")]
[numthreads(THREAD_GROUP_COUNT_X, THREAD_GROUP_COUNT_Y, 1)]
void main_comp(uint3 thread_id : SV_DispatchThreadID)
{
	float2 resolution_out;
	tex_uav.GetDimensions(resolution_out.x, resolution_out.y);
	if (any(int2(thread_id.xy) >= int2(resolution_out)))
		return;

	float4 gbuffer_albedo = tex_albedo[thread_id.xy];
	float4 gbuffer_mat = tex_material[thread_id.xy];

	float4 light_diffuse = tex[thread_id.xy];
	float4 light_specular = tex2[thread_id.xy];

	float alpha = gbuffer_albedo.a;
	float metallic = saturate(gbuffer_mat.b);

	float3 diffuse_term = gbuffer_albedo.rgb * light_diffuse.rgb * (1.0f - metallic);
	float3 specular_term = light_specular.rgb;

	float3 result = diffuse_term + specular_term;
	tex_uav[thread_id.xy] = validate_output(float4(result, alpha));
}
