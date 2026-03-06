/**
 * -------------------------------------------------------
 * Scenery Editor X
 * -------------------------------------------------------
 * Copyright (c) 2026 Thomas Ray 
 * Copyright (c) 2026 Coalition of Freeware Developers
 * -------------------------------------------------------
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 * -------------------------------------------------------
 * device.h
 * -------------------------------------------------------
 * Created: 09/02/2026
 * -------------------------------------------------------
 */
#pragma once
#include "memory_allocator.h"
#include "queue_manager.h"
#include "debug/graphics_checks.h"

// -----------------------------------------------------------------

namespace SceneryEditorX
{

    struct HWDeviceInfo;

    /**
     * @struct DeviceStatics
     * @brief Snapshot of static device capabilities and limits
     * This structure holds various device limits and supported features
     * populated during device initialization for easy access.
     *
     * Usage:
     * - Call Device::GetDeviceStatics() to retrieve a populated instance.
     *
     * @note This structure is read-only after initialization.
     */
	struct DeviceStatics
	{
	    float timestampPeriod = 0.0f;

	    uint64_t minUniformBufferOffsetAlignment	= 0;
	    uint64_t minStorageBufferOffsetAlignment	= 0;
	    uint64_t minAccelBufferOffsetAlignment		= 0;
	    uint64_t optimalBufferCopyOffsetAlignment	= 0;
	
	    uint32_t maxImageDimension1D	= 0;
	    uint32_t maxImageDimension2D	= 0;
	    uint32_t maxImageDimension3D	= 0;
	    uint32_t maxImageDimensionCube	= 0;
	    uint32_t maxImageArrayLayers	= 0;
	    uint32_t maxPushConstantsSize	= 0;
	
	    uint32_t maxShadingRateTexelSizeX = 0;
	    uint32_t maxShadingRateTexelSizeY = 0;
	
	    uint32_t shaderGroupHandleSize		= 0;
	    uint32_t shaderGroupHandleAlignment = 0;
	    uint32_t shaderGroupBaseAlignment	= 0;
	
	    bool xessSupported			= false;
	    bool isShadingRateSupported = false;
	    bool isRayTracingSupported	= false;
	    bool isBindlessSupported	= false;
	    bool wideLinesSupported		= false;
	};

    // -----------------------------------------------------------------

    class Device : public RefCounted
	{
	public:
        Device(VkInstance instance, GraphicsChecks::InstanceProperties gpuProps = GraphicsChecks::GetDefaultInstanceProperties());
        virtual ~Device() override;

        [[nodiscard]] VkInstance GetInstance() const { return m_Instance; }
        [[nodiscard]] VkSurfaceKHR GetWindowSurface() const { return m_WindowSurface; }
        [[nodiscard]] const VkPhysicalDeviceLimits& GetDeviceLimits() const { return m_PhysDeviceProp.properties.limits; }
        [[nodiscard]] const VkPhysicalDeviceProperties& GetDeviceProperties() const { return m_PhysDeviceProp.properties; }
        [[nodiscard]] const VkPhysicalDeviceMemoryProperties2& GetDeviceMemoryProperties() const { return m_PhysDeviceMemProp; }

        static uint32_t GetPhysicalDeviceCount();
        static uintptr_t GetPhysicalDeviceHandle(const uint32_t index);

        /**
	     * @brief Get the global Device instance
	     * @return a reference count (increments ref count)
	     */
        Ref<Device> Get() { return m_Device; }

        /**
		 * @brief Get the Vulkan logical device handle associated with this Device instance
		 * @return VkDevice handle of the logical device, or VK_NULL_HANDLE if not initialized
		 */
		[[nodiscard]] VkDevice GetLogicalDevice() const { return m_LogicalDevice; }

        /**
		 * @brief Get the Vulkan physical device handle associated with this Device instance
		 * @return VkPhysicalDevice handle of the physical device, or VK_NULL_HANDLE if not initialized
		 */
		[[nodiscard]] static VkPhysicalDevice GetPhysicalDevice() { return m_PhysicalDevice; }

        /**
	     * @brief Get the Vulkan logical device handle
	     * @return VkDevice handle or VK_NULL_HANDLE if no device is available
	     */
        [[nodiscard]] VkDevice GetDevice() const { return m_LogicalDevice; }

        /**
	     * @brief Get a snapshot of internal static capabilities populated during device setup
	     * @return DeviceStatics structure with device limits and supported features
	     */
        static DeviceStatics GetDeviceStatics();

		/**
		 * @brief Get the QueueManager instance for this device
		 * @return Reference to the QueueManager
		 */
        Ref<QueueManager> GetQueueManager() { return m_QueueManager; }

        /**
		 * @brief Get the MemoryAllocator instance for this device
		 * @return Reference to the MemoryAllocator
		 */
		MemoryAllocator& GetMemoryAllocator() { return m_MemAllocator; }

        /**
         * @brief Retrieve hardware device specifications by index.
         * @param index Index of the physical device to query.
         * @param outDeviceInfo Reference to a HWDeviceInfo structure to be populated with the device's specifications.
         * @return True if the device specifications were successfully retrieved and populated into outDeviceInfo, false otherwise.
         */
        static bool GetDeviceSpec(const uint32_t index, HWDeviceInfo &outDeviceInfo);
	
        /**
	     * @brief Create a Vulkan logical device (VkDevice) based on the selected physical device and its queue families.
	     * @return VkDevice handle of the created logical device, or VK_NULL_HANDLE on failure.
	     *
	     * @note It also sets up the QueueManager with the allocated queues from the created device.
	     */
	    VkDevice Create();

        /**
         * @brief Select the best physical device available on the system based on scoring of features, memory, and device type.
         * @return VkPhysicalDevice handle of the selected device, or VK_NULL_HANDLE if no suitable device is found.
         */
        static VkPhysicalDevice Choose();

        static bool IsDeviceLost()  { return m_DeviceLost; }
        static void SetDeviceLost() { m_DeviceLost = true; }

    private:
        std::string m_GPUName;
        Ref<QueueManager> m_QueueManager;
        MemoryAllocator m_MemAllocator;
        QueueManager::QueueFamilyIndices m_FamilyIndices;

        Ref<Device> m_Device;
        VkDevice m_LogicalDevice = VK_NULL_HANDLE;
        static VkPhysicalDevice m_PhysicalDevice;
        static uint32_t m_PhysicalDeviceIndex;
        static bool m_DeviceLost;
        GraphicsChecks::InstanceProperties m_InstanceProps;
        HWDeviceInfo *m_HWDeviceInfo;

        VkInstance m_Instance = VK_NULL_HANDLE;
        VkSurfaceKHR m_WindowSurface = VK_NULL_HANDLE;

        VkPhysicalDeviceFeatures2 m_PhysDeviceFeat;
        VkPhysicalDeviceProperties2 m_PhysDeviceProp;
        VkPhysicalDeviceMemoryProperties2 m_PhysDeviceMemProp;
	};

}

// -----------------------------------------------------------------
