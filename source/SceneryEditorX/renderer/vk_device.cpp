/**
* -------------------------------------------------------
* Scenery Editor X
* -------------------------------------------------------
* Copyright (c) 2025 Thomas Ray 
* Copyright (c) 2025 Coalition of Freeware Developers
* -------------------------------------------------------
* vk_device.cpp
* -------------------------------------------------------
* Created: 21/3/2025
* -------------------------------------------------------
*/

#include <SceneryEditorX/core/base.hpp>
#include <SceneryEditorX/renderer/vk_checks.h>
#include <SceneryEditorX/renderer/vk_core.h>
#include <SceneryEditorX/renderer/vk_device.h>
#include <SceneryEditorX/renderer/vk_util.h>

// -------------------------------------------------------

namespace SceneryEditorX
{
	// -------------------------------------------------------

	VulkanPhysicalDevice::VulkanPhysicalDevice()
    {
        const auto instance = GraphicsEngine::GetInstance();

        std::vector<VkPhysicalDevice> device;
        uint32_t GFXDevices = 0; // Number of physical devices
        VkResult result = VK_SUCCESS;

        // -------------------------------------------------------

        VK_CHECK_RESULT(vkEnumeratePhysicalDevices(instance, &GFXDevices, nullptr));
        if (GFXDevices > 0)
        {
            device.resize(GFXDevices);
            result = vkEnumeratePhysicalDevices(instance, &GFXDevices, device.data());
            if ((result != VK_SUCCESS) || (GFXDevices == 0))
            {
                SEDX_CORE_ERROR("Could not enumerate physical devices.");
                //VK_CHECK_RESULT(result, "Vulkan Physical Devices");
                ErrMsg("Could not enumerate physical devices.");
                return;
            }

            // Resize the devices vector to accommodate the number of physical devices
            devices.resize(GFXDevices);

            for (uint32_t index = 0; index < GFXDevices; index++)
            {
                const VkPhysicalDevice physicalDevice = device[index];
                devices[index].physicalDevice = physicalDevice;
            }
        }
        else
        {
            SEDX_CORE_ERROR("No physical devices found.");
            //VK_CHECK_RESULT(result, "Vulkan Physical Devices");
            ErrMsg("No physical devices found.");
        }

        // -------------------------------------------------------

        for (uint32_t index = 0; index < GFXDevices; index++)
        {
            VkPhysicalDevice physicalDevice = device[index];
            devices[index].physicalDevice = physicalDevice;

            // -------------------------------------------------------

            vkGetPhysicalDeviceProperties(physicalDevice, &devices[index].deviceInfo);

            SEDX_CORE_TRACE("============================================");
            SEDX_CORE_TRACE("Device Name: {}",		ToString(devices[index].deviceInfo.deviceName));
            SEDX_CORE_TRACE("Device Type: {}",		ToString(devices[index].deviceInfo.deviceType));
            SEDX_CORE_TRACE("Device ID: {}",		ToString(devices[index].deviceInfo.deviceID));
            SEDX_CORE_TRACE("Driver Version: {}",   ToString(devices[index].deviceInfo.driverVersion));
            SEDX_CORE_TRACE("API Version: {}",		ToString(devices[index].deviceInfo.apiVersion));
            SEDX_CORE_TRACE("Vendor ID: {}",		ToString(devices[index].deviceInfo.vendorID));
            SEDX_CORE_TRACE("============================================");

            // -------------------------------------------------------

            uint32_t numQueueFamilies = 0;
            vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &numQueueFamilies, nullptr);

            // -------------------------------------------------------

            SEDX_CORE_INFO("Number of GPU device family queues: {}", ToString(numQueueFamilies));

            // -------------------------------------------------------

            devices[index].queueFamilyInfo.resize(numQueueFamilies);
            devices[index].queueSupportPresent.resize(numQueueFamilies);

            vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &numQueueFamilies, devices[index].queueFamilyInfo.data());
            for (uint32_t queue = 0; queue < numQueueFamilies; queue++)
            {
                const VkQueueFamilyProperties &queueFamilyInfo = devices[index].queueFamilyInfo[queue];

                SEDX_CORE_INFO("============================================");
                SEDX_CORE_INFO("Queue Family Index: {}", ToString(queue));
                SEDX_CORE_INFO("Queue Count: {}",		 ToString(queueFamilyInfo.queueCount));
                SEDX_CORE_INFO("Queue Flags: {}",		 ToString(queueFamilyInfo.queueFlags));
                SEDX_CORE_INFO("============================================");

                VK_CHECK_RESULT(vkGetPhysicalDeviceSurfaceSupportKHR(physicalDevice, queue, nullptr, &devices[index].queueSupportPresent[queue]));
            }

            // -------------------------------------------------------

            uint32_t numSurfaceFormats = 0;

            VK_CHECK_RESULT(vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, nullptr, &numSurfaceFormats, nullptr));

            SEDX_CORE_INFO("Number of surface formats: {}", ToString(numSurfaceFormats));

            devices[index].surfaceFormats.resize(numSurfaceFormats);

            VK_CHECK_RESULT(vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, nullptr, &numSurfaceFormats, devices[index].surfaceFormats.data()));

            for (uint32_t format = 0; format < numSurfaceFormats; format++)
            {
                const VkSurfaceFormatKHR &surfaceFormat = devices[index].surfaceFormats[format];
                SEDX_CORE_INFO("============================================");
                SEDX_CORE_INFO("Surface Format: {}", ToString(surfaceFormat.format));
                SEDX_CORE_INFO("Color Space: {}",    ToString(surfaceFormat.colorSpace));
                SEDX_CORE_INFO("============================================");
            }

            VK_CHECK_RESULT(vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physicalDevice, nullptr, &(devices[index].surfaceCapabilities)));
            //if (result == VK_ERROR_surfaceLOST_KHR)
            //{
            //    // Handle surface lost error
            //    SEDX_CORE_ERROR("Surface lost. Recreating surface...");
            //    // Code to recreate the surface
            //    // surface = RecreateSurface();
            //    // Retry the call
            //    result = vkGetPhysicalDeviceSurfaceCapabilitiesKHR(PhysicalDevice_,
            //                                                       surface,
            //                                                       &(devices[index].surfaceCapabilities));
            //}
            //VK_CHECK_RESULT(result, "Vulkan Physical Device Surface Capabilities");

            SEDX_CORE_INFO("============================================");
            SEDX_CORE_INFO("Surface Capabilities");
            SEDX_CORE_INFO("____________________________________________");
            SEDX_CORE_INFO("Min Image Count: {}",			ToString(devices[index].surfaceCapabilities.minImageCount));
            SEDX_CORE_INFO("Max Image Count: {}",			ToString(devices[index].surfaceCapabilities.maxImageCount));
            SEDX_CORE_INFO("Current Extent: {}",			ToString(devices[index].surfaceCapabilities.currentExtent.width));
            SEDX_CORE_INFO("Min Image Extent: {}",			ToString(devices[index].surfaceCapabilities.minImageExtent.width));
            SEDX_CORE_INFO("Max Image Extent: {}",			ToString(devices[index].surfaceCapabilities.maxImageExtent.width));
            SEDX_CORE_INFO("Max Image Array Layers: {}",    ToString(devices[index].surfaceCapabilities.maxImageArrayLayers));
            SEDX_CORE_INFO("Supported Transforms: {}",		ToString(devices[index].surfaceCapabilities.supportedTransforms));
            SEDX_CORE_INFO("Current Transform: {}",			ToString(devices[index].surfaceCapabilities.currentTransform));
            SEDX_CORE_INFO("Supported Composite Alpha: {}", ToString(devices[index].surfaceCapabilities.supportedCompositeAlpha));
            SEDX_CORE_INFO("Supported Usage Flags: {}",     ToString(devices[index].surfaceCapabilities.supportedUsageFlags));
            SEDX_CORE_INFO("============================================");

            // -------------------------------------------------------

            uint32_t numPresentModes = 0;
            VK_CHECK_RESULT(vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, nullptr, &numPresentModes, nullptr));

            devices[index].presentModes.resize(numPresentModes);

            VK_CHECK_RESULT(vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, nullptr, &numPresentModes, devices[index].presentModes.data()));
            SEDX_CORE_INFO("Number of present modes: {}", ToString(numPresentModes));

            // -------------------------------------------------------

            vkGetPhysicalDeviceMemoryProperties(physicalDevice, &(devices[index].memoryInfo));

            SEDX_CORE_INFO("Number of memory types: {}", ToString(devices[index].memoryInfo.memoryTypeCount));

            for (uint32_t mem = 0; mem < devices[index].memoryInfo.memoryTypeCount; mem++)
            {
                const auto &[propertyFlags, heapIndex] = devices[index].memoryInfo.memoryTypes[mem];
                SEDX_CORE_INFO("============================================");
                SEDX_CORE_INFO("Memory Type Index: {}",		ToString(mem));
                SEDX_CORE_INFO("Memory Heap Index: {}",		ToString(heapIndex));
                SEDX_CORE_INFO("Memory Property Flags: {}", ToString(propertyFlags));
                SEDX_CORE_INFO("============================================");
            }

            SEDX_CORE_INFO("Number of memory heaps: {}", ToString(devices[index].memoryInfo.memoryHeapCount));

            vkGetPhysicalDeviceFeatures(physicalDevice, &devices[index].GFXFeatures);
        }
	}

	VulkanPhysicalDevice::~VulkanPhysicalDevice() = default;

    // -------------------------------------------------------

	/**
	 * @brief Checks to see if the device is suitable for the application.
	 * @param queueType
	 * @param supportPresent
	 * @param device 
	 * @return True if the device is suitable for the application.
	 * @return False if the device is not suitable for the application.
	 */

    /*
    bool VulkanDevice::IsDeviceSuitable(Ref<VulkanPhysicalDevice> physDevice)
    {
        Ref<VulkanPhysicalDevice> indices = physDevice->Selected()->GetQueueFamilyIndices();

        VulkanChecks checks;
        QueueFamilyIndices indices;

        bool extensionsSupported = checks.CheckDeviceExtensionSupport(physDevice);
        bool swapChainAdequate = false;
        if (extensionsSupported)
        {
            SwapChainSupportDetails swapChainSupport = querySwapChainSupport(PhysicalDevice_);
            swapChainAdequate = !swapChainSupport.formats.empty() && !swapChainSupport.presentModes.empty();
        }

        VkPhysicalDeviceFeatures supportedFeatures;
        vkGetPhysicalDeviceFeatures(PhysicalDevice_, &supportedFeatures);

        return indices.isComplete() && extensionsSupported && swapChainAdequate && supportedFeatures.samplerAnisotropy;
    }
	*/

    /**
	 * @brief Select a device based on the queue type and support present.
	 * @param queueType - The type of queue to select.
	 * @param supportPresent - Whether the queue supports present.
	 * @return - The queue family index.
	 */
    uint32_t VulkanPhysicalDevice::SelectDevice(const VkQueueFlags queueType, const bool supportPresent)
    {
        for (uint32_t index = 0; index < devices.size(); index++)
        {
            for (uint32_t queue = 0; queue < devices[index].queueFamilyInfo.size(); queue++)
            {
                const VkQueueFamilyProperties &queueFamily = devices[index].queueFamilyInfo[queue];
                if (queueFamily.queueFlags & queueType && (bool)devices[index].queueSupportPresent[queue] == supportPresent)
                {
                    deviceIndex = index;
                    int queueFamily = queue;
                    SEDX_CORE_INFO("Using graphics device: {}", ToString(deviceIndex));
                    SEDX_CORE_INFO("Using queue family: {}", ToString(queueFamily));
                    return queueFamily;
                }
            }
        }

        SEDX_CORE_INFO("No suitable device found!");
        SEDX_CORE_INFO("Requires graphics Queue Type: {}", ToString(queueType));
        SEDX_CORE_INFO("Requires graphics support present: {}", ToString(supportPresent));

        ErrMsg("No suitable device found!");

        return 0;
    }

    /**
	* @brief Get the selected device.
	* 
	* @param const GPUDevice& - The selected device.
	* @return const GPUDevice&
	*/
    const GPUDevice &VulkanPhysicalDevice::Selected() const
    {
        if (deviceIndex < 0)
        {
            SEDX_CORE_ERROR("No device selected!");
            ErrMsg("No device selected!");
        }

        return devices[deviceIndex];
    }

    /**
     * @brief Get the queue family indices for the device.
     * @param qFlags - The queue flags to check for.
     * @return - The queue family indices.
     */
    QueueFamilyIndices VulkanPhysicalDevice::GetQueueFamilyIndices(int qFlags) const
    {
        QueueFamilyIndices indices;

		// Dedicated queue for compute
		// Try to find a queue family index that supports compute but not graphics
        if (qFlags & VK_QUEUE_COMPUTE_BIT)
		{
			for (uint32_t i = 0; i < QFamilyProperties.size(); i++)
			{
				auto& queueFamilyProperties = QFamilyProperties[i];
				if ((queueFamilyProperties.queueFlags & VK_QUEUE_COMPUTE_BIT) && ((queueFamilyProperties.queueFlags & VK_QUEUE_GRAPHICS_BIT) == 0))
				{
					indices.Compute = i;
					break;
				}
			}
		}

		// Dedicated queue for transfer
		// Try to find a queue family index that supports transfer but not graphics and compute
        if (qFlags & VK_QUEUE_TRANSFER_BIT)
		{
			for (uint32_t i = 0; i < QFamilyProperties.size(); i++)
			{
				auto& queueFamilyProperties = QFamilyProperties[i];
				if ((queueFamilyProperties.queueFlags & VK_QUEUE_TRANSFER_BIT) && ((queueFamilyProperties.queueFlags & VK_QUEUE_GRAPHICS_BIT) == 0) && ((queueFamilyProperties.queueFlags & VK_QUEUE_COMPUTE_BIT) == 0))
				{
					indices.Transfer = i;
					break;
				}
			}
		}

		// For other queue types or if no separate compute queue is present, return the first one to support the requested flags
		for (uint32_t i = 0; i < QFamilyProperties.size(); i++)
		{
            if ((qFlags & VK_QUEUE_TRANSFER_BIT) && indices.Transfer == -1)
			{
				if (QFamilyProperties[i].queueFlags & VK_QUEUE_TRANSFER_BIT)
					indices.Transfer = i;
			}

			if ((qFlags & VK_QUEUE_COMPUTE_BIT) && indices.Compute == -1)
			{
				if (QFamilyProperties[i].queueFlags & VK_QUEUE_COMPUTE_BIT)
					indices.Compute = i;
			}

			if (qFlags & VK_QUEUE_GRAPHICS_BIT)
			{
				if (QFamilyProperties[i].queueFlags & VK_QUEUE_GRAPHICS_BIT)
					indices.Graphics = i;
			}
		}

		return indices;
    }

	// -------------------------------------------------------

	/**
	 * @brief Create a Vulkan device.
	 * @param physDevice - The physical device to create the logical device for.
	 * @param enabledFeatures - The features to enable for the logical device.
	 */
	VulkanDevice::VulkanDevice(const Ref<VulkanPhysicalDevice>& physDevice, VkPhysicalDeviceFeatures enabledFeatures) : PhysicalDevice(physDevice), EnabledFeatures(enabledFeatures)
    {

        QueueFamilyIndices indices = PhysicalDevice->FindQueueFamilies(PhysicalDevice->GetGPUDevice());
        VulkanChecks checks;

        // Handle potentially separate queues for graphics and presentation
        std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
        std::set uniqueQueueFamilies = {indices.graphicsFamily.value(), indices.presentFamily.value()};

        float queuePriority = 1.0f;
        for (uint32_t queueFamily : uniqueQueueFamilies)
        {
            VkDeviceQueueCreateInfo queueCreateInfo{};
            queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
            queueCreateInfo.queueFamilyIndex = queueFamily;
            queueCreateInfo.queueCount = 1;
            queueCreateInfo.pQueuePriorities = &queuePriority;
            queueCreateInfos.push_back(queueCreateInfo);
        }

		// ---------------------------------------------------------

		// Vulkan Features
        VkPhysicalDeviceFeatures deviceFeatures{};
        deviceFeatures.samplerAnisotropy = VK_TRUE;

		// ---------------------------------------------------------

        VkDeviceCreateInfo createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
        createInfo.pQueueCreateInfos = queueCreateInfos.data();
        createInfo.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size());
        createInfo.pEnabledFeatures = &deviceFeatures;

		const auto& deviceExtensions = checks.DeviceExtensions;

		if (!checks.CheckDeviceExtensionSupport(PhysicalDevice->GetGPUDevice()))
        {
            SEDX_CORE_ERROR("Required device extensions not supported!");
            ErrMsg("Required device extensions not supported!");
        }

        // Enable swap chain extension
        createInfo.enabledExtensionCount = static_cast<uint32_t>(deviceExtensions.size());
        createInfo.ppEnabledExtensionNames = deviceExtensions.data();

        if (enableValidationLayers)
        {
            std::vector validationLayers = {"VK_LAYER_KHRONOS_validation"};
            createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
            createInfo.ppEnabledLayerNames = validationLayers.data();
        }

        if (vkCreateDevice(PhysicalDevice->GetGPUDevice(), &createInfo, nullptr, &device) != VK_SUCCESS)
        {
            SEDX_CORE_ERROR("Failed to create logical device!");
            ErrMsg("Failed to create logical device!");
        }

		vkGetDeviceQueue(device, indices.graphicsFamily.value(), 0, &GraphicsQueue);
		vkGetDeviceQueue(device, indices.presentFamily.value(), 0, &PresentQueue);

    }

	VulkanDevice::~VulkanDevice()
    {
        vkDestroyDevice(device, nullptr);
        device = VK_NULL_HANDLE;
    }

    /**
	 * @brief Find the queue families for the device.
	 * @param device - The device to find the queue families for.
	 * @return - The queue family indices.
	 */
	QueueFamilyIndices VulkanPhysicalDevice::FindQueueFamilies(const VkPhysicalDevice device) const
    {
        QueueFamilyIndices indices;

		uint32_t queueFamilyCount = 0;
        vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);
        std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
        vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamilies.data());

        if (deviceIndex >= 0 && deviceIndex < devices.size())
		{
		    const GPUDevice& selectedDevice = devices[deviceIndex];
		    
		    // Find queue families that support graphics and presentation
		    for (uint32_t i = 0; i < selectedDevice.queueFamilyInfo.size(); i++)
			{

                if (const VkQueueFamilyProperties &queueFamily = selectedDevice.queueFamilyInfo[i]; queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT)
				{
		            indices.graphicsFamily = i;
		            // Store in our other format too for consistency
		            indices.Graphics = i;
		        }

		        // Check presentation support
		        if (selectedDevice.queueSupportPresent[i])
				{
		            indices.presentFamily = i;
		        }

		        if (indices.IsComplete())
				{
		            break;
		        }
		    }
		}
	    
	    return indices;
	}

    void VulkanDevice::Destroy()
    {
        CmdPools.clear();
        vkDeviceWaitIdle(device);
        vkDestroyDevice(device, nullptr);
    }

	const VkDevice &VulkanDevice::Selected() const
	{
	    return device;
	}

	void CommandPool::FlushCmdBuffer(const VkCommandBuffer cmdBuffer) const
    {
        auto device = GetCurrentDevice();
        FlushCmdBuffer(cmdBuffer, device->GetGraphicsQueue());
    }

    void CommandPool::FlushCmdBuffer(const VkCommandBuffer cmdBuffer, const VkQueue queue) const
    {
        auto device = GetCurrentDevice();
        const auto vulkanDevice = device->GetDevice();

		VK_CHECK_RESULT(vkEndCommandBuffer(cmdBuffer))

        VkSubmitInfo submitInfo = {};
        submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        submitInfo.commandBufferCount = 1;
        submitInfo.pCommandBuffers = &cmdBuffer;

        // Create fence to ensure that the command buffer has finished executing
        VkFenceCreateInfo fenceCreateInfo = {};
        fenceCreateInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
        fenceCreateInfo.flags = 0;
        VkFence fence;
        vkCreateFence(vulkanDevice, &fenceCreateInfo, nullptr, &fence);

		// Submit to the queue
        vkQueueSubmit(queue, 1, &submitInfo, fence);


        vkWaitForFences(vulkanDevice, 1, &fence, VK_TRUE, DEFAULT_FENCE_TIMEOUT);
        vkDestroyFence(vulkanDevice, fence, nullptr);
        vkFreeCommandBuffers(vulkanDevice, GraphicsCmdPool, 1, &cmdBuffer);
    }

    void VulkanDevice::LockQueue(bool compute)
    {
        if (compute)
        {
            ComputeQueueMutex.lock();
        }
        else
        {
            GraphicsQueueMutex.lock();
        }
    }

    void VulkanDevice::UnlockQueue(bool compute)
    {
        if (compute)
		{
            ComputeQueueMutex.unlock();
		}
        else
        {
            GraphicsQueueMutex.unlock();
        }
    }

    VkCommandBuffer CommandPool::AllocateCommandBuffer(bool begin, bool compute) const
    {
        auto device = GetCurrentDevice();
        const auto vulkanDevice = device->GetDevice();

        VkCommandBuffer cmdBuffer;
        VkCommandBufferAllocateInfo cmdBufAllocateInfo = {};
        cmdBufAllocateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        cmdBufAllocateInfo.commandPool = compute ? ComputeCmdPool : GraphicsCmdPool;
        cmdBufAllocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        cmdBufAllocateInfo.commandBufferCount = 1;

		VK_CHECK_RESULT(vkAllocateCommandBuffers(vulkanDevice, &cmdBufAllocateInfo, &cmdBuffer))
		if (begin)
        {
            VkCommandBufferBeginInfo cmdBufferBeginInfo{};
            cmdBufferBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
            vkBeginCommandBuffer(cmdBuffer, &cmdBufferBeginInfo);
        }

        return cmdBuffer;
    }

	VkCommandBuffer VulkanDevice::CreateSecondaryCommandBuffer(const char *debugName)
    {
        VkCommandBuffer cmdBuffer;

        VkCommandBufferAllocateInfo cmdBufAllocateInfo = {};
        cmdBufAllocateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        cmdBufAllocateInfo.commandPool = GetOrCreateThreadLocalCmdPool()->GetGraphicsCmdPool();
        cmdBufAllocateInfo.level = VK_COMMAND_BUFFER_LEVEL_SECONDARY;
        cmdBufAllocateInfo.commandBufferCount = 1;

        VK_CHECK_RESULT(vkAllocateCommandBuffers(device, &cmdBufAllocateInfo, &cmdBuffer));
        SetDebugUtilsObjectName(device, VK_OBJECT_TYPE_COMMAND_BUFFER, debugName, cmdBuffer);
        return cmdBuffer;

    }

    void VulkanDevice::FlushCmdBuffer(VkCommandBuffer cmdBuffer)
    {
        GetThreadLocalCmdPool()->FlushCmdBuffer(cmdBuffer);
    }

    void VulkanDevice::FlushCmdBuffer(VkCommandBuffer cmdBuffer, VkQueue queue)
    {
        GetThreadLocalCmdPool()->FlushCmdBuffer(cmdBuffer, queue);
    }


    Ref<CommandPool> VulkanDevice::GetThreadLocalCmdPool()
    {
        auto threadID = std::this_thread::get_id();
        return CmdPools.at(threadID);
    }

    Ref<CommandPool> VulkanDevice::GetOrCreateThreadLocalCmdPool()
    {
        auto threadID = std::this_thread::get_id();
        auto commandPoolIt = CmdPools.find(threadID);
        if (commandPoolIt != CmdPools.end())
        {
            return commandPoolIt->second;
        }

        Ref<CommandPool> commandPool = Ref<CommandPool>::Create();
        CmdPools[threadID] = commandPool;
        return commandPool;
    }

    /**
     * @brief Get the current device.
     * @return The current device.
     */
    Ref<VulkanDevice> GetCurrentDevice()
    {
        Ref<VulkanDevice> currentDevice = nullptr;
        if (!currentDevice)
        {
            SEDX_CORE_ERROR("No current device initialized!");
            throw std::runtime_error("Attempted to access VulkanDevice before initialization");
        }

        return currentDevice;
    }

	// -------------------------------------------------------

    CommandPool::CommandPool()
    {
        auto device = GetCurrentDevice();
        const auto vulkanDevice = device->GetDevice();

        VkCommandPoolCreateInfo cmdPoolInfo = {};
        cmdPoolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
        cmdPoolInfo.queueFamilyIndex = device->GetPhysicalDevice()->GetQueueFamilyIndices().Graphics;
        cmdPoolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;

        VK_CHECK_RESULT(vkCreateCommandPool(vulkanDevice, &cmdPoolInfo, nullptr, &GraphicsCmdPool))
        //if (result != VK_SUCCESS)
        //{
        //    SEDX_CORE_ERROR("Failed to create graphics command pool!");
        //    throw std::runtime_error("Failed to create graphics command pool!");
        //}

		cmdPoolInfo.queueFamilyIndex = device->GetPhysicalDevice()->GetQueueFamilyIndices().Compute;
        VK_CHECK_RESULT(vkCreateCommandPool(vulkanDevice, &cmdPoolInfo, nullptr, &ComputeCmdPool))
		//if (result != VK_SUCCESS) {
		//    SEDX_CORE_ERROR("Failed to create compute command pool!");
		//    throw std::runtime_error("Failed to create compute command pool!");
		//}
    }

    CommandPool::~CommandPool()
    {
        auto device = GetCurrentDevice();
        auto vulkanDevice = device->GetDevice();

        vkDestroyCommandPool(vulkanDevice, GraphicsCmdPool, nullptr);
        vkDestroyCommandPool(vulkanDevice, ComputeCmdPool, nullptr);
    }

} // namespace SceneryEditorX

// -------------------------------------------------------
