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
#include <GLFW/glfw3.h>
#include <SceneryEditorX/core/application/application_data.h>
#include <SceneryEditorX/renderer/render_context.h>
#include <SceneryEditorX/renderer/vulkan/vk_checks.h>
#include <SceneryEditorX/renderer/vulkan/vk_pipeline_cache.h>
#include <SceneryEditorX/renderer/vulkan/vk_util.h>

/// -------------------------------------------------------

namespace SceneryEditorX
{

    /// -------------------------------------------------------

    LOCAL const char* validationLayer[] = {"VK_LAYER_KHRONOS_validation"};

    ///< Define whether validation layers are enabled - usually tied to debug mode
    #ifdef SEDX_DEBUG
    LOCAL constexpr bool enableValidationLayers = true;
    #else
    LOCAL constexpr bool enableValidationLayers = false;
    #endif

    /// -------------------------------------------------------

    /// Static instance of the render context
    LOCAL Ref<RenderContext> s_Instance = nullptr;

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

    RenderContext::RenderContext()
    {
        /// Initialize any member variables
        allocatorCallback = nullptr;
        instance = VK_NULL_HANDLE;
        debugMessenger = VK_NULL_HANDLE;
#ifdef SEDX_DEBUG
        debugCallback = VK_NULL_HANDLE;
#endif
    }

    RenderContext::~RenderContext()
    {
        /// Clean up debug messenger if enabled
#ifdef SEDX_DEBUG
        if (debugMessenger != VK_NULL_HANDLE && instance != VK_NULL_HANDLE)
        {
            /// Cleanup debug messenger
            DestroyDebugUtilsMessengerEXT(instance, debugMessenger, nullptr);
            debugMessenger = VK_NULL_HANDLE;
        }

        if (debugCallback != VK_NULL_HANDLE && instance != VK_NULL_HANDLE)
        {
            /// Cleanup debug callback using appropriate extension function
            if (auto func = (PFN_vkDestroyDebugReportCallbackEXT)vkGetInstanceProcAddr(instance, "vkDestroyDebugReportCallbackEXT"))
                func(instance, debugCallback, nullptr);
            debugCallback = VK_NULL_HANDLE;
        }
#endif

        /// Destroy Vulkan instance if it was created
        if (instance != VK_NULL_HANDLE)
		{
            vkDestroyInstance(instance, nullptr);
            instance = VK_NULL_HANDLE;
        }
    }

    Ref<RenderContext> RenderContext::Get()
    {
        if (!s_Instance)
            s_Instance = CreateRef<RenderContext>();

        return s_Instance;
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

    void RenderContext::Init()
	{
        try
		{
            SEDX_CORE_INFO("Initializing RenderContext");
            bool khronosAvailable = true;
    
            if (!VulkanChecks::CheckAPIVersion(RenderData::minVulkanVersion))
			{
                SEDX_CORE_ERROR_TAG("Graphics Engine", "Incompatible Vulkan driver version!");
                return;
            }
    
    	    SEDX_CORE_TRACE_TAG("Graphics Engine", "Creating Vulkan Instance");
    
    	    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
            /// Application Info
            /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    
            /// Get Vulkan API version
            VulkanChecks::CheckAPIVersion(RenderData::minVulkanVersion);
    
            AppData appData;
            RenderData renderData;
    
            uint32_t apiVersion = 0;
            if (VkResult result = vkEnumerateInstanceVersion(&apiVersion); result != VK_SUCCESS)
			{
                SEDX_CORE_ERROR("Failed to enumerate Vulkan instance version");
                apiVersion = VK_API_VERSION_1_2;  /// Fall back to 1.2
            }
    
    	    VkApplicationInfo appInfo{};
            appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
            appInfo.pNext = nullptr;
            appInfo.pApplicationName = appData.appName.c_str();
            appInfo.applicationVersion = AppData::version;
            appInfo.pEngineName = AppData::renderName.c_str();
            appInfo.engineVersion = VK_MAKE_API_VERSION(1, 0, 0, 0);
            appInfo.apiVersion = apiVersion;

    
            /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
            /// Instance Extensions and Validation Layers
            /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
            ///< Get all available layers
            uint32_t layerCount = 0;
            vkEnumerateInstanceLayerProperties(&layerCount, nullptr);
            std::vector<VkLayerProperties> layerNames(layerCount); ///< @brief Properties of all available Vulkan validation layers on the system.
            vkEnumerateInstanceLayerProperties(&layerCount, layerNames.data());
            
            ///< Check for validation layer availability
            if (enableValidationLayers)
            {
                ///< More thorough validation layer checking
                VulkanChecks layerChecker;
                
                ///< Check for all validation layers and log available ones
                std::vector<const char*> layersToCheck = { validationLayer[0] };
                layerChecker.CheckLayers(layersToCheck);
                
                ///< Specifically check for Khronos validation layer
                khronosAvailable = layerChecker.CheckValidationLayerSupport();
                
                if (!khronosAvailable)
                {
                    SEDX_CORE_ERROR_TAG("Graphics Engine", "Khronos validation layer requested but not available!");
                }
                else
                {
                    SEDX_CORE_INFO_TAG("Graphics Engine", "Validation layers are available and will be enabled");
                }
            }
    
            /// Get all available extensions
            Extensions extensions;
            vkEnumerateInstanceExtensionProperties(nullptr, &extensions.extensionCount, nullptr);
            extensions.instanceExtensions.resize(extensions.extensionCount);
    
            vkEnumerateInstanceExtensionProperties(nullptr, &extensions.extensionCount, extensions.instanceExtensions.data());
            if (enableValidationLayers)
            {
                for (const char* valLayer : validationLayer)
                {
                    uint32_t count = 0;
                    VkResult extResult = vkEnumerateInstanceExtensionProperties(valLayer, &count, nullptr);
                    if (extResult == VK_SUCCESS && count > 0)
                    {
                        const size_t layerSize = extensions.instanceExtensions.size();
                        extensions.instanceExtensions.resize(layerSize + count);
                        vkEnumerateInstanceExtensionProperties(valLayer, &count, extensions.instanceExtensions.data() + layerSize);
                        SEDX_CORE_TRACE_TAG("Graphics Engine", "Added {} extensions from validation layer {}", count, valLayer);
                    }
                }
                if (!khronosAvailable)
                    SEDX_CORE_ERROR_TAG("Graphics Engine", "Khronos validation layer not available!");
            }
    
            std::vector<const char*> instanceExtensions = {
                    VK_KHR_SURFACE_EXTENSION_NAME,
                #if defined(SEDX_PLATFORM_WINDOWS)
                    VK_KHR_WIN32_SURFACE_EXTENSION_NAME,
                #elif defined(SEDX_PLATFORM_LINUX)
                    #if defined(VK_USE_PLATFORM_WAYLAND_KHR)
                        VK_KHR_WAYLAND_SURFACE_EXTENSION_NAME,
                    #else
                        VK_KHR_XLIB_SURFACE_EXTENSION_NAME,
                    #endif
                #elif defined(SEDX_PLATFORM_APPLE)
                    VK_EXT_LAYER_SETTINGS_EXTENSION_NAME, VK_MVK_MACOS_SURFACE_EXTENSION_NAME,
                #endif
            };
    
            VulkanChecks check;
            if (check.CheckExtension(VK_EXT_DEBUG_UTILS_EXTENSION_NAME, extensions.instanceExtensions))
                instanceExtensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
    
            if (enableValidationLayers)
            {
                instanceExtensions.push_back(VK_EXT_VALIDATION_FEATURES_EXTENSION_NAME);
                //instanceExtensions.push_back(VK_EXT_DEBUG_REPORT_EXTENSION_NAME);
                //instanceExtensions.push_back(VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME);
            }
    
    		if (check.CheckExtension(VK_EXT_SURFACE_MAINTENANCE_1_EXTENSION_NAME, extensions.instanceExtensions))
                instanceExtensions.push_back(VK_EXT_SURFACE_MAINTENANCE_1_EXTENSION_NAME);
    
    		if (check.CheckExtension(VK_KHR_GET_SURFACE_CAPABILITIES_2_EXTENSION_NAME, extensions.instanceExtensions))
                instanceExtensions.push_back(VK_KHR_GET_SURFACE_CAPABILITIES_2_EXTENSION_NAME);
    
    		for (const auto&[extensionName, specVersion] : extensions.availableExtensions)
                if (extensionName[0] != '\0')
                    instanceExtensions.push_back(extensionName);

        #ifdef SEDX_PLATFORM_APPLE
    		/// Shader validation doesn't work in MoltenVK for SPIR-V 1.6 under Vulkan 1.3:
    		/// "Invalid SPIR-V binary version 1.6 for target environment SPIR-V 1.5 (under Vulkan 1.2 semantics)."
            const VkValidationFeatureDisableEXT validationFeaturesDisabled[] = {
    			    VK_VALIDATION_FEATURE_DISABLE_SHADERS_EXT,
    			    VK_VALIDATION_FEATURE_DISABLE_SHADER_VALIDATION_CACHE_EXT,
            };
        #endif
    
            VkValidationFeatureEnableEXT enables[] = {VK_VALIDATION_FEATURE_ENABLE_BEST_PRACTICES_EXT};
    
            // ReSharper disable once CppVariableCanBeMadeConstexpr
            const VkValidationFeaturesEXT features = {
                .sType = VK_STRUCTURE_TYPE_VALIDATION_FEATURES_EXT,
                .pNext = nullptr,
            #if defined(SEDX_PLATFORM_APPLE)
                .disabledValidationFeatureCount = enableValidationLayers ? (uint32_t)SEDX_NUM_ARRAY_ELEMENTS(validationFeaturesDisabled) : 0u,
                .pDisabledValidationFeatures = enableValidationLayers ? validationFeaturesDisabled : nullptr,
            #endif
            };
    
            /// ---------------------------------------------------------
    
    		#if defined(VK_EXT_layer_settings) && VK_EXT_layer_settings
    				/// https://github.com/KhronosGroup/MoltenVK/blob/main/Docs/MoltenVK_Configuration_Parameters.md
                    constexpr int useMetalArgumentBuffers = 1;
                    constexpr VkBool32 gpuav_descriptor_checks = VK_FALSE;					/// https://github.com/KhronosGroup/Vulkan-ValidationLayers/issues/8688
                    constexpr VkBool32 gpuav_indirect_draws_buffers = VK_FALSE;				/// https://github.com/KhronosGroup/Vulkan-ValidationLayers/issues/8579
                    constexpr VkBool32 gpuav_post_process_descriptor_indexing = VK_FALSE;	/// https://github.com/KhronosGroup/Vulkan-ValidationLayers/issues/9222
    		
    		    #define LAYER_SETTINGS_BOOL32(name, var)											\
    				VkLayerSettingEXT                                                               \
    				{                                                                               \
    				    .pLayerName = validationLayer[0],											\
    		            .pSettingName = name, .type = VK_LAYER_SETTING_TYPE_BOOL32_EXT,				\
    				    .valueCount = 1, .pValues = var,                                            \
    				}
    		
    				const VkLayerSettingEXT settings[] = {
    				    LAYER_SETTINGS_BOOL32("gpuav_descriptor_checks", &gpuav_descriptor_checks),
    				    LAYER_SETTINGS_BOOL32("gpuav_indirect_draws_buffers", &gpuav_indirect_draws_buffers),
    				    LAYER_SETTINGS_BOOL32("gpuav_post_process_descriptor_indexing", &gpuav_post_process_descriptor_indexing),
    				    {"MoltenVK",
    				     "MVK_CONFIG_USE_METAL_ARGUMENT_BUFFERS",
    				     VK_LAYER_SETTING_TYPE_INT32_EXT,
    				     1,
    				     &useMetalArgumentBuffers},
    				};
    		    #undef LAYER_SETTINGS_BOOL32
    		
    				VkLayerSettingsCreateInfoEXT layerSettingsCreateInfo;
                    layerSettingsCreateInfo.sType = VK_STRUCTURE_TYPE_LAYER_SETTINGS_CREATE_INFO_EXT;
                    layerSettingsCreateInfo.pNext = enableValidationLayers ? &features : nullptr;
                    layerSettingsCreateInfo.settingCount = (uint32_t)std::size(settings);
                    layerSettingsCreateInfo.pSettings = settings;
    		#endif
    
    	    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
            /// Instance and Surface Creation
            /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    
    		VkInstanceCreateFlags createFlags = 0;
    	    VkInstanceCreateInfo createInfo = {};
            createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
        #if defined(VK_EXT_layer_settings) && VK_EXT_layer_settings
            createInfo.pNext = &layerSettingsCreateInfo,
        #else
            createInfo.pNext = enableValidationLayers ? &features : nullptr;
        #endif
            createInfo.pApplicationInfo = &appInfo;
            createInfo.enabledLayerCount = enableValidationLayers ? (uint32_t)std::size(validationLayer) : 0u;
            createInfo.ppEnabledLayerNames = enableValidationLayers ? validationLayer : nullptr;
            createInfo.flags = createFlags;
            createInfo.enabledExtensionCount = static_cast<uint32_t>(instanceExtensions.size());
            createInfo.ppEnabledExtensionNames = instanceExtensions.data();

            if (VkResult createResult = vkCreateInstance(&createInfo, nullptr, &instance); createResult != VK_SUCCESS)
            {
                SEDX_CORE_ERROR_TAG("Graphics Engine", "Failed to create instance! Error code: {}", static_cast<int>(createResult));
                return;
            }
    
            /// ---------------------------------------------------------
    
            if (enableValidationLayers)
            {
                VkDebugUtilsMessengerCreateInfoEXT messengerInfo;
                PopulateDebugMsgCreateInfo(messengerInfo);
                if (CreateDebugUtilsMessengerEXT(instance, &messengerInfo, nullptr, &debugMessenger) != VK_SUCCESS)
                    SEDX_CORE_ERROR_TAG("Graphics Engine", "Failed to set up debug messenger!");
                else
                    SEDX_CORE_INFO_TAG("Graphics Engine", "Debug messenger set up successfully");
            }
    
            SEDX_CORE_TRACE_TAG("Graphics Engine", "Vulkan Instance Created");
    
            #ifdef SEDX_DEBUG
                VulkanLoadDebugUtilsExtensions(instance);
            #endif
    
    	    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
            /// Initialize the Vulkan Physical Device & Vulkan Device
            /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    	    vkPhysicalDevice = VulkanPhysicalDevice::Select(instance);
            if (!vkPhysicalDevice)
			{
                SEDX_CORE_ERROR_TAG("Graphics Engine", "No suitable Vulkan physical device found!");
                return;
            }
    
            vkDevice = CreateRef<VulkanDevice>(vkPhysicalDevice);
    
            /// Verify the device was created successfully before proceeding
            if (!vkDevice || vkDevice->GetDevice() == VK_NULL_HANDLE)
			{
                SEDX_CORE_ERROR_TAG("Graphics Engine", "Failed to create valid Vulkan device!");
                return;
            }
    
            SEDX_CORE_INFO("Vulkan device created successfully");
    
            /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
            /// Pipeline Cache Creation
            /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

            PipelineCache pipelineCache;
            pipelineCache.CreateCache();

            SEDX_CORE_INFO("Pipeline cache created successfully");
            SEDX_CORE_INFO("RenderContext initialization complete");
        }
        catch (const std::exception& e)
		{
            SEDX_CORE_ERROR("Exception during RenderContext initialization: {}", e.what());
        }
        catch (...)
		{
            SEDX_CORE_ERROR("Unknown exception during RenderContext initialization");
        }
	}

    /// -------------------------------------------------------


    
    VkInstance RenderContext::GetInstance()
    {
        if (!s_Instance)
		{
            SEDX_CORE_WARN("Attempting to get Vulkan instance before RenderContext is initialized");
            return VK_NULL_HANDLE;
        }
        return s_Instance->instance;
    }

}

/// -------------------------------------------------------
