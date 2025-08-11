/**
* -------------------------------------------------------
* Scenery Editor X
* -------------------------------------------------------
* Copyright (c) 2025 Thomas Ray 
* Copyright (c) 2025 Coalition of Freeware Developers
* -------------------------------------------------------
* vk_image_view.cpp
* -------------------------------------------------------
* Created: 8/8/2025
* -------------------------------------------------------
*/
#include <format>
#include <SceneryEditorX/renderer/renderer.h>
#include <SceneryEditorX/renderer/vulkan/vk_image_view.h>
#include <SceneryEditorX/renderer/vulkan/vk_util.h>

/// -------------------------------------------------------

namespace SceneryEditorX
{
	
	ImageView::ImageView(const ImageViewData &spec) : m_Specification(spec)
	{
		Invalidate();
	}

	ImageView::~ImageView()
	{
		Renderer::SubmitResourceFree([imageView = m_ImageView]() mutable
		{
			auto device = RenderContext::GetCurrentDevice()->GetDevice();
			vkDestroyImageView(device, imageView, nullptr);
		});

		m_ImageView = nullptr;
	}

	void ImageView::Invalidate()
	{
        Ref<ImageView> instance(this);
		Renderer::Submit([instance]() mutable
		{
		    instance->Invalidate_RenderThread();
		});
	}

	void ImageView::Invalidate_RenderThread()
	{
		auto device = RenderContext::GetCurrentDevice()->GetDevice();
		Ref<Image2D> vulkanImage = m_Specification.image.As<Image2D>();
		const auto &imageSpec = vulkanImage->GetSpecification();

		VkImageAspectFlags aspectMask = IsDepthFormat(imageSpec.format) ? VK_IMAGE_ASPECT_DEPTH_BIT : VK_IMAGE_ASPECT_COLOR_BIT;
        if (imageSpec.format == RenderContext::GetCurrentDevice()->GetPhysicalDevice()->GetDepthFormat())
			aspectMask |= VK_IMAGE_ASPECT_STENCIL_BIT;

		VkFormat vulkanFormat = imageSpec.format;
		VkImageViewCreateInfo imageViewCreateInfo = {};
		imageViewCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		imageViewCreateInfo.viewType = imageSpec.layers > 1 ? VK_IMAGE_VIEW_TYPE_2D_ARRAY : VK_IMAGE_VIEW_TYPE_2D;
		imageViewCreateInfo.format = vulkanFormat;
		imageViewCreateInfo.flags = 0;
		imageViewCreateInfo.subresourceRange = {};
		imageViewCreateInfo.subresourceRange.aspectMask = aspectMask;
		imageViewCreateInfo.subresourceRange.baseMipLevel = m_Specification.mip;
		imageViewCreateInfo.subresourceRange.levelCount = 1;
		imageViewCreateInfo.subresourceRange.baseArrayLayer = 0;
		imageViewCreateInfo.subresourceRange.layerCount = imageSpec.layers;
		imageViewCreateInfo.image = vulkanImage->GetImageInfo().image;
        VK_CHECK_RESULT(vkCreateImageView(device, &imageViewCreateInfo, nullptr, &m_ImageView))
		SetDebugUtilsObjectName(device, VK_OBJECT_TYPE_IMAGE_VIEW, std::format("{} default image view", m_Specification.debugName), m_ImageView);

		m_DescriptorImageInfo = vulkanImage->GetDescriptorInfoVulkan();
		m_DescriptorImageInfo.imageView = m_ImageView;
	}

}

/// -------------------------------------------------------
