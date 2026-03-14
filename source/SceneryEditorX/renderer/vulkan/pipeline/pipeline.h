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
#include <vector>
#include <SceneryEditorX/renderer/vulkan/shader/shader_manager.h>

// --------------------------------------------------------------

namespace SceneryEditorX
{
	// Pipeline helper: creates pipelines from an input descriptor struct to make
	// constructing different kinds of pipelines (graphics/compute) easier.
	class Pipeline 
	{
	public:
		Pipeline() = default;
		~Pipeline() = default;
	
		// Descriptor grouping inputs required for creating a graphics pipeline.
		struct GraphicsCreateInfo 
		{
			VkDevice device{ VK_NULL_HANDLE };
			VkPipelineLayout layout{ VK_NULL_HANDLE };
			const ShaderManager* shaderManager{ nullptr };
			const char* vertexEntryPoint{ "main" };
			const char* fragmentEntryPoint{ "main" };
			VkVertexInputBindingDescription vertexBinding{};
			std::vector<VkVertexInputAttributeDescription> vertexAttributes{};
			VkFormat colorFormat{ VK_FORMAT_UNDEFINED };
			VkFormat depthFormat{ VK_FORMAT_UNDEFINED };
		};
	
		// Create a graphics pipeline using a single grouped input structure.
		// Returns VK_NULL_HANDLE on failure.
		static VkPipeline CreateGraphics(const GraphicsCreateInfo& info);
	};

}

// --------------------------------------------------------------
