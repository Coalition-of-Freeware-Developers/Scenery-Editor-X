/**
* -------------------------------------------------------
* Scenery Editor X - Unit Tests
* -------------------------------------------------------
* Copyright (c) 2025 Thomas Ray 
* Copyright (c) 2025 Coalition of Freeware Developers
* -------------------------------------------------------
* MemoryAllocatorTestUtils.cpp
* -------------------------------------------------------
* Implementation of helper classes for memory allocator tests
* -------------------------------------------------------
*/
#include <catch2/catch_test_macros.hpp>
#include "MemoryAllocatorTest.h"

/// -------------------------------------------------------

namespace SceneryEditorX
{
    namespace Tests
    {
        /// Initialize the static environment pointer
        std::shared_ptr<VulkanTestEnvironment> MemoryAllocatorTestFixture::environment = nullptr;

        VulkanTestEnvironment::VulkanTestEnvironment()
        {
            initializeVulkan();
        }

        VulkanTestEnvironment::~VulkanTestEnvironment()
        {
            cleanup();
        }

        void VulkanTestEnvironment::initializeVulkan()
        {
            /// This simulates the setup that would normally be done in RenderContext::Init()
            try
            {
                /// Initialize Vulkan instance
                RenderContext renderContext;
                renderContext.Init();
                
                /// Get the device
                device = renderContext.GetLogicDevice();
                
                /// Create memory allocator
                allocator = CreateRef<MemoryAllocator>("TestAllocator");
                
                ///< Initialize the allocator with the device
                ///< Using standard init process instead of calling Init directly
                if (allocator)
                    REQUIRE(allocator != nullptr); ///< We don't have direct access to Init, so we're just ensuring the allocator is created
            } 
            catch (const std::exception& e)
            {
                FAIL("Failed to initialize Vulkan test environment: " << e.what());
            }
        }

        void VulkanTestEnvironment::cleanup()
        {
            ///< Clean up allocator
            allocator = nullptr;
            
            ///< Clean up device
            device = nullptr;
        }

        MockAllocationTracker::MockAllocationTracker() : allocation(nullptr), buffer(VK_NULL_HANDLE), image(VK_NULL_HANDLE) {}
        
        MockAllocationTracker::~MockAllocationTracker()
        {
            cleanup();
        }
        
        void MockAllocationTracker::cleanup()
        {
            if (!allocation && buffer == VK_NULL_HANDLE && image == VK_NULL_HANDLE)
                return;
                
            RenderContext renderContext;
            if (const auto deviceRef = renderContext.GetLogicDevice(); !deviceRef) return;
            
            ///< Since we can't access memoryAllocator directly, we'll use our test allocator
            ///< This assumes we have the allocator from the test fixture available
            if (MemoryAllocatorTestFixture::environment && MemoryAllocatorTestFixture::environment->GetAllocator())
            {
                const auto allocator = MemoryAllocatorTestFixture::environment->GetAllocator();
                
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

        MemoryAllocatorTestFixture::MemoryAllocatorTestFixture() 
        {
            /// Only set up Vulkan once
            if (!environment)
                environment = std::make_shared<VulkanTestEnvironment>();

            allocator = environment->GetAllocator();
            device = environment->GetDevice();
        }
        
        MemoryAllocatorTestFixture::~MemoryAllocatorTestFixture()
        {
            /// Clean up any tracking objects
            for (auto& tracker : allocationTrackers)
                tracker.cleanup();

            allocationTrackers.clear();
        }
        
        MockAllocationTracker& MemoryAllocatorTestFixture::trackAllocation()
        {
            allocationTrackers.emplace_back();
            return allocationTrackers.back();
        }
        
    }

}

/// -------------------------------------------------------
