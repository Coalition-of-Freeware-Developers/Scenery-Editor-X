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
 * reference_overlay.slang
 * -------------------------------------------------------
 * Created: 20/10/2026
 * -------------------------------------------------------
 *
 * Draws a reference image (an X-Plane 12 screenshot) over frame_output for the viewport REFERENCE menu.
 *   tex     the reference, RGBA8 UNORM holding sRGB values, row 0 at the bottom (TextureLoader orientation)
 *   tex_uav frame_output, display-referred linear RGB
 *   pass_get_f4_value(): x mode (1 reference, 2 split, 3 difference), y split position (0-1 of the width, the
 *   reference shows left of it), z difference gain
 */

import common;
import resources;
import constants;

static const uint MODE_REFERENCE = 1;
static const uint MODE_SPLIT = 2;
static const uint MODE_DIFFERENCE = 3;

float3 decode_srgb(float3 c)
{
    return select(c <= 0.04045, c / 12.92, pow((c + 0.055) / 1.055, 2.4));
}

float3 encode_srgb(float3 c)
{
    c = saturate(c);
    return select(c <= 0.0031308, c * 12.92, 1.055 * pow(c, 1.0 / 2.4) - 0.055);
}

[shader("compute")]
[numthreads(THREAD_GROUP_COUNT_X, THREAD_GROUP_COUNT_Y, 1)]
void main_comp(uint3 thread_id: SV_DispatchThreadID)
{
    float2 resolution;
    tex_uav.GetDimensions(resolution.x, resolution.y);
    if (any(thread_id.xy >= uint2(resolution)))
        return;

    const float4 params = pass_get_f4_value();
    const uint mode = uint(params.x + 0.5);
    const float2 uv = (float2(thread_id.xy) + 0.5) / resolution;

    // The reference is stored bottom row first
    const float3 reference_srgb = tex.SampleLevel(samplers[sampler_bilinear_clamp], float2(uv.x, 1.0 - uv.y), 0).rgb;
    const float4 frame = tex_uav[thread_id.xy];

    float3 result = frame.rgb;
    if (mode == MODE_REFERENCE || (mode == MODE_SPLIT && uv.x < params.y))
    {
        result = decode_srgb(reference_srgb);
    }
    else if (mode == MODE_DIFFERENCE)
    {
        // |a - b| x gain on the values a viewer sees (sRGB-encoded), shown as such
        const float3 difference = saturate(abs(encode_srgb(frame.rgb) - reference_srgb) * params.z);
        result = decode_srgb(difference);
    }

    // The split line: one pixel wide, white
    if (mode == MODE_SPLIT && abs(float(thread_id.x) + 0.5 - params.y * resolution.x) < 0.75)
        result = float3(1.0, 1.0, 1.0);

    tex_uav[thread_id.xy] = float4(result, frame.a);
}
