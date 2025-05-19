/**
* -------------------------------------------------------
* Scenery Editor X
* -------------------------------------------------------
* Copyright (c) 2025 Thomas Ray 
* Copyright (c) 2025 Coalition of Freeware Developers
* -------------------------------------------------------
* vk_render_pass.cpp
* -------------------------------------------------------
* Created: 16/4/2025
* -------------------------------------------------------
*/
#include <SceneryEditorX/platform/editor_config.hpp>
#include <SceneryEditorX/vulkan/vk_buffers.h>
#include <SceneryEditorX/vulkan/vk_render_pass.h>

// -------------------------------------------------------

namespace SceneryEditorX
{
	RenderPass::RenderPass(const RenderSpec &spec)
	{
        SEDX_CORE_VERIFY(spec.vkPipeline);
	}

	RenderPass::~RenderPass()
	{
        if (renderPass != VK_NULL_HANDLE && gfxEngine->GetLogicDevice()->GetDevice())
	    {
	        vkDestroyRenderPass(gfxEngine->GetLogicDevice()->GetDevice(), renderPass, 
	                           (gfxEngine != nullptr) ? gfxEngine->GetAllocatorCallback() : nullptr);
	        renderPass = VK_NULL_HANDLE;
	    }
	    
	    // The buffer resources should be managed by their respective classes
	    // UniformBuffer, VertexBuffer, and IndexBuffer each have their own destructors
	    // that handle resource cleanup. If you need to explicitly clean up these resources,
	    // you should store instances of these classes as members of RenderPass and let
	    // their destructors handle the cleanup automatically, or provide explicit clean-up
	    // methods that you can call here.
	}

	void RenderPass::CreateRenderPass()
	{
        VkAttachmentDescription colorAttachment{};
        colorAttachment.flags = 0;
        colorAttachment.format = vkSwapChain->GetColorFormat();
        colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
        colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

        VkAttachmentDescription depthAttachment{};
        depthAttachment.format = vkSwapChain->GetDepthFormat();
        depthAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
        depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        depthAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        depthAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        depthAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        depthAttachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

        // -------------------------------------------------------

        VkAttachmentReference colorAttachmentRef{};
        colorAttachmentRef.attachment = 0;
        colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

        VkAttachmentReference depthAttachmentRef{};
        depthAttachmentRef.attachment = 1;
        depthAttachmentRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

        // -------------------------------------------------------

        VkSubpassDescription subpass{};
        subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
        subpass.colorAttachmentCount = 1;
        subpass.pColorAttachments = &colorAttachmentRef;
        subpass.pDepthStencilAttachment = &depthAttachmentRef;

        // -------------------------------------------------------

        VkSubpassDependency dependency{};
        dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
        dependency.dstSubpass = 0;
        dependency.srcStageMask =
            VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT |
			VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
        dependency.srcAccessMask = 0;
        dependency.dstStageMask =
            VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT |
			VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
        dependency.dstAccessMask =
			VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT |
			VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

        // -------------------------------------------------------

        std::array<VkAttachmentDescription, 2> attachments = {colorAttachment, depthAttachment};

        VkRenderPassCreateInfo renderPassInfo{};
        renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
        renderPassInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
        renderPassInfo.pAttachments = attachments.data();
        renderPassInfo.subpassCount = 1;
        renderPassInfo.pSubpasses = &subpass;
        renderPassInfo.dependencyCount = 1;
        renderPassInfo.pDependencies = &dependency;

        if (vkCreateRenderPass(gfxEngine->GetLogicDevice()->GetDevice(), &renderPassInfo, gfxEngine->GetAllocatorCallback(), &renderPass) != VK_SUCCESS)
            SEDX_CORE_ERROR("Failed to create render pass!");
    }

	/*
	void RenderPass::CreateDescriptorSets() const
    {
        std::vector<VkDescriptorSetLayout> layouts(RenderData::framesInFlight, descriptors->descriptorSetLayout);

        // -------------------------------------------------------

        VkDescriptorSetAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
        allocInfo.descriptorPool = descriptors->descriptorPool;
        allocInfo.descriptorSetCount = RenderData::framesInFlight;
        allocInfo.pSetLayouts = layouts.data();

        descriptors->descriptorSets.resize(RenderData::framesInFlight);
        if (vkAllocateDescriptorSets(gfxEngine->GetLogicDevice()->GetDevice(), &allocInfo, descriptors->descriptorSets.data()) != VK_SUCCESS)
            SEDX_CORE_ERROR_TAG("Graphics Engine", "Failed to allocate descriptor sets!");

        // -------------------------------------------------------

        for (size_t i = 0; i < RenderData::framesInFlight; i++)
        {
            VkDescriptorBufferInfo bufferInfo{};
            bufferInfo.buffer = uniformBuffer->uniformBuffers[i];
            bufferInfo.offset = 0;
            bufferInfo.range = sizeof(UniformBuffer::UBO);

            // Get texture resources from appropriate source - in this case the SwapChain
            VkDescriptorImageInfo imageInfo{};
            imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
            imageInfo.imageView = vkSwapChain->textureImageView;
            imageInfo.sampler = vkSwapChain->textureSampler;

            // -------------------------------------------------------

            std::array<VkWriteDescriptorSet, 2> descriptorWrites{};

            // -------------------------------------------------------

            descriptorWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            descriptorWrites[0].dstSet = descriptors->descriptorSets[i];
            descriptorWrites[0].dstBinding = 0;
            descriptorWrites[0].dstArrayElement = 0;
            descriptorWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
            descriptorWrites[0].descriptorCount = 1;
            descriptorWrites[0].pBufferInfo = &bufferInfo;

            // -------------------------------------------------------

            descriptorWrites[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            descriptorWrites[1].dstSet = descriptors->descriptorSets[i];
            descriptorWrites[1].dstBinding = 1;
            descriptorWrites[1].dstArrayElement = 0;
            descriptorWrites[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
            descriptorWrites[1].descriptorCount = 1;
            descriptorWrites[1].pImageInfo = &imageInfo;

            // -------------------------------------------------------

            vkUpdateDescriptorSets(gfxEngine->GetLogicDevice()->GetDevice(), descriptorWrites.size(), descriptorWrites.data(), 0, nullptr);
        }

        // -------------------------------------------------------
    }
    */

	void RenderPass::GenerateMipmaps(VkImage image, VkFormat imageFormat, int32_t texWidth, int32_t texHeight, uint32_t mipLevels) const
    {
        Ref<VulkanPhysicalDevice> physicalDevice;
        /// Check if image format supports linear blitting
        VkFormatProperties formatProperties;
        vkGetPhysicalDeviceFormatProperties(physicalDevice->GetGPUDevices(), imageFormat, &formatProperties);

        if (!(formatProperties.optimalTilingFeatures & VK_FORMAT_FEATURE_SAMPLED_IMAGE_FILTER_LINEAR_BIT))
            SEDX_CORE_ERROR_TAG("Texture", "Texture image format does not support linear blitting!");

        cmdBuffer->Begin(Queue::Graphics);

        VkImageMemoryBarrier barrier{};
        barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        barrier.image = image;
        barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        barrier.subresourceRange.baseArrayLayer = 0;
        barrier.subresourceRange.layerCount = 1;
        barrier.subresourceRange.levelCount = 1;

        int32_t mipWidth = texWidth;
        int32_t mipHeight = texHeight;

        for (uint32_t i = 1; i < mipLevels; i++)
        {
            barrier.subresourceRange.baseMipLevel = i - 1;
            barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
            barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
            barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
            barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;

            vkCmdPipelineBarrier(cmdBuffer->GetActiveCommandBuffer(),
                                 VK_PIPELINE_STAGE_TRANSFER_BIT,
                                 VK_PIPELINE_STAGE_TRANSFER_BIT,
                                 0,
                                 0,
                                 nullptr,
                                 0,
                                 nullptr,
                                 1,
                                 &barrier);

            VkImageBlit blit{};
            blit.srcOffsets[0] = {.x = 0,.y = 0,.z = 0};
            blit.srcOffsets[1] = {.x = mipWidth,.y = mipHeight,.z = 1};
            blit.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            blit.srcSubresource.mipLevel = i - 1;
            blit.srcSubresource.baseArrayLayer = 0;
            blit.srcSubresource.layerCount = 1;
            blit.dstOffsets[0] = {.x = 0,.y = 0,.z = 0};
            blit.dstOffsets[1] = {.x = mipWidth > 1 ? mipWidth / 2 : 1,.y = mipHeight > 1 ? mipHeight / 2 : 1,.z = 1};
            blit.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            blit.dstSubresource.mipLevel = i;
            blit.dstSubresource.baseArrayLayer = 0;
            blit.dstSubresource.layerCount = 1;

            vkCmdBlitImage(cmdBuffer->GetActiveCommandBuffer(),
                           image,
                           VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                           image,
                           VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                           1,
                           &blit,
                           VK_FILTER_LINEAR);

            barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
            barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
            barrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
            barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

            vkCmdPipelineBarrier(cmdBuffer->GetActiveCommandBuffer(),
                                 VK_PIPELINE_STAGE_TRANSFER_BIT,
                                 VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
                                 0,
                                 0,
                                 nullptr,
                                 0,
                                 nullptr,
                                 1,
                                 &barrier);

            if (mipWidth > 1)
                mipWidth /= 2;
            if (mipHeight > 1)
                mipHeight /= 2;
        }

        barrier.subresourceRange.baseMipLevel = mipLevels - 1;
        barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

        vkCmdPipelineBarrier(cmdBuffer->GetActiveCommandBuffer(),
                             VK_PIPELINE_STAGE_TRANSFER_BIT,
                             VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
                             0,
                             0,
                             nullptr,
                             0,
                             nullptr,
                             1,
                             &barrier);

        //EndSingleTimeCommands(cmdBuffer->GetActiveCommandBuffer());

        /// Create a submit info structure
        VkSubmitInfo submitInfo{};
        submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        /// No wait semaphores needed for this operation
        submitInfo.waitSemaphoreCount = 0;
        submitInfo.pWaitSemaphores = nullptr;
        submitInfo.pWaitDstStageMask = nullptr;

        cmdBuffer->End(submitInfo);
    }

    void RenderPass::CreateImage(uint32_t width, uint32_t height, uint32_t mipLevels, VkSampleCountFlagBits numSamples, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage,
                                     VkMemoryPropertyFlags properties, VkImage &image, VkDeviceMemory &imageMemory) const
    {
        VkImageCreateInfo imageInfo{};
        imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
        imageInfo.imageType = VK_IMAGE_TYPE_2D;
        imageInfo.extent.width = width;
        imageInfo.extent.height = height;
        imageInfo.extent.depth = 1;
        imageInfo.mipLevels = mipLevels;
        imageInfo.arrayLayers = 1;
        imageInfo.format = format;
        imageInfo.tiling = tiling;
        imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        imageInfo.usage = usage;
        imageInfo.samples = numSamples;
        imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

        if (vkCreateImage(gfxEngine->GetLogicDevice()->GetDevice(), &imageInfo, gfxEngine->GetAllocatorCallback(), &image) != VK_SUCCESS)
            SEDX_CORE_ERROR_TAG("Graphics Engine", "Failed to create image!");

        VkMemoryRequirements memRequirements;
        vkGetImageMemoryRequirements(gfxEngine->GetLogicDevice()->GetDevice(), image, &memRequirements);

        VkMemoryAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        allocInfo.allocationSize = memRequirements.size;
        allocInfo.memoryTypeIndex = GraphicsEngine::GetCurrentDevice()->FindMemoryType(memRequirements.memoryTypeBits, properties);

        if (vkAllocateMemory(gfxEngine->GetLogicDevice()->GetDevice(), &allocInfo, gfxEngine->GetAllocatorCallback(), &imageMemory) != VK_SUCCESS)
            SEDX_CORE_ERROR_TAG("Graphics Engine", "Failed to allocate image memory!");

        vkBindImageMemory(gfxEngine->GetLogicDevice()->GetDevice(), image, imageMemory, 0);
    }

} // namespace SceneryEditorX

// -------------------------------------------------------
