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
 * downsample_min.slang
 * -------------------------------------------------------
 * Created: 18/10/2026
 * -------------------------------------------------------
 */
import common;
import resources;
import constants;

// One level of a min mip chain (Renderer::Pass_Downscale with Renderer_DownsampleFilter::Min while the FidelityFX SPD
// shaders aren't ported): tex is the level above (a single-mip view), tex_uav the level written. Each texel takes the
// smallest value of the 2 x 2 texels it covers; on the last row / column of an odd-sized level it also takes the odd
// one out, so every texel of the level above lands in exactly the texel min(p / 2, size - 1). For a reverse-Z depth
// pyramid that is the farthest depth under each texel, what occlusion culling compares against (gpu_cull.slang).

[shader("compute")]
[numthreads(THREAD_GROUP_COUNT_X, THREAD_GROUP_COUNT_Y, 1)]
void main_comp(uint3 thread_id : SV_DispatchThreadID)
{
    uint2 size_out;
    tex_uav.GetDimensions(size_out.x, size_out.y);
    if (any(thread_id.xy >= size_out))
        return;

    uint2 size_in;
    tex.GetDimensions(size_in.x, size_in.y);

    const uint2 first = thread_id.xy * 2u;
    uint2 last = first + 1u;
    if (thread_id.x == size_out.x - 1u && (size_in.x & 1u) != 0u)
        last.x += 1u;
    if (thread_id.y == size_out.y - 1u && (size_in.y & 1u) != 0u)
        last.y += 1u;
    last = min(last, size_in - 1u);

    float4 value = tex.Load(int3(int2(first), 0));
    for (uint y = first.y; y <= last.y; ++y)
    {
        for (uint x = first.x; x <= last.x; ++x)
            value = min(value, tex.Load(int3(int(x), int(y), 0)));
    }

    tex_uav[thread_id.xy] = value;
}
