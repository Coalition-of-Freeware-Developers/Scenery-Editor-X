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
 * pipeline_cache.h
 * -------------------------------------------------------
 * Created: 17/10/2026
 * -------------------------------------------------------
 */
#pragma once
#include "pipeline_state.h"

// --------------------------------------------------------------

namespace SceneryEditorX
{
	class DescriptorSet;
	class Pipeline;
	class Shader;

	/**
	 * @class PipelineCache
	 * @brief Process-wide cache of the descriptor-set layout and compiled pipeline built for each PipelineState.
	 *
	 * Entries are keyed by PipelineState::GetHash(), which covers every shader program (Shader::GetHash()),
	 * the render targets and the fixed-function state. CommandList binds the cached objects instead of
	 * creating a VkDescriptorSetLayout, VkPipelineLayout and VkPipeline each time a pass sets its PSO.
	 *
	 * An entry is evicted when one of its shaders hot-reloads, or when it has not been used for
	 * STALE_FRAME_COUNT frames (recreated render targets, e.g. after a resize, produce new hashes).
	 * Evicted objects release their Vulkan handles through QueueManager's deletion queue, so command
	 * buffers still in flight stay valid.
	 *
	 * @note The binding state of a cached DescriptorSet is shared by every command list that uses the
	 * PSO; CommandList resets it whenever it switches to a different PSO. Recording the same PSO on two
	 * command lists concurrently is not supported.
	 */
	class PipelineCache
	{
	public:
		/// Frames an entry may go unused before Tick() evicts it.
		static constexpr uint64_t STALE_FRAME_COUNT = 1024;

		/**
		 * @brief Get the descriptor-set layout for a PSO, building it from the PSO's reflected descriptors on a miss.
		 * @param pso Prepared pipeline state; its hash must be current.
		 * @return The cached layout, or nullptr when reflection reports a binding type conflict or the layout cannot be created.
		 */
		static Ref<DescriptorSet> GetDescriptorLayout(PipelineState& pso);

		/**
		 * @brief Get the pipeline for a PSO, compiling it against the cached descriptor-set layout on a miss.
		 * Call after a successful GetDescriptorLayout() for the same PSO.
		 * @param pso Prepared pipeline state; its hash must be current.
		 * @return The cached pipeline (Pipeline::Get() is VK_NULL_HANDLE when creation failed), or nullptr when no layout is cached for the PSO.
		 */
		static Ref<Pipeline> GetPipeline(PipelineState& pso);

		/* @brief Advance the frame counter and evict entries unused for STALE_FRAME_COUNT frames. Call once per frame. */
		static void Tick();

		/**
		 * @brief Evict every entry built from a shader program.
		 * @param shader The shader whose entries are stale (called from its reload callback).
		 */
		static void EvictShader(const Shader* shader);

		/* @brief Destroy every cached object. Call from Renderer::Shutdown() while the device and the deletion queue are alive. */
		static void Clear();

		/**
		 * @brief Get the number of cached entries.
		 * @return The entry count.
		 */
		[[nodiscard]] static size_t GetEntryCount();
	};

}

// --------------------------------------------------------------
