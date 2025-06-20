/**
* -------------------------------------------------------
* Scenery Editor X - Unit Tests
* -------------------------------------------------------
* Copyright (c) 2025 Thomas Ray 
* Copyright (c) 2025 Coalition of Freeware Developers
* -------------------------------------------------------
* MemoryAllocatorTest.h
* -------------------------------------------------------
* Helper classes and utilities for memory allocator tests
* -------------------------------------------------------
*/
#pragma once

#include <SceneryEditorX/renderer/vulkan/vk_allocator.h>
#include <SceneryEditorX/renderer/render_context.h>
#include <memory>
#include <vector>

namespace SceneryEditorX
{
    namespace Tests
    {
        /**
         * @class VulkanTestEnvironment
         * @brief Manages Vulkan test environment setup and teardown
         * 
         * This class handles initialization and cleanup of Vulkan resources
         * needed for memory allocator tests. It ensures proper setup of
         * Vulkan instance, physical device, logical device, and memory allocator.
         */
        class VulkanTestEnvironment
        {
        public:
            /**
             * @brief Constructor initializes Vulkan instance and resources
             */
            VulkanTestEnvironment();
            
            /**
             * @brief Destructor cleans up Vulkan resources
             */
            ~VulkanTestEnvironment();
            
            /**
             * @brief Get the VulkanDevice reference
             * @return Reference to the Vulkan device
             */
            Ref<VulkanDevice> GetDevice() const { return device; }
            
            /**
             * @brief Get the MemoryAllocator reference
             * @return Reference to the memory allocator
             */
            Ref<MemoryAllocator> GetAllocator() const { return allocator; }

        private:
            /**
             * @brief Initialize Vulkan resources
             */
            void initializeVulkan();
            
            /**
             * @brief Clean up Vulkan resources
             */
            void cleanup();

            Ref<VulkanDevice> device;
            Ref<MemoryAllocator> allocator;
        };

        /**
         * @class MockAllocationTracker
         * @brief Tracks and manages Vulkan memory allocations for tests
         * 
         * This class helps track Vulkan buffer and image allocations during tests
         * and ensures proper cleanup even if tests fail.
         */
        class MockAllocationTracker
        {
        public:
            /**
             * @brief Constructor initializes to null handles
             */
            MockAllocationTracker();
            
            /**
             * @brief Destructor ensures resource cleanup
             */
            ~MockAllocationTracker();
            
            /**
             * @brief Clean up tracked resources
             */
            void cleanup();
            
            VmaAllocation allocation;
            VkBuffer buffer;
            VkImage image;
        };

        /**
         * @class MemoryAllocatorTestFixture
         * @brief Test fixture for MemoryAllocator tests
         * 
         * This fixture class provides common setup and teardown for
         * memory allocator tests, tracking allocations and ensuring cleanup.
         */
        class MemoryAllocatorTestFixture
        {
        public:
            /**
             * @brief Constructor initializes test environment
             */
            MemoryAllocatorTestFixture();
            
            /**
             * @brief Destructor cleans up tracked allocations
             */
            ~MemoryAllocatorTestFixture();
            
            /**
             * @brief Track a new allocation for automatic cleanup
             * @return Reference to the new allocation tracker
             */
            MockAllocationTracker& trackAllocation();
            
            static std::shared_ptr<VulkanTestEnvironment> environment;
            Ref<MemoryAllocator> allocator;
            Ref<VulkanDevice> device;
            std::vector<MockAllocationTracker> allocationTrackers;
        };
    }  // namespace Tests
}  // namespace SceneryEditorX
