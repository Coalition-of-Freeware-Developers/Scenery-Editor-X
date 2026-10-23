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
 * aerial_perspective.slang
 * -------------------------------------------------------
 * Created: 20/10/2026
 * -------------------------------------------------------
 *
 * Aerial perspective volumes (plan TASK-045, Hillaire 2020 section 5.4; XP12: scatter_compute_aerial): 32x32 froxel
 * columns over the screen, 32 slices out to atmosphere_view.x meters (spread as the square of the depth, see
 * aerial_slice_depth), each holding the sun's and moon's in-scattering (pre-exposed) and the transmittance between the
 * camera and the slice. One thread marches one column front to back.
 *   tex3d_uav   lut_aerial_inscatter;  tex3d_uav2  lut_aerial_transmittance;  reads tex_transmittance, tex_multiscatter
 */

import resources;
import constants;
import position;
import atmosphere_common;

static const float STEPS_PER_SLICE = 2.0f;

[shader("compute")]
[numthreads(8, 8, 1)]
void main_comp(uint3 thread_id: SV_DispatchThreadID)
{
    float3 resolution;
    tex3d_uav.GetDimensions(resolution.x, resolution.y, resolution.z);
    if (any(thread_id.xy >= uint2(resolution.xy)))
        return;
    const float2 uv = (float2(thread_id.xy) + 0.5f) / resolution.xy;
    const AtmosphereParameters a = get_atmosphere();
    const float3 camera = get_camera_atmosphere_position(a);
    const float3 direction = normalize(get_position(0.5f, uv) - get_camera_position());
    const float ray_end = atmosphere_ray_length(a, camera, direction);
    const float pre_exposure = get_pre_exposure();

    float3 luminance = 0.0f;
    float3 transmittance = 1.0f;
    float t = 0.0f;
    for (uint slice = 0; slice < uint(resolution.z); ++slice)
    {
        // March from the previous slice to this one (stopping at the ground or space)
        const float t_end = min(aerial_slice_depth(float(slice)), ray_end);
        const float dt = max(t_end - t, 0.0f) / STEPS_PER_SLICE;
        for (float s = 0.0f; s < STEPS_PER_SLICE && dt > 0.0f; s += 1.0f)
        {
            const float3 p = camera + direction * (t + (s + 0.5f) * dt);
            const MediumSample medium = sample_medium(a, p);
            const float3 step_transmittance = exp(-medium.extinction * dt);
            const float3 scattering = sky_scattering(a, p, medium, direction);
            luminance += transmittance * (scattering - scattering * step_transmittance) / max(medium.extinction, 1e-6f);
            transmittance *= step_transmittance;
        }
        t = max(t, t_end);

        tex3d_uav[uint3(thread_id.xy, slice)] = float4(luminance * pre_exposure, 1.0f);
        tex3d_uav2[uint3(thread_id.xy, slice)] = float4(transmittance, 1.0f);
    }
}
