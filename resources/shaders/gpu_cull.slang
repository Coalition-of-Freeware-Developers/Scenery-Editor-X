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

// GPU culling and LOD selection for the GPU scene, one thread per instance. A visible instance is appended to the
// visible list of its batch's draw for the chosen LOD; the draw's instance_count (reset to 0 by the CPU-recorded
// copy of the command templates) is the append counter. The draws then run as instanced indirect draws.
//
// Push constants: buffer_pass.draw_index = instance count; values[2] = (minimum projected diameter in pixels,
// LOD distance scale, frustum culling on (> 0), unused).

[[vk::binding(24, 11)]] StructuredBuffer<GpuInstance> cull_instances : register(t24, space11);
[[vk::binding(25, 12)]] StructuredBuffer<GpuBatch> cull_batches : register(t25, space12);
[[vk::binding(26, 13)]] RWStructuredBuffer<IndirectDrawArgs> cull_commands : register(u26, space13);
[[vk::binding(27, 14)]] RWStructuredBuffer<uint> cull_visible_instances : register(u27, space14);

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

[numthreads(64, 1, 1)]
[shader("compute")]
void main_comp(uint3 thread_id : SV_DispatchThreadID)
{
    const uint index = thread_id.x;
    if (index >= buffer_pass.draw_index)
        return;

    const GpuInstance instance = cull_instances[index];
    if (!instance.enabled())
        return;

    const float3 bounds_min = instance.bounds_min_batch.xyz;
    const float3 bounds_max = instance.bounds_max_flags.xyz;
    const float3 center     = (bounds_min + bounds_max) * 0.5f;
    const float3 extent     = (bounds_max - bounds_min) * 0.5f;
    const float4 options    = pass_get_f4_value();

    // the frame's view-projection reads back transposed (column-vector matrix stored row-major, see GpuMatrix)
    if (options.z > 0.0f && is_outside_frustum(transpose(get_view_projection_matrix()), center, extent))
        return;

    // how much of the screen height the bounding sphere covers (0 distance: the camera is inside the box)
    const float3 camera          = get_camera_position();
    const float distance         = length(camera - clamp(camera, bounds_min, bounds_max));
    const float projection_y     = get_projection_matrix()[1][1]; // 1 / tan(vertical fov / 2)
    const float screen_fraction  = distance > 0.0f ? length(extent) * projection_y / distance : 1e30f;

    // small-feature culling: below a few pixels an object can't be told apart from its neighbours
    if (screen_fraction * get_resolution_render().y < options.x)
        return;

    // mesh LOD by screen coverage
    const GpuBatch batch   = cull_batches[instance.batch()];
    const uint lod_count   = clamp(batch.lod_count, 1u, GPU_MAX_LODS);
    const float coverage   = screen_fraction * options.y;
    uint lod               = lod_count - 1;
    for (uint i = 0; i + 1 < lod_count; ++i)
    {
        if (coverage >= GPU_LOD_SCREEN_FRACTIONS[i])
        {
            lod = i;
            break;
        }
    }

    const uint command = batch.first_command + lod;
    uint slot;
    InterlockedAdd(cull_commands[command].instance_count, 1u, slot);
    cull_visible_instances[cull_commands[command].first_instance + slot] = index;
}
