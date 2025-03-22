/**
* -------------------------------------------------------
* Scenery Editor X
* -------------------------------------------------------
* Copyright (c) 2025 Thomas Ray 
* Copyright (c) 2025 Coalition of Freeware Developers
* -------------------------------------------------------
* vk_core.cpp
* -------------------------------------------------------
* Created: 17/3/2025
* -------------------------------------------------------
*/

#include <vulkan/vulkan.h>
#include <SceneryEditorX/renderer/vk_core.h>
#include <SceneryEditorX/renderer/vk_queue.h>
#include <SceneryEditorX/renderer/vk_util.h>

// -------------------------------------------------------

/**
 * @brief Callback function for Vulkan debug messages.
 *
 * This function is called by Vulkan when a validation error occurs.
 * It prints the validation error message to the console.
 *
 * @param severity The severity of the message.
 * @param type The type of the message.
 * @param pCallbackData The callback data containing the message.
 * @param pUserData User data provided when the callback was registered.
 * @return VkBool32 Always returns VK_FALSE.
 */
static VKAPI_ATTR VkBool32 VKAPI_CALL DebugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT severity,
													VkDebugUtilsMessageTypeFlagsEXT type,
													const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
													void* pUserData)
{
	printf("Debug callback: %s\n",pCallbackData->pMessage);
	printf("  Severity %s\n",VK_DEBUG_SEVERITY_STRING(severity));
	printf("  Type %s\n",VK_DEBUG_TYPE(type));
	printf("  Objects ");

	for (uint32_t i = 0; i < pCallbackData->objectCount; i++)
	{
		printf("%llx ",pCallbackData->pObjects[i].objectHandle);
	}

	printf("\n");

	return VK_FALSE; // Return false to continue execution
}

void DestroyDebugUtilsMessengerEXT(VkInstance m_Instance,
                                   VkDebugUtilsMessengerEXT debugMessenger,
                                   const VkAllocationCallbacks *pAllocator)
{
    // search for the requested function and return null if cannot find
    auto func = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(m_Instance, "vkDestroyDebugUtilsMessengerEXT");
    if (func != nullptr)
    {
        func(m_Instance, debugMessenger, pAllocator);
    }
}

void DebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT &MessengerInfo)
{
    MessengerInfo = {};
        MessengerInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
        MessengerInfo.pNext = VK_NULL_HANDLE;
        MessengerInfo.flags = 0;
        MessengerInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT |
                                        VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
                                        VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
        MessengerInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
                                    VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
                                    VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
        MessengerInfo.pfnUserCallback = &DebugCallback;
        MessengerInfo.pUserData = VK_NULL_HANDLE; // Optional
}


// -------------------------------------------------------

/**
 * @brief Required vulkan instance extensions.
 */
std::vector<const char *> instanceExtensions =
{
	{
		VK_KHR_SURFACE_EXTENSION_NAME,
		VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME,
		VK_KHR_WIN32_SURFACE_EXTENSION_NAME
	}
};

// -------------------------------------------------------

GraphicsEngine::GraphicsEngine(GLFWwindow &window) : m_Window(window)
{
	//m_PhysicalDevice = std::make_shared<VulkanDevice>();
	initEngine();

}

GraphicsEngine::~GraphicsEngine()
{
	// -------------------------------------------------------
 
	//vkDestroySwapchainKHR(*m_Device, *m_SwapChain, nullptr);

	vkDestroyDevice(*m_Device, nullptr);

	// -------------------------------------------------------

	PFN_vkDestroySurfaceKHR vkDestroySurface = VK_NULL_HANDLE;
	vkDestroySurface = (PFN_vkDestroySurfaceKHR)vkGetInstanceProcAddr(m_Instance, "vkDestroySurfaceKHR");
	if (!vkDestroySurface)
	{
		EDITOR_LOG_ERROR("Vulkan surface was destroyed");
		exit(1);
	};

	vkDestroySurface(m_Instance, m_Surface, NULL);

	// -------------------------------------------------------

	PFN_vkDestroyDebugUtilsMessengerEXT vkDestroyDebugUtilsMessenger = VK_NULL_HANDLE;
	vkDestroyDebugUtilsMessenger =
		(PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(m_Instance, "vkDestroyDebugUtilsMessengerEXT");
	if (!vkDestroyDebugUtilsMessenger)
	{
		EDITOR_LOG_ERROR("Cannot find address of vkDestroyDebugUtilsMessenger");
		exit(1);
	}

	vkDestroyDebugUtilsMessenger(m_Instance, m_DebugMessenger, NULL);

	// -------------------------------------------------------
 
    //Don't knwow which one I will keep
    destroy_instance();

	vkDestroyInstance(m_Instance, NULL);
	EDITOR_LOG_INFO("Vulkan instance destroyed");

	// -------------------------------------------------------

}

/**
 * @brief Initialize Vulkan.
 *
 * This function initializes Vulkan by creating an instance and setting up a debug callback.
 *
 * @param windowTitleTChar The name of the application.
 * @param window The handle to the window.
 *
 * @return true if Vulkan is initialized successfully, false otherwise.
 */
void GraphicsEngine::initEngine()
{
	create_instance(&m_Window);
	create_debug_callback();
	m_PhysicalDevice->initPhysicalDevice(m_Instance, m_Surface);
	//queue_family = physical_device.selectDevice(VK_QUEUE_GRAPHICS_BIT,true);
	create_device();
    create_logic_device();
	create_destruction_handler();
	//create_swap_chain();
	create_command_pool();
	create_sync_objects();
	create_descriptors();
	//ImgQueue.InitQueue(device,swap_chain,queue_family,0);
	//CreateCommandBuffers(1,&CpyCmdBuff);
}

void GraphicsEngine::create_instance(GLFWwindow* window)
{
    uint32_t layerCount;
    vkEnumerateInstanceLayerProperties(&layerCount, nullptr);
    layers.resize(layerCount);
    activeLayers.resize(layerCount);
    vkEnumerateInstanceLayerProperties(&layerCount, layers.data());

    // get all available extensions
    uint32_t extensionCount = 0;
    vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, 0);
    instanceExtensions.resize(extensionCount);
    activeExtensions.resize(extensionCount);
    vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, instanceExtensions.data());

    // Convert VkExtensionProperties to const char*
    std::vector<const char *> extensionNames;
    for (const auto &extension : instanceExtensions)
    {
        extensionNames.push_back(extension.extensionName);
    }

    // get api version
    vkEnumerateInstanceVersion(&apiVersion);

    // active default khronos validation layer
    bool khronosAvailable = false;
    for (size_t i = 0; i < layers.size(); i++)
    {
        activeLayers[i] = false;
        if (strcmp("VK_LAYER_KHRONOS_validation", layers[i].layerName) == 0)
        {
            activeLayers[i] = true;
            khronosAvailable = true;
            break;
        }
    }

    if (enableValidationLayers && !khronosAvailable)
    {
        EDITOR_LOG_ERROR("Default validation layer not available!");
    }

	//const std::vector<const char *> layers = {"VK_LAYER_KHRONOS_validation"};
    // active vulkan layer
    allocator = nullptr;

    // get the name of all layers if they are enabled
    if (enableValidationLayers)
    {
        for (size_t i = 0; i < layers.size(); i++)
        {
            if (activeLayers[i])
            {
                activeLayersNames.push_back(layers[i].layerName);
            }
        }
    }

	/*
	 * -------------------------------------------------------
	 * Initialize Vulkan Application Info (simplified)
	 * -------------------------------------------------------
	 */
    VkApplicationInfo AppInfo = {};
		AppInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;           // Vulkan structure type
		AppInfo.pNext = nullptr;                                      // Pointer to extension-specific structure
        AppInfo.pApplicationName = "Scenery Editor X";                // Application name
		AppInfo.applicationVersion = VK_MAKE_API_VERSION(1, 0, 0, 0); // Application version
        AppInfo.pEngineName = "Editor X Vulkan Engine";               // Engine name
        AppInfo.engineVersion = VK_MAKE_API_VERSION(1, 0, 0, 0);      // Engine version
        AppInfo.apiVersion = VK_API_VERSION_1_3;                      // Vulkan API version

    /*
	 * -------------------------------------------------------
	 * Initialize Vulkan Instance Info (simplified)
	 * -------------------------------------------------------
	 */
    VkInstanceCreateInfo CreateInfo{};
        CreateInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO; // Vulkan structure type
        CreateInfo.pApplicationInfo = &AppInfo;                    // Application info
        CreateInfo.flags = 0;                                      // Reserved for future use

    // get all extensions required by glfw
    uint32_t glfwExtensionCount = 0;
    const char **glfwExtensions;
    glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);
    auto requiredExtensions = std::vector<const char *>(glfwExtensions, glfwExtensions + glfwExtensionCount);

    // include the extensions required by us
    if (enableValidationLayers)
    {
        requiredExtensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
    }

    // set to active all extensions that we enabled
    for (size_t i = 0; i < requiredExtensions.size(); i++)
    {
        for (size_t j = 0; j < instanceExtensions.size(); j++)
        {
            if (strcmp(requiredExtensions[i], instanceExtensions[j].extensionName) == 0)
            {
                activeExtensions[j] = true;
                break;
            }
        }
    }

    // get the name of all extensions that we enabled
    activeExtensionsNames.clear();
    for (size_t i = 0; i < instanceExtensions.size(); i++)
    {
        if (activeExtensions[i])
        {
            activeExtensionsNames.push_back(instanceExtensions[i].extensionName);
        }
    }

        // get and set all required extensions
        CreateInfo.enabledExtensionCount = static_cast<uint32_t>(activeExtensionsNames.size());
        CreateInfo.ppEnabledExtensionNames = activeExtensionsNames.data();

    VkDebugUtilsMessengerCreateInfoEXT debugMessengerInfo;
    if (enableValidationLayers)
    {
        CreateInfo.enabledLayerCount = static_cast<uint32_t>(activeLayersNames.size()); // Number of enabled layers
        CreateInfo.ppEnabledLayerNames = activeLayersNames.data();                      // Enabled layers

        DebugMessengerCreateInfo(debugMessengerInfo); // Pointer to extension-specific structure
        // CreateInfo.pNext = (VkDebugUtilsMessengerCreateInfoEXT*)&debugMessengerInfo;
    }
    else
    {
        CreateInfo.enabledLayerCount = 0;
        CreateInfo.pNext = nullptr;
    }

    auto result = vkCreateInstance(&CreateInfo, allocator, &m_Instance);

    if (enableValidationLayers)
    {
        VkDebugUtilsMessengerCreateInfoEXT messengerInfo;
        DebugMessengerCreateInfo(messengerInfo);

        VK_CHECK_RESULT(vkCreateDebugUtilsMessengerEXT(m_Instance, &messengerInfo, allocator, &m_DebugMessenger),
                        "Vulkan Debug Messenger");
        DEBUG_TRACE("Created debug messenger.");
    }

	if (vkCreateInstance(&CreateInfo, nullptr, &m_Instance) != VK_SUCCESS)
	{
		EDITOR_LOG_INFO("Failed to create Vulkan instance.");
		ErrMsg("Failed to create Vulkan instance.");
		exit(1);
	}

    result = glfwCreateWindowSurface(m_Instance, window, allocator, &m_Surface);
	//VkResult result = vkCreateInstance(&CreateInfo, nullptr, &m_Instance);
	VK_CHECK_RESULT(result,"Vulkan Instance");
	EDITOR_LOG_INFO("Vulkan instance created successfully.");
	EDITOR_LOG_INFO("Vulkan API Version: {}.{}.{}",
						VK_VERSION_MAJOR(AppInfo.apiVersion),
						VK_VERSION_MINOR(AppInfo.apiVersion),
						VK_VERSION_PATCH(AppInfo.apiVersion));
}

void GraphicsEngine::destroy_instance()
{
    activeLayersNames.clear();
    activeExtensionsNames.clear();
    if (m_DebugMessenger)
    {
        DestroyDebugUtilsMessengerEXT(m_Instance, m_DebugMessenger, allocator);
        DEBUG_TRACE("Destroyed debug messenger.");
        m_DebugMessenger = nullptr;
    }
    vkDestroySurfaceKHR(m_Instance, m_Surface, allocator);
    DEBUG_TRACE("Destroyed surface.");
    vkDestroyInstance(m_Instance, allocator);
    DEBUG_TRACE("Destroyed instance.");
    EDITOR_LOG_INFO("Destroyed Vulkan Instance");
}

/**
 * @brief - Create a debug callback.
 *
 * @param m_instance - The Vulkan instance.
 * @param DebugMessenger - The Vulkan debug messenger.
 * @return VkDebugUtilsMessengerEXT The Vulkan debug messenger.
 */
void GraphicsEngine::create_debug_callback()
{
	VkDebugUtilsMessengerCreateInfoEXT debugMessengerInfo = {
		.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT,
		.pNext = VK_NULL_HANDLE,
		.flags = 0,
		.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT |
						   VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
						   VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT,
		.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
					   VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT,
		.pfnUserCallback = &DebugCallback,
		.pUserData = VK_NULL_HANDLE, // Optional
	};

    // ---------------------------

	PFN_vkCreateDebugUtilsMessengerEXT vkCreateDebugUtilsMessenger = VK_NULL_HANDLE;
	vkCreateDebugUtilsMessenger = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(m_Instance,"vkCreateDebugUtilsMessengerEXT");
	if (!vkCreateDebugUtilsMessenger)
	{
		EDITOR_LOG_ERROR("Cannot find address of vkCreateDebugUtilsMessenger");
		ErrMsg("Cannot find address of vkCreateDebugUtilsMessenger");
		exit(1);
	}

	VkResult result = vkCreateDebugUtilsMessenger(m_Instance,&debugMessengerInfo,VK_NULL_HANDLE,&m_DebugMessenger);
	VK_CHECK_RESULT(result,"Vulkan Debug Utility Messanger");
}

void GraphicsEngine::create_device()
{
    uint32_t count = 0;
    vkEnumeratePhysicalDevices(m_Instance, &count, nullptr);
    ASSERT(count != 0, "no GPUs with Vulkan support!");
    std::vector<VkPhysicalDevice> devices(count);
    vkEnumeratePhysicalDevices(m_Instance, &count, devices.data());

    // get first physical device that matches all requirements
    for (const auto &device : devices)
    {
        // get all available extensions
        uint32_t extensionCount;
        vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, nullptr);
        availableExtensions.resize(extensionCount);
        vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, availableExtensions.data());

        // get all available families
        uint32_t familyCount = 0;
        vkGetPhysicalDeviceQueueFamilyProperties(device, &familyCount, nullptr);
        availableFamilies.resize(familyCount);
        vkGetPhysicalDeviceQueueFamilyProperties(device, &familyCount, availableFamilies.data());

        int computeFamily = -1;
        int transferFamily = -1;
        int graphicsFamily = -1;

        // select the family for each type of queue that we want
        for (int i = 0; i < familyCount; i++)
        {
            auto &family = availableFamilies[i];
            if (family.queueFlags & VK_QUEUE_GRAPHICS_BIT && graphicsFamily == -1)
            {
                VkBool32 present = false;
                vkGetPhysicalDeviceSurfaceSupportKHR(device, i, this->m_Surface, &present);
                if (present)
                {
                    graphicsFamily = i;
                }
                continue;
            }

            if (family.queueFlags & VK_QUEUE_COMPUTE_BIT && computeFamily == -1)
            {
                computeFamily = i;
                continue;
            }

            if (family.queueFlags & VK_QUEUE_TRANSFER_BIT && transferFamily == -1)
            {
                transferFamily = i;
                continue;
            }
        }

        queues[Queue::Graphics].family = graphicsFamily;
        queues[Queue::Compute].family = computeFamily == -1 ? graphicsFamily : computeFamily;
        queues[Queue::Transfer].family = transferFamily == -1 ? graphicsFamily : transferFamily;

        // get max number of samples
        vkGetPhysicalDeviceFeatures(device, &physicalFeatures);
        vkGetPhysicalDeviceProperties(device, &physicalProperties);
        vkGetPhysicalDeviceMemoryProperties(device, &memoryProperties);

        VkSampleCountFlags counts = physicalProperties.limits.framebufferColorSampleCounts;
        counts &= physicalProperties.limits.framebufferDepthSampleCounts;

        maxSamples = VK_SAMPLE_COUNT_1_BIT;
        if (counts & VK_SAMPLE_COUNT_64_BIT) { maxSamples = VK_SAMPLE_COUNT_64_BIT; }
        else if (counts & VK_SAMPLE_COUNT_32_BIT) { maxSamples = VK_SAMPLE_COUNT_32_BIT; }
        else if (counts & VK_SAMPLE_COUNT_16_BIT) { maxSamples = VK_SAMPLE_COUNT_16_BIT; }
        else if (counts & VK_SAMPLE_COUNT_8_BIT) { maxSamples = VK_SAMPLE_COUNT_8_BIT; }
        else if (counts & VK_SAMPLE_COUNT_4_BIT) { maxSamples = VK_SAMPLE_COUNT_4_BIT; }
        else if (counts & VK_SAMPLE_COUNT_2_BIT) { maxSamples = VK_SAMPLE_COUNT_2_BIT; }

        // check if all required extensions are available
        std::set<std::string> required(instanceExtensions.begin(), instanceExtensions.end());
        for (const auto &extension : availableExtensions)
        {
            required.erase(std::string(extension.extensionName));
        }

        // check if all required queues are supported
        bool suitable = required.empty();
        suitable &= graphicsFamily != -1;

        if (suitable)
        {
            g_PhysicalDevice = device;
            break;
        }
    }
}

void GraphicsEngine::create_logic_device()
{
    std::set<uint32_t> uniqueFamilies;
    for (int q = 0; q < Queue::Count; q++)
    {
        uniqueFamilies.emplace(queues[q].family);
    };

    // -----------------------------------------

    // priority for each type of queue
    float priority = 1.0f;
    std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
    for (uint32_t family : uniqueFamilies)
    {
        VkDeviceQueueCreateInfo createInfo{};
            createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
            createInfo.queueFamilyIndex = family;
            createInfo.queueCount = 1;
            createInfo.pQueuePriorities = &priority;

        queueCreateInfos.push_back(createInfo);
    }

    // -----------------------------------------

    auto supportedFeatures = physicalFeatures;

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

    // -----------------------------------------

    auto requiredExtensions = instanceExtensions;
    auto allExtensions = availableExtensions;
    for (auto &required : instanceExtensions)
    {
        bool available = false;
        for (size_t i = 0; i < availableExtensions.size(); i++)
        {
            if (strcmp(availableExtensions[i].extensionName,  ToConstChar(required)) == 0)
            {
                available = true;
                break;
            }
        }
        if (!available)
        {
            EDITOR_LOG_ERROR("Required extension {0} not available!", ToString(required));
        }
    }

    // -----------------------------------------

    VkPhysicalDeviceDescriptorIndexingFeatures descriptorIdxFeat{};
        descriptorIdxFeat.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DESCRIPTOR_INDEXING_FEATURES;
        descriptorIdxFeat.runtimeDescriptorArray = true;
        descriptorIdxFeat.descriptorBindingPartiallyBound = true;
        descriptorIdxFeat.shaderSampledImageArrayNonUniformIndexing = true;
        descriptorIdxFeat.shaderUniformBufferArrayNonUniformIndexing = true;
        descriptorIdxFeat.shaderStorageBufferArrayNonUniformIndexing = true;
        descriptorIdxFeat.descriptorBindingSampledImageUpdateAfterBind = true;
        descriptorIdxFeat.descriptorBindingStorageImageUpdateAfterBind = true;

    VkPhysicalDeviceBufferDeviceAddressFeatures bufferDeviceAddresFeatures{};
        bufferDeviceAddresFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_BUFFER_DEVICE_ADDRESS_FEATURES;
        bufferDeviceAddresFeatures.bufferDeviceAddress = VK_TRUE;
        bufferDeviceAddresFeatures.pNext = &descriptorIdxFeat;

    VkPhysicalDeviceRayTracingPipelineFeaturesKHR rayTracingPipelineFeatures{};
        rayTracingPipelineFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_TRACING_PIPELINE_FEATURES_KHR;
        rayTracingPipelineFeatures.rayTracingPipeline = VK_TRUE;
        rayTracingPipelineFeatures.pNext = &bufferDeviceAddresFeatures;

    VkPhysicalDeviceAccelerationStructureFeaturesKHR accelerationStructureFeatures{};
        accelerationStructureFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ACCELERATION_STRUCTURE_FEATURES_KHR;
        accelerationStructureFeatures.accelerationStructure = VK_TRUE;
        accelerationStructureFeatures.descriptorBindingAccelerationStructureUpdateAfterBind = VK_TRUE;
        accelerationStructureFeatures.accelerationStructureCaptureReplay = VK_TRUE;
        accelerationStructureFeatures.pNext = &rayTracingPipelineFeatures;

    VkPhysicalDeviceRayQueryFeaturesKHR rayQueryFeatures{};
        rayQueryFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_QUERY_FEATURES_KHR;
        rayQueryFeatures.rayQuery = VK_TRUE;
        rayQueryFeatures.pNext = &accelerationStructureFeatures;

    VkPhysicalDeviceDynamicRenderingFeaturesKHR dynamicRenderingFeatures{};
        dynamicRenderingFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DYNAMIC_RENDERING_FEATURES_KHR;
        dynamicRenderingFeatures.dynamicRendering = VK_TRUE;
        dynamicRenderingFeatures.pNext = &rayQueryFeatures;

    VkPhysicalDeviceSynchronization2FeaturesKHR sync2Features{};
        sync2Features.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SYNCHRONIZATION_2_FEATURES_KHR;
        sync2Features.synchronization2 = VK_TRUE;
        sync2Features.pNext = &dynamicRenderingFeatures;

    VkPhysicalDeviceShaderAtomicFloatFeaturesEXT atomicFeatures{};
        atomicFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SHADER_ATOMIC_FLOAT_FEATURES_EXT;
        atomicFeatures.shaderBufferFloat32AtomicAdd = VK_TRUE;
        atomicFeatures.pNext = &sync2Features;

    features2.pNext = &atomicFeatures;

    // -----------------------------------------

    VkDeviceCreateInfo createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
        createInfo.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size());
        createInfo.pQueueCreateInfos = queueCreateInfos.data();
        createInfo.enabledExtensionCount = static_cast<uint32_t>(instanceExtensions.size());
        createInfo.ppEnabledExtensionNames = instanceExtensions.data();
        createInfo.pEnabledFeatures;
        createInfo.pNext = &features2;

    // specify the required layers to the device
    if (enableValidationLayers)
    {
        auto &layers = activeLayersNames;
        createInfo.enabledLayerCount = static_cast<uint32_t>(layers.size());
        createInfo.ppEnabledLayerNames = layers.data();
    }
    else
    {
        createInfo.enabledLayerCount = 0;
    }

    auto result = vkCreateDevice(g_PhysicalDevice, &createInfo, nullptr, &g_Device);
    //DEBUG_VK(result, "Failed to create logical device!");

    // -----------------------------------------

    VmaVulkanFunctions vulkanFunctions = {};
        vulkanFunctions.vkGetInstanceProcAddr = &vkGetInstanceProcAddr;
        vulkanFunctions.vkGetDeviceProcAddr = &vkGetDeviceProcAddr;

    // -----------------------------------------

    VmaAllocatorCreateInfo allocatorCreateInfo = {};
        allocatorCreateInfo.flags =
            VMA_ALLOCATOR_CREATE_EXT_MEMORY_BUDGET_BIT |
            VMA_ALLOCATOR_CREATE_BUFFER_DEVICE_ADDRESS_BIT;
        allocatorCreateInfo.vulkanApiVersion = VK_API_VERSION_1_3;
        allocatorCreateInfo.physicalDevice = g_PhysicalDevice;
        allocatorCreateInfo.device = g_Device;
        allocatorCreateInfo.instance = m_Instance;
        allocatorCreateInfo.pVulkanFunctions = &vulkanFunctions;

    vmaCreateAllocator(&allocatorCreateInfo, &vmaAllocator);

    for (int q = 0; q < Queue::Count; q++)
    {
        vkGetDeviceQueue(g_Device, queues[q].family, 0, &queues[q].queue);
    }

    // -----------------------------------------

    genericSampler = CreateSampler(1.0);

    vkSetDebugUtilsObjectNameEXT = (PFN_vkSetDebugUtilsObjectNameEXT)vkGetDeviceProcAddr(g_Device, "vkSetDebugUtilsObjectNameEXT");
    vkGetAccelerationStructureBuildSizesKHR = (PFN_vkGetAccelerationStructureBuildSizesKHR)vkGetDeviceProcAddr(g_Device, "vkGetAccelerationStructureBuildSizesKHR");
    vkCreateAccelerationStructureKHR = (PFN_vkCreateAccelerationStructureKHR)vkGetDeviceProcAddr(g_Device, "vkCreateAccelerationStructureKHR");
    vkCmdBuildAccelerationStructuresKHR = (PFN_vkCmdBuildAccelerationStructuresKHR)vkGetDeviceProcAddr(g_Device, "vkCmdBuildAccelerationStructuresKHR");
    vkGetAccelerationStructureDeviceAddressKHR = (PFN_vkGetAccelerationStructureDeviceAddressKHR)vkGetDeviceProcAddr(g_Device, "vkGetAccelerationStructureDeviceAddressKHR");
    vkDestroyAccelerationStructureKHR =(PFN_vkDestroyAccelerationStructureKHR)vkGetDeviceProcAddr(g_Device, "vkDestroyAccelerationStructureKHR");

    VkDescriptorPoolSize imguiPoolSizes[] = {
        {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1000},
        {VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1000}
    };

    // -----------------------------------------

    VkDescriptorPoolCreateInfo imguiPoolInfo{};
        imguiPoolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
        imguiPoolInfo.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
        imguiPoolInfo.maxSets = (uint32_t)(1024);
        imguiPoolInfo.poolSizeCount = sizeof(imguiPoolSizes) / sizeof(VkDescriptorPoolSize);
        imguiPoolInfo.pPoolSizes = imguiPoolSizes;

    //VkResult result = vkCreateDescriptorPool(g_Device, &imguiPoolInfo, allocator, &imguiDescriptorPool);
    //DEBUG_VK(result, "Failed to create imgui descriptor pool!");


    // -----------------------------------------
    // 
    // Create bindless resources
    {
        const uint32_t MAX_STORAGE                  = 8192;
        const uint32_t MAX_SAMPLEDIMAGES            = 8192;
        const uint32_t MAX_ACCELERATIONSTRUCTURE    = 64;
        const uint32_t MAX_STORAGE_IMAGES           = 8192;

        for (int i = 0; i < MAX_STORAGE; i++)
        {
            availableBufferRID.push_back(i);
        }
        for (int i = 0; i < MAX_SAMPLEDIMAGES; i++)
        {
            availableImageRID.push_back(i);
        }
        for (int i = 0; i < MAX_ACCELERATIONSTRUCTURE; i++)
        {
            availableTLASRID.push_back(i);
        }

        // create descriptor set pool for bindless resources
        std::vector<VkDescriptorPoolSize> bindlessPoolSizes =
        {
            {VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, MAX_SAMPLEDIMAGES},
            {VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, MAX_STORAGE},
            {VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR, MAX_ACCELERATIONSTRUCTURE},
            {VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, MAX_STORAGE_IMAGES},
        };

        VkDescriptorPoolCreateInfo bindlessPoolInfo{};
            bindlessPoolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
            bindlessPoolInfo.flags = VK_DESCRIPTOR_POOL_CREATE_UPDATE_AFTER_BIND_BIT;
            bindlessPoolInfo.maxSets = 1;
            bindlessPoolInfo.poolSizeCount = bindlessPoolSizes.size();
            bindlessPoolInfo.pPoolSizes = bindlessPoolSizes.data();

        result = vkCreateDescriptorPool(g_Device, &bindlessPoolInfo, allocator, &bindlessDescriptorPool);
        EDITOR_LOG_ERROR("Failed to create bindless descriptor pool!");
        //DEBUG_VK(result, "Failed to create bindless descriptor pool!");

        // create descriptor set layout for bindless resources
        std::vector<VkDescriptorSetLayoutBinding> bindings;
        std::vector<VkDescriptorBindingFlags> bindingFlags;

        VkDescriptorSetLayoutBinding texturesBinding{};
            texturesBinding.binding = 0;
            texturesBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
            texturesBinding.descriptorCount = MAX_SAMPLEDIMAGES;
            texturesBinding.stageFlags = VK_SHADER_STAGE_ALL;
            bindings.push_back(texturesBinding);
            bindingFlags.push_back(
                {VK_DESCRIPTOR_BINDING_PARTIALLY_BOUND_BIT | VK_DESCRIPTOR_BINDING_UPDATE_AFTER_BIND_BIT});

        VkDescriptorSetLayoutBinding storageBuffersBinding{};
            storageBuffersBinding.binding = 0;
            storageBuffersBinding.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
            storageBuffersBinding.descriptorCount = MAX_STORAGE;
            storageBuffersBinding.stageFlags = VK_SHADER_STAGE_ALL;
            bindings.push_back(storageBuffersBinding);
            bindingFlags.push_back({VK_DESCRIPTOR_BINDING_PARTIALLY_BOUND_BIT});

        VkDescriptorSetLayoutBinding accelerationStructureBinding{};
            accelerationStructureBinding.binding = 2;
            accelerationStructureBinding.descriptorType = VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR;
            accelerationStructureBinding.descriptorCount = MAX_ACCELERATIONSTRUCTURE;
            accelerationStructureBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
            bindings.push_back(accelerationStructureBinding);
            bindingFlags.push_back(
                {VK_DESCRIPTOR_BINDING_PARTIALLY_BOUND_BIT | VK_DESCRIPTOR_BINDING_UPDATE_AFTER_BIND_BIT});

        VkDescriptorSetLayoutBinding imageStorageBinding{};
            imageStorageBinding.binding = 3;
            imageStorageBinding.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
            imageStorageBinding.descriptorCount = MAX_STORAGE_IMAGES;
            imageStorageBinding.stageFlags = VK_SHADER_STAGE_ALL;
            bindings.push_back(imageStorageBinding);
            bindingFlags.push_back(
                {VK_DESCRIPTOR_BINDING_PARTIALLY_BOUND_BIT | VK_DESCRIPTOR_BINDING_UPDATE_AFTER_BIND_BIT});

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

        result = vkCreateDescriptorSetLayout(g_Device, &descriptorLayoutInfo, allocator, &bindlessDescriptorLayout);
        //DEBUG_VK(result, "Failed to create bindless descriptor set layout!");

        // create descriptor set for bindless resources
        VkDescriptorSetAllocateInfo allocInfo{};
            allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
            allocInfo.descriptorPool = bindlessDescriptorPool;
            allocInfo.descriptorSetCount = 1;
            allocInfo.pSetLayouts = &bindlessDescriptorLayout;

        result = vkAllocateDescriptorSets(g_Device, &allocInfo, &bindlessDescriptorSet);
        //DEBUG_VK(result, "Failed to allocate bindless descriptor set!");
    }

    asScratchBuffer = CreateBuffer(initialScratchBufferSize,BufferUsage::Address | BufferUsage::Storage,Memory::GPU);

    VkBufferDeviceAddressInfo scratchInfo{};
        scratchInfo.sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO;
        scratchInfo.buffer = asScratchBuffer.resource->buffer;

    asScratchAddress = vkGetBufferDeviceAddress(g_Device, &scratchInfo);

    dummyVertexBuffer = CreateBuffer(6 * 3 * sizeof(float), BufferUsage::Vertex | BufferUsage::AccelerationStructureInput, Memory::GPU,"VertexBuffer#Dummy");
}

const VkPhysicalDeviceProperties &GraphicsEngine::getPhysicalProperties() const
{
    return physicalProperties;
}

void GraphicsEngine::create_destruction_handler() {}

void GraphicsEngine::create_command_pool() {}

void GraphicsEngine::create_sync_objects() {}

void GraphicsEngine::create_descriptors() {}

int CmdBeginTimeStamp(const std::string &name)
{
    DEBUG_ASSERT(currentQueue != Queue::Transfer, "Time Stamp not supported in Transfer queue");
    auto &cmd = GetCurrentCommandResources();
    int id = cmd.timeStamps.size();
    if (id >= timeStampPerPool - 1)
    {
        EDITOR_LOG_WARN("Maximum number of time stamp per pool exceeded. Ignoring Time stamp {}", name.c_str());
        return -1;
    }
    vkCmdWriteTimestamp(cmd.buffer, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, cmd.queryPool, id);
        cmd.timeStamps.push_back(0);
        cmd.timeStamps.push_back(0);
        cmd.timeStampNames.push_back(name);
    return id;
}
