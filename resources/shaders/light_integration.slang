/**
 * -------------------------------------------------------
 * Scenery Editor X
 * -------------------------------------------------------
 * Copyright (c) 2026 Thomas Ray
 * Copyright (c) 2026 Coalition of Freeware Developers
 * -------------------------------------------------------
 * light_integration.slang
 * -------------------------------------------------------
 * Shared integration shader:
 *  - BRDF split-sum LUT generation
 *  - Skysphere environment mip filtering
 * Selection is driven by pass constants.
 * -------------------------------------------------------
 */
import common;
import resources;
import constants;

float radical_inverse_vdc(uint bits)
{
	bits = (bits << 16u) | (bits >> 16u);
	bits = ((bits & 0x55555555u) << 1u) | ((bits & 0xAAAAAAAAu) >> 1u);
	bits = ((bits & 0x33333333u) << 2u) | ((bits & 0xCCCCCCCCu) >> 2u);
	bits = ((bits & 0x0F0F0F0Fu) << 4u) | ((bits & 0xF0F0F0F0u) >> 4u);
	bits = ((bits & 0x00FF00FFu) << 8u) | ((bits & 0xFF00FF00u) >> 8u);
	return float(bits) * 2.3283064365386963e-10f;
}

float2 hammersley(uint i, uint n)
{
	return float2(float(i) / float(n), radical_inverse_vdc(i));
}

float3 importance_sample_ggx(float2 xi, float roughness, float3 n)
{
	float a = roughness * roughness;
	float phi = 2.0f * PI * xi.x;
	float cos_theta = sqrt((1.0f - xi.y) / (1.0f + (a * a - 1.0f) * xi.y));
	float sin_theta = sqrt(max(1.0f - cos_theta * cos_theta, 0.0f));

	float3 h;
	h.x = cos(phi) * sin_theta;
	h.y = sin(phi) * sin_theta;
	h.z = cos_theta;

	float3 up = abs(n.z) < 0.999f ? float3(0.0f, 0.0f, 1.0f) : float3(1.0f, 0.0f, 0.0f);
	float3 tangent = normalize(cross(up, n));
	float3 bitangent = cross(n, tangent);

	return normalize(tangent * h.x + bitangent * h.y + n * h.z);
}

float geometry_schlick_ggx(float n_dot_v, float roughness)
{
	float r = roughness + 1.0f;
	float k = (r * r) / 8.0f;
	return n_dot_v / max(n_dot_v * (1.0f - k) + k, 1e-5f);
}

float geometry_smith(float n_dot_v, float n_dot_l, float roughness)
{
	return geometry_schlick_ggx(n_dot_v, roughness) * geometry_schlick_ggx(n_dot_l, roughness);
}

float2 integrate_brdf(float n_dot_v, float roughness)
{
	const uint sample_count = 1024u;
	float3 v = float3(sqrt(max(1.0f - n_dot_v * n_dot_v, 0.0f)), 0.0f, n_dot_v);
	float a = 0.0f;
	float b = 0.0f;

	float3 n = float3(0.0f, 0.0f, 1.0f);
	for (uint i = 0u; i < sample_count; i++)
	{
		float2 xi = hammersley(i, sample_count);
		float3 h = importance_sample_ggx(xi, roughness, n);
		float3 l = normalize(2.0f * dot(v, h) * h - v);

		float n_dot_l = saturate(l.z);
		float n_dot_h = saturate(h.z);
		float v_dot_h = saturate(dot(v, h));

		if (n_dot_l > 0.0f)
		{
			float g = geometry_smith(n_dot_v, n_dot_l, roughness);
			float g_vis = (g * v_dot_h) / max(n_dot_h * n_dot_v, 1e-5f);
			float fc = pow(1.0f - v_dot_h, 5.0f);

			a += (1.0f - fc) * g_vis;
			b += fc * g_vis;
		}
	}

	float inv_count = 1.0f / float(sample_count);
	return float2(a * inv_count, b * inv_count);
}

float3 filter_environment(float2 uv, float roughness)
{
	float2 dims;
	tex.GetDimensions(dims.x, dims.y);
	float2 texel = 1.0f / max(dims, 1.0f);

	float2 offs = texel * lerp(0.5f, 8.0f, roughness);

	float3 c0 = tex.SampleLevel(samplers[sampler_trilinear_clamp], uv, 0).rgb;
	float3 c1 = tex.SampleLevel(samplers[sampler_trilinear_clamp], uv + float2(offs.x, 0.0f), 0).rgb;
	float3 c2 = tex.SampleLevel(samplers[sampler_trilinear_clamp], uv - float2(offs.x, 0.0f), 0).rgb;
	float3 c3 = tex.SampleLevel(samplers[sampler_trilinear_clamp], uv + float2(0.0f, offs.y), 0).rgb;
	float3 c4 = tex.SampleLevel(samplers[sampler_trilinear_clamp], uv - float2(0.0f, offs.y), 0).rgb;

	return (c0 * 0.5f) + ((c1 + c2 + c3 + c4) * 0.125f);
}

[shader("compute")]
[numthreads(THREAD_GROUP_COUNT_X, THREAD_GROUP_COUNT_Y, 1)]
void main_comp(uint3 thread_id : SV_DispatchThreadID)
{
	float2 resolution_out;
	tex_uav.GetDimensions(resolution_out.x, resolution_out.y);
	if (any(int2(thread_id.xy) >= int2(resolution_out)))
		return;

	float3 pass_values = pass_get_f3_value();

	// Environment filter mode: called from skysphere mip filtering pass.
	if (pass_values.y > 1.0f)
	{
		float2 uv = (float2(thread_id.xy) + 0.5f) / resolution_out;
		float roughness = saturate(pass_values.x / max(pass_values.y - 1.0f, 1.0f));
		float3 filtered = filter_environment(uv, roughness);
		tex_uav[thread_id.xy] = validate_output(float4(filtered, 1.0f));
		return;
	}

	// BRDF LUT integration mode.
	float2 uv = (float2(thread_id.xy) + 0.5f) / resolution_out;
	float n_dot_v = saturate(uv.x);
	float roughness = saturate(uv.y);
	float2 brdf = integrate_brdf(n_dot_v, roughness);

	tex_uav[thread_id.xy] = validate_output(float4(brdf, 0.0f, 1.0f));
}
