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
 * gpu_cull.slang
 * -------------------------------------------------------
 * Created: 18/10/2026
 * -------------------------------------------------------
 */
import common;
import resources;
import gpu_scene_types;

// GPU culling and LOD selection for the GPU scene, one thread per instance, with two-phase occlusion culling:
//
// - early pass (no define): instances that pass the frustum and small-feature tests and were visible last frame
//   (cull_visibility) are appended to the early draw commands. With occlusion culling off it appends every instance
//   that passes and is the only pass.
// - the depth prepass draws them; Renderer::Pass_GpuScene_Occlusion builds a min-depth Hi-Z pyramid of that depth.
// - late pass (GPU_CULL_LATE): every instance that passes is tested against the pyramid. The result is this frame's
//   visibility (read by the next frame's early pass), and the visible ones the early pass didn't draw are appended to
//   the late draw commands, which the depth prepass and the g-buffer draw after the early ones.
//
// Everything the late pass culls is hidden behind geometry drawn this frame from this camera, so nothing pops in when
// the view changes: last frame's visibility only decides what is drawn early (and occludes the rest).
//
// A visible instance is appended to the visible list of its batch's draw for the chosen LOD; the draw's instance_count
// (reset to 0 by the CPU-recorded copy of the command templates) is the append counter.
//
// - shadow pass (GPU_CULL_SHADOW, plan TASK-053): every enabled instance that casts shadows is tested against each sun
//   shadow cascade (buffer_environment.csm_*) and appended to that cascade's commands, which follow the occluded counter
//   (cascade c's command = first shadow command + c x late offset + the early command). LODs are the camera's, so the
//   casters match what is drawn.
//
// Push constants: buffer_pass.draw_index = instance count; values[2] = (minimum projected diameter in pixels (shadow
// pass: in cascade texels), LOD distance scale, frustum culling on (> 0), unused); values[0].xyz = (late command
// offset, occlusion culling on (> 0), first shadow command).

[[vk::binding(24, 11)]] StructuredBuffer<GpuInstance> cull_instances : register(t24, space11);
[[vk::binding(25, 12)]] StructuredBuffer<GpuBatch> cull_batches : register(t25, space12);
[[vk::binding(26, 13)]] RWStructuredBuffer<IndirectDrawArgs> cull_commands : register(u26, space13);
[[vk::binding(27, 14)]] RWStructuredBuffer<uint> cull_visible_instances : register(u27, space14);
// per instance: non-zero when it was visible (in view and not occluded) last frame; written by the late pass
[[vk::binding(28, 15)]] RWStructuredBuffer<uint> cull_visibility : register(u28, space15);

// clip = M * p with clip.z in [0, w] (reverse-Z, D3D-style); the planes are combinations of M's rows. Unnormalized
// planes are fine for the sign test.
bool is_outside_frustum(float4x4 clip_rows, float3 center, float3 extent)
{
    const float4 planes[6] =
    {
        clip_rows[3] + clip_rows[0], // left
        clip_rows[3] - clip_rows[0], // right
        clip_rows[3] + clip_rows[1], // bottom
        clip_rows[3] - clip_rows[1], // top
        clip_rows[3] - clip_rows[2], // near (z <= w)
        clip_rows[2]                 // far (z >= 0)
    };

    [unroll]
    for (int i = 0; i < 6; ++i)
    {
        const float distance = dot(planes[i].xyz, center) + planes[i].w;
        const float radius   = dot(abs(planes[i].xyz), extent);
        if (distance + radius < 0.0f)
            return true;
    }
    return false;
}

// How much of the screen height an instance's bounding sphere covers (huge with the camera inside its box)
float screen_fraction(float3 bounds_min, float3 bounds_max)
{
    const float4x4 projection = get_projection_matrix();
    const float radius        = length((bounds_max - bounds_min) * 0.5f);

    // orthographic (w stays 1): the size on screen doesn't depend on distance; [1][1] = 2 / view height
    if (projection[3][3] > 0.5f)
        return radius * projection[1][1];

    const float3 camera      = get_camera_position();
    const float distance     = length(camera - clamp(camera, bounds_min, bounds_max));
    const float projection_y = projection[1][1]; // 1 / tan(vertical fov / 2)
    return distance > 0.0f ? radius * projection_y / distance : 1e30f;
}

// The early draw command of an instance's batch for the mesh LOD its screen coverage picks
uint lod_command(GpuInstance instance, float fraction)
{
    const GpuBatch batch = cull_batches[instance.batch()];
    const uint lod_count = clamp(batch.lod_count, 1u, GPU_MAX_LODS);
    const float coverage = fraction * pass_get_f4_value().y;
    uint lod             = lod_count - 1;
    for (uint i = 0; i + 1 < lod_count; ++i)
    {
        if (coverage >= GPU_LOD_SCREEN_FRACTIONS[i])
        {
            lod = i;
            break;
        }
    }
    return batch.first_command + lod;
}

// Frustum and small-feature tests and LOD selection: false when the instance isn't drawn, else its early draw command
// (the batch's command for the chosen LOD).
bool select_command(GpuInstance instance, out uint command)
{
    command = 0;
    if (!instance.enabled())
        return false;

    const float3 bounds_min = instance.bounds_min_batch.xyz;
    const float3 bounds_max = instance.bounds_max_flags.xyz;
    const float3 center     = (bounds_min + bounds_max) * 0.5f;
    const float3 extent     = (bounds_max - bounds_min) * 0.5f;
    const float4 options    = pass_get_f4_value();

    // the frame's view-projection reads back transposed (column-vector matrix stored row-major, see GpuMatrix)
    if (options.z > 0.0f && is_outside_frustum(transpose(get_view_projection_matrix()), center, extent))
        return false;

    // small-feature culling: below a few pixels an object can't be told apart from its neighbours
    const float fraction = screen_fraction(bounds_min, bounds_max);
    if (fraction * get_resolution_render().y < options.x)
        return false;

    // mesh LOD by screen coverage
    command = lod_command(instance, fraction);
    return true;
}

#if defined(GPU_CULL_SHADOW)
// A sun-shadow cascade's four sides and far plane (renderer/csm.h CsmCascade::CanCast): the near plane is left out, so
// casters nearer the light than the cascade still cast (the depth clamp puts them on its near plane while drawn)
bool is_outside_cascade(float4x4 clip_rows, float3 center, float3 extent)
{
    const float4 planes[5] =
    {
        clip_rows[3] + clip_rows[0], // left
        clip_rows[3] - clip_rows[0], // right
        clip_rows[3] + clip_rows[1], // bottom
        clip_rows[3] - clip_rows[1], // top
        clip_rows[2]                 // far (z >= 0)
    };

    [unroll]
    for (int i = 0; i < 5; ++i)
    {
        if (dot(planes[i].xyz, center) + planes[i].w + dot(abs(planes[i].xyz), extent) < 0.0f)
            return true;
    }
    return false;
}
#endif

void append(uint command, uint index)
{
    uint slot;
    InterlockedAdd(cull_commands[command].instance_count, 1u, slot);
    cull_visible_instances[cull_commands[command].first_instance + slot] = index;
}

#if defined(GPU_CULL_LATE)
// tex: reverse-Z min-depth pyramid of what the early pass (and the CPU draws) put in the depth buffer. Mip 0 is that
// depth, each texel of a smaller mip the farthest depth of the texels it covers (Renderer::Pass_Downscale, Min
// filter), so pixel p falls in texel min(p >> mip, mip size - 1).
bool is_occluded(float3 bounds_min, float3 bounds_max)
{
    // the box's screen rectangle and its nearest depth (reverse-Z: the largest)
    const float4x4 view_projection = get_view_projection_matrix();
    float2 ndc_min = float2(1e30f, 1e30f);
    float2 ndc_max = float2(-1e30f, -1e30f);
    float nearest  = 0.0f;
    [unroll]
    for (uint i = 0; i < 8; ++i)
    {
        const float3 corner = float3((i & 1u) != 0 ? bounds_max.x : bounds_min.x,
                                     (i & 2u) != 0 ? bounds_max.y : bounds_min.y,
                                     (i & 4u) != 0 ? bounds_max.z : bounds_min.z);
        const float4 clip = mul(float4(corner, 1.0f), view_projection);

        // a box reaching in front of the near plane (or behind the camera) covers the view: keep it
        if (clip.w <= 0.0f || clip.z > clip.w)
            return false;

        const float3 ndc = clip.xyz / clip.w;
        ndc_min = min(ndc_min, ndc.xy);
        ndc_max = max(ndc_max, ndc.xy);
        nearest = max(nearest, ndc.z);
    }

    uint2 size;
    uint mip_count;
    tex.GetDimensions(0, size.x, size.y, mip_count);

    // the pixels it covers (uv.y = 0 at the top, ndc +y up), with a pixel of slack for rasterization and rounding
    const float2 viewport = float2(size) * get_resolution_scale();
    const int2 pixel_last = max(int2(viewport) - 1, 0);
    const float2 uv_min   = saturate(float2(ndc_min.x, -ndc_max.y) * 0.5f + 0.5f);
    const float2 uv_max   = saturate(float2(ndc_max.x, -ndc_min.y) * 0.5f + 0.5f);
    const int2 pixel_min  = clamp(int2(floor(uv_min * viewport)) - 1, 0, pixel_last);
    const int2 pixel_max  = clamp(int2(floor(uv_max * viewport)) + 1, 0, pixel_last);

    // the mip whose texels are at least as large as the rectangle: it touches at most 2 x 2 of them
    const uint span = uint(max(pixel_max.x - pixel_min.x, pixel_max.y - pixel_min.y)); // pixels - 1
    const uint mip  = min(span == 0 ? 0u : firstbithigh(span) + 1u, mip_count - 1u);
    const int2 texel_last = max(int2(size >> mip), 1) - 1;
    const int2 a = min(pixel_min >> mip, texel_last);
    const int2 b = min(pixel_max >> mip, texel_last);
    const float farthest = min(min(tex.Load(int3(a.x, a.y, mip)).r, tex.Load(int3(b.x, a.y, mip)).r),
                               min(tex.Load(int3(a.x, b.y, mip)).r, tex.Load(int3(b.x, b.y, mip)).r));

    // hidden when even its nearest point is behind everything drawn there (a little margin for depth precision)
    return nearest * (1.0f + 1e-5f) < farthest;
}
#endif

[numthreads(64, 1, 1)]
[shader("compute")]
void main_comp(uint3 thread_id : SV_DispatchThreadID)
{
    const uint index = thread_id.x;
    if (index >= buffer_pass.draw_index)
        return;

    const GpuInstance instance = cull_instances[index];
    const uint late_offset     = uint(pass_get_f3_value().x);

#if defined(GPU_CULL_SHADOW)
    if (!instance.enabled() || (instance.flags() & GPU_INSTANCE_CASTS_SHADOW) == 0)
        return;

    const float3 bounds_min = instance.bounds_min_batch.xyz;
    const float3 bounds_max = instance.bounds_max_flags.xyz;
    const float3 center     = (bounds_min + bounds_max) * 0.5f;
    const float3 extent     = (bounds_max - bounds_min) * 0.5f;
    const uint command      = lod_command(instance, screen_fraction(bounds_min, bounds_max));
    const uint first_shadow = uint(pass_get_f3_value().z);
    const float min_texels  = pass_get_f4_value().x;
    const uint cascades     = min(uint(buffer_environment.csm_params.x + 0.5f), 4u);
    for (uint cascade = 0; cascade < cascades; ++cascade)
    {
        // smaller than a texel or so of this cascade: its shadow wouldn't show
        if (2.0f * length(extent) < min_texels * buffer_environment.csm_texel[cascade])
            continue;
        // the cascade's matrix reads back transposed like the frame's (see GpuMatrix)
        if (is_outside_cascade(transpose(buffer_environment.csm_view_projection[cascade]), center, extent))
            continue;
        append(first_shadow + cascade * late_offset + command, index);
    }
#else
    uint command;
    const bool in_view = select_command(instance, command);

#if defined(GPU_CULL_LATE)
    // the early pass drew what was visible last frame (and in view); an instance it drew isn't hidden by its own depth
    const bool drawn_early = in_view && cull_visibility[index] != 0;
    const bool visible     = in_view && !is_occluded(instance.bounds_min_batch.xyz, instance.bounds_max_flags.xyz);
    cull_visibility[index] = visible ? 1u : 0u;

    if (visible && !drawn_early)
        append(late_offset + command, index);

    // statistics: instances in view that neither pass draws
    if (in_view && !visible && !drawn_early)
        InterlockedAdd(cull_commands[gpu_occluded_counter_command(late_offset)].instance_count, 1u);
#else
    // occlusion culling on: what wasn't visible last frame waits for the late pass
    const bool occlusion = pass_get_f3_value().y > 0.0f;
    if (in_view && (!occlusion || cull_visibility[index] != 0))
        append(command, index);
#endif
#endif
}
