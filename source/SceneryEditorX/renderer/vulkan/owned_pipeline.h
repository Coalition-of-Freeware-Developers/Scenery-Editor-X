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
	
	// Simple RAII for pipeline + layout. Provides explicit destroy(m_Device)
	// to ensure deterministic ordering during teardown.
	class OwnedPipeline 
    {
	public:
	    OwnedPipeline() = default;
	    OwnedPipeline(VkDevice device, VkPipeline pipeline, VkPipelineLayout layout)
	        : device_(device), pipeline_(pipeline), layout_(layout) {}
	
	    ~OwnedPipeline()
	    {
	        if (!destroyed_)
			{
	            // best-effort cleanup; explicit destroy(m_Device) preferred
	        }
	    }
	
	    void Destroy(VkDevice device)
	    {
	        if (!destroyed_) {
	            if (pipeline_ != VK_NULL_HANDLE)
				{
	                vkDestroyPipeline(device, pipeline_, nullptr);
	                pipeline_ = VK_NULL_HANDLE;
	            }
	            if (layout_ != VK_NULL_HANDLE)
				{
	                vkDestroyPipelineLayout(device, layout_, nullptr);
	                layout_ = VK_NULL_HANDLE;
	            }
	            destroyed_ = true;
	        }
	    }
	
	    VkPipeline GetPipeline() const { return pipeline_; }
	    VkPipelineLayout GetLayout() const { return layout_; }
	
	private:
	    VkDevice device_{ VK_NULL_HANDLE };
	    VkPipeline pipeline_{ VK_NULL_HANDLE };
	    VkPipelineLayout layout_{ VK_NULL_HANDLE };
	    bool destroyed_{ false };
	};

}

// -------------------------------------------------------
