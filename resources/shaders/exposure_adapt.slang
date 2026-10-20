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
 * exposure_adapt.slang
 * -------------------------------------------------------
 * Created: 20/10/2026
 * -------------------------------------------------------
 *
 * Exposure metering, step 2 (plan TASK-038): one group of 256 threads, one per histogram bin. The mean log2 luminance
 * of the counts between the trims (exposure_adapt.zw, fractions of the total) is the metered luminance; EV100 =
 * log2(L x 100 / K), clamped to the range, minus the bias table and the compensation, is the target; the exposure
 * moves toward it at the adaptation speeds. The same steps as renderer/exposure.cpp (MeterHistogram, GetTargetEv,
 * AdaptEv), which the unit tests check.
 *   exposure_histogram  read, then cleared for the next frame
 *   exposure_state      EXPOSURE_STATE_* (resources.slang); the CPU reads EXPOSURE_STATE_EV back for the pre-exposure
 *   pass_get_f4_value(): x 1 to jump to the target (reset), y manual EV100 (used when z > 0.5), z manual mode
 */

import resources;
import constants;

groupshared float g_prefix[EXPOSURE_HISTOGRAM_BINS];
groupshared float g_sum[EXPOSURE_HISTOGRAM_BINS];
groupshared float g_weight[EXPOSURE_HISTOGRAM_BINS];

float get_bias(float ev)
{
    const float x[5] = { buffer_environment.exposure_bias_ev.x, buffer_environment.exposure_bias_ev.y, buffer_environment.exposure_bias_ev.z,
                         buffer_environment.exposure_bias_ev.w, buffer_environment.exposure_bias_last.x };
    const float y[5] = { buffer_environment.exposure_bias.x, buffer_environment.exposure_bias.y, buffer_environment.exposure_bias.z,
                         buffer_environment.exposure_bias.w, buffer_environment.exposure_bias_last.y };
    if (ev <= x[0])
        return y[0];
    for (uint i = 1; i < 5; ++i)
    {
        if (ev <= x[i])
            return lerp(y[i - 1], y[i], (ev - x[i - 1]) / max(x[i] - x[i - 1], 1e-6f));
    }
    return y[4];
}

[shader("compute")]
[numthreads(EXPOSURE_HISTOGRAM_BINS, 1, 1)]
void main_comp(uint bin: SV_GroupIndex)
{
    const float count = float(exposure_histogram[bin]);
    exposure_histogram[bin] = 0;

    // Inclusive prefix sum of the counts (Hillis-Steele)
    g_prefix[bin] = count;
    GroupMemoryBarrierWithGroupSync();
    for (uint offset = 1; offset < EXPOSURE_HISTOGRAM_BINS; offset *= 2)
    {
        const float before = bin >= offset ? g_prefix[bin - offset] : 0.0f;
        GroupMemoryBarrierWithGroupSync();
        g_prefix[bin] += before;
        GroupMemoryBarrierWithGroupSync();
    }

    // The part of this bin's count between the trims, at the bin's centre
    const float total = g_prefix[EXPOSURE_HISTOGRAM_BINS - 1];
    const float low = total * saturate(buffer_environment.exposure_adapt.z);
    const float high = total * saturate(buffer_environment.exposure_adapt.w);
    const float cumulative = g_prefix[bin] - count;
    const float part = max(0.0f, min(cumulative + count, high) - max(cumulative, low));
    const float bin_width = (EXPOSURE_LOG2_MAX - EXPOSURE_LOG2_MIN) / EXPOSURE_HISTOGRAM_BINS;
    g_sum[bin] = part * (EXPOSURE_LOG2_MIN + (bin + 0.5f) * bin_width);
    g_weight[bin] = part;
    GroupMemoryBarrierWithGroupSync();

    for (uint stride = EXPOSURE_HISTOGRAM_BINS / 2; stride > 0; stride /= 2)
    {
        if (bin < stride)
        {
            g_sum[bin] += g_sum[bin + stride];
            g_weight[bin] += g_weight[bin + stride];
        }
        GroupMemoryBarrierWithGroupSync();
    }

    if (bin != 0)
        return;

    const float4 controls = pass_get_f4_value();
    const float4 meter = buffer_environment.exposure_meter;	// EV100 min, max, K, radius
    const float4 adapt = buffer_environment.exposure_adapt;	// speed up, speed down, trims

    const bool metered = g_weight[0] > 0.0f;
    const float mean_log2 = metered ? g_sum[0] / g_weight[0] : EXPOSURE_LOG2_MIN;
    const float raw_ev = mean_log2 + log2(100.0f / max(meter.z, 1e-3f));
    const float clamped_ev = clamp(raw_ev, meter.x, meter.y);
    const float target = controls.z > 0.5f ? controls.y : clamped_ev - get_bias(clamped_ev) - buffer_environment.exposure_bias_last.z;

    const float current = exposure_state[EXPOSURE_STATE_EV];
    const bool valid = exposure_state[EXPOSURE_STATE_VALID] > 0.5f && isfinite(current) && controls.x < 0.5f;
    const float dt = clamp(get_delta_time_seconds(), 0.0f, 0.25f);
    const float next = valid ? current + clamp(target - current, -adapt.y * dt, adapt.x * dt) : target;

    exposure_state[EXPOSURE_STATE_EV] = next;
    exposure_state[EXPOSURE_STATE_METERED] = clamped_ev;
    exposure_state[EXPOSURE_STATE_TARGET] = target;
    exposure_state[EXPOSURE_STATE_MEAN_LOG2] = mean_log2;
    exposure_state[EXPOSURE_STATE_VALID] = 1.0f;
    exposure_state[EXPOSURE_STATE_RAW] = raw_ev;
}
