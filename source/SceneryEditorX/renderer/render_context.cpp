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
#include <SceneryEditorX/core/application_data.h>
#include <SceneryEditorX/renderer/render_context.h>
#include <SceneryEditorX/renderer/renderer.h>
#include <SceneryEditorX/renderer/vulkan/vk_checks.h>
#include <SceneryEditorX/renderer/vulkan/vk_util.h>

/// -------------------------------------------------------

namespace SceneryEditorX
{

    /// -------------------------------------------------------

    LOCAL const char* validationLayer[] = {"VK_LAYER_KHRONOS_validation"};

    /// -------------------------------------------------------

	RenderContext::RenderContext() = default;

    RenderContext::~RenderContext()
    {
        /// Device is destroyed in GLFWWindow::Shutdown()
        vkDestroyInstance(instance, nullptr);
        instance = VK_NULL_HANDLE;
    }

    // Implementation of the Get() method that was moved from header to break circular dependency
    Ref<RenderContext> RenderContext::Get()
    {
        return Renderer::GetContext().DynamicCast<RenderContext>();
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

    void RenderContext::Init()
	{
        bool khronosAvailable = false;

        if (!VulkanChecks::CheckAPIVersion(RenderData::minVulkanVersion))
            SEDX_CORE_ERROR_TAG("Graphics Engine", "Incompatible Vulkan driver version!");

	    SEDX_CORE_TRACE_TAG("Graphics Engine", "Creating Vulkan Instance");

	    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
        /// Application Info
        /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

        /// Get Vulkan API version
        VulkanChecks::CheckAPIVersion(RenderData::minVulkanVersion);

        AppData appData;
        RenderData renderData;

        uint32_t apiVersion = 0;
        vkEnumerateInstanceVersion(&apiVersion);

	    VkApplicationInfo appInfo{};
        appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
        appInfo.pNext = nullptr;
        appInfo.pApplicationName = appData.appName.c_str();
        appInfo.applicationVersion = AppData::version;
        appInfo.pEngineName = AppData::renderName.c_str();
        appInfo.engineVersion = VK_MAKE_API_VERSION(1, 0, 0, 0);
        appInfo.apiVersion = apiVersion;

		/// ---------------------------------------------------------

        /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
        /// Instance Extensions and Validation Layers
        /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
        /// Get all available layers
        uint32_t layerCount = 0;
        vkEnumerateInstanceLayerProperties(&layerCount, nullptr);
        std::vector<VkLayerProperties> layerNames(layerCount); /// @brief Properties of all available Vulkan validation layers on the system.
        vkEnumerateInstanceLayerProperties(&layerCount, layerNames.data());
        [this, &layerNames, &khronosAvailable]() -> void
        {
            for (const VkLayerProperties &props : layerNames)
            {
                for (const char *layer : validationLayer)
                    if (!strcmp(props.layerName, layer))
                        return;
            }
            khronosAvailable = false;
        }();

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
				SEDX_ASSERT(vkEnumerateInstanceExtensionProperties(valLayer, &count, nullptr));
                if (count > 0)
                {
                    const size_t layerSize = extensions.instanceExtensions.size();
                    extensions.instanceExtensions.resize(layerSize + count);
                    SEDX_ASSERT(vkEnumerateInstanceExtensionProperties(valLayer, &count, extensions.instanceExtensions.data() + layerSize));
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

		for (const VkExtensionProperties& extension : extensions.availableExtensions)
		{
            if (extension.extensionName[0] != '\0')
                instanceExtensions.push_back(extension.extensionName);
        }

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
				// https://github.com/KhronosGroup/MoltenVK/blob/main/Docs/MoltenVK_Configuration_Parameters.md
				const int useMetalArgumentBuffers = 1;
				const VkBool32 gpuav_descriptor_checks = VK_FALSE; // https://github.com/KhronosGroup/Vulkan-ValidationLayers/issues/8688
				const VkBool32 gpuav_indirect_draws_buffers = VK_FALSE; // https://github.com/KhronosGroup/Vulkan-ValidationLayers/issues/8579
				const VkBool32 gpuav_post_process_descriptor_indexing = VK_FALSE; // https://github.com/KhronosGroup/Vulkan-ValidationLayers/issues/9222
		
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
        .pNext = enableValidationLayers ? &features : nullptr,
    #endif
        createInfo.pApplicationInfo = &appInfo;
        createInfo.enabledLayerCount = enableValidationLayers ? (uint32_t)std::size(validationLayer) : 0u;
        createInfo.flags = createFlags;
        createInfo.enabledExtensionCount = static_cast<uint32_t>(instanceExtensions.size());
        createInfo.ppEnabledExtensionNames = instanceExtensions.data();
        if (vkCreateInstance(&createInfo, nullptr, &instance) != VK_SUCCESS)
            SEDX_CORE_ERROR_TAG("Graphics Engine", "Failed to create instance!");

        /// ---------------------------------------------------------

        if (enableValidationLayers)
        {
            VkDebugUtilsMessengerCreateInfoEXT messengerInfo;
            PopulateDebugMsgCreateInfo(messengerInfo);
            SEDX_ASSERT(CreateDebugUtilsMessengerEXT(instance, &messengerInfo, nullptr, &debugMessenger) == VK_SUCCESS);
        }

        if (validationLayer[0])
        {
            SEDX_CORE_ERROR_TAG("Graphics Engine", "Validation layers enabled but none available!");
            return;
        }

        SEDX_CORE_TRACE_TAG("Graphics Engine", "Vulkan Instance Created");

        #ifdef SEDX_DEBUG
            VulkanLoadDebugUtilsExtensions(instance);
        #endif

	    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
        /// Initalize the Vulkan Physical Device & Vulkan Device
        /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	    vkPhysicalDevice = VulkanPhysicalDevice::Select(instance);
        if (!vkPhysicalDevice)
            SEDX_CORE_ERROR_TAG("Graphics Engine", "No suitable Vulkan physical device found!");

        vkDevice = CreateRef<VulkanDevice>(vkPhysicalDevice);

        /// Verify the device was created successfully before proceeding
        if (!vkDevice || vkDevice->GetDevice() == VK_NULL_HANDLE)
            SEDX_CORE_ERROR_TAG("Graphics Engine", "Failed to create valid Vulkan device!");


        /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
        /// Pipeline Cache Creation
        /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

        VkPipelineCacheCreateInfo pipelineCacheInfo = {};
        pipelineCacheInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_CACHE_CREATE_INFO;
        pipelineCacheInfo.pNext = nullptr;
        pipelineCacheInfo.flags = static_cast<VkPipelineCacheCreateFlags>(0);
        pipelineCacheInfo.initialDataSize = pipelineCacheDataSize;
        pipelineCacheInfo.pInitialData = pipelineCacheData;
        VK_CHECK_RESULT(vkCreatePipelineCache(vkDevice->GetDevice(), &pipelineCacheInfo, nullptr, &pipelineCache))
	}

    /// -------------------------------------------------------

    std::vector<uint8_t> RenderContext::GetPipelineCacheData() const
    {
        size_t size = 0;
        vkGetPipelineCacheData(vkDevice->GetDevice(), pipelineCache, &size, nullptr);
		std::vector<uint8_t> data(size);
        if (size)
            vkGetPipelineCacheData(vkDevice->GetDevice(), pipelineCache, &size, data.data());

        return data;
    }

} // namespace SceneryEditorX

/// -------------------------------------------------------
