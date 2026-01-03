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
#include <SceneryEditorX/renderer/device.h>
#include "swapchain.h"
#include "SceneryEditorX/core/window/window.h"
#include <utility>
#include <SceneryEditorX/renderer/memory_allocator.h>
#include <SceneryEditorX/renderer/render_context.h>
#include <SceneryEditorX/renderer/vulkan_checks.h>
#include <SceneryEditorX/renderer/vulkan_utils.h>

// -------------------------------------------------------

namespace SceneryEditorX
{
	namespace properties
    {
        uint64_t min_UniformBufferOffsetAlignment = 0;
        uint64_t min_StorageBufferOffsetAlignment = 0;
        uint64_t optimalBufferCopyOffsetAlignment = 0;
        uint32_t max_Cube_TextureDimension = 0;
        uint32_t max_1D_TextureDimension = 0;
        uint32_t max_2D_TextureDimension = 0;
        uint32_t max_3D_TextureDimension = 0;
        uint32_t max_TextureArrayLayers = 0;
        uint32_t max_PushConstantSize = 0;
        uint32_t max_X_ShadingRateTexel = 0;
        uint32_t max_Y_ShadingRateTexel = 0;;
    }

    // -------------------------------------------------------

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
    VulkanPhysicalDevice::VulkanPhysicalDevice()
    {
        VkInstance vkInstance = RenderContext::GetInstance();

        uint32_t GFXDevices = 0; // Number of physical devices
        vkEnumeratePhysicalDevices(vkInstance, &GFXDevices, nullptr);
        SEDX_CORE_ASSERT(GFXDevices > 0, "No Vulkan-compatible physical devices found.");
        std::vector<VkPhysicalDevice> physicalDevices(GFXDevices);
        VK_CHECK_RESULT(vkEnumeratePhysicalDevices(vkInstance, &GFXDevices, physicalDevices.data()));

        VkPhysicalDevice selectedPhysicalDevice = nullptr;
        for (VkPhysicalDevice physicalDevice : physicalDevices)
        {
            vkGetPhysicalDeviceProperties2(physicalDevice, &m_DeviceProperties);
            if (m_DeviceProperties.properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU)
            {
                selectedPhysicalDevice = physicalDevice;
                break;
            }
        }

        if (!selectedPhysicalDevice)
        {
            SEDX_CORE_INFO_TAG("Renderer", "Could not find discrete GPU.");
            selectedPhysicalDevice = physicalDevices.back();
        }
        SEDX_CORE_ASSERT(selectedPhysicalDevice, "Could not find any physical devices!");
        m_PhysicalDevice = selectedPhysicalDevice;

        vkGetPhysicalDeviceFeatures2(m_PhysicalDevice, &m_Features);
        vkGetPhysicalDeviceMemoryProperties2(m_PhysicalDevice, &m_MemProperties);

        uint32_t queueFamilyCount;
        vkGetPhysicalDeviceQueueFamilyProperties2(m_PhysicalDevice, &queueFamilyCount, nullptr);
        SEDX_CORE_ASSERT(queueFamilyCount > 0, "");
        m_QueueFamilyInfo.resize(queueFamilyCount);
        vkGetPhysicalDeviceQueueFamilyProperties2(m_PhysicalDevice, &queueFamilyCount, m_QueueFamilyInfo.data());


        uint32_t extCount = 0;
        vkEnumerateDeviceExtensionProperties(m_PhysicalDevice, nullptr, &extCount, nullptr);
        if (extCount > 0)
        {
            std::vector<VkExtensionProperties> extensions(extCount);
            if (vkEnumerateDeviceExtensionProperties(m_PhysicalDevice, nullptr, &extCount, &extensions.front()) ==
                VK_SUCCESS)
            {
                SEDX_CORE_INFO_TAG("Renderer", "Selected physical device has {0} extensions", extensions.size());
                for (const auto &ext : extensions)
                {
                    m_SupportedExtensions.emplace(ext.extensionName);
                    SEDX_CORE_INFO_TAG("Renderer", "  {0}", ext.extensionName);
                }
            }
        }

        /**
         * Queue families
         *
         * Desired queues need to be requested upon logical device creation
         * Due to differing queue family configurations of Vulkan implementations this can be a bit tricky, especially if the application
         * requests different queue types
         *
         * Get queue family indices for the requested queue family types
         * @note that the indices may overlap depending on the implementation
         */
        static const float DEFAULT_QUEUE_PRIORITY(0.0f);

        int requestedQueueTypes = VK_QUEUE_GRAPHICS_BIT | VK_QUEUE_COMPUTE_BIT | VK_QUEUE_TRANSFER_BIT;
        m_QueueFamilyIndices = GetQueueFamilyIndices(requestedQueueTypes);

        //////////////////////////////////////////////////////////////////////////////////////////////////////////
        // Graphics Queue																						//
        //////////////////////////////////////////////////////////////////////////////////////////////////////////
        if (requestedQueueTypes & VK_QUEUE_GRAPHICS_BIT)
        {
            VkDeviceQueueCreateInfo queueInfo{};
            queueInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
            queueInfo.queueFamilyIndex = m_QueueFamilyIndices.graphics;
            queueInfo.queueCount = 1;
            queueInfo.pQueuePriorities = &DEFAULT_QUEUE_PRIORITY;
            m_QueueCreateInfos.push_back(queueInfo);
        }

		//////////////////////////////////////////////////////////////////////////////////////////////////////////
        // Dedicated Compute Queue																				//
        //////////////////////////////////////////////////////////////////////////////////////////////////////////
        if (requestedQueueTypes & VK_QUEUE_COMPUTE_BIT)
        {
            if (m_QueueFamilyIndices.compute != m_QueueFamilyIndices.graphics)
            {
                // If compute family index differs, we need an additional queue create info for the compute queue
                VkDeviceQueueCreateInfo queueInfo{};
                queueInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
                queueInfo.queueFamilyIndex = m_QueueFamilyIndices.compute;
                queueInfo.queueCount = 1;
                queueInfo.pQueuePriorities = &DEFAULT_QUEUE_PRIORITY;
                m_QueueCreateInfos.push_back(queueInfo);
            }
        }

	    //////////////////////////////////////////////////////////////////////////////////////////////////////////
        // Dedicated Transfer Queue																				//
        //////////////////////////////////////////////////////////////////////////////////////////////////////////
        if (requestedQueueTypes & VK_QUEUE_TRANSFER_BIT)
        {
            if ((m_QueueFamilyIndices.transfer != m_QueueFamilyIndices.graphics) &&
                (m_QueueFamilyIndices.transfer != m_QueueFamilyIndices.compute))
            {
                // If compute family index differs, we need an additional queue create info for the compute queue
                VkDeviceQueueCreateInfo queueInfo{};
                queueInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
                queueInfo.queueFamilyIndex = m_QueueFamilyIndices.transfer;
                queueInfo.queueCount = 1;
                queueInfo.pQueuePriorities = &DEFAULT_QUEUE_PRIORITY;
                m_QueueCreateInfos.push_back(queueInfo);
            }
        }

        m_DepthFormat = FindDepthFormat();
        SEDX_CORE_ASSERT(m_DepthFormat);
        SEDX_CORE_INFO_TAG("Renderer", "Selected depth format: {}", ToString(m_DepthFormat));
    }

    VulkanPhysicalDevice::~VulkanPhysicalDevice() = default;

    // -------------------------------------------------------

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
	 * @return VkFormat The best supported depth format for the device
	 *
	 * @see FindSupportedFormat, @enum VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT
	 */
    VkFormat VulkanPhysicalDevice::FindDepthFormat() const
    {
        // Since all depth formats may be optional, we need to find a suitable depth format to use
        // Start with the highest precision packed format
        std::vector<VkFormat> depthFormats = {VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D32_SFLOAT, VK_FORMAT_D24_UNORM_S8_UINT, VK_FORMAT_D16_UNORM_S8_UINT, VK_FORMAT_D16_UNORM};

        for (auto &format : depthFormats)
        {
            VkFormatProperties formatProps;
            vkGetPhysicalDeviceFormatProperties(m_PhysicalDevice, format, &formatProps);
            // Format must support depth stencil attachment for optimal tiling
            if (formatProps.optimalTilingFeatures & VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT)
                return format;
        }

        return VK_FORMAT_UNDEFINED;
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

	bool VulkanPhysicalDevice::IsExtensionSupported(const std::string &extensionName) { return m_SupportedExtensions.contains(extensionName); }

	// -------------------------------------------------------

    /**
	 * @fn GetQueueFamilyIndices
	 *
	 * @brief Identifies queue family indices that support the requested queue types.
	 * This method searches through the physical device's queue families
	 * to find indices that support the specified queue types (graphics, compute, transfer).
	 *
	 * @details The method attempts to find dedicated queue families for compute and transfer
	 * operations that do not also support graphics, to optimize workload distribution.
	 * If no dedicated queues are found, it falls back to using the first available
	 * queue family that supports the requested type.
	 *
	 * @param flags Bitmask of requested queue types (VK_QUEUE_GRAPHICS_BIT, VK_QUEUE_COMPUTE_BIT, VK_QUEUE_TRANSFER_BIT)
	 * @return QueueFamilyIndices Struct containing the identified queue family indices
	 */
	VulkanPhysicalDevice::QueueFamilyIndices VulkanPhysicalDevice::GetQueueFamilyIndices(int flags)
	{
		QueueFamilyIndices indices;

        //////////////////////////////////////////////////////////////////////////////////////////////////////////
        // Dedicated Queue for Compute																			//
        //////////////////////////////////////////////////////////////////////////////////////////////////////////
        if (flags & VK_QUEUE_COMPUTE_BIT)
        {
            for (uint32_t i = 0; i < m_QueueFamilyInfo.size(); i++)
            {
                auto &queueFamilyProperties = m_QueueFamilyInfo[i];
                if ((queueFamilyProperties.queueFamilyProperties.queueFlags & VK_QUEUE_COMPUTE_BIT) &&
                    ((queueFamilyProperties.queueFamilyProperties.queueFlags & VK_QUEUE_GRAPHICS_BIT) == 0))
                {
                    indices.compute = i;
                    break;
                }
            }
        }

        //////////////////////////////////////////////////////////////////////////////////////////////////////////
        // Dedicated Queue for Transfer																			//
        //////////////////////////////////////////////////////////////////////////////////////////////////////////
        if (flags & VK_QUEUE_TRANSFER_BIT)
        {
            for (uint32_t i = 0; i < m_QueueFamilyInfo.size(); i++)
            {
                auto &queueFamilyProperties = m_QueueFamilyInfo[i];
                if ((queueFamilyProperties.queueFamilyProperties.queueFlags & VK_QUEUE_TRANSFER_BIT) &&
                    ((queueFamilyProperties.queueFamilyProperties.queueFlags & VK_QUEUE_GRAPHICS_BIT) == 0) &&
                    ((queueFamilyProperties.queueFamilyProperties.queueFlags & VK_QUEUE_COMPUTE_BIT) == 0))
                {
                    indices.transfer = i;
                    break;
                }
            }
        }

        //////////////////////////////////////////////////////////////////////////////////////////////////////////
        // Other Queue Types
        // If no separate compute queue is present, return the first one to support the requested flags
        //////////////////////////////////////////////////////////////////////////////////////////////////////////
        for (uint32_t i = 0; i < m_QueueFamilyInfo.size(); i++)
        {
            if ((flags & VK_QUEUE_TRANSFER_BIT) && indices.transfer == -1)
            {
                if (m_QueueFamilyInfo[i].queueFamilyProperties.queueFlags & VK_QUEUE_TRANSFER_BIT)
                    indices.transfer = i;
            }

            if ((flags & VK_QUEUE_COMPUTE_BIT) && indices.compute == -1)
            {
                if (m_QueueFamilyInfo[i].queueFamilyProperties.queueFlags & VK_QUEUE_COMPUTE_BIT)
                    indices.compute = i;
            }

            if (flags & VK_QUEUE_GRAPHICS_BIT)
            {
                if (m_QueueFamilyInfo[i].queueFamilyProperties.queueFlags & VK_QUEUE_GRAPHICS_BIT)
                    indices.graphics = i;
            }
        }

        return indices;
    }

	// -------------------------------------------------------

    bool VulkanPhysicalDevice::QueueFamilyIndices::IsComplete() const
    {
        return graphics != INVALID && compute != INVALID && transfer != INVALID;
    }

    // -------------------------------------------------------

	uint32_t VulkanPhysicalDevice::GetMemoryTypeIndex(uint32_t typeBits, VkMemoryPropertyFlags properties) const
    {
        // Iterate over all memory types available for the device used in this example
        for (uint32_t i = 0; i < m_MemProperties.memoryProperties.memoryTypeCount; i++)
        {
            if ((typeBits & 1) == 1)
            {
                if ((m_MemProperties.memoryProperties.memoryTypes[i].propertyFlags & properties) == properties)
                    return i;
            }
            typeBits >>= 1;
        }

        SEDX_CORE_ASSERT(false, "Could not find a suitable memory type!");
        return UINT32_MAX;
    }

	// -------------------------------------------------------

    uint32_t VulkanPhysicalDevice::QueueFamilyIndices::GetGraphicsFamily() const noexcept
    {
        SEDX_CORE_ASSERT(graphics != INVALID, "Graphics queue must be initialized");
        return graphics;
    }

	// -------------------------------------------------------

    uint32_t VulkanPhysicalDevice::QueueFamilyIndices::GetPresentFamily() const noexcept
    {
        SEDX_CORE_ASSERT(present != INVALID, "Present queue must be initialized");
        return present;
    }

	// -------------------------------------------------------

    uint32_t VulkanPhysicalDevice::QueueFamilyIndices::GetComputeFamily() const noexcept
    {
        SEDX_CORE_ASSERT(compute != INVALID, "Compute queue must be initialized");
        return compute;
    }

	// -------------------------------------------------------

    uint32_t VulkanPhysicalDevice::QueueFamilyIndices::GetTransferFamily() const noexcept
    {
        SEDX_CORE_ASSERT(transfer != INVALID, "Transfer queue must be initialized");
        return transfer;
    }

    // Unified accessor
    [[nodiscard]] uint32_t VulkanPhysicalDevice::QueueFamilyIndices::Get(uint32_t type) const
    {
        switch (type)
        {
			case compute: return GetGraphicsFamily();
			case present:  return GetPresentFamily();
			case compute:  return GetComputeFamily();
			case transfer: return GetTransferFamily();
			default:
			    SEDX_CORE_ASSERT(false, "Unknown queue type");
			    return 0;
        }
    }

    uint32_t VulkanPhysicalDevice::FindMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties)
    {
        //auto ctx = RenderContext::Get(); // Get render context reference
        auto device = RenderContext::GetCurrentDevice()->GetPhysicalDevice()->GetDevice();
        VkPhysicalDeviceMemoryProperties memProperties;
        vkGetPhysicalDeviceMemoryProperties(device, &memProperties);
        for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++)
        {
            if (typeFilter & 1 << i && (memProperties.memoryTypes[i].propertyFlags & properties) == properties)
                return i;
        }

        SEDX_CORE_ERROR("Failed to find suitable memory type!");
        return UINT32_MAX; // Return an invalid memory type index
    }

    // -------------------------------------------------------

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
	Ref<VulkanPhysicalDevice> VulkanPhysicalDevice::Select()
	{
	    return CreateRef<VulkanPhysicalDevice>();
	}

	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	/// Vulkan Device Implementation																     		  	  ///
	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

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
	VulkanDevice::VulkanDevice (const Ref<VulkanPhysicalDevice>& physDevice) : m_PhysicalDevice (physDevice)
    {

        if (physDevice == VK_NULL_HANDLE)
        {
            SEDX_CORE_ERROR_TAG ("Graphics Engine", "Cannot create logical device: Invalid physical device handle");
            return;
        }

	    // Prepare device extensions as vector<const char*>
	    std::vector<const char*> deviceExtensions;

	    // In headless mode, avoid swapchain to keep validation silent and remove WSI requirements
	    bool headlessMode = false;
        #if defined(SEDX_PLATFORM_WINDOWS) && defined(SEDX_HEADLESS)
		    if (const char* env = std::getenv("SEDX_HEADLESS"))
		        headlessMode = (std::strcmp(env, "1") == 0 || _stricmp(env, "true") == 0);
		#endif
		    if (!headlessMode)
		        deviceExtensions.push_back(VK_KHR_SWAPCHAIN_EXTENSION_NAME);

        // ---------------------------------------------------------

        {
            // Optionally add NVIDIA/AMD extensions if supported
            if (VulkanChecks::IsExtensionSupported (VK_NV_DEVICE_DIAGNOSTIC_CHECKPOINTS_EXTENSION_NAME))
                deviceExtensions.push_back(VK_NV_DEVICE_DIAGNOSTIC_CHECKPOINTS_EXTENSION_NAME);
            if (VulkanChecks::IsExtensionSupported (VK_NV_DEVICE_DIAGNOSTICS_CONFIG_EXTENSION_NAME))
                deviceExtensions.push_back(VK_NV_DEVICE_DIAGNOSTICS_CONFIG_EXTENSION_NAME);
            
            // TODO: Implement Nvidia Aftermath integration
			#if SEDX_NV_AFTERMATH
			    VkDeviceDiagnosticsConfigCreateInfoNV aftermathInfo = {};
			    bool canEnableAftermath = enableAftermath &&
			        m_PhysicalDevice->IsExtensionSupported(VK_NV_DEVICE_DIAGNOSTIC_CHECKPOINTS_EXTENSION_NAME) &&
			        m_PhysicalDevice->IsExtensionSupported(VK_NV_DEVICE_DIAGNOSTICS_CONFIG_EXTENSION_NAME);
			    if (canEnableAftermath)
			    {
			        // Must be initialized ~before~ device has been created
			        GpuCrashTracker *gpuCrashTracker = new GpuCrashTracker();
			        gpuCrashTracker->Initialize();

			        VkDeviceDiagnosticsConfigFlagBitsNV aftermathFlags =
			            (VkDeviceDiagnosticsConfigFlagBitsNV)(VK_DEVICE_DIAGNOSTICS_CONFIG_ENABLE_RESOURCE_TRACKING_BIT_NV |
			                                                  VK_DEVICE_DIAGNOSTICS_CONFIG_ENABLE_AUTOMATIC_CHECKPOINTS_BIT_NV |
			                                                  VK_DEVICE_DIAGNOSTICS_CONFIG_ENABLE_SHADER_DEBUG_INFO_BIT_NV);

			        aftermathInfo.sType = VK_STRUCTURE_TYPE_DEVICE_DIAGNOSTICS_CONFIG_CREATE_INFO_NV;
			        aftermathInfo.flags = aftermathFlags;
			    }
			#endif
        }

		// ---------------------------------------------------------

        {
            VulkanChecks checks; // Check device extension support
            std::vector<VkExtensionProperties> availableExtensions;
            checks.CheckDeviceExtensionSupport(physDevice->m_PhysicalDevice, availableExtensions, nullptr);
            for (const char* ext : deviceExtensions)
            {
                bool found = false;
                for (const auto& [extensionName, specVersion] : availableExtensions)
                {
                    if (strcmp (ext, extensionName) == 0)
                    {
                        found = true; break;
                    }
                }
                if (!found)
                {
                    SEDX_CORE_ERROR_TAG ("Graphics Engine", "Required device extension not supported: {}", ext);
                    return;
                }
            }
        }

        // ---------------------------------------------------------

        {
            VkPhysicalDeviceFeatures2 features2 = {};
			VkPhysicalDeviceVulkan12Features vulkan12Features = {};
			VkPhysicalDeviceVulkan13Features vulkan13Features = {};
			VkPhysicalDeviceVulkan14Features vulkan14Features = {};
			VkPhysicalDeviceRobustness2FeaturesEXT robustness = {};

			VkPhysicalDeviceFragmentShadingRateFeaturesKHR fragmentShadingRate = {};
			VkPhysicalDeviceFragmentShaderBarycentricFeaturesKHR barycentric = {};

			VkPhysicalDeviceDescriptorIndexingFeatures descriptorIndexingFeatures{};
			VkPhysicalDeviceMutableDescriptorTypeFeaturesEXT mutableDescriptor = {};

			VkPhysicalDeviceBufferDeviceAddressFeatures bufferDeviceAddressFeatures{};
			VkPhysicalDeviceDynamicRenderingFeaturesKHR dynamicRenderingFeatures{};
			VkPhysicalDeviceSynchronization2FeaturesKHR sync2Features{};
			VkPhysicalDeviceShaderAtomicFloatFeaturesEXT atomicFeatures{};
			VkPhysicalDeviceAccelerationStructureFeaturesKHR accelerationStructureFeatures{};

			/*
			VkPhysicalDeviceFeatures deviceFeatures = {};
			deviceFeatures.samplerAnisotropy = VK_TRUE;                    // Enable anisotropic filtering
			deviceFeatures.wideLines = VK_TRUE;                            // Enable wide lines if needed
			deviceFeatures.fillModeNonSolid = VK_TRUE;                     // Enable non-solid fill modes
			deviceFeatures.geometryShader = VK_TRUE;                       // Enable geometry shaders if needed
			deviceFeatures.tessellationShader = VK_TRUE;                   // Enable tessellation shaders if needed
			deviceFeatures.independentBlend = VK_TRUE;                     // Enable independent blending if needed
			deviceFeatures.pipelineStatisticsQuery = VK_TRUE;              // Enable pipeline statistics queries if needed
			deviceFeatures.shaderStorageImageWriteWithoutFormat = VK_TRUE; // Enable storage image writes without format
			*/

			// ---------------------------------------------------------

			fragmentShadingRate.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FRAGMENT_SHADING_RATE_FEATURES_KHR;
			fragmentShadingRate.pNext = nullptr;

			// ---------------------------------------------------------

			robustness.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ROBUSTNESS_2_FEATURES_EXT;
			robustness.pNext = &fragmentShadingRate;

			// ---------------------------------------------------------

			vulkan12Features.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES;
			vulkan12Features.pNext = &robustness;

			// ---------------------------------------------------------

			vulkan13Features.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES;
			vulkan13Features.pNext = &vulkan12Features;

			// ---------------------------------------------------------

			vulkan14Features.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_4_FEATURES;
			vulkan14Features.pNext = &vulkan13Features;

			// ---------------------------------------------------------

			barycentric.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FRAGMENT_SHADER_BARYCENTRIC_FEATURES_KHR;
			barycentric.pNext = &vulkan14Features;

			// ---------------------------------------------------------

			mutableDescriptor.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MUTABLE_DESCRIPTOR_TYPE_FEATURES_EXT;
			mutableDescriptor.pNext = &barycentric;

			// ---------------------------------------------------------

			descriptorIndexingFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DESCRIPTOR_INDEXING_FEATURES;
			descriptorIndexingFeatures.pNext = &mutableDescriptor;
			descriptorIndexingFeatures.runtimeDescriptorArray = true;
			descriptorIndexingFeatures.descriptorBindingPartiallyBound = true;
			descriptorIndexingFeatures.shaderSampledImageArrayNonUniformIndexing = true;
			descriptorIndexingFeatures.shaderUniformBufferArrayNonUniformIndexing = true;
			descriptorIndexingFeatures.shaderStorageBufferArrayNonUniformIndexing = true;
			descriptorIndexingFeatures.descriptorBindingSampledImageUpdateAfterBind = true;
			descriptorIndexingFeatures.descriptorBindingStorageImageUpdateAfterBind = true;

			// ---------------------------------------------------------
			bufferDeviceAddressFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_BUFFER_DEVICE_ADDRESS_FEATURES;
			bufferDeviceAddressFeatures.bufferDeviceAddress = VK_TRUE;
			bufferDeviceAddressFeatures.pNext = &descriptorIndexingFeatures;

			// ---------------------------------------------------------

			accelerationStructureFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ACCELERATION_STRUCTURE_FEATURES_KHR;
			accelerationStructureFeatures.accelerationStructure = VK_TRUE;
			accelerationStructureFeatures.descriptorBindingAccelerationStructureUpdateAfterBind = VK_TRUE;
			accelerationStructureFeatures.accelerationStructureCaptureReplay = VK_TRUE;
			accelerationStructureFeatures.pNext = &bufferDeviceAddressFeatures;

			// ---------------------------------------------------------

			dynamicRenderingFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DYNAMIC_RENDERING_FEATURES_KHR;
			dynamicRenderingFeatures.dynamicRendering = VK_TRUE;
			dynamicRenderingFeatures.pNext = &accelerationStructureFeatures;

			// ---------------------------------------------------------

			sync2Features.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SYNCHRONIZATION_2_FEATURES_KHR;
			sync2Features.synchronization2 = VK_TRUE;
			sync2Features.pNext = &dynamicRenderingFeatures;

			// ---------------------------------------------------------

			atomicFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SHADER_ATOMIC_FLOAT_FEATURES_EXT;
			atomicFeatures.shaderBufferFloat32AtomicAdd = VK_TRUE;
			atomicFeatures.pNext = &sync2Features;

			// ---------------------------------------------------------

            features2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
			features2.features.geometryShader = VK_TRUE;
			features2.pNext = &mutableDescriptor;

			vkGetPhysicalDeviceFeatures2(physDevice->m_PhysicalDevice, &features2);

			// ---------------------------------------------------------

			{
				if (vulkan12Features.timelineSemaphore) vulkan12Features.timelineSemaphore = VK_TRUE;
                if (vulkan12Features.descriptorBindingVariableDescriptorCount) vulkan12Features.descriptorBindingVariableDescriptorCount = VK_TRUE;
				if (vulkan12Features.descriptorBindingSampledImageUpdateAfterBind) vulkan12Features.descriptorBindingSampledImageUpdateAfterBind = VK_TRUE;
				if (vulkan12Features.descriptorBindingPartiallyBound) vulkan12Features.descriptorBindingPartiallyBound = VK_TRUE;
				if (vulkan12Features.runtimeDescriptorArray) vulkan12Features.runtimeDescriptorArray = VK_TRUE;
				if (vulkan12Features.descriptorIndexing) vulkan12Features.descriptorIndexing = VK_TRUE;
				if (vulkan12Features.shaderStorageBufferArrayNonUniformIndexing) vulkan12Features.shaderStorageBufferArrayNonUniformIndexing = VK_TRUE;
				if (vulkan12Features.shaderSubgroupExtendedTypes) vulkan12Features.shaderSubgroupExtendedTypes = VK_TRUE;
				if (vulkan12Features.shaderFloat16) vulkan12Features.shaderFloat16 = VK_TRUE;
				if (vulkan12Features.shaderInt8) vulkan12Features.shaderInt8 = VK_TRUE;
				if (vulkan12Features.scalarBlockLayout) vulkan12Features.scalarBlockLayout = VK_TRUE;
			}

			// ---------------------------------------------------------

            {
				if (features2.features.logicOp) features2.features.logicOp = VK_TRUE;
				if (features2.features.shaderFloat64) features2.features.shaderFloat64 = VK_TRUE;
				if (features2.features.samplerAnisotropy) features2.features.samplerAnisotropy = VK_TRUE;
				if (features2.features.sampleRateShading) features2.features.sampleRateShading = VK_TRUE;
				if (features2.features.fillModeNonSolid) features2.features.fillModeNonSolid = VK_TRUE;
				if (features2.features.tessellationShader) features2.features.tessellationShader = VK_TRUE;
				if (features2.features.wideLines) features2.features.wideLines = VK_TRUE;
				if (features2.features.depthClamp) features2.features.depthClamp = VK_TRUE;
				if (features2.features.imageCubeArray) features2.features.imageCubeArray = VK_TRUE;
				if (features2.features.pipelineStatisticsQuery) features2.features.pipelineStatisticsQuery = VK_TRUE;
				if (features2.features.shaderInt16) features2.features.shaderInt16 = VK_TRUE;
				if (features2.features.geometryShader) features2.features.geometryShader = VK_TRUE;

                // ---------------------------------------------------------

				if (vulkan13Features.dynamicRendering) vulkan13Features.dynamicRendering = VK_TRUE;
				if (vulkan13Features.synchronization2) vulkan13Features.synchronization2 = VK_TRUE;
				if (vulkan13Features.shaderDemoteToHelperInvocation) vulkan13Features.shaderDemoteToHelperInvocation = VK_TRUE;
				if (vulkan13Features.subgroupSizeControl) vulkan13Features.subgroupSizeControl = VK_TRUE;
				if (vulkan13Features.shaderIntegerDotProduct) vulkan13Features.shaderIntegerDotProduct = VK_TRUE;

				if (robustness.nullDescriptor) robustness.nullDescriptor = VK_TRUE;
				if (mutableDescriptor.mutableDescriptorType) mutableDescriptor.mutableDescriptorType = VK_TRUE;

            }

            // ---------------------------------------------------------

            {
				if (vulkan14Features.pushDescriptor) vulkan14Features.pushDescriptor = VK_TRUE;
            }

        }

        // Create the logical device
        VkDeviceCreateInfo createInfo = {};
        createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    #if SEDX_NV_AFTERMATH
        if (canEnableAftermath)
            createInfo.pNext = &aftermathInfo;
    #endif
        createInfo.pQueueCreateInfos = physDevice->m_QueueCreateInfos.data();
        createInfo.queueCreateInfoCount = static_cast<uint32_t>(physDevice->m_QueueCreateInfos.size());

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

        //createInfo.pEnabledFeatures = &features2;
        //createInfo.pNext = &features2;

		VkPhysicalDeviceFragmentShadingRatePropertiesKHR shadingRateProperties = {};
		shadingRateProperties.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FRAGMENT_SHADING_RATE_PROPERTIES_KHR;

		VkPhysicalDeviceVulkan13Properties vulkan13Properties = {};
		vulkan13Properties.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_PROPERTIES;
        vulkan13Properties.pNext = &shadingRateProperties;

		VkPhysicalDeviceProperties2 deviceProperties2 = {};
        deviceProperties2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2;
        deviceProperties2.pNext = &vulkan13Properties;

		vkGetPhysicalDeviceProperties2(physDevice->m_PhysicalDevice, &deviceProperties2);

		properties::min_UniformBufferOffsetAlignment	= deviceProperties2.properties.limits.minUniformBufferOffsetAlignment;
		properties::min_StorageBufferOffsetAlignment	= deviceProperties2.properties.limits.minStorageBufferOffsetAlignment;
        properties::optimalBufferCopyOffsetAlignment	= deviceProperties2.properties.limits.optimalBufferCopyOffsetAlignment;
        properties::max_1D_TextureDimension				= deviceProperties2.properties.limits.maxImageDimension1D;
        properties::max_2D_TextureDimension				= deviceProperties2.properties.limits.maxImageDimension2D;
        properties::max_3D_TextureDimension				= deviceProperties2.properties.limits.maxImageDimension3D;
        properties::max_Cube_TextureDimension			= deviceProperties2.properties.limits.maxImageDimensionCube;
        properties::max_TextureArrayLayers				= deviceProperties2.properties.limits.maxImageArrayLayers;
        properties::max_PushConstantSize				= deviceProperties2.properties.limits.maxPushConstantsSize;
        properties::max_X_ShadingRateTexel				= shadingRateProperties.maxFragmentShadingRateAttachmentTexelSize.width;
        properties::max_Y_ShadingRateTexel				= shadingRateProperties.maxFragmentShadingRateAttachmentTexelSize.height;

        // Create the logical device
        if (VkResult result = vkCreateDevice(physDevice->m_PhysicalDevice, &createInfo, nullptr, &m_Device); result != VK_SUCCESS)
        {
            SEDX_CORE_ERROR_TAG("Graphics Engine", "Failed to create logical device! Error: {}", static_cast<int>(result));
            m_Device = VK_NULL_HANDLE; // Ensure device is set to null for error checking elsewhere
            return;
        }

        SEDX_CORE_INFO_TAG("Graphics Engine", "Logical device created successfully");

        // Get device queues
        vkGetDeviceQueue(m_Device, physDevice->m_QueueFamilyIndices.graphics, 0, &m_GraphicsQueue);
        vkGetDeviceQueue(m_Device, physDevice->m_QueueFamilyIndices.compute, 0, &m_ComputeQueue);
        vkGetDeviceQueue(m_Device, physDevice->m_QueueFamilyIndices.present, 0, &m_PresentQueue);
        vkGetDeviceQueue(m_Device, physDevice->m_QueueFamilyIndices.transfer, 0, &m_TransferQueue);

		#ifdef SEDX_DEBUG
		    SEDX_CORE_INFO_TAG("Graphics Engine", "Using queue family indices:");
		    SEDX_CORE_INFO("Graphics {}", physDevice->m_QueueFamilyIndices.GetGraphicsFamily());
		    SEDX_CORE_INFO("Compute {}", physDevice->m_QueueFamilyIndices.GetComputeFamily());
		    SEDX_CORE_INFO("Present {}", physDevice->m_QueueFamilyIndices.GetPresentFamily());
		    SEDX_CORE_INFO("Transfer {}", physDevice->m_QueueFamilyIndices.GetTransferFamily());
		#endif

		LoadExtensionFunctions(); // Load device extension function pointers

		/*uint32_t apiVersion = RenderData::GetVulkanAPIVersion(); // Get the Vulkan API version

		#ifdef SEDX_DEBUG
		    SEDX_CORE_INFO("Retrieved the current Vulkan API Version: {}", apiVersion);
		#endif*/
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

		#ifdef SEDX_DEBUG
			// Load debug utils functions
        vkSetDebugUtilsObjectNameEXT = reinterpret_cast<PFN_vkSetDebugUtilsObjectNameEXT>(vkGetDeviceProcAddr(this->m_Device, "vkSetDebugUtilsObjectNameEXT"));
		#endif

		    // Load ray tracing functions
		#ifdef SEDX_RAYTRACE
			vkGetAccelerationStructureBuildSizesKHR = reinterpret_cast<PFN_vkGetAccelerationStructureBuildSizesKHR>(vkGetDeviceProcAddr(this->device, "vkGetAccelerationStructureBuildSizesKHR"));
			vkCreateAccelerationStructureKHR = reinterpret_cast<PFN_vkCreateAccelerationStructureKHR>(vkGetDeviceProcAddr(this->device, "vkCreateAccelerationStructureKHR"));
			vkGetBufferDeviceAddressKHR = reinterpret_cast<PFN_vkGetBufferDeviceAddressKHR>(vkGetDeviceProcAddr(this->device, "vkGetBufferDeviceAddressKHR"));
			vkCmdBuildAccelerationStructuresKHR = reinterpret_cast<PFN_vkCmdBuildAccelerationStructuresKHR>(vkGetDeviceProcAddr(this->device, "vkCmdBuildAccelerationStructuresKHR"));
			vkGetAccelerationStructureDeviceAddressKHR = reinterpret_cast<PFN_vkGetAccelerationStructureDeviceAddressKHR>(vkGetDeviceProcAddr(this->device, "vkGetAccelerationStructureDeviceAddressKHR"));
			vkDestroyAccelerationStructureKHR = reinterpret_cast<PFN_vkDestroyAccelerationStructureKHR>(vkGetDeviceProcAddr(this->device, "vkDestroyAccelerationStructureKHR"));
		#endif
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
		// Wait for device to be idle before cleanup
		vkDeviceWaitIdle(m_Device);

		// Shutdown bindless descriptor system prior to device destruction
		// BindlessDescriptorManager::Shutdown();

		// Shutdown memory allocator before destroying the device
		MemoryAllocator::Shutdown();

		// Destroy logical device
        if (m_Device != VK_NULL_HANDLE)
		{
            vkDestroyDevice(m_Device, nullptr);
            m_Device = VK_NULL_HANDLE;
		}
	}

    // From device, or use the FrameTick in MemoryAllocator class.
    void VulkanDevice::Tick(const uint64_t frameCount) const
    {
        // https://gpuopen-librariesandsdks.github.io/VulkanMemoryAllocator/html/staying_within_budget.html
        // make sure to call vmaSetCurrentFrameIndex() every frame
        // budget is queried from Vulkan inside of it to avoid overhead of querying it with every allocation
        vmaSetCurrentFrameIndex(GetMemoryAllocator(), static_cast<uint32_t>(frameCount));
    }

	// -------------------------------------------------------

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
		// Query the global allocator directly; don't gate on a local member flag
        if (VmaAllocator alloc = MemoryAllocator::GetAllocator())
			return alloc;

		SEDX_CORE_ERROR_TAG ("Graphics Engine",
			"Memory allocator (VMA) not initialized. Ensure RenderContext::Init() completed successfully and MemoryAllocator::Init() was called after device creation.");
		return nullptr;
	}

    /**
	 * @brief Get a command buffer from the command pool
	 * @param begin Whether to begin the command buffer with ONE_TIME_SUBMIT
	 * @return The allocated command buffer
	 */
	/*
	VkCommandBuffer VulkanDevice::GetCommandBuffer (bool begin)
	{
		// Allocate a primary command buffer from the thread-local graphics command pool.
		// If begin is true, it will be begun with ONE_TIME_SUBMIT.
		return GetOrCreateThreadLocalCommandPool ()->AllocateCommandBuffer (begin, /*compute=#1#false);
	}
	*/

    /*
    bool VulkanDevice::IsValidResolution(uint32_t width, uint32_t height)
    {
        return width  > 4 && width  <= m_max_texture_2d_dimension &&
               height > 4 && height <= m_max_texture_2d_dimension;
    }
    */

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
          /*bindlessResources(other.bindlessResources),*/
            vkGetBufferDeviceAddressKHR(other.vkGetBufferDeviceAddressKHR),
            vkSetDebugUtilsObjectNameEXT(other.vkSetDebugUtilsObjectNameEXT),
            vkCreateAccelerationStructureKHR(other.vkCreateAccelerationStructureKHR),
            vkDestroyAccelerationStructureKHR(other.vkDestroyAccelerationStructureKHR),
            vkCmdBuildAccelerationStructuresKHR(other.vkCmdBuildAccelerationStructuresKHR),
            vkGetAccelerationStructureBuildSizesKHR(other.vkGetAccelerationStructureBuildSizesKHR),
            vkGetAccelerationStructureDeviceAddressKHR(other.vkGetAccelerationStructureDeviceAddressKHR),
            m_Device(other.m_Device), m_MemoryAlloc(std::move(other.m_MemoryAlloc)),
            m_PhysicalDevice(std::move(other.m_PhysicalDevice)),
            m_EnabledFeatures(other.m_EnabledFeatures)
    {
        other.m_Device = nullptr;
        other.m_TextureSampler = nullptr;
        /* other.scratchAddress = 0; */
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
            // Clean up existing resources
            Destroy();

            // Move resources from the other VulkanDevice
            m_Device = other.m_Device;
            m_PhysicalDevice = std::move(other.m_PhysicalDevice);
            m_EnabledFeatures = other.m_EnabledFeatures;
            m_GraphicsQueue = other.m_GraphicsQueue;
            m_ComputeQueue = other.m_ComputeQueue;
            m_PresentQueue = other.m_PresentQueue;
            m_MemoryAlloc = std::move(other.m_MemoryAlloc);
            /*bindlessResources = other.bindlessResources*/

            // Nullify the moved-from object
            other.m_Device = nullptr;
            other.m_GraphicsQueue = VK_NULL_HANDLE;
            other.m_ComputeQueue = VK_NULL_HANDLE;
            other.m_PresentQueue = VK_NULL_HANDLE;
            other.m_TextureSampler = nullptr;
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
		// Wait for device to be idle
        if (m_Device != VK_NULL_HANDLE)
		{
            vkDeviceWaitIdle(m_Device);
            vkDestroyDevice(m_Device, nullptr);
            m_Device = VK_NULL_HANDLE;
		}
	}

    uint32_t VulkanDevice::GetGPUQueues(const Ref<VulkanPhysicalDevice> &physDevice)
    {
        uint32_t queueFamilyCount = 0;
        vkGetPhysicalDeviceQueueFamilyProperties(physDevice->m_PhysicalDevice, &queueFamilyCount, nullptr);

        std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
        vkGetPhysicalDeviceQueueFamilyProperties(physDevice->m_PhysicalDevice, &queueFamilyCount, queueFamilies.data());

        uint32_t availableQueues = 0;

        // Track which queue families have been assigned
        bool computeQueueFound = false;
        bool graphicsQueueFound = false;
        bool transferQueueFound = false;
        bool presentQueueFound = false;

        // Search for separate compute queue
        for (size_t i = 0; i < queueFamilies.size(); i++)
        {
            if (queueFamilies[i].queueFlags & VK_QUEUE_COMPUTE_BIT && !computeQueueFound &&
                (queueFamilies[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) == 0)
            {
                m_ComputeQueueFamilyIndex = static_cast<uint32_t>(i);
                computeQueueFound = true;
                availableQueues++;
                break;
            }
        }

        // Find all queue types
        for (size_t i = 0; i < queueFamilies.size(); i++)
        {
            if (queueFamilies[i].queueFlags & VK_QUEUE_COMPUTE_BIT && !computeQueueFound)
            {
                m_ComputeQueueFamilyIndex = static_cast<uint32_t>(i);
                computeQueueFound = true;
                availableQueues++;
            }

            if (queueFamilies[i].queueFlags & VK_QUEUE_GRAPHICS_BIT && !graphicsQueueFound)
            {
                m_GraphicsQueueFamilyIndex = static_cast<uint32_t>(i);
                graphicsQueueFound = true;
                availableQueues++;
            }

            if (queueFamilies[i].queueFlags & VK_QUEUE_TRANSFER_BIT && !transferQueueFound)
            {
                m_TransferQueueFamilyIndex = static_cast<uint32_t>(i);
                transferQueueFound = true;
                availableQueues++;
            }

            VkBool32 presentSupport = false;
            vkGetPhysicalDeviceSurfaceSupportKHR(physDevice->m_PhysicalDevice,
                                                 static_cast<uint32_t>(i),
                                                 m_Surface,
                                                 &presentSupport);

            if (presentSupport && !presentQueueFound)
            {
                m_PresentQueueFamilyIndex = static_cast<uint32_t>(i);
                presentQueueFound = true;
                availableQueues++;
            }
        }

        SEDX_CORE_INFO_TAG("Renderer", "Found {} queue families:", availableQueues);
        if (graphicsQueueFound)
            SEDX_CORE_INFO_TAG("Renderer", "  Graphics queue family: {}", m_GraphicsQueueFamilyIndex);
        if (computeQueueFound)
            SEDX_CORE_INFO_TAG("Renderer", "  Compute queue family: {}", m_ComputeQueueFamilyIndex);
        if (transferQueueFound)
            SEDX_CORE_INFO_TAG("Renderer", "  Transfer queue family: {}", m_TransferQueueFamilyIndex);
        if (presentQueueFound)
            SEDX_CORE_INFO_TAG("Renderer", "  Present queue family: {}", m_PresentQueueFamilyIndex);

        return availableQueues;
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
			m_ComputeQueueMutex.lock();
		else
			m_GraphicsQueueMutex.lock();
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
			m_ComputeQueueMutex.unlock();
		else
			m_GraphicsQueueMutex.unlock();
	}

    /*
    void VulkanDevice::DeletionQueueAdd(const ResourceType type, void* resource)
    {
        if (!resource)
            return;

		std::lock_guard lock(mutexDeletionQueue);
        deletionQueue[type].emplace_back(resource);
    }
    */

    /*
    void VulkanDevice::DeletionQueueParse()
    {
        std::scoped_lock guard(mutexDeletionQueue);
        VkDevice device = RenderContext::GetCurrentDevice()->GetDevice();

        for (auto &it : deletionQueue)
        {
            ResourceType resourceType = it.first;

            for (auto resource : it.second)
            {
                switch (resourceType)
                {
                case ResourceType::Image: MemoryTextureDestroy(resource); 
                    break;
                case ResourceType::ImageView: vkDestroyImageView(device, static_cast<VkImageView>(resource), nullptr); 
                    break;
                case ResourceType::Sampler: vkDestroySampler(device, reinterpret_cast<VkSampler>(resource), nullptr);  
                    break;
                case ResourceType::Buffer: MemoryBufferDestroy(resource); 
                    break;
                case ResourceType::Shader: vkDestroyShaderModule(device, static_cast<VkShaderModule>(resource), nullptr); 
                    break;
                case ResourceType::Semaphore: vkDestroySemaphore(device, static_cast<VkSemaphore>(resource), nullptr);  
                    break;
                case ResourceType::Fence: vkDestroyFence(device, static_cast<VkFence>(resource), nullptr); 
                    break;
                case ResourceType::DescriptorSetLayout: vkDestroyDescriptorSetLayout(device, static_cast<VkDescriptorSetLayout>(resource), nullptr);
                    break;
                case ResourceType::QueryPool: vkDestroyQueryPool(device, static_cast<VkQueryPool>(resource), nullptr);
                    break;
                case ResourceType::Pipeline: vkDestroyPipeline(device, static_cast<VkPipeline>(resource), nullptr);
                    break;
                case ResourceType::PipelineLayout: vkDestroyPipelineLayout(device, static_cast<VkPipelineLayout>(resource), nullptr);
                    break;
                case ResourceType::AccelerationStructure: functions::destroy_acceleration_structure(device, static_cast<VkAccelerationStructureKHR>(resource), nullptr);
                    break;
                default:
                    SEDX_ASSERT(false, "Unknown resource");
                    break;
                }

                // delete descriptor sets which are now invalid (because they are referring to a deleted resource)
                if (resourceType == ResourceType::ImageView || resourceType == ResourceType::Buffer)
                {
                    for (auto it = descriptors::sets.begin(); it != descriptors::sets.end();)
                    {
                        if (it->second.IsReferingToResource(resource))
                        {
                            it = descriptors::sets.erase(it);
                            // ideally the descriptor set pool is not oblivious to the fact that we don't use this set anymore
                            // maybe after a certain number of deletions we reset the entire pool to free memory
                        }
                        else
                        {
                            ++it;
                        }
                    }
                }

                // samplers are bindless so they just update the set again
            }
        }

        deletionQueue.clear();
    }
    */


    /*
    bool VulkanDevice::DeletionQueueNeedsToParse()
    {
        static uint32_t framesEquilibrium = 0;
        static uint32_t objectsToDeletePrevious = 0;

        // count deletions in the queue
        uint32_t objectsToDelete = 0;
        for (uint32_t i = 0; i < static_cast<uint32_t>(ResourceType::MaxEnum); i++)
        {
            objectsToDelete += static_cast<uint32_t>(deletionQueue[static_cast<ResourceType>(i)].size());
        }

        // check if the number of objects to delete has remained unchanged
        if (objectsToDelete > 0 && objectsToDelete == objectsToDeletePrevious)
        {
            framesEquilibrium++;

            // if it’s been stable for frame_self life frames, reset counter and delete
            if (framesEquilibrium >= RENDERER_RESOURCE_FRAME_LIFETIME)
            {
                framesEquilibrium = 0;
                return true;
            }
        }
        else
        {
            // reset counter if the count changed or if nothing is in the queue
            framesEquilibrium = 0;
        }

        // update the previous object count to the current count
        objectsToDeletePrevious = objectsToDelete;

        return false;
    }
    */


    /*
    Ref<CommandPool> VulkanDevice::LocalCommandPool()
    {
        const auto threadID = std::this_thread::get_id();
        SEDX_CORE_VERIFY(CmdPools.contains(threadID));
        return CmdPools.at(threadID);
    }
    */

    /*
    Ref<CommandPool> VulkanDevice::CreateLocalCommandPool()
    {
        const auto threadID = std::this_thread::get_id();
        if (const auto commandPoolIt = CmdPools.find(threadID); commandPoolIt != CmdPools.end())
            return commandPoolIt->second;

        // Use CreateRef to construct a new CommandPool smart pointer
        Ref<CommandPool> commandPool = CreateRef<CommandPool>(Ref<VulkanDevice>(this), Queue::Graphics);
        CmdPools[threadID] = commandPool;
        return commandPool;
    }
    */

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
			for (uint32_t idx = 0; idx < selectedDevice.m_queueFamilyInfo.size(); idx++)
			{
				if (const VkQueueFamilyProperties &queueFamily = selectedDevice.m_queueFamilyInfo[idx]; queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT)
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

	// TODO: This should be moved to UI Renderer class
	/*
	VkCommandBuffer VulkanDevice::CreateUICmdBuffer(const char *debugName)
	{
		// Get the command pool for the current thread
        Ref<CommandPool> cmdPool = CreateLocalCommandPool();

		VkCommandBufferAllocateInfo allocInfo{};
		allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
		allocInfo.commandPool = cmdPool->GetGraphicsCmdPool();
		allocInfo.level = VK_COMMAND_BUFFER_LEVEL_SECONDARY;
		allocInfo.commandBufferCount = 1;

		VkCommandBuffer cmdBuffer;
		VK_CHECK_RESULT(vkAllocateCommandBuffers(device, &allocInfo, &cmdBuffer))

		// Set debug name if available
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
	*/

	/*
	Ref<CommandPool> VulkanDevice::GetThreadLocalCommandPool()
    {
        const auto threadID = std::this_thread::get_id();
        SEDX_CORE_VERIFY(CmdPools.contains(threadID));

        return CmdPools.at(threadID);
    }
    */

	/*
	Ref<CommandPool> VulkanDevice::GetOrCreateThreadLocalCommandPool()
    {
        const auto threadID = std::this_thread::get_id();
        if (const auto commandPoolIt = CmdPools.find(threadID); commandPoolIt != CmdPools.end())
            return commandPoolIt->second;

        Ref<CommandPool> commandPool = CreateRef<CommandPool>(Ref<VulkanDevice>(this), Queue::Graphics);
        CmdPools[threadID] = commandPool;
        return commandPool;
    }
    */

    /*
    void VulkanDevice::FlushCmdBuffer(VkCommandBuffer cmdBuffer) { GetThreadLocalCommandPool()->FlushCmdBuffer(cmdBuffer); }
    void VulkanDevice::FlushCmdBuffer(VkCommandBuffer cmdBuffer, VkQueue queue) { GetThreadLocalCommandPool()->FlushCmdBuffer (cmdBuffer, queue); }
    */

    // -------------------------------------------------------

	/**
	 * @brief Determine the maximum MSAA sample count supported by the GPU
	 *
	 * This method queries the physical device properties to determine the highest multisample
	 * antialiasing (MSAA) sample count that is supported for both color and depth attachments.
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
        Ref<VulkanPhysicalDevice> physDevice = GetPhysicalDevice();
		VkPhysicalDeviceProperties physicalDeviceProperties;
		vkGetPhysicalDeviceProperties(physDevice->m_PhysicalDevice, &physicalDeviceProperties);

		VkSampleCountFlags counts = physicalDeviceProperties.limits.framebufferColorSampleCounts;
		counts &= physicalDeviceProperties.limits.framebufferDepthSampleCounts;

		// Get the highest sample count that is supported
		if (counts & VK_SAMPLE_COUNT_64_BIT) return VK_SAMPLE_COUNT_64_BIT;
		if (counts & VK_SAMPLE_COUNT_32_BIT) return VK_SAMPLE_COUNT_32_BIT;
		if (counts & VK_SAMPLE_COUNT_16_BIT) return VK_SAMPLE_COUNT_16_BIT;
		if (counts & VK_SAMPLE_COUNT_8_BIT)  return VK_SAMPLE_COUNT_8_BIT;
		if (counts & VK_SAMPLE_COUNT_4_BIT)  return VK_SAMPLE_COUNT_4_BIT;
		if (counts & VK_SAMPLE_COUNT_2_BIT)  return VK_SAMPLE_COUNT_2_BIT;

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
		std::vector<VkDeviceQueueCreateInfo> m_queueCreateInfos;
		for (uint32_t family : uniqueFamilies) {
			VkDeviceQueueCreateInfo createInfo{};
			createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
			createInfo.queueFamilyIndex = family;
			createInfo.queueCount = 1;
			createInfo.pQueuePriorities = &priority;
			m_queueCreateInfos.push_back(createInfo);
		}

		auto supportedFeatures = GetDevice()->Selected().GFXFeatures;

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
		createInfo.queueCreateInfoCount = static_cast<uint32_t>(m_queueCreateInfos.size());
		createInfo.pQueueCreateInfos = m_queueCreateInfos.data();
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

		auto res = vkCreateDevice(GetDevice()->physicalDevice, &createInfo, renderData.allocator, &device);
		SEDX_ASSERT(res, "Failed to create logical device!");

		VmaVulkanFunctions vulkanFunctions = {};
		vulkanFunctions.vkGetInstanceProcAddr = &vkGetInstanceProcAddr;
		vulkanFunctions.vkGetDeviceProcAddr = &vkGetDeviceProcAddr;

		VmaAllocatorCreateInfo allocatorCreateInfo = {};
		allocatorCreateInfo.flags = VMA_ALLOCATOR_CREATE_EXT_MEMORY_BUDGET_BIT | VMA_ALLOCATOR_CREATE_BUFFER_DEVICE_ADDRESS_BIT;
		allocatorCreateInfo.vulkanApiVersion = VK_API_VERSION_1_3;
		allocatorCreateInfo.physicalDevice = GetDevice()->physicalDevice;
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
	/*
	VkSampler VulkanDevice::CreateSampler(float maxLOD) const
    {
		VkSamplerCreateInfo samplerInfo{};
		samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
		samplerInfo.magFilter = VK_FILTER_LINEAR;
		samplerInfo.minFilter = VK_FILTER_LINEAR;
		samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
		samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
		samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
		// TODO: Create separate sampler for shadow maps

		// Check if anisotropy is supported
		VkPhysicalDeviceFeatures deviceFeatures;
		vkGetPhysicalDeviceFeatures(m_PhysicalDevice->GetGPUDevices(), &deviceFeatures);

		if (deviceFeatures.samplerAnisotropy)
		{
			samplerInfo.anisotropyEnable = VK_TRUE;
			samplerInfo.maxAnisotropy = m_PhysicalDevice->GetLimits().maxSamplerAnisotropy;
		}
		else
		{
			samplerInfo.anisotropyEnable = VK_FALSE;
			samplerInfo.maxAnisotropy = 1.0f;
		}

		// Color to return when clamp is active in addressing mode
		samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
		samplerInfo.unnormalizedCoordinates = VK_FALSE;
		samplerInfo.compareEnable = VK_FALSE; //texels will be compared, result used in filtering ops
		samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;
		samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
		samplerInfo.mipLodBias = 0.0f;
		samplerInfo.minLod = 0.0f;
		samplerInfo.maxLod = maxLOD;

		VkSampler sampler = nullptr;
        VK_CHECK_RESULT(vkCreateSampler(m_Device, &samplerInfo, nullptr, &sampler))

		return sampler;
	}
	*/

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

	/*
	uint32_t VulkanDevice::FindMemoryType(const uint32_t typeFilter, const VkMemoryPropertyFlags properties) const
	{
		// Get memory properties from the physical m_Device
		VkPhysicalDeviceMemoryProperties memProperties;
		vkGetPhysicalDeviceMemoryProperties(m_PhysicalDevice->GetGPUDevices(), &memProperties);

		// Find a memory type that satisfies both the type filter and the property requirements
		for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++)
		{
			if (typeFilter & 1 << i && (memProperties.memoryTypes[i].propertyFlags & properties) == properties)
                return i;
        }

		SEDX_CORE_ERROR_TAG("Graphics Engine", "Failed to find suitable memory type!");
		return 0; // Return a default value to avoid undefined behavior
	}
	*/

    void VulkanDevice::SetDebugName(void *resource, const ResourceType resourceType, const char *name)
    {
        SEDX_ASSERT(resource != nullptr);
        SEDX_ASSERT(vkSetDebugUtilsObjectNameEXT != nullptr);

        VkDebugUtilsObjectNameInfoEXT nameInfo = {};
        nameInfo.sType			= VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT;
        nameInfo.objectType		= static_cast<VkObjectType>(resourceType);
        nameInfo.objectHandle	= reinterpret_cast<uint64_t>(resource);
        nameInfo.pObjectName	= name;

        // Set the debug name
        vkSetDebugUtilsObjectNameEXT(m_Device, &nameInfo);
    }

}

// -------------------------------------------------------
