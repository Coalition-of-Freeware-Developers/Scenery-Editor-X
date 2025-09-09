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
#include <SceneryEditorX/renderer/render_context.h>
#include <SceneryEditorX/renderer/vulkan/vk_allocator.h>
#include <SceneryEditorX/renderer/vulkan/vk_checks.h>
#include <SceneryEditorX/renderer/vulkan/vk_device.h>
#include <SceneryEditorX/renderer/vulkan/vk_util.h>
#include <SceneryEditorX/renderer/bindless_descriptor_manager.h>

/// -------------------------------------------------------

namespace SceneryEditorX
{
	//////////////////////////////////////////////////////////
	/// VulkanPhysicalDevice Implementation
	//////////////////////////////////////////////////////////

    /**
     * @fn VulkanPhysicalDevice::VulkanPhysicalDevice
     * @brief Constructor that enumerates and initializes available physical GPU devices
     *
     * @details This constructor performs the following operations:
     * 1. Retrieves the Vulkan instance from the Graphics Engine
     * 2. Enumerates all physical devices (GPUs) available in the system
     * 3. Stores device handles and queries detailed device properties
     * 4. Prioritizes discrete GPUs over integrated GPUs
     * 5. Logs detailed information about each discovered GPU
     * 6. Queries device features, memory properties, queue families, and extension support
     * 7. Identifies and configures dedicated graphics, compute, and transfer queues
     * 8. Sets up queue create infos required for logical device creation
     * 9. Determines appropriate depth buffer formats supported by the device
     *
     * The constructor first attempts to find a discrete GPU for optimal performance.
     * If none is found, it logs an error as the engine currently requires a discrete GPU.
     * For each device, it queries comprehensive hardware capabilities and prepares queue
     * configurations that will later be used for logical device creation.
     *
     * @note - This constructor doesn't create a logical device - it only prepares the physical device
     *       information needed for logical device creation in the VulkanDevice class.
     * @note - Errors during device enumeration or if no discrete GPU is found are logged
     *       but don't throw exceptions.
     *
     * @see VulkanDevice, GetQueueFamilyIndices, FindDepthFormat
     */
    VulkanPhysicalDevice::VulkanPhysicalDevice(VkInstance &instance)
    {
        this->vkInstance = &instance;                      /// Store the Vulkan instance

        uint32_t GFXDevices = 0;                           /// Number of physical devices
        vkEnumeratePhysicalDevices(*vkInstance, &GFXDevices, nullptr);
        //VK_CHECK_RESULT(vkEnumeratePhysicalDevices(vkInstance, &GFXDevices, device.data()))

        std::vector<VkPhysicalDevice> device(GFXDevices); /// Vector to hold physical devices
        if (GFXDevices > 0)
        {
            device.resize(GFXDevices);
            if (const VkResult result = vkEnumeratePhysicalDevices(*vkInstance, &GFXDevices, device.data()); result != VK_SUCCESS || GFXDevices == 0)
            {
                SEDX_CORE_ERROR_TAG("Graphics Engine","Could not enumerate physical devices.");
                return;
            }

            /// Resize the devices vector to accommodate the number of physical devices
            devices.resize(GFXDevices);
            for (uint32_t index = 0; index < GFXDevices; index++)
            {
                const VkPhysicalDevice physicalDevice = device[index];  /// Get device properties
                devices[index].physicalDevice         = physicalDevice; /// Store device properties
            }

            device.resize(GFXDevices);

            if (const VkResult result = vkEnumeratePhysicalDevices(*vkInstance, &GFXDevices, device.data()); result != VK_SUCCESS || GFXDevices == 0)
            {
                SEDX_CORE_ERROR_TAG("Graphics Engine", "Could not enumerate physical devices.");
                return;
            }

            /// Resize the devices vector to accommodate the number of physical devices
            devices.resize(GFXDevices);
            /// Get device properties for each physical device
            VkPhysicalDevice selectedPhysicalDevice = nullptr;
            for (int index = 0; index < devices.size(); ++index)
            {
                VkPhysicalDevice GFXDevice = devices[index].physicalDevice; /// Access the VkPhysicalDevice from GPUDevice
                vkGetPhysicalDeviceProperties(GFXDevice, &devices[index].deviceProperties); /// Get device properties

                if (devices[index].deviceProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU)
                {
                    SEDX_CORE_INFO("============================================");
                    SEDX_CORE_INFO("Device Name: {}", ToString(devices[index].deviceProperties.deviceName));
                    SEDX_CORE_INFO("Device Type: {}", ToString(devices[index].deviceProperties.deviceType));
                    SEDX_CORE_INFO("Device ID: {}", ToString(devices[index].deviceProperties.deviceID));
                    SEDX_CORE_INFO("Driver Version: {}", ToString(devices[index].deviceProperties.driverVersion));
                    SEDX_CORE_INFO("API Version: {}", ToString(devices[index].deviceProperties.apiVersion));
                    SEDX_CORE_INFO("Vendor ID: {}", ToString(devices[index].deviceProperties.vendorID));
                    SEDX_CORE_INFO("============================================");
                    selectedPhysicalDevice = GFXDevice;
                    deviceIndex = index; ///< Set the selected device index
                    break;
                }

                if (devices[index].deviceProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU)
                {
                    SEDX_CORE_WARN_TAG("Graphics Engine", "Integrated GPU found: {}", ToString(devices[index].deviceProperties.deviceName));
                    SEDX_CORE_ERROR_TAG("Graphics Engine", "Scenery Editor X cannot utilize an integrated GPU currently.");
                }
            }

            if (!selectedPhysicalDevice)
            {
                SEDX_CORE_ERROR_TAG("Graphics Engine", "Could not find discrete GPU.");

                /// Fallback to integrated GPU if no discrete GPU was found
                for (int index = 0; index < devices.size(); ++index)
                {
                    if (devices[index].deviceProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU)
                    {
                        SEDX_CORE_WARN_TAG("Graphics Engine", "Falling back to integrated GPU: {}", ToString(devices[index].deviceProperties.deviceName));
                        selectedPhysicalDevice = devices[index].physicalDevice;
                        deviceIndex = index;
                        break;
                    }
                }

                /// If still no GPU found, use the first available
                if (!selectedPhysicalDevice && !devices.empty())
                {
                    SEDX_CORE_WARN_TAG("Graphics Engine", "Falling back to first available GPU: {}", ToString(devices[0].deviceProperties.deviceName));
                    selectedPhysicalDevice = devices[0].physicalDevice;  // <- Assign the value, not just access it
                    deviceIndex = 0;
                }
            }
        }
        else
        {
            SEDX_CORE_ERROR_TAG("Graphics Engine","No physical devices found.");
            return;
        }

        /// Initialize device information for each physical device
        for (uint32_t index = 0; index < GFXDevices; index++)
        {
            /// Use the device-specific physical device handle
            const VkPhysicalDevice vkDevice = devices[index].physicalDevice;

            /// Get device features
            vkGetPhysicalDeviceFeatures(vkDevice, &devices[index].deviceFeatures);

            /// Get memory properties
            vkGetPhysicalDeviceMemoryProperties(vkDevice, &(devices[index].memoryProperties));
            SEDX_CORE_INFO("Number of memory types: {}", ToString(devices[index].memoryProperties.memoryTypeCount));
            for (uint32_t mem = 0; mem < devices[index].memoryProperties.memoryTypeCount; mem++)
            {
                const auto &[propertyFlags, heapIndex] = devices[index].memoryProperties.memoryTypes[mem];
                //SEDX_CORE_TRACE("============================================");
                //SEDX_CORE_TRACE("Memory Type Index: {}", ToString(mem));
                //SEDX_CORE_TRACE("Memory Heap Index: {}", ToString(heapIndex));
                //SEDX_CORE_TRACE("Memory Property Flags: {}", ToString(propertyFlags));
                //SEDX_CORE_TRACE("============================================");
            }

            SEDX_CORE_INFO("Number of memory heaps: {}", ToString(devices[index].memoryProperties.memoryHeapCount));

            /// Get queue family properties
            uint32_t numQueueFamilies = 0;
            vkGetPhysicalDeviceQueueFamilyProperties(vkDevice, &numQueueFamilies, nullptr);
            SEDX_CORE_ASSERT(numQueueFamilies > 0, "No queue families found for the physical device.");

            devices[index].queueFamilyInfo.resize(numQueueFamilies);
            devices[index].queueSupportPresent.resize(numQueueFamilies);
            vkGetPhysicalDeviceQueueFamilyProperties(vkDevice, &numQueueFamilies, devices[index].queueFamilyInfo.data());
            SEDX_CORE_INFO("Number of GPU device family queues: {}", ToString(numQueueFamilies));

            vkEnumerateDeviceExtensionProperties(vkDevice, nullptr, &numQueueFamilies, nullptr);
            SEDX_CORE_INFO("Number of device extensions: {}", ToString(numQueueFamilies));
            if (numQueueFamilies > 0)
            {
                std::vector<VkExtensionProperties> extensions(numQueueFamilies);
                if (vkEnumerateDeviceExtensionProperties(vkDevice, nullptr, &numQueueFamilies, &extensions.front()) == VK_SUCCESS)
                {
                    for (const auto &extension : extensions)
                    {
                        supportedExtensions.emplace(extension.extensionName, extension.specVersion);
                        SEDX_CORE_INFO("Extension Name: {}", ToString(extension.extensionName));
                        SEDX_CORE_INFO("Extension Version: {}", ToString(extension.specVersion));
                    }
                }
            }
            else
                SEDX_CORE_WARN("No device extensions found.");

            SEDX_CORE_INFO("============================================");

            /// -----------------------------------------------

            static constexpr float defaultQueuePriority(0.0f);
            int requestedQueueTypes = VK_QUEUE_GRAPHICS_BIT | VK_QUEUE_COMPUTE_BIT | VK_QUEUE_TRANSFER_BIT;
            QFamilyIndices          = GetQueueFamilyIndices(requestedQueueTypes);

            /// -----------------------------------------------

            /// Dedicated Graphics Queue
            if (requestedQueueTypes & VK_QUEUE_GRAPHICS_BIT)
            {
                VkDeviceQueueCreateInfo queueCreateInfo{};
                queueCreateInfo.sType            = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
                queueCreateInfo.queueFamilyIndex = QFamilyIndices.GetGraphicsFamily();
                queueCreateInfo.queueCount       = 1;
                queueCreateInfo.pQueuePriorities = &defaultQueuePriority;
                devices[index].queueCreateInfos.push_back(queueCreateInfo);
            }

            /// -----------------------------------------------

            /// Dedicated Compute Queue
            if (requestedQueueTypes & VK_QUEUE_COMPUTE_BIT)
            {
                if ((QFamilyIndices.GetComputeFamily() != QFamilyIndices.GetGraphicsFamily()) &&
					(QFamilyIndices.GetComputeFamily() != QFamilyIndices.GetTransferFamily()))
				{
                    VkDeviceQueueCreateInfo queueCreateInfo{};
                    queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
                    queueCreateInfo.queueFamilyIndex = QFamilyIndices.GetComputeFamily();
                    queueCreateInfo.queueCount = 1;
                    queueCreateInfo.pQueuePriorities = &defaultQueuePriority;
                    devices[index].queueCreateInfos.push_back(queueCreateInfo);
				}

            }

            /// -----------------------------------------------

            /// Dedicated Transfer Queue
            if (requestedQueueTypes & VK_QUEUE_TRANSFER_BIT)
            {
                if ((QFamilyIndices.GetTransferFamily() != QFamilyIndices.GetGraphicsFamily()) &&
                    (QFamilyIndices.GetTransferFamily() != QFamilyIndices.GetComputeFamily()))
                {
                    VkDeviceQueueCreateInfo queueCreateInfo{};
                    queueCreateInfo.sType            = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
                    queueCreateInfo.queueFamilyIndex = QFamilyIndices.GetTransferFamily();
                    queueCreateInfo.queueCount       = 1;
                    queueCreateInfo.pQueuePriorities = &defaultQueuePriority;
                    devices[index].queueCreateInfos.push_back(queueCreateInfo);
                }
            }

            /// -----------------------------------------------

            FindDepthFormat(devices[index]);
        }
    }

    /**
	 * @fn FindDepthFormat
	 * @brief Determines the best supported depth format for the given physical device
	 *
	 * @details This method selects an appropriate depth format from a list of preferred
	 * candidates in order of preference. It uses FindSupportedFormat to check which
	 * format is supported with optimal tiling and depth/stencil attachment capabilities.
	 * The candidate formats are:
	 * 1. @enum VK_FORMAT_D32_SFLOAT - 32-bit floating-point depth only (preferred)
	 * 2. @enum VK_FORMAT_D32_SFLOAT_S8_UINT - 32-bit float depth with 8-bit stencil
	 * 3. @enum VK_FORMAT_D24_UNORM_S8_UINT - 24-bit normalized depth with 8-bit stencil
	 *
	 * This method is typically called during device initialization to determine the
	 * appropriate format for depth buffers used in the rendering pipeline.
	 *
	 * @param device The GPU device for which to find a compatible depth format
	 * @return VkFormat The best supported depth format for the device
	 *
	 * @see FindSupportedFormat, @enum VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT
	 */
	VkFormat VulkanPhysicalDevice::FindDepthFormat(const GPUDevice &device)
	{
		std::vector candidates = {VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT};

		VkFormat depthFormat = FindSupportedFormat(device.physicalDevice, candidates, VK_IMAGE_TILING_OPTIMAL,VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT);

		return depthFormat;
	}

	/**
	 * @fn FindSupportedFormat
	 * @brief Find the first format in the provided candidates list that supports the required features
	 *
	 * @details This method examines each format in the candidates list to find the first one that
	 * supports the specified feature flags with the given tiling mode. For each candidate format:
	 * 1. It queries the physical device for the format properties using vkGetPhysicalDeviceFormatProperties
	 * 2. It checks if the format supports the requested features with the specified tiling mode:
	 *    - For linear tiling: Checks linearTilingFeatures against the required features
	 *    - For optimal tiling: Checks optimalTilingFeatures against the required features
	 * 3. Returns the first format that satisfies all requirements
	 *
	 * This function is typically used to find appropriate depth/stencil formats or
	 * other specialized formats with specific hardware feature requirements.
	 *
	 * @param physicalDevice The physical device to query for format support
	 * @param candidates A list of format candidates to check in order of preference
	 * @param tiling The desired tiling mode (linear or optimal)
	 * @param features Required format features that must be supported
	 *
	 * @return VkFormat The first format from the candidate list that supports the requested features
	 *
	 * @throws Logs an error if no suitable format is found among the candidates
	 *
	 * @note - Linear tiling is typically used for host-accessible images, while optimal tiling
	 *       provides better performance for GPU-only access images like depth buffers and textures
	 *
	 * @see vkGetPhysicalDeviceFormatProperties, VkFormatProperties, VkImageTiling
	 */
	VkFormat VulkanPhysicalDevice::FindSupportedFormat(const VkPhysicalDevice physicalDevice, const std::vector<VkFormat> &candidates, VkImageTiling tiling, VkFormatFeatureFlags features)
	{
		for (const VkFormat format : candidates)
		{
			VkFormatProperties props;
			vkGetPhysicalDeviceFormatProperties(physicalDevice, format, &props);

			if (tiling == VK_IMAGE_TILING_LINEAR && (props.linearTilingFeatures & features) == features)
				return format;
			if (tiling == VK_IMAGE_TILING_OPTIMAL && (props.optimalTilingFeatures & features) == features)
				return format;
		}

		SEDX_CORE_ERROR_TAG("Graphics Engine", "Failed to find a supported format!");
        return candidates[0]; // Return the first format as a fallback
	}

	/// -------------------------------------------------------

/*
	/// Get surface formats
	uint32_t numSurfaceFormats = 0;
	VK_CHECK_RESULT(vkGetPhysicalDeviceSurfaceFormatsKHR(vkDevice, nullptr, &numSurfaceFormats, nullptr))
	SEDX_CORE_INFO("Number of surface formats: {}", ToString(numSurfaceFormats));

	devices[index].surfaceFormats.resize(numSurfaceFormats);
	VK_CHECK_RESULT(vkGetPhysicalDeviceSurfaceFormatsKHR(vkDevice, nullptr, &numSurfaceFormats, devices[index].surfaceFormats.data()))
	for (uint32_t format = 0; format < numSurfaceFormats; format++)
	{
		const VkSurfaceFormatKHR &surfaceFormat = devices[index].surfaceFormats[format];
		SEDX_CORE_INFO("============================================");
		SEDX_CORE_INFO("Surface Format: {}", ToString(surfaceFormat.format));
		SEDX_CORE_INFO("Color Space: {}", ToString(surfaceFormat.colorSpace));
		SEDX_CORE_INFO("============================================");
	}

	/// Get surface capabilities
	VK_CHECK_RESULT(vkGetPhysicalDeviceSurfaceCapabilitiesKHR(vkDevice, nullptr, &(devices[index].surfaceCapabilities)))
	SEDX_CORE_INFO("============================================");
	SEDX_CORE_INFO("Device Surface Capabilities:");
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
	VK_CHECK_RESULT(vkGetPhysicalDeviceSurfacePresentModesKHR(vkDevice, nullptr, &numPresentModes, nullptr))
	devices[index].presentModes.resize(numPresentModes);
	VK_CHECK_RESULT(vkGetPhysicalDeviceSurfacePresentModesKHR(vkDevice, nullptr, &numPresentModes, devices[index].presentModes.data()))
	SEDX_CORE_INFO("Number of present modes: {}", ToString(numPresentModes));

	// Store queue family properties for later queue index lookup
	if (!devices.empty())
	{
		queueFamilyProperties = devices[0].queueFamilyInfo;
	}
}
*/

    /**
     * @fn VulkanPhysicalDevice
	 * @brief Default destructor for VulkanPhysicalDevice
	 *
	 * @details This destructor handles the cleanup of VulkanPhysicalDevice resources.
	 * Since the class uses RAII (Resource Acquisition Is Initialization) pattern,
	 * most resources are automatically cleaned up by their own destructors.
	 *
	 * Key points:
	 * - Uses the default implementation as no manual resource cleanup is needed
	 * - Any Vulkan handles stored in the devices vector are not explicitly destroyed here
	 *   as they are owned by the Vulkan instance, not by this class
	 * - The physical device handle itself is not destroyed as it's managed by the Vulkan runtime
	 *
	 * @note - Physical device handles (VkPhysicalDevice) are not created or destroyed by the
	 *       application. They are managed by the Vulkan implementation.
	 */
	VulkanPhysicalDevice::~VulkanPhysicalDevice() = default;

	/// -------------------------------------------------------

	/**
	 * @fn Select
	 * @brief Creates and returns a new Vulkan physical device instance
	 *
	 * @details This static factory method creates a new VulkanPhysicalDevice instance
	 * which performs physical device enumeration and selection. The implementation:
	 * 1. Creates a new VulkanPhysicalDevice object using the provided Vulkan instance
	 * 2. During construction, the object automatically enumerates all available GPUs
	 * 3. Evaluates device capabilities (queue families, features, extensions)
	 * 4. Returns a shared pointer (Ref) to the newly created object
	 *
	 * This method is the primary way for the graphics engine to initialize
	 * physical device functionality, typically followed by logical device creation.
	 *
	 * @param instance The Vulkan instance to use for device enumeration
	 * @return Ref<VulkanPhysicalDevice> A shared pointer to the newly created physical device object
	 *
	 * @note - The returned physical device has enumerated all available GPUs but may not
	 *       have explicitly selected one yet. Call SelectDevice() to choose a specific device.
	 *
	 * @see VulkanPhysicalDevice(), VulkanDevice
	 */
	Ref<VulkanPhysicalDevice> VulkanPhysicalDevice::Select(VkInstance &instance)
	{
		return CreateRef<VulkanPhysicalDevice>(instance);
	}

	/**
	 * @fn Selected
	 * @brief Returns a reference to the currently selected physical device
	 *
	 * @details This method retrieves the GPUDevice object representing the currently selected
	 * physical GPU in the system. It performs validation to ensure a valid device has been
	 * selected before returning a reference to it.
	 *
	 * The selection is based on the deviceIndex property that should have been set during
	 * physical device enumeration and selection. This device represents the GPU that will
	 * be used for rendering operations.
	 *
	 * @return const GPUDevice& A reference to the selected GPU device object containing
	 *         all relevant capabilities, properties, and feature information
	 *
	 * @throws Logs an error if no device has been selected or if the device index is invalid
	 *
	 * @note - This method should only be called after a physical device has been properly
	 *       selected via SelectDevice() or an equivalent selection method
	 *
	 * @see @struct GPUDevice, SelectDevice
	 */
	const GPUDevice &VulkanPhysicalDevice::Selected() const
	{
        /// Ensure deviceIndex is within the valid range
        if (deviceIndex < 0 || std::cmp_greater_equal(deviceIndex, devices.size()))
        {
            SEDX_CORE_ERROR_TAG("Graphics Engine", "No device selected or invalid device index: {}", deviceIndex);
            /// Return the first device as a fallback or handle the error more gracefully
            static GPUDevice fallbackDevice{};
            return fallbackDevice;
        }

        return devices[deviceIndex];
	}

    VkPhysicalDevice VulkanPhysicalDevice::GetGPUDevices() const
    {
        /// First check if we have the direct physical device handle (preferred)
        if (physicalDevice != VK_NULL_HANDLE)
            return physicalDevice;

        /// Fall back to the devices array if direct handle isn't set
        if (deviceIndex >= 0 && deviceIndex < static_cast<int>(devices.size()))
        {
            if (VkPhysicalDevice device = devices[deviceIndex].physicalDevice; device != VK_NULL_HANDLE)
                return device;
        }

        /// If we still don't have a valid device but have some devices available, use the first one
        if (!devices.empty() && devices[0].physicalDevice != VK_NULL_HANDLE)
        {
            SEDX_CORE_WARN_TAG("Graphics Engine", "No valid device selected, falling back to first available device");
            return devices[0].physicalDevice;
        }

        SEDX_CORE_ERROR_TAG("Graphics Engine", "No valid physical device available!");
        return VK_NULL_HANDLE;
    }

    /**
	 * @fn GetQueueFamilyIndices
	 * @brief Identifies queue families available on the physical device that match requested capabilities
	 *
	 * @details This method analyzes the queue families provided by a physical device and
	 * determines which queue families can support the requested queue operations. The implementation:
	 * 1. First attempts to find dedicated queue families for specialized tasks:
	 *    - Dedicated compute queues (support compute but not graphics operations)
	 *    - Dedicated transfer queues (support transfer but neither graphics nor compute operations)
	 *
	 * 2. Then assigns general-purpose queues to any required roles that weren't filled by dedicated queues
	 *
	 * This approach optimizes performance by using hardware queues that are specialized for specific
	 * tasks when available, falling back to more general queues when necessary. Dedicated queues often
	 * provide better performance for their specialized operations since they don't compete with other
	 * operation types.
	 *
	 * The function logs extensive information about available queue families and the final selections
	 * to assist with debugging and optimization.
	 *
	 * @param qFlags Bitfield of required queue capabilities (@enum VK_QUEUE_GRAPHICS_BIT, @enum VK_QUEUE_COMPUTE_BIT, etc.)
	 * @return @struct QueueFamilyIndices containing the selected queue family indices for different operations
	 *
	 * @note - If the device has no dedicated compute or transfer queues, the general graphics queue will
	 *       be used for all operations.
	 * @note - The commented-out section contains code for checking presentation support once surface
	 *       creation is implemented.
	 *
	 * @see VkQueueFlagBits, QueueFamilyIndices
	 */
	VulkanPhysicalDevice::QueueFamilyIndices VulkanPhysicalDevice::GetQueueFamilyIndices(VkQueueFlags qFlags) const
	{
		QueueFamilyIndices queueFamilies;

		/// Early return if no devices available
		if (devices.empty())
		{
			SEDX_CORE_ERROR_TAG("Graphics Engine", "No physical devices available");
			return queueFamilies;
		}

		/// Only process the selected device (or first device if none selected)
		uint32_t deviceIdx = deviceIndex >= 0 && deviceIndex < static_cast<int>(devices.size()) ? deviceIndex : 0;
		const VkPhysicalDevice vkDevice = devices[deviceIdx].physicalDevice;

		/// Get queue family properties for the device
		uint32_t numQueueFamilies = 0;
		vkGetPhysicalDeviceQueueFamilyProperties(vkDevice, &numQueueFamilies, nullptr);
		SEDX_CORE_ASSERT(numQueueFamilies > 0, "No queue families found for the physical device.");

		std::vector<VkQueueFamilyProperties> queueFamilyProperties(numQueueFamilies);
		vkGetPhysicalDeviceQueueFamilyProperties(vkDevice, &numQueueFamilies, queueFamilyProperties.data());

		/// Log queue family information
		for (uint32_t queueIdx = 0; queueIdx < numQueueFamilies; queueIdx++) {
			const VkQueueFamilyProperties &queueFamilyInfo = queueFamilyProperties[queueIdx];

			SEDX_CORE_INFO("============================================");
			SEDX_CORE_INFO("Queue Family Index: {}", ToString(queueIdx));
			SEDX_CORE_INFO("Queue Count: {}", ToString(queueFamilyInfo.queueCount));
			SEDX_CORE_INFO("Queue Flags: {}", ToString(queueFamilyInfo.queueFlags));
			SEDX_CORE_INFO("============================================");
		}

		/// First pass: find a graphics queue
		for (uint32_t queueIdx = 0; queueIdx < numQueueFamilies; queueIdx++)
		{
            if (const auto &props = queueFamilyProperties[queueIdx]; props.queueFlags & VK_QUEUE_GRAPHICS_BIT)
			{
				queueFamilies.graphicsFamily = std::make_optional(std::make_pair(Queue::Graphics, queueIdx));
				break;
			}
		}

		/// First pass: look for dedicated queues
		if (qFlags & VK_QUEUE_COMPUTE_BIT)
		{
			/// Find dedicated compute queue (compute, but not graphics)
			for (uint32_t queueIdx = 0; queueIdx < numQueueFamilies; queueIdx++)
			{
				const auto &props = queueFamilyProperties[queueIdx];
				const bool supportsCompute = (props.queueFlags & VK_QUEUE_COMPUTE_BIT) != 0;
                if (const bool supportsGraphics = (props.queueFlags & VK_QUEUE_GRAPHICS_BIT)
					!= 0; supportsCompute && !supportsGraphics)
				{
					queueFamilies.computeFamily = std::make_optional(std::make_pair(Queue::Compute, queueIdx));
					break;
				}
			}
		}

		if (qFlags & VK_QUEUE_TRANSFER_BIT)
		{
			/// Find dedicated transfer queue (transfer, but not graphics or compute)
			for (uint32_t queueIdx = 0; queueIdx < numQueueFamilies; queueIdx++)
		    {
				const auto &props = queueFamilyProperties[queueIdx];
				const bool supportsTransfer = (props.queueFlags & VK_QUEUE_TRANSFER_BIT) != 0;
				const bool supportsGraphics = (props.queueFlags & VK_QUEUE_GRAPHICS_BIT) != 0;
                if (const bool supportsCompute = (props.queueFlags & VK_QUEUE_COMPUTE_BIT)
					!= 0; supportsTransfer && !supportsGraphics && !supportsCompute)
				{
					queueFamilies.transferFamily = std::make_optional(std::make_pair(Queue::Transfer, queueIdx));
					break;
				}
			}
		}

		/// Second pass: set any remaining indices to general-purpose queues
		for (uint32_t queueIdx = 0; queueIdx < numQueueFamilies; queueIdx++)
		{
			const auto &props = queueFamilyProperties[queueIdx];

			/// Set compute queue if not already set and if needed
			if ((qFlags & VK_QUEUE_COMPUTE_BIT) && !queueFamilies.computeFamily.has_value() && (props.queueFlags & VK_QUEUE_COMPUTE_BIT))
                queueFamilies.computeFamily = std::make_optional(std::make_pair(Queue::Compute, queueIdx));

            /// Set transfer queue if not already set and if needed
			if ((qFlags & VK_QUEUE_TRANSFER_BIT) && !queueFamilies.transferFamily.has_value() && (props.queueFlags & VK_QUEUE_TRANSFER_BIT))
			{
				queueFamilies.transferFamily = std::make_optional(std::make_pair(Queue::Transfer, queueIdx));
				break;
			}

			/// Set presentation queue
			/// Note: This would normally check presentation support against a surface
			/// Since we don't have a surface at this point, we'll just use the graphics queue
			if (!queueFamilies.presentFamily.has_value() && queueFamilies.graphicsFamily.has_value() && queueFamilies.graphicsFamily.value().second == queueIdx)
                queueFamilies.presentFamily = std::make_optional(std::make_pair(Queue::Present, queueIdx));
        }

		/// Fallback: If we couldn't find dedicated compute/transfer queues, use the graphics queue
		if ((qFlags & VK_QUEUE_COMPUTE_BIT) && !queueFamilies.computeFamily.has_value() && queueFamilies.graphicsFamily.has_value())
            queueFamilies.computeFamily = std::make_optional(std::make_pair(Queue::Compute, queueFamilies.graphicsFamily.value().second));

        if ((qFlags & VK_QUEUE_TRANSFER_BIT) && !queueFamilies.transferFamily.has_value() && queueFamilies.graphicsFamily.has_value())
            queueFamilies.transferFamily = std::make_optional(std::make_pair(Queue::Transfer, queueFamilies.graphicsFamily.value().second));

        SEDX_CORE_INFO("============================================");
		SEDX_CORE_INFO("Selected Queue Families:");
		SEDX_CORE_INFO("Graphics: {}", queueFamilies.graphicsFamily.has_value() ? ToString(queueFamilies.graphicsFamily.value().second) : "Not Available");
		SEDX_CORE_INFO("Compute: {}", queueFamilies.computeFamily.has_value() ? ToString(queueFamilies.computeFamily.value().second) : "Not Available");
		SEDX_CORE_INFO("Transfer: {}", queueFamilies.transferFamily.has_value() ? ToString(queueFamilies.transferFamily.value().second) : "Not Available");
		SEDX_CORE_INFO("Present: {}", queueFamilies.presentFamily.has_value() ? ToString(queueFamilies.presentFamily.value().second) : "Not Available");
		SEDX_CORE_INFO("============================================");

		return queueFamilies;
	}

	///////////////////////////////////////////////////////////
	/// Vulkan Device Implementation                        ///
	///////////////////////////////////////////////////////////

	/**
	 * @fn VulkanDevice::VulkanDevice
	 * @brief Creates a Vulkan logical device from a physical device
	 *
	 * @details This constructor initializes a logical Vulkan device with the following steps:
	 * 1. Verifies that required device extensions are supported
	 * 2. Adds necessary extensions such as VK_KHR_SWAPCHAIN_EXTENSION_NAME
	 * 3. Conditionally adds vendor-specific extensions (NVIDIA diagnostic extensions)
	 * 4. Creates the logical device with appropriate queue configurations
	 * 5. Retrieves handles to graphics and compute queues
	 * 6. Loads function pointers for extended Vulkan functionality
	 * 7. Sets up bindless resources for efficient shader resource access
	 * 8. Creates an initial scratch buffer for general GPU operations
	 *
	 * The constructor configures the device based on the supplied physical device capabilities
	 * and the requested feature set, ensuring all necessary features and extensions are enabled.
	 *
	 * @param physDevice The physical device to create a logical device from
	 * @param enabledFeatures The device features to be enabled on the logical device
	 *
	 * @note - The device creation may fail if required extensions are not supported, in which
	 *       case an error is logged and the function returns early.
	 * @note - Device queues are acquired based on queue family indices determined during
	 *       physical device selection.
	 *
	 * @see LoadExtensionFunctions, InitializeBindlessResources, CreateBuffer
	 */
	VulkanDevice::VulkanDevice(const Ref<VulkanPhysicalDevice> &physDevice) : vkPhysicalDevice(physDevice)
    {
		VulkanChecks checks;

        // Verify we have a valid physical device before proceeding
        VkPhysicalDevice physicalDevice = physDevice->GetGPUDevices();
        if (physicalDevice == VK_NULL_HANDLE)
        {
            SEDX_CORE_ERROR_TAG("Graphics Engine", "Cannot create logical device: Invalid physical device handle");
            return;
        }

	    VkPhysicalDeviceFeatures deviceFeatures = {};
        deviceFeatures.samplerAnisotropy = VK_TRUE;                    ///< Enable anisotropic filtering
        deviceFeatures.wideLines = VK_TRUE;                            ///< Enable wide lines if needed
        deviceFeatures.fillModeNonSolid = VK_TRUE;                     ///< Enable non-solid fill modes
        deviceFeatures.geometryShader = VK_TRUE;                       ///< Enable geometry shaders if needed
        deviceFeatures.tessellationShader = VK_TRUE;                   ///< Enable tessellation shaders if needed
        deviceFeatures.independentBlend = VK_TRUE;                     ///< Enable independent blending if needed
        deviceFeatures.pipelineStatisticsQuery = VK_TRUE;              ///< Enable pipeline statistics queries if needed
        deviceFeatures.shaderStorageImageWriteWithoutFormat = VK_TRUE; ///< Enable storage image writes without format

        /// Prepare device extensions as vector<const char*>
        std::vector<const char *> deviceExtensions = { VK_KHR_SWAPCHAIN_EXTENSION_NAME };

        /// Optionally add NVIDIA/AMD extensions if supported
        if (VulkanChecks::IsExtensionSupported(VK_NV_DEVICE_DIAGNOSTIC_CHECKPOINTS_EXTENSION_NAME))
            deviceExtensions.push_back(VK_NV_DEVICE_DIAGNOSTIC_CHECKPOINTS_EXTENSION_NAME);
        if (VulkanChecks::IsExtensionSupported(VK_NV_DEVICE_DIAGNOSTICS_CONFIG_EXTENSION_NAME))
            deviceExtensions.push_back(VK_NV_DEVICE_DIAGNOSTICS_CONFIG_EXTENSION_NAME);

        /// Check device extension support
        std::vector<VkExtensionProperties> availableExtensions;
        checks.CheckDeviceExtensionSupport(physicalDevice, availableExtensions, nullptr);
        for (const char* ext : deviceExtensions)
		{
            bool found = false;
            for (const auto&[extensionName, specVersion] : availableExtensions)
			{
                if (strcmp(ext, extensionName) == 0)
				{
                    found = true;
                    break;
                }
            }
            if (!found)
			{
                SEDX_CORE_ERROR_TAG("Graphics Engine", "Required device extension not supported: {}", ext);
                return;
            }
        }

		/// ---------------------------------------------------------

	    auto supportedFeatures = deviceFeatures;

		VkPhysicalDeviceFeatures2 features2 = {};
		features2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
		features2.features.geometryShader = VK_TRUE;
		if (supportedFeatures.logicOp)           { features2.features.logicOp           = VK_TRUE; }
		if (supportedFeatures.samplerAnisotropy) { features2.features.samplerAnisotropy = VK_TRUE; }
		if (supportedFeatures.sampleRateShading) { features2.features.sampleRateShading = VK_TRUE; }
		if (supportedFeatures.fillModeNonSolid)  { features2.features.fillModeNonSolid  = VK_TRUE; }
		if (supportedFeatures.wideLines)         { features2.features.wideLines         = VK_TRUE; }
		if (supportedFeatures.depthClamp)        { features2.features.depthClamp        = VK_TRUE; }

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

		vkGetPhysicalDeviceFeatures2(physicalDevice, &features2);

        /// Create the logical device
        VkDeviceCreateInfo createInfo = {};
        createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;

        // Verify we have valid queue create info before proceeding
        if (physDevice->Selected().queueCreateInfos.empty())
        {
            SEDX_CORE_ERROR_TAG("Graphics Engine", "No queue create info available for device creation");
            return;
        }

        createInfo.pQueueCreateInfos = physDevice->Selected().queueCreateInfos.data();
        createInfo.queueCreateInfoCount = static_cast<uint32_t>(physDevice->Selected().queueCreateInfos.size());

		if (VulkanChecks::IsExtensionSupported(VK_EXT_DEBUG_MARKER_EXTENSION_NAME))
            deviceExtensions.push_back(VK_EXT_DEBUG_MARKER_EXTENSION_NAME);

        if (!deviceExtensions.empty())
		{
			createInfo.enabledExtensionCount = static_cast<uint32_t>(deviceExtensions.size());
			createInfo.ppEnabledExtensionNames = deviceExtensions.data();
		}

        if (enableValidationLayers)
        {
            Layers validation;
            auto &layers = validation.activeLayersNames;
            createInfo.enabledLayerCount = static_cast<uint32_t>(layers.size());
            createInfo.ppEnabledLayerNames = layers.data();
        }
        else
        {
            createInfo.enabledLayerCount = 0;
        }

        createInfo.pEnabledFeatures = &deviceFeatures;
        createInfo.pNext = &features2; /// Use features2 for Vulkan 1.2+

		/// Create the logical device
        auto result = vkCreateDevice(physicalDevice, &createInfo, nullptr, &device);
        if (result != VK_SUCCESS)
		{
			SEDX_CORE_ERROR_TAG("Graphics Engine", "Failed to create logical device! Error: {}", static_cast<int>(result));
            device = VK_NULL_HANDLE; /// Ensure device is set to null for error checking elsewhere
			return;
		}

        SEDX_CORE_INFO_TAG("Graphics Engine", "Logical device created successfully");

        /// Get device queues
        vkGetDeviceQueue(device, physDevice->QFamilyIndices.GetGraphicsFamily(), 0, &GraphicsQueue);
        vkGetDeviceQueue(device, physDevice->QFamilyIndices.GetComputeFamily(), 0, &ComputeQueue);
        vkGetDeviceQueue(device, physDevice->QFamilyIndices.GetPresentFamily(), 0, &PresentQueue);
        vkGetDeviceQueue(device, physDevice->QFamilyIndices.GetTransferFamily(), 0, &TransferQueue);
        SEDX_CORE_INFO_TAG("Graphics Engine",
                           "Using queue family indices: Graphics {}, Compute {}, Present {}, Transfer {}",
                           physDevice->QFamilyIndices.GetGraphicsFamily(),
                           physDevice->QFamilyIndices.GetComputeFamily(),
                           physDevice->QFamilyIndices.GetPresentFamily(),
                           physDevice->QFamilyIndices.GetTransferFamily());

        /// Load device extension function pointers
        LoadExtensionFunctions();

        //RenderData::apiVersion renderData;
		/// Initialize memory allocator
        //MemoryAllocator::Init(device, renderData);

		/// Set up bindless resources and initial buffers
		//InitBindlessResources(device, bindlessResources);

		/// Create initial scratch buffer
		//scratchBuffer = CreateBuffer(initialScratchBufferSize, BufferUsage::Address | BufferUsage::Storage, MemoryType::GPU,"ScratchBuffer");

		/// Get the device address for the scratch buffer
		// if (vkGetBufferDeviceAddressKHR != nullptr)
		// {
		// 	VkBufferDeviceAddressInfo scratchInfo{};
		// 	scratchInfo.sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO;
		// 	scratchInfo.buffer = scratchBuffer.bufferResource->buffer;
		// 	scratchAddress = vkGetBufferDeviceAddressKHR(device, &scratchInfo);
		// }
	}

	/**
	 * @fn LoadExtensionFunctions
	 * @brief Loads function pointers for Vulkan extension functions
	 *
	 * @details This method dynamically loads function pointers for Vulkan extension functions that
	 * are not part of the core API and must be queried at runtime. It loads:
	 * 1. Debug utilities functions:
	 *    - vkSetDebugUtilsObjectNameEXT: Sets a debug name for Vulkan objects for debugging tools
	 *
	 * 2. Ray tracing acceleration structure functions:
	 *    - vkGetAccelerationStructureBuildSizesKHR: Calculates memory requirements for acceleration structures
	 *    - vkCreateAccelerationStructureKHR: Creates acceleration structure objects
	 *    - vkCmdBuildAccelerationStructuresKHR: Records commands to build acceleration structures
	 *    - vkGetAccelerationStructureDeviceAddressKHR: Retrieves device addresses for acceleration structures
	 *    - vkDestroyAccelerationStructureKHR: Destroys acceleration structure objects
	 *
	 * 3. Buffer device address functions:
	 *    - vkGetBufferDeviceAddressKHR: Retrieves device address for a buffer
	 *
	 * These function pointers enable the engine to use extension functionality in a
	 * cross-platform and runtime-compatible way.
	 *
	 * @note - Function pointers are initialized to nullptr and will remain that way if the
	 *       corresponding extension is not supported or enabled.
	 *
	 * @see vkGetDeviceProcAddr
	 */
	void VulkanDevice::LoadExtensionFunctions()
	{
		/// Load debug utils functions
		vkSetDebugUtilsObjectNameEXT = (PFN_vkSetDebugUtilsObjectNameEXT)vkGetDeviceProcAddr(device,"vkSetDebugUtilsObjectNameEXT");
		/// Load ray tracing functions
		vkGetAccelerationStructureBuildSizesKHR = (PFN_vkGetAccelerationStructureBuildSizesKHR)vkGetDeviceProcAddr(device,"vkGetAccelerationStructureBuildSizesKHR");
		vkCreateAccelerationStructureKHR = (PFN_vkCreateAccelerationStructureKHR)vkGetDeviceProcAddr(device, "vkCreateAccelerationStructureKHR");
		vkGetBufferDeviceAddressKHR = (PFN_vkGetBufferDeviceAddressKHR)vkGetDeviceProcAddr(device, "vkGetBufferDeviceAddressKHR");
		vkCmdBuildAccelerationStructuresKHR = (PFN_vkCmdBuildAccelerationStructuresKHR)vkGetDeviceProcAddr(device, "vkCmdBuildAccelerationStructuresKHR");
		vkGetAccelerationStructureDeviceAddressKHR = (PFN_vkGetAccelerationStructureDeviceAddressKHR) vkGetDeviceProcAddr(device, "vkGetAccelerationStructureDeviceAddressKHR");
		vkDestroyAccelerationStructureKHR = (PFN_vkDestroyAccelerationStructureKHR)vkGetDeviceProcAddr(device, "vkDestroyAccelerationStructureKHR");
	}

	/*
	void VulkanDevice::InitializeMemoryAllocator()
	{
		memoryAllocator = CreateRef<MemoryAllocator>("VulkanDevice");
		memoryAllocator->Init(VulkanDevice device);
	}
	*/

	/**
	 * @fn InitializeBindlessResources
	 * @brief Sets up bindless resource system for efficient shader resource access
	 *
	 * @details This method initializes the bindless descriptor system which allows shaders to
	 * access a large number of resources through indices rather than fixed bindings:
	 * 1. Creates a descriptor pool specifically for ImGui rendering with sufficient resources
	 * 2. Initializes resource ID tracking for both buffers and sampled images
	 * 3. Creates a global descriptor pool for bindless resources with update-after-bind support
	 * 4. Sets up the descriptor set layout with three main bindings:
	 *    - Combined image samplers (textures)
	 *    - Storage buffers
	 *    - Storage images
	 * 5. Configures binding flags to enable partial binding and dynamic updates
	 * 6. Allocates the global bindless descriptor set
	 *
	 * The bindless resource system enables the engine to:
	 * - Access thousands of resources from any shader without rebinding
	 * - Update resources at runtime without recreating descriptor sets
	 * - Use dynamic indexing in shaders for data-driven rendering techniques
	 * - Support efficient texture arrays, material systems, and instance data
	 *
	 * @note - This implementation relies on VK_EXT_descriptor_indexing extension
	 * @note - The descriptor pools are sized according to predefined MAX_* constants
	 *
	 * @see VkDescriptorPoolCreateInfo, VkDescriptorSetLayoutCreateInfo
	 */
    /*
    void VulkanDevice::InitializeBindlessResources(const VkDevice device, const BindlessResources& bindlessResources)
    {

        /// Initialize resource ID arrays for tracking available resource slots
	// Legacy bindless resource ID pool initialization removed (handled by BindlessDescriptorManager).
    }
    */

	/**
	 * @fn ~VulkanDevice()
	 * @brief Destroys the VulkanDevice instance and cleans up associated resources
	 *
	 * @details This destructor handles the complete cleanup of all Vulkan resources created by the device:
	 * 1. Waits for all pending device operations to complete using vkDeviceWaitIdle
	 * 2. Cleans up thread-specific command pools stored in the CmdPools map
	 * 3. Releases the scratch buffer used for temporary storage operations
	 * 4. Destroys bindless descriptor resources (layout, descriptor pools)
	 * 5. Shuts down the memory allocator subsystem
	 * 6. Destroys the logical device
	 *
	 * The destructor follows a careful order of destruction to prevent accessing freed resources,
	 * and includes appropriate null-checking to handle partially-initialized states.
	 *
	 * @note - This class follows RAII principles with this destructor ensuring all Vulkan resources
	 *       are properly freed when a VulkanDevice instance goes out of scope.
	 *
	 * @see VulkanDevice::Destroy, MemoryAllocator::Shutdown
	 */
	VulkanDevice::~VulkanDevice()
	{
		/// Wait for device to be idle before cleanup
		if (device != VK_NULL_HANDLE)
            vkDeviceWaitIdle(device);

        /// Clean up command pools
		CmdPools.clear();

		/// Clean up scratch buffer
		//scratchBuffer = {};

		/*
		/// Clean up bindless resources
		if (device != VK_NULL_HANDLE)
		{
            if (bindlessResources.bindlessDescriptorSetLayout != VK_NULL_HANDLE)
                vkDestroyDescriptorSetLayout(device, bindlessResources.bindlessDescriptorSetLayout, nullptr);

            if (bindlessResources.bindlessDescriptorPool != VK_NULL_HANDLE)
                vkDestroyDescriptorPool(device, bindlessResources.bindlessDescriptorPool, nullptr);

            if (bindlessResources.imguiDescriptorPool != VK_NULL_HANDLE)
                vkDestroyDescriptorPool(device, bindlessResources.imguiDescriptorPool, nullptr);
        }
        */

		// Shutdown bindless descriptor system prior to device destruction
		BindlessDescriptorManager::Shutdown();

		/// Destroy logical device
		if (device != VK_NULL_HANDLE)
		{
			vkDestroyDevice(device, nullptr);
			device = VK_NULL_HANDLE;
		}
	}

    // From device, or use the FrameIdx in MemoryAllocator class.
    void VulkanDevice::Tick(const uint64_t frame_count) const
    {
        // https://gpuopen-librariesandsdks.github.io/VulkanMemoryAllocator/html/staying_within_budget.html
        // make sure to call vmaSetCurrentFrameIndex() every frame
        // budget is queried from Vulkan inside of it to avoid overhead of querying it with every allocation
        vmaSetCurrentFrameIndex(GetMemoryAllocator(), static_cast<uint32_t>(frame_count));
    }

	/// -------------------------------------------------------

	/**
	 * @brief Get the memory allocator associated with this device
	 *
	 * Retrieves the VMA allocator object that handles memory management for this Vulkan device.
	 * The memory allocator provides efficient allocation, binding, and management of Vulkan
	 * memory resources, helping to reduce fragmentation and optimize memory usage.
	 *
	 * @return VmaAllocator The Vulkan Memory Allocator handle, or nullptr if not initialized
	 *
	 * @note - This function checks if the memory allocator has been properly initialized and
	 *       logs an error if it hasn't been. Memory operations will fail without a valid allocator.
	 *
	 * @see MemoryAllocator::GetMemAllocator()
	 */
	VmaAllocator VulkanDevice::GetMemoryAllocator() const
	{
		if (memoryAllocator)
			return MemoryAllocator::GetAllocator();

		SEDX_CORE_ERROR_TAG("Graphics Engine", "Memory allocator not initialized.");
		return nullptr;
	}

    bool VulkanDevice::IsValidResolution(uint32_t width, uint32_t height)
    {
        return width  > 4 && width  <= m_max_texture_2d_dimension &&
               height > 4 && height <= m_max_texture_2d_dimension;
    }

    /**
     * @fn VulkanDevice::VulkanDevice(VulkanDevice &&)
     * @brief Move constructor that transfers ownership of Vulkan device resources.
     *
     * @details This constructor efficiently transfers ownership of all Vulkan resources from one
     * VulkanDevice instance to another without performing deep copies. It implements the following:
     * 1. Moves all member variables (descriptors, resources, buffers, etc.) from source to destination.
     * 2. Transfers ownership of all Vulkan handles (device, queues, etc.) to the new instance.
     * 3. Nullifies critical handles in the source object to prevent double-deletion.
     *
     * Move semantics are essential for Vulkan resources which are expensive to copy and must have
     * clearly defined ownership to prevent validation errors or resource leaks. This constructor
     * enables VulkanDevice objects to be stored in standard containers that require move operations.
     *
     * @param other The source VulkanDevice instance to move from (will be left in a valid but empty state).
     *
     * @note - After the move operation, the source object remains in a valid but resource-empty state,
     *       with its critical handles set to null/zero to prevent any destructive operations when
     *       its destructor is called.
     *
     * @see ~VulkanDevice, operator=
     */
    VulkanDevice::VulkanDevice(VulkanDevice &&other) noexcept :
          /*bindlessResources(other.bindlessResources),*/ vkGetBufferDeviceAddressKHR(other.vkGetBufferDeviceAddressKHR),
          vkSetDebugUtilsObjectNameEXT(other.vkSetDebugUtilsObjectNameEXT), vkCreateAccelerationStructureKHR(other.vkCreateAccelerationStructureKHR),
          vkDestroyAccelerationStructureKHR(other.vkDestroyAccelerationStructureKHR), vkCmdBuildAccelerationStructuresKHR(other.vkCmdBuildAccelerationStructuresKHR),




          vkGetAccelerationStructureBuildSizesKHR(other.vkGetAccelerationStructureBuildSizesKHR),
          vkGetAccelerationStructureDeviceAddressKHR(other.vkGetAccelerationStructureDeviceAddressKHR),
          device(other.device),
          memoryAllocator(std::move(other.memoryAllocator)),
          vkPhysicalDevice(std::move(other.vkPhysicalDevice)),
          vkEnabledFeatures(other.vkEnabledFeatures)
    {
        other.device = nullptr;
        other.textureSampler = nullptr;
        //other.scratchAddress = 0;
    }

	/**
	 * @fn operator=
	 * @brief Move assignment operator that transfers ownership of Vulkan device resources
	 *
	 * @details This operator properly handles resource transfer when moving one VulkanDevice
	 * instance to another. It implements the following strategy:
	 * 1. Checks for self-assignment to prevent resource corruption.
	 * 2. Properly cleans up any existing resources in the destination object.
	 * 3. Transfers ownership of all Vulkan handles and resources from source to destination.
	 * 4. Nullifies the source object's handles to prevent double-deletion.
	 *
	 * This ensures safe transfer of device ownership with proper resource management, which
	 * is essential for RAII (Resource Acquisition Is Initialization) in Vulkan applications
	 * where resource cleanup order is critical.
	 *
	 * @param other The source VulkanDevice to move resources from (will be in a valid but empty state after the move)
	 * @return VulkanDevice& A reference to the destination object (*this) containing all moved resources
	 *
	 * @note - After the move, the source object remains valid but with all its Vulkan handles set to null
	 *       and should not be used to execute Vulkan operations without reinitialization.
	 *
	 * @see VulkanDevice, VulkanDevice::Destroy
	 */
    VulkanDevice &VulkanDevice::operator=(VulkanDevice &&other) noexcept
    {
        if (this != &other)
        {
            /// Clean up existing resources
            Destroy();

            /// Move resources from the other VulkanDevice
            device = other.device;
            vkPhysicalDevice = std::move(other.vkPhysicalDevice);
            vkEnabledFeatures = other.vkEnabledFeatures;
            GraphicsQueue = other.GraphicsQueue;
            ComputeQueue = other.ComputeQueue;
            PresentQueue = other.PresentQueue;
            memoryAllocator = std::move(other.memoryAllocator);
            /*bindlessResources = other.bindlessResources*/

            /// Nullify the moved-from object
            other.device = nullptr;
            other.GraphicsQueue = VK_NULL_HANDLE;
            other.ComputeQueue = VK_NULL_HANDLE;
            other.PresentQueue = VK_NULL_HANDLE;
            other.textureSampler = nullptr;
        }
        return *this;
    }

	/**
	 * @brief Destroys the Vulkan logical device and cleans up resources
	 *
	 * This method handles the proper destruction of the Vulkan logical device:
	 * 1. Clears all command pools associated with the device
	 * 2. Waits for all device operations to complete using vkDeviceWaitIdle
	 * 3. Destroys the logical device
	 * 4. Sets the device handle to VK_NULL_HANDLE to prevent reuse
	 *
	 * This should be called before the VulkanDevice instance is destroyed or
	 * when the application is shutting down to ensure proper cleanup of GPU resources.
	 */
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

	/**
	 * @brief Locks a queue for thread-safe access
	 *
	 * This method provides exclusive access to either the graphics or compute queue
	 * by locking the appropriate mutex. This prevents race conditions when multiple
	 * threads attempt to submit work to the same queue simultaneously.
	 *
	 * @param compute If true, locks the compute queue mutex; otherwise locks the graphics queue mutex
	 *
	 * @note - This should be paired with a matching UnlockQueue call in a RAII pattern,
	 *       ideally using a std::lock_guard or similar scope-based locking mechanism.
	 *
	 * @see UnlockQueue
	 */
	void VulkanDevice::LockQueue(const bool compute)
	{
		if (compute)
			ComputeQueueMutex.lock();
		else
			GraphicsQueueMutex.lock();
	}

	/**
	 * @brief Unlocks a previously locked queue
	 *
	 * @details This method releases the lock on either the graphics or compute queue,
	 * allowing other threads to access it. It should only be called after a
	 * corresponding LockQueue call.
	 *
	 * @param compute If true, unlocks the compute queue mutex; otherwise unlocks the graphics queue mutex
	 *
	 * @see LockQueue
	 */
	void VulkanDevice::UnlockQueue(const bool compute)
	{
		if (compute)
			ComputeQueueMutex.unlock();
		else
			GraphicsQueueMutex.unlock();
	}

    Ref<CommandPool> VulkanDevice::LocalCommandPool()
    {
        const auto threadID = std::this_thread::get_id();
        SEDX_CORE_VERIFY(CmdPools.contains(threadID));
        return CmdPools.at(threadID);
    }

    Ref<CommandPool> VulkanDevice::CreateLocalCommandPool()
    {
        const auto threadID = std::this_thread::get_id();
        if (const auto commandPoolIt = CmdPools.find(threadID); commandPoolIt != CmdPools.end())
            return commandPoolIt->second;

        /// Use CreateRef to construct a new CommandPool smart pointer
        Ref<CommandPool> commandPool = CreateRef<CommandPool>(Ref<VulkanDevice>(this), Queue::Graphics);
        CmdPools[threadID] = commandPool;
        return commandPool;
    }

    /**
	 * @brief Find the queue families for the device.
	 * @param debugName
	 * @param device - The device to find the queue families for.
	 * @return - The queue family indices.
	 */
	/*
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
	*/

	VkCommandBuffer VulkanDevice::CreateUICmdBuffer(const char *debugName)
	{
		/// Get the command pool for the current thread
        Ref<CommandPool> cmdPool = CreateLocalCommandPool();

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

	Ref<CommandPool> VulkanDevice::GetThreadLocalCommandPool()
    {
        const auto threadID = std::this_thread::get_id();
        SEDX_CORE_VERIFY(CmdPools.contains(threadID));

        return CmdPools.at(threadID);
    }

	Ref<CommandPool> VulkanDevice::GetOrCreateThreadLocalCommandPool()
    {
        const auto threadID = std::this_thread::get_id();
        if (const auto commandPoolIt = CmdPools.find(threadID); commandPoolIt != CmdPools.end())
            return commandPoolIt->second;

        Ref<CommandPool> commandPool = CreateRef<CommandPool>(Ref<VulkanDevice>(this), Queue::Graphics);
        CmdPools[threadID] = commandPool;
        return commandPool;
    }

    void VulkanDevice::FlushCmdBuffer(VkCommandBuffer cmdBuffer)
    {
        GetThreadLocalCommandPool()->FlushCmdBuffer(cmdBuffer);
    }

    void VulkanDevice::FlushCmdBuffer(VkCommandBuffer cmdBuffer, VkQueue queue)
    {
        GetThreadLocalCommandPool()->FlushCmdBuffer(cmdBuffer);
    }

    /// -------------------------------------------------------

	/**
	 * @brief Determine the maximum MSAA sample count supported by the GPU
	 *
	 * This method queries the physical device properties to determine the highest multisample
	 * anti-aliasing (MSAA) sample count that is supported for both color and depth attachments.
	 * It performs a bitwise AND operation between the supported color and depth sample counts
	 * to find values that are supported by both.
	 *
	 * The method checks sample counts in descending order (64 → 2) and returns the highest
	 * supported value. If no multisampling is supported, it returns VK_SAMPLE_COUNT_1_BIT.
	 *
	 * @return VkSampleCountFlagBits The maximum supported MSAA sample count
	 *
	 * @note - The returned sample count can be used when creating render passes and framebuffers
	 *       to enable MSAA rendering at the highest quality level supported by the hardware.
	 */
	VkSampleCountFlagBits VulkanDevice::GetMaxUsableSampleCount() const
	{
		VkPhysicalDeviceProperties physicalDeviceProperties;
		vkGetPhysicalDeviceProperties(vkPhysicalDevice->GetGPUDevices(), &physicalDeviceProperties);

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

		// Descriptor indexing feature chain (expanded for full bindless usage similar to Spartan engine approach)
		VkPhysicalDeviceDescriptorIndexingFeatures descriptorIndexingFeatures{};
		descriptorIndexingFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DESCRIPTOR_INDEXING_FEATURES;
		descriptorIndexingFeatures.runtimeDescriptorArray = VK_TRUE;
		descriptorIndexingFeatures.descriptorBindingPartiallyBound = VK_TRUE;
		descriptorIndexingFeatures.descriptorBindingVariableDescriptorCount = VK_TRUE; // allow variable sized arrays if needed later
		descriptorIndexingFeatures.shaderSampledImageArrayNonUniformIndexing = VK_TRUE;
		descriptorIndexingFeatures.shaderStorageImageArrayNonUniformIndexing = VK_TRUE;
		descriptorIndexingFeatures.shaderUniformBufferArrayNonUniformIndexing = VK_TRUE;
		descriptorIndexingFeatures.shaderStorageBufferArrayNonUniformIndexing = VK_TRUE;
		descriptorIndexingFeatures.shaderInputAttachmentArrayNonUniformIndexing = VK_TRUE;
		descriptorIndexingFeatures.shaderUniformTexelBufferArrayNonUniformIndexing = VK_TRUE;
		descriptorIndexingFeatures.shaderStorageTexelBufferArrayNonUniformIndexing = VK_TRUE;
		descriptorIndexingFeatures.descriptorBindingSampledImageUpdateAfterBind = VK_TRUE;
		descriptorIndexingFeatures.descriptorBindingStorageImageUpdateAfterBind = VK_TRUE;
		descriptorIndexingFeatures.descriptorBindingStorageBufferUpdateAfterBind = VK_TRUE;
		descriptorIndexingFeatures.descriptorBindingUniformBufferUpdateAfterBind = VK_TRUE;
		descriptorIndexingFeatures.descriptorBindingUpdateUnusedWhilePending = VK_TRUE;
		descriptorIndexingFeatures.descriptorBindingInlineUniformBlockUpdateAfterBind = VK_FALSE; // not used yet
		descriptorIndexingFeatures.shaderUniformBufferArrayNonUniformIndexing = VK_TRUE; // reaffirm

		// NOTE: If any of these end up unsupported on a target GPU we should query first and mask unsupported ones.
		// Spartan-like path: eventually centralize a FeatureQuery struct gathering all required optional features & fallback policy.

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

		// Initialize global bindless descriptor manager (sampled images, samplers, storage images, storage buffers, uniform buffers)
		BindlessDescriptorManager::Init(
			BindlessResources::MAX_SAMPLED_IMAGES,
			512,
			BindlessResources::MAX_STORAGE_IMAGES,
			BindlessResources::MAX_STORAGE_BUFFERS,
			BindlessResources::MAX_UNIFORM_BUFFERS);

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

    /**
     * @brief Initializes the Vulkan memory allocator for efficient GPU memory management.
     *
     * This function creates and configures the Vulkan Memory Allocator (VMA) instance
     * that will handle all memory allocations for buffers, images, and other GPU resources.
     * VMA provides efficient memory management, minimizes fragmentation, and optimizes
     * allocation strategies based on usage patterns.
     *
     * The function:
     * 1. Sets up the core allocator configuration with the physical device, logical device, and instance
     * 2. Configures optional features like buffer device address support when available
     * 3. Provides function pointers for dynamic Vulkan function loading
     * 4. Creates the memory allocator instance
     *
     * Once initialized, all Vulkan memory allocations should be handled through this allocator
     * rather than directly through vkAllocateMemory for optimal performance and resource management.
     */
    /*
    void VulkanDevice::InitializeMemoryAllocator()
    {
        if (device == VK_NULL_HANDLE)
        {
            SEDX_CORE_ERROR_TAG("Vulkan Device", "Cannot initialize memory allocator with null device handle");
            return;
        }

        SEDX_CORE_TRACE_TAG("Vulkan Device", "Initializing Vulkan Memory Allocator");

	    /// Create VMA (Vulkan Memory Allocator) instance
        VmaAllocatorCreateInfo allocatorCreateInfo = {};
        allocatorCreateInfo.physicalDevice = vkPhysicalDevice->GetGPUDevices();
        allocatorCreateInfo.device = device;
        allocatorCreateInfo.instance = instance;

        /// Set up flags
        allocatorCreateInfo.flags = VMA_ALLOCATOR_CREATE_EXT_MEMORY_BUDGET_BIT | VMA_ALLOCATOR_CREATE_BUFFER_DEVICE_ADDRESS_BIT;

        /// Enable buffer device address if available
        if (vkGetBufferDeviceAddressKHR != nullptr)
            allocatorCreateInfo.flags |= VMA_ALLOCATOR_CREATE_BUFFER_DEVICE_ADDRESS_BIT;

        /// Use a descriptor pool for memory allocation if needed
        /* allocatorCreateInfo.pAllocationCallbacks = allocator; #1#

        /// Create the memory allocator
        memoryAllocator = CreateRef<MemoryAllocator>();

        SEDX_CORE_TRACE_TAG("Vulkan Device", "Vulkan Memory Allocator initialized successfully");
    }
    */

	// TODO: Create separate one for shadow maps
	/**
	 * @fn CreateSampler
	 * @brief Creates a texture sampler with specified configuration parameters
	 *
	 * @details This function creates a Vulkan sampler object with common texture sampling
	 * parameters. The sampler is configured with linear filtering for both magnification
	 * and minification, repeat address modes for all dimensions, and conditional anisotropic
	 * filtering based on hardware support.
	 *
	 * Samplers are essential objects in Vulkan that define how texture data is read and
	 * filtered within shaders. They control aspects such as:
	 * - Filtering modes (linear, nearest)
	 * - Address modes (repeat, clamp, mirror)
	 * - Anisotropic filtering
	 * - Mipmap selection and filtering
	 * - LOD (Level of Detail) behavior
	 *
	 * @param maxLOD Maximum level of detail that can be accessed through this sampler,
	 *               useful for controlling mipmap access (default should be the max mip level)
	 *
	 * @return VkSampler A configured Vulkan sampler object that must be destroyed when no longer needed
	 *
	 * @note - The sampler checks for anisotropic filtering support at runtime and enables it
	 *       if available, using the maximum available anisotropy level from the physical device.
	 *       The returned sampler should be destroyed with vkDestroySampler when no longer needed.
	 *
	 * @see VkSamplerCreateInfo, vkCreateSampler, vkDestroySampler
	 */
	VkSampler VulkanDevice::CreateSampler(const float maxLOD) const
	{
		VkSamplerCreateInfo samplerInfo{};
		samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
		samplerInfo.magFilter = VK_FILTER_LINEAR;
		samplerInfo.minFilter = VK_FILTER_LINEAR;
		samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
		samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
		samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;

		/// Check if anisotropy is supported
		VkPhysicalDeviceFeatures deviceFeatures;
		vkGetPhysicalDeviceFeatures(vkPhysicalDevice->GetGPUDevices(), &deviceFeatures);

		if (deviceFeatures.samplerAnisotropy)
		{
			samplerInfo.anisotropyEnable = VK_TRUE;
			samplerInfo.maxAnisotropy = vkPhysicalDevice->GetLimits().maxSamplerAnisotropy;
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

	/**
	 * @fn FindMemoryType
	 * @brief Finds a suitable memory type index that meets specific requirements
	 *
	 * This method locates a memory type that satisfies both:
	 * 1. It must be compatible with the provided type filter (represented as a bit field)
	 * 2. It must have all the required memory properties
	 *
	 * The implementation retrieves memory properties from the physical device and iterates
	 * through available memory types. For each memory type index, it checks:
	 * - Whether the type is suitable according to the type filter (using bit masking)
	 * - Whether the memory type has all the required property flags
	 *
	 * This function is critical for proper allocation of buffers and images in Vulkan,
	 * as it ensures memory is allocated from an appropriate memory heap with the needed
	 * characteristics (e.g., device-local, host-visible, coherent).
	 *
	 * @param typeFilter Bit field where each bit represents a memory type that is suitable
	 * @param properties Required memory properties (e.g., @enum VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT)
	 * @return Index of a suitable memory type, or 0 if no suitable memory type is found
	 *
	 * @see vkGetPhysicalDeviceMemoryProperties, VkMemoryPropertyFlags
	 */
	uint32_t VulkanDevice::FindMemoryType(const uint32_t typeFilter, const VkMemoryPropertyFlags properties) const
	{
		/// Get memory properties from the physical vkDevice
		VkPhysicalDeviceMemoryProperties memProperties;
		vkGetPhysicalDeviceMemoryProperties(vkPhysicalDevice->GetGPUDevices(), &memProperties);

		/// Find a memory type that satisfies both the type filter and the property requirements
		for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++)
		{
			if ((typeFilter & (1 << i)) && (memProperties.memoryTypes[i].propertyFlags & properties) == properties)
                return i;
        }

		SEDX_CORE_ERROR_TAG("Graphics Engine", "Failed to find suitable memory type!");
		return 0; /// Return a default value to avoid undefined behavior
	}

	/// -------------------------------------------------------

	/// -------------------------------------------------------
    /// CommandPool Implementation
    /// -------------------------------------------------------

    /**
	 * @fn CommandPool
	 * @brief Creates command pools for graphics and compute operations
	 *
	 * @details This constructor initializes separate command pools for graphics and compute queues.
	 * Command pools are memory managers for command buffers and should typically be created for
	 * each thread that will record commands. This implementation:
	 * 1. Creates a command pool using the queue type specified (graphics or compute)
	 * 2. Creates a separate compute command pool if a dedicated compute queue is available
	 * 3. Falls back to using the graphics command pool for compute operations if necessary
	 *
	 * Command pools are created with the @enum VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT flag,
	 * allowing individual command buffers to be reset for reuse without resetting the entire pool.
	 *
	 * @param vulkanDevice Reference to the Vulkan device that will own these command pools
	 * @param type Queue type to determine which command pool to create (graphics or compute)
	 *
	 * @note - Command pools are specific to queue families and buffers allocated from a pool
	 *       can only be submitted to queues of the matching family.
	 *
	 * @see vkCreateCommandPool, VkCommandPoolCreateInfo
	 */
    CommandPool::CommandPool(const Ref<VulkanDevice> &vulkanDevice, Queue type)
    {
        const auto vulkanDeviceHandle = vulkanDevice->GetDevice();
        const auto &queueIndices = vulkanDevice->GetPhysicalDevice()->GetQueueFamilyIndices();
        VkCommandPool commandPool;

        queueType = type;

        /// Create command pool
        VkCommandPoolCreateInfo cmdPoolInfo{};
        cmdPoolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
        cmdPoolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
        cmdPoolInfo.queueFamilyIndex = queueIndices.graphicsFamily->first;

        VkResult result = vkCreateCommandPool(vulkanDeviceHandle, &cmdPoolInfo, nullptr, &commandPool);
        if (result != VK_SUCCESS)
            SEDX_CORE_ERROR_TAG("Graphics Engine", "Failed to create graphics command pool! Error: {}", static_cast<int>(result));

        /// Create compute command pool if compute queue is available
        if (queueIndices.computeFamily.has_value())
        {
            cmdPoolInfo.queueFamilyIndex = queueIndices.GetComputeFamily();

            result = vkCreateCommandPool(vulkanDeviceHandle, &cmdPoolInfo, nullptr, &ComputeCmdPool);
            if (result != VK_SUCCESS)
            {
                SEDX_CORE_ERROR_TAG("Graphics Engine", "Failed to create compute command pool! Error: {}", static_cast<int>(result));
                ComputeCmdPool = GraphicsCmdPool; /// Fall back to using graphics pool for compute operations
            }
        }
    }

    /**
	 * @fn ~CommandPool
	 * @brief Destroys the command pools created by this object
	 *
	 * @details This destructor properly cleans up command pool resources by:
	 * 1. Checking if the device is still valid before proceeding
	 * 2. Destroying the compute command pool if it's distinct from the graphics pool
	 * 3. Destroying the graphics command pool
	 * 4. Setting all handles to VK_NULL_HANDLE to prevent use-after-free issues
	 *
	 * The destructor handles the case where the compute and graphics command pools
	 * share the same handle to avoid double-deletion, which would cause a Vulkan
	 * validation error.
	 *
	 * @note - Command pools must be destroyed before their parent device is destroyed.
	 *       When a command pool is destroyed, all command buffers allocated from it
	 *       are implicitly freed and should not be used afterward.
	 *
	 * @see @fn vkDestroyCommandPool
	 */
    CommandPool::~CommandPool()
    {
        const auto device = RenderContext::Get()->GetLogicDevice()->GetDevice();
        if (!device)
            return;

        /// Only destroy compute pool if it's different from graphics pool
        if (ComputeCmdPool != VK_NULL_HANDLE && ComputeCmdPool != GraphicsCmdPool)
            vkDestroyCommandPool(device, ComputeCmdPool, nullptr);

        if (GraphicsCmdPool != VK_NULL_HANDLE)
            vkDestroyCommandPool(device, GraphicsCmdPool, nullptr);

        GraphicsCmdPool = VK_NULL_HANDLE;
        ComputeCmdPool = VK_NULL_HANDLE;
    }

    /**
	 * @fn AllocateCommandBuffer
	 * @brief Allocates a command buffer from the appropriate command pool
	 *
	 * @details This method allocates a new command buffer from either the graphics or compute command pool
	 * based on the provided parameters. It handles proper allocation, initialization, and error checking.
	 * If requested, it will also automatically begin the command buffer with one-time-submit usage flag,
	 * making it ready to record commands immediately.
	 *
	 * The command buffer is allocated as a primary command buffer, which can be submitted directly to a queue
	 * and can call secondary command buffers. Primary command buffers cannot be called by other command buffers.
	 *
	 * @param begin If true, the command buffer will be automatically started with @enum VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT
	 * @param compute If true, the command buffer will be allocated from the compute command pool;
	 *                otherwise, it will be allocated from the graphics command pool
	 *
	 * @return VkCommandBuffer The newly allocated command buffer, or VK_NULL_HANDLE if allocation failed
	 *
	 * @note - Command buffers allocated with this method should either be freed manually or flushed using
	 *       the FlushCmdBuffer method, which will handle submission and automatic cleanup.
	 *
	 * @see FlushCmdBuffer
	 */
    VkCommandBuffer CommandPool::AllocateCommandBuffer(const bool begin, const bool compute) const
    {
        const auto device = RenderContext::Get()->GetLogicDevice()->GetDevice();
        const VkCommandPool cmdPool = compute ? ComputeCmdPool : GraphicsCmdPool;

        VkCommandBufferAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        allocInfo.commandPool = cmdPool;
        allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        allocInfo.commandBufferCount = 1;

        VkCommandBuffer cmdBuffer;
        VkResult result = vkAllocateCommandBuffers(device, &allocInfo, &cmdBuffer);

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
                vkFreeCommandBuffers(device, cmdPool, 1, &cmdBuffer);
                return VK_NULL_HANDLE;
            }
        }

        return cmdBuffer;
    }

    /**
	 * @fn FlushCmdBuffer
	 * @brief Submits a command buffer to a specified queue and waits for its completion
	 *
	 * @details This method handles the complete submission lifecycle of a command buffer:
	 * 1. Ends the command buffer recording with vkEndCommandBuffer
	 * 2. Creates a fence to synchronize execution completion
	 * 3. Submits the command buffer to the specified queue
	 * 4. Waits for execution to complete using the fence
	 * 5. Cleans up resources (fence and command buffer)
	 *
	 * This implementation uses a fence-based synchronization approach to ensure the GPU has
	 * completely processed the submitted commands before returning. This is suitable for
	 * operations that need to be completed before the CPU continues execution, such as
	 * resource initialization or one-time uploads to the GPU.
	 *
	 * @param cmdBuffer The command buffer to submit and execute
	 * @param queue The queue to which the command buffer should be submitted
	 *
	 * @note - After this function returns, the command buffer has been freed and should not be used
	 * @note - This function will block the calling thread until the GPU completes execution
	 * @note - For regular rendering operations that don't need CPU synchronization,
	 *       consider using semaphores instead for better performance
	 *
	 * @see vkEndCommandBuffer, vkCreateFence, vkQueueSubmit, vkWaitForFences
	 */
    void CommandPool::FlushCmdBuffer(const VkCommandBuffer cmdBuffer, const VkQueue queue) const
    {
        auto device = RenderContext::GetCurrentDevice()->GetDevice();
        if (cmdBuffer == VK_NULL_HANDLE)
        {
            SEDX_CORE_WARN_TAG("Graphics Engine", "Attempted to flush a null command buffer");
            return;
        }

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
        result = vkCreateFence(device, &fenceInfo, nullptr, &fence);
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
            vkDestroyFence(device, fence, nullptr);
            return;
        }

        /// Wait for the fence
        result = vkWaitForFences(device, 1, &fence, VK_TRUE, DEFAULT_FENCE_TIMEOUT);
        if (result != VK_SUCCESS)
        {
            SEDX_CORE_ERROR("Failed to wait for fence! Error: {}", static_cast<int>(result));
        }

        /// Clean up
        vkDestroyFence(device, fence, nullptr);
        vkFreeCommandBuffers(device, GraphicsCmdPool, 1, &cmdBuffer);
    }

} // namespace SceneryEditorX

/// -------------------------------------------------------
