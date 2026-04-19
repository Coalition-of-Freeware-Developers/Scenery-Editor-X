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
// ReSharper disable CppInconsistentNaming
#pragma once
#include "memory_allocator.h"
#include "queue_manager.h"
#include <cstdint>
#include <string>
#include <SceneryEditorX/renderer/vulkan/debug/graphics_checks.h>
#include <SceneryEditorX/utils/pointers.h>

// -----------------------------------------------------------------

namespace SceneryEditorX
{
	class CommandList;
	struct HWDeviceInfo;
	class QueueManager;

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
		bool shaderDrawParameters	= false;
	};

	// -----------------------------------------------------------------

	/**
	 * @class Device
	 * @brief Represents a Vulkan device, encapsulating both the physical and logical device handles.
	 *
	 * This class provides methods to create and manage a Vulkan logical device, query device properties,
	 * and access associated resources such as queues and memory allocators.
	 */
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
		 * @brief Get the hardware device information for a specific physical device
		 * @param device Vulkan physical device handle
		 * @return HWDeviceInfo structure containing the specifications of the specified device
		 */
		static HWDeviceInfo GetHWDeviceInfo(const VkPhysicalDevice device);

		/**
		 * @brief Get the hardware device information of the chosen physical device
		 * @return HWDeviceInfo structure containing the specifications of the chosen device
		 */
		static const HWDeviceInfo GetChosenHWDeviceInfo();

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
		const Ref<QueueManager>& GetQueueManager() const { return m_QueueManager; }

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

		/**
		 * @brief Get the current state of the device loss flag.
		 * @return the current state of the device loss flag, indicating whether the device is considered lost and unusable.
		 * This flag is set to true when a device loss condition is detected, such as a GPU reset or driver crash, and can be used by the application to trigger appropriate recovery or cleanup actions.
		 * Once set, this flag remains true until the application takes steps to recreate the device and reset the state.
		 * @note Device loss is a critical condition that typically requires recreating the Vulkan device and associated resources. 
		 */
		static bool IsDeviceLost()  { return m_DeviceLost; }

		/* @brief Set the device loss flag to true, indicating that the device is considered lost and unusable. */
		static void SetDeviceLost() { m_DeviceLost = true; }
	
		/**
		 * @brief 
		 * @param cmd 
		 * @param enabled 
		 */
		static void SetVariableRateShading(const CommandList *cmd, const bool enabled);

	private:
		std::string m_GPUName;								// For logging and debugging purposes
		Ref<QueueManager> m_QueueManager;					// Manages the Vulkan queues associated with this device
		MemoryAllocator m_MemAllocator;						// Manages memory allocations for this device
		QueueManager::QueueFamilyIndices m_FamilyIndices;	// Cached queue family indices for quick access during device creation and queue allocation

		VkDevice m_LogicalDevice = VK_NULL_HANDLE;			// Handle to the Vulkan logical device created from the selected physical device
		static VkPhysicalDevice m_PhysicalDevice;			// Static handle to the Vulkan physical device selected for use by this Device instance.
		static uint32_t m_PhysicalDeviceIndex;				// Static index of the selected physical device among the available devices on the system.
		static bool m_DeviceLost;							// Static flag indicating whether the device is considered lost and unusable.
		GraphicsChecks::InstanceProperties m_InstanceProps; // Cached instance properties used during device selection and creation, such as requested extensions and features.
		//HWDeviceInfo *m_HWDeviceInfo;						// Pointer to a structure containing hardware device information.

		VkInstance m_Instance = VK_NULL_HANDLE;
		VkSurfaceKHR m_WindowSurface = VK_NULL_HANDLE;

		VkPhysicalDeviceFeatures2 m_PhysDeviceFeat;
		VkPhysicalDeviceProperties2 m_PhysDeviceProp;
		VkPhysicalDeviceMemoryProperties2 m_PhysDeviceMemProp;
	};

}

// -----------------------------------------------------------------
