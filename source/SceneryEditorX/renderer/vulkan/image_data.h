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
#include <imgui/imgui.h>
#include <SceneryEditorX/renderer/vulkan/render_data.h>

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

	enum Format : uint8_t
    {
        RGBA8_unorm			= 37,
        BGRA8_unorm			= 44,
        RG32_sfloat			= 103,
        RGB32_sfloat		= 106,
        RGBA32_sfloat		= 109,
        D32_sfloat			= 126,
        D24_unorm_S8_uint	= 129,
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

    struct ImageID
    {
        GLOBAL std::vector<int32_t> availBufferRID;
        GLOBAL std::vector<int32_t> availImageRID;
        GLOBAL std::vector<int32_t> availTLASRID;
    };

	struct ImageResource : Resource
    {
        VkImage image = nullptr;
        VkImageView view = nullptr;
        VmaAllocation allocation = nullptr;
		bool fromSwapchain = false;
		std::vector<VkImageView> layersView;
		std::vector<ImTextureID> imguiRIDs;
    };

	struct Image
	{
	    Ref<ImageResource> resource;
	    uint32_t width  = 0;
	    uint32_t height = 0;
        Format format;
        ImageUsageFlags usage;
        Layout::ImageLayout layout;
        AspectFlags aspect;
	    uint64_t layers = 1;
        [[nodiscard]] uint32_t ID() const;
        [[nodiscard]] ImTextureID ImGuiRID() const;
        [[nodiscard]] ImTextureID ImGuiRID(uint64_t layer) const;
	};

    struct ImageDescriptions
    {
        uint32_t width	= 0;
        uint32_t height = 0;
        Format format;
        ImageUsageFlags usage;
        std::string name;
        uint64_t layers = 1;
    };

} // namespace SceneryEditorX

/// -------------------------------------------------------
