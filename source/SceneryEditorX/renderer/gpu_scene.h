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
 * gpu_scene.h
 * -------------------------------------------------------
 * Created: 18/10/2026
 * -------------------------------------------------------
 */
#pragma once
#include <array>
#include <cstdint>
#include <SceneryEditorX/renderer/vulkan/enums.h>
#include <entt/entity/fwd.hpp>

// -------------------------------------------------------

namespace SceneryEditorX
{
	class Buffer;
	class CommandList;
	class Renderable;

	/**
	 * @class GpuScene
	 * @brief GPU-driven geometry for the deferred passes: GPU frustum / small-feature culling, mesh LOD selection
	 * and instancing.
	 *
	 * Every Renderable the depth prepass and the g-buffer draw is an instance in a persistent GPU buffer (world
	 * matrix and world bounds), written only when it changes. Instances that share geometry (the mesh range) and
	 * material form a batch, drawn as one instanced indirect draw per mesh LOD. Each frame a compute pass
	 * (gpu_cull.slang) tests every instance against the view frustum and a minimum projected size, picks its LOD and
	 * appends it to that draw's visible list. The geometry passes then issue one vkCmdDrawIndexedIndirect per
	 * pipeline bucket (cull mode x alpha test) instead of a CPU draw call per entity.
	 *
	 * Render thread (Renderer::Tick and the passes). Entity, renderable and material changes arrive through
	 * Scene::GetStructureVersion() and OnMaterialsChanged(). Transforms of instances that moved recently are checked
	 * every frame and the rest round-robin; MarkTransformDirty() makes an edited subtree update the same frame.
	 */
	class GpuScene
	{
	public:
		/* @brief Consecutive draw commands sharing one pipeline: a cull mode and whether the depth prepass alpha-tests. */
		struct Bucket
		{
			uint32_t firstCommand = 0;
			uint32_t commandCount = 0;
			CullMode cullMode = CullMode::Back;
			bool alphaTested = false;
		};

		struct Stats
		{
			uint32_t instances = 0;			// Renderables in the GPU scene
			uint32_t enabledInstances = 0;	// Of those, active entities
			uint32_t batches = 0;			// Distinct geometry + material pairs in use
			uint32_t commands = 0;			// Indirect draw commands (a batch has one per mesh LOD)
			uint32_t visibleInstances = 0;	// Survived culling (read back from a recent frame)
			uint32_t drawnCommands = 0;		// Commands with at least one visible instance (read back)
			uint32_t uploadedInstances = 0;	// Instances written to the GPU this frame
			uint32_t validatedTransforms = 0; // World matrices re-read this frame
		};

		static constexpr uint32_t BUCKET_COUNT = 6; // CullMode None / Front / Back x alpha test

		/* @brief Off: every renderable goes through the CPU draw path again (for comparison and debugging). */
		static void SetEnabled(bool enabled);
		[[nodiscard]] static bool IsEnabled();

		/**
		 * @brief Brings the instances, batches and draw commands up to date and records their uploads.
		 * Call once per frame before anything is drawn or bound in cmdList (it may reallocate bindless buffers).
		 */
		static void Update(CommandList *cmdList);

		/* @brief Records the per-frame command reset and the cull dispatch. Call after Update(), outside a render pass. */
		static void Cull(CommandList *cmdList);

		/* @brief True when this frame has GPU-scene draws (Update() ran and there are instances). */
		[[nodiscard]] static bool HasDraws();

		/**
		 * @brief Whether a renderable is drawn by the GPU scene: the CPU deferred passes skip it (the forward
		 * transparent pass still draws its blended part). Cheap, no lookup: the same test Update() uses.
		 */
		[[nodiscard]] static bool IsGpuDriven(const Renderable *renderable);

		[[nodiscard]] static const std::array<Bucket, BUCKET_COUNT> &GetBuckets();

		/* @brief The draw commands (VkDrawIndexedIndirectCommand records) the buckets index into. */
		[[nodiscard]] static Buffer *GetCommandBuffer();

		/* @brief Material slots or properties changed (Renderer::UpdateMaterials): batches re-read them. */
		static void OnMaterialsChanged();

		/**
		 * @brief An entity's transform was edited: its renderables (and those below it) re-read their world matrix in
		 * the next Update() instead of within the round-robin window. Any thread that may touch the scene.
		 */
		static void MarkTransformDirty(entt::entity entity);

		/**
		 * @brief Culling knobs.
		 * @param minPixelSize Instances whose bounding sphere projects smaller than this (diameter, pixels) are culled.
		 * @param lodScale Multiplies the screen coverage used for mesh LOD selection (> 1 keeps detail longer).
		 * @param frustumCulling Off draws everything in front of and behind the camera (debugging).
		 */
		static void SetCullingOptions(float minPixelSize, float lodScale, bool frustumCulling);

		[[nodiscard]] static const Stats &GetStats();

		/* @brief Releases the GPU buffers. Call while the device is alive, after the GPU is idle. */
		static void Shutdown();
	};

}

// -------------------------------------------------------
