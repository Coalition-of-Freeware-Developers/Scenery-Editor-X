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
#include <SceneryEditorX/renderer/render_context.h>
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

    /*
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
	*/

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

    enum class ImageFilter : uint8_t
    {
        None = 0,
        Linear,
        Nearest,
        Cubic
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
        Vec4 FloatValues;
        iVec4 IntValues;
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
        if (format == VK_FORMAT_D24_UNORM_S8_UINT || format == VK_FORMAT_D32_SFLOAT || format == VK_FORMAT_D32_SFLOAT_S8_UINT)
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
