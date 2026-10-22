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
 * z_minmax.slang
 * -------------------------------------------------------
 * Created: 21/10/2026
 * -------------------------------------------------------
 *
 * The min / max depth pyramid (plan TASK-060; XP12: z_minmax, rg32f): each texel holds the farthest and the nearest
 * reverse-Z depth (the smallest and the largest value) of the depth-buffer texels under it. Occlusion culling reads the
 * farthest (.r, gpu_cull.slang), the screen-space reflections march the nearest (.g, ssr_common.slang); light tiling and
 * volumetrics will read both. Renderer::Pass_DepthPyramid builds one level per dispatch:
 *   ZMINMAX_FROM_DEPTH  level 0 from the depth buffer: tex (the depth image) -> tex_uav (level 0's view)
 *   otherwise           level n from level n - 1: tex (the level above, a single-mip view) -> tex_uav
 * The last texel of a row or column whose level above had an odd size also takes the odd one out, so level-0 texel p
 * always falls in texel min(p >> n, size - 1) (renderer/ssr.cpp BuildDepthPyramid is the CPU twin the tests check).
 */
import resources;
import constants;

[shader("compute")]
[numthreads(THREAD_GROUP_COUNT_X, THREAD_GROUP_COUNT_Y, 1)]
void main_comp(uint3 thread_id : SV_DispatchThreadID)
{
    uint2 size_out;
    tex_uav.GetDimensions(size_out.x, size_out.y);
    if (any(thread_id.xy >= size_out))
        return;

#if defined(ZMINMAX_FROM_DEPTH)
    const float depth = tex.Load(int3(int2(thread_id.xy), 0)).r;
    tex_uav[thread_id.xy] = float4(depth, depth, 0.0f, 0.0f);
#else
    uint2 size_in;
    tex.GetDimensions(size_in.x, size_in.y);

    const uint2 first = thread_id.xy * 2u;
    uint2 last = first + 1u;
    if (thread_id.x == size_out.x - 1u && (size_in.x & 1u) != 0u)
        last.x += 1u;
    if (thread_id.y == size_out.y - 1u && (size_in.y & 1u) != 0u)
        last.y += 1u;
    last = min(last, size_in - 1u);

    float2 bounds = tex.Load(int3(int2(first), 0)).rg;	// farthest, nearest
    for (uint y = first.y; y <= last.y; ++y)
    {
        for (uint x = first.x; x <= last.x; ++x)
        {
            const float2 texel = tex.Load(int3(int(x), int(y), 0)).rg;
            bounds = float2(min(bounds.x, texel.x), max(bounds.y, texel.y));
        }
    }
    tex_uav[thread_id.xy] = float4(bounds, 0.0f, 0.0f);
#endif
}
