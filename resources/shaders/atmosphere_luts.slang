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
 * atmosphere_luts.slang
 * -------------------------------------------------------
 * Created: 20/10/2026
 * -------------------------------------------------------
 *
 * The atmosphere's medium LUTs (plan TASK-045; XP12: scatter_compute_transmittance / _multi):
 *   TRANSMITTANCE  256x64: transmittance to the top of the atmosphere by height and view zenith (tex_uav)
 *   MULTISCATTER   32x32: Hillaire's isotropic multiple-scattering transfer, Psi_ms, by sun zenith and height, for a
 *                  unit light (tex_uav; reads tex_transmittance)
 * Both depend on the medium only (visibility, sky condition), not on the sun or the camera.
 */

import resources;
import constants;
import atmosphere_common;

static const float TRANSMITTANCE_STEPS = 40.0f;
static const float DIFFUSE_TRANSMITTANCE_STEPS = 12.0f;

/* @brief Transmittance toward a light through the delta-scaled medium (0 behind the planet). */
float3 diffuse_transmittance_to_space(AtmosphereParameters a, float3 position, float3 direction)
{
    if (ray_sphere_nearest(position, direction, a.bottom_radius) >= 0.0f)
        return 0.0f;
    const float length_km = max(ray_sphere_nearest(position, direction, a.top_radius), 0.0f);
    float3 optical_depth = 0.0f;
    const float dt = length_km / DIFFUSE_TRANSMITTANCE_STEPS;
    for (float s = 0.0f; s < DIFFUSE_TRANSMITTANCE_STEPS; s += 1.0f)
        optical_depth += sample_medium_diffuse(a, position + direction * ((s + 0.5f) * dt)).extinction * dt;
    return exp(-optical_depth);
}
static const float MULTISCATTER_STEPS = 20.0f;
static const uint MULTISCATTER_DIRECTIONS_SQRT = 8;

[shader("compute")]
[numthreads(8, 8, 1)]
void main_comp(uint3 thread_id: SV_DispatchThreadID)
{
    float2 resolution;
    tex_uav.GetDimensions(resolution.x, resolution.y);
    if (any(thread_id.xy >= uint2(resolution)))
        return;
    const float2 uv = (float2(thread_id.xy) + 0.5f) / resolution;
    const AtmosphereParameters a = get_atmosphere();

#if TRANSMITTANCE
    float view_height, cos_zenith;
    uv_to_transmittance_params(a, uv, view_height, cos_zenith);
    const float3 position = float3(0.0f, view_height, 0.0f);
    const float3 direction = float3(sqrt(saturate(1.0f - cos_zenith * cos_zenith)), cos_zenith, 0.0f);
    const float length_km = max(ray_sphere_nearest(position, direction, a.top_radius), 0.0f);

    float3 optical_depth = 0.0f;
    const float dt = length_km / TRANSMITTANCE_STEPS;
    for (float s = 0.0f; s < TRANSMITTANCE_STEPS; s += 1.0f)
        optical_depth += sample_medium(a, position + direction * ((s + 0.5f) * dt)).extinction * dt;
    tex_uav[thread_id.xy] = float4(exp(-optical_depth), 1.0f);
#elif MULTISCATTER
    // Hillaire 2020, section 5.5: second-order light from every direction (isotropic phase, unit sun) and the
    // fraction f_ms the medium re-scatters, summed as the geometric series L2 / (1 - f_ms). Through the delta-scaled
    // medium (sample_medium_diffuse), with the sun's light reaching each point through it too, so a thick haze layer
    // passes its diffuse light down instead of going dark inside
    const float cos_sun_zenith = from_sub_uv_to_unit(uv.x, MULTISCATTER_LUT_SIZE.x) * 2.0f - 1.0f;
    const float view_height = a.bottom_radius + saturate(from_sub_uv_to_unit(uv.y, MULTISCATTER_LUT_SIZE.y)) * (a.top_radius - a.bottom_radius);
    const float3 position = float3(0.0f, view_height + 1e-4f, 0.0f);
    const float3 sun = float3(sqrt(saturate(1.0f - cos_sun_zenith * cos_sun_zenith)), cos_sun_zenith, 0.0f);
    const float isotropic = 1.0f / (4.0f * PI);

    float3 second_order = 0.0f;
    float3 transfer = 0.0f;
    const uint count = MULTISCATTER_DIRECTIONS_SQRT * MULTISCATTER_DIRECTIONS_SQRT;
    for (uint i = 0; i < count; ++i)
    {
        // Stratified over the sphere
        const float u = (float(i % MULTISCATTER_DIRECTIONS_SQRT) + 0.5f) / MULTISCATTER_DIRECTIONS_SQRT;
        const float v = (float(i / MULTISCATTER_DIRECTIONS_SQRT) + 0.5f) / MULTISCATTER_DIRECTIONS_SQRT;
        const float cos_theta = 1.0f - 2.0f * v;
        const float sin_theta = sqrt(saturate(1.0f - cos_theta * cos_theta));
        const float phi = 2.0f * PI * u;
        const float3 direction = float3(sin_theta * cos(phi), cos_theta, sin_theta * sin(phi));

        const float ground = ray_sphere_nearest(position, direction, a.bottom_radius);
        const float length_km = atmosphere_ray_length(a, position, direction);
        const float dt = length_km / MULTISCATTER_STEPS;
        float3 throughput = 1.0f;
        float3 luminance = 0.0f;
        float3 medium_transfer = 0.0f;
        for (float s = 0.0f; s < MULTISCATTER_STEPS; s += 1.0f)
        {
            const float3 p = position + direction * ((s + 0.3f) * dt);
            const MediumSample medium = sample_medium_diffuse(a, p);
            const float3 step_transmittance = exp(-medium.extinction * dt);
            const float3 extinction = max(medium.extinction, 1e-6f);
            const float3 light = medium.scattering * isotropic * diffuse_transmittance_to_space(a, p, sun);
            luminance += throughput * (light - light * step_transmittance) / extinction;
            medium_transfer += throughput * (medium.scattering - medium.scattering * step_transmittance) / extinction;
            throughput *= step_transmittance;
        }

        // The lit ground (Lambertian) where the ray ends on it
        if (ground >= 0.0f)
        {
            const float3 p = position + direction * ground;
            const float3 normal = normalize(p);
            luminance += throughput * diffuse_transmittance_to_space(a, p + normal * 1e-3f, sun) * saturate(dot(normal, sun)) * a.ground_albedo * INV_PI;
        }

        second_order += luminance;
        transfer += medium_transfer;
    }

    // Each direction weighs (4 pi / count) x isotropic phase (1 / 4 pi): the mean
    second_order /= count;
    transfer /= count;
    const float3 psi = second_order / max(1.0f - transfer, 1e-3f);
    tex_uav[thread_id.xy] = float4(psi, 1.0f);
#endif
}
