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
 * image_view.cpp
 * -------------------------------------------------------
 * Created: 05/03/2026
 * -------------------------------------------------------
 */
#include "image_view.h"
#include "render_context.h"
#include "debug/graphics_debug.h"
#include <volk/volk.h>

// -------------------------------------------------------

namespace SceneryEditorX
{

	ImageView::ImageView(ImageViewSpec spec) : m_Spec(std::move(spec))
	{
		const Ref<Device> device = RenderContext::Get()->GetDevice();

		VkImageViewCreateInfo viewCI;
		viewCI.sType		= VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		viewCI.pNext		= nullptr;
		viewCI.flags		= 0;
		viewCI.image		= m_Spec.image->GetImage();
		viewCI.viewType		= VK_IMAGE_VIEW_TYPE_2D;
		viewCI.format		= m_Spec.format;
		viewCI.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
		viewCI.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
		viewCI.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
		viewCI.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
		viewCI.subresourceRange.aspectMask		= m_Spec.aspect;
		viewCI.subresourceRange.baseMipLevel	= m_Spec.mip;
		viewCI.subresourceRange.levelCount		= m_Spec.mipCount;
		viewCI.subresourceRange.baseArrayLayer	= 0;
		viewCI.subresourceRange.layerCount		= m_Spec.layerCount;

		SEDX_VK_RESULT_ASSERT(vkCreateImageView(device->GetLogicalDevice(), &viewCI, nullptr, &m_ImageView), "Failed to create image view");
		Debugging::SetResourceName(m_ImageView, ResourceType::ImageView, m_Spec.name ? m_Spec.name : "Unnamed ImageView");
	}

	ImageView::~ImageView()
	{
		QueueManager::AddDeletionQueue(ResourceType::ImageView, m_ImageView);
		m_ImageView = VK_NULL_HANDLE;
		m_Spec = {};
	}

} // namespace SceneryEditorX

// -------------------------------------------------------
