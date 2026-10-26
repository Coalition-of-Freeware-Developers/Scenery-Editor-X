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

// A GGX half vector around N = +Z. Built in that frame directly (phi measured from +X, where V lies): the table is a
// finite Hammersley sum, and a frame rotated around N would sum different points (0.03 apart at grazing angles)
float3 importance_sample_ggx(float2 xi, float roughness)
{
	float a = roughness * roughness;
	float phi = 2.0f * PI * xi.x;
	float cos_theta = sqrt((1.0f - xi.y) / (1.0f + (a * a - 1.0f) * xi.y));
	float sin_theta = sqrt(max(1.0f - cos_theta * cos_theta, 0.0f));
	return float3(cos(phi) * sin_theta, sin(phi) * sin_theta, cos_theta);
}

// Height-correlated Smith visibility (Heitz 2014), V = G2 / (4 N.L N.V), alpha2 = alpha^2 = roughness^4
float visibility_smith_correlated(float n_dot_v, float n_dot_l, float alpha2)
{
	float lambda_v = n_dot_l * sqrt(n_dot_v * n_dot_v * (1.0f - alpha2) + alpha2);
	float lambda_l = n_dot_v * sqrt(n_dot_l * n_dot_l * (1.0f - alpha2) + alpha2);
	return 0.5f / max(lambda_v + lambda_l, 1e-20f);
}

// The split-sum scale (A) and bias (B) of F0 for X-Plane 12's BRDF: GGX with alpha = roughness^2, height-correlated
// Smith, Schlick Fresnel (environment/brdf_lut.cpp computes the same table; it matches X-Plane's Resources/brdf_lookup)
float2 integrate_brdf(float n_dot_v, float roughness)
{
	const uint sample_count = 1024u;
	n_dot_v = max(n_dot_v, 1e-5f);
	roughness = max(roughness, 1e-3f);
	float alpha = roughness * roughness;
	float alpha2 = alpha * alpha;
	float3 v = float3(sqrt(max(1.0f - n_dot_v * n_dot_v, 0.0f)), 0.0f, n_dot_v);
	float a = 0.0f;
	float b = 0.0f;

	for (uint i = 0u; i < sample_count; i++)
	{
		float2 xi = hammersley(i, sample_count);
		float3 h = importance_sample_ggx(xi, roughness);
		float3 l = 2.0f * dot(v, h) * h - v;

		float n_dot_l = saturate(l.z);
		float n_dot_h = saturate(h.z);
		float v_dot_h = saturate(dot(v, h));

		if (n_dot_l > 0.0f)
		{
			float g_vis = 4.0f * visibility_smith_correlated(n_dot_v, n_dot_l, alpha2) * n_dot_l * v_dot_h / max(n_dot_h, 1e-12f);
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

	// BRDF LUT integration mode: texel (x, y) holds N.V = x / size, roughness = y / size (X-Plane's brdf_lookup
	// layout); sample at uv = value + 0.5 / size
	float2 coordinates = float2(thread_id.xy) / resolution_out;
	float2 brdf = integrate_brdf(coordinates.x, coordinates.y);

	tex_uav[thread_id.xy] = validate_output(float4(brdf, 0.0f, 1.0f));
}
