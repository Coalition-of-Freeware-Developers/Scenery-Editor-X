/**
* -------------------------------------------------------
* Scenery Editor X - Unit Tests
* -------------------------------------------------------
* Copyright (c) 2025 Thomas Ray
* Copyright (c) 2025 Coalition of Freeware Developers
* -------------------------------------------------------
* MemoryAllocatorStressTest.cpp
* -------------------------------------------------------
* Stress tests for the Vulkan Memory Allocator implementation
* -------------------------------------------------------
*/
#include <algorithm>
#include <catch2/benchmark/catch_benchmark.hpp>
#include <catch2/catch_test_macros.hpp>
#include <chrono>
#include <random>
#include <thread>
#include "MemoryAllocatorTest.h"

/// -------------------------------------------------------

namespace SceneryEditorX
{
    namespace Tests
    {
        TEST_CASE_METHOD(MemoryAllocatorTestFixture, "Memory allocator stress test", "[VulkanMemory][Stress]")
        {
            /// Skip stress tests in debug mode as they can be very slow
            #ifndef SEDX_DEBUG
                SKIP("Skipping stress test in debug mode");
            #endif

            SECTION("Repeated allocation and deallocation")
            {
                constexpr int iterations = 100;

                /// Store allocations from each iteration
                std::vector<std::vector<MockAllocationTracker>> allIterationAllocations(iterations);

                for (int iter = 0; iter < iterations; iter++)
				{
                    constexpr int allocationsPerIteration = 50;
                    auto& iterationAllocations = allIterationAllocations[iter];
                    iterationAllocations.resize(allocationsPerIteration);

                    /// Allocate buffers of random sizes
                    std::mt19937 rng(static_cast<unsigned int>(iter));
                    std::uniform_int_distribution<VkDeviceSize> sizeDistribution(1024, 1024 * 1024);

                    for (int i = 0; i < allocationsPerIteration; i++)
					{
                        auto& tracker = iterationAllocations[i];

                        VkBufferCreateInfo bufferInfo = {};
                        bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
                        bufferInfo.size = sizeDistribution(rng);
                        bufferInfo.usage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
                        bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

                        REQUIRE_NOTHROW(tracker.allocation = allocator->AllocateBuffer(bufferInfo, VMA_MEMORY_USAGE_GPU_ONLY, tracker.buffer));
                        REQUIRE(tracker.allocation != nullptr);
                        REQUIRE(tracker.buffer != VK_NULL_HANDLE);
                    }

                    /// Free half of the allocations
                    for (int i = 0; i < allocationsPerIteration; i += 2)
					{
                        auto& tracker = iterationAllocations[i];
                        REQUIRE_NOTHROW(allocator->DestroyBuffer(tracker.buffer, tracker.allocation));
                        tracker.buffer = VK_NULL_HANDLE;
                        tracker.allocation = nullptr;
                    }

                    /// Make some new allocations to replace the freed ones
                    for (int i = 0; i < allocationsPerIteration; i += 2)
					{
                        auto& tracker = iterationAllocations[i];

                        VkBufferCreateInfo bufferInfo = {};
                        bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
                        bufferInfo.size = sizeDistribution(rng);
                        bufferInfo.usage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
                        bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

                        REQUIRE_NOTHROW(tracker.allocation = allocator->AllocateBuffer(bufferInfo, VMA_MEMORY_USAGE_GPU_ONLY, tracker.buffer));
                        REQUIRE(tracker.allocation != nullptr);
                        REQUIRE(tracker.buffer != VK_NULL_HANDLE);
                    }
                }

                /// Clean up all allocations
                for (auto& iterationAllocations : allIterationAllocations)
				{
                    for (auto& tracker : iterationAllocations) {
                        if (tracker.allocation != nullptr)
						{
                            REQUIRE_NOTHROW(allocator->DestroyBuffer(tracker.buffer, tracker.allocation));
                            tracker.buffer = VK_NULL_HANDLE;
                            tracker.allocation = nullptr;
                        }
                    }
                }
            }

            SECTION("Fragmentation test with alternating sizes")
            {
                constexpr int allocations = 200;

                /// Use alternating small and large allocations to induce fragmentation
                std::vector<MockAllocationTracker> trackers(allocations);

                for (int i = 0; i < allocations; i++)
				{
                    auto& tracker = trackers[i];

                    /// Alternate between small and large allocations
                    VkDeviceSize size = (i % 2 == 0) ? 1024 : 1024 * 1024;

                    VkBufferCreateInfo bufferInfo = {};
                    bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
                    bufferInfo.size = size;
                    bufferInfo.usage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
                    bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

                    REQUIRE_NOTHROW(tracker.allocation = allocator->AllocateBuffer(bufferInfo, VMA_MEMORY_USAGE_GPU_ONLY, tracker.buffer));
                }

                /// Free small allocations to create gaps
                for (int i = 0; i < allocations; i++)
				{
                    if (i % 2 == 0)
					{
                        auto& tracker = trackers[i];
                        REQUIRE_NOTHROW(allocator->DestroyBuffer(tracker.buffer, tracker.allocation));
                        tracker.buffer = VK_NULL_HANDLE;
                        tracker.allocation = nullptr;
                    }
                }

                /// Get fragmentation stats
                auto stats = allocator->GetStats();
                REQUIRE(stats.fragmentationRatio >= 0.0f);

                /// Try to defragment
                REQUIRE_NOTHROW(allocator->BeginDefragmentation());

                for (auto& tracker : trackers)
				{
                    if (tracker.allocation != nullptr)
					{
                        REQUIRE_NOTHROW(allocator->MarkForDefragmentation(tracker.allocation));
                    }
                }

                REQUIRE_NOTHROW(allocator->EndDefragmentation());

                /// Get stats after defragmentation
                auto defragStats = allocator->GetStats();

                /// Clean up remaining allocations
                for (auto& tracker : trackers)
				{
                    if (tracker.allocation != nullptr)
					{
                        REQUIRE_NOTHROW(allocator->DestroyBuffer(tracker.buffer, tracker.allocation));
                        tracker.buffer = VK_NULL_HANDLE;
                        tracker.allocation = nullptr;
                    }
                }
            }
        }

		/// -------------------------------------------------------

        TEST_CASE_METHOD(MemoryAllocatorTestFixture, "Memory allocator benchmarks", "[VulkanMemory][Benchmark]")
        {
            /// Skip benchmarks in debug mode
            #ifndef NDEBUG
                SKIP("Skipping benchmarks in debug mode");
            #endif

            BENCHMARK("Buffer allocation 4KB")
            {
                MockAllocationTracker tracker;

                VkBufferCreateInfo bufferInfo = {};
                bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
                bufferInfo.size = 4 * 1024;
                bufferInfo.usage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
                bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

                tracker.allocation = allocator->AllocateBuffer(bufferInfo, VMA_MEMORY_USAGE_GPU_ONLY, tracker.buffer);

                allocator->DestroyBuffer(tracker.buffer, tracker.allocation);
                return tracker.buffer != VK_NULL_HANDLE; /// Just to avoid optimization
            };

            BENCHMARK("Buffer allocation 1MB")
            {
                MockAllocationTracker tracker;

                VkBufferCreateInfo bufferInfo = {};
                bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
                bufferInfo.size = 1024 * 1024;
                bufferInfo.usage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
                bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

                tracker.allocation = allocator->AllocateBuffer(bufferInfo, VMA_MEMORY_USAGE_GPU_ONLY, tracker.buffer);

                allocator->DestroyBuffer(tracker.buffer, tracker.allocation);
                return tracker.buffer != VK_NULL_HANDLE;
            };

            BENCHMARK("Memory mapping operation")
            {
                MockAllocationTracker tracker;

                VkBufferCreateInfo bufferInfo = {};
                bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
                bufferInfo.size = 64 * 1024;
                bufferInfo.usage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
                bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

                tracker.allocation = allocator->AllocateBuffer(bufferInfo, VMA_MEMORY_USAGE_CPU_TO_GPU, tracker.buffer);

                uint32_t* mappedData = allocator->MapMemory<uint32_t>(tracker.allocation);
                mappedData[0] = 42; /// Make sure it's actually mapped
                // Unmapping is not required or handled differently

                allocator->DestroyBuffer(tracker.buffer, tracker.allocation);
                return mappedData != nullptr;
            };

            BENCHMARK("Batch allocation (10 buffers)")
            {
                const std::vector<VkDeviceSize> sizes(10, 4 * 1024); // 10 buffers of 4KB each

                const auto allocations = allocator->AllocateBufferBatch(
                    static_cast<const std::vector<VkDeviceSize>&>(sizes),
                    static_cast<BufferUsageFlags>(VK_BUFFER_USAGE_VERTEX_BUFFER_BIT),
                    static_cast<VmaMemoryUsage>(VMA_MEMORY_USAGE_GPU_ONLY)
                );

                allocator->FreeBufferBatch(allocations);
                return !allocations.empty();
            };

            BENCHMARK("Memory strategy switching")
            {
                /// Test how quickly we can change strategies and allocate
                allocator->SetAllocationStrategy(MemoryAllocator::AllocationStrategy::SpeedOptimized);

                MockAllocationTracker tracker;

                VkBufferCreateInfo bufferInfo = {};
                bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
                bufferInfo.size = 4 * 1024;
                bufferInfo.usage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
                bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

                tracker.allocation = allocator->AllocateBuffer(bufferInfo, VMA_MEMORY_USAGE_GPU_ONLY, tracker.buffer);

                allocator->DestroyBuffer(tracker.buffer, tracker.allocation);

                /// Switch back
                allocator->SetAllocationStrategy(MemoryAllocator::AllocationStrategy::Default);
                return tracker.buffer != VK_NULL_HANDLE;
            };
        }

        TEST_CASE_METHOD(MemoryAllocatorTestFixture, "Memory allocator error handling", "[VulkanMemory][Errors]")
        {
            SECTION("Handle invalid allocation size")
            {
                auto& tracker = trackAllocation();

                VkBufferCreateInfo bufferInfo = {};
                bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
                bufferInfo.size = 0; /// Invalid size
                bufferInfo.usage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
                bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

                /// This might fail, but it shouldn't crash
                tracker.allocation = allocator->AllocateBuffer(bufferInfo, VMA_MEMORY_USAGE_GPU_ONLY, tracker.buffer);

                /// Don't require success - just that it handled the error gracefully
                if (tracker.allocation != nullptr)
                    allocator->DestroyBuffer(tracker.buffer, tracker.allocation);
            }

            SECTION("Handle invalid memory usage")
            {
                auto& tracker = trackAllocation();

                VkBufferCreateInfo bufferInfo = {};
                bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
                bufferInfo.size = 1024;
                bufferInfo.usage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
                bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

                /// This might fail depending on implementation, but it shouldn't crash
                /// Using an extremely high memory usage value that's likely invalid
                VmaMemoryUsage invalidUsage = static_cast<VmaMemoryUsage>(0xFF);
                tracker.allocation = allocator->AllocateBuffer(bufferInfo, invalidUsage, tracker.buffer);

                if (tracker.allocation != nullptr)
                    allocator->DestroyBuffer(tracker.buffer, tracker.allocation);
            }

            SECTION("Handle null allocation in Free")
            {
                /// Should not crash when freeing a null allocation
                REQUIRE_NOTHROW(allocator->Free(nullptr));
            }

            SECTION("Handle null buffer in DestroyBuffer")
            {
                /// Should not crash when destroying a null buffer
                REQUIRE_NOTHROW(allocator->DestroyBuffer(VK_NULL_HANDLE, nullptr));
            }

            SECTION("Handle null image in DestroyImage")
            {
                /// Should not crash when destroying a null image
                REQUIRE_NOTHROW(allocator->DestroyImage(VK_NULL_HANDLE, nullptr));
            }

            SECTION("Invalid marker for defragmentation")
            {
                /// Begin defragmentation
                REQUIRE_NOTHROW(allocator->BeginDefragmentation());

                /// Try to mark a null allocation
                REQUIRE_NOTHROW(allocator->MarkForDefragmentation(nullptr));

                /// Create a valid allocation
                auto& tracker = trackAllocation();
                VkBufferCreateInfo bufferInfo = {};
                bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
                bufferInfo.size = 1024;
                bufferInfo.usage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
                bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

                tracker.allocation = allocator->AllocateBuffer(bufferInfo, VMA_MEMORY_USAGE_GPU_ONLY, tracker.buffer);

                /// Mark it for defragmentation
                REQUIRE_NOTHROW(allocator->MarkForDefragmentation(tracker.allocation));

                /// End defragmentation
                REQUIRE_NOTHROW(allocator->EndDefragmentation());
            }

            SECTION("Double defragmentation")
            {
                /// Begin defragmentation twice without ending
                REQUIRE_NOTHROW(allocator->BeginDefragmentation());
                REQUIRE_NOTHROW(allocator->BeginDefragmentation()); /// Should handle this gracefully

                /// End defragmentation
                REQUIRE_NOTHROW(allocator->EndDefragmentation());
            }
        }
    }  // namespace Tests
}  // namespace SceneryEditorX

/// -------------------------------------------------------
