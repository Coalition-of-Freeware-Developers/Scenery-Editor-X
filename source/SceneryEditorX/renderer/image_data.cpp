/**
* -------------------------------------------------------
* Scenery Editor X
* -------------------------------------------------------
* Copyright (c) 2025 Thomas Ray
* Copyright (c) 2025 Coalition of Freeware Developers
* -------------------------------------------------------
* image_data.cpp
* -------------------------------------------------------
* Created: 11/5/2025
* -------------------------------------------------------
*/
// ReSharper disable CommentTypo
#include <imgui/imgui.h>
#include <SceneryEditorX/renderer/image_data.h>

/// -------------------------------------------------------

namespace SceneryEditorX
{
    /**
     * @brief Retrieves the unique resource ID for this image.
     *
     * This method asserts that the underlying resource has a valid resource ID assigned.
     * The resource ID is used to uniquely identify the image within the graphics system.
     *
     * @return The unique resource ID as a uint32_t.
     * @throws Assertion failure if the resource ID is invalid (-1).
     */
    uint32_t Image::ID() const
    {
        SEDX_ASSERT(resource->resourceID != -1, "Invalid Image Resource ID!");
        return static_cast<uint32_t>(resource->resourceID);
    }

	/**
	 * @brief Retrieves the ImGui texture ID for the first layer of this image.
	 *
	 * This method returns the ImGui texture ID (ImTextureID) associated with the first layer
	 * of the image resource. The ImGui texture ID is used to display the image in ImGui widgets.
	 * If the image resource is invalid or does not have any ImGui texture IDs assigned,
	 * the method returns a null ImTextureID.
	 *
	 * @return ImTextureID for the first image layer, or nullptr if unavailable.
	 */
	ImTextureID Image::ImGuiRID() const
	{
	    if (!resource || resource->resourceID == -1 || resource->imguiRIDs.empty())
            return reinterpret_cast<ImTextureID>(nullptr);

        return resource->imguiRIDs[0];
	}

	/**
	 * @brief Retrieves the ImGui texture ID for a specific image layer.
	 *
	 * This method returns the ImGui texture ID (ImTextureID) associated with the specified layer
	 * of the image resource. The ImGui texture ID is used to display the image in ImGui widgets.
	 * If the image resource is invalid, the resource ID is not assigned, or the requested layer
	 * does not have an associated ImGui texture ID, the method returns a null ImTextureID.
	 *
	 * @param layer The index of the image layer for which to retrieve the ImGui texture ID.
	 * @return ImTextureID for the specified image layer, or nullptr if unavailable.
	 */
	ImTextureID Image::ImGuiRID(uint64_t layer) const
	{
		if (!resource || resource->resourceID == -1 || resource->imguiRIDs.size() <= layer)
            return reinterpret_cast<ImTextureID>(nullptr);

        return resource->imguiRIDs[layer];
	}

    void InsertImageMemoryBarrier(const VkCommandBuffer cmdbuffer, const VkImage image,
        const VkAccessFlags srcAccessMask, const VkAccessFlags dstAccessMask,
        const VkImageLayout oldImageLayout, const VkImageLayout newImageLayout,
        const VkPipelineStageFlags srcStageMask, const VkPipelineStageFlags dstStageMask,
        const VkImageSubresourceRange &subresourceRange)
    {
        VkImageMemoryBarrier imageMemoryBarrier{};
        imageMemoryBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        imageMemoryBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        imageMemoryBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;

        imageMemoryBarrier.srcAccessMask = srcAccessMask;
        imageMemoryBarrier.dstAccessMask = dstAccessMask;
        imageMemoryBarrier.oldLayout = oldImageLayout;
        imageMemoryBarrier.newLayout = newImageLayout;
        imageMemoryBarrier.image = image;
        imageMemoryBarrier.subresourceRange = subresourceRange;

        vkCmdPipelineBarrier(cmdbuffer, srcStageMask, dstStageMask, 0, 0, nullptr, 0, nullptr, 1, &imageMemoryBarrier);
    }

    void SetImageLayout(const VkCommandBuffer cmdbuffer, const VkImage image,
                        const VkImageLayout oldImageLayout, const VkImageLayout newImageLayout,
                        const VkImageSubresourceRange &subresourceRange,
                        const VkPipelineStageFlags srcStageMask, const VkPipelineStageFlags dstStageMask)
    {
        /// Create an image barrier object
        VkImageMemoryBarrier imageMemoryBarrier = {};
        imageMemoryBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        imageMemoryBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        imageMemoryBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        imageMemoryBarrier.oldLayout = oldImageLayout;
        imageMemoryBarrier.newLayout = newImageLayout;
        imageMemoryBarrier.image = image;
        imageMemoryBarrier.subresourceRange = subresourceRange;

        /// Source layouts (old)
        /// Source access mask controls actions that have to be finished on the old layout
        /// before it will be transitioned to the new layout
        switch (oldImageLayout)
        {
        case VK_IMAGE_LAYOUT_UNDEFINED:
            /// Image layout is undefined (or does not matter)
            /// Only valid as initial layout
            /// No flags required, listed only for completeness
            imageMemoryBarrier.srcAccessMask = 0;
            break;

        case VK_IMAGE_LAYOUT_PREINITIALIZED:
            /// Image is preinitialized
            /// Only valid as initial layout for linear images, preserves memory contents
            /// Make sure host writes have been finished
            imageMemoryBarrier.srcAccessMask = VK_ACCESS_HOST_WRITE_BIT;
            break;

        case VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL:
            /// Image is a color attachment
            /// Make sure any writes to the color buffer have been finished
            imageMemoryBarrier.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
            break;

        case VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL:
            /// Image is a depth/stencil attachment
            /// Make sure any writes to the depth/stencil buffer have been finished
            imageMemoryBarrier.srcAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
            break;

        case VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL:
            /// Image is a transfer source
            /// Make sure any reads from the image have been finished
            imageMemoryBarrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
            break;

        case VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL:
            /// Image is a transfer destination
            /// Make sure any writes to the image have been finished
            imageMemoryBarrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
            break;

        case VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL:
            /// Image is read by a shader
            /// Make sure any shader reads from the image have been finished
            imageMemoryBarrier.srcAccessMask = VK_ACCESS_SHADER_READ_BIT;
            break;
        default:
            /// Other source layouts aren't handled (yet)
            break;
        }

        /// Target layouts (new)
        /// Destination access mask controls the dependency for the new image layout
        switch (newImageLayout)
        {
        case VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL:
            /// Image will be used as a transfer destination
            /// Make sure any writes to the image have been finished
            imageMemoryBarrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
            break;

        case VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL:
            /// Image will be used as a transfer source
            /// Make sure any reads from the image have been finished
            imageMemoryBarrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
            break;

        case VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL:
            /// Image will be used as a color attachment
            /// Make sure any writes to the color buffer have been finished
            imageMemoryBarrier.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
            break;

        case VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL:
            /// Image layout will be used as a depth/stencil attachment
            /// Make sure any writes to depth/stencil buffer have been finished
            imageMemoryBarrier.dstAccessMask =
                imageMemoryBarrier.dstAccessMask | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
            break;

        case VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL:
            /// Image will be read in a shader (sampler, input attachment)
            /// Make sure any writes to the image have been finished
            if (imageMemoryBarrier.srcAccessMask == 0)
            {
                imageMemoryBarrier.srcAccessMask = VK_ACCESS_HOST_WRITE_BIT | VK_ACCESS_TRANSFER_WRITE_BIT;
            }
            imageMemoryBarrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
            break;
        default:
            /// Other source layouts aren't handled (yet)
            break;
        }

        /// Put barrier inside setup command buffer
        vkCmdPipelineBarrier(cmdbuffer, srcStageMask, dstStageMask, 0, 0, nullptr, 0, nullptr, 1, &imageMemoryBarrier);
    }

    void SetImageLayout(const VkCommandBuffer cmdbuffer, const VkImage image, const VkImageAspectFlags aspectMask,
						const VkImageLayout oldImageLayout, const VkImageLayout newImageLayout,
						const VkPipelineStageFlags srcStageMask, const VkPipelineStageFlags dstStageMask)
    {
        VkImageSubresourceRange subresourceRange = {};
        subresourceRange.aspectMask = aspectMask;
        subresourceRange.baseMipLevel = 0;
        subresourceRange.levelCount = 1;
        subresourceRange.layerCount = 1;
        SetImageLayout(cmdbuffer, image, oldImageLayout, newImageLayout, subresourceRange, srcStageMask, dstStageMask);
    }

    /*
    VkImageView Image2D::GetMipImageView(uint32_t mip)
    {
        if (m_PerMipImageViews.find(mip) == m_PerMipImageViews.end())
        {
            Ref<Image2D> instance = this;
            Renderer::Submit([instance, mip]() mutable { instance->RT_GetMipImageView(mip); });
            return nullptr;
        }

        return m_PerMipImageViews.at(mip);
    }
    */

	/*
    VkImageView Image2D::RT_GetMipImageView(const uint32_t mip)
    {
        auto it = m_PerMipImageViews.find(mip);
        if (it != m_PerMipImageViews.end())
            return it->second;

        VkDevice device = RenderContext::GetCurrentDevice()->GetDevice();

        VkImageAspectFlags aspectMask = IsDepthFormat(m_Specification.Format) ? VK_IMAGE_ASPECT_DEPTH_BIT : VK_IMAGE_ASPECT_COLOR_BIT;
        if (m_Specification.Format == VkFormat::DEPTH24STENCIL8)
            aspectMask |= VK_IMAGE_ASPECT_STENCIL_BIT;

        VkFormat vulkanFormat = Utils::VulkanImageFormat(m_Specification.Format);

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
        imageViewCreateInfo.image = m_Info.Image;

        VK_CHECK_RESULT(vkCreateImageView(device, &imageViewCreateInfo, nullptr, &m_PerMipImageViews[mip]));
        SetDebugUtilsObjectName(device,VK_OBJECT_TYPE_IMAGE_VIEW,std::format("{} image view mip: {}", m_Specification.debugName, mip), m_PerMipImageViews[mip]);
        return m_PerMipImageViews.at(mip);
    }
    */

    /*
    void Image2D::RT_CreatePerSpecificLayerImageViews(const std::vector<uint32_t> &layerIndices)
    {
        SEDX_CORE_ASSERT(m_Specification.Layers > 1);

        VkDevice device = RenderContext::GetCurrentDevice()->GetDevice();

        VkImageAspectFlags aspectMask = IsDepthFormat(m_Specification.Format) ? VK_IMAGE_ASPECT_DEPTH_BIT : VK_IMAGE_ASPECT_COLOR_BIT;
        if (m_Specification.Format == VkFormat::DEPTH24STENCIL8)
            aspectMask |= VK_IMAGE_ASPECT_STENCIL_BIT;

        const VkFormat vulkanFormat = VkFormat(m_Specification.Format);

        if (m_PerLayerImageViews.empty())
            m_PerLayerImageViews.resize(m_Specification.Layers);

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
            imageViewCreateInfo.subresourceRange.levelCount = m_Specification.Mips;
            imageViewCreateInfo.subresourceRange.baseArrayLayer = layer;
            imageViewCreateInfo.subresourceRange.layerCount = 1;
            imageViewCreateInfo.image = m_Info.Image;
            VK_CHECK_RESULT(vkCreateImageView(device, &imageViewCreateInfo, nullptr, &m_PerLayerImageViews[layer]));
            SetDebugUtilsObjectName(device,VK_OBJECT_TYPE_IMAGE_VIEW, std::format("{} image view layer: {}", m_Specification.debugName, layer), m_PerLayerImageViews[layer]);
        }
    }
    */

    /*
    void Image2D::UpdateDescriptor()
    {
        if (m_Specification.Format == VkFormat::DEPTH24STENCIL8 || m_Specification.Format == VkFormat::DEPTH32F ||
            m_Specification.Format == VkFormat::VK_FORMAT_D32_SFLOAT_S8_UINT)
            m_DescriptorImageInfo.imageLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;
        else if (m_Specification.Usage == ImageUsage::Storage)
            m_DescriptorImageInfo.imageLayout = VK_IMAGE_LAYOUT_GENERAL;
        else
            m_DescriptorImageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

        if (m_Specification.Usage == ImageUsage::Storage)
            m_DescriptorImageInfo.imageLayout = VK_IMAGE_LAYOUT_GENERAL;
        else if (m_Specification.Usage == ImageUsage::HostRead)
            m_DescriptorImageInfo.imageLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;

        m_DescriptorImageInfo.imageView = m_Info.ImageView;
        m_DescriptorImageInfo.sampler = m_Info.Sampler;
    }
    */

    /*
    const std::map<VkImage, WeakRef<Image2D>> &Image2D::GetImageRefs()
    {
        return s_ImageReferences;
    }
    */

    /*
    void Image2D::SetData(Buffer buffer)
    {
        SEDX_CORE_VERIFY(m_Specification.Transfer, "Image must be created with ImageSpecification::Transfer enabled!");

        if (buffer)
        {
            Ref<VulkanDevice> device = RenderContext::GetCurrentDevice();

            VkDeviceSize size = buffer.Size;

            VkMemoryAllocateInfo memAllocInfo{};
            memAllocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;

            MemoryAllocator allocator("Image2D");

            /// Create staging buffer
            VkBufferCreateInfo bufferCreateInfo{};
            bufferCreateInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
            bufferCreateInfo.size = size;
            bufferCreateInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
            bufferCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
            VkBuffer stagingBuffer;
            VmaAllocation stagingBufferAllocation =
                allocator.AllocateBuffer(bufferCreateInfo, VMA_MEMORY_USAGE_CPU_TO_GPU, stagingBuffer);

            // Copy data to staging buffer
            uint8_t *destData = allocator.MapMemory<uint8_t>(stagingBufferAllocation);
            SEDX_CORE_VERIFY(buffer.Data);
            memcpy(destData, buffer.Data, size);
            MemoryAllocator::UnmapMemory(stagingBufferAllocation);

            VkCommandBuffer copyCmd = device->GetCmdBuffer(true);

            // Image memory barriers for the texture image

            // The sub resource range describes the regions of the image that will be transitioned using the memory barriers below
            VkImageSubresourceRange subresourceRange = {};
            // Image only contains color data
            subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            // Start at first mip level
            subresourceRange.baseMipLevel = 0;
            subresourceRange.levelCount = 1;
            subresourceRange.layerCount = 1;

            // Transition the texture image layout to transfer target, so we can safely copy our buffer data to it.
            VkImageMemoryBarrier imageMemoryBarrier{};
            imageMemoryBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
            imageMemoryBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            imageMemoryBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            imageMemoryBarrier.image = m_Info.Image;
            imageMemoryBarrier.subresourceRange = subresourceRange;
            imageMemoryBarrier.srcAccessMask = 0;
            imageMemoryBarrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
            imageMemoryBarrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
            imageMemoryBarrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;

            // Insert a memory dependency at the proper pipeline stages that will execute the image layout transition
            // Source pipeline stage is host write/read execution (VK_PIPELINE_STAGE_HOST_BIT)
            // Destination pipeline stage is copy command execution (VK_PIPELINE_STAGE_TRANSFER_BIT)
            vkCmdPipelineBarrier(copyCmd,
                                 VK_PIPELINE_STAGE_HOST_BIT,
                                 VK_PIPELINE_STAGE_TRANSFER_BIT,
                                 0,
                                 0,
                                 nullptr,
                                 0,
                                 nullptr,
                                 1,
                                 &imageMemoryBarrier);

            VkBufferImageCopy bufferCopyRegion = {};
            bufferCopyRegion.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            bufferCopyRegion.imageSubresource.mipLevel = 0;
            bufferCopyRegion.imageSubresource.baseArrayLayer = 0;
            bufferCopyRegion.imageSubresource.layerCount = 1;
            bufferCopyRegion.imageExtent.width = m_Specification.Width;
            bufferCopyRegion.imageExtent.height = m_Specification.Height;
            bufferCopyRegion.imageExtent.depth = 1;
            bufferCopyRegion.bufferOffset = 0;

            // Copy mip levels from staging buffer
            vkCmdCopyBufferToImage(copyCmd, stagingBuffer, m_Info.Image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,1, &bufferCopyRegion);

#if 0
			// Once the data has been uploaded we transfer to the texture image to the shader read layout, so it can be sampled from
			imageMemoryBarrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
			imageMemoryBarrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
			imageMemoryBarrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
			imageMemoryBarrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

			// Insert a memory dependency at the proper pipeline stages that will execute the image layout transition
			// Source pipeline stage is copy command execution (VK_PIPELINE_STAGE_TRANSFER_BIT)
			// Destination pipeline stage fragment shader access (VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT)
			vkCmdPipelineBarrier(
				copyCmd,
				VK_PIPELINE_STAGE_TRANSFER_BIT,
				VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
				0,
				0, nullptr,
				0, nullptr,
				1, &imageMemoryBarrier);

#endif

            Utils::InsertImageMemoryBarrier(copyCmd,
                                            m_Info.Image,
                                            VK_ACCESS_TRANSFER_READ_BIT,
                                            VK_ACCESS_SHADER_READ_BIT,
                                            VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                                            m_DescriptorImageInfo.imageLayout,
                                            VK_PIPELINE_STAGE_TRANSFER_BIT,
                                            VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
                                            subresourceRange);


            device->FlushCommandBuffer(copyCmd);

            /// Clean up staging resources
            allocator.DestroyBuffer(stagingBuffer, stagingBufferAllocation);

            UpdateDescriptor();
        }
    }
    */

    /*
    void Image2D::CopyToHostBuffer(Buffer &buffer) const
    {
        auto device = RenderContext::GetCurrentDevice();
        auto vulkanDevice = device->GetDevice();
        MemoryAllocator allocator("Image2D");

        uint64_t bufferSize =
            m_Specification.Width * m_Specification.Height * Utils::GetImageFormatBPP(m_Specification.Format);

        // Create staging buffer
        VkBufferCreateInfo bufferCreateInfo{};
        bufferCreateInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        bufferCreateInfo.size = bufferSize;
        bufferCreateInfo.usage = VK_BUFFER_USAGE_TRANSFER_DST_BIT;
        bufferCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

#if MEM_INFO
        VkMemoryRequirements memReqs;
        vkGetImageMemoryRequirements(vulkanDevice, m_Info.Image, &memReqs);
        SEDX_CORE_WARN("MemReq = {} ({})", memReqs.size, memReqs.alignment);
        SEDX_CORE_WARN("Expected size = {}", bufferSize);
#endif

        VkBuffer stagingBuffer;
        VmaAllocation stagingBufferAllocation =
            allocator.AllocateBuffer(bufferCreateInfo, VMA_MEMORY_USAGE_GPU_TO_CPU, stagingBuffer);

        uint32_t mipCount = 1;
        uint32_t mipWidth = m_Specification.Width, mipHeight = m_Specification.Height;

        VkCommandBuffer copyCmd = device->GetCommandBuffer(true);

        VkImageSubresourceRange subresourceRange = {};
        subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        subresourceRange.baseMipLevel = 0;
        subresourceRange.levelCount = mipCount;
        subresourceRange.layerCount = 1;

        InsertImageMemoryBarrier(copyCmd, m_Info.Image, VK_ACCESS_TRANSFER_READ_BIT, 0, m_DescriptorImageInfo.imageLayout,
								 VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, subresourceRange);

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

            vkCmdCopyImageToBuffer(copyCmd, m_Info.Image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, stagingBuffer,1, &bufferCopyRegion);

            uint64_t mipDataSize = mipWidth * mipHeight * sizeof(float) * 4 * 6;
            mipDataOffset += mipDataSize;
            mipWidth /= 2;
            mipHeight /= 2;
        }

        InsertImageMemoryBarrier(copyCmd,
                                        m_Info.Image,
                                        VK_ACCESS_TRANSFER_READ_BIT,
                                        0,
                                        VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                                        m_DescriptorImageInfo.imageLayout,
                                        VK_PIPELINE_STAGE_TRANSFER_BIT,
                                        VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
                                        subresourceRange);

        device->FlushCommandBuffer(copyCmd);

        // Copy data from staging buffer
        uint8_t *srcData = allocator.MapMemory<uint8_t>(stagingBufferAllocation);
        buffer.Allocate(bufferSize);
        memcpy(buffer.Data, srcData, bufferSize);
        allocator.UnmapMemory(stagingBufferAllocation);

        allocator.DestroyBuffer(stagingBuffer, stagingBufferAllocation);
    }
    */

    /*
    ImageView::ImageView(const ImageViewData &specification) : m_Specification(specification)
    {
        Invalidate();
    }
    */

    /*
    ImageView::~ImageView()
    {
        Renderer::SubmitResourceFree([imageView = m_ImageView]() mutable {
            auto device = RenderContext::GetCurrentDevice();
            VkDevice vulkanDevice = device->GetDevice();

            vkDestroyImageView(vulkanDevice, imageView, nullptr);
        });

        m_ImageView = nullptr;
    }
    */

    /*
    void ImageView::Invalidate()
    {
        Ref<ImageView> instance = this;
        Renderer::Submit([instance]() mutable { instance->Invalidate_RenderThread(); });
    }
    */

    /*
    void ImageView::Invalidate_RenderThread()
    {
        auto device = RenderContext::GetCurrentDevice();
        VkDevice vulkanDevice = device->GetDevice();

        Ref<Image2D> vulkanImage = m_Specification.Image.As<Image2D>();
        const auto &imageSpec = vulkanImage->GetSpecification();

        VkImageAspectFlags aspectMask =
			DepthFormat(imageSpec.Format) ? VK_IMAGE_ASPECT_DEPTH_BIT : VK_IMAGE_ASPECT_COLOR_BIT;
        if (imageSpec.Format == VkFormat::DEPTH24STENCIL8)
            aspectMask |= VK_IMAGE_ASPECT_STENCIL_BIT;

        VkFormat vulkanFormat = VkFormat(imageSpec.Format);

        VkImageViewCreateInfo imageViewCreateInfo = {};
        imageViewCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        imageViewCreateInfo.viewType = imageSpec.Layers > 1 ? VK_IMAGE_VIEW_TYPE_2D_ARRAY : VK_IMAGE_VIEW_TYPE_2D;
        imageViewCreateInfo.format = vulkanFormat;
        imageViewCreateInfo.flags = 0;
        imageViewCreateInfo.subresourceRange = {};
        imageViewCreateInfo.subresourceRange.aspectMask = aspectMask;
        imageViewCreateInfo.subresourceRange.baseMipLevel = m_Specification.Mip;
        imageViewCreateInfo.subresourceRange.levelCount = 1;
        imageViewCreateInfo.subresourceRange.baseArrayLayer = 0;
        imageViewCreateInfo.subresourceRange.layerCount = imageSpec.Layers;
        imageViewCreateInfo.image = vulkanImage->GetImageInfo().Image;
        VK_CHECK_RESULT(vkCreateImageView(vulkanDevice, &imageViewCreateInfo, nullptr, &m_ImageView));
        VKUtils::SetDebugUtilsObjectName(vulkanDevice,
                                         VK_OBJECT_TYPE_IMAGE_VIEW,
                                         std::format("{} default image view", m_Specification.debugName),
                                         m_ImageView);

        m_DescriptorImageInfo = vulkanImage->GetDescriptorInfoVulkan();
        m_DescriptorImageInfo.imageView = m_ImageView;
    }
    */

}

/// -------------------------------------------------------
