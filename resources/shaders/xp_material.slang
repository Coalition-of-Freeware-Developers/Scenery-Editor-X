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
 * xp_material.slang
 * -------------------------------------------------------
 * Created: 20/10/2026
 * -------------------------------------------------------
 *
 * X-Plane 12 material rules (docs/xplane/xp12_render_reference.md, "Materials"), shared by the g-buffer and the
 * forward transparent pass:
 *   - The normal map carries the normal in RG (Z rebuilt), gloss in A and, by mode, F0 / metalness (NORMAL_METALNESS)
 *     or translucency (NORMAL_TRANSLUCENCY) in B.
 *   - Perceptual roughness = 1 - 0.96875 gloss; gloss is scaled by GLOBAL_specular / ATTR_shiny_rat.
 *   - One value v carries metalness and F0: F0 = lerp(min(v, 0.08), albedo, v), diffuse x (1 - v).
 *   - Emissive (LIT) texels are luminance in nits, pre-exposed like the rest of the frame.
 * The g-buffer stores perceptual roughness in material.g and v in material.b.
 */
module xp_material;

import common;
import resources;
import color;

public static const uint xp_normal_none         = 0;	// No normal map
public static const uint xp_normal_legacy       = 1;	// RG normal, A gloss, B unused
public static const uint xp_normal_metalness    = 2;	// NORMAL_METALNESS: B = F0 / metalness
public static const uint xp_normal_translucency = 3;	// NORMAL_TRANSLUCENCY: B = translucency (vegetation)

/* @brief A surface's material values after the X-Plane rules (or the editor's own, for non X-Plane materials). */
public struct XpSurfaceMaterial
{
    public float3 tangent_normal;	// Tangent-space normal (0, 0, 1 without a normal map)
    public float roughness;			// Perceptual roughness
    public float v;					// Metalness / F0 value
    public float translucency;		// NORMAL_TRANSLUCENCY only, otherwise 0
};

/* @brief Perceptual roughness from X-Plane gloss: the smoothest surface is 1/32, not 0. */
public float xp_roughness_from_gloss(float gloss)
{
    return 1.0f - 0.96875f * saturate(gloss);
}

/* @brief F0 from the albedo and the combined metalness / F0 value. */
public float3 xp_f0(float3 albedo, float v)
{
    return lerp(min(v, 0.08f).xxx, albedo, saturate(v));
}

/**
 * @brief Decodes a material's normal / roughness / metalness.
 * @param normal_texel The normal map texel as stored (0-1 per channel); ignored without a normal map.
 */
public XpSurfaceMaterial xp_decode_material(MaterialParameters material, float4 normal_texel)
{
    XpSurfaceMaterial result;
    result.tangent_normal = float3(0.0f, 0.0f, 1.0f);
    result.translucency = 0.0f;

    const bool has_normal = material.has_texture_normal();
    if (material.is_xp_material())
    {
        // material.xp: x specular gain, y super roughness floor, z F0 when B carries none, w emissive nits
        float gloss = material.xp.x;
        result.v = material.xp.z;
        if (has_normal)
        {
            result.tangent_normal.xy = normal_texel.xy * 2.0f - 1.0f;
            gloss *= normal_texel.a;
            const uint mode = material.xp_normal_mode();
            if (mode == xp_normal_metalness)
                result.v = normal_texel.b;
            else if (mode == xp_normal_translucency)
                result.translucency = normal_texel.b;
        }
        result.roughness = max(xp_roughness_from_gloss(gloss), material.xp.y);
    }
    else
    {
        // The editor's own materials: a tangent normal map and scalar roughness / metalness (F0 0.04 for dielectrics)
        if (has_normal)
            result.tangent_normal.xy = normalize(unpack(normal_texel.xyz)).xy;
        result.roughness = material.roughness;
        result.v = max(material.metallness, 0.04f);
    }

    result.tangent_normal.xy *= saturate(max(0.01f, material.normal));
    result.tangent_normal.z = sqrt(max(0.0f, 1.0f - dot(result.tangent_normal.xy, result.tangent_normal.xy)));
    return result;
}

/**
 * @brief What emissive texels are multiplied by: luminance (nits) x pre-exposure for X-Plane materials; other materials
 * keep their texels as display values.
 */
public float xp_emissive_scale(MaterialParameters material)
{
    return material.xp.w > 0.0f ? material.xp.w * get_pre_exposure() : 1.0f;
}

// -------------------------------------------------------
// Draped polygons (.pol): X-Plane 12 runway / taxiway art
//
//   - RUNWAY_TILE: the albedo is an atlas of tiles x tiles; the mesh's UVs span the whole atlas (SCALE meters), and each
//     tile-sized cell shows the tile its texel of the mapping texture (color slot 1) names: R = column, G = row, both
//     in steps of 256 / tiles. The mapping repeats every drape_tiles.zw cells; without one, cells pick at random.
//   - RUNWAY_MARKINGS: the albedo is a marking atlas whose R is the paint and G, where it reaches past R, a dark border
//     (drape_misc: paint rgb, border opacity).
//   - Detail decals (color slots 2, 3) are grey-centred and modulate the albedo 2x; normal decals (normal slots 1, 2) add
//     their RG slopes. Both repeat per meter of the surface, so their scale is independent of the atlas.

public static const uint xp_drape_plain    = 0;
public static const uint xp_drape_tiled    = 1;
public static const uint xp_drape_markings = 2;

static const uint xp_drape_slot_tile_map     = material_texture_index_albedo + 1;
static const uint xp_drape_slot_decal        = material_texture_index_albedo + 2;
static const uint xp_drape_slot_normal_decal = material_texture_index_normal + 1;

float xp_drape_hash(float2 cell)
{
    uint2 v = uint2(int2(cell) + 0x8000);
    uint h = v.x * 0x8da6b343u ^ v.y * 0xd8163841u;
    h ^= h >> 13;
    h *= 0x5bd1e995u;
    h ^= h >> 15;
    return float(h & 0xffffu) / 65536.0f;
}

/**
 * @brief Atlas UV for a tile atlas: the cell's tile (from the mapping texture, else random) at the cell's position.
 * @param material_base pass_get_material_index() of the draw.
 */
public float2 xp_drape_tile_uv(MaterialParameters material, uint material_base, float2 uv)
{
    const float2 tiles = max(material.drape_tiles.xy, 1.0f);
    const float2 position = uv * tiles;			// in tiles
    const float2 cell = floor(position);
    float2 tile;
    if (material.drape_tiles.z > 0.5f && material.drape_tiles.w > 0.5f)
    {
        const float2 map_size = material.drape_tiles.zw;
        const float2 texel = cell - map_size * floor(cell / map_size);
        const float4 choice = material_textures[material_base + xp_drape_slot_tile_map].SampleLevel(samplers[sampler_point_wrap], (texel + 0.5f) / map_size, 0.0f);
        tile = round(choice.xy * (255.0f / 256.0f) * tiles);	// 8-bit steps of 256 / tiles (63 reads as 64)
    }
    else
    {
        tile = floor(float2(xp_drape_hash(cell), xp_drape_hash(cell + 7919.0f)) * tiles);
    }
    tile = clamp(tile, 0.0f, tiles - 1.0f);
    return (tile + (position - cell)) / tiles;
}

/**
 * @brief The albedo texel of a draped material as the g-buffer / forward / depth passes use it (rgb in the texture's
 * encoding, a = coverage): tile atlases pick their cell's tile (sampled with the surface's own gradients, so cell edges
 * keep their mip), marking atlases decode paint and border.
 */
public float4 xp_drape_sample_albedo(MaterialParameters material, uint material_base, float2 uv)
{
    Texture2D albedo = material_textures[material_base + material_texture_index_albedo];
    const uint mode = material.xp_drape_mode();
    if (mode == xp_drape_tiled)
        return albedo.SampleGrad(samplers[sampler_anisotropic_wrap], xp_drape_tile_uv(material, material_base, uv), ddx(uv), ddy(uv));

    const float4 texel = albedo.Sample(samplers[sampler_anisotropic_wrap], uv);
    if (mode == xp_drape_markings)
    {
        const float paint = texel.r;
        const float border = saturate(texel.g - texel.r) * material.drape_misc.a;
        const float coverage = paint + (1.0f - paint) * border;
        const float3 color = coverage > 1e-4f ? material.drape_misc.rgb * (paint / coverage) : 0.0f;
        return float4(color, coverage);
    }
    return texel;
}

/**
 * @brief The normal-map texel of an X-Plane material: TEXTURE_NORMAL's ratio over the albedo's coordinates (terrain_base.z
 * for drapes; 0, as OBJ8s leave it, means 1), and in tile atlases the same tile as the albedo.
 */
public float4 xp_drape_sample_normal(MaterialParameters material, uint material_base, float2 uv)
{
    Texture2D normal_map = material_textures[material_base + material_texture_index_normal];
    if (material.xp_drape_mode() == xp_drape_tiled)
        return normal_map.SampleGrad(samplers[sampler_anisotropic_wrap], xp_drape_tile_uv(material, material_base, uv), ddx(uv), ddy(uv));
    const float ratio = material.terrain_base.z > 0.0f ? material.terrain_base.z : 1.0f;
    return normal_map.Sample(samplers[sampler_anisotropic_wrap], uv * ratio);
}

/**
 * @brief Forests with 3D trees (XPlane::Forest3d): the billboards draw from terrain_base.x meters from the camera on, the
 * 3D meshes nearer, the hand-over dithered over a tenth of that distance so neither pops. True when this fragment is on
 * the other side (discard it). The same test in the g-buffer, depth and shadow passes keeps each tree drawn once.
 */
public bool xp_lod_switch_discard(MaterialParameters material, float3 position_world, float2 pixel)
{
    const uint mode = material.xp_lod_switch();
    if (mode == 0u)
        return false;
    const float edge = max(material.terrain_base.x, 1.0f);
    const float band = 0.1f * edge;
    const float far_side = saturate((length(position_world - get_camera_position()) - (edge - 0.5f * band)) / band);
    const float dither = frac(52.9829189f * frac(dot(floor(pixel), float2(0.06711056f, 0.00583715f))));
    return mode == 1u ? far_side < dither : far_side >= dither;
}

/* @brief UVs in meters (decals repeat per meter). */
float2 xp_drape_meters(MaterialParameters material, float2 uv)
{
    return uv * max(material.drape_misc.zw, 1e-3f);
}

/* @brief Albedo (linear) after the detail decals: 2x modulation by grey-centred textures, weighted by strength. */
public float3 xp_drape_apply_detail_decals(MaterialParameters material, uint material_base, float2 uv, float4 albedo)
{
    if (material.xp_drape_mode() == xp_drape_markings || material.drape_decal.x <= 0.0f)
        return albedo.rgb;
    const float2 meters = xp_drape_meters(material, uv) * float2(1.0f, max(material.drape_misc.x, 1e-3f));
    // X-Plane 12 works on the texels as stored (sRGB-encoded, keys included) and converts to linear after the decals
    float4 base = float4(linear_to_srgb(albedo.rgb), albedo.a);
    [unroll]
    for (uint i = 0; i < 2; i++)
    {
        const float scale = i == 0 ? material.drape_decal.x : material.drape_decal.z;
        if (scale <= 0.0f)
            continue;
        // X-Plane 12's decals, as strongly as their keys give for this texel (the previous decal's result for the second):
        // the RGB adds the decal's detail (its texel less its average: the coarsest mip), so the mean colour stays; the
        // alpha then lightens (above 0.5) or darkens (below) like an overlay
        Texture2D decal_texture = material_textures[material_base + xp_drape_slot_decal + i];
        const float2 decal_uv = meters * scale;
        const float4 decal = decal_texture.Sample(samplers[sampler_anisotropic_wrap], decal_uv);
        const float3 average = decal_texture.SampleLevel(samplers[sampler_anisotropic_wrap], decal_uv, 16.0f).rgb;
        const float rgb_amount = saturate(dot(base, material.drape_decal_key[i * 2]) + (i == 0 ? material.drape_decal_bias.x : material.drape_decal_bias.z));
        const float alpha_amount = saturate(dot(base, material.drape_decal_key[i * 2 + 1]) + (i == 0 ? material.drape_decal_bias.y : material.drape_decal_bias.w));
        float3 color = base.rgb + (decal.rgb - average) * rgb_amount;
        const float modulator = lerp(0.5f, decal.a, alpha_amount);
        color = modulator > 0.5f ? 1.0f - (1.0f - color) * (2.0f - 2.0f * modulator) : color * (2.0f * modulator);
        base.rgb = saturate(color);
    }
    return srgb_to_linear(base.rgb);
}

/* @brief Tangent-space normal from the normal decals ((0, 0, 1) without any). */
public float3 xp_drape_normal_decals(MaterialParameters material, uint material_base, float2 uv)
{
    float2 slope = 0.0f;
    if (material.xp_drape_mode() != xp_drape_markings && material.drape_normal_decal.x > 0.0f)
    {
        const float2 meters = xp_drape_meters(material, uv) * float2(1.0f, max(material.drape_misc.y, 1e-3f));
        [unroll]
        for (uint i = 0; i < 2; i++)
        {
            const float scale = i == 0 ? material.drape_normal_decal.x : material.drape_normal_decal.z;
            const float gain = i == 0 ? material.drape_normal_decal.y : material.drape_normal_decal.w;
            if (scale <= 0.0f)
                continue;
            const float2 texel = material_textures[material_base + xp_drape_slot_normal_decal + i].Sample(samplers[sampler_anisotropic_wrap], meters * scale).rg;
            slope += (texel * 2.0f - 1.0f) * gain;
        }
    }
    return normalize(float3(slope, 1.0f));
}

/* @brief Whether a draped material has normal decals. */
public bool xp_drape_has_normal_decals(MaterialParameters material)
{
    return material.xp_drape_mode() != xp_drape_markings && material.drape_normal_decal.x > 0.0f;
}
