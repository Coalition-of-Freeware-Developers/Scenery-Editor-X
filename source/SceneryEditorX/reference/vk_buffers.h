/**
* -------------------------------------------------------
* Scenery Editor X
* -------------------------------------------------------
* Copyright (c) 2025 Thomas Ray 
* Copyright (c) 2025 Coalition of Freeware Developers
* -------------------------------------------------------
* vk_buffers.h
* -------------------------------------------------------
* Created: 19/3/2025
* -------------------------------------------------------
*/

#pragma once

#include <vulkan/vulkan_core.h>
#include <vk_mem_alloc.h>
#include <SceneryEditorX/renderer/vk_core.h>

// -------------------------------------------------------

using Flags = uint32_t;

enum Memory
{
    GPU = 0x00000001,
    CPU = 0x00000002 | 0x00000004,
};

const uint32_t timeStampPerPool = 64;
extern VkDevice g_Device;

using MemoryFlags = Flags;
using BufferUsageFlags = Flags;

// -------------------------------------------------------

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

struct Resource
{
    std::string name;
    int32_t rid = -1;
    virtual ~Resource() {};
};

struct BufferResource : Resource
{
    VkBuffer buffer;
    VmaAllocation allocation;
    VmaAllocator vmaAllocator;

    virtual ~BufferResource()
    {
        vmaDestroyBuffer(vmaAllocator, buffer, allocation);
    }
};

struct Buffer
{
    std::shared_ptr<BufferResource> resource;
    uint32_t size;
    BufferUsageFlags usage;
    MemoryFlags memory;
    uint32_t RID();
};

// -------------------------------------------------------

const uint32_t initialScratchBufferSize = 64 * 1024 * 1024;
Buffer asScratchBuffer;
VkDeviceAddress asScratchAddress;

// -----------------------------------------

VmaAllocator vmaAllocator;
std::vector<int32_t> availableBufferRID;
std::vector<int32_t> availableImageRID;
std::vector<int32_t> availableTLASRID;
VkSampler genericSampler;

// -------------------------------------------------------

VkDescriptorPool imguiDescriptorPool = VK_NULL_HANDLE;
VkDescriptorSet bindlessDescriptorSet = VK_NULL_HANDLE;
VkDescriptorPool bindlessDescriptorPool = VK_NULL_HANDLE;
VkDescriptorSetLayout bindlessDescriptorLayout = VK_NULL_HANDLE;

// -------------------------------------------------------

