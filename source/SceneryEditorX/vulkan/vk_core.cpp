/**
* -------------------------------------------------------
* Scenery Editor X
* -------------------------------------------------------
* Copyright (c) 2025 Thomas Ray 
* Copyright (c) 2025 Coalition of Freeware Developers
* -------------------------------------------------------
* vk_core.cpp
* -------------------------------------------------------
* Created: 21/3/2025
* -------------------------------------------------------
*/
#define TINYOBJLOADER_IMPLEMENTATION
#include <imgui_impl_glfw.h>
#include <imgui_impl_vulkan.h>
#include <SceneryEditorX/core/application_data.h>
#include <SceneryEditorX/core/window.h>
#include <SceneryEditorX/platform/editor_config.hpp>
#include <SceneryEditorX/platform/file_manager.hpp>
#include <SceneryEditorX/ui/ui.h>
#include <SceneryEditorX/ui/ui_manager.h>
#include <SceneryEditorX/vulkan/render_data.h>
#include <SceneryEditorX/vulkan/vk_buffers.h>
#include <SceneryEditorX/vulkan/vk_checks.h>
#include <SceneryEditorX/vulkan/vk_cmd_buffers.h>
#include <SceneryEditorX/vulkan/vk_core.h>
#include <SceneryEditorX/vulkan/vk_util.h>
#include <stb_image.h>
#include <tiny_obj_loader.h>

// -------------------------------------------------------

namespace SceneryEditorX
{

    // -------------------------------------------------------

	PFN_vkSetDebugUtilsObjectNameEXT vkSetDebugUtilsObjectNameEXT;
	PFN_vkGetAccelerationStructureBuildSizesKHR vkGetAccelerationStructureBuildSizesKHR;
	PFN_vkCreateAccelerationStructureKHR vkCreateAccelerationStructureKHR;
	PFN_vkGetBufferDeviceAddressKHR vkGetBufferDeviceAddressKHR;
	PFN_vkCmdBuildAccelerationStructuresKHR vkCmdBuildAccelerationStructuresKHR;
	PFN_vkGetAccelerationStructureDeviceAddressKHR vkGetAccelerationStructureDeviceAddressKHR;
	PFN_vkDestroyAccelerationStructureKHR vkDestroyAccelerationStructureKHR;

    INTERNAL std::string VK_DEBUG_SEVERITY(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity)
    {
        if (messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT)
            return "VERBOSE";
        if (messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT)
            return "INFO";
        if (messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT)
            return "WARNING";
        if (messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT)
            return "ERROR";
        return "UNKNOWN";
    }

    INTERNAL std::string VK_DEBUG_TYPE(VkFlags messageType)
    {
        if (messageType & VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT)
            return "GENERAL";
        if (messageType & VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT)
            return "VALIDATION";
        if (messageType & VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT)
            return "PERFORMANCE";
        return "UNKNOWN";
    }

    /*
    static VKAPI_ATTR VkBool32 VKAPI_CALL VulkanDebugMsgCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity, VkDebugUtilsMessageTypeFlagsEXT messageType, const VkDebugUtilsMessengerCallbackDataEXT *pCallbackData, void *pUserData)
    {
        (void)pUserData; //Unused argument

        std::string labels, objects;
        if (constexpr bool performanceWarnings = false; !performanceWarnings)
        {
            if (messageType & VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT)
                return VK_FALSE;
        }

        if (pCallbackData->cmdBufLabelCount)
        {
            labels = std::format("\tLabels({}): \n", pCallbackData->cmdBufLabelCount);
            for (uint32_t i = 0; i < pCallbackData->cmdBufLabelCount; ++i)
            {
                const auto &label = pCallbackData->pCmdBufLabels[i];
                const std::string colorStr = std::format("[ {}, {}, {}, {} ]", label.color[0], label.color[1], label.color[2], label.color[3]);
                labels.append(std::format("\t\t- Command Buffer Label[{0}]: name: {1}, color: {2}\n",i, label.pLabelName ? label.pLabelName : "NULL", colorStr));
            }
        }

        /*
        /// Also log any objects that were involved in the message
        if (pCallbackData->objectCount > 0)
        {
            for (uint32_t i = 0; i < pCallbackData->objectCount; i++)
            {
                const auto &obj = pCallbackData->pObjects[i];
                std::string objInfo = "   Object[" + std::to_string(i) + "] - Type: ";
                objInfo += std::to_string(obj.objectType) + ", Handle: " + std::to_string((uint64_t)obj.objectHandle);

                if (obj.pObjectName)
                {
                    objInfo += ", Name: \"" + std::string(obj.pObjectName) + "\"";
                }

                Log::LogVulkanDebug(objInfo);
            }
        }
        #1#

        if (pCallbackData->objectCount)
        {
            objects = std::format("\tObjects({}): \n", pCallbackData->objectCount);
            for (uint32_t i = 0; i < pCallbackData->objectCount; ++i)
            {
                const auto &object = pCallbackData->pObjects[i];
                objects.append(std::format("\t\t- Object[{0}] name: {1}, type: {2}, handle: {3:#x}\n",i, object.pObjectName ? object.pObjectName : "NULL", VkObjectTypeToString(object.objectType), object.objectHandle));
            }
        }

		SEDX_CORE_WARN("{0} {1} message: \n\t{2}\n {3} {4}", VK_DEBUG_TYPE(messageType), VK_DEBUG_SEVERITY(messageSeverity), pCallbackData->pMessage, labels, objects);
        return VK_FALSE; // Don't abort call
    }
    */

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
        // search for the requested function and return null if unable find
        auto func = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT");
        if (func != nullptr)
            func(instance, debugMessenger, pAllocator);
    }

    LOCAL VKAPI_ATTR VkBool32 VKAPI_CALL VulkanDebugMsgCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity, VkDebugUtilsMessageTypeFlagsEXT messageType, const VkDebugUtilsMessengerCallbackDataEXT *pCallbackData, void *pUserData)
    {
        // log the message
        // here we can set a minimum severity to log the message
        // if (messageSeverity > VK_DEBUG_UTILS...)
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

    /*
    VkResult GraphicsEngine::CreateDebugUtilsMessengerEXT( VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT *pCreateInfo, const VkAllocationCallbacks *pAllocator, VkDebugUtilsMessengerEXT*pDebugMessenger)
    {
        auto func = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT");
        if (func != nullptr)
        {
            return func(instance, pCreateInfo, pAllocator, pDebugMessenger);
        }
        else
        {
            return VK_ERROR_EXTENSION_NOT_PRESENT;
        }
    }
    */

    // -------------------------------------------------------

    /*
    void GraphicsEngine::DestroyDebugUtilsMessengerEXT(VkInstance instance, VkDebugUtilsMessengerEXT debugMessenger, const VkAllocationCallbacks *pAllocator)
    {
        auto func = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT");
        if (func != nullptr)
        {
            func(instance, debugMessenger, pAllocator);
        }
    }
	*/

    void GraphicsEngine::glfwSetWindowUserPointer(const Ref<Window> &window, GLFWwindow *pointer)
    {
        /// Set the GLFWwindow user pointer to point to our Window instance
        ::glfwSetWindowUserPointer(pointer, window.get());
    }

    // -------------------------------------------------------

	GraphicsEngine::GraphicsEngine() = default;

    GraphicsEngine::~GraphicsEngine()
    {
        if (debugMessenger != VK_NULL_HANDLE)
        {
            DestroyDebugUtilsMessengerEXT(vkInstance, debugMessenger, allocator);
            SEDX_CORE_TRACE_TAG("Graphics Engine", "Destroyed vulkan debugger.");
        }
        if (vkInstance != VK_NULL_HANDLE)
        {
            vkDestroyInstance(vkInstance, allocator);
            SEDX_CORE_TRACE_TAG("Graphics Engine", "Destroyed Graphics Engine instance destroyed.");
        }

        vkInstance = nullptr;
    }

    // -------------------------------------------------------

    void GraphicsEngine::Init(const Ref<Window> &window)
    {
        /// Store the window reference
        editorWindow = window;

        if (!checks->CheckAPIVersion(SoftwareStats::minVulkanVersion))
            SEDX_CORE_ERROR_TAG("Graphics Engine", "Incompatible Vulkan driver version!");

        CreateInstance(window);

		/// Use width and height from WindowData
        renderData.width = WindowData::width;
        renderData.height = WindowData::height;

        SEDX_CORE_INFO("Initializing graphics engine with window size: {}x{}", renderData.width, renderData.height);
    }

    static Extensions GetRequiredExtensions(Extensions vkExtensions)
    {
        uint32_t glfwExtensionCount = 0;
        const char** glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);
        vkExtensions.requiredExtensions = std::vector(glfwExtensions, glfwExtensions + glfwExtensionCount);
        if (enableValidationLayers)
            vkExtensions.requiredExtensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);

        return vkExtensions;
    }

    // -------------------------------------------------------

    void GraphicsEngine::CreateInstance(const Ref<Window> &window)
    {
        SEDX_CORE_TRACE_TAG("Graphics Engine", "Creating Vulkan Instance");

        /// Set the window user pointer to point to the Window singleton
        glfwSetWindowUserPointer(window, Window::GetWindow());

        /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
        /// Extensions and Validation
        /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

		/// Get all available layers
        uint32_t layerCount = 0;
        vkEnumerateInstanceLayerProperties(&layerCount, nullptr);
        vkLayers.layers.resize(layerCount);
        vkLayers.activeLayers.resize(layerCount);
        vkEnumerateInstanceLayerProperties(&layerCount, vkLayers.layers.data());

        /// Get all available extensions
        uint32_t extensionCount = 0;
        vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, nullptr);
        vkExtensions.instanceExtensions.resize(extensionCount);
        vkExtensions.activeExtensions.resize(extensionCount);
        vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, vkExtensions.instanceExtensions.data());

        /// Get Vulkan API version
        vkEnumerateInstanceVersion(&renderData.apiVersion);
        SEDX_CORE_TRACE_TAG("Graphics Engine", "Vulkan Instance API Version: {}", renderData.apiVersion);

		bool khronosAvailable = false;
        for (size_t i = 0; i < vkLayers.layers.size(); i++)
        {
            vkLayers.activeLayers[i] = false;
            if (strcmp("VK_LAYER_KHRONOS_validation", vkLayers.layers[i].layerName) == 0)
            {
                vkLayers.activeLayers[i] = true;
                khronosAvailable = true;
                break;
            }
        }

        if (enableValidationLayers && !khronosAvailable)
            SEDX_CORE_ERROR_TAG("Graphics Engine", "Khronos validation layer not available!");
        

		/// Get the name of all layers if they are enabled
        if (enableValidationLayers)
        {
            for (size_t i = 0; i < vkLayers.layers.size(); i++)
            {
                if (vkLayers.activeLayers[i])
                {
                    vkLayers.activeLayersNames.push_back(vkLayers.layers[i].layerName);
                }
#ifdef SEDX_DEBUG
                SEDX_CORE_TRACE_TAG("Graphics Engine","Active Layers: {} Layer Names: {}", i, vkLayers.layers[i].layerName);
#endif
            }
        }

        /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
        /// Application Info
        /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	    VkApplicationInfo appInfo{};
        appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
        appInfo.pApplicationName = SoftwareStats::appName.c_str();
        appInfo.applicationVersion = SoftwareStats::version;
        appInfo.pEngineName = SoftwareStats::renderName.c_str();
        appInfo.engineVersion = VK_MAKE_API_VERSION(1, 0, 0, 0);
        appInfo.apiVersion = SoftwareStats::maxVulkanVersion;

        VkInstanceCreateInfo createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
        createInfo.pApplicationInfo = &appInfo;

		// ---------------------------------------------------------


        std::vector<const char *> instanceExtensions = {VK_KHR_SURFACE_EXTENSION_NAME, VK_KHR_WIN32_SURFACE_EXTENSION_NAME};
        instanceExtensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
        if (enableValidationLayers)
        {
            instanceExtensions.push_back(VK_EXT_DEBUG_REPORT_EXTENSION_NAME);
            instanceExtensions.push_back(VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME);
        }

        auto extensions = GetRequiredExtensions(vkExtensions);
        createInfo.enabledExtensionCount = static_cast<uint32_t>(instanceExtensions.size());
        createInfo.ppEnabledExtensionNames = instanceExtensions.data();


		// ---------------------------------------------------------

        /*
        vkExtensions.requiredExtensions.clear();
        for (size_t i = 0; i < vkExtensions.instanceExtensions.size(); i++)
        {
            if (vkExtensions.activeExtensions[i])
            {
                vkExtensions.requiredExtensions.push_back(vkExtensions.instanceExtensions[i].extensionName);
            }
        #ifdef SEDX_DEBUG
            SEDX_CORE_TRACE_TAG("Graphics Engine","Active Extensions: {} Active Extensions Names: {}", i, vkExtensions.instanceExtensions[i].extensionName);
        #endif
        }

        /// Get and set all required extensions
        createInfo.enabledExtensionCount = static_cast<uint32_t>(vkExtensions.requiredExtensions.size());
        createInfo.ppEnabledExtensionNames = vkExtensions.requiredExtensions.data();
        */

		// ---------------------------------------------------------

        VkDebugUtilsMessengerCreateInfoEXT vkDebugCreateInfo;
        if (enableValidationLayers)
        {
            createInfo.enabledLayerCount = static_cast<uint32_t>(vkLayers.validationLayer.size());
            createInfo.ppEnabledLayerNames = vkLayers.validationLayer.data();
            /// We need to set up a separate logger just for the instance creation/destruction because our "default" logger is created after
            PopulateDebugMsgCreateInfo(vkDebugCreateInfo);
            // createInfo.pNext = (VkDebugUtilsMessengerCreateInfoEXT*)&debugCreateInfo;
        }
        else
        {
            createInfo.enabledLayerCount = 0;
            createInfo.pNext = nullptr;
        }

		/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		/// Instance and Surface Creation
		/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

        if (vkLayers.validationLayer.empty() && enableValidationLayers)
        {
            SEDX_CORE_ERROR_TAG("Graphics Engine", "Validation layers enabled but none available!");
            return;
        }

        if (vkCreateInstance(&createInfo, allocator, &vkInstance) != VK_SUCCESS)
            SEDX_CORE_ERROR_TAG("Graphics Engine", "Failed to create instance!");

        SEDX_CORE_TRACE_TAG("Graphics Engine", "Vulkan Instance Created");

        VulkanLoadDebugUtilsExtensions(vkInstance);

		// ---------------------------------------------------------

		if (enableValidationLayers)
        {
            VkDebugUtilsMessengerCreateInfoEXT messengerInfo;
            PopulateDebugMsgCreateInfo(messengerInfo);

			SEDX_ASSERT(CreateDebugUtilsMessengerEXT(vkInstance, &messengerInfo, allocator, &debugMessenger) == VK_SUCCESS);

        }

		// ---------------------------------------------------------

		//TODO: Move this to the swapchain creation
		if (glfwCreateWindowSurface(vkInstance, Window::GetWindow(), allocator, &surface) != VK_SUCCESS)
            SEDX_CORE_ERROR_TAG("Graphics Engine", "Failed to create window surface!");

        /*
		if (enableValidationLayers)
        {
			auto vkCreateDebugUtilsMessengerEXT = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(vkInstance, "vkCreateDebugUtilsMessengerEXT");
            //SEDX_ASSERT(vkCreateDebugUtilsMessengerEXT != nullptr, "");

            VkDebugUtilsMessengerCreateInfoEXT createInfo{};
            PopulateDebugMsgCreateInfo(createInfo);
            if (CreateDebugUtilsMessengerEXT(vkInstance, &createInfo, allocator, &debugMessenger) != VK_SUCCESS)
            {
                SEDX_CORE_ERROR_TAG("Graphics Engine","Failed to set up debug messenger!");
                //ErrMsg("Failed to set up debug messenger!");
            }

			SEDX_CORE_TRACE_TAG("Graphics Engine","Vulkan Debug messenger initialized.");
        }
        */

        /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
        /// Initalize the Vulkan Physical Device & Vulkan Device
        /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

        vkPhysicalDevice = VulkanPhysicalDevice::Select();

        /*
        try
        {
            physDevice = VulkanPhysicalDevice::GetInstance();
        }
        catch (const std::exception &e)
        {
            SEDX_CORE_ERROR_TAG("Graphics Engine", "Failed to create physical device: {}", e.what());
            return;
        }

        physDevice->SelectDevice(VK_QUEUE_GRAPHICS_BIT, true);
        */

        VulkanDeviceFeatures deviceFeatures;
        deviceFeatures.GetPhysicalDeviceFeatures();

        vkDevice = CreateRef<VulkanDevice>(vkPhysicalDevice, deviceFeatures.GetPhysicalDeviceFeatures());

		/// Memory Allocator initialization.
        MemoryAllocator::Init(vkDevice);

        //allocatorManager->Init(vkDevice->GetDevice(), vkPhysDevice, vkInstance);

		/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
        /// Pipeline Cache Creation
        /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

		VkPipelineCacheCreateInfo pipelineCacheInfo{};
        pipelineCacheInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_CACHE_CREATE_INFO;
        pipelineCacheInfo.initialDataSize = 0;
        pipelineCacheInfo.pInitialData = nullptr;
        pipelineCacheInfo.flags = 0;
        if (vkCreatePipelineCache(vkDevice->GetDevice(), &pipelineCacheInfo, allocator, &pipelineCache) != VK_SUCCESS)
            SEDX_CORE_ERROR_TAG("Graphics Engine", "Failed to create pipeline cache!");

        /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
        /// Surface Creation
        /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

		if (glfwCreateWindowSurface(vkInstance, Window::GetWindow(), allocator, &surface) != VK_SUCCESS)
            SEDX_CORE_ERROR_TAG("Graphics Engine", "Failed to create window surface!");

        SEDX_CORE_TRACE_TAG("Graphics Engine", "Vulkan Instance Created");
    }

    void GraphicsEngine::WaitIdle(const Ref<VulkanDevice> &device)
    {
        vkDeviceWaitIdle(device->GetDevice());
    }

    /// -------------------------------------------------------

    void GraphicsEngine::CleanUp() 
    {
        UI::GUI guiInstance;
        guiInstance.CleanUp();

        CleanupSwapChain();

        vkDestroyDescriptorPool(vkDevice->GetDevice(), descriptorPool, allocator);

        vkDestroySampler(vkDevice->GetDevice(), textureSampler, allocator);

        for (auto imageView : swapChainImageViews)
        {
            vkDestroyImageView(vkDevice->GetDevice(), imageView, allocator);
        }

        vkDestroyDescriptorSetLayout(vkDevice->GetDevice(), descriptorSetLayout, allocator);

		for (size_t i = 0; i < RenderData::framesInFlight; i++)
        {
            vkDestroySemaphore(vkDevice->GetDevice(), renderFinishedSemaphores[i], allocator);
            vkDestroySemaphore(vkDevice->GetDevice(), imageAvailableSemaphores[i], allocator);
            vkDestroyFence(vkDevice->GetDevice(), inFlightFences[i], allocator);
        }

		vkSwapChain->Destroy();

        //vkDestroySwapchainKHR(vkDevice->GetDevice(), vkSwapChain, allocator);
        vkDestroyDevice(vkDevice->GetDevice(), allocator);

        if (enableValidationLayers)
        {
            DestroyDebugUtilsMessengerEXT(vkInstance, debugMessenger, allocator);
        }

        vkDestroySurfaceKHR(vkInstance, surface, allocator);
        vkDestroyInstance(vkInstance, allocator);
    }

	void GraphicsEngine::CleanupSwapChain()
    {
        vkDestroyImageView(vkDevice->GetDevice(), depthImageView, allocator);
        vkDestroyImage(vkDevice->GetDevice(), depthImage, allocator);
        vkFreeMemory(vkDevice->GetDevice(), depthImageMemory, allocator);

        for (auto framebuffer : swapChainFramebuffers)
            vkDestroyFramebuffer(vkDevice->GetDevice(), framebuffer, allocator);

        for (size_t i = 0; i < vkSwapChain->swapChainImages.size(); i++)
            vkDestroyImageView(vkDevice->GetDevice(), swapChainImageViews[i], allocator);

        vkDestroyPipeline(vkDevice->GetDevice(), graphicsPipeline, allocator);
        vkDestroyPipelineLayout(vkDevice->GetDevice(), pipelineLayout, allocator);
        vkDestroyRenderPass(vkDevice->GetDevice(), renderPass, allocator);

        for (size_t i = 0; i < renderData.currentFrame; i++)
        {
            vkDestroySemaphore(vkDevice->GetDevice(), imageAvailableSemaphores[i], allocator);
            vkDestroySemaphore(vkDevice->GetDevice(), renderFinishedSemaphores[i], allocator);
        }

		vkSwapChain->Destroy();
        //vkDestroySwapchainKHR(vkDevice->GetDevice(), vkSwapChain, allocator);


        //for (int q = 0; q < Queue::Count; q++)
        //{
        //    for (int i = 0; i < currentFrame; i++)
        //    {
        //        vkDestroyCommandPool(vkDevice->GetDevice(), queues[q].commands[i].pool, allocator);
        //        queues[q].commands[i].staging = {};
        //        queues[q].commands[i].stagingCpu = nullptr;
        //        vkDestroyFence(vkDevice->GetDevice(), queues[q].commands[i].fence, allocator);
        //        vkDestroyQueryPool(vkDevice->GetDevice(), queues[q].commands[i].queryPool, allocator);
        //    }
        //}


        imageAvailableSemaphores.clear();
        renderFinishedSemaphores.clear();
        swapChainImageViews.clear();
        vkSwapChain->swapChainImages.clear();
        //vkSwapChain = VK_NULL_HANDLE;
	}

    // -------------------------------------------------------

    /*
    void GraphicsEngine::RenderFrame()
    {
        UI::GUI guiInstance;

        vkWaitForFences(vkDevice->GetDevice(), 1, &inFlightFences[renderData.currentFrame], VK_TRUE, UINT64_MAX);

        // -------------------------------------------------------

        uint32_t imageIndex;
        VkResult result = vkAcquireNextImageKHR(vkDevice->GetDevice(),
                                                vkSwapChain->swapChain,
                                                UINT64_MAX,
                                                imageAvailableSemaphores[renderData.currentFrame],
                                                VK_NULL_HANDLE,
                                                &imageIndex);
        if (result == VK_ERROR_OUT_OF_DATE_KHR)
        {
            SEDX_CORE_INFO("VK_ERROR_OUT_OF_DATE_KHR returned from vkAcquireNextImageKHR - recreating swap chain");
            vkSwapChain->Create(Window::GetWidth(), Window::GetHeight(), renderData.VSync);
            return;
        }
        if (result == VK_SUBOPTIMAL_KHR)
            SEDX_CORE_INFO("VK_SUBOPTIMAL_KHR returned from vkAcquireNextImageKHR - continuing with render");
        else if (result != VK_SUCCESS)
            SEDX_CORE_ERROR_TAG("Graphics Engine", "Failed to acquire swap chain image: {}", ToString(result));

        /// Check if a Resize has been requested through the Window class
        if (Window::GetFramebufferResized())
        {
            SEDX_CORE_INFO("Framebuffer Resize detected from Window class");
            vkSwapChain->Create(Window::GetWidth(), Window::GetHeight(), renderData.VSync);
            return;
        }

        uniformBuffer->UpdateUniformBuffer(renderData.currentFrame);

        vkResetFences(vkDevice->GetDevice(), 1, &inFlightFences[renderData.currentFrame]);

        vkResetCommandBuffer(commandBuffers[renderData.currentFrame], /*VkCommandBufferResetFlagBits#1# 0);
        RecordCommandBuffer(commandBuffers[renderData.currentFrame], imageIndex);

        // -------------------------------------------------------

        VkSubmitInfo submitInfo{};
        submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

        VkSemaphore waitSemaphores[] = {imageAvailableSemaphores[renderData.currentFrame]};
        VkPipelineStageFlags waitStages[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
        submitInfo.waitSemaphoreCount = 1;
        submitInfo.pWaitSemaphores = waitSemaphores;
        submitInfo.pWaitDstStageMask = waitStages;
        submitInfo.commandBufferCount = 1;
        submitInfo.pCommandBuffers = &commandBuffers[renderData.currentFrame];

        VkSemaphore signalSemaphores[] = {renderFinishedSemaphores[renderData.currentFrame]};
        submitInfo.signalSemaphoreCount = 1;
        submitInfo.pSignalSemaphores = signalSemaphores;

        if (vkQueueSubmit(vkDevice->GetGraphicsQueue(), 1, &submitInfo, inFlightFences[renderData.currentFrame]) != VK_SUCCESS)
            SEDX_CORE_ERROR_TAG("Graphics Engine", "Failed to submit draw command buffer");

        // -------------------------------------------------------

        VkPresentInfoKHR presentInfo{};
        presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
        presentInfo.waitSemaphoreCount = 1;
        presentInfo.pWaitSemaphores = signalSemaphores;

        VkSwapchainKHR swapChains[] = {vkSwapChain->swapChain};
        presentInfo.swapchainCount = 1;
        presentInfo.pSwapchains = swapChains;
        presentInfo.pImageIndices = &imageIndex;

        // -------------------------------------------------------

        result = vkQueuePresentKHR(vkDevice->GetPresentQueue(), &presentInfo);
        if (result == VK_ERROR_OUT_OF_DATE_KHR)
        {
            SEDX_CORE_INFO("VK_ERROR_OUT_OF_DATE_KHR returned from vkQueuePresentKHR - recreating swap chain");
            renderData.framebufferResized = false;
            vkSwapChain->Create(viewportData.width, viewportData.height, renderData.VSync);
        }
        else if (result == VK_SUBOPTIMAL_KHR)
        {
            SEDX_CORE_INFO("VK_SUBOPTIMAL_KHR returned from vkQueuePresentKHR - recreating swap chain");
            renderData.framebufferResized = false;
            vkSwapChain->Create(viewportData.width, viewportData.height, renderData.VSync);
        }
        else if (Window::GetFramebufferResized())
        {
            SEDX_CORE_INFO("Window framebuffer Resize flag set - recreating swap chain");
            renderData.framebufferResized = false;
            vkSwapChain->Create(viewportData.width, viewportData.height, renderData.VSync);
        }
        else if (result != VK_SUCCESS)
            SEDX_CORE_ERROR_TAG("Graphics Engine", "Failed to present swap chain image: {}", ToString(result));

        renderData.currentFrame = (renderData.currentFrame + 1) % RenderData::framesInFlight;
    }
    */

	/*
	void GraphicsEngine::pickPhysicalDevice()
	{
	    // Initialize the physical device manager
	    physDeviceManager.Init(instance, surface);
	    
	    // Select a device that supports graphics operations and presentation
	    uint32_t queueFamilyIndex = physDeviceManager.SelectDevice(VK_QUEUE_GRAPHICS_BIT, true);
	    
	    // Get the selected device data
	    const GPUDevice& selectedDevice = physDeviceManager.Selected();
	    
	    // Store the physical device handle
	    physicalDevice = selectedDevice.physicalDevice;
	    
	    // The following properties are now available in selectedDevice:
	    // - selectedDevice.deviceInfo       (VkPhysicalDeviceProperties)
	    // - selectedDevice.GFXFeatures      (VkPhysicalDeviceFeatures)
	    // - selectedDevice.memoryInfo       (VkPhysicalDeviceMemoryProperties)
	    // - selectedDevice.queueFamilyInfo  (std::vector<VkQueueFamilyProperties>)
	    // - selectedDevice.queueSupportPresent (std::vector<VkBool32>)
	    // - selectedDevice.presentModes     (std::vector<VkPresentModeKHR>)
	    // - selectedDevice.surfaceFormats   (std::vector<VkSurfaceFormatKHR>)
	    // - selectedDevice.surfaceCapabilities (VkSurfaceCapabilitiesKHR)
	    
	    SEDX_CORE_INFO("Selected physical device: {}", ToString(selectedDevice.deviceInfo.deviceName));
	    
	    // Now we can use the data for other operations
	    // For example, instead of directly calling these functions:
	    // vkGetPhysicalDeviceFeatures(device, &physicalFeatures);
	    // vkGetPhysicalDeviceProperties(device, &physicalProperties);
	    // vkGetPhysicalDeviceMemoryProperties(device, &memoryProperties);
	    
	    // We can use the already collected data:
	    physicalFeatures = selectedDevice.GFXFeatures;
	    physicalProperties = selectedDevice.deviceInfo;
	    memoryProperties = selectedDevice.memoryInfo;
	}
	*/

    /*
    void GraphicsEngine::CreateLogicalDevice()
    {
        QueueFamilyIndices indices = FindQueueFamilies(vkPhysicalDevice->GetGPUDevice());

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

        VkPhysicalDeviceFeatures deviceFeatures{};
        deviceFeatures.samplerAnisotropy = VK_TRUE;

        VkDeviceCreateInfo createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
        createInfo.pQueueCreateInfos = queueCreateInfos.data();
        createInfo.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size());
        createInfo.pEnabledFeatures = &deviceFeatures;

		if (!checks->CheckDeviceExtensionSupport(vkPhysicalDevice->GetGPUDevice()))
        {
            SEDX_CORE_ERROR_TAG("Graphics Engine", "Required device extensions not supported!");
        }

        /// Enable swap chain extension
        createInfo.enabledExtensionCount = static_cast<uint32_t>(vkExtensions.requiredExtensions.size());
        createInfo.ppEnabledExtensionNames = vkExtensions.requiredExtensions.data();

        if (enableValidationLayers)
        {
            createInfo.enabledLayerCount = static_cast<uint32_t>(vkLayers.validationLayer.size());
            createInfo.ppEnabledLayerNames = vkLayers.validationLayer.data();
        }
        else
        {
            createInfo.enabledLayerCount = 0;
        }

        if (vkCreateDevice(vkPhysicalDevice->GetGPUDevice(), &createInfo, allocator, &device) != VK_SUCCESS)
        {
            SEDX_CORE_ERROR_TAG("Graphics Engine", "Failed to create logical device!");
        }

        vkGetDeviceQueue(vkDevice->GetDevice(), indices.graphicsFamily.value(), 0, &graphicsQueue);
        vkGetDeviceQueue(vkDevice->GetDevice(), indices.presentFamily.value(), 0, &presentQueue);
    }
    */

    void GraphicsEngine::CreateSwapChain()
    {
        /// Get swap chain support details from the selected device
        const GPUDevice &selectedDevice = vkPhysicalDevice->Selected();

        /// Create our swap chain using the capabilities from the selected device
        VkSwapchainCreateInfoKHR createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
        createInfo.surface = surface;

        /// Set minimum image count (usually min+1 for triple buffering)
        uint32_t imageCount = selectedDevice.surfaceCapabilities.minImageCount + 1;
        if (selectedDevice.surfaceCapabilities.maxImageCount > 0 &&
            imageCount > selectedDevice.surfaceCapabilities.maxImageCount)
        {
            imageCount = selectedDevice.surfaceCapabilities.maxImageCount;
        }
        createInfo.minImageCount = imageCount;

        /// Select format
        VkSurfaceFormatKHR surfaceFormat = ChooseSwapSurfaceFormat(selectedDevice.surfaceFormats);
        createInfo.imageFormat = surfaceFormat.format;
        createInfo.imageColorSpace = surfaceFormat.colorSpace;

        /// Select extent
        VkExtent2D extent = ChooseSwapExtent(selectedDevice.surfaceCapabilities);
        createInfo.imageExtent = extent;

        /// Additional settings
        createInfo.imageArrayLayers = 1;
        createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

        /*
        /// Handle queue families
        QueueFamilyIndices indices = FindQueueFamilies(vkPhysicalDevice->GetGPUDevice());
        uint32_t queueFamilyIndices[] = {indices.graphicsFamily.value(), indices.presentFamily.value()};

        if (indices.graphicsFamily != indices.presentFamily)
        {
            createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
            createInfo.queueFamilyIndexCount = 2;
            createInfo.pQueueFamilyIndices = queueFamilyIndices;
        }
        else
        {
            createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
            createInfo.queueFamilyIndexCount = 0;
            createInfo.pQueueFamilyIndices = nullptr;
        }
        */

        createInfo.preTransform = selectedDevice.surfaceCapabilities.currentTransform;
        createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
        createInfo.presentMode = ChooseSwapPresentMode(selectedDevice.presentModes);
        createInfo.clipped = VK_TRUE;

        createInfo.oldSwapchain = VK_NULL_HANDLE;

        if (vkCreateSwapchainKHR(vkDevice->GetDevice(), &createInfo, allocator, &vkSwapChain->swapChain) != VK_SUCCESS)
        {
            SEDX_CORE_ERROR_TAG("Graphics Engine", "Failed to create swap chain!");
        }


        /*
        /// Get swap chain images
        vkGetSwapchainImagesKHR(vkDevice->GetDevice(), vkSwapChain->swapChain, &imageCount, nullptr);
        vkSwapChain->swapImages.resize(imageCount);
        vkGetSwapchainImagesKHR(vkDevice->GetDevice(), vkSwapChain->swapChain, &imageCount, vkSwapChain->swapChainImages.data());

        /// Store format and extent for later use
        vkSwapChain->swapChainImageFormat = surfaceFormat.format;
        vkSwapChain->swapChainExtent = extent;

        SEDX_CORE_INFO("Swap chain created successfully with {} images", imageCount);
		*/
    }

	void GraphicsEngine::CreateImageViews()
    {
        swapChainImageViews.resize(vkSwapChain->swapChainImages.size());

        for (size_t i = 0; i < vkSwapChain->swapChainImages.size(); i++)
        {
            swapChainImageViews[i] = CreateImageView(vkSwapChain->swapChainImages[i].resource->image,static_cast<VkFormat>(vkSwapChain->swapChainImages[i].format),VK_IMAGE_ASPECT_COLOR_BIT,1);
        }
    }

    VkImageView GraphicsEngine::CreateImageView(VkImage image, VkFormat format, VkImageAspectFlags aspectFlags, uint32_t mipLevels) const
    {
        VkImageViewCreateInfo viewInfo{};
        viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        viewInfo.image = image;
        viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
        viewInfo.format = format;
        viewInfo.subresourceRange.aspectMask = aspectFlags;
        viewInfo.subresourceRange.baseMipLevel = 0;
        viewInfo.subresourceRange.levelCount = mipLevels;
        viewInfo.subresourceRange.baseArrayLayer = 0;
        viewInfo.subresourceRange.layerCount = 1;

        VkImageView imageView;
        if (vkCreateImageView(vkDevice->GetDevice(), &viewInfo, allocator, &imageView) != VK_SUCCESS)
        {
            SEDX_CORE_ERROR_TAG("Graphics Engine", "Failed to create texture image view!");
        }

        return imageView;
    }

	/*
	void GraphicsEngine::TransitionImageLayout(VkImage image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout, uint32_t mipLevels) const
    {
        Ref<CommandBuffer> cmdBuffer;
        cmdBuffer->Begin();

        VkImageMemoryBarrier barrier{};
        barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        barrier.oldLayout = oldLayout;
        barrier.newLayout = newLayout;
        barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.image = image;
        barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        barrier.subresourceRange.baseMipLevel = 0;
        barrier.subresourceRange.levelCount = mipLevels;
        barrier.subresourceRange.baseArrayLayer = 0;
        barrier.subresourceRange.layerCount = 1;

        VkPipelineStageFlags sourceStage;
        VkPipelineStageFlags destinationStage;

        if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL)
        {
            barrier.srcAccessMask = 0;
            barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

            sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
            destinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
        }
        else if (oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)
        {
            barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
            barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

            sourceStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
            destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
        }
        else
        {
            SEDX_CORE_ERROR_TAG("Graphics Engine", "Unsupported layout transition!");
        }

        vkCmdPipelineBarrier(commandBuffer, sourceStage, destinationStage, 0, 0, nullptr, 0, nullptr, 1, &barrier);

        EndSingleTimeCommands(commandBuffer);
    }
    */

    void GraphicsEngine::FramebufferResizeCallback(GLFWwindow *window, int width, int height)
    {
        /// Mark the framebuffer as resized
        if (static_cast<Window *>(glfwGetWindowUserPointer(window)))
        {
            Window::SetFramebufferResized(true);
            SEDX_CORE_INFO("Framebuffer resized: {}x{}", width, height);
        }
        else
        /// Fallback if Window pointer isn't set
            EDITOR_LOG_WARN("Framebuffer Resize detected but no Window instance found");
    }

	/// -------------------------------------------------------

    /*
    void GraphicsEngine::CreateDepthResources()
    {
        VkFormat depthFormat = FindDepthFormat();

        CreateImage(renderData.width,
                    renderData.height,
					1,
                    renderData.msaaSamples, depthFormat,
                    VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                    depthImage, depthImageMemory);
        depthImageView = CreateImageView(depthImage, depthFormat, VK_IMAGE_ASPECT_DEPTH_BIT, 1);
    }
    */

	/// -------------------------------------------------------

	void GraphicsEngine::CreateDescriptorPool()
    {
        std::array<VkDescriptorPoolSize, 2> poolSizes{};

		// -------------------------------------------------------

        poolSizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        poolSizes[0].descriptorCount = RenderData::framesInFlight;

        // -------------------------------------------------------

        poolSizes[1].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        poolSizes[1].descriptorCount = RenderData::framesInFlight;

		// -------------------------------------------------------

        VkDescriptorPoolCreateInfo poolInfo{};
        poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
        poolInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
        poolInfo.pPoolSizes = poolSizes.data();
        poolInfo.maxSets = RenderData::framesInFlight;

        if (vkCreateDescriptorPool(vkDevice->GetDevice(), &poolInfo, allocator, &descriptorPool) != VK_SUCCESS)
        {
            SEDX_CORE_ERROR_TAG("Graphics Engine", "Failed to create descriptor pool!");
        }
    }

	void GraphicsEngine::CreateDescriptorSetLayout()
    {
		// -------------------------------------------------------

        VkDescriptorSetLayoutBinding uboLayoutBinding{};
        uboLayoutBinding.binding = 0;
        uboLayoutBinding.descriptorCount = 1;
        uboLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        uboLayoutBinding.pImmutableSamplers = nullptr;
        uboLayoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

        VkDescriptorSetLayoutBinding samplerLayoutBinding{};
        samplerLayoutBinding.binding = 1;
        samplerLayoutBinding.descriptorCount = 1;
        samplerLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        samplerLayoutBinding.pImmutableSamplers = nullptr;
        samplerLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

		// -------------------------------------------------------

        std::array<VkDescriptorSetLayoutBinding, 2> bindings = {uboLayoutBinding, samplerLayoutBinding};

		// -------------------------------------------------------

        VkDescriptorSetLayoutCreateInfo layoutInfo{};
        layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
        layoutInfo.bindingCount = static_cast<uint32_t>(bindings.size());
        layoutInfo.pBindings = bindings.data();

        if (vkCreateDescriptorSetLayout(vkDevice->GetDevice(), &layoutInfo, allocator, &descriptorSetLayout) != VK_SUCCESS)
            SEDX_CORE_ERROR_TAG("Graphics Engine", "Failed to create descriptor set layout!");
    }

	void GraphicsEngine::CreateFramebuffers()
    {
        swapChainFramebuffers.resize(swapChainImageViews.size());

        for (size_t i = 0; i < swapChainImageViews.size(); i++)
        {
            std::array<VkImageView, 2> attachments = {swapChainImageViews[i], depthImageView};

            VkFramebufferCreateInfo framebufferInfo{};
            framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
            framebufferInfo.renderPass = renderPass;
            framebufferInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
            framebufferInfo.pAttachments = attachments.data();
            framebufferInfo.width = viewportData.width;
            framebufferInfo.height = viewportData.height;
            framebufferInfo.layers = 1;

            if (vkCreateFramebuffer(vkDevice->GetDevice(), &framebufferInfo, allocator, &swapChainFramebuffers[i]) != VK_SUCCESS)
                SEDX_CORE_ERROR_TAG("Graphics Engine", "Failed to create framebuffer!");
        }
    }

    void GraphicsEngine::CreateSyncObjects()
    {
        imageAvailableSemaphores.resize(RenderData::framesInFlight);
        renderFinishedSemaphores.resize(RenderData::framesInFlight);
        inFlightFences.resize(RenderData::framesInFlight);

        VkSemaphoreCreateInfo semaphoreInfo{};
        semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

        VkFenceCreateInfo fenceInfo{};
        fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
        fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

        for (size_t i = 0; i < RenderData::framesInFlight; i++)
            if (vkCreateSemaphore(vkDevice->GetDevice(), &semaphoreInfo, allocator, &imageAvailableSemaphores[i]) != VK_SUCCESS ||
                vkCreateSemaphore(vkDevice->GetDevice(), &semaphoreInfo, allocator, &renderFinishedSemaphores[i]) != VK_SUCCESS ||
                vkCreateFence(vkDevice->GetDevice(), &fenceInfo, allocator, &inFlightFences[i]) != VK_SUCCESS)
            {
                ErrMsg("failed to create synchronization objects for a frame!");
            }
    }

	/*
	void GraphicsEngine::RecreateSwapChain()
    {
        SEDX_CORE_INFO("Beginning swap chain recreation");

        int width = 0, height = 0;
        glfwGetFramebufferSize(WindowData::window, &width, &height);
        SEDX_CORE_INFO("Current framebuffer size: {}x{}", width, height);

        while (width == 0 || height == 0)
        {
            SEDX_CORE_INFO("Window minimized, waiting for restore");
            glfwGetFramebufferSize(WindowData::window, &width, &height);
            glfwWaitEvents();
        }

		SEDX_CORE_INFO("Window class dimensions: {}x{}", Window::GetWidth(), Window::GetHeight());

		SEDX_CORE_INFO("Waiting for vkDevice to be idle");
        vkDeviceWaitIdle(vkDevice->GetDevice());

        SEDX_CORE_INFO("Destroying old swap chain");
        CleanupSwapChain();

		SEDX_CORE_INFO("Creating new swap chain with updated dimensions");
        //CreateSwapChain();
        CreateImageViews();
        CreateRenderPass();
        CreateGraphicsPipeline();
        CreateFramebuffers();

		SEDX_CORE_INFO("Swap chain recreation completed");

		/// Reset the framebuffer resized flag
        renderData.framebufferResized = false;
    }*/

	// -------------------------------------------------------

    VkSampleCountFlagBits GraphicsEngine::GetMaxUsableSampleCount() const
    {
        VkPhysicalDeviceProperties physicalDeviceProperties;
        vkGetPhysicalDeviceProperties(vkPhysDevice, &physicalDeviceProperties);

        VkSampleCountFlags counts = physicalDeviceProperties.limits.framebufferColorSampleCounts &
                                    physicalDeviceProperties.limits.framebufferDepthSampleCounts;
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

	// -------------------------------------------------------

    VkPresentModeKHR GraphicsEngine::ChooseSwapPresentMode(const std::vector<VkPresentModeKHR> &availablePresentModes)
    {
        for (const auto &availablePresentMode : availablePresentModes)
            if (availablePresentMode == VK_PRESENT_MODE_MAILBOX_KHR)
                return availablePresentMode;

        return VK_PRESENT_MODE_FIFO_KHR;
    }

    VkSurfaceFormatKHR GraphicsEngine::ChooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR> &availableFormats)
    {
        for (const auto &availableFormat : availableFormats)
            if (availableFormat.format == VK_FORMAT_B8G8R8A8_SRGB &&
                availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
                return availableFormat;

        return availableFormats[0];
    }

    VkExtent2D GraphicsEngine::ChooseSwapExtent(const VkSurfaceCapabilitiesKHR &capabilities)
    {
        /// If the current extent width is the max value, it means the window manager
        /// allows us to set dimensions other than the current window size
        if (capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max())
        {
            EDITOR_LOG_INFO("Using surface extent: {}x{}", capabilities.currentExtent.width, capabilities.currentExtent.height);
            return capabilities.currentExtent;
        }

        /// Get the actual framebuffer size from GLFW directly for consistency
        glfwGetFramebufferSize(Window::GetWindow(), &WindowData::width, &WindowData::height);

        EDITOR_LOG_INFO("Window framebuffer size: {}x{}", WindowData::width, WindowData::height);
			
        /// Create the extent using the retrieved dimensions
        VkExtent2D actualExtent = {static_cast<uint32_t>(WindowData::width),static_cast<uint32_t>(WindowData::height)};

        /// Clamp to the allowed min/max extents from the surface capabilities
        actualExtent.width = std::clamp(actualExtent.width, capabilities.minImageExtent.width, capabilities.maxImageExtent.width);
        actualExtent.height =  std::clamp(actualExtent.height, capabilities.minImageExtent.height, capabilities.maxImageExtent.height);

        EDITOR_LOG_INFO("Using calculated extent: {}x{}", actualExtent.width, actualExtent.height);
        return actualExtent;
    }

    void GraphicsEngine::CreateTextureImageView()
    {
        textureImageView = CreateImageView(textureImage, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_ASPECT_COLOR_BIT, 1);
    }

    void GraphicsEngine::CreateTextureSampler()
    {
        VkPhysicalDeviceProperties properties{};
        vkGetPhysicalDeviceProperties(vkPhysicalDevice->GetGPUDevices(), &properties);

        VkSamplerCreateInfo samplerInfo{};
        samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
        samplerInfo.magFilter = VK_FILTER_LINEAR;
        samplerInfo.minFilter = VK_FILTER_LINEAR;
        samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
        samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
        samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
        samplerInfo.anisotropyEnable = VK_TRUE;
        samplerInfo.maxAnisotropy = properties.limits.maxSamplerAnisotropy;
        samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
        samplerInfo.unnormalizedCoordinates = VK_FALSE;
        samplerInfo.compareEnable = VK_FALSE;
        samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;
        samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;

        if (vkCreateSampler(vkDevice->GetDevice(), &samplerInfo, allocator, &textureSampler) != VK_SUCCESS)
            SEDX_CORE_ERROR_TAG("Graphics Engine", "failed to create texture sampler!");
    }

    SwapChainDetails GraphicsEngine::QuerySwapChainSupport(VkPhysicalDevice device) const
    {
        SwapChainDetails details;
        const GPUDevice &selectedDevice = vkPhysicalDevice->Selected();

        /// Copy the data that's already available
        details.capabilities = selectedDevice.surfaceCapabilities;
        details.formats = selectedDevice.surfaceFormats;
        details.presentModes = selectedDevice.presentModes;

        return details;
    }

	// -------------------------------------------------------

	VkFormat GraphicsEngine::FindSupportedFormat(const std::vector<VkFormat> &candidates, VkImageTiling tiling, VkFormatFeatureFlags features) const
    {
        for (VkFormat format : candidates)
        {
            VkFormatProperties props;

            vkGetPhysicalDeviceFormatProperties(vkPhysDevice, format, &props);

            if (tiling == VK_IMAGE_TILING_LINEAR && (props.linearTilingFeatures & features) == features)
                return format;
            if (tiling == VK_IMAGE_TILING_OPTIMAL && (props.optimalTilingFeatures & features) == features)
                return format;
        }

        SEDX_CORE_ERROR_TAG("Graphics Engine", "Failed to find supported format!");
        return VK_FORMAT_UNDEFINED;
    }

    VkFormat GraphicsEngine::FindDepthFormat() const
    {
        return FindSupportedFormat({VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT},
                                   VK_IMAGE_TILING_OPTIMAL,
                                   VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT);
    }

} // namespace SceneryEditorX

// -------------------------------------------------------
