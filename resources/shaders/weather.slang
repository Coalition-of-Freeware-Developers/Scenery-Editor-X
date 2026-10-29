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
 * weather.slang
 * -------------------------------------------------------
 * Created: 25/10/2026
 * -------------------------------------------------------
 *
 * Weather on surfaces (plan Phase 12, TASK-078; XP12: weather_apply): wet ground, puddles, snow and ice on whatever a
 * pass shades, applied where the surface is written (the g-buffer, and the forward pass for blended drapes) rather than
 * in a pass of its own over the g-buffer as X-Plane does: SEDX's albedo target is sRGB without a storage view, and blended
 * drapes never reach the g-buffer.
 *
 * A surface's receptivity (weather_receptivity) says how readily it takes each kind: r rain, g puddles, b snow. It comes
 * from the art: a WEATHER mask; else, for snow, the albedo dotted with weights (SNOW_ALBEDO_LUMA, or the snow/luma_*
 * defaults) and no rain or puddles; WEATHER_TRANSPARENT takes the receptivity of what lies under it. A kind shows where
 * the receptivity, broken up by a noise over the ground, passes the environment's threshold for it (renderer/
 * weather_surface.h); smooth edges of the twxr widths. Then:
 *   rain     darkens the albedo (less on bright and on rough surfaces), raises gloss toward a water film's, F0 -> water's
 *   puddles  a mirror-smooth film with the normal turned up
 *   snow     on surfaces facing up (slope ramp), X-Plane's snow art laid over the ground from above, blended by its height
 *            channel and broken up by its decal
 *   ice      where the ground is wet enough, X-Plane's ice art the same way
 * Materials stay in X-Plane's terms while weather works on them: gloss (1 - roughness) / 0.96875 and the F0 / metalness v.
 */
module weather;

import common;
import resources;
import xp_material;

static const float3 WORLD_UP = float3(0.0f, 1.0f, 0.0f);
static const float3 LUMA_709 = float3(0.2126f, 0.7152f, 0.0722f);

// The weather art's slots in its hidden material (renderer_weather.cpp SetWeatherArt)
static const uint weather_slot_snow_albedo = material_texture_index_albedo + 0;
static const uint weather_slot_ice_albedo  = material_texture_index_albedo + 1;
static const uint weather_slot_snow_decal  = material_texture_index_albedo + 2;
static const uint weather_slot_ice_decal   = material_texture_index_albedo + 3;
static const uint weather_slot_snow_normal = material_texture_index_normal + 0;
static const uint weather_slot_ice_normal  = material_texture_index_normal + 1;
static const uint weather_slot_noise       = material_texture_index_roughness + 0;

// A material's WEATHER mask
static const uint weather_slot_mask = material_texture_index_emission + 3;

/* @brief What weather works on: a surface in X-Plane's terms. */
public struct WeatherSurface
{
    public float3 albedo;   // linear
    public float gloss;     // X-Plane gloss (0-1)
    public float v;         // metalness / F0 value (xp_material.slang)
    public float3 normal;   // world, unit
};

public WeatherSurface weather_surface(float3 albedo, float roughness, float v, float3 normal)
{
    WeatherSurface s;
    s.albedo = albedo;
    s.gloss = saturate((1.0f - roughness) / 0.96875f);
    s.v = v;
    s.normal = normalize(normal);
    return s;
}

public float weather_roughness(WeatherSurface s)
{
    return xp_roughness_from_gloss(s.gloss);
}

/* @brief Weather is on: some kind passes for some receptivity (the CPU sets a threshold no receptivity reaches otherwise). */
public bool weather_active()
{
    return any(buffer_environment.weather < 2.0f);
}

bool weather_art()
{
    return buffer_environment.weather_textures.y > 0.5f;
}

Texture2D weather_texture(uint slot)
{
    return material_textures[uint(buffer_environment.weather_textures.x + 0.5f) + slot];
}

float4 weather_sample(uint slot, float2 uv)
{
    return weather_texture(slot).Sample(samplers[sampler_anisotropic_wrap], uv);
}

float weather_noise(float2 position_xz, float per_meter)
{
    return weather_art() ? weather_sample(weather_slot_noise, position_xz * per_meter).r : 0.5f;
}

float ramp(float x, float start, float end)
{
    return saturate((x - start) / max(end - start, 1e-4f));
}

// Overlay blend of a coverage with a grey detail (0.5 leaves it unchanged)
float overlay(float coverage, float detail)
{
    return detail > 0.5f ? 1.0f - (1.0f - 2.0f * (detail - 0.5f)) * (1.0f - coverage) : 2.0f * coverage * detail;
}

/**
 * @brief How readily a surface takes rain (r), puddles (g) and snow (b, -1 none .. 1 and above all); see the header.
 * @param uv The coordinates its WEATHER mask is laid out on (the albedo's).
 * @param albedo The linear albedo, a its alpha.
 */
public float3 weather_receptivity(MaterialParameters material, uint material_base, float2 uv, float4 albedo)
{
    if (material.has_weather_mask())
        return material_textures[material_base + weather_slot_mask].Sample(samplers[sampler_anisotropic_wrap], uv).rgb;

    const float4 weights = material.has_snow_albedo_luma() ? float4(material.snow_luma, 0.0f, 0.0f) : buffer_environment.weather_snow_luma;
    return float3(0.0f, 0.0f, dot(weights, albedo));
}

/* @brief The receptivity as gbuffer_weather stores it (snow 0.5 + 0.5 s), and back. */
public float4 weather_encode(float3 receptivity) { return float4(saturate(receptivity.rg), saturate(0.5f + 0.5f * receptivity.b), 1.0f); }
public float3 weather_decode(float4 stored) { return float3(stored.rg, stored.b * 2.0f - 1.0f); }

// X-Plane's art laid flat over the ground: its colour, height or decal, and a top-down normal with gloss and F0
struct WeatherLayer
{
    float coverage;     // 0-1, after the height blend and the decal
    float3 albedo;
    float3 normal;
    float gloss;
    float v;
};

WeatherLayer weather_layer(float amount, float2 xz, float4 scale, float decal1, float decal2, uint slot_albedo, uint slot_normal, uint slot_decal)
{
    WeatherLayer layer;
    if (!weather_art())
    {
        // Without the art: plain white, no detail, matte
        layer.coverage = saturate(amount - 0.5f);
        layer.albedo = 0.8f;
        layer.normal = WORLD_UP;
        layer.gloss = 0.5f;
        layer.v = 0.04f;
        return layer;
    }

    const float4 albedo = weather_sample(slot_albedo, xz * scale.x);
    const float2 decal_uv = xz * scale.z;
    const float4 decal = weather_sample(slot_decal, decal_uv);
    float4 normal = weather_sample(slot_normal, xz * scale.y);
    // A block-compressed normal map keeps its blue and alpha two slots up (weather_textures.z snow, .w ice)
    const float companion = slot_normal == weather_slot_snow_normal ? buffer_environment.weather_textures.z : buffer_environment.weather_textures.w;
    if (companion > 0.5f)
        normal.ba = weather_sample(slot_normal + 2, xz * scale.y).rg;

    // The amount (0-2) raises the art's height channel; the decal's alpha breaks up the edge, its colour adds detail
    const float height_blend = saturate(amount - 1.0f + albedo.a);
    const float edge = lerp(0.5f, decal.a, decal2 * saturate(amount));
    layer.coverage = overlay(height_blend, edge);
    const float3 detail = decal.rgb - weather_texture(slot_decal).SampleLevel(samplers[sampler_bilinear_wrap], decal_uv, 8.0f).rgb;
    layer.albedo = max(albedo.rgb + detail * decal1, 0.0f);

    // Normal map laid flat: tangent x -> world x (east), tangent y -> world z
    const float2 n = normal.rg * 2.0f - 1.0f;
    layer.normal = normalize(float3(n.x, sqrt(saturate(1.0f - dot(n, n))), n.y));
    layer.gloss = normal.a;
    layer.v = normal.b;
    return layer;
}

void weather_cover(inout WeatherSurface s, WeatherLayer layer)
{
    s.albedo = lerp(s.albedo, layer.albedo, layer.coverage);
    s.gloss = lerp(s.gloss, layer.gloss, layer.coverage);
    s.v = lerp(s.v, layer.v, layer.coverage);
    s.normal = normalize(lerp(s.normal, layer.normal, layer.coverage));
}

/**
 * @brief Applies the environment's weather to a surface.
 * @param receptivity From weather_receptivity (or weather_decode of what lies under a WEATHER_TRANSPARENT surface).
 */
public void apply_surface_weather(inout WeatherSurface s, float3 position_world, float3 receptivity)
{
    if (!weather_active())
        return;

    const float4 threshold = buffer_environment.weather;   // rain, ice, snow, puddles
    const float2 xz = position_world.xz;

    // Rain and puddles: the rain noise breaks up the receptivity
    const float rain_noise = weather_noise(xz, buffer_environment.weather_rain.x);
    const float rain_width = buffer_environment.weather_rain.y;
    const bool takes_rain = receptivity.r > 0.0f;
    const float rain = takes_rain ? saturate(rain_noise * (receptivity.r + buffer_environment.weather_rain.z)) : 0.0f;
    const float wet = smoothstep(threshold.x - rain_width, threshold.x + rain_width, rain);
    const float puddle = smoothstep(threshold.w - rain_width, threshold.w + rain_width, receptivity.g * rain_noise);
    if (takes_rain)
    {
        const float luma = dot(s.albedo, LUMA_709);
        const float4 darkening = buffer_environment.weather_darkening;          // start, end, bright relief, albedo ratio
        const float4 darkening_gloss = buffer_environment.weather_darkening_gloss; // start, end, multiplier dark, light
        const float glossiness = s.gloss * lerp(darkening_gloss.z, darkening_gloss.w, luma);
        const float darken = saturate(ramp(wet, darkening.x, darkening.y) * ramp(glossiness, darkening_gloss.x, darkening_gloss.y) *
                                      (1.0f - saturate(luma * darkening.z)) * buffer_environment.weather_rain.w);
        s.albedo *= 1.0f - darken * darkening.w;

        // A water film: water's F0 once soaked, its gloss where darkened and in the puddles, puddles lie level
        const float film_gloss = buffer_environment.weather_water.y;
        s.v = lerp(s.v, 0.04f, saturate((wet - 0.5f) * 2.0f));
        s.gloss = max(lerp(s.gloss, film_gloss * buffer_environment.weather_water.x, saturate((darken - 0.1f) * 2.5f)),
                      lerp(s.gloss, film_gloss, saturate((puddle - 0.1f) * 2.5f)));
        s.normal = normalize(lerp(s.normal, WORLD_UP, saturate((puddle - 0.2f) * 1.25f)));
    }

    // Snow on what faces up enough, the snow noise moving the receptivity a little
    if (threshold.z < 2.0f)
    {
        const float snow_noise = weather_noise(xz, buffer_environment.weather_snow.x);
        const float snow_width = buffer_environment.weather_snow.y;
        const float facing = ramp(dot(WORLD_UP, s.normal), buffer_environment.weather_snow_detail.x, buffer_environment.weather_snow_detail.y);
        const float receptive = receptivity.b + buffer_environment.weather_snow.z * (2.0f * snow_noise - 1.0f);
        const float amount = smoothstep(threshold.z - snow_width, threshold.z + snow_width, receptive) * facing * 2.0f;
        if (amount > 0.0f)
            weather_cover(s, weather_layer(amount, xz, buffer_environment.weather_snow_scale, buffer_environment.weather_snow_detail.z,
                                           buffer_environment.weather_snow_detail.w, weather_slot_snow_albedo, weather_slot_snow_normal,
                                           weather_slot_snow_decal));
    }

    // Ice where the rain would wet
    if (takes_rain && threshold.y < 2.0f)
    {
        const float ice_width = buffer_environment.weather_ice.y;
        const float amount = smoothstep(threshold.y - ice_width, threshold.y + ice_width, rain) * 2.0f;
        if (amount > 0.0f)
            weather_cover(s, weather_layer(amount, xz, buffer_environment.weather_ice_scale, buffer_environment.weather_ice.z,
                                           buffer_environment.weather_ice.w, weather_slot_ice_albedo, weather_slot_ice_normal,
                                           weather_slot_ice_decal));
    }
}
