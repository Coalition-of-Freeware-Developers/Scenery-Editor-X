/**
* -------------------------------------------------------
* Scenery Editor X
* -------------------------------------------------------
* Copyright (c) 2025 Thomas Ray 
* Copyright (c) 2025 Coalition of Freeware Developers
* -------------------------------------------------------
* buffer_data.h
* -------------------------------------------------------
* Created: 20/4/2025
* -------------------------------------------------------
*/
#pragma once
#include <SceneryEditorX/renderer/render_data.h>
#include <vma/vk_mem_alloc.h>

// -----------------------------------------

namespace SceneryEditorX
{
    class MemoryAllocator;

	using Flags = uint32_t;

	enum MemoryType : uint8_t
    {
	    GPU = 0x00000001,
	    CPU = 0x00000002 | 0x00000004,
	};
	using MemoryFlags = Flags;

    namespace BufferUsage
    {
		enum
		{
		    TransferSrc = 0x00000001,
		    TransferDst = 0x00000002,
		    UniformTexel = 0x00000004,
		    StorageTexel = 0x00000008,
		    Uniform = 0x00000010,
		    Storage = 0x00000020,
		    Index = 0x00000040,
		    Vertex = 0x00000080,
		    Indirect = 0x00000100,
		    Address = 0x00020000,
		    VideoDecodeSrc = 0x00002000,
		    VideoDecodeDst = 0x00004000,
		    TransformFeedback = 0x00000800,
		    TransformFeedbackCounter = 0x00001000,
		    ConditionalRendering = 0x00000200,
		    AccelerationStructureInput = 0x00080000,
		    AccelerationStructure = 0x00100000,
		    ShaderBindingTable = 0x00000400,
		    SamplerDescriptor = 0x00200000,
		    ResourceDescriptor = 0x00400000,
		    PushDescriptors = 0x04000000,
		    MicromapBuildInputReadOnly = 0x00800000,
		    MicromapStorage = 0x01000000,
		};

    }
    using BufferUsageFlags = Flags;

    // -------------------------------------------------------

    namespace VulkanMemoryUtils
    {

        void DestroyBuffer(VkBuffer buffer, VmaAllocation allocation);

    }

    // -------------------------------------------------------

    struct BufferResource : Resource
    {
        VkBuffer buffer;
        VmaAllocation allocation;

        virtual ~BufferResource()
        {
            VulkanMemoryUtils::DestroyBuffer(buffer, allocation);
        }
    };

    struct Buffer
    {
        std::shared_ptr<BufferResource> resource;
        uint32_t size;
        BufferUsageFlags usage;
        MemoryFlags memory;
        [[nodiscard]] uint32_t ResourceID() const;
    };

    inline uint32_t Buffer::ResourceID() const
    {
        SEDX_ASSERT(resource->resourceID != -1, "Invalid buffer rid");
        return uint32_t(resource->resourceID);
    }

}  // namespace SceneryEditorX

// ---------------------------------------------------------
