/**
* -------------------------------------------------------
* Scenery Editor X - Unit Tests
* -------------------------------------------------------
* Copyright (c) 2025 Thomas Ray 
* Copyright (c) 2025 Coalition of Freeware Developers
* -------------------------------------------------------
* MemoryPoolTest.cpp
* -------------------------------------------------------
* Tests for the memory pool functionality of MemoryAllocator
* -------------------------------------------------------
*/
#include <catch2/catch_test_macros.hpp>
#include "MemoryAllocatorTest.h"

/// -------------------------------------------------------

namespace SceneryEditorX
{
    namespace Tests
    {
        TEST_CASE_METHOD(MemoryAllocatorTestFixture, "Memory pool operations", "[VulkanMemory][Pool]")
        {
            SECTION("Small buffer pool allocations")
            {
                /// Make multiple allocations of small buffer size to trigger pool creation
                constexpr int numAllocations = 10;
                std::vector<MockAllocationTracker> trackers(numAllocations);
                
                for (auto& tracker : trackers)
				{
                    VkBufferCreateInfo bufferInfo = {};
                    bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
                    bufferInfo.size = SMALL_BUFFER_SIZE - 1024; /// Just under the small buffer size
                    bufferInfo.usage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
                    bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
                    
                    REQUIRE_NOTHROW(tracker.allocation = allocator->AllocateBuffer(bufferInfo, VMA_MEMORY_USAGE_GPU_ONLY, tracker.buffer));
                    REQUIRE(tracker.allocation != nullptr);
                    REQUIRE(tracker.buffer != VK_NULL_HANDLE);
                }
                
                /// Check allocator stats to confirm pool allocation
                auto stats = allocator->GetStats();
                
                /// Free all allocations
                for (auto& tracker : trackers)
				{
                    REQUIRE_NOTHROW(allocator->DestroyBuffer(tracker.buffer, tracker.allocation));
                    tracker.buffer = VK_NULL_HANDLE;
                    tracker.allocation = nullptr;
                }
            }
            
            SECTION("Medium buffer pool allocations")
            {
                /// Make multiple allocations of medium buffer size to trigger pool creation
                constexpr int numAllocations = 5;
                std::vector<MockAllocationTracker> trackers(numAllocations);
                
                for (auto& tracker : trackers)
				{
                    VkBufferCreateInfo bufferInfo = {};
                    bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
                    bufferInfo.size = MEDIUM_BUFFER_SIZE - 1024; /// Just under the medium buffer size
                    bufferInfo.usage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
                    bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
                    
                    REQUIRE_NOTHROW(tracker.allocation = allocator->AllocateBuffer(bufferInfo, VMA_MEMORY_USAGE_GPU_ONLY, tracker.buffer));
                    REQUIRE(tracker.allocation != nullptr);
                    REQUIRE(tracker.buffer != VK_NULL_HANDLE);
                }
                
                /// Free all allocations
                for (auto& tracker : trackers)
				{
                    REQUIRE_NOTHROW(allocator->DestroyBuffer(tracker.buffer, tracker.allocation));
                    tracker.buffer = VK_NULL_HANDLE;
                    tracker.allocation = nullptr;
                }
            }
            
            SECTION("Large buffer pool allocations")
            {
                /// Make a few allocations of large buffer size
                constexpr int numAllocations = 2;
                std::vector<MockAllocationTracker> trackers(numAllocations);
                
                for (auto& tracker : trackers)
				{
                    VkBufferCreateInfo bufferInfo = {};
                    bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
                    bufferInfo.size = LARGE_BUFFER_SIZE - 1024; /// Just under the large buffer size
                    bufferInfo.usage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
                    bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
                    
                    REQUIRE_NOTHROW(tracker.allocation = allocator->AllocateBuffer(bufferInfo, VMA_MEMORY_USAGE_GPU_ONLY, tracker.buffer));
                    REQUIRE(tracker.allocation != nullptr);
                    REQUIRE(tracker.buffer != VK_NULL_HANDLE);
                }
                
                /// Free all allocations
                for (auto& tracker : trackers)
				{
                    REQUIRE_NOTHROW(allocator->DestroyBuffer(tracker.buffer, tracker.allocation));
                    tracker.buffer = VK_NULL_HANDLE;
                    tracker.allocation = nullptr;
                }
            }
            
            SECTION("Oversized buffer allocation")
            {
                /// Allocate a buffer larger than LARGE_BUFFER_SIZE
                auto& tracker = trackAllocation();
                
                VkBufferCreateInfo bufferInfo = {};
                bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
                bufferInfo.size = LARGE_BUFFER_SIZE * 2; /// Twice the large buffer size
                bufferInfo.usage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
                bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
                
                REQUIRE_NOTHROW(tracker.allocation = allocator->AllocateBuffer(bufferInfo, VMA_MEMORY_USAGE_GPU_ONLY, tracker.buffer));
                REQUIRE(tracker.allocation != nullptr);
                REQUIRE(tracker.buffer != VK_NULL_HANDLE);
                
                /// Free the allocation
                REQUIRE_NOTHROW(allocator->DestroyBuffer(tracker.buffer, tracker.allocation));
                tracker.buffer = VK_NULL_HANDLE;
                tracker.allocation = nullptr;
            }
            
            SECTION("Mixed pool allocations")
            {
                /// Allocate buffers of various sizes to test all pools
                std::vector<MockAllocationTracker> trackers;
                trackers.resize(6);
                
                /// Small buffer
                {
                    auto& tracker = trackers[0];
                    VkBufferCreateInfo bufferInfo = {};
                    bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
                    bufferInfo.size = SMALL_BUFFER_SIZE / 2;
                    bufferInfo.usage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
                    bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
                    
                    REQUIRE_NOTHROW(tracker.allocation = allocator->AllocateBuffer(bufferInfo, VMA_MEMORY_USAGE_GPU_ONLY, tracker.buffer));
                }
                
                /// Medium buffer
                {
                    auto& tracker = trackers[1];
                    VkBufferCreateInfo bufferInfo = {};
                    bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
                    bufferInfo.size = MEDIUM_BUFFER_SIZE / 2;
                    bufferInfo.usage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
                    bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
                    
                    REQUIRE_NOTHROW(tracker.allocation = allocator->AllocateBuffer(bufferInfo, VMA_MEMORY_USAGE_GPU_ONLY, tracker.buffer));
                }
                
                /// Large buffer
                {
                    auto& tracker = trackers[2];
                    VkBufferCreateInfo bufferInfo = {};
                    bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
                    bufferInfo.size = LARGE_BUFFER_SIZE / 2;
                    bufferInfo.usage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
                    bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
                    
                    REQUIRE_NOTHROW(tracker.allocation = allocator->AllocateBuffer(bufferInfo, VMA_MEMORY_USAGE_GPU_ONLY, tracker.buffer));
                }
                
                /// Another small buffer
                {
                    auto& tracker = trackers[3];
                    VkBufferCreateInfo bufferInfo = {};
                    bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
                    bufferInfo.size = SMALL_BUFFER_SIZE / 4;
                    bufferInfo.usage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
                    bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
                    
                    REQUIRE_NOTHROW(tracker.allocation = allocator->AllocateBuffer(bufferInfo, VMA_MEMORY_USAGE_GPU_ONLY, tracker.buffer));
                }
                
                /// Another medium buffer
                {
                    auto& tracker = trackers[4];
                    VkBufferCreateInfo bufferInfo = {};
                    bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
                    bufferInfo.size = MEDIUM_BUFFER_SIZE / 4;
                    bufferInfo.usage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
                    bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
                    
                    REQUIRE_NOTHROW(tracker.allocation = allocator->AllocateBuffer(bufferInfo, VMA_MEMORY_USAGE_GPU_ONLY, tracker.buffer));
                }
                
                /// Another large buffer
                {
                    auto& tracker = trackers[5];
                    VkBufferCreateInfo bufferInfo = {};
                    bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
                    bufferInfo.size = LARGE_BUFFER_SIZE / 4;
                    bufferInfo.usage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
                    bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
                    
                    REQUIRE_NOTHROW(tracker.allocation = allocator->AllocateBuffer(bufferInfo, VMA_MEMORY_USAGE_GPU_ONLY, tracker.buffer));
                }
                
                /// Free the allocations
                for (auto& tracker : trackers)
				{
                    REQUIRE_NOTHROW(allocator->DestroyBuffer(tracker.buffer, tracker.allocation));
                    tracker.buffer = VK_NULL_HANDLE;
                    tracker.allocation = nullptr;
                }
            }
        }

        TEST_CASE_METHOD(MemoryAllocatorTestFixture, "Image pool operations", "[VulkanMemory][Pool][Image]")
        {
            SECTION("Image allocations of different sizes")
            {
                /// Allocate images of different sizes
                std::vector<MockAllocationTracker> trackers;
                trackers.resize(3);
                
                /// Small image
                {
                    auto& tracker = trackers[0];
                    VkImageCreateInfo imageInfo = {};
                    imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
                    imageInfo.imageType = VK_IMAGE_TYPE_2D;
                    imageInfo.format = VK_FORMAT_R8G8B8A8_UNORM;
                    imageInfo.extent = {.width = 128,.height = 128,.depth = 1 };
                    imageInfo.mipLevels = 1;
                    imageInfo.arrayLayers = 1;
                    imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
                    imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
                    imageInfo.usage = VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
                    imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
                    imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
                    
                    REQUIRE_NOTHROW(tracker.allocation = allocator->AllocateImage(imageInfo, VMA_MEMORY_USAGE_GPU_ONLY, tracker.image));
                    REQUIRE(tracker.allocation != nullptr);
                    REQUIRE(tracker.image != VK_NULL_HANDLE);
                }
                
                /// Medium image
                {
                    auto& tracker = trackers[1];
                    VkImageCreateInfo imageInfo = {};
                    imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
                    imageInfo.imageType = VK_IMAGE_TYPE_2D;
                    imageInfo.format = VK_FORMAT_R8G8B8A8_UNORM;
                    imageInfo.extent = {.width = 512,.height = 512,.depth = 1 };
                    imageInfo.mipLevels = 1;
                    imageInfo.arrayLayers = 1;
                    imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
                    imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
                    imageInfo.usage = VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
                    imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
                    imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
                    
                    REQUIRE_NOTHROW(tracker.allocation = allocator->AllocateImage(imageInfo, VMA_MEMORY_USAGE_GPU_ONLY, tracker.image));
                    REQUIRE(tracker.allocation != nullptr);
                    REQUIRE(tracker.image != VK_NULL_HANDLE);
                }
                
                /// Large image
                {
                    auto& tracker = trackers[2];
                    VkImageCreateInfo imageInfo = {};
                    imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
                    imageInfo.imageType = VK_IMAGE_TYPE_2D;
                    imageInfo.format = VK_FORMAT_R8G8B8A8_UNORM;
                    imageInfo.extent = {.width = 2048,.height = 2048,.depth = 1 };
                    imageInfo.mipLevels = 1;
                    imageInfo.arrayLayers = 1;
                    imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
                    imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
                    imageInfo.usage = VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
                    imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
                    imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
                    
                    REQUIRE_NOTHROW(tracker.allocation = allocator->AllocateImage(imageInfo, VMA_MEMORY_USAGE_GPU_ONLY, tracker.image));
                    REQUIRE(tracker.allocation != nullptr);
                    REQUIRE(tracker.image != VK_NULL_HANDLE);
                }
                
                /// Free the allocations
                for (auto& tracker : trackers)
				{
                    REQUIRE_NOTHROW(allocator->DestroyImage(tracker.image, tracker.allocation));
                    tracker.image = VK_NULL_HANDLE;
                    tracker.allocation = nullptr;
                }
            }
            
            SECTION("Image and buffer mixed allocations")
            {
                /// Allocate both images and buffers
                std::vector<MockAllocationTracker> trackers;
                trackers.resize(4);
                
                /// Buffer
                {
                    auto& tracker = trackers[0];
                    VkBufferCreateInfo bufferInfo = {};
                    bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
                    bufferInfo.size = MEDIUM_BUFFER_SIZE / 2;
                    bufferInfo.usage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
                    bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
                    
                    REQUIRE_NOTHROW(tracker.allocation = allocator->AllocateBuffer(bufferInfo, VMA_MEMORY_USAGE_GPU_ONLY, tracker.buffer));
                    REQUIRE(tracker.allocation != nullptr);
                    REQUIRE(tracker.buffer != VK_NULL_HANDLE);
                }
                
                /// Image
                {
                    auto& tracker = trackers[1];
                    VkImageCreateInfo imageInfo = {};
                    imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
                    imageInfo.imageType = VK_IMAGE_TYPE_2D;
                    imageInfo.format = VK_FORMAT_R8G8B8A8_UNORM;
                    imageInfo.extent = {.width = 512,.height = 512,.depth = 1 };
                    imageInfo.mipLevels = 1;
                    imageInfo.arrayLayers = 1;
                    imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
                    imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
                    imageInfo.usage = VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
                    imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
                    imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
                    
                    REQUIRE_NOTHROW(tracker.allocation = allocator->AllocateImage(imageInfo, VMA_MEMORY_USAGE_GPU_ONLY, tracker.image));
                    REQUIRE(tracker.allocation != nullptr);
                    REQUIRE(tracker.image != VK_NULL_HANDLE);
                }
                
                /// Another buffer
                {
                    auto& tracker = trackers[2];
                    VkBufferCreateInfo bufferInfo = {};
                    bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
                    bufferInfo.size = SMALL_BUFFER_SIZE / 2;
                    bufferInfo.usage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
                    bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
                    
                    REQUIRE_NOTHROW(tracker.allocation = allocator->AllocateBuffer(bufferInfo, VMA_MEMORY_USAGE_GPU_ONLY, tracker.buffer));
                    REQUIRE(tracker.allocation != nullptr);
                    REQUIRE(tracker.buffer != VK_NULL_HANDLE);
                }
                
                /// Another image
                {
                    auto& tracker = trackers[3];
                    VkImageCreateInfo imageInfo = {};
                    imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
                    imageInfo.imageType = VK_IMAGE_TYPE_2D;
                    imageInfo.format = VK_FORMAT_R8G8B8A8_UNORM;
                    imageInfo.extent = { 256, 256, 1 };
                    imageInfo.mipLevels = 1;
                    imageInfo.arrayLayers = 1;
                    imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
                    imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
                    imageInfo.usage = VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
                    imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
                    imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
                    
                    REQUIRE_NOTHROW(tracker.allocation = allocator->AllocateImage(imageInfo, VMA_MEMORY_USAGE_GPU_ONLY, tracker.image));
                    REQUIRE(tracker.allocation != nullptr);
                    REQUIRE(tracker.image != VK_NULL_HANDLE);
                }
                
                /// Free the allocations
                for (auto& tracker : trackers)
				{
                    if (tracker.buffer != VK_NULL_HANDLE)
					{
                        REQUIRE_NOTHROW(allocator->DestroyBuffer(tracker.buffer, tracker.allocation));
                        tracker.buffer = VK_NULL_HANDLE;
                    }
                    else if (tracker.image != VK_NULL_HANDLE)
					{
                        REQUIRE_NOTHROW(allocator->DestroyImage(tracker.image, tracker.allocation));
                        tracker.image = VK_NULL_HANDLE;
                    }

                    tracker.allocation = nullptr;
                }
            }
        }
    }

}

/// -------------------------------------------------------
