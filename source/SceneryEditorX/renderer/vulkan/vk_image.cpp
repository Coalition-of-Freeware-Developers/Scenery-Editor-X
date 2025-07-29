/**
* -------------------------------------------------------
* Scenery Editor X
* -------------------------------------------------------
* Copyright (c) 2025 Thomas Ray 
* Copyright (c) 2025 Coalition of Freeware Developers
* -------------------------------------------------------
* vk_image.cpp
* -------------------------------------------------------
* Created: 27/6/2025
* -------------------------------------------------------
*/
#include <SceneryEditorX/renderer/renderer.h>
#include <SceneryEditorX/renderer/vulkan/vk_image.h>

#include "vk_sampler.h"
#include "vk_util.h"

/// -----------------------------------------------------------

namespace SceneryEditorX
{
	static std::map<VkImage, WeakRef<Image2D>> s_ImageReferences;

    /**
	 * @brief 
	 * @param specification
	 */

	Image2D::Image2D(const ImageSpecification &specification) : m_Specification(specification)
    {
        SEDX_CORE_VERIFY(m_Specification.width > 0 && m_Specification.height > 0);
    }

	Image2D::~Image2D()
	{
        Image2D::Release();
	}

	void Image2D::Invalidate()
	{
#if INVESTIGATE
		Ref<Image2D> instance = this;
		Renderer::Submit([instance]() mutable
		{
			instance->Invalidate_RenderThread();
		});
#endif

		Invalidate_RenderThread();
	}

	void Image2D::Release()
	{
		if (m_Info.image == nullptr)
			return;

		const ImageResource &info = m_Info;
		Renderer::SubmitResourceFree([info, mipViews = m_PerMipImageViews, layerViews = m_PerLayerImageViews]() mutable
		{
			const auto vulkanDevice = RenderContext::GetCurrentDevice()->GetDevice();
			vkDestroyImageView(vulkanDevice, info.view, nullptr);
			DestroySampler(info.sampler);

			for (auto& view : mipViews)
			{
				if (view.second)
					vkDestroyImageView(vulkanDevice, view.second, nullptr);
			}
			for (auto& view : layerViews)
			{
				if (view)
					vkDestroyImageView(vulkanDevice, view, nullptr);
			}
			MemoryAllocator allocator("Image2D");
			allocator.DestroyImage(info.image, info.allocation);
			s_ImageReferences.erase(info.image);
		});
		m_Info.image = nullptr;
		m_Info.view = nullptr;
		if (m_Specification.createSampler)
			m_Info.sampler = nullptr;
		m_PerLayerImageViews.clear();
		m_PerMipImageViews.clear();

	}

	int Image2D::GetClosestMipLevel(const uint32_t width, const uint32_t height) const
	{
		if (width > m_Specification.width / 2 || height > m_Specification.height / 2)
			return 0;

		const int a = glm::log2(glm::min(m_Specification.width, m_Specification.height));
		const int b = glm::log2(glm::min(width, height));
		return a - b;
	}

	std::pair<uint32_t, uint32_t> Image2D::GetMipLevelSize(int mipLevel) const
	{
		uint32_t width = m_Specification.width;
		uint32_t height = m_Specification.height;
		return { width >> mipLevel, height >> mipLevel };
	}

	void Image2D::Invalidate_RenderThread()
	{
		SEDX_CORE_VERIFY(m_Specification.width > 0 && m_Specification.height > 0);

		/// Try release first if necessary
		Release();

		VkDevice device = RenderContext::GetCurrentDevice()->GetDevice();
		MemoryAllocator allocator("Image2D");

		VkImageUsageFlags usage = VK_IMAGE_USAGE_SAMPLED_BIT; // TODO: this (probably) shouldn't be implied
		if (m_Specification.usage == ImageUsage::DepthAttachment)
		{
			if (IsDepthFormat(m_Specification.format))
				usage |= VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
			else
				usage |= VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
		}
        if (m_Specification.usage == ImageUsage::ColorAttachment)
        {
            usage |= VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
        }
		if (m_Specification.transfer || m_Specification.usage == ImageUsage::TransferSrc)
		{
			usage |= VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
		}
        if (m_Specification.transfer || m_Specification.usage == ImageUsage::TransferDst)
        {
            usage |= VK_IMAGE_USAGE_TRANSFER_DST_BIT;
        }
		if (m_Specification.usage == ImageUsage::Storage)
		{
			usage |= VK_IMAGE_USAGE_STORAGE_BIT;
		}

		VkImageAspectFlags aspectMask = IsDepthFormat(m_Specification.format) ? VK_IMAGE_ASPECT_DEPTH_BIT : VK_IMAGE_ASPECT_COLOR_BIT;
        if (m_Specification.format == RenderContext::GetCurrentDevice()->GetPhysicalDevice()->GetDepthFormat())
			aspectMask |= VK_IMAGE_ASPECT_STENCIL_BIT;

		VkFormat vulkanFormat = m_Specification.format;
		VmaMemoryUsage memoryUsage = m_Specification.usage == ImageUsage::TransferSrc ? VMA_MEMORY_USAGE_GPU_TO_CPU : VMA_MEMORY_USAGE_GPU_ONLY;

		VkImageCreateInfo imageCreateInfo = {};
		imageCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
		imageCreateInfo.imageType = VK_IMAGE_TYPE_2D;
		imageCreateInfo.format = vulkanFormat;
		imageCreateInfo.extent.width = m_Specification.width;
		imageCreateInfo.extent.height = m_Specification.height;
		imageCreateInfo.extent.depth = 1;
		imageCreateInfo.mipLevels = m_Specification.mips;
		imageCreateInfo.arrayLayers = m_Specification.layers;
		imageCreateInfo.samples = VK_SAMPLE_COUNT_1_BIT;
        /// TODO: Evaluate later a better way to determine tiling
		imageCreateInfo.tiling = m_Specification.usage == ImageUsage::TransferSrc? VK_IMAGE_TILING_LINEAR : VK_IMAGE_TILING_OPTIMAL;
		imageCreateInfo.usage = usage;
		m_Info.allocation = allocator.AllocateImage(imageCreateInfo, memoryUsage, m_Info.image, &m_GPUAllocationSize);
		s_ImageReferences[m_Info.image] = CreateRef<Image2D>(this);
		SetDebugUtilsObjectName(device, VK_OBJECT_TYPE_IMAGE, m_Specification.debugName, m_Info.image);

		/// Create a default image view
		VkImageViewCreateInfo imageViewCreateInfo = {};
		imageViewCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		imageViewCreateInfo.viewType = m_Specification.layers > 1 ? VK_IMAGE_VIEW_TYPE_2D_ARRAY : VK_IMAGE_VIEW_TYPE_2D;
		imageViewCreateInfo.format = vulkanFormat;
		imageViewCreateInfo.flags = 0;
		imageViewCreateInfo.subresourceRange = {};
		imageViewCreateInfo.subresourceRange.aspectMask = aspectMask;
		imageViewCreateInfo.subresourceRange.baseMipLevel = 0;
		imageViewCreateInfo.subresourceRange.levelCount = m_Specification.mips;
		imageViewCreateInfo.subresourceRange.baseArrayLayer = 0;
		imageViewCreateInfo.subresourceRange.layerCount = m_Specification.layers;
		imageViewCreateInfo.image = m_Info.image;
		VK_CHECK_RESULT(vkCreateImageView(device, &imageViewCreateInfo, nullptr, &m_Info.view))
		SetDebugUtilsObjectName(device, VK_OBJECT_TYPE_IMAGE_VIEW, std::format("{} default image view", m_Specification.debugName), m_Info.view);

		// TODO: Renderer should contain some kind of sampler cache
		if (m_Specification.createSampler)
		{
			VkSamplerCreateInfo samplerCreateInfo = {};
			samplerCreateInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
			samplerCreateInfo.maxAnisotropy = 1.0f;
			if (IsIntegerBased(m_Specification.format))
			{
				samplerCreateInfo.magFilter = VK_FILTER_NEAREST;
				samplerCreateInfo.minFilter = VK_FILTER_NEAREST;
				samplerCreateInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_NEAREST;
			}
			else
			{
				samplerCreateInfo.magFilter = VK_FILTER_LINEAR;
				samplerCreateInfo.minFilter = VK_FILTER_LINEAR;
				samplerCreateInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
			}

			samplerCreateInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
			samplerCreateInfo.addressModeV = samplerCreateInfo.addressModeU;
			samplerCreateInfo.addressModeW = samplerCreateInfo.addressModeU;
			samplerCreateInfo.mipLodBias = 0.0f;
			samplerCreateInfo.minLod = 0.0f;
			samplerCreateInfo.maxLod = 100.0f;
			samplerCreateInfo.borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE;
			m_Info.sampler = CreateSampler(samplerCreateInfo);
			SetDebugUtilsObjectName(device, VK_OBJECT_TYPE_SAMPLER, std::format("{} default sampler", m_Specification.debugName), m_Info.sampler);
		}

		if (m_Specification.usage == Layout::ImageLayout::General)
		{
			/// Transition image to GENERAL layout
			VkCommandBuffer commandBuffer = RenderContext::GetCurrentDevice()->GetCommandBuffer(true);

			VkImageSubresourceRange subresourceRange = {};
			subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			subresourceRange.baseMipLevel = 0;
			subresourceRange.levelCount = m_Specification.mips;
			subresourceRange.layerCount = m_Specification.layers;

			InsertImageMemoryBarrier(commandBuffer, m_Info.image,
				0, 0,
				VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_GENERAL,
				VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
				subresourceRange);

			RenderContext::GetCurrentDevice()->FlushCmdBuffer(commandBuffer);
		}
        else if (m_Specification.usage == Layout::ImageLayout::TransferDst)
		{
			/// Transition image to TRANSFER_DST layout
			VkCommandBuffer commandBuffer = RenderContext::GetCurrentDevice()->GetCommandBuffer(true);

			VkImageSubresourceRange subresourceRange = {};
			subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			subresourceRange.baseMipLevel = 0;
			subresourceRange.levelCount = m_Specification.mips;
			subresourceRange.layerCount = m_Specification.layers;

			InsertImageMemoryBarrier(commandBuffer, m_Info.image,
				0, 0,
				VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
				VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
				subresourceRange);

			RenderContext::GetCurrentDevice()->FlushCmdBuffer(commandBuffer);
		}

		UpdateDescriptor();
	}


	void Image2D::CreatePerLayerImageViews_RenderThread()
	{
		SEDX_CORE_ASSERT(m_Specification.layers > 1);

		VkDevice device = RenderContext::GetCurrentDevice()->GetDevice();

		VkImageAspectFlags aspectMask = IsDepthFormat(m_Specification.format) ? VK_IMAGE_ASPECT_DEPTH_BIT : VK_IMAGE_ASPECT_COLOR_BIT;
		if (m_Specification.format == RenderContext::GetCurrentDevice()->GetPhysicalDevice()->GetDepthFormat())
			aspectMask |= VK_IMAGE_ASPECT_STENCIL_BIT;

		const VkFormat vulkanFormat = m_Specification.format;

		m_PerLayerImageViews.resize(m_Specification.layers);
		for (uint32_t layer = 0; layer < m_Specification.layers; layer++)
		{
			VkImageViewCreateInfo imageViewCreateInfo = {};
			imageViewCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
			imageViewCreateInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
			imageViewCreateInfo.format = vulkanFormat;
			imageViewCreateInfo.flags = 0;
			imageViewCreateInfo.subresourceRange = {};
			imageViewCreateInfo.subresourceRange.aspectMask = aspectMask;
			imageViewCreateInfo.subresourceRange.baseMipLevel = 0;
			imageViewCreateInfo.subresourceRange.levelCount = m_Specification.mips;
			imageViewCreateInfo.subresourceRange.baseArrayLayer = layer;
			imageViewCreateInfo.subresourceRange.layerCount = 1;
			imageViewCreateInfo.image = m_Info.image;
			VK_CHECK_RESULT(vkCreateImageView(device, &imageViewCreateInfo, nullptr, &m_PerLayerImageViews[layer]));
			SetDebugUtilsObjectName(device, VK_OBJECT_TYPE_IMAGE_VIEW, std::format("{} image view layer: {}", m_Specification.debugName, layer), m_PerLayerImageViews[layer]);
		}
	}

    void Image2D::CreatePerLayerImageViews()
    {
        Ref<Image2D> instance(this);
        Renderer::Submit([instance]() mutable { instance->CreatePerLayerImageViews_RenderThread(); });
    }

	VkImageView Image2D::GetRenderThreadMipImageView(const uint32_t mip)
	{
        if (const auto it = m_PerMipImageViews.find(mip); it != m_PerMipImageViews.end())
			return it->second;

		VkDevice device = RenderContext::GetCurrentDevice()->GetDevice();

		VkImageAspectFlags aspectMask = IsDepthFormat(m_Specification.format) ? VK_IMAGE_ASPECT_DEPTH_BIT : VK_IMAGE_ASPECT_COLOR_BIT;
        if (m_Specification.format == RenderContext::GetCurrentDevice()->GetPhysicalDevice()->GetDepthFormat())
			aspectMask |= VK_IMAGE_ASPECT_STENCIL_BIT;

		VkFormat vulkanFormat = m_Specification.format;

		VkImageViewCreateInfo imageViewCreateInfo = {};
		imageViewCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		imageViewCreateInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
		imageViewCreateInfo.format = vulkanFormat;
		imageViewCreateInfo.flags = 0;
		imageViewCreateInfo.subresourceRange = {};
		imageViewCreateInfo.subresourceRange.aspectMask = aspectMask;
		imageViewCreateInfo.subresourceRange.baseMipLevel = mip;
		imageViewCreateInfo.subresourceRange.levelCount = 1;
		imageViewCreateInfo.subresourceRange.baseArrayLayer = 0;
		imageViewCreateInfo.subresourceRange.layerCount = 1;
		imageViewCreateInfo.image = m_Info.image;

		VK_CHECK_RESULT(vkCreateImageView(device, &imageViewCreateInfo, nullptr, &m_PerMipImageViews[mip]))
		SetDebugUtilsObjectName(device, VK_OBJECT_TYPE_IMAGE_VIEW, std::format("{} image view mip: {}", m_Specification.debugName, mip), m_PerMipImageViews[mip]);
		return m_PerMipImageViews.at(mip);
	}

    VkImageView Image2D::GetMipImageView(uint32_t mip)
    {
        if (!m_PerMipImageViews.contains(mip))
        {
            Ref<Image2D> instance(this);
            Renderer::Submit([instance, mip]() mutable { instance->GetRenderThreadMipImageView(mip); });
            return nullptr;
        }

        return m_PerMipImageViews.at(mip);
    }

	void Image2D::CreatePerSpecificLayerImageViews_RenderThread(const std::vector<uint32_t> &layerIndices)
	{
		SEDX_CORE_ASSERT(m_Specification.layers > 1);

		VkDevice device = RenderContext::GetCurrentDevice()->GetDevice();

		VkImageAspectFlags aspectMask = IsDepthFormat(m_Specification.format) ? VK_IMAGE_ASPECT_DEPTH_BIT : VK_IMAGE_ASPECT_COLOR_BIT;
        if (m_Specification.format == RenderContext::GetCurrentDevice()->GetPhysicalDevice()->GetDepthFormat())
			aspectMask |= VK_IMAGE_ASPECT_STENCIL_BIT;

		const VkFormat vulkanFormat = m_Specification.format;

		SEDX_CORE_ASSERT(m_PerLayerImageViews.size() == m_Specification.layers);
		if (m_PerLayerImageViews.empty())
			m_PerLayerImageViews.resize(m_Specification.layers);

		for (uint32_t layer : layerIndices)
		{
			VkImageViewCreateInfo imageViewCreateInfo = {};
			imageViewCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
			imageViewCreateInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
			imageViewCreateInfo.format = vulkanFormat;
			imageViewCreateInfo.flags = 0;
			imageViewCreateInfo.subresourceRange = {};
			imageViewCreateInfo.subresourceRange.aspectMask = aspectMask;
			imageViewCreateInfo.subresourceRange.baseMipLevel = 0;
			imageViewCreateInfo.subresourceRange.levelCount = m_Specification.mips;
			imageViewCreateInfo.subresourceRange.baseArrayLayer = layer;
			imageViewCreateInfo.subresourceRange.layerCount = 1;
			imageViewCreateInfo.image = m_Info.image;
			VK_CHECK_RESULT(vkCreateImageView(device, &imageViewCreateInfo, nullptr, &m_PerLayerImageViews[layer]));
			SetDebugUtilsObjectName(device, VK_OBJECT_TYPE_IMAGE_VIEW, std::format("{} image view layer: {}", m_Specification.debugName, layer), m_PerLayerImageViews[layer]);
		}

	}

	void Image2D::UpdateDescriptor()
	{
        if (m_Specification.format == RenderContext::GetCurrentDevice()->GetPhysicalDevice()->GetDepthFormat() ||
            m_Specification.format == VkFormat::VK_FORMAT_D32_SFLOAT ||
            m_Specification.format == VkFormat::VK_FORMAT_D32_SFLOAT_S8_UINT)
			m_DescriptorImageInfo.imageLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;
		else if (m_Specification.usage == ImageUsage::Storage)
			m_DescriptorImageInfo.imageLayout = VK_IMAGE_LAYOUT_GENERAL;
		else
			m_DescriptorImageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

		if (m_Specification.usage == ImageUsage::Storage)
			m_DescriptorImageInfo.imageLayout = VK_IMAGE_LAYOUT_GENERAL;
        else if (m_Specification.usage == Layout::ImageLayout::TransferDst)
			m_DescriptorImageInfo.imageLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;

		m_DescriptorImageInfo.imageView = m_Info.view;
		m_DescriptorImageInfo.sampler = m_Info.sampler;

		//SEDX_CORE_WARN_TAG("Renderer", "Image2D::UpdateDescriptor to ImageView = {0}", (const void*)m_Info.ImageView);
	}

	const std::map<VkImage, WeakRef<Image2D>>& Image2D::GetImageRefs()
	{
		return s_ImageReferences;
	}

	void Image2D::SetData(const Buffer buffer)
	{
		SEDX_CORE_VERIFY(m_Specification.transfer, "Image must be created with ImageSpecification::Transfer enabled!");

		if (buffer)
		{
			const Ref<VulkanDevice> device = RenderContext::GetCurrentDevice();
			const VkDeviceSize size = buffer.size;

			VkMemoryAllocateInfo memAllocInfo{};
			memAllocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;

			MemoryAllocator allocator("Image2D");

			///TODO: Replace with  
			/// Create staging buffer
			VkBufferCreateInfo bufferCreateInfo{};
			bufferCreateInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
			bufferCreateInfo.size = size;
			bufferCreateInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
			bufferCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
			VkBuffer stagingBuffer;
			VmaAllocation stagingBufferAllocation = allocator.AllocateBuffer(bufferCreateInfo, VMA_MEMORY_USAGE_CPU_TO_GPU, stagingBuffer);

			/// Copy data to staging buffer
			uint8_t* destData = allocator.MapMemory<uint8_t>(stagingBufferAllocation);
			SEDX_CORE_VERIFY(buffer.data);
			memcpy(destData, buffer.data, size);
            MemoryAllocator::UnmapMemory(stagingBufferAllocation);

			VkCommandBuffer copyCmd = device->GetCommandBuffer(true);

			/// Image memory barriers for the texture image

			/// The sub resource range describes the regions of the image that will be transitioned using the memory barriers below
			VkImageSubresourceRange subresourceRange = {};
			/// Image only contains color data
			subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			/// Start at first mip level
			subresourceRange.baseMipLevel = 0;
			subresourceRange.levelCount = 1;
			subresourceRange.layerCount = 1;

			/// Transition the texture image layout to transfer target, so we can safely copy our buffer data to it.
			VkImageMemoryBarrier imageMemoryBarrier{};
			imageMemoryBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
			imageMemoryBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
			imageMemoryBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
			imageMemoryBarrier.image = m_Info.image;
			imageMemoryBarrier.subresourceRange = subresourceRange;
			imageMemoryBarrier.srcAccessMask = 0;
			imageMemoryBarrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
			imageMemoryBarrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
			imageMemoryBarrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;

			/// Insert a memory dependency at the proper pipeline stages that will execute the image layout transition 
			/// Source pipeline stage is host write/read exection (VK_PIPELINE_STAGE_HOST_BIT)
			/// Destination pipeline stage is copy command exection (VK_PIPELINE_STAGE_TRANSFER_BIT)
			vkCmdPipelineBarrier(
				copyCmd,
				VK_PIPELINE_STAGE_HOST_BIT,
				VK_PIPELINE_STAGE_TRANSFER_BIT,
				0,
				0, nullptr,
				0, nullptr,
				1, &imageMemoryBarrier);

			VkBufferImageCopy bufferCopyRegion = {};
			bufferCopyRegion.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			bufferCopyRegion.imageSubresource.mipLevel = 0;
			bufferCopyRegion.imageSubresource.baseArrayLayer = 0;
			bufferCopyRegion.imageSubresource.layerCount = 1;
			bufferCopyRegion.imageExtent.width = m_Specification.width;
			bufferCopyRegion.imageExtent.height = m_Specification.height;
			bufferCopyRegion.imageExtent.depth = 1;
			bufferCopyRegion.bufferOffset = 0;

			/// Copy mip levels from staging buffer
			vkCmdCopyBufferToImage(
				copyCmd,
				stagingBuffer,
				m_Info.image,
				VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
				1,
				&bufferCopyRegion);

#if 0
			/// Once the data has been uploaded we transfer to the texture image to the shader read layout, so it can be sampled from
			imageMemoryBarrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
			imageMemoryBarrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
			imageMemoryBarrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
			imageMemoryBarrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

			/**
			 * Insert a memory dependency at the proper pipeline stages that will execute the image layout transition 
			 * Source pipeline stage is copy command execution (VK_PIPELINE_STAGE_TRANSFER_BIT)
			 * Destination pipeline stage fragment shader access (VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT)
             */
			vkCmdPipelineBarrier(
				copyCmd,
				VK_PIPELINE_STAGE_TRANSFER_BIT,
				VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
				0,
				0, nullptr,
				0, nullptr,
				1, &imageMemoryBarrier);

#endif

			InsertImageMemoryBarrier(copyCmd, m_Info.image,
				VK_ACCESS_TRANSFER_READ_BIT, VK_ACCESS_SHADER_READ_BIT,
				VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, m_DescriptorImageInfo.imageLayout,
				VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
				subresourceRange);


			device->FlushCmdBuffer(copyCmd);

			/// Clean up staging resources
			allocator.DestroyBuffer(stagingBuffer, stagingBufferAllocation);

			UpdateDescriptor();
		}
	}

	void Image2D::CopyToHostBuffer(Buffer& buffer) const
	{
		auto device = RenderContext::GetCurrentDevice();
		auto vulkanDevice = device->GetDevice();
		MemoryAllocator allocator("Image2D");

		uint64_t bufferSize = m_Specification.width * m_Specification.height * getBPP(m_Specification.format);

		/// Create staging buffer
		VkBufferCreateInfo bufferCreateInfo{};
		bufferCreateInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
		bufferCreateInfo.size = bufferSize;
		bufferCreateInfo.usage = VK_BUFFER_USAGE_TRANSFER_DST_BIT;
		bufferCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

#if MEM_INFO
		VkMemoryRequirements memReqs;
		vkGetImageMemoryRequirements(vulkanDevice, m_Info.image, &memReqs);
		SEDX_CORE_WARN("MemReq = {} ({})", memReqs.size, memReqs.alignment);
		SEDX_CORE_WARN("Expected size = {}", bufferSize);
#endif

		VkBuffer stagingBuffer;
		VmaAllocation stagingBufferAllocation = allocator.AllocateBuffer(bufferCreateInfo, VMA_MEMORY_USAGE_GPU_TO_CPU, stagingBuffer);

		uint32_t mipCount = 1;
		uint32_t mipWidth = m_Specification.width, mipHeight = m_Specification.height;

		VkCommandBuffer copyCmd = device->GetCommandBuffer(true);

		VkImageSubresourceRange subresourceRange = {};
		subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		subresourceRange.baseMipLevel = 0;
		subresourceRange.levelCount = mipCount;
		subresourceRange.layerCount = 1;

		InsertImageMemoryBarrier(copyCmd, m_Info.image,VK_ACCESS_TRANSFER_READ_BIT, 0,m_DescriptorImageInfo.imageLayout,
								 VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
								 VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
								 VK_PIPELINE_STAGE_TRANSFER_BIT,subresourceRange);

		uint64_t mipDataOffset = 0;
		for (uint32_t mip = 0; mip < mipCount; mip++)
		{
			VkBufferImageCopy bufferCopyRegion = {};
			bufferCopyRegion.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			bufferCopyRegion.imageSubresource.mipLevel = mip;
			bufferCopyRegion.imageSubresource.baseArrayLayer = 0;
			bufferCopyRegion.imageSubresource.layerCount = 1;
			bufferCopyRegion.imageExtent.width = mipWidth;
			bufferCopyRegion.imageExtent.height = mipHeight;
			bufferCopyRegion.imageExtent.depth = 1;
			bufferCopyRegion.bufferOffset = mipDataOffset;

			vkCmdCopyImageToBuffer(copyCmd,m_Info.image,VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,stagingBuffer,1,&bufferCopyRegion);

			uint64_t mipDataSize = mipWidth * mipHeight * sizeof(float) * 4 * 6;
			mipDataOffset += mipDataSize;
			mipWidth /= 2;
			mipHeight /= 2;
		}

		InsertImageMemoryBarrier(copyCmd, m_Info.image,
										VK_ACCESS_TRANSFER_READ_BIT, 0,
										VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, m_DescriptorImageInfo.imageLayout,
										VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
										subresourceRange);

		device->FlushCmdBuffer(copyCmd);

		/// Copy data from staging buffer
		uint8_t* srcData = allocator.MapMemory<uint8_t>(stagingBufferAllocation);
		buffer.Allocate(bufferSize);
		memcpy(buffer.data, srcData, bufferSize);
        MemoryAllocator::UnmapMemory(stagingBufferAllocation);

		allocator.DestroyBuffer(stagingBuffer, stagingBufferAllocation);
	}

	ImageView::ImageView(ImageViewData spec) : m_Specification(std::move(spec))
	{
		Invalidate();
	}

	ImageView::~ImageView()
	{
		Renderer::SubmitResourceFree([imageView = m_ImageView]() mutable
		{
			const auto device = RenderContext::GetCurrentDevice()->GetDevice();
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
		const auto device = RenderContext::GetCurrentDevice()->GetDevice();
		Ref<Image2D> vulkanImage = m_Specification.image.As<Image2D>();
		const auto& imageSpec = vulkanImage->GetSpecification();

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

/// -----------------------------------------------------------
