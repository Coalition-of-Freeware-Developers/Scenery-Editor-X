/**
* -------------------------------------------------------
* Scenery Editor X
* -------------------------------------------------------
* Copyright (c) 2025 Thomas Ray
* Copyright (c) 2025 Coalition of Freeware Developers
* -------------------------------------------------------
* vk_buffers.cpp
* -------------------------------------------------------
* Created: 5/5/2025
* -------------------------------------------------------
*/
#include "vk_buffers.h"
#include "SceneryEditorX/renderer/image_data.h"
#include "SceneryEditorX/renderer/render_context.h"
#include "SceneryEditorX/renderer/bindless_descriptor_manager.h"

/// ----------------------------------------------------------

namespace SceneryEditorX
{
    /*
    VkCommandBuffer BeginCommands()
    {
        const VkDevice vkDevice = GraphicsEngine::GetCurrentDevice()->GetDevice();
        const VkCommandPool cmdPool = GraphicsEngine::Get()->GetCommandBuffer()->GetCommandPool()->GetComputeCmdPool();

        VkCommandBufferAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        allocInfo.commandPool = cmdPool;
        allocInfo.commandBufferCount = 1;

        VkCommandBuffer commandBuffer;
        vkAllocateCommandBuffers(vkDevice, &allocInfo, &commandBuffer);

        VkCommandBufferBeginInfo beginInfo{};
        beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

        vkBeginCommandBuffer(commandBuffer, &beginInfo);

        return commandBuffer;
    }*/

    /*
    static void EndCommands(VkCommandBuffer commandBuffer)
    {
        vkEndCommandBuffer(commandBuffer);

        const VkDevice vkDevice = GraphicsEngine::GetCurrentDevice()->GetDevice();
        const VkQueue graphicsQueue = GraphicsEngine::GetCurrentDevice()->GetGraphicsQueue();
        const VkCommandPool cmdPool = GraphicsEngine::Get()->GetCommandBuffer()->GetCommandPool()->GetComputeCmdPool();

        VkSubmitInfo submitInfo{};
        submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        submitInfo.commandBufferCount = 1;
        submitInfo.pCommandBuffers = &commandBuffer;

        vkQueueSubmit(graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE);
        vkQueueWaitIdle(graphicsQueue);

        vkFreeCommandBuffers(vkDevice, cmdPool, 1, &commandBuffer);
    }*/

	/**
	 * @brief Retrieves the unique resource ID of the buffer.
	 *
	 * This method returns the unique identifier associated with the underlying buffer resource.
	 * It asserts that the resource ID is valid (not equal to -1) before returning the value.
	 * The resource ID is used for tracking and referencing the buffer within the engine's
	 * resource management and bindless descriptor systems.
	 *
	 * @return uint32_t The unique resource ID of the buffer.
	 * @throws Assertion failure if the resource ID is invalid.
	 */
	uint32_t Buffer::ID() const
	{
	    SEDX_ASSERT(resource->resourceID != -1, "Invalid Buffer Resource ID!");
	    return static_cast<uint32_t>(resource->resourceID);
	}

	/**
	 * @brief Creates a Vulkan buffer with specified parameters
	 *
	 * This function handles the creation of a Vulkan buffer with appropriate memory allocation using VMA.
	 * It automatically applies usage flags based on the buffer's intended purpose and handles memory
	 * alignment requirements for different buffer types.
	 *
	 * The function performs the following:
	 * 1. Adjusts usage flags based on the provided buffer usage type
	 * 2. Adds transfer destination flags for vertex and index buffers
	 * 3. Handles special requirements for storage buffers and acceleration structures
	 * 4. Creates the buffer with VMA memory allocation
	 * 5. Sets up descriptor updates for storage buffers
	 *
	 * @param size   Size of the buffer in bytes
	 * @param usage  Flags specifying how the buffer will be used
	 * @param memory Memory property flags for the buffer allocation
	 * @param name   Optional name for the buffer (for debugging purposes)
	 *
	 * @return A Buffer structure containing the created buffer and its metadata
	 */
    Buffer CreateBuffer(uint64_t size, BufferUsageFlags usage, MemoryFlags memory, const std::string &name)
	{
        const auto device = RenderContext::Get()->GetLogicDevice();
        /// Get the allocator from the current device
        const VmaAllocator vmaAllocator = device->GetMemoryAllocator();

	    /// ---------------------------------------------------------

	    /// Add transfer destination flag for vertex buffers
	    if (usage & BufferUsage::Vertex)
	        usage |= BufferUsage::TransferDst;

	    /// Add transfer destination flag for index buffers
	    if (usage & BufferUsage::Index)
	        usage |= BufferUsage::TransferDst;

	    /// Handle storage buffers - add address flag and align size
	    if (usage & BufferUsage::Storage)
	    {
	        usage |= BufferUsage::Address;
	        /// Align storage buffer size to minimum required alignment
            size += size % device->GetPhysicalDevice()->GetDeviceProperties().limits.minStorageBufferOffsetAlignment;
        }

	    /// Handle acceleration structure input buffers
	    if (usage & BufferUsage::AccelerationStructureInput)
	    {
	        usage |= BufferUsage::Address;
	        usage |= BufferUsage::TransferDst;
	    }

	    /// Handle acceleration structure buffers
	    if (usage & BufferUsage::AccelerationStructure)
	        usage |= BufferUsage::Address;

	    /// Create buffer resource
	    const Ref<BufferResource> resource = CreateRef<BufferResource>();

	    /// Configure buffer creation info
	    VkBufferCreateInfo bufferInfo{};
	    bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	    bufferInfo.size = size;
	    bufferInfo.usage = static_cast<VkBufferUsageFlagBits>(usage);
	    bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

	    /// Configure memory allocation info
	    VmaAllocationCreateInfo allocInfo = {};
	    allocInfo.usage = VMA_MEMORY_USAGE_AUTO;
	    /// Enable memory mapping for CPU-accessible buffers
	    if (memory & CPU)
	        allocInfo.flags = VMA_ALLOCATION_CREATE_MAPPED_BIT | VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT;

	    /// Create the buffer with VMA
	    SEDX_ASSERT(vmaCreateBuffer(vmaAllocator, &bufferInfo, &allocInfo, &resource->buffer, &resource->allocation, nullptr));

	    /// Create and populate the buffer wrapper
	    Buffer buffer;
	    buffer.resource = resource;
	    buffer.size = size;
	    buffer.usage = usage;
	    buffer.memory = memory;

			// Register storage buffer into global bindless manager if available
			if (usage & BufferUsage::Storage)
			{
				// Register buffer in bindless descriptor manager (legacy RID fallback removed)
				uint32_t bIndex = BindlessDescriptorManager::RegisterStorageBuffer(resource->buffer, size, 0);
				resource->resourceID = bIndex;
			}

	    return buffer;
	}

    /**
     * @brief Maps a Vulkan buffer to CPU-accessible memory
     *
     * @param buffer The buffer resource to map to CPU-accessible memory
     * @return void* Pointer to the mapped memory region
     */
    void *MapBuffer(const Buffer &buffer)  // NOLINT(misc-use-internal-linkage)
    {
        const auto device = RenderContext::Get()->GetLogicDevice();
        SEDX_ASSERT(buffer.memory & MemoryType::CPU, "Buffer not accessible to the CPU.");
        void *data = nullptr; ///< Initialize data
        vmaMapMemory(device->GetMemoryAllocator(), buffer.resource->allocation, &data);
        return data;
	}

    /**
     * @brief Unmaps a Vulkan buffer from CPU-accessible memory
     *
     * @param buffer The buffer resource to unmap from CPU-accessible memory
     */
    void UnmapBuffer(const Buffer &buffer)  // NOLINT(misc-use-internal-linkage)
    {
        const auto device = RenderContext::Get()->GetLogicDevice();
        SEDX_ASSERT(buffer.memory & MemoryType::CPU, "Buffer not accessible to the CPU.");
        vmaUnmapMemory(device->GetMemoryAllocator(), buffer.resource->allocation);
    }

    /// ----------------------------------------------------------

	/**
	 * @fn CreateStagingBuffer
	 * @brief Creates a host-visible staging buffer for CPU-to-GPU data transfer
	 *
	 * @details This method creates a buffer with transfer source usage and CPU-accessible memory,
	 * which is optimized for staging operations where data needs to be transferred from the CPU
	 * to the GPU. Staging buffers are commonly used in graphics pipelines for uploading textures,
	 * mesh data, and other GPU resources.
	 *
	 * The staging buffer is allocated with @enum VK_BUFFER_USAGE_TRANSFER_SRC_BIT and mapped memory
	 * that can be written to by the CPU. After writing data to this buffer, use a command buffer
	 * to copy the data to a GPU-local device buffer for optimal rendering performance.
	 *
	 * @param size Size of the buffer in bytes
	 * @param name Optional debug name for the buffer (defaults to "Staging Buffer" if empty)
	 * @return Buffer A configured buffer object ready for staging operations
	 *
	 * @see @fn CreateBuffer
	 */
    Buffer VulkanDevice::CreateStagingBuffer(const uint64_t size, const std::string &name)
    {
        return CreateBuffer(size, BufferUsage::TransferSrc, MemoryType::CPU, name.empty() ? "Staging Buffer" : name);
    }

    /**
     * @brief Copies data from one Vulkan buffer to another
     *
     * @param srcBuffer
     * @param dstBuffer
     * @param size
     */
    void CopyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size)
    {
        /// Get device and command buffer from render context
        const auto device = RenderContext::Get()->GetLogicDevice();
        VkCommandBuffer commandBuffer = device->GetCommandBuffer(true);

        VkBufferCopy copyRegion{};
        copyRegion.size = size;
        vkCmdCopyBuffer(commandBuffer, srcBuffer, dstBuffer, 1, &copyRegion);

        device->FlushCmdBuffer(commandBuffer);
    }

    /**
     * @brief Copies data between two Vulkan buffers with offset support
     *
     * @param srcBuffer Source buffer to copy from
     * @param dstBuffer Destination buffer to copy to
     * @param size Number of bytes to copy
     * @param srcOffset Offset in the source buffer (in bytes)
     * @param dstOffset Offset in the destination buffer (in bytes)
     */
    void CopyBufferRegion(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size,
                         VkDeviceSize srcOffset, VkDeviceSize dstOffset)
    {
        /// Get device and command buffer from render context
        const auto device = RenderContext::Get()->GetLogicDevice();
        VkCommandBuffer commandBuffer = device->GetCommandBuffer(true);

        VkBufferCopy copyRegion{};
        copyRegion.srcOffset = srcOffset;
        copyRegion.dstOffset = dstOffset;
        copyRegion.size = size;

        vkCmdCopyBuffer(commandBuffer, srcBuffer, dstBuffer, 1, &copyRegion);

        device->FlushCmdBuffer(commandBuffer);
    }

    /**
	 * @brief Copies data from a Vulkan buffer to a Vulkan image
	 *
	 * @param buffer The source buffer containing the data to copy
	 * @param image The destination image to copy the data into
	 * @param width The width of the image in pixels
	 * @param height The height of the image in pixels
	 */
    void CopyBufferToImage(VkBuffer buffer, VkImage image, uint32_t width, uint32_t height)
    {
        /// Get device and command buffer from render context
        const auto device = RenderContext::Get()->GetLogicDevice();
        const VkCommandBuffer commandBuffer = device->GetCommandBuffer(true);

        VkBufferImageCopy region;
        region.bufferOffset = 0;
        region.bufferRowLength = 0;
        region.bufferImageHeight = 0;
        region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        region.imageSubresource.mipLevel = 0;
        region.imageSubresource.baseArrayLayer = 0;
        region.imageSubresource.layerCount = 1;
        region.imageOffset = {.x = 0,.y = 0,.z = 0};
        region.imageExtent = {.width = width,.height = height,.depth = 1};

        vkCmdCopyBufferToImage(commandBuffer, buffer, image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);
        device->FlushCmdBuffer(commandBuffer);
    }

    /// ----------------------------------------------------------

    /*
    inline VkAttachmentLoadOp GetVkAttachmentLoadOp(const FramebufferSpecification &specification, const FramebufferTextureSpec &attachmentSpecification)
    {
        if (attachmentSpecification.loadOp == AttachmentLoadOp::Inherit)
        {
            if (IsDepthFormat(attachmentSpecification.Format))
                return specification.ClearDepthOnLoad ? VK_ATTACHMENT_LOAD_OP_CLEAR : VK_ATTACHMENT_LOAD_OP_LOAD;

            return specification.ClearColorOnLoad ? VK_ATTACHMENT_LOAD_OP_CLEAR : VK_ATTACHMENT_LOAD_OP_LOAD;
        }

        return attachmentSpecification.loadOp == AttachmentLoadOp::Clear ? VK_ATTACHMENT_LOAD_OP_CLEAR : VK_ATTACHMENT_LOAD_OP_LOAD;
    }
    */

    /// ----------------------------------------------------------

    /*
    Framebuffer::Framebuffer(const FramebufferSpecification &specification) : m_Specification(specification)
	{
		if (specification.Width == 0)
		{
            fb_width = Application::Get().GetWindow().GetWidth();
			fb_height = Application::Get().GetWindow().GetHeight();
		}
		else
		{
            fb_width = (uint32_t)(specification.Width * m_Specification.Scale);
			fb_height = (uint32_t)(specification.Height * m_Specification.Scale);
		}

		/// Create all image objects immediately so we can start referencing them elsewhere.
        if (!m_Specification.ExistingFramebuffer)
        {
            uint32_t attachmentIndex = 0;
            for (auto& attachmentSpec : m_Specification.Attachments.Attachments)
			{
				if (m_Specification.ExistingImage)
				{
					if (IsDepthFormat(attachmentSpec.Format))
						depthAttachmentImage = m_Specification.ExistingImage;
					else
						attachmentImages.emplace_back(m_Specification.ExistingImage);
				}
				else if (m_Specification.ExistingImages.find(attachmentIndex) != m_Specification.ExistingImages.end())
				{
					if (IsDepthFormat(attachmentSpec.Format))
						depthAttachmentImage = m_Specification.ExistingImages.at(attachmentIndex);
					else
						attachmentImages.emplace_back(); // This will be set later
				}
				else if (IsDepthFormat(attachmentSpec.Format))
				{
					ImageSpecification spec;
					spec.Format = attachmentSpec.Format;
					spec.Usage = ImageUsage::Attachment;
					spec.Transfer = m_Specification.Transfer;
                    spec.Width = (uint32_t)(fb_width * m_Specification.Scale);
					spec.Height = (uint32_t)(fb_height * m_Specification.Scale);
					spec.debugName = std::format("{0}-DepthAttachment{1}", m_Specification.debugName.empty() ? "Unnamed FB" : m_Specification.debugName, attachmentIndex);
					depthAttachmentImage = Image2D::Create(spec);
				}
				else
				{
					ImageSpecification spec;
					spec.Format = attachmentSpec.Format;
					spec.Usage = ImageUsage::Attachment;
					spec.Transfer = m_Specification.Transfer;
                    spec.Width = (uint32_t)(fb_width * m_Specification.Scale);
					spec.Height = (uint32_t)(fb_height * m_Specification.Scale);
					spec.debugName = std::format("{0}-ColorAttachment{1}", m_Specification.debugName.empty() ? "Unnamed FB" : m_Specification.debugName, attachmentIndex);
					attachmentImages.emplace_back(Image2D::Create(spec));
				}
				attachmentIndex++;
			}
		}

		SEDX_CORE_ASSERT(specification.Attachments.Attachments.size());
        Framebuffer::Resize(fb_width, fb_height, true);
	}

    void Framebuffer::Resize(uint32_t width, uint32_t height, bool forceRecreate)
    {
        if (!forceRecreate && (fb_width == width && fb_height == height))
            return;

        Ref<Framebuffer> instance = this;
        Renderer::SubmitFrame([instance, width, height]() mutable
        {
            instance->fb_width = (uint32_t)(width * instance->m_Specification.Scale);
            instance->fb_height = (uint32_t)(height * instance->m_Specification.Scale);
            if (!instance->m_Specification.SwapChainTarget)
            {
                instance->Invalidate_RenderThread();
            }
            else
            {
                SwapChain &swapChain = Application::Get().GetWindow().GetSwapChain();
                instance->renderPass = swapChain.GetRenderPass();

                instance->clearValues.clear();
                instance->clearValues.emplace_back().color = {{0.0f, 0.0f, 0.0f, 1.0f}};
            }
        });

        for (auto &callback : resizeCallbacks)
            callback(this);
    }


	void Framebuffer::AddResizeCallback(const std::function<void(Ref<Framebuffer>)>& func)
	{
		resizeCallbacks.push_back(func);
	}

	void Framebuffer::Invalidate()
	{
		Ref<Framebuffer> instance = this;
		Renderer::Submit([instance]() mutable
		{
			instance->Invalidate_RenderThread();
		});
	}

	void Framebuffer::Invalidate_RenderThread()
	{
		auto device = RenderContext::GetCurrentDevice()->GetDevice();

		Release();

		MemoryAllocator allocator("Framebuffer");

		std::vector<VkAttachmentDescription> attachmentDescriptions;

		std::vector<VkAttachmentReference> colorAttachmentReferences;
		VkAttachmentReference depthAttachmentReference;

		clearValues.resize(m_Specification.Attachments.Attachments.size());

		bool createImages = attachmentImages.empty();

		if (m_Specification.ExistingFramebuffer)
			attachmentImages.clear();

		uint32_t attachmentIndex = 0;
		for (const auto& attachmentSpec : m_Specification.Attachments.Attachments)
		{
			if (IsDepthFormat(attachmentSpec.Format))
			{
				if (m_Specification.ExistingImage)
				{
					depthAttachmentImage = m_Specification.ExistingImage;
				}
				else if (m_Specification.ExistingFramebuffer)
				{
					Ref<Framebuffer> existingFramebuffer = m_Specification.ExistingFramebuffer.As<Framebuffer>();
					depthAttachmentImage = existingFramebuffer->GetDepthImage();
				}
				else if (m_Specification.ExistingImages.find(attachmentIndex) != m_Specification.ExistingImages.end())
				{
					Ref<Image2D> existingImage = m_Specification.ExistingImages.at(attachmentIndex);
					SEDX_CORE_ASSERT(IsDepthFormat(existingImage->GetSpecification().Format), "Trying to attach non-depth image as depth attachment");
					depthAttachmentImage = existingImage;
				}
				else
				{
					Ref<Image2D> depthAttachmentImage = depthAttachmentImage.As<Image2D>();
					auto& spec = depthAttachmentImage->GetSpecification();
                    spec.Width = (uint32_t)(fb_width * m_Specification.Scale);
                    spec.Height = (uint32_t)(fb_height * m_Specification.Scale);
					depthAttachmentImage->Invalidate_RenderThread(); // Create immediately
				}

				VkAttachmentDescription& attachmentDescription = attachmentDescriptions.emplace_back();
				attachmentDescription.flags = 0;
				attachmentDescription.format = Utils::VulkanImageFormat(attachmentSpec.Format);
				attachmentDescription.samples = VK_SAMPLE_COUNT_1_BIT;
				attachmentDescription.loadOp = Utils::GetVkAttachmentLoadOp(m_Specification, attachmentSpec);
				attachmentDescription.storeOp = VK_ATTACHMENT_STORE_OP_STORE; // TODO: if sampling, needs to be store (otherwise DONT_CARE is fine)
				attachmentDescription.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
				attachmentDescription.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
				attachmentDescription.initialLayout = attachmentDescription.loadOp == VK_ATTACHMENT_LOAD_OP_CLEAR ? VK_IMAGE_LAYOUT_UNDEFINED : VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;
				if (attachmentSpec.Format == ImageFormat::DEPTH24STENCIL8 || true) // Separate layouts requires a "separate layouts" flag to be enabled
				{
					attachmentDescription.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL; // TODO: if not sampling
					attachmentDescription.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL; // TODO: if sampling
					depthAttachmentReference = { attachmentIndex, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL };
				}
				else
				{
					attachmentDescription.finalLayout = VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL; // TODO: if not sampling
					attachmentDescription.finalLayout = VK_IMAGE_LAYOUT_DEPTH_READ_ONLY_OPTIMAL; // TODO: if sampling
					depthAttachmentReference = { attachmentIndex, VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL };
				}
				clearValues[attachmentIndex].depthStencil = { m_Specification.DepthClearValue, 0 };
			}
			else
			{
				Ref<Image2D> colorAttachment;
				if (m_Specification.ExistingFramebuffer)
				{
					Ref<Framebuffer> existingFramebuffer = m_Specification.ExistingFramebuffer.As<Framebuffer>();
					Ref<Image2D> existingImage = existingFramebuffer->GetImage(attachmentIndex);
					colorAttachment = attachmentImages.emplace_back(existingImage).As<Image2D>();
				}
				else if (m_Specification.ExistingImages.find(attachmentIndex) != m_Specification.ExistingImages.end())
				{
					Ref<Image2D> existingImage = m_Specification.ExistingImages[attachmentIndex];
					SEDX_CORE_ASSERT(!IsDepthFormat(existingImage->GetSpecification().Format), "Trying to attach depth image as color attachment");
					colorAttachment = existingImage.As<Image2D>();
					attachmentImages[attachmentIndex] = existingImage;
				}
				else
				{
					if (createImages)
					{
						ImageSpecification spec;
						spec.Format = attachmentSpec.Format;
						spec.Usage = ImageUsage::Attachment;
						spec.Transfer = m_Specification.Transfer;
                        spec.Width = (uint32_t)(fb_width * m_Specification.Scale);
						spec.Height = (uint32_t)(fb_height * m_Specification.Scale);
						colorAttachment = attachmentImages.emplace_back(Image2D::Create(spec)).As<Image2D>();
						SEDX_CORE_VERIFY(false);

					}
					else
					{
						Ref<Image2D> image = attachmentImages[attachmentIndex];
						ImageSpecification& spec = image->GetSpecification();
                        spec.Width = (uint32_t)(fb_width * m_Specification.Scale);
						spec.Height = (uint32_t)(fb_height * m_Specification.Scale);
						colorAttachment = image.As<Image2D>();
						if (colorAttachment->GetSpecification().Layers == 1)
							colorAttachment->Invalidate_RenderThread(); // Create immediately
						else if (attachmentIndex == 0 && m_Specification.ExistingImageLayers[0] == 0)// Only invalidate the first layer from only the first framebuffer
						{
							colorAttachment->Invalidate_RenderThread(); // Create immediately
							colorAttachment->RT_CreatePerSpecificLayerImageViews(m_Specification.ExistingImageLayers);
						}
						else if (attachmentIndex == 0)
						{
							colorAttachment->RT_CreatePerSpecificLayerImageViews(m_Specification.ExistingImageLayers);
						}
					}

				}

				VkAttachmentDescription& attachmentDescription = attachmentDescriptions.emplace_back();
				attachmentDescription.flags = 0;
				attachmentDescription.format = Utils::VulkanImageFormat(attachmentSpec.Format);
				attachmentDescription.samples = VK_SAMPLE_COUNT_1_BIT;
				attachmentDescription.loadOp = Utils::GetVkAttachmentLoadOp(m_Specification, attachmentSpec);
				attachmentDescription.storeOp = VK_ATTACHMENT_STORE_OP_STORE; // TODO: if sampling, needs to be store (otherwise DONT_CARE is fine)
				attachmentDescription.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
				attachmentDescription.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
				attachmentDescription.initialLayout = attachmentDescription.loadOp == VK_ATTACHMENT_LOAD_OP_CLEAR ? VK_IMAGE_LAYOUT_UNDEFINED : VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
				attachmentDescription.finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

				const auto& clearColor = m_Specification.ClearColor;
				clearValues[attachmentIndex].color = { {clearColor.r, clearColor.g, clearColor.b, clearColor.a } };
				colorAttachmentReferences.emplace_back(VkAttachmentReference { attachmentIndex, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL });
			}

			attachmentIndex++;
		}

		VkSubpassDescription subpassDescription = {};
		subpassDescription.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
		subpassDescription.colorAttachmentCount = uint32_t(colorAttachmentReferences.size());
		subpassDescription.pColorAttachments = colorAttachmentReferences.data();
		if (depthAttachmentImage)
			subpassDescription.pDepthStencilAttachment = &depthAttachmentReference;

		// TODO: do we need these?
		// Use subpass dependencies for layout transitions
		std::vector<VkSubpassDependency> dependencies;

		if (attachmentImages.size())
		{
			{
				VkSubpassDependency& dependency = dependencies.emplace_back();
				dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
				dependency.dstSubpass = 0;
				dependency.srcStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
				dependency.srcAccessMask = VK_ACCESS_SHADER_READ_BIT;
				dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
				dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
				dependency.dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;
			}
			{
				VkSubpassDependency& dependency = dependencies.emplace_back();
				dependency.srcSubpass = 0;
				dependency.dstSubpass = VK_SUBPASS_EXTERNAL;
				dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
				dependency.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
				dependency.dstStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
				dependency.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
				dependency.dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;
			}
		}

		if (depthAttachmentImage)
		{
			{
				VkSubpassDependency& dependency = dependencies.emplace_back();
				dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
				dependency.dstSubpass = 0;
				dependency.srcStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
				dependency.dstStageMask = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
				dependency.srcAccessMask = VK_ACCESS_SHADER_READ_BIT;
				dependency.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
				dependency.dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;
			}

			{
				VkSubpassDependency& dependency = dependencies.emplace_back();
				dependency.srcSubpass = 0;
				dependency.dstSubpass = VK_SUBPASS_EXTERNAL;
				dependency.srcStageMask = VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
				dependency.dstStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
				dependency.srcAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
				dependency.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
				dependency.dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;
			}
		}

		/// Create the actual renderpass
		VkRenderPassCreateInfo renderPassInfo = {};
		renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
		renderPassInfo.attachmentCount = static_cast<uint32_t>(attachmentDescriptions.size());
		renderPassInfo.pAttachments = attachmentDescriptions.data();
		renderPassInfo.subpassCount = 1;
		renderPassInfo.pSubpasses = &subpassDescription;
		renderPassInfo.dependencyCount = static_cast<uint32_t>(dependencies.size());
		renderPassInfo.pDependencies = dependencies.data();

		VK_CHECK_RESULT(vkCreateRenderPass(device, &renderPassInfo, nullptr, &renderPass));
		SetDebugUtilsObjectName(device, VK_OBJECT_TYPE_RENDER_PASS, m_Specification.debugName, renderPass);

		std::vector<VkImageView> attachments(attachmentImages.size());
		for (uint32_t i = 0; i < attachmentImages.size(); i++)
		{
			Ref<Image2D> image = attachmentImages[i].As<Image2D>();
			if (image->GetSpecification().Layers > 1)
				attachments[i] = image->GetLayerImageView(m_Specification.ExistingImageLayers[i]);
			else
				attachments[i] = image->GetImageInfo().ImageView;
			SEDX_CORE_ASSERT(attachments[i]);
		}

		if (depthAttachmentImage)
		{
			Ref<Image2D> image = depthAttachmentImage.As<Image2D>();
			if (m_Specification.ExistingImage && image->GetSpecification().Layers > 1)
			{
				SEDX_CORE_ASSERT(m_Specification.ExistingImageLayers.size() == 1, "Depth attachments do not support deinterleaving");
				attachments.emplace_back(image->GetLayerImageView(m_Specification.ExistingImageLayers[0]));
			}
			else
				attachments.emplace_back(image->GetImageInfo().ImageView);

			SEDX_CORE_ASSERT(attachments.back());
		}

		VkFramebufferCreateInfo framebufferCreateInfo = {};
		framebufferCreateInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
		framebufferCreateInfo.renderPass = renderPass;
		framebufferCreateInfo.attachmentCount = uint32_t(attachments.size());
		framebufferCreateInfo.pAttachments = attachments.data();
        framebufferCreateInfo.width = fb_width;
		framebufferCreateInfo.height = fb_height;
		framebufferCreateInfo.layers = 1;

		VK_CHECK_RESULT(vkCreateFramebuffer(device, &framebufferCreateInfo, nullptr, &framebuffer));
		SetDebugUtilsObjectName(device, VK_OBJECT_TYPE_FRAMEBUFFER, m_Specification.debugName, framebuffer);
	}
	*/

}

/// ----------------------------------------------------------
