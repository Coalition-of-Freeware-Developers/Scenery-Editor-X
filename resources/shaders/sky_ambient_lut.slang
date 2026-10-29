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
 * sky_ambient_lut.slang
 * -------------------------------------------------------
 * Created: 20/10/2026
 * -------------------------------------------------------
 *
 * Sky irradiance on a level surface (plan TASK-045): 64x16, by the light's cos zenith (x) and height above the ground
 * (y, squared: finer near the ground), for a white light of unit illuminance outside the atmosphere. Light is linear:
 * the sun's and the moon's ambient light are this times their illuminance. Absolute (not pre-exposed): the environment
 * probes and the ground below the horizon read it.
 *   tex_uav  lut_sky_ambient;  reads tex_transmittance, tex_multiscatter
 */

import resources;
import constants;
import atmosphere_common;

static const uint DIRECTIONS_SQRT = 8;
static const float STEPS = 16.0f;

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

    const float cos_light_zenith = from_sub_uv_to_unit(uv.x, SKY_AMBIENT_LUT_SIZE.x) * 2.0f - 1.0f;
    const float h = saturate(from_sub_uv_to_unit(uv.y, SKY_AMBIENT_LUT_SIZE.y));
    const float3 position = float3(0.0f, a.bottom_radius + 1e-3f + h * h * (a.top_radius - a.bottom_radius), 0.0f);
    const float3 light = float3(sqrt(saturate(1.0f - cos_light_zenith * cos_light_zenith)), cos_light_zenith, 0.0f);

    // Cosine-weighted directions over the upper hemisphere: irradiance = pi x their mean radiance
    float3 sum = 0.0f;
    const uint count = DIRECTIONS_SQRT * DIRECTIONS_SQRT;
    for (uint i = 0; i < count; ++i)
    {
        const float u = (float(i % DIRECTIONS_SQRT) + 0.5f) / DIRECTIONS_SQRT;
        const float v = (float(i / DIRECTIONS_SQRT) + 0.5f) / DIRECTIONS_SQRT;
        const float sin_theta = sqrt(v);
        const float phi = 2.0f * PI * u;
        const float3 direction = float3(sin_theta * cos(phi), sqrt(saturate(1.0f - v)), sin_theta * sin(phi));

        const float length_km = atmosphere_ray_length(a, position, direction);
        const float dt = length_km / STEPS;
        float3 throughput = 1.0f;
        for (float s = 0.0f; s < STEPS; s += 1.0f)
        {
            const float3 p = position + direction * ((s + 0.3f) * dt);
            const MediumSample medium = sample_medium(a, p);
            const float3 step_transmittance = exp(-medium.extinction * dt);
            const float3 scattering = light_scattering(a, p, medium, direction, light, 1.0f);
            sum += throughput * (scattering - scattering * step_transmittance) / max(medium.extinction, 1e-6f);
            throughput *= step_transmittance;
        }
    }
    tex_uav[thread_id.xy] = float4(sum * (PI / count), 1.0f);
}
