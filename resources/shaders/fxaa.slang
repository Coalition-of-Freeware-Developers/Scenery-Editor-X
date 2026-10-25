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
 * fxaa.slang
 * -------------------------------------------------------
 * Created: 18/04/2026
 * -------------------------------------------------------
 *
 * Fast approximate anti-aliasing after the tone map (plan TASK-044), the steps of Timothy Lottes' FXAA 3.11 quality
 * preset: find edges by local luma contrast, decide whether each runs horizontally or vertically, walk along it in
 * both directions to its ends, and blend toward the neighbour across it by how far this pixel is from the nearer end
 * (plus a sub-pixel term for isolated texels).
 *   tex      the tone-mapped frame (display-referred linear: luma is taken perceptual, as a square root)
 *   tex_uav  output
 */

import resources;
import constants;

static const float3 LUMA = float3(0.2126f, 0.7152f, 0.0722f);
static const float EDGE_THRESHOLD = 0.125f;		// local contrast an edge needs, relative to the brightest neighbour
static const float EDGE_THRESHOLD_MIN = 0.0312f;	// and at least this much (skips noise in the dark)
static const float SUBPIXEL_QUALITY = 0.75f;
static const uint SEARCH_STEPS = 12;
static const float SEARCH_STEP_SIZES[SEARCH_STEPS] = { 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.5f, 2.0f, 2.0f, 2.0f, 2.0f, 4.0f, 8.0f };

float luma_at(float2 uv)
{
    return sqrt(dot(max(tex.SampleLevel(samplers[sampler_bilinear_clamp], uv, 0.0f).rgb, 0.0f), LUMA));
}

float luma_load(int2 pixel, int2 size)
{
    return sqrt(dot(max(tex.Load(int3(clamp(pixel, int2(0, 0), size - 1), 0)).rgb, 0.0f), LUMA));
}

[shader("compute")]
[numthreads(THREAD_GROUP_COUNT_X, THREAD_GROUP_COUNT_Y, 1)]
void main_comp(uint3 thread_id: SV_DispatchThreadID)
{
    float2 resolution;
    tex_uav.GetDimensions(resolution.x, resolution.y);
    if (any(thread_id.xy >= uint2(resolution)))
        return;

    const int2 size = int2(resolution);
    const int2 p = int2(thread_id.xy);
    const float2 rcp_resolution = 1.0f / resolution;
    const float2 uv = (float2(p) + 0.5f) * rcp_resolution;
    const float4 center = tex.Load(int3(p, 0));

    // Up is -y in texture space
    const float m = sqrt(dot(max(center.rgb, 0.0f), LUMA));
    const float up = luma_load(p + int2(0, -1), size);
    const float down = luma_load(p + int2(0, 1), size);
    const float left = luma_load(p + int2(-1, 0), size);
    const float right = luma_load(p + int2(1, 0), size);
    const float luma_min = min(m, min(min(up, down), min(left, right)));
    const float luma_max = max(m, max(max(up, down), max(left, right)));
    const float range = luma_max - luma_min;
    if (range < max(EDGE_THRESHOLD_MIN, luma_max * EDGE_THRESHOLD))
    {
        tex_uav[thread_id.xy] = center;
        return;
    }

    const float up_left = luma_load(p + int2(-1, -1), size);
    const float up_right = luma_load(p + int2(1, -1), size);
    const float down_left = luma_load(p + int2(-1, 1), size);
    const float down_right = luma_load(p + int2(1, 1), size);

    // Edge orientation from the second derivatives across both axes
    const float edge_horizontal = abs(up_left + up_right - 2.0f * up) + 2.0f * abs(left + right - 2.0f * m) + abs(down_left + down_right - 2.0f * down);
    const float edge_vertical = abs(up_left + down_left - 2.0f * left) + 2.0f * abs(up + down - 2.0f * m) + abs(up_right + down_right - 2.0f * right);
    const bool horizontal = edge_horizontal >= edge_vertical;

    // The side of the edge with the steeper gradient
    const float luma1 = horizontal ? up : left;
    const float luma2 = horizontal ? down : right;
    const float gradient1 = luma1 - m;
    const float gradient2 = luma2 - m;
    const bool steeper1 = abs(gradient1) >= abs(gradient2);
    const float gradient_scaled = 0.25f * max(abs(gradient1), abs(gradient2));

    float step_length = horizontal ? rcp_resolution.y : rcp_resolution.x;
    float luma_local_average;
    if (steeper1)
    {
        step_length = -step_length;
        luma_local_average = 0.5f * (luma1 + m);
    }
    else
    {
        luma_local_average = 0.5f * (luma2 + m);
    }

    // Walk along the edge, half a pixel toward the steeper side, until the luma leaves the edge's average
    float2 edge_uv = uv;
    if (horizontal)
        edge_uv.y += step_length * 0.5f;
    else
        edge_uv.x += step_length * 0.5f;
    const float2 along = horizontal ? float2(rcp_resolution.x, 0.0f) : float2(0.0f, rcp_resolution.y);

    float2 uv1 = edge_uv - along;
    float2 uv2 = edge_uv + along;
    float end1 = luma_at(uv1) - luma_local_average;
    float end2 = luma_at(uv2) - luma_local_average;
    bool reached1 = abs(end1) >= gradient_scaled;
    bool reached2 = abs(end2) >= gradient_scaled;
    for (uint i = 1; i < SEARCH_STEPS && !(reached1 && reached2); ++i)
    {
        if (!reached1)
        {
            uv1 -= along * SEARCH_STEP_SIZES[i];
            end1 = luma_at(uv1) - luma_local_average;
            reached1 = abs(end1) >= gradient_scaled;
        }
        if (!reached2)
        {
            uv2 += along * SEARCH_STEP_SIZES[i];
            end2 = luma_at(uv2) - luma_local_average;
            reached2 = abs(end2) >= gradient_scaled;
        }
    }

    // Blend by the distance to the nearer end, if the luma there varies the right way
    const float distance1 = horizontal ? uv.x - uv1.x : uv.y - uv1.y;
    const float distance2 = horizontal ? uv2.x - uv.x : uv2.y - uv.y;
    const bool nearer1 = distance1 < distance2;
    const float edge_length = distance1 + distance2;
    const float pixel_offset = 0.5f - min(distance1, distance2) / max(edge_length, 1e-6f);
    const bool center_smaller = m < luma_local_average;
    const bool correct_variation = ((nearer1 ? end1 : end2) < 0.0f) != center_smaller;
    float offset = correct_variation ? pixel_offset : 0.0f;

    // Sub-pixel aliasing: an isolated texel against its 3x3 neighbourhood
    const float average = (2.0f * (up + down + left + right) + up_left + up_right + down_left + down_right) / 12.0f;
    const float subpixel = saturate(abs(average - m) / range);
    const float subpixel_smooth = (-2.0f * subpixel + 3.0f) * subpixel * subpixel;
    offset = max(offset, subpixel_smooth * subpixel_smooth * SUBPIXEL_QUALITY);

    float2 final_uv = uv;
    if (horizontal)
        final_uv.y += offset * step_length;
    else
        final_uv.x += offset * step_length;
    tex_uav[thread_id.xy] = float4(tex.SampleLevel(samplers[sampler_bilinear_clamp], final_uv, 0.0f).rgb, center.a);
}
