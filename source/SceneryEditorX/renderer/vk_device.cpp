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
#include <SceneryEditorX/renderer/vk_allocator.h>
#include <SceneryEditorX/renderer/vk_checks.h>
#include <SceneryEditorX/renderer/vk_core.h>
#include <SceneryEditorX/renderer/vk_device.h>
#include <SceneryEditorX/renderer/vk_util.h>

// -------------------------------------------------------

namespace SceneryEditorX
{
	// -------------------------------------------------------
	// VulkanPhysicalDevice Implementation
	// -------------------------------------------------------

    VulkanPhysicalDevice::VulkanPhysicalDevice() : instance(GraphicsEngine::GetInstance())
	{
	    EnumerateDevices();
	}

    void VulkanPhysicalDevice::EnumerateDevices()
    {
        const auto instance = GraphicsEngine::GetInstance();

        std::vector<VkPhysicalDevice> device;
        uint32_t GFXDevices = 0; // Number of physical devices

        // -------------------------------------------------------

        VK_CHECK_RESULT(vkEnumeratePhysicalDevices(instance, &GFXDevices, nullptr))
        if (GFXDevices > 0)
        {
            device.resize(GFXDevices);
            if (const VkResult result = vkEnumeratePhysicalDevices(instance, &GFXDevices, device.data()); (result != VK_SUCCESS) || (GFXDevices == 0))
            {
                SEDX_CORE_ERROR("Could not enumerate physical devices.");
                return;
            }

            /// Resize the devices vector to accommodate the number of physical devices
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
            ErrMsg("No physical devices found.");
            return;
        }

        // -------------------------------------------------------

        /// Initialize device information for each physical device
        for (uint32_t index = 0; index < GFXDevices; index++)
        {
            const VkPhysicalDevice physicalDevice = device[index];
            devices[index].physicalDevice = physicalDevice;

            // -------------------------------------------------------

            vkGetPhysicalDeviceProperties(physicalDevice, &devices[index].deviceProperties); /// Get device properties
            devices[index].GFXLimits = devices[index].deviceProperties.limits;               /// Store device limits

            SEDX_CORE_TRACE("============================================");
            SEDX_CORE_TRACE("Device Name: {}",		ToString(devices[index].deviceProperties.deviceName));
            SEDX_CORE_TRACE("Device Type: {}",		ToString(devices[index].deviceProperties.deviceType));
            SEDX_CORE_TRACE("Device ID: {}",		ToString(devices[index].deviceProperties.deviceID));
            SEDX_CORE_TRACE("Driver Version: {}",   ToString(devices[index].deviceProperties.driverVersion));
            SEDX_CORE_TRACE("API Version: {}",		ToString(devices[index].deviceProperties.apiVersion));
            SEDX_CORE_TRACE("Vendor ID: {}",		ToString(devices[index].deviceProperties.vendorID));
            SEDX_CORE_TRACE("============================================");

            // -------------------------------------------------------

            /// Get queue family properties
            uint32_t numQueueFamilies = 0;
            vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &numQueueFamilies, nullptr);
            devices[index].queueFamilyInfo.resize(numQueueFamilies);
            devices[index].queueSupportPresent.resize(numQueueFamilies);

            vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &numQueueFamilies, devices[index].queueFamilyInfo.data());
            SEDX_CORE_INFO("Number of GPU vkDevice family queues: {}", ToString(numQueueFamilies));

			/// Check presentation support for each queue family
            for (uint32_t queue = 0; queue < numQueueFamilies; queue++)
            {
                const VkQueueFamilyProperties &queueFamilyInfo = devices[index].queueFamilyInfo[queue];
				VK_CHECK_RESULT(vkGetPhysicalDeviceSurfaceSupportKHR(physicalDevice, queue, nullptr, &devices[index].queueSupportPresent[queue]))

                SEDX_CORE_INFO("============================================");
                SEDX_CORE_INFO("Queue Family Index: {}", ToString(queue));
                SEDX_CORE_INFO("Queue Count: {}", ToString(queueFamilyInfo.queueCount));
                SEDX_CORE_INFO("Queue Flags: {}", ToString(queueFamilyInfo.queueFlags));
                SEDX_CORE_INFO("============================================");

            }

            // -------------------------------------------------------

            /// Get surface formats
            uint32_t numSurfaceFormats = 0;
            VK_CHECK_RESULT(vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, nullptr, &numSurfaceFormats, nullptr))
            SEDX_CORE_INFO("Number of surface formats: {}", ToString(numSurfaceFormats));

            devices[index].surfaceFormats.resize(numSurfaceFormats);
            VK_CHECK_RESULT(vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, nullptr, &numSurfaceFormats, devices[index].surfaceFormats.data()))
            for (uint32_t format = 0; format < numSurfaceFormats; format++)
            {
                const VkSurfaceFormatKHR &surfaceFormat = devices[index].surfaceFormats[format];
                SEDX_CORE_INFO("============================================");
                SEDX_CORE_INFO("Surface Format: {}", ToString(surfaceFormat.format));
                SEDX_CORE_INFO("Color Space: {}", ToString(surfaceFormat.colorSpace));
                SEDX_CORE_INFO("============================================");
            }

			/// Get surface capabilities
            VK_CHECK_RESULT(vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physicalDevice, nullptr, &(devices[index].surfaceCapabilities)))
            SEDX_CORE_INFO("============================================");
            SEDX_CORE_INFO("Surface Capabilities");
            SEDX_CORE_INFO("____________________________________________");
            SEDX_CORE_INFO("Min Image Count: {}",			ToString(devices[index].surfaceCapabilities.minImageCount));
            SEDX_CORE_INFO("Max Image Count: {}",			ToString(devices[index].surfaceCapabilities.maxImageCount));
            SEDX_CORE_INFO("Current Extent: {}",			ToString(devices[index].surfaceCapabilities.currentExtent.width));
            SEDX_CORE_INFO("Min Image Extent: {}",			ToString(devices[index].surfaceCapabilities.minImageExtent.width));
            SEDX_CORE_INFO("Max Image Extent: {}",			ToString(devices[index].surfaceCapabilities.maxImageExtent.width));
            SEDX_CORE_INFO("Max Image Array Layers: {}",	ToString(devices[index].surfaceCapabilities.maxImageArrayLayers));
            SEDX_CORE_INFO("Supported Transforms: {}",		ToString(devices[index].surfaceCapabilities.supportedTransforms));
            SEDX_CORE_INFO("Current Transform: {}",			ToString(devices[index].surfaceCapabilities.currentTransform));
            SEDX_CORE_INFO("Supported Composite Alpha: {}", ToString(devices[index].surfaceCapabilities.supportedCompositeAlpha));
            SEDX_CORE_INFO("Supported Usage Flags: {}",     ToString(devices[index].surfaceCapabilities.supportedUsageFlags));
            SEDX_CORE_INFO("============================================");

            // -------------------------------------------------------

            /// Get present modes
            uint32_t numPresentModes = 0;
            VK_CHECK_RESULT(vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, nullptr, &numPresentModes, nullptr))
            devices[index].presentModes.resize(numPresentModes);
            VK_CHECK_RESULT(vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, nullptr, &numPresentModes, devices[index].presentModes.data()))
            SEDX_CORE_INFO("Number of present modes: {}", ToString(numPresentModes));

            // -------------------------------------------------------

			/// Get memory properties
            vkGetPhysicalDeviceMemoryProperties(physicalDevice, &(devices[index].memoryInfo));
            SEDX_CORE_INFO("Number of memory types: {}", ToString(devices[index].memoryInfo.memoryTypeCount));
            for (uint32_t mem = 0; mem < devices[index].memoryInfo.memoryTypeCount; mem++)
            {
                const auto &[propertyFlags, heapIndex] = devices[index].memoryInfo.memoryTypes[mem];
                SEDX_CORE_INFO("============================================");
                SEDX_CORE_INFO("Memory Type Index: {}", ToString(mem));
                SEDX_CORE_INFO("Memory Heap Index: {}", ToString(heapIndex));
                SEDX_CORE_INFO("Memory Property Flags: {}", ToString(propertyFlags));
                SEDX_CORE_INFO("============================================");
            }

            SEDX_CORE_INFO("Number of memory heaps: {}", ToString(devices[index].memoryInfo.memoryHeapCount));

			/// Get device features
            vkGetPhysicalDeviceFeatures(physicalDevice, &devices[index].GFXFeatures);
        }

        // Store queue family properties for later queue index lookup
        if (!devices.empty())
        {
            QFamilyProperties = devices[0].queueFamilyInfo;
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
            SwapChainSupportDetails swapChainSupport = querySwapChainSupport(vkPhysicalDevice);
            swapChainAdequate = !swapChainSupport.formats.empty() && !swapChainSupport.presentModes.empty();
        }

        VkPhysicalDeviceFeatures supportedFeatures;
        vkGetPhysicalDeviceFeatures(vkPhysicalDevice, &supportedFeatures);

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
        /// Try to find a discrete GPU that supports the requested queue type and presentation mode
        for (uint32_t index = 0; index < devices.size(); index++)
        {
            for (uint32_t queue = 0; queue < devices[index].queueFamilyInfo.size(); queue++)
            {
                const VkQueueFamilyProperties &queueFamily = devices[index].queueFamilyInfo[queue];
                if (queueFamily.queueFlags & queueType && static_cast<bool>(devices[index].queueSupportPresent[queue]) == supportPresent)
                {
                    deviceIndex = index;
                    int queueFamily = queue;
                    SEDX_CORE_INFO("Using graphics vkDevice: {}", ToString(deviceIndex));
                    SEDX_CORE_INFO("Using queue family: {}", ToString(queueFamily));
                    return queueFamily;
                }
            }
        }

        SEDX_CORE_INFO("No suitable vkDevice found!");
        SEDX_CORE_INFO("Requires graphics Queue Type: {}", ToString(queueType));
        SEDX_CORE_INFO("Requires graphics support present: {}", ToString(supportPresent));

        ErrMsg("No suitable vkDevice found!");

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
        if (deviceIndex < 0 || deviceIndex >= devices.size())
        {
            SEDX_CORE_ERROR("No GPU device selected or invalid device index.");
            ErrMsg("No GPU device selected.");
            throw std::runtime_error("No GPU device selected");
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

		if (QFamilyProperties.empty())
        {
            return indices;
        }

        /// Find dedicated compute queue (compute, but not graphics)
        if (qFlags & VK_QUEUE_COMPUTE_BIT)
        {
            for (uint32_t i = 0; i < QFamilyProperties.size(); i++)
            {
                const auto &props = QFamilyProperties[i];
                const bool supportsCompute = (props.queueFlags & VK_QUEUE_COMPUTE_BIT) != 0;
                if (const bool supportsGraphics = (props.queueFlags & VK_QUEUE_GRAPHICS_BIT) != 0; supportsCompute && !supportsGraphics)
                {
                    indices.Compute = i;
                    break;
                }
            }
        }

        /// Find dedicated transfer queue (transfer, but not graphics or compute)
        if (qFlags & VK_QUEUE_TRANSFER_BIT)
        {
            for (uint32_t i = 0; i < QFamilyProperties.size(); i++)
            {
                const auto &props = QFamilyProperties[i];
                const bool supportsTransfer = (props.queueFlags & VK_QUEUE_TRANSFER_BIT) != 0;
                const bool supportsGraphics = (props.queueFlags & VK_QUEUE_GRAPHICS_BIT) != 0;

                if (const bool supportsCompute = (props.queueFlags & VK_QUEUE_COMPUTE_BIT) != 0; supportsTransfer && !supportsGraphics && !supportsCompute)
                {
                    indices.Transfer = i;
                    break;
                }
            }
        }

        /// Find general-purpose queues if dedicated ones weren't found
        for (uint32_t i = 0; i < QFamilyProperties.size(); i++)
        {
            const auto &props = QFamilyProperties[i];

            /// Set transfer queue if not already set
            if ((qFlags & VK_QUEUE_TRANSFER_BIT) && indices.Transfer == -1)
            {
                if (props.queueFlags & VK_QUEUE_TRANSFER_BIT)
                {
                    indices.Transfer = i;
                }
            }

            /// Set compute queue if not already set
            if ((qFlags & VK_QUEUE_COMPUTE_BIT) && indices.Compute == -1)
            {
                if (props.queueFlags & VK_QUEUE_COMPUTE_BIT)
                {
                    indices.Compute = i;
                }
            }

            /// Set graphics queue
            if (qFlags & VK_QUEUE_GRAPHICS_BIT)
            {
                if (props.queueFlags & VK_QUEUE_GRAPHICS_BIT)
                {
                    indices.Graphics = i;
                }
            }
        }

        return indices;
    }

    /// -------------------------------------------------------
    /// VulkanDevice Implementation
    /// -------------------------------------------------------

	/**
	 * @brief Create a Vulkan device.
	 * @param physDevice - The physical device to create the logical device for.
	 * @param enabledFeatures - The features to enable for the logical device.
	 */
    VulkanDevice::VulkanDevice(const Ref<VulkanPhysicalDevice> &physDevice, VkPhysicalDeviceFeatures enabledFeatures)
        : vkPhysDevice(physDevice), vkEnabledFeatures(enabledFeatures)
    {

        QueueFamilyIndices indices = vkPhysDevice->FindQueueFamilies(vkPhysDevice->GetGPUDevice());
        VulkanChecks checks;

        /// Validate that necessary queue families were found
        if (!indices.IsComplete())
        {
            SEDX_CORE_ERROR("Could not find all required queue families.");
            ErrMsg("Could not find all required queue families.");
            return;
        }

        /// Create unique queue create infos for each queue family
        std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
        std::set uniqueQueueFamilies = {
            indices.graphicsFamily.value(),
            indices.presentFamily.value()
        };

        auto queuePriority = 1.0f;
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

        /// Verify extension support
        const auto &deviceExtensions = vkExtensions.requiredExtensions;
        if (!checks.CheckDeviceExtensionSupport(vkPhysDevice->GetGPUDevice()))
        {
            SEDX_CORE_ERROR("Required device extensions not supported!");
            ErrMsg("Required device extensions not supported!");
            return;
        }
		// ---------------------------------------------------------

        // Create the logical device
        VkDeviceCreateInfo createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
        createInfo.pQueueCreateInfos = queueCreateInfos.data();
        createInfo.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size());
        createInfo.pEnabledFeatures = &vkEnabledFeatures;
        createInfo.enabledExtensionCount = static_cast<uint32_t>(deviceExtensions.size());
        createInfo.ppEnabledExtensionNames = deviceExtensions.data();

        /// Add validation layers if enabled
        if (enableValidationLayers)
        {
            std::vector<const char *> validationLayers = {"VK_LAYER_KHRONOS_validation"};
            createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
            createInfo.ppEnabledLayerNames = validationLayers.data();
        }
        else
        {
            createInfo.enabledLayerCount = 0;
        }

        /// Create the logical device
        VkResult result = vkCreateDevice(vkPhysDevice->GetGPUDevice(), &createInfo, nullptr, &device);
        if (result != VK_SUCCESS)
        {
            SEDX_CORE_ERROR("Failed to create logical device! Error: {}", static_cast<int>(result));
            ErrMsg("Failed to create logical device!");
            return;
        }

        /// Get device queues
        vkGetDeviceQueue(device, indices.graphicsFamily.value(), 0, &GraphicsQueue);
        vkGetDeviceQueue(device, indices.presentFamily.value(), 0, &PresentQueue);

        /// Load device extension function pointers
        LoadExtensionFunctions();

        /// Initialize memory allocator
        InitializeMemoryAllocator();

        /// Set up bindless resources and initial buffers
        InitializeBindlessResources();

        /// Create initial scratch buffer
        scratchBuffer = CreateBuffer(initialScratchBufferSize, BufferUsage::Address | BufferUsage::Storage, 
                                    MemoryType::GPU, 
                                    "ScratchBuffer");

        /// Get the device address for the scratch buffer
        if (vkGetBufferDeviceAddressKHR != nullptr)
        {
            VkBufferDeviceAddressInfo scratchInfo{};
            scratchInfo.sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO;
            scratchInfo.buffer = scratchBuffer.resource->buffer;
            scratchAddress = vkGetBufferDeviceAddressKHR(device, &scratchInfo);
        }
    }

    void VulkanDevice::LoadExtensionFunctions()
    {
        // Load debug utils functions
        vkSetDebugUtilsObjectNameEXT = (PFN_vkSetDebugUtilsObjectNameEXT)vkGetDeviceProcAddr(device,"vkSetDebugUtilsObjectNameEXT");
        // Load ray tracing functions
        vkGetAccelerationStructureBuildSizesKHR = (PFN_vkGetAccelerationStructureBuildSizesKHR)vkGetDeviceProcAddr(device,"vkGetAccelerationStructureBuildSizesKHR");
        vkCreateAccelerationStructureKHR = (PFN_vkCreateAccelerationStructureKHR)vkGetDeviceProcAddr(device, "vkCreateAccelerationStructureKHR");
        vkGetBufferDeviceAddressKHR = (PFN_vkGetBufferDeviceAddressKHR)vkGetDeviceProcAddr(device, "vkGetBufferDeviceAddressKHR");
        vkCmdBuildAccelerationStructuresKHR = (PFN_vkCmdBuildAccelerationStructuresKHR)vkGetDeviceProcAddr(device, "vkCmdBuildAccelerationStructuresKHR");
        vkGetAccelerationStructureDeviceAddressKHR = (PFN_vkGetAccelerationStructureDeviceAddressKHR) vkGetDeviceProcAddr(device, "vkGetAccelerationStructureDeviceAddressKHR");
        vkDestroyAccelerationStructureKHR = (PFN_vkDestroyAccelerationStructureKHR)vkGetDeviceProcAddr(device, "vkDestroyAccelerationStructureKHR");
    }

    void VulkanDevice::InitializeMemoryAllocator()
    {
        memoryAllocator = CreateRef<MemoryAllocator>("VulkanDevice");
        memoryAllocator->Init(device, vkPhysDevice->GetGPUDevice(), GraphicsEngine::GetInstance());
    }

    void VulkanDevice::InitializeBindlessResources()
    {
        // Create ImGui descriptor pool
        const VkDescriptorPoolSize imguiPoolSizes[] = {
            {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1000},
            {VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1000}
        };

        VkDescriptorPoolCreateInfo imguiPoolInfo{};
        imguiPoolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
        imguiPoolInfo.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
        imguiPoolInfo.maxSets = 1024;
        imguiPoolInfo.poolSizeCount = sizeof(imguiPoolSizes) / sizeof(VkDescriptorPoolSize);
        imguiPoolInfo.pPoolSizes = imguiPoolSizes;

        VK_CHECK_RESULT(vkCreateDescriptorPool(device, &imguiPoolInfo, nullptr, &bindlessResources.imguiDescriptorPool))

        /// Initialize resource ID arrays
        for (int i = 0; i < bindlessResources.MAX_STORAGE; i++)
        {
            bindlessResources.availBufferRID.push_back(i);
        }

        for (int i = 0; i < bindlessResources.MAX_SAMPLED_IMAGES; i++)
        {
            bindlessResources.availImageRID.push_back(i);
        }

        // Create bindless descriptor pool
        std::vector<VkDescriptorPoolSize> bindlessPoolSizes = {
            {VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, bindlessResources.MAX_SAMPLED_IMAGES},
            {VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, bindlessResources.MAX_STORAGE},
            {VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, bindlessResources.MAX_STORAGE_IMAGES}
        };

        VkDescriptorPoolCreateInfo bindlessPoolInfo{};
        bindlessPoolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
        bindlessPoolInfo.flags = VK_DESCRIPTOR_POOL_CREATE_UPDATE_AFTER_BIND_BIT;
        bindlessPoolInfo.maxSets = 1;
        bindlessPoolInfo.poolSizeCount = static_cast<uint32_t>(bindlessPoolSizes.size());
        bindlessPoolInfo.pPoolSizes = bindlessPoolSizes.data();

        VK_CHECK_RESULT(vkCreateDescriptorPool(device, &bindlessPoolInfo, nullptr, &bindlessResources.bindlessDescriptorPool))

        /// Create bindless descriptor set layout
        std::vector<VkDescriptorSetLayoutBinding> bindings = {
            /// Textures binding
            {
                BindlessResources::TEXTURE,                // binding
                VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, // descriptorType
                bindlessResources.MAX_SAMPLED_IMAGES,      // descriptorCount
                VK_SHADER_STAGE_ALL,                       // stageFlags
                nullptr                                    // pImmutableSamplers
            },
            /// Storage buffers binding
            {
                BindlessResources::BUFFER,         // binding
                VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, // descriptorType
                bindlessResources.MAX_STORAGE,     // descriptorCount
                VK_SHADER_STAGE_ALL,               // stageFlags
                nullptr                            // pImmutableSamplers
            },
            /// Storage images binding
            {
                BindlessResources::STORAGE_IMAGE,     // binding
                VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,     // descriptorType
                bindlessResources.MAX_STORAGE_IMAGES, // descriptorCount
                VK_SHADER_STAGE_ALL,                  // stageFlags
                nullptr                               // pImmutableSamplers
            }};

        std::vector<VkDescriptorBindingFlags> bindingFlags = {
            VK_DESCRIPTOR_BINDING_PARTIALLY_BOUND_BIT | VK_DESCRIPTOR_BINDING_UPDATE_AFTER_BIND_BIT,
            VK_DESCRIPTOR_BINDING_PARTIALLY_BOUND_BIT,
            VK_DESCRIPTOR_BINDING_PARTIALLY_BOUND_BIT | VK_DESCRIPTOR_BINDING_UPDATE_AFTER_BIND_BIT
        };

        VkDescriptorSetLayoutBindingFlagsCreateInfo bindingFlagsInfo{};
        bindingFlagsInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_BINDING_FLAGS_CREATE_INFO;
        bindingFlagsInfo.bindingCount = static_cast<uint32_t>(bindingFlags.size());
        bindingFlagsInfo.pBindingFlags = bindingFlags.data();

        VkDescriptorSetLayoutCreateInfo layoutInfo{};
        layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
        layoutInfo.bindingCount = static_cast<uint32_t>(bindings.size());
        layoutInfo.pBindings = bindings.data();
        layoutInfo.flags = VK_DESCRIPTOR_SET_LAYOUT_CREATE_UPDATE_AFTER_BIND_POOL_BIT;
        layoutInfo.pNext = &bindingFlagsInfo;

        VK_CHECK_RESULT(vkCreateDescriptorSetLayout(device, &layoutInfo, nullptr, &bindlessResources.bindlessDescriptorLayout))

        /// Create bindless descriptor set
        VkDescriptorSetAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
        allocInfo.descriptorPool = bindlessResources.bindlessDescriptorPool;
        allocInfo.descriptorSetCount = 1;
        allocInfo.pSetLayouts = &bindlessResources.bindlessDescriptorLayout;

        VK_CHECK_RESULT(vkAllocateDescriptorSets(device, &allocInfo, &bindlessResources.bindlessDescriptorSet))
    }

	VulkanDevice::~VulkanDevice()
    {
        /// Wait for device to be idle before cleanup
        if (device != VK_NULL_HANDLE)
        {
            vkDeviceWaitIdle(device);
        }

        /// Clean up command pools
        CmdPools.clear();

        /// Clean up scratch buffer
        scratchBuffer = {};

        /// Clean up bindless resources
        if (device != VK_NULL_HANDLE)
        {
            if (bindlessResources.bindlessDescriptorLayout != VK_NULL_HANDLE)
            {
                vkDestroyDescriptorSetLayout(device, bindlessResources.bindlessDescriptorLayout, nullptr);
            }

            if (bindlessResources.bindlessDescriptorPool != VK_NULL_HANDLE)
            {
                vkDestroyDescriptorPool(device, bindlessResources.bindlessDescriptorPool, nullptr);
            }

            if (bindlessResources.imguiDescriptorPool != VK_NULL_HANDLE)
            {
                vkDestroyDescriptorPool(device, bindlessResources.imguiDescriptorPool, nullptr);
            }
        }

        /// Shutdown memory allocator
        if (memoryAllocator)
        {
            memoryAllocator->Shutdown();
            memoryAllocator = nullptr;
        }

        /// Destroy logical device
        if (device != VK_NULL_HANDLE)
        {
            vkDestroyDevice(device, nullptr);
            device = VK_NULL_HANDLE;
        }
    }

    VmaAllocator VulkanDevice::GetMemoryAllocator()
    {
        if (memoryAllocator)
        {
            return memoryAllocator->GetMemAllocator();
        }

        SEDX_CORE_ERROR("Memory allocator not initialized.");
        ErrMsg("Memory allocator not initialized.");
        return nullptr;
    }


    void VulkanDevice::Destroy()
    {
        /// Clear command pools
        CmdPools.clear();

        /// Wait for device to be idle
        if (device != VK_NULL_HANDLE)
        {
            vkDeviceWaitIdle(device);
            vkDestroyDevice(device, nullptr);
            device = VK_NULL_HANDLE;
        }
    }

    void VulkanDevice::LockQueue(const bool compute)
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

    void VulkanDevice::UnlockQueue(const bool compute)
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
		    for (uint32_t idx = 0; idx < selectedDevice.queueFamilyInfo.size(); idx++)
			{
                if (const VkQueueFamilyProperties &queueFamily = selectedDevice.queueFamilyInfo[idx]; queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT)
				{
		            indices.graphicsFamily = idx;
		            // Store in our other format too for consistency
		            indices.Graphics = idx;
		        }

		        // Check presentation support
		        if (selectedDevice.queueSupportPresent[idx])
				{
		            indices.presentFamily = idx;
		        }
		        if (indices.IsComplete())
				{
		            break;
		        }
		    }
		}
	    
	    return indices;
	}

	const VkDevice &VulkanDevice::Selected() const { return device; }

    VkCommandBuffer VulkanDevice::CreateSecondaryCommandBuffer(const char *debugName)
    {
        /// Get the command pool for the current thread
        Ref<CommandPool> cmdPool = GetOrCreateThreadLocalCmdPool();

        VkCommandBufferAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        allocInfo.commandPool = cmdPool->GetGraphicsCmdPool();
        allocInfo.level = VK_COMMAND_BUFFER_LEVEL_SECONDARY;
        allocInfo.commandBufferCount = 1;

        VkCommandBuffer cmdBuffer;
        VK_CHECK_RESULT(vkAllocateCommandBuffers(device, &allocInfo, &cmdBuffer))

        /// Set debug name if available
        if (debugName && vkSetDebugUtilsObjectNameEXT)
        {
            VkDebugUtilsObjectNameInfoEXT nameInfo{};
            nameInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT;
            nameInfo.objectType = VK_OBJECT_TYPE_COMMAND_BUFFER;
            nameInfo.objectHandle = (uint64_t)cmdBuffer;
            nameInfo.pObjectName = debugName;
            vkSetDebugUtilsObjectNameEXT(device, &nameInfo);
        }

        return cmdBuffer;

    }

    Ref<CommandPool> VulkanDevice::GetThreadLocalCmdPool()
    {
        const auto threadID = std::this_thread::get_id();
        const auto it = CmdPools.find(threadID);
        if (it == CmdPools.end())
        {
            SEDX_CORE_WARN("Command pool for thread {} not found. Creating a new one.",
                           std::hash<std::thread::id>{}(threadID));
            return GetOrCreateThreadLocalCmdPool();
        }

        return it->second;
    }

    Ref<CommandPool> VulkanDevice::GetOrCreateThreadLocalCmdPool()
    {
        const auto threadID = std::this_thread::get_id();
        if (const auto it = CmdPools.find(threadID); it != CmdPools.end())
        {
            return it->second;
        }

        /// Create a new command pool for this thread pass reference to current instance.
        Ref<CommandPool> commandPool = CreateRef<CommandPool>(GetInstance());
        CmdPools[threadID] = commandPool;

        SEDX_CORE_INFO("Created new command pool for thread {}", std::hash<std::thread::id>{}(threadID));

        return commandPool;
    }

	// -------------------------------------------------------

    VkSampleCountFlagBits VulkanDevice::GetMaxUsableSampleCount() const
    {
        VkPhysicalDeviceProperties physicalDeviceProperties;
        vkGetPhysicalDeviceProperties(vkPhysDevice->GetGPUDevice(), &physicalDeviceProperties);

        VkSampleCountFlags counts = physicalDeviceProperties.limits.framebufferColorSampleCounts;
        counts &= physicalDeviceProperties.limits.framebufferDepthSampleCounts;

        /// Get the highest sample count that is supported
        if (counts & VK_SAMPLE_COUNT_64_BIT)
            return VK_SAMPLE_COUNT_64_BIT;
        if (counts & VK_SAMPLE_COUNT_32_BIT)
            return VK_SAMPLE_COUNT_32_BIT;
        if (counts & VK_SAMPLE_COUNT_16_BIT)
            return VK_SAMPLE_COUNT_16_BIT;
        if (counts & VK_SAMPLE_COUNT_8_BIT)
            return VK_SAMPLE_COUNT_8_BIT;
        if (counts & VK_SAMPLE_COUNT_4_BIT)
            return VK_SAMPLE_COUNT_4_BIT;
        if (counts & VK_SAMPLE_COUNT_2_BIT)
            return VK_SAMPLE_COUNT_2_BIT;

        return VK_SAMPLE_COUNT_1_BIT;
    }

    /*
    void VulkanDevice::CreateDeviceFeatures2()
    {
		std::set<uint32_t> uniqueFamilies;
		for (int q = 0; q < Queue::Count; q++)
		{
		    uniqueFamilies.emplace(queues[q].family);
		};

		// priority for each type of queue
		float priority = 1.0f;
		std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
		for (uint32_t family : uniqueFamilies) {
		    VkDeviceQueueCreateInfo createInfo{};
		    createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
		    createInfo.queueFamilyIndex = family;
		    createInfo.queueCount = 1;
		    createInfo.pQueuePriorities = &priority;
		    queueCreateInfos.push_back(createInfo);
		}

		auto supportedFeatures = GetPhysicalDevice()->Selected().GFXFeatures;

		// logical device features
		VkPhysicalDeviceFeatures2 features2 = {};
		features2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
		features2.features.geometryShader = VK_TRUE;
		if (supportedFeatures.logicOp)           { features2.features.logicOp           = VK_TRUE; }
		if (supportedFeatures.samplerAnisotropy) { features2.features.samplerAnisotropy = VK_TRUE; }
		if (supportedFeatures.sampleRateShading) { features2.features.sampleRateShading = VK_TRUE; }
		if (supportedFeatures.fillModeNonSolid)  { features2.features.fillModeNonSolid  = VK_TRUE; }
		if (supportedFeatures.wideLines)         { features2.features.wideLines         = VK_TRUE; }
		if (supportedFeatures.depthClamp)        { features2.features.depthClamp        = VK_TRUE; }

		auto requiredExtensions = Extensions::requiredExtensions;
        auto allExtensions = Extensions::availableExtensions;
		for (auto req : requiredExtensions)
		{
		    bool available = false;
		    for (size_t i = 0; i < allExtensions.size(); i++)
			{
		        if (strcmp(allExtensions[i].extensionName, req) == 0)
				{ 
		            available = true; 
		            break;
		        }
		    }

		    if(!available)
			{
		        SEDX_CORE_ERROR("Required extension {0} not available!", req);
		    }
		}

		VkPhysicalDeviceDescriptorIndexingFeatures descriptorIndexingFeatures{};
		descriptorIndexingFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DESCRIPTOR_INDEXING_FEATURES;
		descriptorIndexingFeatures.runtimeDescriptorArray = true;
		descriptorIndexingFeatures.descriptorBindingPartiallyBound = true;
		descriptorIndexingFeatures.shaderSampledImageArrayNonUniformIndexing = true;
		descriptorIndexingFeatures.shaderUniformBufferArrayNonUniformIndexing = true;
		descriptorIndexingFeatures.shaderStorageBufferArrayNonUniformIndexing = true;
		descriptorIndexingFeatures.descriptorBindingSampledImageUpdateAfterBind = true;
		descriptorIndexingFeatures.descriptorBindingStorageImageUpdateAfterBind = true;

		VkPhysicalDeviceBufferDeviceAddressFeatures bufferDeviceAddresFeatures{};
		bufferDeviceAddresFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_BUFFER_DEVICE_ADDRESS_FEATURES;
		bufferDeviceAddresFeatures.bufferDeviceAddress = VK_TRUE;
		bufferDeviceAddresFeatures.pNext = &descriptorIndexingFeatures;

		VkPhysicalDeviceAccelerationStructureFeaturesKHR accelerationStructureFeatures{};
		accelerationStructureFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ACCELERATION_STRUCTURE_FEATURES_KHR;
		accelerationStructureFeatures.accelerationStructure = VK_TRUE;
		accelerationStructureFeatures.descriptorBindingAccelerationStructureUpdateAfterBind = VK_TRUE;
		accelerationStructureFeatures.accelerationStructureCaptureReplay = VK_TRUE;
        accelerationStructureFeatures.pNext = &bufferDeviceAddresFeatures;

		VkPhysicalDeviceDynamicRenderingFeaturesKHR dynamicRenderingFeatures{};
		dynamicRenderingFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DYNAMIC_RENDERING_FEATURES_KHR;
		dynamicRenderingFeatures.dynamicRendering = VK_TRUE;
        dynamicRenderingFeatures.pNext = &accelerationStructureFeatures;

		VkPhysicalDeviceSynchronization2FeaturesKHR sync2Features{};
		sync2Features.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SYNCHRONIZATION_2_FEATURES_KHR;
		sync2Features.synchronization2 = VK_TRUE;
		sync2Features.pNext = &dynamicRenderingFeatures;

		VkPhysicalDeviceShaderAtomicFloatFeaturesEXT atomicFeatures{};
		atomicFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SHADER_ATOMIC_FLOAT_FEATURES_EXT;
		atomicFeatures.shaderBufferFloat32AtomicAdd = VK_TRUE;
		atomicFeatures.pNext = &sync2Features;

		features2.pNext = &atomicFeatures;

		VkDeviceCreateInfo createInfo{};
		createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
		createInfo.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size());
		createInfo.pQueueCreateInfos = queueCreateInfos.data();
		createInfo.enabledExtensionCount = static_cast<uint32_t>(requiredExtensions.size());
		createInfo.ppEnabledExtensionNames = requiredExtensions.data();
        createInfo.pEnabledFeatures;
		createInfo.pNext = &features2;

		// specify the required layers to the device 
		if (enableValidationLayers)
		{
            auto &layers = Layers::activeLayersNames;
		    createInfo.enabledLayerCount = static_cast<uint32_t>(layers.size());
		    createInfo.ppEnabledLayerNames = layers.data();
		}
		else {
		    createInfo.enabledLayerCount = 0;
		}

		auto res = vkCreateDevice(GetPhysicalDevice()->physicalDevice, &createInfo, renderData.allocator, &device);
		SEDX_ASSERT(res, "Failed to create logical device!");

		VmaVulkanFunctions vulkanFunctions = {};
		vulkanFunctions.vkGetInstanceProcAddr = &vkGetInstanceProcAddr;
		vulkanFunctions.vkGetDeviceProcAddr = &vkGetDeviceProcAddr;

		VmaAllocatorCreateInfo allocatorCreateInfo = {};
		allocatorCreateInfo.flags = VMA_ALLOCATOR_CREATE_EXT_MEMORY_BUDGET_BIT | VMA_ALLOCATOR_CREATE_BUFFER_DEVICE_ADDRESS_BIT;
		allocatorCreateInfo.vulkanApiVersion = VK_API_VERSION_1_3;
        allocatorCreateInfo.physicalDevice = GetPhysicalDevice()->physicalDevice;
		allocatorCreateInfo.device = device;
		allocatorCreateInfo.instance = vkInstance;
		allocatorCreateInfo.pVulkanFunctions = &vulkanFunctions;
        vmaCreateAllocator(&allocatorCreateInfo, &MemoryAllocator::memAllocatorData->Allocator);

		for (int q = 0; q < Queue::Count; q++)
		{
		    vkGetDeviceQueue(device, queues[q].family, 0, &queues[q].queue);
		}

		// ----------------------------------------------

		renderData.baseSampler = CreateSampler(1.0);
		vkSetDebugUtilsObjectNameEXT = (PFN_vkSetDebugUtilsObjectNameEXT)vkGetDeviceProcAddr(device, "vkSetDebugUtilsObjectNameEXT");
		vkGetAccelerationStructureBuildSizesKHR = (PFN_vkGetAccelerationStructureBuildSizesKHR)vkGetDeviceProcAddr(device, "vkGetAccelerationStructureBuildSizesKHR");
		vkCreateAccelerationStructureKHR = (PFN_vkCreateAccelerationStructureKHR)vkGetDeviceProcAddr(device, "vkCreateAccelerationStructureKHR");
		vkCmdBuildAccelerationStructuresKHR = (PFN_vkCmdBuildAccelerationStructuresKHR)vkGetDeviceProcAddr(device, "vkCmdBuildAccelerationStructuresKHR");
		vkGetAccelerationStructureDeviceAddressKHR = (PFN_vkGetAccelerationStructureDeviceAddressKHR)vkGetDeviceProcAddr(device, "vkGetAccelerationStructureDeviceAddressKHR");
		vkDestroyAccelerationStructureKHR = (PFN_vkDestroyAccelerationStructureKHR)vkGetDeviceProcAddr(device, "vkDestroyAccelerationStructureKHR");

		// ---------------------------------------------------------

		VkDescriptorPoolSize imguiPoolSizes[] = {
		    {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1000},
		    {VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1000}
		};

		VkDescriptorPoolCreateInfo imguiPoolInfo{};
		imguiPoolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
		imguiPoolInfo.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
		imguiPoolInfo.maxSets = (uint32_t)(1024);
		imguiPoolInfo.poolSizeCount = sizeof(imguiPoolSizes)/sizeof(VkDescriptorPoolSize);
		imguiPoolInfo.pPoolSizes = imguiPoolSizes;

		VkResult result = vkCreateDescriptorPool(device, &imguiPoolInfo, renderData.allocator, &bindlessResources.imguiDescriptorPool);
		SEDX_ASSERT(result, "Failed to create imgui descriptor pool!");

		// Create bindless resources
		{
            BindlessResources bindlessRes;
            for (int i = 0; i < bindlessRes.MAX_STORAGE; i++)
			{
                bindlessResources.availBufferRID.push_back(i);
		    }
            for (int i = 0; i < bindlessRes.MAX_SAMPLED_IMAGES; i++)
			{
                bindlessResources.availImageRID.push_back(i);
		    }

		    // Create descriptor set pool for bindless resources
		    std::vector<VkDescriptorPoolSize> bindlessPoolSizes =
			{ 
		        {VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, bindlessRes.MAX_SAMPLED_IMAGES},
                {VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, bindlessRes.MAX_STORAGE},
                {VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, bindlessRes.MAX_STORAGE_IMAGES},
		    };

		    VkDescriptorPoolCreateInfo bindlessPoolInfo{};
		    bindlessPoolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
		    bindlessPoolInfo.flags = VK_DESCRIPTOR_POOL_CREATE_UPDATE_AFTER_BIND_BIT;
		    bindlessPoolInfo.maxSets = 1;
		    bindlessPoolInfo.poolSizeCount = bindlessPoolSizes.size();
		    bindlessPoolInfo.pPoolSizes = bindlessPoolSizes.data();

		    result = vkCreateDescriptorPool(device, &bindlessPoolInfo, renderData.allocator, &bindlessRes.bindlessDescriptorPool);
		    SEDX_ASSERT(result, "Failed to create bindless descriptor pool!");

		    // create descriptor set layout for bindless resources
		    std::vector<VkDescriptorSetLayoutBinding> bindings;
		    std::vector<VkDescriptorBindingFlags> bindingFlags;

		    VkDescriptorSetLayoutBinding texturesBinding{};
            texturesBinding.binding = BindlessResources::TEXTURE;
		    texturesBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
            texturesBinding.descriptorCount = bindlessRes.MAX_SAMPLED_IMAGES;
		    texturesBinding.stageFlags = VK_SHADER_STAGE_ALL;
		    bindings.push_back(texturesBinding);
		    bindingFlags.push_back({ VK_DESCRIPTOR_BINDING_PARTIALLY_BOUND_BIT | VK_DESCRIPTOR_BINDING_UPDATE_AFTER_BIND_BIT });

		    VkDescriptorSetLayoutBinding storageBuffersBinding{};
            storageBuffersBinding.binding = BindlessResources::BUFFER;
		    storageBuffersBinding.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
            storageBuffersBinding.descriptorCount = bindlessRes.MAX_STORAGE;
		    storageBuffersBinding.stageFlags = VK_SHADER_STAGE_ALL;
		    bindings.push_back(storageBuffersBinding);
		    bindingFlags.push_back({ VK_DESCRIPTOR_BINDING_PARTIALLY_BOUND_BIT });

		    VkDescriptorSetLayoutBinding imageStorageBinding{};
            imageStorageBinding.binding = BindlessResources::STORAGE_IMAGE;
		    imageStorageBinding.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
            imageStorageBinding.descriptorCount = bindlessRes.MAX_STORAGE_IMAGES;
		    imageStorageBinding.stageFlags = VK_SHADER_STAGE_ALL;
		    bindings.push_back(imageStorageBinding);
		    bindingFlags.push_back({ VK_DESCRIPTOR_BINDING_PARTIALLY_BOUND_BIT | VK_DESCRIPTOR_BINDING_UPDATE_AFTER_BIND_BIT });

		    VkDescriptorSetLayoutBindingFlagsCreateInfo setLayoutBindingFlags{};
		    setLayoutBindingFlags.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_BINDING_FLAGS_CREATE_INFO;
		    setLayoutBindingFlags.bindingCount = bindingFlags.size();
		    setLayoutBindingFlags.pBindingFlags = bindingFlags.data();

		    VkDescriptorSetLayoutCreateInfo descriptorLayoutInfo{};
		    descriptorLayoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
		    descriptorLayoutInfo.bindingCount = bindings.size();
		    descriptorLayoutInfo.pBindings = bindings.data();
		    descriptorLayoutInfo.flags = VK_DESCRIPTOR_SET_LAYOUT_CREATE_UPDATE_AFTER_BIND_POOL_BIT;
		    descriptorLayoutInfo.pNext = &setLayoutBindingFlags;

		    result = vkCreateDescriptorSetLayout(device, &descriptorLayoutInfo, renderData.allocator, &bindlessRes.bindlessDescriptorLayout);
            SEDX_ASSERT(result, "Failed to create bindless descriptor set layout!");

		    // create descriptor set for bindless resources
		    VkDescriptorSetAllocateInfo allocInfo{};
		    allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
            allocInfo.descriptorPool = bindlessRes.bindlessDescriptorPool;
		    allocInfo.descriptorSetCount = 1;
            allocInfo.pSetLayouts = &bindlessRes.bindlessDescriptorLayout;

		    result = vkAllocateDescriptorSets(device, &allocInfo, &bindlessRes.bindlessDescriptorSet);
            SEDX_ASSERT(result, "Failed to allocate bindless descriptor set!");
		}

		scratchBuffer = CreateBuffer(initialScratchBufferSize, BufferUsage::Address | BufferUsage::Storage, MemoryType::GPU);
		VkBufferDeviceAddressInfo scratchInfo{};
		scratchInfo.sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO;
		scratchInfo.buffer = scratchBuffer.resource->buffer;
		scratchAddress = vkGetBufferDeviceAddress(device, &scratchInfo);

		/*
		dummyVertexBuffer = CreateBuffer(6 * 3 * sizeof(float), BufferUsage::Vertex | BufferUsage::AccelerationStructureInput, MemoryType::GPU,"VertexBuffer#Dummy" );
        #1#
    }
    */

    Buffer VulkanDevice::CreateBuffer(uint32_t size, BufferUsageFlags usage, MemoryFlags memory, const std::string &name)
    {
        /// Adjust buffer usage flags based on usage requirements
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

            /// Align storage buffer size to meet device requirements
            const auto alignment = vkPhysDevice->GetLimits().minStorageBufferOffsetAlignment;
            size = (size + alignment - 1) & ~(alignment - 1);
        }

        if (usage & BufferUsage::AccelerationStructureInput)
        {
            usage |= BufferUsage::Address | BufferUsage::TransferDst;
        }

        if (usage & BufferUsage::AccelerationStructure)
        {
            usage |= BufferUsage::Address;
        }

        /// Create buffer resource
        std::shared_ptr<BufferResource> resource = std::make_shared<BufferResource>();
        resource->resourceID = -1;
        resource->name = name;

        /// Set up buffer creation info
        VkBufferCreateInfo bufferInfo{};
        bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        bufferInfo.size = size;
        bufferInfo.usage = static_cast<VkBufferUsageFlags>(usage);
        bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

        /// Determine allocation strategy based on memory type
        VmaMemoryUsage vmaUsage = VMA_MEMORY_USAGE_AUTO;

        if (memory & MemoryType::CPU)
        {
            /// CPU accessible memory
            vmaUsage = VMA_MEMORY_USAGE_AUTO_PREFER_HOST;
            memoryAllocator->SetAllocationStrategy(MemoryAllocator::AllocationStrategy::SpeedOptimized);
        }
        else
        {
            /// GPU-only memory
            vmaUsage = VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE;
            memoryAllocator->SetAllocationStrategy(MemoryAllocator::AllocationStrategy::MemoryOptimized);
        }

        /// Allocate buffer memory
        resource->allocation = memoryAllocator->AllocateBuffer(bufferInfo, vmaUsage, resource->buffer);

        if (!resource->allocation)
        {
            SEDX_CORE_ERROR("Failed to allocate buffer memory for '{}', size: {} bytes", name, size);
            ErrMsg("Failed to allocate buffer memory");
            return {};
        }

        /// For large buffers, mark for potential defragmentation
        if (size > 16 * 1024 * 1024)
        {
            memoryAllocator->MarkForDefragmentation(resource->allocation);
        }

        /// Create buffer object
        Buffer buffer = {.resource = resource, .size = size, .usage = usage, .memory = memory};

        /// Set up storage buffer binding if needed
        if ((usage & BufferUsage::Storage) && !bindlessResources.availBufferRID.empty())
        {
            resource->resourceID = bindlessResources.availBufferRID.back();
            bindlessResources.availBufferRID.pop_back();

            VkDescriptorBufferInfo descriptorInfo{};
            descriptorInfo.buffer = resource->buffer;
            descriptorInfo.offset = 0;
            descriptorInfo.range = size;

            VkWriteDescriptorSet write{};
            write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            write.dstSet = bindlessResources.bindlessDescriptorSet;
            write.dstBinding = BindlessResources::BUFFER;
            write.dstArrayElement = buffer.ResourceID();
            write.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
            write.descriptorCount = 1;
            write.pBufferInfo = &descriptorInfo;

            vkUpdateDescriptorSets(device, 1, &write, 0, nullptr);
        }

        /// Set debug name if debugging is enabled
        if (!name.empty() && vkSetDebugUtilsObjectNameEXT)
        {
            VkDebugUtilsObjectNameInfoEXT nameInfo{};
            nameInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT;
            nameInfo.objectType = VK_OBJECT_TYPE_BUFFER;
            nameInfo.objectHandle = (uint64_t)resource->buffer;
            nameInfo.pObjectName = name.c_str();
            vkSetDebugUtilsObjectNameEXT(device, &nameInfo);
        }

        SEDX_CORE_TRACE("Created buffer '{}': size={} bytes, usage={:#x}",
                        name.empty() ? "Unnamed" : name,
                        size,
                        usage);

        return buffer;
    }

    Buffer VulkanDevice::CreateStagingBuffer(uint32_t size, const std::string &name)
    {
        return CreateBuffer(size, BufferUsage::TransferSrc, MemoryType::CPU, name.empty() ? "Staging Buffer" : name);
    }

    // TODO: Create separate one for shadow maps
    VkSampler VulkanDevice::CreateSampler(float maxLOD) const
    {
        VkSamplerCreateInfo samplerInfo{};
        samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
        samplerInfo.magFilter = VK_FILTER_LINEAR;
        samplerInfo.minFilter = VK_FILTER_LINEAR;
        samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
        samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
        samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;

        // Check if anisotropy is supported
        VkPhysicalDeviceFeatures deviceFeatures;
        vkGetPhysicalDeviceFeatures(vkPhysDevice->GetGPUDevice(), &deviceFeatures);

        if (deviceFeatures.samplerAnisotropy)
        {
            samplerInfo.anisotropyEnable = VK_TRUE;
            samplerInfo.maxAnisotropy = vkPhysDevice->GetLimits().maxSamplerAnisotropy;
        }
        else
        {
            samplerInfo.anisotropyEnable = VK_FALSE;
            samplerInfo.maxAnisotropy = 1.0f;
        }

        samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
        samplerInfo.unnormalizedCoordinates = VK_FALSE;
        samplerInfo.compareEnable = VK_FALSE;
        samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;
        samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
        samplerInfo.mipLodBias = 0.0f;
        samplerInfo.minLod = 0.0f;
        samplerInfo.maxLod = maxLOD;

        VkSampler sampler;
        VK_CHECK_RESULT(vkCreateSampler(device, &samplerInfo, nullptr, &sampler))

        return sampler;
    }

    /// -------------------------------------------------------
    /// CommandPool Implementation
    /// -------------------------------------------------------

    CommandPool::CommandPool(Ref<VulkanDevice> vulkanDevice) : device(std::move(vulkanDevice)), GraphicsCmdPool(VK_NULL_HANDLE), ComputeCmdPool(VK_NULL_HANDLE)
    {
        const auto vulkanDeviceHandle = device->GetDevice();
        const auto &queueIndices = device->GetPhysicalDevice()->GetQueueFamilyIndices();

        /// Create graphics command pool
        VkCommandPoolCreateInfo cmdPoolInfo{};
        cmdPoolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
        cmdPoolInfo.queueFamilyIndex = queueIndices.Graphics;
        cmdPoolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;

        VkResult result = vkCreateCommandPool(vulkanDeviceHandle, &cmdPoolInfo, nullptr, &GraphicsCmdPool);
        if (result != VK_SUCCESS)
        {
            SEDX_CORE_ERROR("Failed to create graphics command pool! Error: {}", static_cast<int>(result));
            throw std::runtime_error("Failed to create graphics command pool");
        }

        /// Create compute command pool if compute queue is available
        if (queueIndices.Compute >= 0)
        {
            cmdPoolInfo.queueFamilyIndex = queueIndices.Compute;
            result = vkCreateCommandPool(vulkanDeviceHandle, &cmdPoolInfo, nullptr, &ComputeCmdPool);

            if (result != VK_SUCCESS)
            {
                SEDX_CORE_ERROR("Failed to create compute command pool! Error: {}", static_cast<int>(result));
                /// Fall back to using graphics pool for compute operations
                ComputeCmdPool = GraphicsCmdPool;
            }
        }
        else
        {
            /// If no separate compute queue, use the graphics pool
            ComputeCmdPool = GraphicsCmdPool;
        }
    }

    CommandPool::~CommandPool()
    {
        if (!device || !device->GetDevice())
        {
            return;
        }

        const auto vulkanDevice = device->GetDevice();

        /// Only destroy compute pool if it's different from graphics pool
        if (ComputeCmdPool != VK_NULL_HANDLE && ComputeCmdPool != GraphicsCmdPool)
        {
            vkDestroyCommandPool(vulkanDevice, ComputeCmdPool, nullptr);
        }

        if (GraphicsCmdPool != VK_NULL_HANDLE)
        {
            vkDestroyCommandPool(vulkanDevice, GraphicsCmdPool, nullptr);
        }

        GraphicsCmdPool = VK_NULL_HANDLE;
        ComputeCmdPool = VK_NULL_HANDLE;
    }

    VkCommandBuffer CommandPool::AllocateCommandBuffer(bool begin, bool compute) const
    {
        const auto vulkanDevice = device->GetDevice();
        const VkCommandPool cmdPool = compute ? ComputeCmdPool : GraphicsCmdPool;

        VkCommandBufferAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        allocInfo.commandPool = cmdPool;
        allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        allocInfo.commandBufferCount = 1;

        VkCommandBuffer cmdBuffer;
        VkResult result = vkAllocateCommandBuffers(vulkanDevice, &allocInfo, &cmdBuffer);

        if (result != VK_SUCCESS)
        {
            SEDX_CORE_ERROR("Failed to allocate command buffer! Error: {}", static_cast<int>(result));
            return VK_NULL_HANDLE;
        }

        if (begin)
        {
            VkCommandBufferBeginInfo beginInfo{};
            beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
            beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

            result = vkBeginCommandBuffer(cmdBuffer, &beginInfo);
            if (result != VK_SUCCESS)
            {
                SEDX_CORE_ERROR("Failed to begin command buffer! Error: {}", static_cast<int>(result));
                vkFreeCommandBuffers(vulkanDevice, cmdPool, 1, &cmdBuffer);
                return VK_NULL_HANDLE;
            }
        }

        return cmdBuffer;
    }

    void CommandPool::FlushCmdBuffer(const VkCommandBuffer cmdBuffer) const
    {
        FlushCmdBuffer(cmdBuffer, device->GetGraphicsQueue());
    }

    void CommandPool::FlushCmdBuffer(const VkCommandBuffer cmdBuffer, const VkQueue queue) const
    {
        if (cmdBuffer == VK_NULL_HANDLE)
        {
            SEDX_CORE_WARN("Attempted to flush a null command buffer");
            return;
        }

        const auto vulkanDevice = device->GetDevice();

        /// End the command buffer
        VkResult result = vkEndCommandBuffer(cmdBuffer);
        if (result != VK_SUCCESS)
        {
            SEDX_CORE_ERROR("Failed to end command buffer! Error: {}", static_cast<int>(result));
            return;
        }

        /// Create a fence to wait for the command buffer to complete
        VkFenceCreateInfo fenceInfo{};
        fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;

        VkFence fence;
        result = vkCreateFence(vulkanDevice, &fenceInfo, nullptr, &fence);
        if (result != VK_SUCCESS)
        {
            SEDX_CORE_ERROR("Failed to create fence! Error: {}", static_cast<int>(result));
            return;
        }

        /// Submit the command buffer
        VkSubmitInfo submitInfo{};
        submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        submitInfo.commandBufferCount = 1;
        submitInfo.pCommandBuffers = &cmdBuffer;

        result = vkQueueSubmit(queue, 1, &submitInfo, fence);
        if (result != VK_SUCCESS)
        {
            SEDX_CORE_ERROR("Failed to submit command buffer! Error: {}", static_cast<int>(result));
            vkDestroyFence(vulkanDevice, fence, nullptr);
            return;
        }

        /// Wait for the fence
        result = vkWaitForFences(vulkanDevice, 1, &fence, VK_TRUE, DEFAULT_FENCE_TIMEOUT);
        if (result != VK_SUCCESS)
        {
            SEDX_CORE_ERROR("Failed to wait for fence! Error: {}", static_cast<int>(result));
        }

        /// Clean up
        vkDestroyFence(vulkanDevice, fence, nullptr);
        vkFreeCommandBuffers(vulkanDevice, GraphicsCmdPool, 1, &cmdBuffer);
    }

} // namespace SceneryEditorX

// -------------------------------------------------------
