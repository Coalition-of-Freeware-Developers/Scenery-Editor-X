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
 * sky.slang
 * -------------------------------------------------------
 * Created: 20/10/2026
 * -------------------------------------------------------
 *
 * The sky where nothing was drawn (plan TASK-047, TASK-050; XP12: sky): the sky-view LUT, the sun disc (limb darkened),
 * the moon disc with its phase, a procedural star field turned with the sidereal time, the night's airglow floor, and,
 * below the horizon, the bare planet: a Lambertian ground of the planet's albedo lit by the sun, the moon and the sky,
 * seen through the atmosphere. Pre-exposed like the rest of frame_render.
 *   tex_uav  frame_render;  tex_depth  g-buffer depth;  reads tex_sky_view, tex_transmittance, tex_sky_ambient
 */

import common;
import resources;
import constants;
import position;
import atmosphere_common;

static const float MOON_ANGULAR_RADIUS = 0.00452f;
static const float3 LIMB_DARKENING = float3(0.48f, 0.60f, 0.70f);	// I(mu) / I(1) = 1 - u (1 - mu) (ASSUMPTION: grey-ish visible coefficients)
static const float STAR_CELLS = 256.0f;								// per cube face of the celestial sphere
static const float STAR_PROBABILITY = 0.023f;						// about 9000 stars brighter than magnitude 6.5
static const float STAR_MAGNITUDE_LIMIT = 6.5f;

float hash12(float2 p)
{
    float3 p3 = frac(float3(p.xyx) * 0.1031f);
    p3 += dot(p3, p3.yzx + 33.33f);
    return frac((p3.x + p3.y) * p3.z);
}

float3 hash32(float2 p)
{
    float3 p3 = frac(float3(p.xyx) * float3(0.1031f, 0.1030f, 0.0973f));
    p3 += dot(p3, p3.yxz + 33.33f);
    return frac((p3.xxy + p3.yzz) * p3.zyx);
}

/* @brief Stars (nits) along a world direction, sigma the radius (rad) each is spread over (about a pixel). */
float3 star_field(float3 direction, float sigma)
{
    const float4 brightness = buffer_environment.atmosphere_view;
    if (brightness.z <= 0.0f)
        return 0.0f;

    // Into the equatorial frame (x the vernal equinox, z the north celestial pole), then a cube-face grid on it
    const float3 e = float3(dot(buffer_environment.celestial[0].xyz, direction), dot(buffer_environment.celestial[1].xyz, direction),
                            dot(buffer_environment.celestial[2].xyz, direction));
    const float3 m = abs(e);
    uint face;
    float2 st;
    if (m.x >= m.y && m.x >= m.z) { face = e.x > 0.0f ? 0 : 1; st = e.yz / m.x; }
    else if (m.y >= m.z) { face = e.y > 0.0f ? 2 : 3; st = e.xz / m.y; }
    else { face = e.z > 0.0f ? 4 : 5; st = e.xy / m.z; }
    const float2 grid = (st * 0.5f + 0.5f) * STAR_CELLS;
    const float2 cell = floor(grid);
    const float2 seed = cell + float2(face * 1013.0f, face * 2741.0f);
    if (hash12(seed) > STAR_PROBABILITY)
        return 0.0f;

    // Where in the cell, how bright (number of stars grows about 10^0.47 per magnitude), what colour
    const float3 random = hash32(seed + 17.0f);
    const float2 star_st = (cell + 0.15f + 0.7f * random.xy) / STAR_CELLS * 2.0f - 1.0f;
    float3 star;
    if (face < 2) star = float3(face == 0 ? 1.0f : -1.0f, star_st.x, star_st.y);
    else if (face < 4) star = float3(star_st.x, face == 2 ? 1.0f : -1.0f, star_st.y);
    else star = float3(star_st.x, star_st.y, face == 4 ? 1.0f : -1.0f);
    star = normalize(star);
    const float angle = length(cross(normalize(e), star));
    const float magnitude = max(STAR_MAGNITUDE_LIMIT + 2.13f * log10(max(random.z, 1e-4f)), -1.5f);
    const float illuminance = pow(10.0f, -0.4f * (magnitude + 14.18f));	// lux
    const float t = hash12(seed + 71.0f);
    const float3 tint = lerp(float3(0.85f, 0.93f, 1.15f), float3(1.15f, 0.97f, 0.80f), t);
    return tint * illuminance * brightness.z / (2.0f * PI * sigma * sigma) * exp(-0.5f * angle * angle / (sigma * sigma));
}

/* @brief The sun's disc (nits outside the atmosphere), limb darkened, its mean the illuminance over its solid angle. */
float3 sun_disc(float3 direction)
{
    const float radius = max(buffer_environment.sun_direction.w, 1e-4f);
    const float cos_angle = dot(direction, get_sun_direction());
    if (cos_angle < cos(radius))
        return 0.0f;
    const float r = saturate(acos(saturate(cos_angle)) / radius);
    const float mu = sqrt(saturate(1.0f - r * r));
    const float3 darkening = (1.0f - LIMB_DARKENING * (1.0f - mu)) / (1.0f - LIMB_DARKENING / 3.0f);
    return get_sun_illuminance_toa() / (PI * radius * radius) * darkening;
}

/* @brief The moon's disc (nits outside the atmosphere), lit where the sun reaches its sphere. */
float3 moon_disc(float3 direction)
{
    const float fraction = buffer_environment.moon_direction.w;
    const float3 moon = get_moon_direction();
    const float cos_moon = dot(direction, moon);
    if (fraction <= 1e-3f || cos_moon < cos(MOON_ANGULAR_RADIUS))
        return 0.0f;
    const float3 offset = (direction - moon * cos_moon) / sin(MOON_ANGULAR_RADIUS);
    const float3 surface = offset - moon * sqrt(saturate(1.0f - dot(offset, offset)));
    if (dot(surface, get_sun_direction()) <= 0.0f)
        return 0.0f;
    return get_moon_illuminance_toa() / (fraction * PI * MOON_ANGULAR_RADIUS * MOON_ANGULAR_RADIUS);
}

[shader("compute")]
[numthreads(THREAD_GROUP_COUNT_X, THREAD_GROUP_COUNT_Y, 1)]
void main_comp(uint3 thread_id: SV_DispatchThreadID)
{
    float2 resolution;
    tex_uav.GetDimensions(resolution.x, resolution.y);
    if (any(thread_id.xy >= uint2(resolution)))
        return;
    if (tex_depth[thread_id.xy].r > 0.0f)	// reverse-z: something was drawn here
        return;

    const float2 uv = (float2(thread_id.xy) + 0.5f) / resolution;
    const float3 direction = normalize(get_position(0.5f, uv) - get_camera_position());
    const AtmosphereParameters a = get_atmosphere();
    const float3 camera = get_camera_atmosphere_position(a);
    const float pre_exposure = get_pre_exposure();

    float3 color = sample_sky_view(a, direction);
    const float ground = ray_sphere_nearest(camera, direction, a.bottom_radius);
    if (ground >= 0.0f)
    {
        // The bare planet: lit by the sun and moon through the atmosphere and by the sky (sky-ambient LUT at the ground)
        const float3 p = camera + direction * ground;
        const float3 normal = normalize(p);
        const float3 lifted = p + normal * 1e-3f;
        const float3 sun = get_sun_direction();
        const float3 moon = get_moon_direction();
        const float2 ambient_sun_uv = float2(from_unit_to_sub_uv(dot(normal, sun) * 0.5f + 0.5f, SKY_AMBIENT_LUT_SIZE.x), from_unit_to_sub_uv(0.0f, SKY_AMBIENT_LUT_SIZE.y));
        const float2 ambient_moon_uv = float2(from_unit_to_sub_uv(dot(normal, moon) * 0.5f + 0.5f, SKY_AMBIENT_LUT_SIZE.x), ambient_sun_uv.y);
        const float3 irradiance =
            get_sun_illuminance_toa() * (get_transmittance_to_space(a, lifted, sun) * saturate(dot(normal, sun)) + tex_sky_ambient.SampleLevel(samplers[sampler_bilinear_clamp], ambient_sun_uv, 0.0f).rgb) +
            get_moon_illuminance_toa() * (get_transmittance_to_space(a, lifted, moon) * saturate(dot(normal, moon)) + tex_sky_ambient.SampleLevel(samplers[sampler_bilinear_clamp], ambient_moon_uv, 0.0f).rgb);

        // Through the air between: a short march of the extinction
        float3 optical_depth = 0.0f;
        const float dt = ground / 12.0f;
        for (float s = 0.0f; s < 12.0f; s += 1.0f)
            optical_depth += sample_medium(a, camera + direction * ((s + 0.5f) * dt)).extinction * dt;
        color += a.ground_albedo * INV_PI * irradiance * exp(-optical_depth) * pre_exposure;
    }
    else
    {
        // Through the whole atmosphere: the discs, the stars, and the airglow under it all
        const float3 to_space = get_transmittance_to_space(a, camera, direction);
        const float pixel_angle = 2.0f / (resolution.y * abs(get_projection_matrix()[1][1]));
        const float3 beyond = sun_disc(direction) + moon_disc(direction) + star_field(direction, max(pixel_angle, 2e-4f));
        color += (beyond * to_space + buffer_environment.atmosphere_view.y) * pre_exposure;
    }

    tex_uav[thread_id.xy] = validate_output(float4(color, 1.0f));
}
