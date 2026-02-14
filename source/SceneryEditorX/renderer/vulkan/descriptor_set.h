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
 * descriptor_set.h
 * -------------------------------------------------------
 * Created: 09/02/2026
 * -------------------------------------------------------
 */
#pragma once
#include "descriptor.h"
#include <vector>
#include <volk/volk.h>

// -----------------------------------------------------------------

namespace SceneryEditorX
{
    class Device;

	// RAII wrapper for descriptor layout + pool + set. Uses the existing Descriptor
	// helper to create resources and exposes an explicit destroy(m_Device) method.
	class DescriptorSet
	{
	public:
		DescriptorSet() = default;
		DescriptorSet(const std::vector<::VkDescriptorImageInfo>& textureDescriptors);
		~DescriptorSet();

        void Destroy();

        VkDescriptorSetLayout GetLayout() const { return m_Layout; }
	    VkDescriptorPool GetPool() const { return m_DescriptorPool; }
	    VkDescriptorSet GetSet() const { return m_Set; }
	
	private:
	    Ref<Device> m_Device;
	    VkDescriptorSetLayout m_Layout = VK_NULL_HANDLE;
	    VkDescriptorPool m_DescriptorPool = VK_NULL_HANDLE;
	    VkDescriptorSet m_Set = VK_NULL_HANDLE;
	    bool m_Destroyed = false;
	};



    } // namespace SceneryEditorX

// -----------------------------------------------------------------
