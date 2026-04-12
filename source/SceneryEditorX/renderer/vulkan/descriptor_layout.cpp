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
 * descriptor_layout.cpp
 * -------------------------------------------------------
 * Created: 06/04/2026
 * -------------------------------------------------------
 */
#include "descriptor_layout.h"
#include "queue_manager.h"
#include "render_context.h"
#include "debug/graphics_debug.h"
#include <SceneryEditorX/logging/logging.hpp>

// -------------------------------------------------------

namespace SceneryEditorX
{
	Ref<DescriptorLayout> DescriptorLayout::Create(const VkDescriptorSetLayoutCreateInfo& createInfo, const char* debugName)
	{
		const Ref<Device> &device = RenderContext::Get()->GetLogicalDevice();
		SEDX_CORE_ASSERT(device.IsValid(), "DescriptorLayout::Create requires a valid device");

		VkDescriptorSetLayout layout = VK_NULL_HANDLE;
		const VkResult result = vkCreateDescriptorSetLayout(device->GetLogicalDevice(), &createInfo, nullptr, &layout);
		if (result != VK_SUCCESS)
		{
			SEDX_CORE_ERROR_TAG("DescriptorLayout", "vkCreateDescriptorSetLayout failed: {}", static_cast<int>(result));
			return nullptr;
		}

		if (debugName)
		{
			Debugging::SetResourceName(layout, ResourceType::DescriptorSetLayout, debugName);
		}

		return CreateRef<DescriptorLayout>(layout);
	}

	DescriptorLayout::DescriptorLayout(VkDescriptorSetLayout layout) : m_Layout(layout)
	{
		const Ref<Device> &device = RenderContext::Get()->GetLogicalDevice();
		SEDX_CORE_ASSERT(device.IsValid(), "DescriptorLayout constructor requires a valid device");
		m_Device = device;
	}

	DescriptorLayout::~DescriptorLayout()
	{
		if (m_Layout != VK_NULL_HANDLE)
		{
			QueueManager::AddDeletionQueue(ResourceType::DescriptorSetLayout, m_Layout);
			m_Layout = VK_NULL_HANDLE;
			m_Device.Reset();
		}
	}

} // namespace SceneryEditorX

// -------------------------------------------------------
