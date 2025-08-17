/**
* -------------------------------------------------------
* Scenery Editor X
* -------------------------------------------------------
* Copyright (c) 2025 Thomas Ray
* Copyright (c) 2025 Coalition of Freeware Developers
* -------------------------------------------------------
* framebuffer.h
* -------------------------------------------------------
* Created: 26/6/2025
* -------------------------------------------------------
*/
#pragma once
#include "SceneryEditorX/renderer/image_data.h"
#include "SceneryEditorX/renderer/vulkan/vk_enums.h"
#include "SceneryEditorX/renderer/vulkan/vk_image.h"

/// ---------------------------------------------------------

namespace SceneryEditorX
{
    /// Forward declarations
    class Framebuffer;

    struct FramebufferTextureSpecification
    {
        FramebufferTextureSpecification() = default;
        FramebufferTextureSpecification(VkFormat format) : Format(format) {}

        VkFormat Format;
        bool Blend = true;
        FramebufferBlendMode BlendMode = FramebufferBlendMode::SrcAlphaOneMinusSrcAlpha;
        AttachmentLoadOp LoadOp = AttachmentLoadOp::Inherit;
        /// TODO: filtering/wrap
    };

    /// ---------------------------------------------------------

    struct FramebufferAttachmentSpecification
    {
        FramebufferAttachmentSpecification() = default;
        FramebufferAttachmentSpecification(const std::initializer_list<FramebufferTextureSpecification> &attachments) : Attachments(attachments) {}

        std::vector<FramebufferTextureSpecification> Attachments;
    };

	/// ---------------------------------------------------------

    struct FramebufferSpecification
	{
		float scale = 1.0f;
		uint32_t width = 0;
		uint32_t height = 0;
		Vec4 clearColor = { 0.0f, 0.0f, 0.0f, 1.0f };
		float depthClearValue = 0.0f;
		bool clearColorOnLoad = true;
		bool clearDepthOnLoad = true;

		FramebufferAttachmentSpecification attachments;
		uint32_t samples = 1; ///< Multi-Sampling

		///< TODO: Temp, needs scale
		bool noResize = false;

		///< Master switch (individual attachments can be disabled in FramebufferTextureSpecification)
		bool blend = true;

		///< None means use BlendMode in FramebufferTextureSpecification
		FramebufferBlendMode blendMode = FramebufferBlendMode::None;

		///< SwapChainTarget = screen buffer (i.e. no framebuffer)
		bool swapChainTarget = false;

		///< Will it be used for transfer ops?
		bool transfer = false;

		/// Note: these are used to attach multi-layered color/depth images
		Ref<Image2D> existingImage;
		std::vector<uint32_t> existingImageLayers;

		///< Specify existing images to attach instead of creating new images. attachment index -> image
		std::map<uint32_t, Ref<Image2D>> existingImages;

		///< At the moment this will just create a new render pass with an existing framebuffer
		Ref<Framebuffer> existingFramebuffer;

		std::string debugName;
	};

    /// ---------------------------------------------------------

    class Framebuffer : public RefCounted
	{
	public:
        Framebuffer(const FramebufferSpecification &specification);
        virtual ~Framebuffer();

	    void Bind();
	    void Unbind();

        virtual void Resize(uint32_t width, uint32_t height, bool forceRecreate = false);
        virtual void AddResizeCallback(const std::function<void(Ref<Framebuffer>)> &func);
	    virtual void BindTexture(uint32_t attachmentIndex = 0, uint32_t slot = 0) const {}

	    virtual uint32_t GetWidth() const { return m_Width; }
		virtual uint32_t GetHeight() const { return m_Height; }
		virtual ResourceID GetRendererID() const { return m_ResourceID; }
		virtual ResourceID GetColorAttachmentRendererID() const { return 0; }
		virtual ResourceID GetDepthAttachmentRendererID() const { return 0; }

		virtual Ref<Image2D> GetImage(uint32_t attachmentIndex = 0) const  { SEDX_CORE_ASSERT(attachmentIndex < m_AttachmentImages.size()); return m_AttachmentImages[attachmentIndex]; }
		virtual Ref<Image2D> GetDepthImage() const { return m_DepthAttachmentImage; }
		virtual size_t GetColorAttachmentCount() const { return m_Specification.swapChainTarget ? 1 : m_AttachmentImages.size(); }
		virtual bool HasDepthAttachment() const { return (bool)m_DepthAttachmentImage; }
		VkRenderPass GetRenderPass() const { return m_RenderPass; }
		VkFramebuffer GetVulkanFramebuffer() const { return m_Framebuffer; }
		const std::vector<VkClearValue>& GetVulkanClearValues() const { return m_ClearValues; }
		virtual const FramebufferSpecification& GetSpecification() const { return m_Specification; }

        void Invalidate();
        void Invalidate_RenderThread();
        void Release() const;
	private:
        FramebufferSpecification m_Specification;
        ResourceID m_ResourceID = 0;
        uint32_t m_Width = 0;
        uint32_t m_Height = 0;

        std::vector<Ref<Image2D>> m_AttachmentImages;
        Ref<Image2D> m_DepthAttachmentImage;

        std::vector<VkClearValue> m_ClearValues;

        VkRenderPass m_RenderPass = nullptr;
        VkFramebuffer m_Framebuffer = nullptr;

        std::vector<std::function<void(Ref<Framebuffer>)>> m_ResizeCallbacks;
	};

}

/// -------------------------------------------------------
