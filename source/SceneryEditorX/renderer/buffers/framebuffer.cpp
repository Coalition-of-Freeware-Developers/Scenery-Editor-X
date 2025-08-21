/**
* -------------------------------------------------------
* Scenery Editor X
* -------------------------------------------------------
* Copyright (c) 2025 Thomas Ray
* Copyright (c) 2025 Coalition of Freeware Developers
* -------------------------------------------------------
* framebuffer.cpp
* -------------------------------------------------------
* Created: 26/6/2025
* -------------------------------------------------------
*/
#include "framebuffer.h"
#include "SceneryEditorX/core/application/application.h"
#include "SceneryEditorX/renderer/renderer.h"
#include "SceneryEditorX/renderer/vulkan/vk_swapchain.h"
#include "SceneryEditorX/renderer/vulkan/vk_util.h"

/// ---------------------------------------------------------

namespace SceneryEditorX
{
	namespace Utils
	{
	    inline VkAttachmentLoadOp GetVkAttachmentLoadOp(const FramebufferSpecification& specification, const FramebufferTextureSpecification& attachmentSpecification)
		{
			if (attachmentSpecification.LoadOp == AttachmentLoadOp::Inherit)
			{
				if (IsDepthFormat(attachmentSpecification.Format))
					return specification.clearDepthOnLoad ? VK_ATTACHMENT_LOAD_OP_CLEAR : VK_ATTACHMENT_LOAD_OP_LOAD;

				return specification.clearColorOnLoad ? VK_ATTACHMENT_LOAD_OP_CLEAR : VK_ATTACHMENT_LOAD_OP_LOAD;
			}

			return attachmentSpecification.LoadOp == AttachmentLoadOp::Clear ? VK_ATTACHMENT_LOAD_OP_CLEAR : VK_ATTACHMENT_LOAD_OP_LOAD;
		}
    }

    /// ---------------------------------------------------------

	Framebuffer::Framebuffer(const FramebufferSpecification &specification) : m_Specification(specification)
	{
	    if (specification.width == 0)
		{
			m_Width = Application::Get().GetWindow().GetWidth();
			m_Height = Application::Get().GetWindow().GetHeight();
		}
		else
		{
			m_Width = (uint32_t)(specification.width * m_Specification.scale);
			m_Height = (uint32_t)(specification.height * m_Specification.scale);
		}

		///< Create all image objects immediately so we can start referencing them elsewhere
        if (!m_Specification.existingFramebuffer)
        {
            uint32_t attachmentIndex = 0;
            for (const auto& attachmentSpec : m_Specification.attachments.Attachments)
			{
				if (m_Specification.existingImage)
				{
					if (IsDepthFormat(attachmentSpec.Format))
						m_DepthAttachmentImage = m_Specification.existingImage;
					else
						m_AttachmentImages.emplace_back(m_Specification.existingImage);
				}
				else if (m_Specification.existingImages.contains(attachmentIndex))
				{
					if (IsDepthFormat(attachmentSpec.Format))
						m_DepthAttachmentImage = m_Specification.existingImages.at(attachmentIndex);
					else
						m_AttachmentImages.emplace_back(); ///< This will be set later
				}
				else if (IsDepthFormat(attachmentSpec.Format))
				{
                    ImageSpecification spec;
					spec.format = attachmentSpec.Format;
					spec.usage = ImageUsage::DepthAttachment;
					spec.transfer = m_Specification.transfer;
					spec.width = (uint32_t)(m_Width * m_Specification.scale);
					spec.height = (uint32_t)(m_Height * m_Specification.scale);
					spec.debugName = std::format("{0}-DepthAttachment{1}", m_Specification.debugName.empty() ? "Unnamed FB" : m_Specification.debugName, attachmentIndex);
                    m_DepthAttachmentImage = CreateRef<Image2D>(spec);
				}
				else
				{
                    ImageSpecification spec;
					spec.format = attachmentSpec.Format;
					spec.usage = ImageUsage::ColorAttachment;
					spec.transfer = m_Specification.transfer;
					spec.width = (uint32_t)(m_Width * m_Specification.scale);
					spec.height = (uint32_t)(m_Height * m_Specification.scale);
					spec.debugName = std::format("{0}-ColorAttachment{1}", m_Specification.debugName.empty() ? "Unnamed FB" : m_Specification.debugName, attachmentIndex);
                    m_AttachmentImages.emplace_back(CreateRef<Image2D>(spec));
				}
				attachmentIndex++;
			}
		}

		SEDX_CORE_ASSERT(specification.attachments.Attachments.size());
        Framebuffer::Resize(m_Width, m_Height, true);
	}

	Framebuffer::~Framebuffer()
	{
        Release();
	}

	void Framebuffer::Bind()
	{
	}

	void Framebuffer::Unbind()
	{
	}

	void Framebuffer::Resize(uint32_t width, uint32_t height, bool forceRecreate)
	{
        if (!forceRecreate && (m_Width == width && m_Height == height))
            return;

        Ref<Framebuffer> instance(this);
        Renderer::Submit([instance, width, height]() mutable
        {
            instance->m_Width = (uint32_t)(width * instance->m_Specification.scale);
            instance->m_Height = (uint32_t)(height * instance->m_Specification.scale);
            if (!instance->m_Specification.swapChainTarget)
                instance->Invalidate_RenderThread();
            else
            {
                SwapChain &swapChain = Application::Get().GetWindow().GetSwapChain();
                instance->m_RenderPass = swapChain.GetRenderPass();

                instance->m_ClearValues.clear();
                instance->m_ClearValues.emplace_back().color = {{0.0f, 0.0f, 0.0f, 1.0f}};
            }
        });

		for (auto &callback : m_ResizeCallbacks)
            callback(Ref<Framebuffer>(this));
	}

	void Framebuffer::AddResizeCallback(const std::function<void(Ref<Framebuffer>)> &func)
	{
        m_ResizeCallbacks.push_back(func);
	}

	void Framebuffer::Invalidate()
	{
        Ref<Framebuffer> instance(this);
        Renderer::Submit([instance]() mutable { instance->Invalidate_RenderThread(); });
	}

	void Framebuffer::Invalidate_RenderThread()
	{
        auto device = RenderContext::GetCurrentDevice()->GetDevice();

        Release();

        MemoryAllocator allocator("Framebuffer");
        std::vector<VkAttachmentDescription> attachmentDescriptions;
        std::vector<VkAttachmentReference> colorAttachmentReferences;
        VkAttachmentReference depthAttachmentReference;

        m_ClearValues.resize(m_Specification.attachments.Attachments.size());

        bool createImages = m_AttachmentImages.empty();

        if (m_Specification.existingFramebuffer)
            m_AttachmentImages.clear();

        uint32_t attachmentIndex = 0;
        for (const auto &attachmentSpec : m_Specification.attachments.Attachments)
        {
            if (IsDepthFormat(attachmentSpec.Format))
            {
                if (m_Specification.existingImage)
                {
                    m_DepthAttachmentImage = m_Specification.existingImage;
                }
                else if (m_Specification.existingFramebuffer)
                {
                    Ref<Framebuffer> existingFramebuffer = m_Specification.existingFramebuffer.As<Framebuffer>();
                    m_DepthAttachmentImage = existingFramebuffer->GetDepthImage();
                }
                else if (m_Specification.existingImages.contains(attachmentIndex))
                {
                    Ref<Image2D> existingImage = m_Specification.existingImages.at(attachmentIndex);
                    SEDX_CORE_ASSERT(IsDepthFormat(existingImage->GetSpecification().format), "Trying to attach non-depth image as depth attachment");
                    m_DepthAttachmentImage = existingImage;
                }
                else
                {
                    Ref<Image2D> depthAttachmentImage = m_DepthAttachmentImage.As<Image2D>();
                    auto &spec = depthAttachmentImage->GetSpecification();
                    spec.width = (uint32_t)(m_Width * m_Specification.scale);
                    spec.height = (uint32_t)(m_Height * m_Specification.scale);
                    depthAttachmentImage->Invalidate_RenderThread(); /// Create immediately
                }

                VkAttachmentDescription &attachmentDescription = attachmentDescriptions.emplace_back();
                attachmentDescription.flags = 0;
                attachmentDescription.format = attachmentSpec.Format;
                attachmentDescription.samples = VK_SAMPLE_COUNT_1_BIT;
                attachmentDescription.loadOp = Utils::GetVkAttachmentLoadOp(m_Specification, attachmentSpec);
                attachmentDescription.storeOp = VK_ATTACHMENT_STORE_OP_STORE; /// TODO: if sampling, needs to be store (otherwise DONT_CARE is fine)
                attachmentDescription.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
                attachmentDescription.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
                attachmentDescription.initialLayout = attachmentDescription.loadOp == VK_ATTACHMENT_LOAD_OP_CLEAR ? VK_IMAGE_LAYOUT_UNDEFINED : VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;
                if (attachmentSpec.Format == VK_FORMAT_D24_UNORM_S8_UINT || true) ///< Separate layouts requires a "separate layouts" flag to be enabled
                {
                    attachmentDescription.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;	///< TODO: if not sampling
                    attachmentDescription.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;	///< TODO: if sampling
                    depthAttachmentReference = {.attachment = attachmentIndex, .layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL};
                }
                else
                {
                    attachmentDescription.finalLayout = VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL; ///< TODO: if not sampling
                    attachmentDescription.finalLayout = VK_IMAGE_LAYOUT_DEPTH_READ_ONLY_OPTIMAL;  ///< TODO: if sampling
                    depthAttachmentReference = {attachmentIndex, VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL};
                }

                m_ClearValues[attachmentIndex].depthStencil = {.depth = m_Specification.depthClearValue,.stencil = 0};
            }
            else
            {
                Ref<Image2D> colorAttachment;
                if (m_Specification.existingFramebuffer)
                {
                    Ref<Framebuffer> existingFramebuffer = m_Specification.existingFramebuffer.As<Framebuffer>();
                    Ref<Image2D> existingImage = existingFramebuffer->GetImage(attachmentIndex);
                    colorAttachment = m_AttachmentImages.emplace_back(existingImage).As<Image2D>();
                }
                else if (m_Specification.existingImages.contains(attachmentIndex))
                {
                    Ref<Image2D> existingImage = m_Specification.existingImages[attachmentIndex];
                    SEDX_CORE_ASSERT(!IsDepthFormat(existingImage->GetSpecification().format), "Trying to attach depth image as color attachment");
                    colorAttachment = existingImage.As<Image2D>();
                    m_AttachmentImages[attachmentIndex] = existingImage;
                }
                else
                {
                    if (createImages)
                    {
                        ImageSpecification spec;
                        spec.format = attachmentSpec.Format;
                        spec.usage = ImageUsage::ColorAttachment;
                        spec.transfer = m_Specification.transfer;
                        spec.width = (uint32_t)(m_Width * m_Specification.scale);
                        spec.height = (uint32_t)(m_Height * m_Specification.scale);
                        colorAttachment = m_AttachmentImages.emplace_back(CreateRef<Image2D>(spec)).As<Image2D>();
                        SEDX_CORE_VERIFY(false);
                    }
                    else
                    {
                        Ref<Image2D> image = m_AttachmentImages[attachmentIndex];
                        ImageSpecification &spec = image->GetSpecification();
                        spec.width = (uint32_t)(m_Width * m_Specification.scale);
                        spec.height = (uint32_t)(m_Height * m_Specification.scale);
                        colorAttachment = image.As<Image2D>();
                        if (colorAttachment->GetSpecification().layers == 1)
                            colorAttachment->Invalidate_RenderThread(); ///< Create immediately
                        else if (attachmentIndex == 0 && m_Specification.existingImageLayers[0] == 0) ///< Only invalidate the first layer from only the first framebuffer
                        {
                            colorAttachment->Invalidate_RenderThread(); ///< Create immediately
                            colorAttachment->CreatePerSpecificLayerImageViews_RenderThread(m_Specification.existingImageLayers);
                        }
                        else if (attachmentIndex == 0)
                            colorAttachment->CreatePerSpecificLayerImageViews_RenderThread(m_Specification.existingImageLayers);
                    }
                }

                VkAttachmentDescription &attachmentDescription = attachmentDescriptions.emplace_back();
                attachmentDescription.flags = 0;
                attachmentDescription.format = attachmentSpec.Format;
                attachmentDescription.samples = VK_SAMPLE_COUNT_1_BIT;
                attachmentDescription.loadOp = Utils::GetVkAttachmentLoadOp(m_Specification, attachmentSpec);
                attachmentDescription.storeOp = VK_ATTACHMENT_STORE_OP_STORE; // TODO: if sampling, needs to be store (otherwise DONT_CARE is fine)
                attachmentDescription.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
                attachmentDescription.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
                attachmentDescription.initialLayout = attachmentDescription.loadOp == VK_ATTACHMENT_LOAD_OP_CLEAR ? VK_IMAGE_LAYOUT_UNDEFINED : VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
                attachmentDescription.finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

                const auto &clearColor = m_Specification.clearColor;
                m_ClearValues[attachmentIndex].color = {{clearColor.r, clearColor.g, clearColor.b, clearColor.a}};
                colorAttachmentReferences.emplace_back(VkAttachmentReference{attachmentIndex, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL});
            }

            attachmentIndex++;
        }

        VkSubpassDescription subpassDescription = {};
        subpassDescription.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
        subpassDescription.colorAttachmentCount = static_cast<uint32_t>(colorAttachmentReferences.size());
        subpassDescription.pColorAttachments = colorAttachmentReferences.data();
        if (m_DepthAttachmentImage)
            subpassDescription.pDepthStencilAttachment = &depthAttachmentReference;

        /// TODO: do we need these?
        /// Use subpass dependencies for layout transitions
        std::vector<VkSubpassDependency> dependencies;

        if (!m_AttachmentImages.empty())
        {
            {
                VkSubpassDependency &dependency = dependencies.emplace_back();
                dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
                dependency.dstSubpass = 0;
                dependency.srcStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
                dependency.srcAccessMask = VK_ACCESS_SHADER_READ_BIT;
                dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
                dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
                dependency.dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;
            }
            {
                VkSubpassDependency &dependency = dependencies.emplace_back();
                dependency.srcSubpass = 0;
                dependency.dstSubpass = VK_SUBPASS_EXTERNAL;
                dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
                dependency.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
                dependency.dstStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
                dependency.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
                dependency.dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;
            }
        }

        if (m_DepthAttachmentImage)
        {
            {
                VkSubpassDependency &dependency = dependencies.emplace_back();
                dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
                dependency.dstSubpass = 0;
                dependency.srcStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
                dependency.dstStageMask = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
                dependency.srcAccessMask = VK_ACCESS_SHADER_READ_BIT;
                dependency.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
                dependency.dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;
            }

            {
                VkSubpassDependency &dependency = dependencies.emplace_back();
                dependency.srcSubpass = 0;
                dependency.dstSubpass = VK_SUBPASS_EXTERNAL;
                dependency.srcStageMask = VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
                dependency.dstStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
                dependency.srcAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
                dependency.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
                dependency.dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;
            }
        }

        ///< Create the actual render-pass
        VkRenderPassCreateInfo renderPassInfo = {};
        renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
        renderPassInfo.attachmentCount = static_cast<uint32_t>(attachmentDescriptions.size());
        renderPassInfo.pAttachments = attachmentDescriptions.data();
        renderPassInfo.subpassCount = 1;
        renderPassInfo.pSubpasses = &subpassDescription;
        renderPassInfo.dependencyCount = static_cast<uint32_t>(dependencies.size());
        renderPassInfo.pDependencies = dependencies.data();

        VK_CHECK_RESULT(vkCreateRenderPass(device, &renderPassInfo, nullptr, &m_RenderPass))
        SetDebugUtilsObjectName(device, VK_OBJECT_TYPE_RENDER_PASS, m_Specification.debugName, m_RenderPass);

        std::vector<VkImageView> attachments(m_AttachmentImages.size());
        for (uint32_t i = 0; i < m_AttachmentImages.size(); i++)
        {
            if (Ref<Image2D> image = m_AttachmentImages[i].As<Image2D>(); image->GetSpecification().layers > 1)
                attachments[i] = image->GetLayerImageView(m_Specification.existingImageLayers[i]);
            else
                attachments[i] = image->GetImageInfo().view;
            SEDX_CORE_ASSERT(attachments[i]);
        }

        if (m_DepthAttachmentImage)
        {
            if (Ref<Image2D> image = m_DepthAttachmentImage.As<Image2D>(); m_Specification.existingImage && image->GetSpecification().layers > 1)
            {
                SEDX_CORE_ASSERT(m_Specification.existingImageLayers.size() == 1, "Depth attachments do not support deinterleaving");
                attachments.emplace_back(image->GetLayerImageView(m_Specification.existingImageLayers[0]));
            }
            else
            {
                attachments.emplace_back(image->GetImageInfo().view);
            }

            SEDX_CORE_ASSERT(attachments.back());
        }

        VkFramebufferCreateInfo framebufferCreateInfo = {};
        framebufferCreateInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        framebufferCreateInfo.renderPass = m_RenderPass;
        framebufferCreateInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
        framebufferCreateInfo.pAttachments = attachments.data();
        framebufferCreateInfo.width = m_Width;
        framebufferCreateInfo.height = m_Height;
        framebufferCreateInfo.layers = 1;

        VK_CHECK_RESULT(vkCreateFramebuffer(device, &framebufferCreateInfo, nullptr, &m_Framebuffer))
        SetDebugUtilsObjectName(device, VK_OBJECT_TYPE_FRAMEBUFFER, m_Specification.debugName, m_Framebuffer);
    }

	void Framebuffer::Release() const
    {
        if (m_Framebuffer)
        {
            VkFramebuffer framebuffer = m_Framebuffer;
            Renderer::SubmitResourceFree([framebuffer]()
            {
                const auto device = RenderContext::GetCurrentDevice()->GetDevice();
                vkDestroyFramebuffer(device, framebuffer, nullptr);
            });

            ///< Don't free the images if we don't own them
            if (!m_Specification.existingFramebuffer)
            {
                uint32_t attachmentIndex = 0;
                for (const Ref<Image2D> &image : m_AttachmentImages)
                {
                    if (m_Specification.existingImages.contains(attachmentIndex))
                        continue;

                    ///< Only destroy deinterleaved image once and prevent clearing layer views on second framebuffer invalidation
                    if (image->GetSpecification().layers == 1 || attachmentIndex == 0 && !image->GetLayerImageView(0))
                        image->Release();
                    attachmentIndex++;
                }

                if (m_DepthAttachmentImage)
                {
                    ///< Do we own the depth image?
                    if (!m_Specification.existingImages.contains((uint32_t)m_Specification.attachments.Attachments.size() - 1))
                        m_DepthAttachmentImage->Release();
                }
            }
        }
	}

}

/// -------------------------------------------------------
