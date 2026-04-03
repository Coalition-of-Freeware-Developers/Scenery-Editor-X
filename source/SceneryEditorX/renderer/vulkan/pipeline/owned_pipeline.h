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
 * owned_pipeline.h
 * -------------------------------------------------------
 * Created: 09/02/2026
 * -------------------------------------------------------
 */
#pragma once
#include <volk/volk.h>

// -------------------------------------------------------

namespace SceneryEditorX
{
	class ShaderManager;

	/**
	 * TODO: This class is currently unused, but will be the main way to manage pipeline lifetimes once the renderer is fully modularized and pipelines are created/destroyed by individual modules.
	 * Should replace and consolidate functionality inside the pipeline class itself.
	 */

	/**
	 * @brief A simple RAII wrapper for Vulkan pipelines and their associated pipeline layouts. This class manages the lifetime of a VkPipeline and VkPipelineLayout, ensuring that they are properly destroyed when no longer needed. The Destroy method allows for explicit cleanup, 
	 * which is preferred to ensure deterministic ordering during teardown.
	 */
	class OwnedPipeline 
	{
	public:
		OwnedPipeline() = default;
		OwnedPipeline(VkDevice device, VkPipeline pipeline, VkPipelineLayout layout) : m_Device(device), m_Pipeline(pipeline), m_Layout(layout) {}
	
		~OwnedPipeline()
		{
			if (!m_Destroyed)
			{
				// best-effort cleanup; explicit destroy(m_Device) preferred
			}
		}
	
		void Destroy(VkDevice device)
		{
			if (!m_Destroyed) {
				if (m_Pipeline != VK_NULL_HANDLE)
				{
					vkDestroyPipeline(device, m_Pipeline, nullptr);
					m_Pipeline = VK_NULL_HANDLE;
				}
				if (m_Layout != VK_NULL_HANDLE)
				{
					vkDestroyPipelineLayout(device, m_Layout, nullptr);
					m_Layout = VK_NULL_HANDLE;
				}
				m_Destroyed = true;
			}
		}
	
		VkPipeline GetPipeline() const { return m_Pipeline; }
		VkPipelineLayout GetLayout() const { return m_Layout; }
	
	private:
		VkDevice m_Device;
		VkPipeline m_Pipeline{ VK_NULL_HANDLE };
		VkPipelineLayout m_Layout{ VK_NULL_HANDLE };
		bool m_Destroyed = false;
	};

}

// -------------------------------------------------------
