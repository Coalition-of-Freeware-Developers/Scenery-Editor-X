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
 * ssr_trace.slang
 * -------------------------------------------------------
 * Created: 21/10/2026
 * -------------------------------------------------------
 *
 * Screen-space reflections of the deferred surfaces and the reflection history they read (plan TASK-061; XP12:
 * ssr_deferred and its "l miptree"). Renderer::Pass_ScreenSpaceReflections / Pass_ReflectionHistory, by define:
 *   SSR_TRACE         every g-buffer pixel glossy enough traces its mirror (ssr_common.slang) into tex_uav (the reflections
 *                     target: rgb pre-exposed, a the weight the lighting gives it over the probes)
 *   SSR_HISTORY_COPY  the finished frame (tex, frame_render after fix_hdr) into level 0 of the history (tex_uav), its
 *                     brightest texels clamped (sedx/ssr_history_clamp: the sun disc would glare in blurred mips), alpha the
 *                     pre-exposure it was lit with
 *   SSR_HISTORY_MIP   one level of the history from the level above (tex, a single-mip view): the average of the 2 x 2
 *                     texels it covers, plus the odd one out at an odd level's last row / column
 */
import resources;
import constants;
import position;
import ssr_common;

[shader("compute")]
[numthreads(THREAD_GROUP_COUNT_X, THREAD_GROUP_COUNT_Y, 1)]
void main_comp(uint3 thread_id : SV_DispatchThreadID)
{
    uint2 size_out;
    tex_uav.GetDimensions(size_out.x, size_out.y);
    if (any(thread_id.xy >= size_out))
        return;

#if defined(SSR_TRACE)
    const float depth = tex_depth.Load(int3(int2(thread_id.xy), 0)).r;
    float4 result = 0.0f;
    if (depth > 0.0f)
    {
        const float2 uv = (float2(thread_id.xy) + 0.5f) / float2(size_out);
        const float3 world = get_position(depth, uv);
        const float3 normal = normalize(tex_normal.Load(int3(int2(thread_id.xy), 0)).xyz);
        const float roughness = xp_ssr_trace_all() ? 0.0f : saturate(tex_material.Load(int3(int2(thread_id.xy), 0)).g);
        result = xp_ssr_trace(world, normal, xp_ssr_view_direction(world), roughness);
    }
    tex_uav[thread_id.xy] = result;
#elif defined(SSR_HISTORY_COPY)
    float3 color = max(tex.Load(int3(int2(thread_id.xy), 0)).rgb, 0.0f);
    const float luminance = dot(color, float3(0.2126f, 0.7152f, 0.0722f));
    const float limit = buffer_environment.ssr_history.x;
    if (luminance > limit)
        color *= limit / luminance;
    tex_uav[thread_id.xy] = float4(color, get_pre_exposure());
#elif defined(SSR_HISTORY_MIP)
    uint2 size_in;
    tex.GetDimensions(size_in.x, size_in.y);

    const uint2 first = thread_id.xy * 2u;
    uint2 last = first + 1u;
    if (thread_id.x == size_out.x - 1u && (size_in.x & 1u) != 0u)
        last.x += 1u;
    if (thread_id.y == size_out.y - 1u && (size_in.y & 1u) != 0u)
        last.y += 1u;
    last = min(last, size_in - 1u);

    float4 sum = 0.0f;
    for (uint y = first.y; y <= last.y; ++y)
    {
        for (uint x = first.x; x <= last.x; ++x)
            sum += tex.Load(int3(int(x), int(y), 0));
    }
    tex_uav[thread_id.xy] = sum / float((last.x - first.x + 1u) * (last.y - first.y + 1u));
#endif
}
