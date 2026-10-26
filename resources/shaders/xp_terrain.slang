/**
 * -------------------------------------------------------
 * Scenery Editor X
 * -------------------------------------------------------
 * Copyright (c) 2026 Thomas Ray
 * Copyright (c) 2026 Coalition of Freeware Developers
 * -------------------------------------------------------
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 * -------------------------------------------------------
 * xp_terrain.slang
 * -------------------------------------------------------
 * Created: 22/10/2026
 * -------------------------------------------------------
 *
 * X-Plane .ter land classes on the DSF base mesh (XP12: the terrain family's g-buffer writers), for gbuffer.slang:
 *   - Textures are projected from above: PROJECTED meters per repeat east (s) and north (t) of the scene's origin.
 *   - COMPOSITE_TEX mixes in by its COMPOSITE_NOISE mask; AUTO_SLOPE_HILL / _CLIFF take over on steep ground; FAR swaps
 *     to a coarser texture far away; TEXTURE_TILE atlases pick tiles per cell (xp_drape_tile_uv).
 *   - Border (overlay) patches fade in by BORDER_TEX at the patch's s / t; with COMPOSITE_BORDERS the fade is a
 *     dithered cut through the noise, not a blend, so it writes the g-buffer like any opaque surface.
 *   - Decals (DECAL_LIB rows) repeat per meter like the draped polygons' (xp_drape_*); LIT textures are nits.
 * COMPOSITE_PARAMS, the noise scale and the LIT / FAR fades aren't documented: the values used are ASSUMPTIONS until
 * Phase 16 calibrates them against X-Plane (plan feature-xp12-environment-rendering-1.md, Phase 10).
 *
 * Texture slots: color 0 base, 1 tile map, 2-3 detail decals; normal 0 normal map, 1-2 normal decals; roughness 0
 * composite, 1 hill, 2 cliff, 3 composite noise; metalness 0 border, 1 far; emission 0 lit.
 */
module xp_terrain;

import common;
import resources;
import color;
import xp_material;

public static const uint xp_terrain_border           = 1u << 0;	// Overlay patch: faded in by the border mask
public static const uint xp_terrain_composite_border = 1u << 1;	// COMPOSITE_BORDERS: the fade is a dithered cut
public static const uint xp_terrain_composite        = 1u << 2;	// COMPOSITE_TEX
public static const uint xp_terrain_hill             = 1u << 3;	// AUTO_SLOPE_HILL
public static const uint xp_terrain_cliff            = 1u << 4;	// AUTO_SLOPE_CLIFF
public static const uint xp_terrain_lit              = 1u << 5;	// TEXTURE_LIT / LIT_TEX
public static const uint xp_terrain_far              = 1u << 6;	// FAR
public static const uint xp_terrain_tiled            = 1u << 7;	// TEXTURE_TILE
public static const uint xp_terrain_noise            = 1u << 8;	// COMPOSITE_NOISE present

static const uint slot_composite = material_texture_index_roughness + 0;
static const uint slot_hill      = material_texture_index_roughness + 1;
static const uint slot_cliff     = material_texture_index_roughness + 2;
static const uint slot_noise     = material_texture_index_roughness + 3;
static const uint slot_border    = material_texture_index_metalness + 0;
static const uint slot_far       = material_texture_index_metalness + 1;

static const float noise_scale = 2.0f;		// ASSUMPTION: the noise covers two composite repeats

/* @brief What the terrain gives the g-buffer writer. */
public struct XpTerrainSurface
{
    public float3 albedo;			// Linear
    public float4 normal_texel;		// The normal map's texel (0.5, 0.5, 1, 1 without one)
    public bool has_normal_map;
    public float3 emissive;			// Nits x pre-exposure
    public bool cut;				// A border pixel outside its fade: discard
};

public bool xp_is_terrain(MaterialParameters material)
{
    return material.is_xp_material() && (material.flags & (1u << 8)) != 0;
}

uint terrain_flags(MaterialParameters material)
{
    return uint(material.terrain_base.w + 0.5f);
}

/* @brief Ground-plane coordinates in meters: east, north. */
float2 ground_meters(float3 position_world)
{
    return float2(position_world.x, -position_world.z);
}

float4 sample_wrap(uint material_base, uint slot, float2 uv)
{
    return material_textures[material_base + slot].Sample(samplers[sampler_anisotropic_wrap], uv);
}

float3 sample_albedo(uint material_base, uint slot, float2 uv)
{
    return srgb_to_linear(sample_wrap(material_base, slot, uv).rgb);
}

/**
 * @brief The terrain's albedo, normal map texel and emissive at a pixel.
 * @param normal The interpolated vertex normal (slope tests), world space.
 * @param border_uv The patch's border s / t.
 * @param distance From the camera, meters.
 */
public XpTerrainSurface xp_terrain_shade(MaterialParameters material, uint material_base, float3 position_world, float3 normal, float2 border_uv, float distance)
{
    XpTerrainSurface surface;
    surface.cut = false;
    surface.emissive = 0.0f;

    const uint flags = terrain_flags(material);
    const float2 meters = ground_meters(position_world);
    const float2 base_scale = max(material.terrain_base.xy, 1e-3f);
    const float2 base_uv = meters / base_scale;

    // Base texture (a tile atlas picks a tile per projected repeat); its alpha keys the decals
    float3 albedo;
    float base_alpha = 1.0f;
    if ((flags & xp_terrain_tiled) != 0)
    {
        const float2 atlas_uv = xp_drape_tile_uv(material, material_base, base_uv / max(material.drape_tiles.xy, 1.0f));
        albedo = srgb_to_linear(material_textures[material_base + material_texture_index_albedo].SampleGrad(samplers[sampler_anisotropic_wrap], atlas_uv, ddx(base_uv), ddy(base_uv)).rgb);
    }
    else
    {
        const float4 texel = sample_wrap(material_base, material_texture_index_albedo, base_uv);
        albedo = srgb_to_linear(texel.rgb);
        base_alpha = texel.a;
    }

    // Composite: a second texture through the noise mask
    float noise = 0.5f;
    if ((flags & xp_terrain_noise) != 0)
    {
        const float2 noise_uv = meters / (max(material.terrain_composite.xy, 1e-3f) * noise_scale);
        noise = sample_wrap(material_base, slot_noise, noise_uv).r;
    }
    if ((flags & xp_terrain_composite) != 0)
    {
        const float3 composite = sample_albedo(material_base, slot_composite, meters / max(material.terrain_composite.xy, 1e-3f));
        const float center = material.terrain_composite.z, width = max(material.terrain_composite.w, 1e-3f);
        albedo = lerp(albedo, composite, smoothstep(center - width, center + width, noise));
    }

    // Slopes: hill, then cliff
    const float slope = degrees(acos(clamp(normalize(normal).y, -1.0f, 1.0f)));
    if ((flags & xp_terrain_hill) != 0)
    {
        const float weight = smoothstep(material.terrain_slope.x, max(material.terrain_slope.y, material.terrain_slope.x + 0.01f), slope);
        if (weight > 0.0f)
            albedo = lerp(albedo, sample_albedo(material_base, slot_hill, meters / max(material.terrain_slope_scale.xy, 1e-3f)), weight);
    }
    if ((flags & xp_terrain_cliff) != 0)
    {
        const float weight = smoothstep(material.terrain_slope.z, max(material.terrain_slope.w, material.terrain_slope.z + 0.01f), slope);
        if (weight > 0.0f)
            albedo = lerp(albedo, sample_albedo(material_base, slot_cliff, meters / max(material.terrain_slope_scale.zw, 1e-3f)), weight);
    }

    // Far texture with distance (ASSUMPTION: a linear blend across FAR's range)
    if ((flags & xp_terrain_far) != 0)
    {
        const float weight = saturate((distance - material.terrain_far.z) / max(material.terrain_far.w - material.terrain_far.z, 1.0f));
        if (weight > 0.0f)
            albedo = lerp(albedo, sample_albedo(material_base, slot_far, meters / max(material.terrain_far.xy, 1e-3f)), weight);
    }

    // Detail decals, per meter of ground
    albedo = xp_drape_apply_detail_decals(material, material_base, meters, float4(albedo, base_alpha));

    // Border patches: in where the mask (luminance) reaches past the noise (COMPOSITE_BORDERS), else at half
    if ((flags & xp_terrain_border) != 0)
    {
        const float mask = material_textures[material_base + slot_border].Sample(samplers[sampler_bilinear_clamp], border_uv).r;
        float threshold = 0.5f;
        if ((flags & xp_terrain_composite_border) != 0)
        {
            // Without a noise texture a stable hash of the ground position, in 25 cm cells
            threshold = (flags & xp_terrain_noise) != 0 ? noise : frac(sin(dot(floor(meters * 4.0f), float2(12.9898f, 78.233f))) * 43758.5453f);
        }
        surface.cut = mask <= threshold;
    }

    surface.albedo = albedo;

    // Normal map: repeats ratio times per base repeat
    surface.has_normal_map = material.has_texture_normal();
    surface.normal_texel = surface.has_normal_map ? sample_wrap(material_base, material_texture_index_normal, base_uv * max(material.terrain_base.z, 1e-3f)) : float4(0.5f, 0.5f, 1.0f, 1.0f);

    // Night lighting: nits, faded in with distance when LIT_FADE_PARAMS says so (ASSUMPTION: a smoothstep across its range)
    if ((flags & xp_terrain_lit) != 0 && material.xp.w > 0.0f)
    {
        float fade = 1.0f;
        if (material.terrain_lit.w > material.terrain_lit.z)
            fade = smoothstep(material.terrain_lit.z, material.terrain_lit.w, distance);
        if (fade > 0.0f)
        {
            const float3 lit = srgb_to_linear(material_textures[material_base + material_texture_index_emission].Sample(samplers[sampler_anisotropic_wrap], meters / max(material.terrain_lit.xy, 1e-3f)).rgb);
            surface.emissive = lit * xp_emissive_scale(material) * fade;
        }
    }
    return surface;
}
