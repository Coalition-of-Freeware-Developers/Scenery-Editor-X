/**
* -------------------------------------------------------
* Scenery Editor X - Unit Tests
* -------------------------------------------------------
* Copyright (c) 2025 Thomas Ray 
* Copyright (c) 2025 Coalition of Freeware Developers
* -------------------------------------------------------
* MemoryAllocatorTest.cpp
* -------------------------------------------------------
* Tests for the Vulkan Memory Allocator implementation
* -------------------------------------------------------
*/
#include <algorithm>
#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>
#include <cmath>
#include <functional>
#include <SceneryEditorX/core/application_data.h>
#include <SceneryEditorX/renderer/render_context.h>
#include <SceneryEditorX/renderer/vulkan/vk_allocator.h>
#include <thread>
#include <vector>

namespace SceneryEditorX
{
    namespace Tests
    {
        /// Helper class to set up and tear down Vulkan resources needed for memory allocator tests
        class VulkanTestEnvironment
        {
        public:
            VulkanTestEnvironment()
            {
                initializeVulkan();
            }

            ~VulkanTestEnvironment()
            {
                cleanup();
            }

            [[nodiscard]] Ref<VulkanDevice> GetDevice() const { return device; }
            [[nodiscard]] Ref<MemoryAllocator> GetAllocator() const { return allocator; }

        private:
            void initializeVulkan()
            {
                /// This simulates the setup that would normally be done in RenderContext::Init()
                try
                {
                    /// Initialize Vulkan instance
                    renderContext = CreateRef<RenderContext>();
                    renderContext->Init();
                    
                    /// Get the device
                    device = renderContext->GetLogicDevice();
                    
                    /// Create memory allocator
                    allocator = CreateRef<MemoryAllocator>("TestAllocator");
                    allocator->Init(device, VK_API_VERSION_1_2);
                    
                    REQUIRE(allocator->GetAllocator() != nullptr);
                } 
                catch (const std::exception& e)
                {
                    FAIL("Failed to initialize Vulkan test environment: " << e.what());
                }
            }

            void cleanup()
            {
                if (allocator)
                    allocator->Shutdown();

                /// Release resources
                device = nullptr;
                renderContext = nullptr;
            }

            Ref<RenderContext> renderContext;
            Ref<VulkanDevice> device;
            Ref<MemoryAllocator> allocator;
        };

        /// Mock allocation tracking for tests
        class MockAllocationTracker
        {
        public:
            MockAllocationTracker() : allocation(nullptr), buffer(VK_NULL_HANDLE), image(VK_NULL_HANDLE) {}
            
            ~MockAllocationTracker()
            {
                cleanup();
            }
            
            void cleanup()
            {
                if (!allocation && buffer == VK_NULL_HANDLE && image == VK_NULL_HANDLE)
                    return;
                
                if (MemoryAllocatorTestFixture::environment && 
                    MemoryAllocatorTestFixture::environment->GetAllocator())
                {
                    auto allocator = MemoryAllocatorTestFixture::environment->GetAllocator();
                    
                    if (buffer != VK_NULL_HANDLE && allocation != nullptr)
                    {
                        allocator->DestroyBuffer(buffer, allocation);
                        buffer = VK_NULL_HANDLE;
                        allocation = nullptr;
                    }
                    
                    if (image != VK_NULL_HANDLE && allocation != nullptr)
                    {
                        allocator->DestroyImage(image, allocation);
                        image = VK_NULL_HANDLE;
                        allocation = nullptr;
                    }
                    
                    if (allocation != nullptr)
                    {
                        allocator->Free(allocation);
                        allocation = nullptr;
                    }
                }
            }
            
            VmaAllocation allocation;
            VkBuffer buffer;
            VkImage image;
        };

        /// Test fixture with Vulkan setup/teardown
        class MemoryAllocatorTestFixture
        {
        public:
            MemoryAllocatorTestFixture() 
            {
                /// Only set up Vulkan once
                if (!environment)
                    environment = std::make_shared<VulkanTestEnvironment>();

                allocator = environment->GetAllocator();
                device = environment->GetDevice();
            }
            
            ~MemoryAllocatorTestFixture()
            {
                /// Clean up any tracking objects
                for (auto& tracker : allocationTrackers)
                    tracker.cleanup();

                allocationTrackers.clear();
            }
            
            /// Helper to track allocations and ensure cleanup
            MockAllocationTracker& trackAllocation()
            {
                allocationTrackers.emplace_back();
                return allocationTrackers.back();
            }
            
            static std::shared_ptr<VulkanTestEnvironment> environment;
            Ref<MemoryAllocator> allocator;
            Ref<VulkanDevice> device;
            std::vector<MockAllocationTracker> allocationTrackers;
        };
        
        /// Initialize the static environment
        std::shared_ptr<VulkanTestEnvironment> MemoryAllocatorTestFixture::environment = nullptr;

        /// Basic functionality tests
        TEST_CASE_METHOD(MemoryAllocatorTestFixture, "MemoryAllocator initialization and shutdown", "[VulkanMemory][Init]")
        {
            SECTION("Basic initialization")
            {
                REQUIRE(allocator != nullptr);
                REQUIRE(allocator->GetAllocator() != nullptr);
            }
            
            SECTION("Create new allocator and shutdown")
            {
                /// Create a new allocator instance
                auto newAllocator = CreateRef<MemoryAllocator>("TestAllocator2");
                REQUIRE_NOTHROW(newAllocator->Init(device, VK_API_VERSION_1_2));
                REQUIRE(newAllocator->GetAllocator() != nullptr);
                
                /// Shutdown should clean up properly
                REQUIRE_NOTHROW(newAllocator->Shutdown());
            }
        }

        TEST_CASE_METHOD(MemoryAllocatorTestFixture, "Buffer allocation and deallocation", "[VulkanMemory][Buffer]")
        {
            SECTION("Single small buffer allocation")
            {
                auto& tracker = trackAllocation();
                
                /// Create a small buffer
                VkBufferCreateInfo bufferInfo = {};
                bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
                bufferInfo.size = 1024; /// 1KB
                bufferInfo.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
                bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
                
                REQUIRE_NOTHROW(tracker.allocation = allocator->AllocateBuffer(bufferInfo, VMA_MEMORY_USAGE_AUTO, tracker.buffer));
                REQUIRE(tracker.allocation != nullptr);
                REQUIRE(tracker.buffer != VK_NULL_HANDLE);
                
                /// Get stats to verify allocation
                auto stats = allocator->GetStats();
                REQUIRE(stats.allocationCount >= 1);
                REQUIRE(stats.usedBytes >= 1024);
                
                /// Free the buffer
                REQUIRE_NOTHROW(allocator->DestroyBuffer(tracker.buffer, tracker.allocation));
                tracker.buffer = VK_NULL_HANDLE;
                tracker.allocation = nullptr;
            }
            
            SECTION("Multiple buffer allocations of different sizes")
            {
                const std::vector<VkDeviceSize> sizes = {
                    256,            /// Tiny
                    4 * 1024,       /// 4KB
                    256 * 1024,     /// 256KB (SMALL_BUFFER_SIZE)
                    1024 * 1024,    /// 1MB (MEDIUM_BUFFER_SIZE)
                    16 * 1024 * 1024 /// 16MB (LARGE_BUFFER_SIZE)
                };
                
                std::vector<MockAllocationTracker> buffers;
                
                /// Allocate buffers of different sizes
                for (const auto& size : sizes)
                {
                    auto& tracker = trackAllocation();
                    
                    VkBufferCreateInfo bufferInfo = {};
                    bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
                    bufferInfo.size = size;
                    bufferInfo.usage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
                    bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
                    
                    REQUIRE_NOTHROW(tracker.allocation = allocator->AllocateBuffer(bufferInfo, VMA_MEMORY_USAGE_AUTO, tracker.buffer));
                    REQUIRE(tracker.allocation != nullptr);
                    REQUIRE(tracker.buffer != VK_NULL_HANDLE);
                    
                    /// Verify allocation is tracked
                    REQUIRE(allocator->ContainsAllocation(tracker.allocation));
                    
                    buffers.push_back(tracker);
                }
                
                /// Get stats
                auto stats = allocator->GetStats();
                REQUIRE(stats.allocationCount >= sizes.size());
                
                /// Free buffers in reverse order
                for (auto it = buffers.rbegin(); it != buffers.rend(); ++it)
                {
                    REQUIRE_NOTHROW(allocator->DestroyBuffer(it->buffer, it->allocation));
                    it->buffer = VK_NULL_HANDLE;
                    it->allocation = nullptr;
                }
            }
            
            SECTION("Batch buffer allocation")
            {
                const std::vector<VkDeviceSize> sizes = { 1024, 2048, 4096, 8192 };
                
                std::vector<MemoryAllocator::BatchBufferAllocation> batchAllocations;
                
                /// Perform batch allocation
                REQUIRE_NOTHROW(batchAllocations = allocator->AllocateBufferBatch(
                    sizes, 
                    VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
                    VMA_MEMORY_USAGE_GPU_ONLY
                ));
                
                REQUIRE(batchAllocations.size() == sizes.size());
                
                /// Verify allocations
                for (size_t i = 0; i < batchAllocations.size(); i++)
                {
                    REQUIRE(batchAllocations[i].buffer != VK_NULL_HANDLE);
                    REQUIRE(batchAllocations[i].allocation != nullptr);
                    REQUIRE(batchAllocations[i].size >= sizes[i]);
                    
                    /// Verify allocation is tracked
                    REQUIRE(allocator->ContainsAllocation(batchAllocations[i].allocation));
                }
                
                /// Free batch
                REQUIRE_NOTHROW(allocator->FreeBufferBatch(batchAllocations));
            }
        }

        TEST_CASE_METHOD(MemoryAllocatorTestFixture, "Image allocation and deallocation", "[VulkanMemory][Image]")
        {
            SECTION("Basic image allocation")
            {
                auto& tracker = trackAllocation();
                
                /// Create a 2D image
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
                
                VkDeviceSize allocatedSize = 0;
                REQUIRE_NOTHROW(tracker.allocation = allocator->AllocateImage(
                    imageInfo, 
                    VMA_MEMORY_USAGE_GPU_ONLY, 
                    tracker.image,
                    &allocatedSize
                ));
                
                REQUIRE(tracker.allocation != nullptr);
                REQUIRE(tracker.image != VK_NULL_HANDLE);
                REQUIRE(allocatedSize > 0);
                
                /// Cleanup
                REQUIRE_NOTHROW(allocator->DestroyImage(tracker.image, tracker.allocation));
                tracker.image = VK_NULL_HANDLE;
                tracker.allocation = nullptr;
            }
            
            SECTION("Multiple image allocations with different formats")
            {
                const std::vector<VkFormat> formats = {
                    VK_FORMAT_R8G8B8A8_UNORM,
                    VK_FORMAT_B8G8R8A8_UNORM,
                    VK_FORMAT_R32G32B32A32_SFLOAT,
                    VK_FORMAT_D24_UNORM_S8_UINT
                };
                
                for (const auto& format : formats)
                {
                    auto& tracker = trackAllocation();
                    
                    VkImageCreateInfo imageInfo = {};
                    imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
                    imageInfo.imageType = VK_IMAGE_TYPE_2D;
                    imageInfo.format = format;
                    imageInfo.extent = { 256, 256, 1 };
                    imageInfo.mipLevels = 1;
                    imageInfo.arrayLayers = 1;
                    imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
                    imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
                    imageInfo.usage = VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
                    imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
                    imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
                    
                    /// Add depth/stencil specific usage if needed
                    if (format == VK_FORMAT_D24_UNORM_S8_UINT)
                        imageInfo.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;

                    REQUIRE_NOTHROW(tracker.allocation = allocator->AllocateImage(imageInfo, VMA_MEMORY_USAGE_GPU_ONLY, tracker.image));
                    
                    REQUIRE(tracker.allocation != nullptr);
                    REQUIRE(tracker.image != VK_NULL_HANDLE);
                    
                    /// Verify allocation is tracked
                    REQUIRE(allocator->ContainsAllocation(tracker.allocation));
                }
            }
        }

        TEST_CASE_METHOD(MemoryAllocatorTestFixture, "Memory mapping operations", "[VulkanMemory][Mapping]")
        {
            SECTION("Map and unmap buffer memory")
            {
                auto& tracker = trackAllocation();
                
                /// Create a mappable buffer
                VkBufferCreateInfo bufferInfo = {};
                bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
                bufferInfo.size = 1024;
                bufferInfo.usage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
                bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
                
                REQUIRE_NOTHROW(tracker.allocation = allocator->AllocateBuffer(
                    bufferInfo, 
                    VMA_MEMORY_USAGE_CPU_TO_GPU, 
                    tracker.buffer
                ));
                
                REQUIRE(tracker.allocation != nullptr);
                REQUIRE(tracker.buffer != VK_NULL_HANDLE);
                
                /// Map the memory
                uint32_t* mappedData = nullptr;
                REQUIRE_NOTHROW(mappedData = allocator->MapMemory<uint32_t>(tracker.allocation));
                REQUIRE(mappedData != nullptr);
                
                /// Write some data
                const uint32_t testPattern = 0xDEADBEEF;
                for (size_t i = 0; i < 256; i++)
                    mappedData[i] = testPattern;

                /// Unmap
                REQUIRE_NOTHROW(MemoryAllocator::UnmapMemory(tracker.allocation));
                
                /// Map again to verify data
                REQUIRE_NOTHROW(mappedData = allocator->MapMemory<uint32_t>(tracker.allocation));
                REQUIRE(mappedData != nullptr);
                
                /// Verify data
                for (size_t i = 0; i < 256; i++)
                    REQUIRE(mappedData[i] == testPattern);

                /// Unmap
                REQUIRE_NOTHROW(MemoryAllocator::UnmapMemory(tracker.allocation));
            }
        }

        TEST_CASE_METHOD(MemoryAllocatorTestFixture, "Memory statistics", "[VulkanMemory][Stats]")
        {
            SECTION("Get initial statistics")
            {
                auto stats = allocator->GetStats();
                
                /// Check that stats have reasonable values
                REQUIRE(stats.totalBytes >= 0);
                REQUIRE(stats.usedBytes >= 0);
                REQUIRE(stats.fragmentationRatio >= 0.0f);
                REQUIRE(stats.fragmentationRatio <= 1.0f);
            }
            
            SECTION("Statistics after allocations")
            {
                auto initialStats = allocator->GetStats();
                
                /// Allocate a series of buffers
                const int numBuffers = 5;
                const VkDeviceSize bufferSize = 1024 * 1024; /// 1MB
                
                std::vector<MockAllocationTracker> buffers(numBuffers);
                
                for (auto& tracker : buffers) {
                    VkBufferCreateInfo bufferInfo = {};
                    bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
                    bufferInfo.size = bufferSize;
                    bufferInfo.usage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
                    bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
                    
                    tracker.allocation = allocator->AllocateBuffer(bufferInfo, VMA_MEMORY_USAGE_GPU_ONLY, tracker.buffer);
                }
                
                /// Get updated stats
                auto updatedStats = allocator->GetStats();
                
                /// Verify allocation count increased
                REQUIRE(updatedStats.allocationCount >= initialStats.allocationCount + numBuffers);
                
                /// Verify used bytes increased
                REQUIRE(updatedStats.usedBytes >= initialStats.usedBytes + (bufferSize * numBuffers));
                
                /// Clean up
                for (auto& tracker : buffers)
                    allocator->DestroyBuffer(tracker.buffer, tracker.allocation);
            }
            
            SECTION("Memory budget")
            {
                auto budget = allocator->GetMemoryBudget();
                
                /// Check budget values
                REQUIRE(budget.totalBytes > 0);
                REQUIRE(budget.usedBytes >= 0);
                REQUIRE(budget.usagePercentage >= 0.0f);
                REQUIRE(budget.usagePercentage <= 1.0f);
                
                /// Set a custom warning threshold
                const float newThreshold = 0.95f;
                REQUIRE_NOTHROW(allocator->SetMemoryUsageWarningThreshold(newThreshold));
            }
            
            SECTION("Reset statistics")
            {
                /// Make some allocations
                auto& tracker = trackAllocation();
                VkBufferCreateInfo bufferInfo = {};
                bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
                bufferInfo.size = 1024 * 1024;
                bufferInfo.usage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
                bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
                
                tracker.allocation = allocator->AllocateBuffer(bufferInfo, VMA_MEMORY_USAGE_GPU_ONLY, tracker.buffer);
                
                /// Reset stats
                REQUIRE_NOTHROW(allocator->ResetStats());
                
                /// The current allocation should still be accounted for
                auto stats = allocator->GetStats();
                REQUIRE(stats.allocationCount >= 1);
                REQUIRE(stats.usedBytes >= bufferInfo.size);
            }
            
            SECTION("Print detailed stats")
                REQUIRE_NOTHROW(allocator->PrintDetailedStats()); /// This is mainly for coverage, as it's a debug utility
        }

        TEST_CASE_METHOD(MemoryAllocatorTestFixture, "Allocation strategies", "[VulkanMemory][Strategy]")
        {
            SECTION("Default strategy")
            {
                auto& tracker = trackAllocation();
                
                VkBufferCreateInfo bufferInfo = {};
                bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
                bufferInfo.size = 1024;
                bufferInfo.usage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
                bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
                
                REQUIRE_NOTHROW(tracker.allocation = allocator->AllocateBuffer(bufferInfo, VMA_MEMORY_USAGE_GPU_ONLY, tracker.buffer));
                REQUIRE(tracker.allocation != nullptr);
            }
            
            SECTION("Speed optimized strategy")
            {
                /// Set speed optimized strategy
                REQUIRE_NOTHROW(allocator->SetAllocationStrategy(MemoryAllocator::AllocationStrategy::SpeedOptimized));
                
                auto& tracker = trackAllocation();
                
                VkBufferCreateInfo bufferInfo = {};
                bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
                bufferInfo.size = 1024;
                bufferInfo.usage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
                bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
                
                REQUIRE_NOTHROW(tracker.allocation = allocator->AllocateBuffer(bufferInfo, VMA_MEMORY_USAGE_GPU_ONLY, tracker.buffer));
                REQUIRE(tracker.allocation != nullptr);
            }
            
            SECTION("Memory optimized strategy")
            {
                /// Set memory optimized strategy
                REQUIRE_NOTHROW(allocator->SetAllocationStrategy(MemoryAllocator::AllocationStrategy::MemoryOptimized));
                
                auto& tracker = trackAllocation();
                
                VkBufferCreateInfo bufferInfo = {};
                bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
                bufferInfo.size = 1024;
                bufferInfo.usage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
                bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
                
                REQUIRE_NOTHROW(tracker.allocation = allocator->AllocateBuffer(bufferInfo, VMA_MEMORY_USAGE_GPU_ONLY, tracker.buffer));
                REQUIRE(tracker.allocation != nullptr);
            }
        }

        TEST_CASE_METHOD(MemoryAllocatorTestFixture, "Custom buffer settings", "[VulkanMemory][CustomBuffer]")
        {
            SECTION("Get default custom buffer size")
            {
                const auto size = MemoryAllocator::GetCustomBufferSize();
                // DEFAULT_CUSTOM_BUFFER_SIZE is defined in vk_allocator.h as 16MB (1024 * 16 * 1024)
                REQUIRE(size == 16 * 1024 * 1024);
            }
            
            SECTION("Set custom buffer size")
            {
                const VkDeviceSize newSize = 32 * 1024 * 1024; /// 32MB
                
                /// This may fail if device doesn't support it, so we don't require success
                bool result = MemoryAllocator::SetCustomBufferSize(newSize, *device);
                if (result)
                    REQUIRE(MemoryAllocator::GetCustomBufferSize() == newSize);
            }
            
            SECTION("Set custom buffer alignment")
            {
                /// Should align to a power of 2
                const VkDeviceSize alignment = 256;
                REQUIRE_NOTHROW(allocator->SetBufferAlignment(alignment));
                
                /// Test with non-power-of-2 (should be rounded up)
                const VkDeviceSize nonPowerOf2 = 100;
                REQUIRE_NOTHROW(allocator->SetBufferAlignment(nonPowerOf2));
            }
        }

        TEST_CASE_METHOD(MemoryAllocatorTestFixture, "Defragmentation", "[VulkanMemory][Defrag]")
        {
            /// Note: For defragmentation tests, we need to create a lot of small allocations
            /// and then free some of them to create fragmentation. This can be time-consuming,
            /// so we'll keep this test simple.
            
            SECTION("Basic defragmentation operation")
            {
                /// Create a bunch of small buffers
                const int numBuffers = 20;
                const VkDeviceSize bufferSize = 4 * 1024; /// 4KB
                
                std::vector<MockAllocationTracker> buffers(numBuffers);
                
                /// Allocate buffers
                for (auto& tracker : buffers)
                {
                    VkBufferCreateInfo bufferInfo = {};
                    bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
                    bufferInfo.size = bufferSize;
                    bufferInfo.usage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
                    bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
                    
                    tracker.allocation = allocator->AllocateBuffer(bufferInfo, VMA_MEMORY_USAGE_GPU_ONLY, tracker.buffer);
                }
                
                /// Free every other buffer to create fragmentation
                for (size_t i = 0; i < buffers.size(); i += 2)
                {
                    allocator->DestroyBuffer(buffers[i].buffer, buffers[i].allocation);
                    buffers[i].buffer = VK_NULL_HANDLE;
                    buffers[i].allocation = nullptr;
                }
                
                /// Mark remaining buffers for defragmentation
                REQUIRE_NOTHROW(allocator->BeginDefragmentation());
                
                for (auto& tracker : buffers)
                {
                    if (tracker.allocation != nullptr)
                        REQUIRE_NOTHROW(allocator->MarkForDefragmentation(tracker.allocation));
                }
                
                /// End defragmentation
                REQUIRE_NOTHROW(allocator->EndDefragmentation());
                
                /// Clean up remaining buffers
                for (auto& tracker : buffers)
                {
                    if (tracker.allocation != nullptr)
                    {
                        allocator->DestroyBuffer(tracker.buffer, tracker.allocation);
                        tracker.buffer = VK_NULL_HANDLE;
                        tracker.allocation = nullptr;
                    }
                }
            }
        }

        TEST_CASE_METHOD(MemoryAllocatorTestFixture, "Thread safety", "[VulkanMemory][ThreadSafety]")
        {
            /// This test checks if the allocator can be used from multiple threads
            SECTION("Concurrent buffer allocations")
            {
                const int numThreads = 4;
                const int allocationsPerThread = 5;
                
                std::vector<std::thread> threads;
                std::vector<std::vector<MockAllocationTracker>> threadAllocations(numThreads);
                
                for (int t = 0; t < numThreads; t++)
                {
                    threads.emplace_back([t, allocationsPerThread, this, &threadAllocations]()
                    {
                        threadAllocations[t].resize(allocationsPerThread);
                        
                        for (int i = 0; i < allocationsPerThread; i++)
                        {
                            auto& tracker = threadAllocations[t][i];
                            
                            VkBufferCreateInfo bufferInfo = {};
                            bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
                            bufferInfo.size = 1024 * (i + 1); /// Different sizes
                            bufferInfo.usage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
                            bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
                            
                            tracker.allocation = allocator->AllocateBuffer(bufferInfo, VMA_MEMORY_USAGE_GPU_ONLY, tracker.buffer);
                        }
                    });
                }
                
                /// Wait for all threads to complete
                for (auto& thread : threads)
                    thread.join();

                /// Verify allocations
                for (int t = 0; t < numThreads; t++)
                {
                    for (int i = 0; i < allocationsPerThread; i++)
                    {
                        auto& tracker = threadAllocations[t][i];
                        REQUIRE(tracker.allocation != nullptr);
                        REQUIRE(tracker.buffer != VK_NULL_HANDLE);
                        
                        /// Add to main tracker for cleanup
                        allocationTrackers.push_back(tracker);
                    }
                }
            }
            
            SECTION("Concurrent batch allocations")
            {
                const int numThreads = 4;
                
                std::vector<std::thread> threads;
                std::vector<std::vector<MemoryAllocator::BatchBufferAllocation>> threadBatches(numThreads);
                
                for (int t = 0; t < numThreads; t++)
                    threads.emplace_back([t, this, &threadBatches]()
                    {
                        std::vector<VkDeviceSize> sizes = { 1024, 2048, 4096, 8192 };
                        
                        threadBatches[t] = allocator->AllocateBufferBatch(sizes,
                        VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, VMA_MEMORY_USAGE_GPU_ONLY);
                    });

                /// Wait for all threads to complete
                for (auto& thread : threads)
                    thread.join();

                /// Verify allocations
                for (int t = 0; t < numThreads; t++)
                {
                    REQUIRE(threadBatches[t].size() == 4);
                    
                    for (auto& batchAlloc : threadBatches[t])
                    {
                        REQUIRE(batchAlloc.allocation != nullptr);
                        REQUIRE(batchAlloc.buffer != VK_NULL_HANDLE);
                    }
                    
                    /// Free the batch
                    allocator->FreeBufferBatch(threadBatches[t]);
                }
            }
        }
    }  // namespace Tests
}  // namespace SceneryEditorX

/// -------------------------------------------------------
