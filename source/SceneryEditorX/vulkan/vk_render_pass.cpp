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
#include <SceneryEditorX/ui/ui.h>
#include <SceneryEditorX/vulkan/vk_render_pass.h>

// -------------------------------------------------------

namespace SceneryEditorX
{
	RenderPass::~RenderPass()
	{
        for (size_t i = 0; i < RenderData::framesInFlight; i++)
        {
            vkDestroyBuffer(vkDevice->GetDevice(), uniformBuffers[i], allocator);
            vkFreeMemory(vkDevice->GetDevice(), uniformBuffersMemory[i], allocator);
        }

        vkDestroyBuffer(vkDevice->GetDevice(), vertexBuffer, allocator);
        vkFreeMemory(vkDevice->GetDevice(), vertexBufferMemory, allocator);

	    if (renderPass != VK_NULL_HANDLE)
	    {
	        vkDestroyRenderPass(vkDevice->GetDevice(), renderPass, allocator);
	        renderPass = VK_NULL_HANDLE;
	    }
	}

	void RenderPass::CreateRenderPass()
	{
        VkAttachmentDescription colorAttachment{};
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

        if (vkCreateRenderPass(vkDevice->GetDevice(), &renderPassInfo, allocator, &renderPass) != VK_SUCCESS)
        {
            SEDX_CORE_ERROR("Failed to create render pass!");
            ErrMsg("failed to create render pass!");
        }
	}

    void GraphicsEngine::RenderFrame()
    {
        UI::GUI guiInstance;

        vkWaitForFences(vkDevice->GetDevice(), 1, &inFlightFences[renderData.currentFrame], VK_TRUE, UINT64_MAX);

        // -------------------------------------------------------

        uint32_t imageIndex;
        VkResult result = vkAcquireNextImageKHR(vkDevice->GetDevice(),
                                                vkSwapChain->swapChain,
                                                UINT64_MAX,
                                                imageAvailableSemaphores[renderData.currentFrame],
                                                VK_NULL_HANDLE,
                                                &imageIndex);
        if (result == VK_ERROR_OUT_OF_DATE_KHR)
        {
            SEDX_CORE_INFO("VK_ERROR_OUT_OF_DATE_KHR returned from vkAcquireNextImageKHR - recreating swap chain");
            vkSwapChain->Create(viewportData.width, viewportData.height, renderData.VSync);
            return;
        }
        else if (result == VK_SUBOPTIMAL_KHR)
        {
            SEDX_CORE_INFO("VK_SUBOPTIMAL_KHR returned from vkAcquireNextImageKHR - continuing with render");
        }
        else if (result != VK_SUCCESS)
        {
            SEDX_CORE_ERROR_TAG("Graphics Engine", "Failed to acquire swap chain image: {}", ToString(result));
        }

        /// Check if a Resize has been requested through the Window class
        if (Window::GetFramebufferResized())
        {
            SEDX_CORE_INFO("Framebuffer Resize detected from Window class");
            vkSwapChain->Create(viewportData.width, viewportData.height, renderData.VSync);
            return;
        }

        RenderPass::UpdateUniformBuffer(renderData.currentFrame);

        vkResetFences(vkDevice->GetDevice(), 1, &inFlightFences[renderData.currentFrame]);

        vkResetCommandBuffer(commandBuffers[renderData.currentFrame], /*VkCommandBufferResetFlagBits*/ 0);
        RecordCommandBuffer(commandBuffers[renderData.currentFrame], imageIndex);

        // -------------------------------------------------------

        VkSubmitInfo submitInfo{};
        submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

        VkSemaphore waitSemaphores[] = {imageAvailableSemaphores[renderData.currentFrame]};
        VkPipelineStageFlags waitStages[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
        submitInfo.waitSemaphoreCount = 1;
        submitInfo.pWaitSemaphores = waitSemaphores;
        submitInfo.pWaitDstStageMask = waitStages;
        submitInfo.commandBufferCount = 1;
        submitInfo.pCommandBuffers = &commandBuffers[renderData.currentFrame];

        VkSemaphore signalSemaphores[] = {renderFinishedSemaphores[renderData.currentFrame]};
        submitInfo.signalSemaphoreCount = 1;
        submitInfo.pSignalSemaphores = signalSemaphores;

        if (vkQueueSubmit(graphicsQueue, 1, &submitInfo, inFlightFences[renderData.currentFrame]) != VK_SUCCESS)
        {
            SEDX_CORE_ERROR_TAG("Graphics Engine", "Failed to submit draw command buffer");
            throw std::runtime_error("failed to submit draw command buffer!");
        }

        // -------------------------------------------------------

        VkPresentInfoKHR presentInfo{};
        presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
        presentInfo.waitSemaphoreCount = 1;
        presentInfo.pWaitSemaphores = signalSemaphores;

        VkSwapchainKHR swapChains[] = {vkSwapChain->swapChain};
        presentInfo.swapchainCount = 1;
        presentInfo.pSwapchains = swapChains;
        presentInfo.pImageIndices = &imageIndex;

        // -------------------------------------------------------

        result = vkQueuePresentKHR(presentQueue, &presentInfo);

        if (result == VK_ERROR_OUT_OF_DATE_KHR)
        {
            SEDX_CORE_INFO("VK_ERROR_OUT_OF_DATE_KHR returned from vkQueuePresentKHR - recreating swap chain");
            renderData.framebufferResized = false;
            vkSwapChain->Create(viewportData.width, viewportData.height, renderData.VSync);
        }
        else if (result == VK_SUBOPTIMAL_KHR)
        {
            SEDX_CORE_INFO("VK_SUBOPTIMAL_KHR returned from vkQueuePresentKHR - recreating swap chain");
            renderData.framebufferResized = false;
            vkSwapChain->Create(viewportData.width, viewportData.height, renderData.VSync);
        }
        else if (Window::GetFramebufferResized())
        {
            SEDX_CORE_INFO("Window framebuffer Resize flag set - recreating swap chain");
            renderData.framebufferResized = false;
            vkSwapChain->Create(viewportData.width, viewportData.height, renderData.VSync);
        }
        else if (result != VK_SUCCESS)
        {
            SEDX_CORE_ERROR_TAG("Graphics Engine", "Failed to present swap chain image: {}", ToString(result));
        }

        renderData.currentFrame = (renderData.currentFrame + 1) % RenderData::framesInFlight;
    }

	void RenderPass::CreateDescriptorSets()
    {
        std::vector<VkDescriptorSetLayout> layouts(RenderData::framesInFlight, descriptorSetLayout);

        // -------------------------------------------------------

        VkDescriptorSetAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
        allocInfo.descriptorPool = descriptorPool;
        allocInfo.descriptorSetCount = RenderData::framesInFlight;
        allocInfo.pSetLayouts = layouts.data();

        descriptorSets.resize(RenderData::framesInFlight);
        if (vkAllocateDescriptorSets(vkDevice->GetDevice(), &allocInfo, descriptorSets.data()) != VK_SUCCESS)
        {
            SEDX_CORE_ERROR_TAG("Graphics Engine", "Failed to allocate descriptor sets!");
        }

        // -------------------------------------------------------

        for (size_t i = 0; i < RenderData::framesInFlight; i++)
        {
            VkDescriptorBufferInfo bufferInfo{};
            bufferInfo.buffer = uniformBuffers[i];
            bufferInfo.offset = 0;
            bufferInfo.range = sizeof(UniformBuffer);

            VkDescriptorImageInfo imageInfo{};
            imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
            imageInfo.imageView = textureImageView;
            imageInfo.sampler = textureSampler;

            // -------------------------------------------------------

            std::array<VkWriteDescriptorSet, 2> descriptorWrites{};

            // -------------------------------------------------------

            descriptorWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            descriptorWrites[0].dstSet = descriptorSets[i];
            descriptorWrites[0].dstBinding = 0;
            descriptorWrites[0].dstArrayElement = 0;
            descriptorWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
            descriptorWrites[0].descriptorCount = 1;
            descriptorWrites[0].pBufferInfo = &bufferInfo;

            // -------------------------------------------------------

            descriptorWrites[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            descriptorWrites[1].dstSet = descriptorSets[i];
            descriptorWrites[1].dstBinding = 1;
            descriptorWrites[1].dstArrayElement = 0;
            descriptorWrites[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
            descriptorWrites[1].descriptorCount = 1;
            descriptorWrites[1].pImageInfo = &imageInfo;

            // -------------------------------------------------------

            vkUpdateDescriptorSets(vkDevice->GetDevice(), descriptorWrites.size(), descriptorWrites.data(), 0, nullptr);
        }

        // -------------------------------------------------------
    }

    void RenderPass::CreateImage(uint32_t width, uint32_t height, uint32_t mipLevels, VkSampleCountFlagBits numSamples, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage,
                                     VkMemoryPropertyFlags properties, VkImage &image, VkDeviceMemory &imageMemory)
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

        if (vkCreateImage(vkDevice->GetDevice(), &imageInfo, allocator, &image) != VK_SUCCESS)
        {
            SEDX_CORE_ERROR_TAG("Graphics Engine", "Failed to create image!");
        }

        VkMemoryRequirements memRequirements;
        vkGetImageMemoryRequirements(vkDevice->GetDevice(), image, &memRequirements);

        VkMemoryAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        allocInfo.allocationSize = memRequirements.size;
        allocInfo.memoryTypeIndex = vkDevice->FindMemoryType(memRequirements.memoryTypeBits, properties);

        if (vkAllocateMemory(vkDevice->GetDevice(), &allocInfo, allocator, &imageMemory) != VK_SUCCESS)
        {
            SEDX_CORE_ERROR_TAG("Graphics Engine", "Failed to allocate image memory!");
        }

        vkBindImageMemory(vkDevice->GetDevice(), image, imageMemory, 0);
    }

} // namespace SceneryEditorX

// -------------------------------------------------------
