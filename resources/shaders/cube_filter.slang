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
 * cube_filter.slang
 * -------------------------------------------------------
 * Created: 20/10/2026
 * -------------------------------------------------------
 *
 * Environment probes (plan TASK-049, REQ-005; XP12: the environment cube probes): a cube array, 7 mips. A probe's
 * content is the sky from the sky-view LUT plus the planet's ground below the horizon (albedo from the environment,
 * lit by the sun, moon and sky), in absolute nits (probes outlive the frame's pre-exposure). The mips hold what
 * lighting reads: mip 0 the radiance, mips 1-5 GGX-filtered radiance for roughness mip / 5, mip 6 cosine-filtered
 * (irradiance / pi). The content is a function, so every mip is filtered from it directly rather than from the mip above
 * (the importance sampling of Khronos glTF-IBL-Sampler, without its source-mip selection).
 *   CAPTURE     mip 0
 *   FILTER      mips 1-5: pass_get_f4_value().x roughness
 *   IRRADIANCE  mip 6
 *   tex_uav_array  the probe array's mip (2D array: probe x 6 + face);  pass_get_f4_value().y first probe layer
 *   reads tex_sky_view (camera-centred: probe 0 sits at the camera), tex_transmittance, tex_sky_ambient
 */

import resources;
import constants;
import atmosphere_common;

static const uint FILTER_SAMPLES = 64;
static const uint IRRADIANCE_SAMPLES = 256;

/* @brief Direction through a cube face texel (Vulkan face order +X, -X, +Y, -Y, +Z, -Z). */
float3 cube_direction(uint face, float2 uv)
{
    const float2 c = uv * 2.0f - 1.0f;
    float3 d;
    switch (face)
    {
        case 0: d = float3(1.0f, -c.y, -c.x); break;
        case 1: d = float3(-1.0f, -c.y, c.x); break;
        case 2: d = float3(c.x, 1.0f, c.y); break;
        case 3: d = float3(c.x, -1.0f, -c.y); break;
        case 4: d = float3(c.x, -c.y, 1.0f); break;
        default: d = float3(-c.x, -c.y, -1.0f); break;
    }
    return normalize(d);
}

/* @brief Radiance (absolute nits) the probe sees along a world direction: sky, and the lit ground below the horizon. */
float3 probe_radiance(AtmosphereParameters a, float3 direction)
{
    const float pre_exposure = max(get_pre_exposure(), 1e-30f);
    float3 radiance = sample_sky_view(a, direction) / pre_exposure + buffer_environment.atmosphere_view.y;

    const float3 camera = get_camera_atmosphere_position(a);
    const float ground = ray_sphere_nearest(camera, direction, a.bottom_radius);
    if (ground >= 0.0f)
    {
        const float3 p = camera + direction * ground;
        const float3 normal = normalize(p);
        const float3 lifted = p + normal * 1e-3f;
        const float3 sun = get_sun_direction();
        const float3 moon = get_moon_direction();
        const float v = from_unit_to_sub_uv(0.0f, SKY_AMBIENT_LUT_SIZE.y);
        const float3 sky_sun = tex_sky_ambient.SampleLevel(samplers[sampler_bilinear_clamp], float2(from_unit_to_sub_uv(dot(normal, sun) * 0.5f + 0.5f, SKY_AMBIENT_LUT_SIZE.x), v), 0.0f).rgb;
        const float3 sky_moon = tex_sky_ambient.SampleLevel(samplers[sampler_bilinear_clamp], float2(from_unit_to_sub_uv(dot(normal, moon) * 0.5f + 0.5f, SKY_AMBIENT_LUT_SIZE.x), v), 0.0f).rgb;
        const float3 irradiance = get_sun_illuminance_toa() * (get_transmittance_to_space(a, lifted, sun) * saturate(dot(normal, sun)) + sky_sun) +
                                  get_moon_illuminance_toa() * (get_transmittance_to_space(a, lifted, moon) * saturate(dot(normal, moon)) + sky_moon);
        float3 optical_depth = 0.0f;
        const float dt = ground / 8.0f;
        for (float s = 0.0f; s < 8.0f; s += 1.0f)
            optical_depth += sample_medium(a, camera + direction * ((s + 0.5f) * dt)).extinction * dt;
        radiance += a.ground_albedo * INV_PI * irradiance * exp(-optical_depth);
    }
    return radiance;
}

float2 hammersley(uint i, uint count)
{
    return float2((float(i) + 0.5f) / float(count), float(reversebits(i)) * 2.3283064365386963e-10f);
}

/* @brief An orthonormal frame around n. */
void make_frame(float3 n, out float3 t, out float3 b)
{
    const float3 up = abs(n.y) < 0.999f ? float3(0.0f, 1.0f, 0.0f) : float3(1.0f, 0.0f, 0.0f);
    t = normalize(cross(up, n));
    b = cross(n, t);
}

[shader("compute")]
[numthreads(8, 8, 1)]
void main_comp(uint3 thread_id: SV_DispatchThreadID)
{
    float3 size;
    tex_uav_array.GetDimensions(size.x, size.y, size.z);
    if (any(thread_id.xy >= uint2(size.xy)))
        return;
    const uint face = thread_id.z % 6;
    const uint layer = uint(pass_get_f4_value().y + 0.5f) + thread_id.z;
    const float2 uv = (float2(thread_id.xy) + 0.5f) / size.xy;
    const float3 n = cube_direction(face, uv);
    const AtmosphereParameters a = get_atmosphere();

#if CAPTURE
    const float3 result = probe_radiance(a, n);
#elif FILTER
    // GGX importance sampling with n = v = r (the split-sum assumption)
    const float roughness = max(pass_get_f4_value().x, 0.01f);
    const float alpha = roughness * roughness;
    float3 t, b;
    make_frame(n, t, b);
    float3 sum = 0.0f;
    float weight = 0.0f;
    for (uint i = 0; i < FILTER_SAMPLES; ++i)
    {
        const float2 xi = hammersley(i, FILTER_SAMPLES);
        const float cos_h = sqrt((1.0f - xi.y) / (1.0f + (alpha * alpha - 1.0f) * xi.y));
        const float sin_h = sqrt(saturate(1.0f - cos_h * cos_h));
        const float phi = 2.0f * PI * xi.x;
        const float3 h = t * (sin_h * cos(phi)) + b * (sin_h * sin(phi)) + n * cos_h;
        const float3 l = reflect(-n, h);
        const float n_dot_l = dot(n, l);
        if (n_dot_l > 0.0f)
        {
            sum += probe_radiance(a, l) * n_dot_l;
            weight += n_dot_l;
        }
    }
    const float3 result = sum / max(weight, 1e-6f);
#elif IRRADIANCE
    // Cosine-weighted: the mean radiance is irradiance / pi
    float3 t, b;
    make_frame(n, t, b);
    float3 sum = 0.0f;
    for (uint i = 0; i < IRRADIANCE_SAMPLES; ++i)
    {
        const float2 xi = hammersley(i, IRRADIANCE_SAMPLES);
        const float r = sqrt(xi.y);
        const float phi = 2.0f * PI * xi.x;
        const float3 l = t * (r * cos(phi)) + b * (r * sin(phi)) + n * sqrt(saturate(1.0f - xi.y));
        sum += probe_radiance(a, l);
    }
    const float3 result = sum / IRRADIANCE_SAMPLES;
#else
    const float3 result = 0.0f;
#endif

    tex_uav_array[uint3(thread_id.xy, layer)] = float4(result, 1.0f);
}
