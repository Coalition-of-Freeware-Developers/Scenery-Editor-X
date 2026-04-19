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
 * auto_exposure.slang
 * -------------------------------------------------------
 * Created: 18/04/2026
 * -------------------------------------------------------
 */
import common;

[shader("compute")]
[numthreads(1, 1, 1)]
void main_comp(uint3 thread_id : SV_DispatchThreadID)
{
    // 1. compute a center-weighted log average from a small mip grid
    float avg_nits = 0.0f;
    {
        uint w, h, mip_count;
        tex.GetDimensions(0, w, h, mip_count);

        // use a small grid instead of the final 1x1 average so localized highlights
        // don't dominate the meter.
        uint mip = 0;
        const uint target_metering_resolution = 16;

        while ((mip + 1) < mip_count && (w > target_metering_resolution || h > target_metering_resolution))
        {
            mip++;
            tex.GetDimensions(mip, w, h, mip_count);
        }

        tex.GetDimensions(mip, w, h, mip_count);

        const float min_luminance_nits      = 0.0001f;
        const float highlight_start_nits    = 1500.0f;
        const float highlight_end_nits      = 10000.0f;
        const float min_highlight_weight    = 0.25f;
        const float edge_weight             = 0.15f;

        float weighted_log_sum = 0.0f;
        float weight_sum       = 0.0f;

        for (uint y = 0; y < h; y++)
        {
            for (uint x = 0; x < w; x++)
            {
                float3 col = tex.Load(int3(x, y, mip)).rgb;

                float lum_nits = max(dot(radiometric_to_photometric(col), float3(0.2126f, 0.7152f, 0.0722f)), min_luminance_nits);

                float2 uv = (float2(x + 0.5f, y + 0.5f) / float2(w, h)) * 2.0f - 1.0f;
                float center_weight = lerp(edge_weight, 1.0f, exp(-dot(uv, uv) * 1.5f));

                // keep very hot highlights from dragging the meter too hard.
                float highlight_weight = 1.0f - smoothstep(highlight_start_nits, highlight_end_nits, lum_nits);
                float sample_weight    = center_weight * lerp(min_highlight_weight, 1.0f, highlight_weight);

                weighted_log_sum += log2(lum_nits) * sample_weight;
                weight_sum       += sample_weight;
            }
        }
        avg_nits = exp2(weighted_log_sum / max(weight_sum, 0.000001f));
    }

    // 2. keep the existing camera-relative metering so artistic exposure choices remain intact
    float camera_exposure   = max(buffer_frame.camera_exposure, 0.000001f);
    float current_brightness = avg_nits * camera_exposure;

    // 3. target middle gray using the resolved exposure that will be promoted for the next frame.
    // the current frame keeps using the previous resolved exposure so post-processing and upscaling stay in sync.
    const float target_luminance = 0.18f;

    // 4. compute the camera-relative auto exposure trim
    float desired_exposure = target_luminance / max(current_brightness, 0.000001f);

    // 5. clamp the auto exposure trim to a practical range
    const float min_ev = -6.0f;
    const float max_ev =  6.0f;

    float min_exposure = exp2(min_ev);
    float max_exposure = exp2(max_ev);

    desired_exposure = clamp(desired_exposure, min_exposure, max_exposure);

    // 6. resolve the final exposure and adapt it in ev space so large changes remain stable
    float desired_total_exposure = camera_exposure * desired_exposure;
    float prev_exposure = tex2.Load(int3(0, 0, 0)).r;
    float adaptation_speed = pass_get_f3_value().x;

    // start from the current target to avoid a first-frame flash
    if (isnan(prev_exposure) || prev_exposure <= 0.0f)
    {
        prev_exposure = desired_total_exposure;
    }

    // higher values should adapt faster, so the response scales with the user-controlled speed directly
    float prev_ev          = log2(max(prev_exposure, 0.000001f));
    float desired_ev       = log2(max(desired_total_exposure, 0.000001f));
    float adaptation_alpha = 1.0f - exp(-max(adaptation_speed, 0.0f) * 4.0f * buffer_frame.delta_time);
    float exposure         = exp2(lerp(prev_ev, desired_ev, adaptation_alpha));

    // write output
    tex_uav[uint2(0, 0)] = float4(exposure, exposure, exposure, 1.0f);
}
