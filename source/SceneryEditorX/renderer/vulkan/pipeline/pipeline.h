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
 * pipeline.h
 * -------------------------------------------------------
 * Created: 09/02/2026
 * -------------------------------------------------------
 */
#pragma once
#include "pipeline_state.h"
#include <vector>

// --------------------------------------------------------------

namespace SceneryEditorX
{
	class Device;
	class DescriptorSet;
	class ShaderManager;

	/**
	 * @class Pipeline
	 * @brief A helper class for creating Vulkan pipelines from 
	 * a single input descriptor struct, to simplify the process of constructing different kinds of pipelines (e.g., graphics, compute).
	 */
	class Pipeline : public SharedObject
	{
	public:
		Pipeline() = default;

		/**
		 * @brief Construct and compile a Vulkan pipeline from a PipelineState.
		 * @param state  The pipeline state object describing shaders, render targets, and fixed-function state.
		 * @param layout Optional descriptor set layout for bindless resources and push-constant reflection.
		 *               When nullptr a minimal push-constant-only VkPipelineLayout is created (bootstrap path).
		 */
		Pipeline(PipelineState &state, DescriptorSet *layout = nullptr);

		/* @brief Destroy the Vulkan pipeline and release associated resources. */
		virtual ~Pipeline() override;

		Pipeline(Pipeline&& other) noexcept;
		Pipeline& operator=(Pipeline&& other) noexcept;
		Pipeline(const Pipeline&) = delete;
		Pipeline& operator=(const Pipeline&) = delete;

		// Descriptor grouping inputs required for creating a graphics pipeline.
		struct GraphicsCreateInfo 
		{
			VkDevice device{ VK_NULL_HANDLE };
			VkPipelineLayout layout{ VK_NULL_HANDLE };
			Ref<ShaderManager> shaderManager{ nullptr };
			const char* vertexEntryPoint{ "main" };
			const char* fragmentEntryPoint{ "main" };
			VkVertexInputBindingDescription vertexBinding{};
			std::vector<VkVertexInputAttributeDescription> vertexAttributes{};
			VkFormat colorFormat{ VK_FORMAT_UNDEFINED };
			VkFormat depthFormat{ VK_FORMAT_UNDEFINED };
		};

		/**
		 * @brief Get the Vulkan pipeline cache.
		 * @return The Vulkan pipeline cache handle.
		 */
		static VkPipelineCache GetPipelineCache();

		/**
		 * @brief Destroy the Vulkan pipeline cache and release associated resources.
		 * @param device The Vulkan device associated with the pipeline cache.
		 */
		void DestroyPipeline(VkDevice device = VK_NULL_HANDLE);

		/**
		 * @brief Get the current state of the pipeline.
		 * @return A pointer to the pipeline state object.
		 */
		PipelineState* GetState() { return &m_State; }

		/**
		 * @brief Get the Vulkan pipeline handle.
		 * @return The Vulkan pipeline handle.
		 */
		VkPipeline Get() const { return m_Pipeline; }

		/**
		 * @brief Get the Vulkan pipeline layout.
		 * @return The Vulkan pipeline layout handle.
		 */
		VkPipelineLayout GetLayout() const { return m_Layout; }

		/**
		 * @brief Get the stages that use push constants.
		 * @return A bitmask representing the shader stages that use push constants.
		 */
		uint32_t GetPushConstantStages() const { return m_PushConstant_Stages; }

		/**
		 * @brief Check if the pipeline has been destroyed.
		 * @return True if the pipeline has been destroyed, false otherwise.
		 */
		bool IsDestroyed() const { return m_Destroyed; }

		// Create a graphics pipeline using a single grouped input structure.
		// Returns VK_NULL_HANDLE on failure.
		[[deprecated]] static VkPipeline CreateGraphics(const GraphicsCreateInfo& info);

	private:
		/* @brief Create the Vulkan pipeline cache. */
		static void CreatePipelineCache();

		/* @brief Save the Vulkan pipeline cache to disk. */
		static void SavePipelineCache();

		Ref<Device> m_Device;
		VkPipeline m_Pipeline{ VK_NULL_HANDLE };
		PipelineState m_State;
		VkPipelineLayout m_Layout{ VK_NULL_HANDLE };
		uint32_t m_PushConstant_Stages = 0;
		bool m_Destroyed = false;
	};

}

// --------------------------------------------------------------
