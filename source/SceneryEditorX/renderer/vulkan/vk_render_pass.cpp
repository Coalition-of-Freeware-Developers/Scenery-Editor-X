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
#include <SceneryEditorX/platform/config/editor_config.hpp>
#include <SceneryEditorX/renderer/buffers/storage_buffer.h>
#include <SceneryEditorX/renderer/vulkan/vk_buffers.h>
#include <SceneryEditorX/renderer/vulkan/vk_render_pass.h>

/// -------------------------------------------------------

namespace SceneryEditorX
{
	/**
	 * @brief Constructs a RenderPass object with the given specification.
	 *
	 * This constructor initializes the RenderPass with the provided RenderSpec,
	 * which must contain a valid Vulkan pipeline reference. The RenderPass is
	 * responsible for managing the Vulkan render pass and associated resources.
	 *
	 * @param spec The RenderSpec structure containing the Vulkan pipeline and other configuration.
	 *
	 * @note The gfxEngine pointer is initialized to nullptr and should be set before use.
	 * @note The renderData member is default-initialized.
	 * @throws Assertion failure if spec.vkPipeline is not valid.
	 */

	/*
	RenderPass::RenderPass(const RenderSpec &spec) : renderData(), renderSpec(spec)
    {
        SEDX_CORE_VERIFY(spec.Pipeline);

        DescriptorSetManagerSpecification dmSpec;
        dmSpec.debugName = spec.debugName;
        dmSpec.shader = spec.Pipeline->GetSpecification().shader.As<Shader>();
        dmSpec.startSet = 1;
        m_DescriptorSetManager = DescriptorSetManager(dmSpec);
    }
    */

	/**
	 * @brief Destructor for the RenderPass class.
	 *
	 * This destructor is responsible for cleaning up the Vulkan render pass resource
	 * associated with this RenderPass instance. If the render pass handle is valid and
	 * the logical device is available, it destroys the Vulkan render pass using vkDestroyRenderPass.
	 * After destruction, the renderPass handle is set to VK_NULL_HANDLE to prevent accidental reuse.
	 *
	 * @note
	 * - The destructor does not explicitly clean up buffer resources such as uniform buffers,
	 *   vertex buffers, or index buffers. These resources should be managed by their respective
	 *   classes (UniformBuffer, VertexBuffer, IndexBuffer), which have their own destructors
	 *   to handle resource cleanup.
	 * - If explicit cleanup of these resources is required, consider storing instances of these
	 *   classes as members of RenderPass and letting their destructors handle cleanup automatically,
	 *   or provide explicit cleanup methods to be called here.
	 */

	/*
	RenderPass::~RenderPass()
	{

        if (renderPass != VK_NULL_HANDLE && RenderContext::Get()->GetLogicDevice())
	    {
            const auto device = RenderContext::Get()->GetLogicDevice();
            vkDestroyRenderPass(device, renderPass, RenderContext::Get()->GetAllocatorCallback() : nullptr);
	        renderPass = VK_NULL_HANDLE;
	    }

	}

    void RenderPass::AddInput(std::string_view name, const Ref<UniformBuffer> &uniformBuffer)
    {
        m_DescriptorSetManager.AddInput(name, uniformBuffer);
    }


    void RenderPass::AddInput(std::string_view name, const Ref<UniformBufferSet> &uniformBufferSet)
    {
        m_DescriptorSetManager.AddInput(name, uniformBufferSet);
    }


	void RenderPass::AddInput(std::string_view name, const Ref<StorageBufferSet> &storageBufferSet)
    {
        m_DescriptorSetManager.AddInput(name, storageBufferSet);
    }

    void RenderPass::AddInput(std::string_view name, const Ref<StorageBuffer> &storageBuffer)
    {
        m_DescriptorSetManager.AddInput(name, storageBuffer);
    }
    */

    /*
    void RenderPass::AddInput(std::string_view name, const Ref<TextureAsset> &texture)
    {
        //m_DescriptorSetManager.AddInput(name, texture);
    }
    */

    /*
    Ref<Image2D> RenderPass::GetOutput(uint32_t index)
    {
        return {};
    }
    */

    /*
    Ref<Image2D> RenderPass::GetDepthOutput()
    {
        return {};
    }
    */

    /*
    uint32_t RenderPass::GetFirstSetIndex() const
    {
        return 0;
    }
    */

    Ref<Framebuffer> RenderPass::GetTargetFramebuffer() const
    {
        return renderSpec.Pipeline->GetSpecification().dstFramebuffer;
    }

    Ref<Pipeline> RenderPass::GetPipeline() const
    {
        return renderSpec.Pipeline;
    }

    /*
    bool RenderPass::Validate()
    {
        return m_DescriptorSetManager.Validate();
    }

    void RenderPass::Bake()
    {
        m_DescriptorSetManager.Bake();
    }

    void RenderPass::Prepare()
    {
    }

    bool RenderPass::HasDescriptorSets() const
    {
        return m_DescriptorSetManager.HasDescriptorSets();
    }

    const std::vector<VkDescriptorSet> & RenderPass::GetDescriptorSets(uint32_t frameIndex) const
    {
        SEDX_CORE_ASSERT(!m_DescriptorSetManager.m_DescriptorSets.empty());
        if (frameIndex > 0 && m_DescriptorSetManager.m_DescriptorSets.size() == 1)
            return m_DescriptorSetManager.m_DescriptorSets[0]; /// Frame index is irrelevant for this type of render pass
        return m_DescriptorSetManager.m_DescriptorSets[frameIndex];
    }

    bool RenderPass::IsInputValid(std::string_view name) const
    {
        std::string nameStr(name);
        return m_DescriptorSetManager.inputDeclarations.contains(nameStr);
    }

	const RenderPassInputDeclaration *RenderPass::GetInputDeclaration(std::string_view name)
    {
        std::string nameStr(name);
        if (!m_DescriptorSetManager.inputDeclarations.contains(nameStr))
            return nullptr;
        const RenderPassInputDeclaration &decl = m_DescriptorSetManager.inputDeclarations.at(nameStr);
        return &decl;
    }
    */

    void RenderPass::CreateDescriptorSets() const
    {
    }

    bool RenderPass::IsInvalidated(uint32_t set, uint32_t binding) const
    {
        return false;
    }

    VkCommandBuffer RenderPass::BeginSingleTimeCommands() const
    {
        return nullptr;
    }

    /**
	 * @brief Creates the Vulkan render pass for the graphics pipeline.
	 *
	 * This method sets up the color and depth attachments, subpass, and subpass dependency
	 * required for rendering. It configures the attachment descriptions for both color and depth,
	 * specifying their formats, load/store operations, and layouts. The subpass is set up to use
	 * these attachments, and a dependency is defined to ensure proper synchronization between
	 * subpasses and external operations.
	 *
	 * The method then fills out a VkRenderPassCreateInfo structure and creates the Vulkan render pass
	 * using vkCreateRenderPass. If creation fails, an error is logged.
	 *
	 * @note The color and depth formats are retrieved from the swap chain.
	 * @note The created render pass handle is stored in the renderPass member.
	 * @throws Logs an error if vkCreateRenderPass fails.
	 */
	/*
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
	
	    /// -------------------------------------------------------
	
	    VkAttachmentReference colorAttachmentRef{};
	    colorAttachmentRef.attachment = 0;
	    colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
	
	    VkAttachmentReference depthAttachmentRef{};
	    depthAttachmentRef.attachment = 1;
	    depthAttachmentRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
	
	    /// -------------------------------------------------------
	
	    VkSubpassDescription subpass{};
	    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
	    subpass.colorAttachmentCount = 1;
	    subpass.pColorAttachments = &colorAttachmentRef;
	    subpass.pDepthStencilAttachment = &depthAttachmentRef;
	
	    /// -------------------------------------------------------
	
	    VkSubpassDependency dependency{};
	    dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
	    dependency.dstSubpass = 0;
	    dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT |VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
	    dependency.srcAccessMask = 0;
	    dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
	    dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
	
	    /// -------------------------------------------------------
	
	    std::array<VkAttachmentDescription, 2> attachments = {colorAttachment, depthAttachment};
        const auto device = RenderContext::Get()->GetLogicDevice()->GetDevice();
		
	    VkRenderPassCreateInfo renderPassInfo{};
	    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
	    renderPassInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
	    renderPassInfo.pAttachments = attachments.data();
	    renderPassInfo.subpassCount = 1;
	    renderPassInfo.pSubpasses = &subpass;
	    renderPassInfo.dependencyCount = 1;
	    renderPassInfo.pDependencies = &dependency;
	
	    if (vkCreateRenderPass(device, &renderPassInfo, RenderContext::Get()->GetAllocatorCallback(), &renderPass) != VK_SUCCESS)
	        SEDX_CORE_ERROR("Failed to create render pass!");
	}
	*/

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

    /**
     * @brief Generates mipmaps for a Vulkan image.
     *
     * This method generates mipmaps for the specified Vulkan image by performing a series of
     * blit operations, transitioning each mip level to the appropriate layout and filtering
     * the image data down to lower resolutions. It first checks if the image format supports
     * linear blitting, then iteratively transitions each mip level, performs the blit, and
     * transitions the layout for shader access.
     *
     * @param image        The Vulkan image to generate mipmaps for.
     * @param imageFormat  The format of the image (VkFormat).
     * @param texWidth     The width of the image in pixels.
     * @param texHeight    The height of the image in pixels.
     * @param mipLevels    The total number of mipmap levels to generate.
     *
     * @note
     * - The image must have been created with VK_IMAGE_USAGE_TRANSFER_SRC_BIT and VK_IMAGE_USAGE_TRANSFER_DST_BIT.
     * - The command buffer used for the operation must support graphics queue operations.
     * - The method assumes the image is already in VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL for the base level.
     * - The method ends the command buffer and submits it for execution after mipmap generation.
     * - If the image format does not support linear blitting, an error is logged.
     */
    /*
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

            vkCmdPipelineBarrier(cmdBuffer->GetActiveCmdBuffer(),
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

            vkCmdBlitImage(cmdBuffer->GetActiveCmdBuffer(),
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

            vkCmdPipelineBarrier(cmdBuffer->GetActiveCmdBuffer(),
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

        vkCmdPipelineBarrier(cmdBuffer->GetActiveCmdBuffer(),
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
    */

	/**
	 * @brief Creates a Vulkan image and allocates memory for it.
	 *
	 * This method sets up a 2D Vulkan image with the specified parameters, including width, height,
	 * mip levels, sample count, format, tiling, usage, and memory properties. It then allocates
	 * device memory for the image and binds the memory to the image.
	 *
	 * @param width         The width of the image in pixels.
	 * @param height        The height of the image in pixels.
	 * @param mipLevels     The number of mipmap levels for the image.
	 * @param numSamples    The number of samples per texel (for multisampling).
	 * @param format        The format of the image (VkFormat).
	 * @param tiling        The tiling arrangement for the image (VkImageTiling).
	 * @param usage         Usage flags specifying how the image will be used (VkImageUsageFlags).
	 * @param properties    Memory property flags for the image allocation (VkMemoryPropertyFlags).
	 * @param image         Reference to a VkImage handle to receive the created image.
	 * @param imageMemory   Reference to a VkDeviceMemory handle to receive the allocated memory.
	 *
	 * @throws Logs an error if image creation or memory allocation fails.
	 *
	 * @note The created image is initialized with VK_IMAGE_LAYOUT_UNDEFINED.
	 * @note The image is created with exclusive sharing mode.
	 */
	/*
	void RenderPass::CreateImage(uint32_t width, uint32_t height, uint32_t mipLevels,
	                             VkSampleCountFlagBits numSamples, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage,
	                             VkMemoryPropertyFlags properties, VkImage &image, VkDeviceMemory &imageMemory) const
	{
        const auto device = RenderContext::Get()->GetLogicDevice()->GetDevice();
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
	
	    if (vkCreateImage(device, &imageInfo, RenderContext::Get()->GetAllocatorCallback(), &image) != VK_SUCCESS)
	        SEDX_CORE_ERROR_TAG("Graphics Engine", "Failed to create image!");
	
	    VkMemoryRequirements memRequirements;
        vkGetImageMemoryRequirements(device, image, &memRequirements);
	
	    VkMemoryAllocateInfo allocInfo{};
	    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	    allocInfo.allocationSize = memRequirements.size;
	    allocInfo.memoryTypeIndex = RenderContext::GetCurrentDevice()->FindMemoryType(memRequirements.memoryTypeBits, properties);
	
	    if (vkAllocateMemory(device, &allocInfo, RenderContext::Get()->GetAllocatorCallback(), &imageMemory) != VK_SUCCESS)
	        SEDX_CORE_ERROR_TAG("Graphics Engine", "Failed to allocate image memory!");
	
	    vkBindImageMemory(device, image, imageMemory, 0);
	}
	*/

} // namespace SceneryEditorX

/// -------------------------------------------------------
