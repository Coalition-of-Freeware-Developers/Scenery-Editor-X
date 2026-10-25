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
 * sky_view_lut.slang
 * -------------------------------------------------------
 * Created: 20/10/2026
 * -------------------------------------------------------
 *
 * The sky seen from the camera (plan TASK-045, Hillaire 2020 section 5.3; XP12: scatter_compute_sky): 192x108, every
 * frame, the in-scattered radiance of the sun and the moon (single scattering, the planet's shadow, the multiple-
 * scattering LUT) to the top of the atmosphere or the ground, pre-exposed. Directions: sky_view_params_to_uv.
 *   tex_uav  lut_sky_view;  reads tex_transmittance, tex_multiscatter
 */

import resources;
import constants;
import atmosphere_common;

static const float SAMPLES_MIN = 12.0f;
static const float SAMPLES_MAX = 32.0f;

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
    const float3 camera = get_camera_atmosphere_position(a);

    float cos_view_zenith, azimuth;
    uv_to_sky_view_params(a, uv, length(camera), cos_view_zenith, azimuth);
    const float3 direction = direction_from_zenith_azimuth(cos_view_zenith, azimuth);

    // Longer rays take more samples (a few hundred km at the horizon)
    const float length_km = atmosphere_ray_length(a, camera, direction);
    const float samples = floor(lerp(SAMPLES_MIN, SAMPLES_MAX, saturate(length_km / 150.0f)));
    float3 luminance, transmittance;
    integrate_sky(a, camera, direction, length_km, samples, luminance, transmittance);
    tex_uav[thread_id.xy] = float4(luminance * get_pre_exposure(), 1.0f);
}
