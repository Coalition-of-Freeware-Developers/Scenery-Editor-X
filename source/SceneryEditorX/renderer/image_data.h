/**
* -------------------------------------------------------
* Scenery Editor X
* -------------------------------------------------------
* Copyright (c) 2025 Thomas Ray
* Copyright (c) 2025 Coalition of Freeware Developers
* -------------------------------------------------------
* image_data.h
* -------------------------------------------------------
* Created: 29/4/2025
* -------------------------------------------------------
*/
#pragma once
#include <cstdint>
#include <xMath/includes/xmath.hpp>
#include <imgui/imgui.h>
#include "render_context.h"
#include "SceneryEditorX/renderer/vulkan/vk_includes.h"
#include "vulkan/resource.h"
#include "vulkan/vk_allocator.h" /// Used by VmaAllocation in struct. !MUST KEEP!

/// -------------------------------------------------------

namespace SceneryEditorX
{
	namespace ImageUsage
	{
		enum ImageUsageFlags : uint8_t
		{
		    TransferSrc		= 0x00000001,
		    TransferDst		= 0x00000002,
		    Sampled			= 0x00000004,
		    Storage			= 0x00000008,
		    ColorAttachment = 0x00000010,
		    DepthAttachment = 0x00000020,
		};
	}
	//using ImageUsageFlags = Flags;
    using ImageUsageFlags = uint32_t; // previously Flags (uint32_t) – localized to remove dependency on vk_data.h

	namespace Layout
	{
	    enum ImageLayout
	    {
	        Undefined					= 0,
	        General						= 1,
	        ColorAttachment				= 2,
	        DepthStencilAttachment		= 3,
	        DepthStencilRead			= 4,
	        ShaderRead					= 5,
	        TransferSrc					= 6,
	        TransferDst					= 7,
            Preinitialized				= 8,
	        DepthReadStencilAttachment	= 1000117000,
	        DepthAttachmentStencilRead	= 1000117001,
	        DepthAttachment				= 1000241000,
	        DepthRead					= 1000241001,
	        StencilAttachment			= 1000241002,
	        StencilRead					= 1000241003,
            FragmentShadeRate			= 1000164003,
	        Read						= 1000314000,
	        Attachment					= 1000314001,
	        Present						= 1000001002,
            Max							= 0x7FFFFFFF
	    };
    }

    inline bool IsIntegerBased(const VkFormat format)
	{
        switch (format)
        {
        case VK_FORMAT_R16_UINT:
        case VK_FORMAT_R32_UINT:
        case VK_FORMAT_R8_UINT:
        case VK_FORMAT_D32_SFLOAT_S8_UINT:
            return true;
        case VK_FORMAT_D32_SFLOAT:
        case VK_FORMAT_R8_UNORM:
        case VK_FORMAT_R32G32B32A32_SFLOAT:
        case VK_FORMAT_B10G11R11_UFLOAT_PACK32:
        case VK_FORMAT_R16G16_SFLOAT:
        case VK_FORMAT_R32G32_SFLOAT:
        case VK_FORMAT_R32_SFLOAT:
        case VK_FORMAT_R8G8_UNORM:
        case VK_FORMAT_R8G8B8A8_UNORM:
        case VK_FORMAT_R16G16B16A16_SFLOAT:
        case VK_FORMAT_R8G8B8_UNORM:
        case VK_FORMAT_R8G8B8_SRGB:
        case VK_FORMAT_R8G8B8A8_SRGB:
        case VK_FORMAT_UNDEFINED:
            return false;
        }
        SEDX_CORE_ASSERT(false);
        return false;
	}

    inline void GetDepthFormat()
    {
		RenderContext::GetCurrentDevice()->GetPhysicalDevice()->GetDepthFormat();
    }

    struct ImageSubresourceRange
    {
        uint32_t mip = 0;
        uint32_t mipCount = 0xffffffff;
        uint32_t layer = 0;
        uint32_t layerCount = 0xffffffff;
    };

    namespace Aspect
    {
        enum AspectFlags : uint8_t
		{
		    Color	= 1,
		    Depth	= 2,
		    Stencil = 4,
		};
    }
	using AspectFlags = Flags;

    /// -------------------------------------------------------

    struct ImageID { /* Legacy resource ID pools removed */ };

    /// -------------------------------------------------------

	struct ImageResource : Resource
    {
        VkImage image = nullptr;
        VkImageView view = nullptr;
        VkSampler sampler = nullptr;
        VmaAllocation allocation = nullptr;
		bool fromSwapchain = false;
		std::vector<VkImageView> layersView;
		std::vector<ImTextureID> imguiRIDs;
        virtual ResourceDescriptorInfo GetDescriptorInfo() const override;
    };

    /// -------------------------------------------------------

	struct Image
	{
	    Ref<ImageResource> resource;
	    uint32_t width  = 0;
	    uint32_t height = 0;
        uint32_t mips = 1;
        uint32_t layers = 1;
        VkFormat format;
        ImageUsageFlags usage;
        Layout::ImageLayout layout;
        AspectFlags aspect;
        bool transfer = false; /// Will it be used for transfer ops?
        bool createSampler = true;
        [[nodiscard]] uint32_t ID() const;
        [[nodiscard]] ImTextureID ImGuiRID() const;
        [[nodiscard]] ImTextureID ImGuiRID(uint64_t layer) const;
	};

    /// -------------------------------------------------------

    inline bool IsDepthFormat(const VkFormat format)
    {
        if (format == VK_FORMAT_D24_UNORM_S8_UINT ||
			format == VK_FORMAT_D32_SFLOAT ||
			format == VK_FORMAT_D32_SFLOAT_S8_UINT)
            return true;

        return false;
    }

    /// -------------------------------------------------------

    struct ImageDescriptions
    {
        std::string name;
        VkFormat format;
        ImageUsageFlags usage;
        uint32_t width = 1;
        uint32_t height = 1;
        uint64_t layers = 1;
        uint32_t mips = 1;
    };

    /// -------------------------------------------------------


    void InsertImageMemoryBarrier(VkCommandBuffer cmdbuffer, VkImage image,
                                  VkAccessFlags srcAccessMask, VkAccessFlags dstAccessMask,
                                  VkImageLayout oldImageLayout, VkImageLayout newImageLayout,
                                  VkPipelineStageFlags srcStageMask, VkPipelineStageFlags dstStageMask,
                                  VkImageSubresourceRange &subresourceRange);


    void SetImageLayout(VkCommandBuffer cmdbuffer, VkImage image,
                        VkImageLayout oldImageLayout, VkImageLayout newImageLayout,
                        VkImageSubresourceRange &subresourceRange,
                        VkPipelineStageFlags srcStageMask, VkPipelineStageFlags dstStageMask);

    void SetImageLayout(VkCommandBuffer cmdbuffer, VkImage image, VkImageAspectFlags aspectMask,
                        VkImageLayout oldImageLayout, VkImageLayout newImageLayout,
                        VkPipelineStageFlags srcStageMask, VkPipelineStageFlags dstStageMask);


}

/// -------------------------------------------------------
