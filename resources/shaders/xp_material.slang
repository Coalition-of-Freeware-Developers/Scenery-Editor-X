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
