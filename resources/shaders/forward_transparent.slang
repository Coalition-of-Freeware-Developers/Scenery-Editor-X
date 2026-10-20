/**
 * -------------------------------------------------------
 * Scenery Editor X
 * -------------------------------------------------------
 * Copyright (c) 2026 Thomas Ray
 * Copyright (c) 2026 Coalition of Freeware Developers
 * -------------------------------------------------------
 * forward_transparent.slang
 * -------------------------------------------------------
 * Alpha-blended surfaces, drawn back to front over the composed frame (Renderer::Pass_Forward_Transparent).
 *
 * A blended material is split by its alpha cutoff: texels at or above it went through the deferred passes
 * (depth_prepass.slang cuts the rest out), this pass draws the ones below it. Output is premultiplied:
 *   blend_mode 1 (X-Plane ATTR_blend): (diffuse + specular) * alpha
 *   blend_mode 2 (X-Plane BLEND_GLASS): diffuse * alpha + specular, so reflections stay on clear glass
 * Lighting is the placeholder sun and sky (fallback_lighting.slang) the deferred composition uses.
 * -------------------------------------------------------
 */
import constants;
import common;
import vertex_processing;
import resources;
import color;
import luminance;
import position;
import fallback_lighting;
import xp_material;

[shader("vertex")]
gbuffer_vertex main_vs(Vertex_PosUvNorTan input, uint instance_id: SV_InstanceID)
{
    _draw = draw_data[buffer_pass.draw_index];

    float3 position_world = 0.0f;
    float3 position_world_previous = 0.0f;
    gbuffer_vertex vertex = transform_to_world_space(input, instance_id, _draw.transform, position_world, position_world_previous);
    vertex.material_index = _draw.material_index;
    return apply_drape_depth(transform_to_clip_space(vertex, position_world, position_world_previous), _draw.drape_rank);
}

static float4 sample_material_texture(uint texture_index, float2 uv)
{
    return material_textures[pass_get_material_index() + texture_index].Sample(samplers[sampler_anisotropic_wrap], uv);
}

[shader("fragment")]
float4 main_frag(gbuffer_vertex vertex, bool is_front_face: SV_IsFrontFace) : SV_TARGET
{
    pass_load_draw_data_from_vertex(vertex.material_index);
    MaterialParameters material = GetMaterial();

    // albedo
    float4 albedo = material.color;
    if (material.has_texture_albedo())
    {
        float4 albedo_sample = sample_material_texture(material_texture_index_albedo, vertex.uv_misc.xy);
        albedo_sample.rgb = srgb_to_linear(albedo_sample.rgb);
        albedo *= albedo_sample;
    }

    // texels at or above the cutoff were drawn opaque by the deferred passes
    const float cutoff = material.alpha_cutoff > 0.0f ? material.alpha_cutoff : 1.0f;
    const bool glass = material.blend_mode > 1.5f;
    if (albedo.a >= cutoff || (!glass && albedo.a < 1.0f / 255.0f))
        discard;

    // normal, roughness and metalness / F0 (xp_material.slang); two-sided: back faces of no-cull glass face the camera too
    const float4 normal_texel = material.has_texture_normal() ? sample_material_texture(material_texture_index_normal, vertex.uv_misc.xy) : float4(0.5f, 0.5f, 1.0f, 1.0f);
    const XpSurfaceMaterial surface_material = xp_decode_material(material, normal_texel);
    float3 normal = normalize(vertex.normal);
    if (material.has_texture_normal())
        normal = normalize(mul(surface_material.tangent_normal, make_tangent_to_world_matrix(normalize(vertex.normal), normalize(vertex.tangent))).xyz);
    if (!is_front_face)
        normal = -normal;

    float roughness = clamp(surface_material.roughness, 0.04f, 1.0f);
    float metalness = surface_material.v;

    // view
    const float2 screen_uv = vertex.position.xy / (get_resolution_render() * get_resolution_scale());
    float3 position_world = get_position(vertex.position.z, screen_uv);
    float3 v = normalize(get_camera_position() - position_world);
    float n_dot_v = saturate(dot(normal, v));

    // diffuse: placeholder sun + ambient, emission (LIT) on top
    float3 diffuse = 0.0f;
    float3 specular = 0.0f;
    if (fallback_lighting_active())
    {
        float3 l = fallback_sun_direction();
        float n_dot_l = saturate(dot(normal, l));
        diffuse = albedo.rgb * (1.0f - metalness) * (fallback_sun_color() * n_dot_l * INV_PI + fallback_ambient(normal));

        // sun highlight (normalized Blinn-Phong) and sky reflection, both weighted by Schlick's fresnel
        float3 f0 = xp_f0(albedo.rgb, metalness);
        float3 fresnel = f0 + (1.0f - f0) * pow(1.0f - n_dot_v, 5.0f);
        float3 h = normalize(l + v);
        float shininess = 2.0f / max(roughness * roughness * roughness * roughness, 1e-4f) - 2.0f;
        float highlight = pow(saturate(dot(normal, h)), shininess) * (shininess + 8.0f) / (8.0f * PI);
        specular = fresnel * fallback_sun_color() * highlight * n_dot_l;
        specular += fresnel * fallback_sky_color(reflect(-v, normal)) * (1.0f - roughness);
    }
    else
    {
        diffuse = albedo.rgb * 0.05f; // the scene's own lights only reach the deferred surfaces
    }

    // emissive luminance (LIT), pre-exposed like the deferred surfaces' gbuffer_emissive
    if (material.has_texture_emissive())
        diffuse += srgb_to_linear(sample_material_texture(material_texture_index_emission, vertex.uv_misc.xy).rgb) * xp_emissive_scale(material);

    float alpha = saturate(albedo.a);
    float3 color = glass ? diffuse * alpha + specular : (diffuse + specular) * alpha;
    return validate_output(float4(color, alpha));
}
