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
 * gpu_scene.slang
 * -------------------------------------------------------
 * Created: 18/10/2026
 * -------------------------------------------------------
 */
__exported import gpu_scene_types;
import resources;

// Read-only views for the instanced geometry passes (bindless sets 11, 12 and 14, see BindlessResource). A vertex
// stage may not write storage buffers without vertexPipelineStoresAndAtomics, so the cull pass declares its own.
public [[vk::binding(24, 11)]] StructuredBuffer<GpuInstance> gpu_instances : register(t24, space11);
public [[vk::binding(25, 12)]] StructuredBuffer<GpuBatch> gpu_batches : register(t25, space12);
public [[vk::binding(27, 14)]] StructuredBuffer<uint> gpu_visible_instances : register(t27, space14);

// Loads the per-draw state (_draw) of an instanced GPU-scene draw. SV_InstanceID excludes the draw's first
// instance, which is where its visible instances start.
public uint gpu_scene_load_draw(uint instance_id, uint base_instance)
{
    const uint index = gpu_visible_instances[base_instance + instance_id];
    const GpuInstance instance = gpu_instances[index];

    _draw.transform          = instance.transform();
    _draw.transform_previous = _draw.transform; // static scenery: motion vectors come from the camera only
    _draw.material_index     = gpu_batches[instance.batch()].material_index;
    _draw.is_transparent     = 0;
    _draw.aabb_index         = 0;
    _draw.pick_id            = instance.pick_id();
    _draw.drape_rank         = 0; // drapes are drawn on the CPU path, in order
    return index;
}
