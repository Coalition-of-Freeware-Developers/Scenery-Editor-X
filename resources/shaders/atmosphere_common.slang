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
 * atmosphere_common.slang
 * -------------------------------------------------------
 * Created: 20/10/2026
 * -------------------------------------------------------
 *
 * The atmosphere shared by every pass (plan REQ-008, TASK-045): the medium from buffer_environment, ray / sphere
 * helpers, phase functions, and the parameterizations of the transmittance, multi-scattering, sky-view and aerial
 * perspective LUTs. After Sébastien Hillaire, "A Scalable and Production Ready Sky and Atmosphere Rendering Technique"
 * (EGSR 2020) and its reference implementation sebh/UnrealEngineSkyAtmosphere (MIT); LUT mappings after Bruneton and
 * Neyret 2008. XP12: scatter_compute_* (transmittance, multi-scatter, sky, aerial perspective volumes).
 *
 * Units are kilometers (and 1/km), as in the reference, so the planet's radii stay small floats. The planet's centre
 * sits under the camera: a world position p (meters, +Y up) is (p - camera) / 1000 + (0, sea radius + camera
 * altitude, 0). Densities fall off from sea level; the ground sphere is the airport's ground (sea radius + datum
 * elevation), which is what the sky's lower half shows where no scenery is drawn.
 */
module atmosphere_common;

import resources;
import constants;

public static const float3 SUN_COLOR = float3(1.0236f, 0.9961f, 0.9693f);	// environment/sky_light.h: the sun's colour, unit luminance

// LUT sizes (renderer_resources.cpp)
public static const float2 TRANSMITTANCE_LUT_SIZE = float2(256.0f, 64.0f);
public static const float2 MULTISCATTER_LUT_SIZE = float2(32.0f, 32.0f);
public static const float2 SKY_VIEW_LUT_SIZE = float2(192.0f, 108.0f);
public static const float2 SKY_AMBIENT_LUT_SIZE = float2(64.0f, 16.0f);
public static const float AERIAL_SLICES = 32.0f;

// The LUTs as the passes after the atmosphere pass read them
public [[vk::binding(55)]] TextureCubeArray tex_env_probes : register(t55);
public [[vk::binding(56)]] Texture3D tex_aerial_inscatter : register(t56);
public [[vk::binding(57)]] Texture3D tex_aerial_transmittance : register(t57);
public [[vk::binding(58)]] Texture2D tex_sky_view : register(t58);
public [[vk::binding(59)]] Texture2D tex_transmittance : register(t59);
public [[vk::binding(60)]] Texture2D tex_multiscatter : register(t60);
public [[vk::binding(61)]] Texture2D tex_sky_ambient : register(t61);

// Storage targets the atmosphere passes write besides tex_uav / tex3d_uav
public [[vk::binding(106)]] [[vk::image_format("unknown")]] RWTexture2DArray<float4> tex_uav_array : register(u106);
public [[vk::binding(107)]] [[vk::image_format("unknown")]] RWTexture3D<float4> tex3d_uav2 : register(u107);

public struct AtmosphereParameters
{
    public float sea_radius;            // km: densities fall off from here
    public float bottom_radius;         // km: the ground (sea radius + the airport's elevation)
    public float top_radius;
    public float3 rayleigh_scattering;  // 1/km at sea level
    public float rayleigh_exp_scale;    // -1 / scale height (1/km)
    public float3 mie_scattering;       // Per channel: the green value times the Angstrom factors
    public float3 mie_extinction;
    public float3 mie_spectral;         // Those factors (green 1)
    public float mie_exp_scale;
    public float mie_g;
    public float multiple_ratio;        // X-Plane scattering/multi_rat on the multiple-scattering estimate
    public float3 ozone_absorption;
    public float ozone_center;          // km
    public float ozone_half_width;
    public float haze_extinction;       // 1/km inside the surface haze layer: extra Mie (green; per channel x mie_spectral)
    public float fog_extinction;        // 1/km inside the same layer: X-Plane's fog, grey
    public float fog_albedo;
    public float haze_top;              // km above sea level
    public float haze_band;             // km
    public float3 ground_albedo;
};

public AtmosphereParameters get_atmosphere()
{
    AtmosphereParameters a;
    const EnvironmentData e = buffer_environment;
    a.sea_radius = e.planet.y * 0.001f;
    a.bottom_radius = a.sea_radius + max(e.camera.y, 0.0f) * 0.001f;
    a.top_radius = e.planet.z * 0.001f;
    a.rayleigh_scattering = e.rayleigh.rgb * 1000.0f;
    a.rayleigh_exp_scale = -1000.0f / max(e.rayleigh.w, 1.0f);
    a.mie_spectral = e.atmosphere_scattering.rgb;
    a.mie_scattering = e.mie.x * 1000.0f * a.mie_spectral;
    a.mie_extinction = e.mie.y * 1000.0f * a.mie_spectral;
    a.mie_exp_scale = -1000.0f / max(e.mie.z, 1.0f);
    a.mie_g = e.mie.w;
    a.multiple_ratio = e.atmosphere_scattering.w;
    a.ozone_absorption = e.ozone.rgb * 1000.0f;
    a.ozone_center = e.ozone.w * 0.001f;
    a.ozone_half_width = max(e.planet.x * 0.001f, 0.001f);
    a.haze_extinction = e.atmosphere_haze.x * 1000.0f;
    a.fog_extinction = e.atmosphere_view.w * 1000.0f;
    a.fog_albedo = e.atmosphere_haze.y;
    a.haze_top = (max(e.camera.y, 0.0f) + e.atmosphere_haze.z) * 0.001f;
    a.haze_band = max(e.atmosphere_haze.w * 0.001f, 1e-3f);
    a.ground_albedo = e.planet.w;
    return a;
}

/* @brief The camera in the atmosphere's frame (km; the planet's centre at the origin), a little above the ground. */
public float3 get_camera_atmosphere_position(AtmosphereParameters a)
{
    const float altitude = max(buffer_environment.camera.x * 0.001f, a.bottom_radius - a.sea_radius + 0.001f);
    return float3(0.0f, a.sea_radius + altitude, 0.0f);
}

/* @brief A world position (meters) in the atmosphere's frame. */
public float3 world_to_atmosphere(AtmosphereParameters a, float3 world)
{
    return (world - get_camera_position()) * 0.001f + get_camera_atmosphere_position(a);
}

// -------------------------------------------------------
// The medium
// -------------------------------------------------------

/**
 * The medium at a point. The visibility thickens the Mie over a layer from the ground (X-Plane 12.4.4 raises its
 * turbidity: the extra Mie is spectral, has the Mie's lobe and is in the sun's way too) and, for short visibilities
 * (sedx/fog_start_visibility_m, sedx/fog_full_visibility_m), turns into X-Plane's fog (scatter_compute_* behaviour):
 * grey, scattering isotropically, and only on view paths. The sun's light reaches every point (and the ground) without the fog (the transmittance and
 * multi-scattering LUTs leave it out), so thick fog glows white instead of going dark.
 */
public struct MediumSample
{
    public float3 rayleigh_scattering;
    public float3 mie_scattering;       // The clear Mie and the haze
    public float fog_scattering;
    public float3 scattering;           // All of it
    public float3 extinction;
};

/* @brief The haze and fog layer's density: 1 up to its top, easing to 0 over the band above (smoothstep). */
public float haze_profile(AtmosphereParameters a, float altitude)
{
    return smoothstep(0.0f, 1.0f, saturate(1.0f - (altitude - a.haze_top) / a.haze_band));
}

/* @brief The medium without the fog: what the sun's light crosses on its way in (the transmittance and multi-scattering LUTs). */
public MediumSample sample_medium_without_fog(AtmosphereParameters a, float3 position)
{
    const float altitude = max(length(position) - a.sea_radius, 0.0f);
    const float rayleigh = exp(altitude * a.rayleigh_exp_scale);
    const float mie = exp(altitude * a.mie_exp_scale);
    const float ozone = max(0.0f, 1.0f - abs(altitude - a.ozone_center) / a.ozone_half_width);
    const float3 haze = a.haze_extinction * haze_profile(a, altitude) * a.mie_spectral;

    MediumSample m;
    m.rayleigh_scattering = a.rayleigh_scattering * rayleigh;
    m.mie_scattering = a.mie_scattering * mie + haze * (a.mie_scattering.g / max(a.mie_extinction.g, 1e-9f));	// the haze scatters like the clear Mie
    m.fog_scattering = 0.0f;
    m.scattering = m.rayleigh_scattering + m.mie_scattering;
    m.extinction = m.rayleigh_scattering + a.mie_extinction * mie + haze + a.ozone_absorption * ozone;
    return m;
}

/* @brief The medium a view ray crosses: with the fog. */
public MediumSample sample_medium(AtmosphereParameters a, float3 position)
{
    MediumSample m = sample_medium_without_fog(a, position);
    const float fog = a.fog_extinction * haze_profile(a, max(length(position) - a.sea_radius, 0.0f));
    m.fog_scattering = fog * a.fog_albedo;
    m.scattering += m.fog_scattering;
    m.extinction += fog;
    return m;
}

/* @brief Optical depth (grey) of the fog along a ray from position (km) for up to length km. */
public float fog_optical_depth(AtmosphereParameters a, float3 position, float3 direction, float length_km)
{
    if (a.fog_extinction <= 0.0f)
        return 0.0f;
    // Only where the ray is inside the layer's top
    const float radius = a.sea_radius + a.haze_top + a.haze_band;
    const float b = dot(position, direction);
    const float discriminant = b * b - (dot(position, position) - radius * radius);
    if (discriminant <= 0.0f)
        return 0.0f;
    const float t0 = max(-b - sqrt(discriminant), 0.0f);
    const float t1 = min(-b + sqrt(discriminant), length_km);
    if (t1 <= t0)
        return 0.0f;
    float depth = 0.0f;
    const float dt = (t1 - t0) / 16.0f;
    for (float s = 0.0f; s < 16.0f; s += 1.0f)
        depth += haze_profile(a, max(length(position + direction * (t0 + (s + 0.5f) * dt)) - a.sea_radius, 0.0f)) * dt;
    return depth * a.fog_extinction;
}

// -------------------------------------------------------
// Rays, phases
// -------------------------------------------------------

/* @brief Distance along d (unit) from o to the nearest crossing of a sphere at the origin in front of o; -1 if none. */
public float ray_sphere_nearest(float3 o, float3 d, float radius)
{
    const float b = dot(o, d);
    const float c = dot(o, o) - radius * radius;
    const float discriminant = b * b - c;
    if (discriminant < 0.0f)
        return -1.0f;
    const float s = sqrt(discriminant);
    const float t0 = -b - s;
    const float t1 = -b + s;
    if (t0 >= 0.0f)
        return t0;
    return t1 >= 0.0f ? t1 : -1.0f;
}

/* @brief Rayleigh phase function (per steradian). */
public float rayleigh_phase(float cos_theta)
{
    return 3.0f / (16.0f * PI) * (1.0f + cos_theta * cos_theta);
}

/* @brief Cornette-Shanks phase function (per steradian): the sky's Mie lobe (environment/sky_light.cpp). */
public float cornette_shanks_phase(float cos_theta, float g)
{
    const float g2 = g * g;
    const float denominator = (2.0f + g2) * pow(max(1.0f + g2 - 2.0f * g * cos_theta, 1e-4f), 1.5f);
    return 3.0f / (8.0f * PI) * (1.0f - g2) * (1.0f + cos_theta * cos_theta) / denominator;
}

// -------------------------------------------------------
// LUT parameterizations
// -------------------------------------------------------

public float from_unit_to_sub_uv(float u, float resolution) { return (u + 0.5f / resolution) * (resolution / (resolution + 1.0f)); }
public float from_sub_uv_to_unit(float u, float resolution) { return (u - 0.5f / resolution) * (resolution / (resolution - 1.0f)); }

/* @brief Transmittance LUT (Bruneton): distance to the top of the atmosphere and height, both made uniform. */
public float2 transmittance_params_to_uv(AtmosphereParameters a, float view_height, float cos_zenith)
{
    const float h = sqrt(max(0.0f, a.top_radius * a.top_radius - a.bottom_radius * a.bottom_radius));
    const float rho = sqrt(max(0.0f, view_height * view_height - a.bottom_radius * a.bottom_radius));
    const float discriminant = view_height * view_height * (cos_zenith * cos_zenith - 1.0f) + a.top_radius * a.top_radius;
    const float d = max(0.0f, -view_height * cos_zenith + sqrt(max(discriminant, 0.0f)));
    const float d_min = a.top_radius - view_height;
    const float d_max = rho + h;
    return float2((d - d_min) / max(d_max - d_min, 1e-6f), rho / max(h, 1e-6f));
}

public void uv_to_transmittance_params(AtmosphereParameters a, float2 uv, out float view_height, out float cos_zenith)
{
    const float h = sqrt(max(0.0f, a.top_radius * a.top_radius - a.bottom_radius * a.bottom_radius));
    const float rho = h * uv.y;
    view_height = sqrt(rho * rho + a.bottom_radius * a.bottom_radius);
    const float d_min = a.top_radius - view_height;
    const float d_max = rho + h;
    const float d = d_min + uv.x * (d_max - d_min);
    cos_zenith = d == 0.0f ? 1.0f : (h * h - rho * rho - d * d) / (2.0f * view_height * d);
    cos_zenith = clamp(cos_zenith, -1.0f, 1.0f);
}

/**
 * @brief Transmittance from a point (km, planet frame) to space toward a direction: 0 below the point's horizon. The
 * horizon test (not a ray / sphere hit) keeps points on the ground, and the scenery a little below it, in the sun.
 */
public float3 get_transmittance_to_space(AtmosphereParameters a, float3 position, float3 direction)
{
    const float height = max(length(position), a.bottom_radius);
    const float3 up = normalize(position);
    const float cos_zenith = dot(up, direction);
    const float ratio = a.bottom_radius / height;
    if (cos_zenith < -sqrt(max(0.0f, 1.0f - ratio * ratio)))
        return 0.0f;
    const float2 uv = transmittance_params_to_uv(a, height, cos_zenith);
    return tex_transmittance.SampleLevel(samplers[sampler_bilinear_clamp], uv, 0.0f).rgb;
}

/* @brief Multi-scattering LUT: sun cos zenith and height above the ground. */
public float2 multiscatter_params_to_uv(AtmosphereParameters a, float view_height, float cos_sun_zenith)
{
    const float2 uv = saturate(float2(cos_sun_zenith * 0.5f + 0.5f, (view_height - a.bottom_radius) / (a.top_radius - a.bottom_radius)));
    return float2(from_unit_to_sub_uv(uv.x, MULTISCATTER_LUT_SIZE.x), from_unit_to_sub_uv(uv.y, MULTISCATTER_LUT_SIZE.y));
}

public float3 get_multiple_scattering(AtmosphereParameters a, float3 position, float cos_light_zenith)
{
    const float2 uv = multiscatter_params_to_uv(a, length(position), cos_light_zenith);
    return tex_multiscatter.SampleLevel(samplers[sampler_bilinear_clamp], uv, 0.0f).rgb * a.multiple_ratio;
}

/**
 * Sky-view LUT (Hillaire 2020, section 5.3): world azimuth on x (0 = east, the whole circle, so the moon fits too),
 * the zenith angle on y with the horizon in the middle and texels gathered toward it (a squared mapping each side).
 */
public float2 sky_view_params_to_uv(AtmosphereParameters a, bool hits_ground, float cos_view_zenith, float azimuth, float view_height)
{
    const float v_horizon = sqrt(max(0.0f, view_height * view_height - a.bottom_radius * a.bottom_radius));
    const float beta = acos(clamp(v_horizon / view_height, -1.0f, 1.0f));
    const float zenith_horizon_angle = PI - beta;
    float2 uv;
    if (!hits_ground)
    {
        float coord = acos(clamp(cos_view_zenith, -1.0f, 1.0f)) / zenith_horizon_angle;
        coord = 1.0f - sqrt(saturate(1.0f - coord));
        uv.y = coord * 0.5f;
    }
    else
    {
        float coord = (acos(clamp(cos_view_zenith, -1.0f, 1.0f)) - zenith_horizon_angle) / max(beta, 1e-6f);
        uv.y = sqrt(saturate(coord)) * 0.5f + 0.5f;
    }
    uv.x = frac(azimuth / (2.0f * PI));
    return float2(from_unit_to_sub_uv(uv.x, SKY_VIEW_LUT_SIZE.x), from_unit_to_sub_uv(uv.y, SKY_VIEW_LUT_SIZE.y));
}

public void uv_to_sky_view_params(AtmosphereParameters a, float2 uv, float view_height, out float cos_view_zenith, out float azimuth)
{
    uv = float2(from_sub_uv_to_unit(uv.x, SKY_VIEW_LUT_SIZE.x), from_sub_uv_to_unit(uv.y, SKY_VIEW_LUT_SIZE.y));
    const float v_horizon = sqrt(max(0.0f, view_height * view_height - a.bottom_radius * a.bottom_radius));
    const float beta = acos(clamp(v_horizon / view_height, -1.0f, 1.0f));
    const float zenith_horizon_angle = PI - beta;
    if (uv.y < 0.5f)
    {
        float coord = 1.0f - 2.0f * uv.y;
        coord = 1.0f - coord * coord;
        cos_view_zenith = cos(zenith_horizon_angle * coord);
    }
    else
    {
        float coord = uv.y * 2.0f - 1.0f;
        coord *= coord;
        cos_view_zenith = cos(zenith_horizon_angle + beta * coord);
    }
    azimuth = uv.x * 2.0f * PI;
}

/* @brief World azimuth of a direction (rad, 0 = east (+X), toward south (+Z) at pi / 2). */
public float world_azimuth(float3 direction)
{
    const float azimuth = atan2(direction.z, direction.x);
    return azimuth < 0.0f ? azimuth + 2.0f * PI : azimuth;
}

public float3 direction_from_zenith_azimuth(float cos_zenith, float azimuth)
{
    const float sin_zenith = sqrt(saturate(1.0f - cos_zenith * cos_zenith));
    return float3(sin_zenith * cos(azimuth), cos_zenith, sin_zenith * sin(azimuth));
}

/* @brief Sky-view LUT radiance (pre-exposed nits) seen from the camera along a world direction. */
public float3 sample_sky_view(AtmosphereParameters a, float3 direction)
{
    const float3 camera = get_camera_atmosphere_position(a);
    const float view_height = length(camera);
    const bool hits_ground = ray_sphere_nearest(camera, direction, a.bottom_radius) >= 0.0f;
    const float2 uv = sky_view_params_to_uv(a, hits_ground, direction.y, world_azimuth(direction), view_height);
    return tex_sky_view.SampleLevel(samplers[sampler_bilinear_wrap], uv, 0.0f).rgb;
}

// -------------------------------------------------------
// Aerial perspective volume: slices spread as the square of the depth (more near the camera), out to
// atmosphere_view.x meters
// -------------------------------------------------------

public float aerial_range_km() { return max(buffer_environment.atmosphere_view.x, 1.0f) * 0.001f; }

/* @brief Depth (km) of a froxel slice's centre. */
public float aerial_slice_depth(float slice)
{
    const float w = (slice + 0.5f) / AERIAL_SLICES;
    return w * w * aerial_range_km();
}

/**
 * @brief Aerial perspective between the camera and a point at distance_m along the pixel at uv: rgb in-scattering
 * (pre-exposed nits) and rgb transmittance. Fades to none in the first slice's near half.
 */
public void sample_aerial_perspective(float2 uv, float distance_m, out float3 inscatter, out float3 transmittance)
{
    const float w = sqrt(saturate(distance_m * 0.001f / aerial_range_km()));
    const float3 uvw = float3(uv, w);
    inscatter = tex_aerial_inscatter.SampleLevel(samplers[sampler_bilinear_clamp], uvw, 0.0f).rgb;
    transmittance = tex_aerial_transmittance.SampleLevel(samplers[sampler_bilinear_clamp], uvw, 0.0f).rgb;
    const float fade = saturate(w * AERIAL_SLICES * 2.0f);
    inscatter *= fade;
    transmittance = lerp(1.0f, transmittance, fade);
}

// -------------------------------------------------------
// Light sources: the sun and the moon outside the atmosphere
// -------------------------------------------------------

public float3 get_sun_illuminance_toa() { return buffer_environment.light_toa.rgb; }
public float3 get_moon_illuminance_toa() { return buffer_environment.light_toa.w * SUN_COLOR; }

/**
 * @brief In-scattered radiance (nits, not pre-exposed) per unit length at a point from one light: single scattering
 * (the light's transmittance without the fog, the planet's shadow; Rayleigh and Cornette-Shanks phases, the fog
 * isotropic) plus Hillaire's multiple-scattering term on all of it.
 */
public float3 light_scattering(AtmosphereParameters a, float3 position, MediumSample medium, float3 view_direction, float3 light_direction, float3 illuminance)
{
    const float height = length(position);
    const float3 up = position / height;
    const float cos_light_zenith = dot(up, light_direction);
    const float cos_theta = dot(view_direction, light_direction);
    const float3 phase_scattering = medium.rayleigh_scattering * rayleigh_phase(cos_theta) + medium.mie_scattering * cornette_shanks_phase(cos_theta, a.mie_g) +
                                    medium.fog_scattering / (4.0f * PI);
    const float3 to_light = get_transmittance_to_space(a, position, light_direction);
    const float3 multiple = get_multiple_scattering(a, position, cos_light_zenith);
    return illuminance * (phase_scattering * to_light + multiple * medium.scattering);
}

/* @brief Both lights. */
public float3 sky_scattering(AtmosphereParameters a, float3 position, MediumSample medium, float3 view_direction)
{
    return light_scattering(a, position, medium, view_direction, get_sun_direction(), get_sun_illuminance_toa()) +
           light_scattering(a, position, medium, view_direction, get_moon_direction(), get_moon_illuminance_toa());
}

/**
 * @brief Integrates in-scattering and transmittance along a ray from position (km) for t_max km (Hillaire's energy-
 * conserving step: S (1 - T_step) / extinction), sample_count steps.
 */
public void integrate_sky(AtmosphereParameters a, float3 position, float3 direction, float t_max, float sample_count, out float3 luminance, out float3 transmittance)
{
    luminance = 0.0f;
    transmittance = 1.0f;
    const float dt = t_max / sample_count;
    for (float s = 0.0f; s < sample_count; s += 1.0f)
    {
        const float3 p = position + direction * ((s + 0.3f) * dt);
        const MediumSample medium = sample_medium(a, p);
        const float3 sample_transmittance = exp(-medium.extinction * dt);
        const float3 scattering = sky_scattering(a, p, medium, direction);
        const float3 integral = (scattering - scattering * sample_transmittance) / max(medium.extinction, 1e-6f);
        luminance += transmittance * integral;
        transmittance *= sample_transmittance;
    }
}

/* @brief How far a ray from position goes through the atmosphere before the ground or space (km), 0 if it misses. */
public float atmosphere_ray_length(AtmosphereParameters a, float3 position, float3 direction)
{
    const float ground = ray_sphere_nearest(position, direction, a.bottom_radius);
    const float top = ray_sphere_nearest(position, direction, a.top_radius);
    if (ground >= 0.0f)
        return ground;
    return max(top, 0.0f);
}
