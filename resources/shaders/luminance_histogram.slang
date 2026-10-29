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
 * luminance_histogram.slang
 * -------------------------------------------------------
 * Created: 20/10/2026
 * -------------------------------------------------------
 *
 * Exposure metering, step 1 (plan TASK-038): a 256-bin histogram of log2 luminance (nits, -2..18) over the frame at
 * half resolution, each sample the average of a 2x2 block. Samples within exposure_meter.w screen heights of the
 * centre count 4 times (X-Plane's _exp_radius). exposure_adapt.slang meters and clears it.
 *   tex                 frame_render (pre-exposed: divided by get_pre_exposure() back to nits)
 *   exposure_histogram  256 counts
 * Dispatch: ceil(width / 2 / 16) x ceil(height / 2 / 16) groups.
 */

import resources;
import constants;

static const float3 LUMA = float3(0.2126f, 0.7152f, 0.0722f);
static const uint CENTRE_WEIGHT = 4;

groupshared uint g_bins[EXPOSURE_HISTOGRAM_BINS];

uint get_bin(float nits)
{
    if (!(nits > 0.0f))
        return 0;
    const float t = (log2(nits) - EXPOSURE_LOG2_MIN) / (EXPOSURE_LOG2_MAX - EXPOSURE_LOG2_MIN);
    return uint(clamp(t * EXPOSURE_HISTOGRAM_BINS, 0.0f, EXPOSURE_HISTOGRAM_BINS - 1.0f));
}

[shader("compute")]
[numthreads(16, 16, 1)]
void main_comp(uint3 thread_id: SV_DispatchThreadID, uint group_index: SV_GroupIndex)
{
    g_bins[group_index] = 0;
    GroupMemoryBarrierWithGroupSync();

    float2 resolution;
    tex.GetDimensions(resolution.x, resolution.y);
    const uint2 half_resolution = uint2(ceil(resolution * 0.5f));
    if (all(thread_id.xy < half_resolution))
    {
        // The corner a 2x2 block shares: bilinear filtering averages the block
        const float2 uv = min((float2(thread_id.xy) * 2.0f + 1.0f) / resolution, 1.0f);
        const float3 color = tex.SampleLevel(samplers[sampler_bilinear_clamp], uv, 0.0f).rgb;
        const float nits = dot(color, LUMA) / max(get_pre_exposure(), 1e-30f);

        const float aspect = resolution.x / max(resolution.y, 1.0f);
        const float centre_distance = length((uv - 0.5f) * float2(aspect, 1.0f)); // screen heights
        const uint weight = centre_distance < buffer_environment.exposure_meter.w ? CENTRE_WEIGHT : 1;
        InterlockedAdd(g_bins[get_bin(nits)], weight);
    }
    GroupMemoryBarrierWithGroupSync();

    const uint count = g_bins[group_index];
    if (count > 0)
        InterlockedAdd(exposure_histogram[group_index], count);
}
