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
 * environment_lighting.slang
 * -------------------------------------------------------
 * Created: 20/10/2026
 * -------------------------------------------------------
 *
 * The world's light in physical units (plan TASK-037): the sun and moon after the atmosphere (lux), the sky's radiance
 * and irradiance (nits, lux) from the samples the CPU sky model (environment/sky_light.h) puts in buffer_environment.
 * Shared by light_composition.slang (deferred surfaces, the sky behind them) and forward_transparent.slang (blended
 * surfaces) so both light alike. Everything here is radiance in nits: callers multiply by get_pre_exposure().
 *
 * With the atmosphere on (plan Phase 6) the sun and moon reach each pixel through the transmittance LUT, ambient light
 * and reflections come from the environment probes (cube_filter.slang), and aerial perspective is applied by the
 * callers (xp_apply_aerial_perspective). With those features off it falls back to the interim CPU sky: the sun and
 * moon at the camera, the sky interpolated from seven directions, two hemispheres of ambient light. The sun (or the
 * moon at night) is shadowed by the cascades and the clouds (plan Phase 7, csm_sample.slang): callers bind them with
 * Renderer::BindShadows.
 */
module environment_lighting;

import resources;
import constants;
import atmosphere_common;
import csm_sample;

// RendererFeatures bits (renderer_features.cpp table order) the lighting follows
public static const uint FEATURE_ATMOSPHERE = 0;
public static const uint FEATURE_AERIAL_PERSPECTIVE = 1;
public static const uint FEATURE_ENV_PROBES = 2;
public static const float PROBE_DIFFUSE_MIP = 6.0f;

static const float3 LUMA = float3(0.2126f, 0.7152f, 0.0722f);
static const float MOON_ANGULAR_RADIUS = 0.00452f;	// rad (mean distance)
static const float NIGHT_SKY_NITS = 2e-4f;			// Airglow and starlight of a moonless sky (ASSUMPTION: 22 mag/arcsec^2)
static const float SUN_MIN_ALPHA = 0.005f;			// GGX alpha floor: about the sun disc's angular radius, so mirrors stay in range

/* @brief Cornette-Shanks phase function (per steradian), as the CPU sky model uses for the Mie lobe. */
public float xp_mie_phase(float cos_theta, float g)
{
    const float g2 = g * g;
    const float denominator = (2.0f + g2) * pow(max(1.0f + g2 - 2.0f * g * cos_theta, 1e-4f), 1.5f);
    return 3.0f / (8.0f * PI) * (1.0f - g2) * (1.0f + cos_theta * cos_theta) / denominator;
}

// Sky sample i (0 zenith, 1-3 horizon toward / beside / away from the sun, 4-6 the same 30 degrees up)
static float3 sky_sample(uint i, bool mie)
{
    return mie ? buffer_environment.sky_mie[i].rgb : buffer_environment.sky_radiance[i].rgb;
}

// The samples interpolated at an elevation (deg, 0-90) and an azimuth from the sun's (deg, 0-180)
static float3 sky_interpolate(float elevation, float azimuth, bool mie)
{
    const float a = saturate(azimuth / 90.0f);
    const float b = saturate(azimuth / 90.0f - 1.0f);
    const float3 horizon = lerp(lerp(sky_sample(1, mie), sky_sample(2, mie), a), sky_sample(3, mie), b);
    const float3 mid = lerp(lerp(sky_sample(4, mie), sky_sample(5, mie), a), sky_sample(6, mie), b);
    const float3 zenith = sky_sample(0, mie);
    // The horizon's haze gives way quickly above it (its path length falls as 1 / sin(elevation)): square root in the
    // lower band
    return elevation < 30.0f ? lerp(horizon, mid, sqrt(saturate((elevation - 0.5f) / 29.5f))) : lerp(mid, zenith, saturate((elevation - 30.0f) / 60.0f));
}

/* @brief Sunlight on a level surface (lux): the direct sun and moon on the ground below the observer. */
public float3 xp_direct_level_illuminance()
{
    return get_sun_illuminance() * max(get_sun_direction().y, 0.0f) + get_moon_illuminance() * max(get_moon_direction().y, 0.0f);
}

/* @brief Radiance (nits) of the lit ground below the horizon: a Lambertian ground of the planet's albedo. */
public float3 xp_ground_radiance()
{
    const float albedo = buffer_environment.sky_irradiance.w;
    return albedo * (buffer_environment.sky_irradiance.rgb + xp_direct_level_illuminance()) * INV_PI;
}

/* @brief Sky radiance (nits) along a direction (unit, world frame), without the sun and moon discs. */
public float3 xp_sky_radiance(float3 direction)
{
    const float3 sun = get_sun_direction();
    const float elevation = degrees(asin(clamp(direction.y, -1.0f, 1.0f)));

    // Azimuth from the sun's, folded to 0-180 (the sky is symmetric about the sun's vertical plane)
    const float2 horizontal = direction.xz;
    const float2 sun_horizontal = sun.xz;
    const float lengths = length(horizontal) * length(sun_horizontal);
    const float cos_azimuth = lengths > 1e-6f ? dot(horizontal, sun_horizontal) / lengths : 1.0f;
    const float azimuth = degrees(acos(clamp(cos_azimuth, -1.0f, 1.0f)));

    const float up = max(elevation, 0.0f);
    float3 sky = sky_interpolate(up, azimuth, false);
    sky += sky_interpolate(up, azimuth, true) * xp_mie_phase(dot(direction, sun), buffer_environment.mie.w);
    sky += NIGHT_SKY_NITS;

    // Below the horizon the ground, blended in over the first few degrees
    return lerp(sky, xp_ground_radiance(), saturate(-elevation / 3.0f));
}

/**
 * @brief The sun and moon discs (nits) seen along a direction: the sun uniform over its disc, the moon over its lit
 * part (its phase from the sun's direction across the sphere).
 */
public float3 xp_sun_moon_discs(float3 direction)
{
    float3 radiance = 0.0f;

    const float sun_radius = max(buffer_environment.sun_direction.w, 1e-4f);
    const float3 sun = get_sun_direction();
    if (dot(direction, sun) > cos(sun_radius))
        radiance += get_sun_illuminance() / (PI * sun_radius * sun_radius);

    const float3 moon = get_moon_direction();
    const float fraction = buffer_environment.moon_direction.w;
    const float cos_moon = dot(direction, moon);
    if (fraction > 1e-3f && cos_moon > cos(MOON_ANGULAR_RADIUS))
    {
        // The point on the moon's sphere this ray meets (offset in disc radii, then up out of the disc toward the viewer)
        const float3 offset = (direction - moon * cos_moon) / sin(MOON_ANGULAR_RADIUS);
        const float3 surface = offset - moon * sqrt(saturate(1.0f - dot(offset, offset)));
        if (dot(surface, sun) > 0.0f)
            radiance += get_moon_illuminance() / (fraction * PI * MOON_ANGULAR_RADIUS * MOON_ANGULAR_RADIUS);
    }
    return radiance;
}

/**
 * @brief Light reaching a surface facing normal from the sky and the ground (lux): the sky's level irradiance over the
 * upper hemisphere, the lit ground's over the lower one, each weighted by how much of it the normal sees.
 */
public float3 xp_ambient_irradiance(float3 normal)
{
    const float3 sky = buffer_environment.sky_irradiance.rgb + NIGHT_SKY_NITS * PI;
    const float3 ground = xp_ground_radiance() * PI;
    const float t = saturate(normal.y * 0.5f + 0.5f);
    return sky * t + ground * (1.0f - t);
}

/* @brief Mirror-ish sky radiance (nits) a surface reflects along r, blurred toward the irradiance by roughness. */
public float3 xp_ambient_reflection(float3 r, float roughness)
{
    const float blur = smoothstep(0.0f, 0.75f, roughness);
    return lerp(xp_sky_radiance(r), xp_ambient_irradiance(r) * INV_PI, blur);
}

/* @brief A surface ready to light: the g-buffer's or the forward pass's decoded material. */
public struct XpLitSurface
{
    public float3 albedo;
    public float3 normal;		// unit, world
    public float3 geometric_normal;	// unit, world: the surface's plane without its normal map (the shadow lookups)
    public float3 view;			// unit, from the surface toward the camera
    public float roughness;		// perceptual (alpha = roughness^2)
    public float v;				// metalness / F0 parameter (xp_material.slang)
    public float occlusion;
    public float2 env_brdf;		// BRDF LUT at (n.v, roughness): F0 scale, bias
};

static float3 fresnel_schlick(float3 f0, float v_dot_h)
{
    return f0 + (1.0f - f0) * pow(1.0f - saturate(v_dot_h), 5.0f);
}

/* @brief Diffuse and specular radiance (nits) a directional light of illuminance lux along l reflects toward the view. */
public void xp_directional_light(XpLitSurface surface, float3 f0, float3 l, float3 illuminance, inout float3 diffuse, inout float3 specular)
{
    const float n_dot_l = saturate(dot(surface.normal, l));
    if (n_dot_l <= 0.0f)
        return;

    const float3 h = normalize(l + surface.view);
    const float n_dot_v = max(dot(surface.normal, surface.view), 1e-4f);
    const float n_dot_h = saturate(dot(surface.normal, h));
    const float alpha = max(surface.roughness * surface.roughness, SUN_MIN_ALPHA);
    const float a2 = alpha * alpha;

    // GGX, height-correlated Smith visibility, Schlick fresnel (the BRDF the LUT integrates)
    const float d_denominator = n_dot_h * n_dot_h * (a2 - 1.0f) + 1.0f;
    const float d = a2 / (PI * d_denominator * d_denominator);
    const float vis = 0.5f / max(n_dot_l * sqrt(n_dot_v * n_dot_v * (1.0f - a2) + a2) + n_dot_v * sqrt(n_dot_l * n_dot_l * (1.0f - a2) + a2), 1e-6f);
    const float3 f = fresnel_schlick(f0, dot(surface.view, h));

    diffuse += surface.albedo * (1.0f - surface.v) * INV_PI * (1.0f - f) * illuminance * n_dot_l;
    specular += d * vis * f * illuminance * n_dot_l;
}

/* @brief The sun's illuminance (lux) at a world position: through the transmittance LUT, or at the camera (CPU). */
public float3 xp_sun_illuminance_at(float3 world)
{
    if (!is_feature_enabled(FEATURE_ATMOSPHERE))
        return get_sun_illuminance();
    const AtmosphereParameters a = get_atmosphere();
    return get_sun_illuminance_toa() * get_transmittance_to_space(a, world_to_atmosphere(a, world), get_sun_direction());
}

public float3 xp_moon_illuminance_at(float3 world)
{
    if (!is_feature_enabled(FEATURE_ATMOSPHERE))
        return get_moon_illuminance();
    const AtmosphereParameters a = get_atmosphere();
    return get_moon_illuminance_toa() * get_transmittance_to_space(a, world_to_atmosphere(a, world), get_moon_direction());
}

/* @brief Probe 0's prefiltered radiance (absolute nits) along r for a roughness (REQ-005: LOD roughness x 5 + bias). */
public float3 xp_probe_specular(float3 r, float roughness)
{
    const float lod = clamp(roughness * 5.0f + buffer_environment.probe.w, 0.0f, 5.0f);
    return tex_env_probes.SampleLevel(samplers[sampler_trilinear_clamp], float4(r, 0.0f), lod).rgb;
}

/* @brief Probe 0's cosine-filtered radiance (irradiance / pi, absolute) around a normal. */
public float3 xp_probe_diffuse(float3 n)
{
    return tex_env_probes.SampleLevel(samplers[sampler_trilinear_clamp], float4(n, 0.0f), PROBE_DIFFUSE_MIP).rgb;
}

/**
 * @brief Everything the environment lights a surface at world with (nits), split into diffuse and specular: the sun and
 * moon (unless a scene directional light replaces them) through their shadows, the ambient light and the reflected sky
 * (probes, or the interim two-hemisphere sky), through the BRDF LUT.
 */
public void xp_environment_lighting_split(XpLitSurface surface, float3 f0, bool sun_and_moon, float3 world, out float3 diffuse, out float3 specular)
{
    diffuse = 0.0f;
    specular = 0.0f;
    if (sun_and_moon)
    {
        const float2 shadows = xp_light_shadows(world, surface.geometric_normal);
        xp_directional_light(surface, f0, get_sun_direction(), xp_sun_illuminance_at(world) * shadows.x, diffuse, specular);
        xp_directional_light(surface, f0, get_moon_direction(), xp_moon_illuminance_at(world) * shadows.y, diffuse, specular);
    }

    const float3 r = reflect(-surface.view, surface.normal);
    const float3 specular_brdf = f0 * surface.env_brdf.x + surface.env_brdf.y;
    if (is_feature_enabled(FEATURE_ENV_PROBES) && is_feature_enabled(FEATURE_ATMOSPHERE))	// probes are drawn from the atmosphere
    {
        diffuse += surface.albedo * (1.0f - surface.v) * xp_probe_diffuse(surface.normal) * surface.occlusion;
        specular += specular_brdf * xp_probe_specular(r, surface.roughness) * surface.occlusion;
    }
    else
    {
        diffuse += surface.albedo * (1.0f - surface.v) * xp_ambient_irradiance(surface.normal) * INV_PI * surface.occlusion;
        specular += specular_brdf * xp_ambient_reflection(r, surface.roughness) * surface.occlusion;
    }
}

/* @brief xp_environment_lighting_split, summed. */
public float3 xp_environment_lighting(XpLitSurface surface, float3 f0, bool sun_and_moon, float3 world)
{
    float3 diffuse, specular;
    xp_environment_lighting_split(surface, f0, sun_and_moon, world, diffuse, specular);
    return diffuse + specular;
}

/**
 * @brief Aerial perspective over a pre-exposed colour seen at distance_m through screen_uv (XP12: the aerial volumes in
 * the deferred lighting): colour x transmittance + in-scattering x coverage (coverage: a blended surface's alpha; the
 * frame behind it already has the in-scattering for the rest). Unchanged with the feature off.
 */
public float3 xp_apply_aerial_perspective(float3 color, float2 screen_uv, float distance_m, float coverage)
{
    if (!is_feature_enabled(FEATURE_AERIAL_PERSPECTIVE) || !is_feature_enabled(FEATURE_ATMOSPHERE))
        return color;
    float3 inscatter, transmittance;
    sample_aerial_perspective(screen_uv, distance_m, inscatter, transmittance);
    return color * transmittance + inscatter * coverage;
}

/* @brief The transmittance part alone, for additive light (billboards: the in-scattering is in the frame already). */
public float3 xp_aerial_transmittance(float2 screen_uv, float distance_m)
{
    if (!is_feature_enabled(FEATURE_AERIAL_PERSPECTIVE) || !is_feature_enabled(FEATURE_ATMOSPHERE))
        return 1.0f;
    float3 inscatter, transmittance;
    sample_aerial_perspective(screen_uv, distance_m, inscatter, transmittance);
    return transmittance;
}
