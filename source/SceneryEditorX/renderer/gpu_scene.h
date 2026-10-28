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
	class ImageResource;
	class Renderable;

	/**
	 * @class GpuScene
	 * @brief GPU-driven geometry for the deferred passes: GPU frustum / small-feature / occlusion culling, mesh LOD
	 * selection and instancing.
	 *
	 * Every Renderable the depth prepass and the g-buffer draw is an instance in a persistent GPU buffer (world
	 * matrix and world bounds), written only when it changes. Instances that share geometry (the mesh range) and
	 * material form a batch, drawn as one instanced indirect draw per mesh LOD. Each frame a compute pass
	 * (gpu_cull.slang) tests every instance against the view frustum and a minimum projected size, picks its LOD and
	 * appends it to that draw's visible list. The geometry passes then issue one vkCmdDrawIndexedIndirect per
	 * pipeline bucket (cull mode x alpha test) instead of a CPU draw call per entity.
	 *
	 * Occlusion culling is two-phase, so nothing pops in when the camera moves: the early pass (Cull) only lets
	 * through the instances that were visible last frame; the depth prepass draws them and the renderer builds a
	 * min-depth Hi-Z pyramid of that depth; the late pass (CullLate) tests every instance against it, records this
	 * frame's visibility and lets through the visible ones the early pass held back. Both geometry passes draw both
	 * phases' commands (the early ones, then the late ones at GetLateCommandOffset()).
	 *
	 * Render thread (Renderer::Tick and the passes). Entity, renderable and material changes arrive through
	 * Scene::GetStructureVersion(), MaterialAsset::GetDrawPathVersion() and OnMaterialsChanged(); active flags and
	 * cast-shadow switches as Scene render-state changes (Scene::TakeRenderStateChanges), so a frame costs what changed,
	 * not every instance. Transforms of instances that moved recently are checked every frame and the rest
	 * round-robin; MarkTransformDirty() makes an edited subtree update the same frame.
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
			uint32_t earlyInstances = 0;	// Of those, drawn by the early pass (all of them without occlusion culling)
			uint32_t lateInstances = 0;		// Drawn by the late pass: visible now but not last frame
			uint32_t occludedInstances = 0;	// In view but hidden behind what the early pass drew (read back)
			uint32_t drawnCommands = 0;		// Commands with at least one visible instance (read back)
			uint32_t uploadedInstances = 0;	// Instances written to the GPU this frame
			uint32_t validatedTransforms = 0; // World matrices re-read this frame
			uint32_t shadowInstances = 0;	// Instance draws into the sun-shadow cascades, all cascades (read back)
		};

		static constexpr uint32_t BUCKET_COUNT = 6; // CullMode None / Front / Back x alpha test
		static constexpr uint32_t SHADOW_CASCADES = 4; // Command sets laid out for sun-shadow cascades (CSM_MAX_CASCADES)

		/* @brief Off: every renderable goes through the CPU draw path again (for comparison and debugging). */
		static void SetEnabled(bool enabled);
		[[nodiscard]] static bool IsEnabled();

		/* @brief Two-phase Hi-Z occlusion culling (on by default). Off: every instance in view is drawn (for comparison). */
		static void SetOcclusionCulling(bool enabled);
		[[nodiscard]] static bool IsOcclusionCullingEnabled();

		/**
		 * @brief Brings the instances, batches and draw commands up to date and records their uploads.
		 * Call once per frame before anything is drawn or bound in cmdList (it may reallocate bindless buffers).
		 */
		static void Update(CommandList *cmdList);

		/**
		 * @brief Records the per-frame command reset and the early cull dispatch. Call after Update(), outside a render
		 * pass. With occlusion culling active, the instances that weren't visible last frame wait for CullLate().
		 */
		static void Cull(CommandList *cmdList);

		/**
		 * @brief Late cull: tests every instance against hiz and lets the newly visible ones through to the late commands.
		 * Must run once per frame when IsOcclusionActive(), after the early commands were drawn into the depth.
		 * @param hiz Reverse-Z min-depth pyramid of the depth drawn so far, every mip in ShaderRead layout.
		 */
		static void CullLate(CommandList *cmdList, ImageResource *hiz);

		/**
		 * @brief Sun-shadow culling (plan TASK-053): every enabled instance that casts shadows goes into the draws of each
		 * cascade it can cast into (gpu_cull.slang GPU_CULL_SHADOW; the cascades come from the environment buffer), at the
		 * LOD the camera picks. Call after Cull(), outside a render pass, before the cascades are drawn.
		 * @param cascadeCount Cascades in use this frame (at most SHADOW_CASCADES); 0 does nothing.
		 */
		static void CullShadows(CommandList *cmdList, uint32_t cascadeCount);

		/* @brief A bucket's draws into cascade c start at GetShadowCommandOffset(c) plus its firstCommand. */
		[[nodiscard]] static uint32_t GetShadowCommandOffset(uint32_t cascade);

		/* @brief Copies this frame's cull results for the stats (read when the command list comes round again). Call once
		 * after the frame's last cull, outside a render pass. */
		static void RecordReadBack(CommandList *cmdList);

		/* @brief This frame culls in two phases: the depth prepass builds the Hi-Z, calls CullLate() and draws the late commands. */
		[[nodiscard]] static bool IsOcclusionActive();

		/* @brief The late commands follow the early ones: a bucket's late commands start at its firstCommand plus this. */
		[[nodiscard]] static uint32_t GetLateCommandOffset();

		/* @brief True when this frame has GPU-scene draws (Update() ran and there are instances). */
		[[nodiscard]] static bool HasDraws();

		/**
		 * @brief Whether a renderable is drawn by the GPU scene: the CPU deferred passes skip it (the forward
		 * transparent pass still draws its blended part). Cheap, no lookup: the same test Update() uses.
		 */
		[[nodiscard]] static bool IsGpuDriven(const Renderable *renderable);

		/* @brief Whether IsGpuDriven() can be true at all: enabled, and its shaders were built when Update() last ran. */
		[[nodiscard]] static bool IsDrawing();

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
