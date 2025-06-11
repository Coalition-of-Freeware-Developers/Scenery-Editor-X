/**
* -------------------------------------------------------
* Scenery Editor X
* -------------------------------------------------------
* Copyright (c) 2025 Thomas Ray 
* Copyright (c) 2025 Coalition of Freeware Developers
* -------------------------------------------------------
* render_context.cpp
* -------------------------------------------------------
* Created: 7/6/2025
* -------------------------------------------------------
*/
#include <GraphicsEngine/vulkan/vk_util.h>
#include <SceneryEditorX/renderer/render_context.h>

/// -------------------------------------------------------

namespace SceneryEditorX
{

	RenderContext::RenderContext() = default;

    RenderContext::~RenderContext()
    {
        /// Device is destroyed in GLFWWindow::Shutdown()
        vkDestroyInstance(instance, nullptr);
        instance = VK_NULL_HANDLE;
    }

    /// DebugUtilsMessenger utility functions
    LOCAL VkResult CreateDebugUtilsMessengerEXT(VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT *pCreateInfo, const VkAllocationCallbacks *pAllocator,VkDebugUtilsMessengerEXT *pDebugMessenger)
    {
        /// search for the requested function and return null if unable find
        auto func = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT");
        if (func != nullptr)
            return func(instance, pCreateInfo, pAllocator, pDebugMessenger);
        return VK_ERROR_EXTENSION_NOT_PRESENT;
    }

    LOCAL void DestroyDebugUtilsMessengerEXT(VkInstance instance, VkDebugUtilsMessengerEXT debugMessenger, const VkAllocationCallbacks *pAllocator)
    {
        /// search for the requested function and return null if unable find
        auto func = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT");
        if (func != nullptr)
            func(instance, debugMessenger, pAllocator);
    }

    LOCAL VKAPI_ATTR VkBool32 VKAPI_CALL VulkanDebugMsgCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity, VkDebugUtilsMessageTypeFlagsEXT messageType, const VkDebugUtilsMessengerCallbackDataEXT *pCallbackData, void *pUserData)
    {
        /// if (messageSeverity > VK_DEBUG_UTILS...)
        if (messageSeverity >= VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT)
            SEDX_CORE_WARN("[Validation Layer] {0}", pCallbackData->pMessage);

        return VK_FALSE;
    }

    LOCAL void PopulateDebugMsgCreateInfo(VkDebugUtilsMessengerCreateInfoEXT &createInfo)
    {
        createInfo = {};
        createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
        createInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT;
        createInfo.messageSeverity |= VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT;
        createInfo.messageSeverity |= VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
        createInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT;
        createInfo.messageType |= VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT;
        createInfo.messageType |= VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
        createInfo.pfnUserCallback = VulkanDebugMsgCallback;
        createInfo.pUserData = nullptr;
    }

    LOCAL Extensions GetRequiredExtensions(Extensions vkExtensions)
    {
        uint32_t glfwExtensionCount = 0;
        const char **glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);
        vkExtensions.deviceExtensions = std::vector(glfwExtensions, glfwExtensions + glfwExtensionCount);
        if (enableValidationLayers)
            vkExtensions.deviceExtensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);

        return vkExtensions;
    }

    void RenderContext::Init()
	{
        if (!VulkanChecks::CheckAPIVersion(RenderData::minVulkanVersion))
            SEDX_CORE_ERROR_TAG("Graphics Engine", "Incompatible Vulkan driver version!");

	    SEDX_CORE_TRACE_TAG("Graphics Engine", "Creating Vulkan Instance");

	    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
        /// Application Info
        /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
        AppData appData;
        RenderData renderData;
        uint32_t apiVersion = 0;

	    VkApplicationInfo appInfo{};
        appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
        appInfo.pApplicationName = appData.appName.c_str();
        appInfo.applicationVersion = AppData::version;
        appInfo.pEngineName = AppData::renderName.c_str();
        appInfo.engineVersion = VK_MAKE_API_VERSION(1, 0, 0, 0);
        appInfo.apiVersion = apiVersion;

		/// ---------------------------------------------------------

        std::vector<const char *> instanceExtensions = {VK_KHR_SURFACE_EXTENSION_NAME, VK_KHR_WIN32_SURFACE_EXTENSION_NAME};
        instanceExtensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
        if (enableValidationLayers)
        {
            instanceExtensions.push_back(VK_EXT_DEBUG_REPORT_EXTENSION_NAME);
            instanceExtensions.push_back(VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME);
        }

        VkValidationFeatureEnableEXT enables[] = {VK_VALIDATION_FEATURE_ENABLE_BEST_PRACTICES_EXT};
        VkValidationFeaturesEXT features = {};
        features.sType = VK_STRUCTURE_TYPE_VALIDATION_FEATURES_EXT;
        features.enabledValidationFeatureCount = 1;
        features.pEnabledValidationFeatures = enables;

	    VkInstanceCreateInfo createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
        createInfo.pApplicationInfo = &appInfo;

		
        /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
        /// Extensions and Validation
        /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

		/// Get all available layers
        Layers layersInstance;
        layersInstance.layerCount = 0;
        vkEnumerateInstanceLayerProperties(&layersInstance.layerCount, nullptr);  
        layersInstance.layers.resize(layersInstance.layerCount);  
        layersInstance.activeLayers.resize(layersInstance.layerCount);  
        vkEnumerateInstanceLayerProperties(&layersInstance.layerCount, layersInstance.layers.data());

        /// Get all available extensions
        Extensions extensions;
        vkEnumerateInstanceExtensionProperties(nullptr, &extensions.extensionCount, nullptr);
        extensions.instanceExtensions.resize(extensions.extensionCount);
        extensions.activeExtensions.resize(extensions.extensionCount);
        vkEnumerateInstanceExtensionProperties(nullptr, &extensions.extensionCount, extensions.instanceExtensions.data());

        extensions = GetRequiredExtensions(extensions);
        createInfo.enabledExtensionCount = static_cast<uint32_t>(instanceExtensions.size());
        createInfo.ppEnabledExtensionNames = instanceExtensions.data();

		/// ---------------------------------------------------------

	    /// Get Vulkan API version
        VulkanChecks::CheckAPIVersion(RenderData::minVulkanVersion);

		vkEnumerateInstanceVersion(&apiVersion);

		bool khronosAvailable = false;
        for (size_t i = 0; i < layersInstance.layers.size(); i++)
        {
            layersInstance.activeLayers[i] = false;
            if (strcmp("VK_LAYER_KHRONOS_validation", layersInstance.layers[i].layerName) == 0)
            {
                layersInstance.activeLayers[i] = true;
                khronosAvailable = true;
                break;
            }
        }

		/// Get the name of all layers if they are enabled
        if (enableValidationLayers)
        {
            for (size_t i = 0; i < layersInstance.layers.size(); i++)
            {
                if (layersInstance.activeLayers[i])
                {
                    layersInstance.activeLayersNames.push_back(layersInstance.layers[i].layerName);
                }
#ifdef SEDX_DEBUG
                SEDX_CORE_TRACE_TAG("Graphics Engine","Active Layers: {} Layer Names: {}", i, layersInstance.layers[i].layerName);
#endif
            }
            if (!khronosAvailable)
                SEDX_CORE_ERROR_TAG("Graphics Engine", "Khronos validation layer not available!");
        }

	    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
        /// Instance and Surface Creation
        /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

        if (enableValidationLayers)
        {
            VkDebugUtilsMessengerCreateInfoEXT messengerInfo;
            PopulateDebugMsgCreateInfo(messengerInfo);

            SEDX_ASSERT(CreateDebugUtilsMessengerEXT(instance, &messengerInfo, nullptr, &debugMessenger) == VK_SUCCESS);
        }

        /// ---------------------------------------------------------
        if (layersInstance.validationLayer.empty())
        {
            SEDX_CORE_ERROR_TAG("Graphics Engine", "Validation layers enabled but none available!");
            return;
        }

        if (vkCreateInstance(&createInfo, nullptr, &instance) != VK_SUCCESS)
            SEDX_CORE_ERROR_TAG("Graphics Engine", "Failed to create instance!");

        SEDX_CORE_TRACE_TAG("Graphics Engine", "Vulkan Instance Created");

        VulkanLoadDebugUtilsExtensions(instance);

	    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
        /// Initalize the Vulkan Physical Device & Vulkan Device
        /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	    vkPhysicalDevice = VulkanPhysicalDevice::Select(instance);
        if (!vkPhysicalDevice)
        {
            SEDX_CORE_ERROR_TAG("Graphics Engine", "No suitable Vulkan physical device found!");
        }

	    vkDevice = CreateRef<VulkanDevice>(vkPhysicalDevice);

        /// Verify the device was created successfully before proceeding
        if (!vkDevice || vkDevice->GetDevice() == VK_NULL_HANDLE)
        {
            SEDX_CORE_ERROR_TAG("Graphics Engine", "Failed to create valid Vulkan device!");
        }

        /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
        /// Pipeline Cache Creation
        /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

        VkPipelineCacheCreateInfo pipelineCacheInfo = {};
        pipelineCacheInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_CACHE_CREATE_INFO;
        VK_CHECK_RESULT(vkCreatePipelineCache(vkDevice->GetDevice(), &pipelineCacheInfo, nullptr, &pipelineCache))
	}


} // namespace SceneryEditorX

/// -------------------------------------------------------
