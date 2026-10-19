/**
 * -------------------------------------------------------
 * Scenery Editor X
 * -------------------------------------------------------
 * Copyright (c) 2026 Thomas Ray
 * Copyright (c) 2026 Coalition of Freeware Developers
 * -------------------------------------------------------
 * light.slang
 * -------------------------------------------------------
 * Compatibility entry point for deferred direct lighting pass.
 * -------------------------------------------------------
 */
import common;
import resources;
import constants;
import structs;
import brdf;
import shadow_mapping;
import fog;

float sample_cloud_shadow(float3 world_pos)
{
	if (get_cloud_shadows_strength() <= 0.0 || get_cloud_coverage() <= 0.0)
		return 1.0;

	float shadow_map_size = 10000.0;
	float2 shadow_dims;
	tex3.GetDimensions(shadow_dims.x, shadow_dims.y);
	float texel_size = shadow_map_size / max(shadow_dims.x, 1.0);

	float2 snapped_center = floor(get_camera_position().xz / texel_size) * texel_size;
	float2 relative_pos = world_pos.xz - snapped_center;
	float2 uv = relative_pos / shadow_map_size + 0.5;

	if (any(uv < 0.0) || any(uv > 1.0))
		return 1.0;

	return tex3.SampleLevel(samplers[sampler_bilinear_clamp], uv, 0).r;
}

float3 subsurface_scattering(Surface surface, Light light, AngularInfo angular_info)
{
	const float wrap_factor = 0.5f;
	const float sss_exponent = 3.0f;
	const float thickness_exponent = 1.5f;
	const float sss_scale = 0.8f;
	const float min_scatter = 0.05f;

	float3 L = normalize(-light.to_pixel);
	float3 V = normalize(-surface.camera_to_pixel);
	float3 N = surface.normal;

	float n_dot_l_wrapped = saturate((dot(N, L) + wrap_factor) / (1.0f + wrap_factor));
	float wrapped_diffuse = n_dot_l_wrapped * n_dot_l_wrapped;

	const float distortion = 0.4f;
	float3 N_distorted = normalize(N + L * distortion);
	float back_scatter = saturate(dot(V, -N_distorted));
	back_scatter = pow(back_scatter, sss_exponent);

	float sss_term = lerp(back_scatter, wrapped_diffuse, saturate(dot(N, L) * 0.5f + 0.5f));
	sss_term = max(sss_term, min_scatter);

	float n_dot_v = saturate(dot(N, V));
	float view_thickness = pow(1.0f - n_dot_v, thickness_exponent);
	float n_dot_l = saturate(dot(N, L));
	float light_thickness = pow(1.0f - n_dot_l, 1.0f);
	float thickness_modulation = saturate(view_thickness + light_thickness * 0.5f);

	float sss_strength = surface.subsurface_scattering * sss_scale;
	return light.radiance * sss_term * thickness_modulation * sss_strength * surface.albedo;
}

[shader("compute")]
[numthreads(THREAD_GROUP_COUNT_X, THREAD_GROUP_COUNT_Y, 1)]
void main_comp(uint3 thread_id : SV_DispatchThreadID)
{
	float2 resolution_out;
	tex_uav.GetDimensions(resolution_out.x, resolution_out.y);

	Surface surface;
	surface.Build(thread_id.xy, resolution_out, true, true);

	bool early_exit_1 = pass_is_opaque() && surface.is_transparent() && !surface.is_sky();
	bool early_exit_2 = pass_is_transparent() && surface.is_opaque();
	if (early_exit_1 || early_exit_2)
		return;

	float3 out_diffuse = 0.0f;
	float3 out_specular = 0.0f;
	float3 out_volumetric = 0.0f;

	float3 light_precomputed = surface.alpha * surface.occlusion;

	uint light_count = (uint)max(pass_get_f3_value().x, 0.0);
	for (uint i = 0; i < light_count; i++)
	{
		Light light;
		light.Build(i, surface);

		float L_shadow = 1.0f;
		float3 L_specular_sum = 0.0f;
		float3 L_diffuse_term = 0.0f;
		float3 L_subsurface = 0.0f;
		float3 L_volumetric = 0.0f;

		if (!surface.is_sky())
		{
			if (light.has_shadows())
			{
				L_shadow = compute_shadow(surface, light);
				if (light.has_shadows_screen_space() && surface.is_opaque())
				{
					L_shadow = min(L_shadow, tex_uav_sss[int3(thread_id.xy, light.screen_space_shadows_slice_index)].x);
				}

				light.radiance *= L_shadow;
			}

			if (light.is_directional())
			{
				float cloud_shadow = sample_cloud_shadow(surface.position);
				L_shadow = min(L_shadow, cloud_shadow);
				light.radiance *= cloud_shadow;
			}

			AngularInfo angular_info;
			angular_info.Build(light, surface);

			float original_roughness = surface.roughness;
			float original_roughness_alpha = surface.roughness_alpha;
			if (light.is_area())
			{
				surface.roughness_alpha = light.compute_area_roughness_modification(surface.roughness_alpha, light.distance_to_pixel);
				surface.roughness = sqrt(surface.roughness_alpha);
			}

			if (surface.anisotropic > 0.0f)
			{
				L_specular_sum += BRDF_Specular_Anisotropic(surface, angular_info);
			}
			else
			{
				L_specular_sum += BRDF_Specular_Isotropic(surface, angular_info);
			}

			if (surface.clearcoat > 0.0f)
			{
				L_specular_sum += BRDF_Specular_Clearcoat(surface, angular_info);
			}

			if (surface.sheen > 0.0f)
			{
				L_specular_sum += BRDF_Specular_Sheen(surface, angular_info);
			}

			if (surface.subsurface_scattering > 0.0f)
			{
				L_subsurface += subsurface_scattering(surface, light, angular_info);
			}

			surface.roughness = original_roughness;
			surface.roughness_alpha = original_roughness_alpha;
			L_diffuse_term += BRDF_Diffuse(surface, angular_info);
		}

		if (light.is_volumetric())
		{
			L_volumetric += compute_volumetric_fog(surface, light, thread_id.xy);
		}

		float3 write_diffuse = L_diffuse_term * light.radiance * light_precomputed * surface.diffuse_energy + L_subsurface;
		float3 write_specular = L_specular_sum * light.radiance * light_precomputed;
		float3 write_volumetric = L_volumetric;

		out_diffuse += write_diffuse;
		out_specular += write_specular;
		out_volumetric += write_volumetric;
	}

	tex_uav[thread_id.xy] = validate_output(float4(out_diffuse, 1.0f));
	tex_uav2[thread_id.xy] = validate_output(float4(out_specular, 1.0f));
	tex_uav3[thread_id.xy] = validate_output(float4(out_volumetric, 1.0f));
}
