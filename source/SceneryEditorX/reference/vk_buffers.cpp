/**
* -------------------------------------------------------
* Scenery Editor X
* -------------------------------------------------------
* Copyright (c) 2025 Thomas Ray 
* Copyright (c) 2025 Coalition of Freeware Developers
* -------------------------------------------------------
* vk_buffer.cpp
* -------------------------------------------------------
* Created: 19/3/2025
* -------------------------------------------------------
*/

#include <SceneryEditorX/renderer/vk_buffers.h>
#include <SceneryEditorX/renderer/vk_util.h>

// -------------------------------------------------------

extern VkPhysicalDeviceProperties physicalProperties;

// -------------------------------------------------------

Buffer CreateBuffer(uint32_t size, BufferUsageFlags usage, MemoryFlags memory, const std::string &name)
{
    if (usage & BufferUsage::Vertex)
    {
        usage |= BufferUsage::TransferDst;
    }

    if (usage & BufferUsage::Index)
    {
        usage |= BufferUsage::TransferDst;
    }

    if (usage & BufferUsage::Storage)
    {
        usage |= BufferUsage::Address;
        size += size % physicalProperties.limits.minStorageBufferOffsetAlignment;
    }

    if (usage & BufferUsage::AccelerationStructureInput)
    {
        usage |= BufferUsage::Address;
        usage |= BufferUsage::TransferDst;
    }

    if (usage & BufferUsage::AccelerationStructure)
    {
        usage |= BufferUsage::Address;
    }

    std::shared_ptr<BufferResource> result = std::make_shared<BufferResource>();
    VkBufferCreateInfo bufferInfo{};
        bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        bufferInfo.size = size;
        bufferInfo.usage = (VkBufferUsageFlagBits)usage;
        bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    VmaAllocationCreateInfo allocInfo = {};
        allocInfo.usage = VMA_MEMORY_USAGE_AUTO;
        if (memory & Memory::CPU)
        {
            allocInfo.flags = VMA_ALLOCATION_CREATE_MAPPED_BIT | VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT;
        }

    auto result = vmaCreateBuffer(vmaAllocator, &bufferInfo, &allocInfo, &result->buffer, &result->allocation, nullptr);
    //VK_CHECK_RESULT(result, "Failed to create buffer!");

    Buffer buffer = {
        .resource = result,
        .size = size,
        .usage = usage,
        .memory = memory,
    };

    if (usage & BufferUsage::Storage)
    {
        result->rid = availableBufferRID.back();
        availableBufferRID.pop_back();
        VkDescriptorBufferInfo descriptorInfo = {};
            descriptorInfo.buffer = result->buffer;
            descriptorInfo.offset = 0;
            descriptorInfo.range = size;

        VkWriteDescriptorSet write = {};
            write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            write.dstSet = bindlessDescriptorSet;
            write.dstBinding = 1;
            write.dstArrayElement = buffer.RID();
            write.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
            write.descriptorCount = 1;
            write.pBufferInfo = &descriptorInfo;
        vkUpdateDescriptorSets(g_Device, 1, &write, 0, nullptr);
    }

    return buffer;
}
