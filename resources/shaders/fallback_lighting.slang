/**
 * -------------------------------------------------------
 * Scenery Editor X
 * -------------------------------------------------------
 * Copyright (c) 2026 Thomas Ray
 * Copyright (c) 2026 Coalition of Freeware Developers
 * -------------------------------------------------------
 * fallback_lighting.slang
 * -------------------------------------------------------
 * Placeholder sun and sky used while the scene has no directional light of its own (bootstrap renderer).
 * Shared by light_composition.slang and forward_transparent.slang so blended surfaces match deferred ones.
 * Both passes push:
 *   f3_value   (values[0].xyz) - unit direction towards the fallback sun
 *   f2_value.x (values[0].w)  - 1 when the fallback lighting is active (no directional light), else 0
 *   f2_value.y (values[1].w)  - daylight, 1 at noon .. 0 at night (Renderer::SetFallbackDaylight)
 * -------------------------------------------------------
 */
module fallback_lighting;

import resources;

static const float3 sky_zenith_day     = float3(0.20f, 0.40f, 0.75f);
static const float3 sky_horizon_day    = float3(0.62f, 0.74f, 0.88f);
static const float3 sky_zenith_night   = float3(0.004f, 0.006f, 0.014f);
static const float3 sky_horizon_night  = float3(0.012f, 0.016f, 0.028f);
static const float3 ground_color       = float3(0.30f, 0.29f, 0.27f);
static const float3 sun_color          = float3(1.00f, 0.96f, 0.90f) * 2.5f;
static const float ambient_amount      = 0.35f;
static const float ambient_amount_night = 0.03f;

public bool fallback_lighting_active() { return pass_get_f2_value().x > 0.0f; }
public float fallback_daylight() { return saturate(pass_get_f2_value().y); }
public float3 fallback_sun_direction() { return normalize(pass_get_f3_value()); }

/* @brief Sky radiance seen along a direction. */
public float3 fallback_sky_color(float3 direction)
{
    float daylight = fallback_daylight();
    float3 zenith  = lerp(sky_zenith_night, sky_zenith_day, daylight);
    float3 horizon = lerp(sky_horizon_night, sky_horizon_day, daylight);

    float up = direction.y;
    float3 sky    = lerp(horizon, zenith, pow(saturate(up), 0.5f));
    float3 ground = lerp(horizon, ground_color * lerp(0.03f, 1.0f, daylight), saturate(-up * 6.0f));
    return up >= 0.0f ? sky : ground;
}

/* @brief Sun color at the current daylight (black at night). */
public float3 fallback_sun_color()
{
    return sun_color * fallback_daylight();
}

/* @brief Hemispherical ambient light for a normal. */
public float3 fallback_ambient(float3 n)
{
    float daylight = fallback_daylight();
    float3 sky = lerp(sky_zenith_night, sky_zenith_day, daylight);
    float3 ground = ground_color * lerp(0.03f, 1.0f, daylight);
    return lerp(ground, sky, n.y * 0.5f + 0.5f) * lerp(ambient_amount_night, ambient_amount, daylight);
}
