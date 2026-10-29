/**
 * -------------------------------------------------------
 * Scenery Editor X
 * -------------------------------------------------------
 * fsr1_easu.slang
 * -------------------------------------------------------
 * Created: 20/10/2026
 * -------------------------------------------------------
 *
 * AMD FidelityFX Super Resolution 1.0, Edge Adaptive Spatial Upsampling (EASU), ported to Slang from ffx_fsr1.h
 * (plan TASK-044). The 12-tap kernel, the direction and length analysis and the Lanczos-2 approximation follow AMD's
 * FsrEasuF / FsrEasuSetF / FsrEasuTapF; texels are loaded one by one instead of with gathers, in 32-bit float.
 *   tex      input (display-referred linear; the kernel runs on perceptual values, square roots)
 *   tex_uav  output, perceptual (fsr1_rcas.slang squares it back)
 *
 * Copyright (c) 2021 Advanced Micro Devices, Inc. All rights reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

import resources;
import constants;

static int2 s_size;

float3 load_perceptual(int2 pixel)
{
    return sqrt(max(tex.Load(int3(clamp(pixel, int2(0, 0), s_size - 1), 0)).rgb, 0.0f));
}

// Luma times 2 (FSR: 0.5 R + G + 0.5 B)
float luma2(float3 c) { return c.b * 0.5f + (c.r * 0.5f + c.g); }

// Accumulates direction and length for one of the four bilinear corners
//    a
//  b c d
//    e
void easu_set(inout float2 dir, inout float len, float w, float la, float lb, float lc, float ld, float le)
{
    float dc = ld - lc;
    float cb = lc - lb;
    float len_x = max(abs(dc), abs(cb));
    len_x = 1.0f / max(len_x, 1e-8f);
    float dir_x = ld - lb;
    dir.x += dir_x * w;
    len_x = saturate(abs(dir_x) * len_x);
    len_x *= len_x;
    len += len_x * w;

    float ec = le - lc;
    float ca = lc - la;
    float len_y = max(abs(ec), abs(ca));
    len_y = 1.0f / max(len_y, 1e-8f);
    float dir_y = le - la;
    dir.y += dir_y * w;
    len_y = saturate(abs(dir_y) * len_y);
    len_y *= len_y;
    len += len_y * w;
}

// One tap of the approximated, rotated and stretched Lanczos-2 kernel
void easu_tap(inout float3 color_sum, inout float weight_sum, float2 offset, float2 dir, float2 len, float lob, float clp, float3 color)
{
    float2 v;
    v.x = offset.x * dir.x + offset.y * dir.y;
    v.y = offset.x * -dir.y + offset.y * dir.x;
    v *= len;
    float d2 = min(v.x * v.x + v.y * v.y, clp);

    // (25/16 * (2/5 * x^2 - 1)^2 - (25/16 - 1)) * (lob * x^2 - 1)^2
    float wb = 2.0f / 5.0f * d2 - 1.0f;
    float wa = lob * d2 - 1.0f;
    wb *= wb;
    wa *= wa;
    wb = 25.0f / 16.0f * wb - (25.0f / 16.0f - 1.0f);
    float w = wb * wa;
    color_sum += color * w;
    weight_sum += w;
}

[shader("compute")]
[numthreads(THREAD_GROUP_COUNT_X, THREAD_GROUP_COUNT_Y, 1)]
void main_comp(uint3 thread_id: SV_DispatchThreadID)
{
    float2 output_size;
    tex_uav.GetDimensions(output_size.x, output_size.y);
    if (any(thread_id.xy >= uint2(output_size)))
        return;
    float2 input_size;
    tex.GetDimensions(input_size.x, input_size.y);
    s_size = int2(input_size);

    // Position in the input, relative to texel centres
    float2 pp = (float2(thread_id.xy) + 0.5f) * (input_size / output_size) - 0.5f;
    const float2 fp = floor(pp);
    pp -= fp;
    const int2 f0 = int2(fp);

    //    b c
    //  e f g h
    //  i j k l
    //    n o
    const float3 b = load_perceptual(f0 + int2(0, -1));
    const float3 c = load_perceptual(f0 + int2(1, -1));
    const float3 e = load_perceptual(f0 + int2(-1, 0));
    const float3 f = load_perceptual(f0);
    const float3 g = load_perceptual(f0 + int2(1, 0));
    const float3 h = load_perceptual(f0 + int2(2, 0));
    const float3 i = load_perceptual(f0 + int2(-1, 1));
    const float3 j = load_perceptual(f0 + int2(0, 1));
    const float3 k = load_perceptual(f0 + int2(1, 1));
    const float3 l = load_perceptual(f0 + int2(2, 1));
    const float3 n = load_perceptual(f0 + int2(0, 2));
    const float3 o = load_perceptual(f0 + int2(1, 2));

    const float bl = luma2(b), cl = luma2(c), el = luma2(e), fl = luma2(f), gl = luma2(g), hl = luma2(h);
    const float il = luma2(i), jl = luma2(j), kl = luma2(k), ll = luma2(l), nl = luma2(n), ol = luma2(o);

    // Direction and length from the four bilinear corners
    float2 dir = 0.0f;
    float len = 0.0f;
    easu_set(dir, len, (1.0f - pp.x) * (1.0f - pp.y), bl, el, fl, gl, jl);
    easu_set(dir, len, pp.x * (1.0f - pp.y), cl, fl, gl, hl, kl);
    easu_set(dir, len, (1.0f - pp.x) * pp.y, fl, il, jl, kl, nl);
    easu_set(dir, len, pp.x * pp.y, gl, jl, kl, ll, ol);

    // Normalize, cleaning up near zero
    const float2 dir2 = dir * dir;
    float dir_r = dir2.x + dir2.y;
    const bool zero = dir_r < 1.0f / 32768.0f;
    dir_r = zero ? 1.0f : rsqrt(dir_r);
    dir.x = zero ? 1.0f : dir.x;
    dir *= dir_r;

    // {0 to 2} to {0 to 1}, shaped with a square
    len = len * 0.5f;
    len *= len;

    // Stretch the kernel from 1 (horizontal / vertical) to sqrt(2) (diagonal)
    const float stretch = (dir.x * dir.x + dir.y * dir.y) / max(max(abs(dir.x), abs(dir.y)), 1e-8f);
    const float2 len2 = float2(1.0f + (stretch - 1.0f) * len, 1.0f - 0.5f * len);
    // The window: +/- sqrt(2) to a little past 2 with the amount of edge
    const float lob = 0.5f + ((1.0f / 4.0f - 0.04f) - 0.5f) * len;
    const float clp = 1.0f / lob;

    // De-ringing bounds: the four nearest texels
    const float3 min4 = min(min(f, g), min(j, k));
    const float3 max4 = max(max(f, g), max(j, k));

    float3 color_sum = 0.0f;
    float weight_sum = 0.0f;
    easu_tap(color_sum, weight_sum, float2(0.0f, -1.0f) - pp, dir, len2, lob, clp, b);
    easu_tap(color_sum, weight_sum, float2(1.0f, -1.0f) - pp, dir, len2, lob, clp, c);
    easu_tap(color_sum, weight_sum, float2(-1.0f, 1.0f) - pp, dir, len2, lob, clp, i);
    easu_tap(color_sum, weight_sum, float2(0.0f, 1.0f) - pp, dir, len2, lob, clp, j);
    easu_tap(color_sum, weight_sum, float2(0.0f, 0.0f) - pp, dir, len2, lob, clp, f);
    easu_tap(color_sum, weight_sum, float2(-1.0f, 0.0f) - pp, dir, len2, lob, clp, e);
    easu_tap(color_sum, weight_sum, float2(1.0f, 1.0f) - pp, dir, len2, lob, clp, k);
    easu_tap(color_sum, weight_sum, float2(2.0f, 1.0f) - pp, dir, len2, lob, clp, l);
    easu_tap(color_sum, weight_sum, float2(2.0f, 0.0f) - pp, dir, len2, lob, clp, h);
    easu_tap(color_sum, weight_sum, float2(1.0f, 0.0f) - pp, dir, len2, lob, clp, g);
    easu_tap(color_sum, weight_sum, float2(1.0f, 2.0f) - pp, dir, len2, lob, clp, o);
    easu_tap(color_sum, weight_sum, float2(0.0f, 2.0f) - pp, dir, len2, lob, clp, n);

    const float3 color = min(max4, max(min4, color_sum / weight_sum));
    tex_uav[thread_id.xy] = float4(color, 1.0f);
}
