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
 * gpu_scene_types.slang
 * -------------------------------------------------------
 * Created: 18/10/2026
 * -------------------------------------------------------
 */

// GPU-driven scene data shared by the cull pass (gpu_cull.slang) and the instanced geometry passes (gpu_scene.slang).
// No resource declarations here: the cull pass binds the same buffers writable. Keep in sync with GpuScene
// (source/SceneryEditorX/renderer/gpu_scene.h).

// One drawn renderable: its world matrix and world-space bounds, written by the CPU only when they change.
public struct GpuInstance
{
    public float4 world0;           // rows 0-2 of the column-vector world matrix (world.x = dot(world0, float4(p, 1)))
    public float4 world1;
    public float4 world2;
    public float4 bounds_min_batch; // xyz = world AABB min, w = batch index (bit pattern)
    public float4 bounds_max_flags; // xyz = world AABB max, w = GPU_INSTANCE_* flags (bit pattern)

    public uint batch() { return asuint(bounds_min_batch.w); }
    public uint flags() { return asuint(bounds_max_flags.w); }
    public bool enabled() { return (flags() & GPU_INSTANCE_ENABLED) != 0; }
    public uint pick_id() { return flags() >> GPU_INSTANCE_PICK_SHIFT; } // editor picking: entity index + 1, 0 = none

    // the row-vector form the vertex code multiplies with: mul(float4(p, 1), transform())
    public float4x4 transform() { return transpose(float4x4(world0, world1, world2, float4(0.0f, 0.0f, 0.0f, 1.0f))); }
};

public static const uint GPU_INSTANCE_ENABLED = 1u << 0;
// Bits 8..31 of the flags carry the instance's pick id (entity index + 1, 24 bits), which gbuffer_pick receives
public static const uint GPU_INSTANCE_PICK_SHIFT = 8u;

// Instances that share geometry and material: one indirect draw per mesh LOD, its visible instances listed in a
// region of the visible-instance buffer starting at the draw's first_instance.
public struct GpuBatch
{
    public uint first_command;  // first of lod_count consecutive draw commands
    public uint lod_count;
    public uint material_index; // bindless material slot (MaterialAsset::GetIndex)
    public uint flags;
};

// Two-phase occlusion culling (gpu_cull.slang): the late pass's draw commands follow the early pass's (late command =
// early command + late offset, its visible list in the second half of the visible-instance buffer), then one record
// whose instance_count counts the instances the late pass found occluded (never drawn).
public uint gpu_occluded_counter_command(uint late_offset) { return 2u * late_offset; }

public static const uint GPU_MAX_LODS = 4;

// Screen-height fraction an object's bounding sphere must cover to use each mesh LOD (Renderable::UpdateLodIndices)
public static const float GPU_LOD_SCREEN_FRACTIONS[GPU_MAX_LODS] = { 0.05f, 0.025f, 0.012f, 0.006f };
