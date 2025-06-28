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
#include <glm/gtc/integer.hpp>
#include <imgui/imgui.h>
#include <SceneryEditorX/renderer/vulkan/resource.h>
#include <SceneryEditorX/renderer/vulkan/vk_allocator.h> /// Used by VmaAllocation in struct. !MUST KEEP!

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
    using ImageUsageFlags = Flags;

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
	        DepthReadStencilAttachment	= 1000117000,
	        DepthAttachmentStencilRead	= 1000117001,
	        DepthAttachment				= 1000241000,
	        DepthRead					= 1000241001,
	        StencilAttachment			= 1000241002,
	        StencilRead					= 1000241003,
	        Read						= 1000314000,
	        Attachment					= 1000314001,
	        Present						= 1000001002,
	    };
    }

	enum ImageFormat : uint8_t
    {
        RGBA8_unorm			= 37,
        BGRA8_unorm			= 44,
        RG32_sfloat			= 103,
        RGB32_sfloat		= 106,
        RGBA32_sfloat		= 109,
        D32_sfloat			= 126,
        D24_unorm_S8_uint	= 129,
    };

    inline void GetVkFormat()
    {
		RenderContext::GetCurrentDevice()->GetPhysicalDevice()->GetDepthFormat();
    }

    enum class UVWrap : uint8_t
    {
        None = 0,
        Clamp,
        Repeat
    };

    struct ImageSubresourceRange
    {
        uint32_t mip = 0;
        uint32_t mipCount = 0xffffffff;
        uint32_t layer = 0;
        uint32_t layerCount = 0xffffffff;
    };

    union ImageClearValue
    {
        glm::vec4 FloatValues;
        glm::ivec4 IntValues;
        glm::uvec4 UIntValues;
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

    struct ImageID
    {
        GLOBAL std::vector<int32_t> availBufferRID;
        GLOBAL std::vector<int32_t> availImageRID;
        GLOBAL std::vector<int32_t> availTLASRID;
    };

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

    };

    /// -------------------------------------------------------

	struct Image
	{
	    Ref<ImageResource> resource;
	    uint32_t width  = 0;
	    uint32_t height = 0;
        uint32_t mips = 1;
        uint32_t layers = 1;
        ImageFormat format;
        ImageUsageFlags usage;
        Layout::ImageLayout layout;
        AspectFlags aspect;
        bool createSampler = true;
        bool transfer = false; /// Will it be used for transfer ops?
        [[nodiscard]] uint32_t ID() const;
        [[nodiscard]] ImTextureID ImGuiRID() const;
        [[nodiscard]] ImTextureID ImGuiRID(uint64_t layer) const;
	};

    /// -------------------------------------------------------

    inline bool IsDepthFormat(const ImageFormat format)
    {
        if (format == VK_FORMAT_D24_UNORM_S8_UINT || format == VK_FORMAT_D32_SFLOAT || format == VK_FORMAT_D32_SFLOAT_S8_UINT)
            return true;

        return false;
    }

    /// -------------------------------------------------------

    struct ImageDescriptions
    {
        std::string name;
        ImageFormat format;
        ImageUsageFlags usage;
        uint32_t width = 1;
        uint32_t height = 1;
        uint64_t layers = 1;
        uint32_t mips = 1;
    };

    /// -------------------------------------------------------

    
    void InsertImageMemoryBarrier(VkCommandBuffer cmdbuffer,
                                  VkImage image,
                                  VkAccessFlags srcAccessMask,
                                  VkAccessFlags dstAccessMask,
                                  VkImageLayout oldImageLayout,
                                  VkImageLayout newImageLayout,
                                  VkPipelineStageFlags srcStageMask,
                                  VkPipelineStageFlags dstStageMask,
                                  VkImageSubresourceRange subresourceRange)
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

    void SetImageLayout(VkCommandBuffer cmdbuffer,
                        VkImage image,
                        VkImageLayout oldImageLayout,
                        VkImageLayout newImageLayout,
                        VkImageSubresourceRange subresourceRange,
                        VkPipelineStageFlags srcStageMask,
                        VkPipelineStageFlags dstStageMask)
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

    void SetImageLayout(VkCommandBuffer cmdbuffer,
                        VkImage image,
                        VkImageAspectFlags aspectMask,
                        VkImageLayout oldImageLayout,
                        VkImageLayout newImageLayout,
                        VkPipelineStageFlags srcStageMask,
                        VkPipelineStageFlags dstStageMask)
    {
        VkImageSubresourceRange subresourceRange = {};
        subresourceRange.aspectMask = aspectMask;
        subresourceRange.baseMipLevel = 0;
        subresourceRange.levelCount = 1;
        subresourceRange.layerCount = 1;
        SetImageLayout(cmdbuffer, image, oldImageLayout, newImageLayout, subresourceRange, srcStageMask, dstStageMask);
    }


} // namespace SceneryEditorX

/// -------------------------------------------------------
