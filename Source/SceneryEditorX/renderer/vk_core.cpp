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
#include <optional>
#include <SceneryEditorX/core/window.h>
#include <SceneryEditorX/platform/windows/editor_config.hpp>
#include <SceneryEditorX/renderer/vk_core.h>
#include <SceneryEditorX/renderer/vk_device.h>
#include <SceneryEditorX/renderer/vk_util.h>
#include <SceneryEditorX/ui/ui.h>
#include <stb_image.h>
#include <tiny_obj_loader.h>
#include <unordered_map>
#include <vector>

// -------------------------------------------------------

namespace SceneryEditorX
{
    vkPhysDevice physDevice;
	GPUDevice gpuDevice;

	// -------------------------------------------------------

	static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity, VkDebugUtilsMessageTypeFlagsEXT messageType,
	                                                    const VkDebugUtilsMessengerCallbackDataEXT *pCallbackData, void *pUserData)
	{
	    // Build a rich debug message that includes severity and type info
	    std::string severityStr = VK_DEBUG_SEVERITY_STRING(messageSeverity);
	    std::string typeStr = VK_DEBUG_TYPE(messageType);
	
	    std::string formattedMessage = "[" + severityStr + "][" + typeStr + "] " + pCallbackData->pMessage;
	
	    // Log through our custom Vulkan logger
	    Log::LogVulkanDebug(formattedMessage);
	
	    // Also log any objects that were involved in the message
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
	
	    return VK_FALSE; // Don't abort call
	}

    std::vector<const char *> getRequiredExtensions()
    {
        uint32_t glfwExtensionCount = 0;
        const char **glfwExtensions;
        glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

        std::vector<const char *> extensions(glfwExtensions, glfwExtensions + glfwExtensionCount);

        if (physDevice.enableValidationLayers)
        {
            extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
        }

        return extensions;
    }

	QueueFamilyIndices GraphicsEngine::findQueueFamilies(VkPhysicalDevice device)
    {
        return physDeviceManager.findQueueFamilies(device, physDevice.GetSurface());
    }

    // -------------------------------------------------------

	GraphicsEngine::GraphicsEngine(GLFWwindow *window)
    {
    }

    GraphicsEngine::~GraphicsEngine()
    {
    }

    void GraphicsEngine::initEngine(GLFWwindow *window, uint32_t width, uint32_t height)
	{
	    this->window = window;

        EDITOR_LOG_INFO("Initializing graphics engine with window size: {}x{}", width, height);

        // Set the window user pointer to point to the Window singleton
        glfwSetWindowUserPointer(window, Window::GetGLFWwindow());

		// -------------------------------------------------------

        createInstance();
        createDebugMessenger();

		VkSurfaceKHR surface;
        createSurface(window, surface);

        //VkSurfaceKHR surface = VK_NULL_HANDLE;
        //VkResult createSurface(GLFWwindow * window, VkSurfaceKHR & surface)
        //{
        //    return glfwCreateWindowSurface(instance, window, nullptr, &surface);
        //};

        // Initialize the physical device
        physDeviceManager.Init(instance, surface);

        // Select a device with graphics and presentation capabilities
        physDeviceManager.SelectDevice(VK_QUEUE_GRAPHICS_BIT, true);

        // Get the physical and logical devices
        physicalDevice = physDeviceManager.GetPhysicalDevice();
        device = physDeviceManager.GetDevice();
        graphicsQueue = physDeviceManager.GetGraphicsQueue();
        presentQueue = physDeviceManager.GetPresentQueue();

        // Get queue family indices for later use
        queueFamilyIndices = physDeviceManager.findQueueFamilies(physicalDevice, surface);
    
        createSwapChain();
        createImageViews();
        createRenderPass();
        createDescriptorSetLayout();
        createGraphicsPipeline();
        createCommandPool();
        createColorResources();
        createDepthResources();
        createFramebuffers();
        createTextureImage();
        createTextureImageView();
        createTextureSampler();
        loadModel();
        createVertexBuffer();
        createIndexBuffer();
        createUniformBuffers();
        createDescriptorPool();
        createDescriptorSets();
        createCommandBuffers();
        createSyncObjects();

		EDITOR_LOG_INFO("Graphics engine initialization complete");
    }

    void GraphicsEngine::cleanUp(VkDevice device, VkSurfaceKHR surface) 
    {
        GUI guiInstance;
        guiInstance.cleanUp();

        cleanupSwapChain(device, surface);

		for (size_t i = 0; i < framesInFlight; i++)
        {
            vkDestroyBuffer(device, uniformBuffers[i], allocator);
            vkFreeMemory(device, uniformBuffersMemory[i], allocator);
        }

        vkDestroyDescriptorPool(device, descriptorPool, allocator);

        vkDestroySampler(device, textureSampler, allocator);

        for (auto imageView : swapChainImageViews)
        {
            vkDestroyImageView(device, imageView, allocator);
        }

        vkDestroyImage(device, textureImage, allocator);
        vkFreeMemory(device, textureImageMemory, allocator);

        vkDestroyDescriptorSetLayout(device, descriptorSetLayout, allocator);

        vkDestroyBuffer(device, indexBuffer, allocator);
        vkFreeMemory(device, indexBufferMemory, allocator);

        vkDestroyBuffer(device, vertexBuffer, allocator);
        vkFreeMemory(device, vertexBufferMemory, allocator);

		for (size_t i = 0; i < framesInFlight; i++)
        {
            vkDestroySemaphore(device, renderFinishedSemaphores[i], allocator);
            vkDestroySemaphore(device, imageAvailableSemaphores[i], allocator);
            vkDestroyFence(device, inFlightFences[i], allocator);
        }

        vkDestroySwapchainKHR(device, swapChain, allocator);

		physDevice.~vkPhysDevice();

        if (physDevice.enableValidationLayers)
        {
            GraphicsEngine::DestroyDebugUtilsMessengerEXT(instance, debugMessenger, allocator);
        }

        vkDestroySurfaceKHR(instance, surface, allocator);
        vkDestroyInstance(instance, allocator);
    }

	void GraphicsEngine::cleanupSwapChain(VkDevice device, VkSurfaceKHR surface)
    {
        vkDestroyImageView(device, depthImageView, allocator);
        vkDestroyImage(device, depthImage, allocator);
        vkFreeMemory(device, depthImageMemory, allocator);

        for (auto framebuffer : swapChainFramebuffers)
        {
            vkDestroyFramebuffer(device, framebuffer, allocator);
        }

        for (size_t i = 0; i < swapChainImages.size(); i++)
        {
            vkDestroyImageView(device, swapChainImageViews[i], allocator);
        }

		vkDestroyPipeline(device, graphicsPipeline, allocator);
        vkDestroyPipelineLayout(device, pipelineLayout, allocator);
        vkDestroyRenderPass(device, renderPass, allocator);

        for (size_t i = 0; i < currentFrame; i++)
        {
            vkDestroySemaphore(device, imageAvailableSemaphores[i], allocator);
            vkDestroySemaphore(device, renderFinishedSemaphores[i], allocator);
        }

        vkDestroySwapchainKHR(device, swapChain, allocator);

		/*
        for (int q = 0; q < Queue::Count; q++)
        {
            for (int i = 0; i < currentFrame; i++)
            {
                vkDestroyCommandPool(device, queues[q].commands[i].pool, allocator);
                queues[q].commands[i].staging = {};
                queues[q].commands[i].stagingCpu = nullptr;
                vkDestroyFence(device, queues[q].commands[i].fence, allocator);
                vkDestroyQueryPool(device, queues[q].commands[i].queryPool, allocator);
            }
        }
		*/

        imageAvailableSemaphores.clear();
        renderFinishedSemaphores.clear();
        swapChainImageViews.clear();
        swapChainImages.clear();
        swapChain = VK_NULL_HANDLE;
	}

    // -------------------------------------------------------

    VkResult GraphicsEngine::CreateDebugUtilsMessengerEXT(VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT *pCreateInfo, const VkAllocationCallbacks *pAllocator,
                                                          VkDebugUtilsMessengerEXT *pDebugMessenger)
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

    void GraphicsEngine::DestroyDebugUtilsMessengerEXT(VkInstance instance, VkDebugUtilsMessengerEXT debugMessenger, const VkAllocationCallbacks *pAllocator)
    {
        auto func = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT");
        if (func != nullptr)
        {
            func(instance, debugMessenger, pAllocator);
        }
    }

    void GraphicsEngine::createDebugMessenger()
    {
        if (!physDevice.enableValidationLayers)
            return;

        VkDebugUtilsMessengerCreateInfoEXT createInfo;
        populateDebugMessengerCreateInfo(createInfo);

        if (CreateDebugUtilsMessengerEXT(instance, &createInfo, allocator, &debugMessenger) != VK_SUCCESS)
        {
            EDITOR_LOG_ERROR("Failed to set up debug messenger!");
            ErrMsg("Failed to set up debug messenger!");
        }
    }

    void GraphicsEngine::createSurface(GLFWwindow *glfwWindow, VkSurfaceKHR surface)
    {
        if (glfwCreateWindowSurface(instance, glfwWindow, allocator, &surface) != VK_SUCCESS)
        {
            EDITOR_LOG_ERROR("Failed to create window surface!");
            ErrMsg("Failed to create window surface!");
        }
    }

    void GraphicsEngine::populateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT & createInfo)
    {
        createInfo = {};
        createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;

		    // Include all severity levels for comprehensive logging
        createInfo.messageSeverity =
            VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT |
            VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;

        // Include all message types
        createInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
                                 VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
                                 VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;

        createInfo.pfnUserCallback = debugCallback;
        createInfo.pUserData = nullptr;
    }

    // -------------------------------------------------------

    void GraphicsEngine::createInstance()
    {
        if (physDevice.enableValidationLayers && !physDevice.checkValidationLayerSupport())
        {
            EDITOR_LOG_ERROR("Validation layers requested, but not available!");
            ErrMsg("Validation layers requested, but not available!");
        }

        uint32_t layerCount = 0;
        vkEnumerateInstanceLayerProperties(&layerCount, nullptr);
        layers.resize(layerCount);
        physDevice.activeLayers.resize(layerCount);
        vkEnumerateInstanceLayerProperties(&layerCount, layers.data());

        // get all available extensions
        uint32_t extensionCount = 0;
        vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, 0);
        physDevice.instanceExtensions.resize(extensionCount);
        physDevice.activeExtensions.resize(extensionCount);
        vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, physDevice.instanceExtensions.data());
        vkEnumerateInstanceVersion(&apiVersion);

        VkApplicationInfo appInfo{};
        appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
        appInfo.pApplicationName = "Scenery Editor X";
        appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
        appInfo.pEngineName = "SEDX Editor Engine";
        appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
        appInfo.apiVersion = VK_API_VERSION_1_0;

        VkInstanceCreateInfo createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
        createInfo.pApplicationInfo = &appInfo;

        auto extensions = getRequiredExtensions();
        createInfo.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
        createInfo.ppEnabledExtensionNames = extensions.data();

        VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo{};
        if (physDevice.enableValidationLayers)
        {
            const auto &validationLayers = VulkanExtensions::GetValidationLayers();
            createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
            createInfo.ppEnabledLayerNames = validationLayers.data();

            populateDebugMessengerCreateInfo(debugCreateInfo);
            createInfo.pNext = (VkDebugUtilsMessengerCreateInfoEXT *)&debugCreateInfo;
        }
        else
        {
            createInfo.enabledLayerCount = 0;
            createInfo.pNext = nullptr;
        }

        if (vkCreateInstance(&createInfo, allocator, &instance) != VK_SUCCESS)
        {
            EDITOR_LOG_ERROR("Failed to create instance!");
            ErrMsg("Failed to create graphics instance!");
        }
    }

    void GraphicsEngine::createSwapChain()
    {
        // Get swap chain support details from the selected device
        const GPUDevice &selectedDevice = physDeviceManager.Selected();

        // Create our swap chain using the capabilities from the selected device
        VkSwapchainCreateInfoKHR createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
        createInfo.surface = physDevice.GetSurface();

        // Set minimum image count (usually min+1 for triple buffering)
        uint32_t imageCount = selectedDevice.surfaceCapabilities.minImageCount + 1;
        if (selectedDevice.surfaceCapabilities.maxImageCount > 0 &&
            imageCount > selectedDevice.surfaceCapabilities.maxImageCount)
        {
            imageCount = selectedDevice.surfaceCapabilities.maxImageCount;
        }
        createInfo.minImageCount = imageCount;

        // Select format
        VkSurfaceFormatKHR surfaceFormat = chooseSwapSurfaceFormat(selectedDevice.surfaceFormats);
        createInfo.imageFormat = surfaceFormat.format;
        createInfo.imageColorSpace = surfaceFormat.colorSpace;

        // Select extent
        VkExtent2D extent = chooseSwapExtent(selectedDevice.surfaceCapabilities);
        createInfo.imageExtent = extent;

        // Additional settings
        createInfo.imageArrayLayers = 1;
        createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

        // Handle queue families
        QueueFamilyIndices indices = findQueueFamilies(physicalDevice);
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

        createInfo.preTransform = selectedDevice.surfaceCapabilities.currentTransform;
        createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
        createInfo.presentMode = chooseSwapPresentMode(selectedDevice.presentModes);
        createInfo.clipped = VK_TRUE;

        createInfo.oldSwapchain = VK_NULL_HANDLE;

        if (vkCreateSwapchainKHR(device, &createInfo, allocator, &swapChain) != VK_SUCCESS)
        {
            EDITOR_LOG_ERROR("Failed to create swap chain!");
            ErrMsg("Failed to create swap chain!");
        }

        // Get swap chain images
        vkGetSwapchainImagesKHR(device, swapChain, &imageCount, nullptr);
        swapChainImages.resize(imageCount);
        vkGetSwapchainImagesKHR(device, swapChain, &imageCount, swapChainImages.data());

        // Store format and extent for later use
        swapChainImageFormat = surfaceFormat.format;
        swapChainExtent = extent;

        EDITOR_LOG_INFO("Swap chain created successfully with {} images", imageCount);
    }

	void GraphicsEngine::createImageViews()
    {
        swapChainImageViews.resize(swapChainImages.size());

        for (size_t i = 0; i < swapChainImages.size(); i++)
        {
            swapChainImageViews[i] = GraphicsEngine::createImageView(swapChainImages[i], swapChainImageFormat, VK_IMAGE_ASPECT_COLOR_BIT, 1);
        }
    }

    VkImageView GraphicsEngine::createImageView(VkImage image, VkFormat format, VkImageAspectFlags aspectFlags, uint32_t mipLevels)
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
        if (vkCreateImageView(device, &viewInfo, allocator, &imageView) != VK_SUCCESS)
        {
            EDITOR_LOG_ERROR("Failed to create texture image view!");
            ErrMsg("Failed to create texture image view!");
        }

        return imageView;
    }

	void GraphicsEngine::createImage(uint32_t width, uint32_t height, uint32_t mipLevels, VkSampleCountFlagBits numSamples, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage, VkMemoryPropertyFlags properties, VkImage &image, VkDeviceMemory &imageMemory)
    {
        VkImageCreateInfo imageInfo{};
        imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
        imageInfo.imageType = VK_IMAGE_TYPE_2D;
        imageInfo.extent.width = width;
        imageInfo.extent.height = height;
        imageInfo.extent.depth = 1;
        imageInfo.mipLevels = mipLevels;
        imageInfo.arrayLayers = 1;
        imageInfo.format = format;
        imageInfo.tiling = tiling;
        imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        imageInfo.usage = usage;
        imageInfo.samples = numSamples;
        imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

        if (vkCreateImage(device, &imageInfo, allocator, &image) != VK_SUCCESS)
        {
            EDITOR_LOG_ERROR("Failed to create image!");
            ErrMsg("Failed to create image!");
        }

        VkMemoryRequirements memRequirements;
        vkGetImageMemoryRequirements(device, image, &memRequirements);

        VkMemoryAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        allocInfo.allocationSize = memRequirements.size;
        allocInfo.memoryTypeIndex = findMemoryType(memRequirements.memoryTypeBits, properties);

        if (vkAllocateMemory(device, &allocInfo, allocator, &imageMemory) != VK_SUCCESS)
        {
            EDITOR_LOG_ERROR("Failed to allocate image memory!");
            ErrMsg("Failed to allocate image memory!");
        }

        vkBindImageMemory(device, image, imageMemory, 0);
    }

	void GraphicsEngine::transitionImageLayout(VkImage image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout, uint32_t mipLevels)
    {
        VkCommandBuffer commandBuffer = beginSingleTimeCommands();

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
        else if (oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL &&
                 newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)
        {
            barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
            barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

            sourceStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
            destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
        }
        else
        {
            EDITOR_LOG_ERROR("Unsupported layout transition!");
            ErrMsg("Unsupported layout transition!");
        }

        vkCmdPipelineBarrier(commandBuffer, sourceStage, destinationStage, 0, 0, nullptr, 0, nullptr, 1, &barrier);

        endSingleTimeCommands(commandBuffer);
    }

    void GraphicsEngine::framebufferResizeCallback(GLFWwindow *window, int width, int height)
    {
        // Get the window instance that was set with glfwSetWindowUserPointer
        auto windowInstance = static_cast<Window *>(glfwGetWindowUserPointer(window));

        // Mark the framebuffer as resized
        if (windowInstance)
        {
            windowInstance->setFramebufferResized(true);
            EDITOR_LOG_INFO("Framebuffer resized: {}x{}", width, height);
        }
        else
        {
            // Fallback if Window pointer isn't set
            EDITOR_LOG_WARN("Framebuffer resize detected but no Window instance found");
        }
    }

	void GraphicsEngine::createRenderPass()
    {
        // -------------------------------------------------------

        VkAttachmentDescription colorAttachment{};
        colorAttachment.format = swapChainImageFormat;
        colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
        colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

		VkAttachmentDescription depthAttachment{};
        depthAttachment.format = physDeviceManager.findDepthFormat();
        depthAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
        depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        depthAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        depthAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        depthAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        depthAttachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

		// -------------------------------------------------------

        VkAttachmentReference colorAttachmentRef{};
        colorAttachmentRef.attachment = 0;
        colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

        VkAttachmentReference depthAttachmentRef{};
        depthAttachmentRef.attachment = 1;
        depthAttachmentRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

		// -------------------------------------------------------

        VkSubpassDescription subpass{};
        subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
        subpass.colorAttachmentCount = 1;
        subpass.pColorAttachments = &colorAttachmentRef;
        subpass.pDepthStencilAttachment = &depthAttachmentRef;

		// -------------------------------------------------------

        VkSubpassDependency dependency{};
        dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
        dependency.dstSubpass = 0;
        dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
        dependency.srcAccessMask = 0;
        dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
        dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

		// -------------------------------------------------------

		std::array<VkAttachmentDescription, 2> attachments = {colorAttachment, depthAttachment};

        VkRenderPassCreateInfo renderPassInfo{};
        renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
        renderPassInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
        renderPassInfo.pAttachments = attachments.data();
        renderPassInfo.subpassCount = 1;
        renderPassInfo.pSubpasses = &subpass;
        renderPassInfo.dependencyCount = 1;
        renderPassInfo.pDependencies = &dependency;

        if (vkCreateRenderPass(device, &renderPassInfo, allocator, &renderPass) != VK_SUCCESS)
        {
            EDITOR_LOG_ERROR("Failed to create render pass!");
            ErrMsg("failed to create render pass!");
        }
    }

	// -------------------------------------------------------

	void GraphicsEngine::createBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer& buffer, VkDeviceMemory& bufferMemory)
    {
        VkBufferCreateInfo bufferInfo{};
        bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        bufferInfo.size = size;
        bufferInfo.usage = usage;
        bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

        if (vkCreateBuffer(device, &bufferInfo, allocator, &buffer) != VK_SUCCESS)
        {
            EDITOR_LOG_ERROR("Failed to create buffer!");
            ErrMsg("Failed to create buffer!");
        }

		// -------------------------------------------------------

        VkMemoryRequirements memRequirements;
        vkGetBufferMemoryRequirements(device, buffer, &memRequirements);

		// -------------------------------------------------------

        VkMemoryAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        allocInfo.allocationSize = memRequirements.size;
        allocInfo.memoryTypeIndex = findMemoryType(memRequirements.memoryTypeBits, properties);

        if (vkAllocateMemory(device, &allocInfo, allocator, &bufferMemory) != VK_SUCCESS)
        {
            EDITOR_LOG_ERROR("Failed to allocate buffer memory!");
            ErrMsg("Failed to allocate buffer memory!");
        }

        vkBindBufferMemory(device, buffer, bufferMemory, 0);
    }

    void GraphicsEngine::createVertexBuffer()
    {
        VkDeviceSize bufferSize = sizeof(vertices[0]) * vertices.size();

        VkBuffer stagingBuffer;
        VkDeviceMemory stagingBufferMemory;
        createBuffer(bufferSize,
                     VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                     VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                     stagingBuffer,
                     stagingBufferMemory);

        void *data;
        vkMapMemory(device, stagingBufferMemory, 0, bufferSize, 0, &data);
        memcpy(data, vertices.data(), (size_t)bufferSize);
        vkUnmapMemory(device, stagingBufferMemory);

        createBuffer(bufferSize,
                     VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
                     VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                     vertexBuffer,
                     vertexBufferMemory);

        copyBuffer(stagingBuffer, vertexBuffer, bufferSize);

        vkDestroyBuffer(device, stagingBuffer, allocator);
        vkFreeMemory(device, stagingBufferMemory, allocator);
    }

    void GraphicsEngine::createDepthResources()
    {
        VkFormat depthFormat = physDeviceManager.findDepthFormat();

        createImage(swapChainExtent.width,
                    swapChainExtent.height,
					1,
                    msaaSamples, depthFormat,
                    VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                    depthImage, depthImageMemory);
        depthImageView = createImageView(depthImage, depthFormat, VK_IMAGE_ASPECT_DEPTH_BIT, 1);
    }

    void GraphicsEngine::createIndexBuffer()
    {
        VkDeviceSize bufferSize = sizeof(indices[0]) * indices.size();

        VkBuffer stagingBuffer;
        VkDeviceMemory stagingBufferMemory;
        createBuffer(bufferSize,
                     VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                     VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                     stagingBuffer,
                     stagingBufferMemory);

        void *data;
        vkMapMemory(device, stagingBufferMemory, 0, bufferSize, 0, &data);
        memcpy(data, indices.data(), (size_t)bufferSize);
        vkUnmapMemory(device, stagingBufferMemory);

        createBuffer(bufferSize,
                     VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
                     VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                     indexBuffer,
                     indexBufferMemory);

        copyBuffer(stagingBuffer, indexBuffer, bufferSize);

        vkDestroyBuffer(device, stagingBuffer, allocator);
        vkFreeMemory(device, stagingBufferMemory, allocator);
    }

    void GraphicsEngine::createUniformBuffers()
    {
        VkDeviceSize bufferSize = sizeof(UniformBufferObject);

        uniformBuffers.resize(framesInFlight);
        uniformBuffersMemory.resize(framesInFlight);

        for (size_t i = 0; i < framesInFlight; i++)
        {
            createBuffer(bufferSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                         uniformBuffers[i],
                         uniformBuffersMemory[i]);
        }
    }

	// -------------------------------------------------------

	void GraphicsEngine::createDescriptorPool()
    {
        std::array<VkDescriptorPoolSize, 2> poolSizes{};

		// -------------------------------------------------------

        poolSizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        poolSizes[0].descriptorCount = static_cast<uint32_t>(framesInFlight);

        // -------------------------------------------------------

        poolSizes[1].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        poolSizes[1].descriptorCount = static_cast<uint32_t>(framesInFlight);

		// -------------------------------------------------------

        VkDescriptorPoolCreateInfo poolInfo{};
        poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
        poolInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
        poolInfo.pPoolSizes = poolSizes.data();
        poolInfo.maxSets = static_cast<uint32_t>(framesInFlight);

        if (vkCreateDescriptorPool(device, &poolInfo, allocator, &descriptorPool) != VK_SUCCESS)
        {
            EDITOR_LOG_ERROR("Failed to create descriptor pool!");
            ErrMsg("failed to create descriptor pool!");
        }
    }

	void GraphicsEngine::createDescriptorSetLayout()
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

        if (vkCreateDescriptorSetLayout(device, &layoutInfo, allocator, &descriptorSetLayout) != VK_SUCCESS)
        {
            EDITOR_LOG_ERROR("Failed to create descriptor set layout!");
            ErrMsg("failed to create descriptor set layout!");
        }
    }

	void GraphicsEngine::createDescriptorSets()
    {
        std::vector<VkDescriptorSetLayout> layouts(framesInFlight, descriptorSetLayout);

		// -------------------------------------------------------

        VkDescriptorSetAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
        allocInfo.descriptorPool = descriptorPool;
        allocInfo.descriptorSetCount = static_cast<uint32_t>(framesInFlight);
        allocInfo.pSetLayouts = layouts.data();

        descriptorSets.resize(framesInFlight);
        if (vkAllocateDescriptorSets(device, &allocInfo, descriptorSets.data()) != VK_SUCCESS)
        {
            EDITOR_LOG_ERROR("Failed to allocate descriptor sets!");
            ErrMsg("Failed to allocate descriptor sets!");
        }

		// -------------------------------------------------------

        for (size_t i = 0; i < framesInFlight; i++)
        {
            VkDescriptorBufferInfo bufferInfo{};
            bufferInfo.buffer = uniformBuffers[i];
            bufferInfo.offset = 0;
            bufferInfo.range = sizeof(UniformBufferObject);

            VkDescriptorImageInfo imageInfo{};
            imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
            imageInfo.imageView = textureImageView;
            imageInfo.sampler = textureSampler;

			// -------------------------------------------------------

            std::array<VkWriteDescriptorSet, 2> descriptorWrites{};

			// -------------------------------------------------------

            descriptorWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            descriptorWrites[0].dstSet = descriptorSets[i];
            descriptorWrites[0].dstBinding = 0;
            descriptorWrites[0].dstArrayElement = 0;
            descriptorWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
            descriptorWrites[0].descriptorCount = 1;
            descriptorWrites[0].pBufferInfo = &bufferInfo;

			// -------------------------------------------------------

            descriptorWrites[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            descriptorWrites[1].dstSet = descriptorSets[i];
            descriptorWrites[1].dstBinding = 1;
            descriptorWrites[1].dstArrayElement = 0;
            descriptorWrites[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
            descriptorWrites[1].descriptorCount = 1;
            descriptorWrites[1].pImageInfo = &imageInfo;

			// -------------------------------------------------------

            vkUpdateDescriptorSets(device, static_cast<uint32_t>(descriptorWrites.size()), descriptorWrites.data(), 0, nullptr);
        }

		// -------------------------------------------------------

    }

	void GraphicsEngine::createFramebuffers()
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
            framebufferInfo.width = swapChainExtent.width;
            framebufferInfo.height = swapChainExtent.height;
            framebufferInfo.layers = 1;

            if (vkCreateFramebuffer(device, &framebufferInfo, allocator, &swapChainFramebuffers[i]) != VK_SUCCESS)
            {
                EDITOR_LOG_ERROR("Failed to create framebuffer!");
                ErrMsg("Failed to create framebuffer!");
            }
        }
    }

    void GraphicsEngine::createCommandPool()
    {
        QueueFamilyIndices queueFamilyIndices = findQueueFamilies(physicalDevice);

        VkCommandPoolCreateInfo poolInfo{};
        poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
        poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
        poolInfo.queueFamilyIndex = queueFamilyIndices.graphicsFamily.value();

        if (vkCreateCommandPool(device, &poolInfo, allocator, &commandPool) != VK_SUCCESS)
        {
            ErrMsg("Failed to create command pool!");
        }
    }

    void GraphicsEngine::createCommandBuffers()
    {
        commandBuffers.resize(framesInFlight);

        VkCommandBufferAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        allocInfo.commandPool = commandPool;
        allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        allocInfo.commandBufferCount = (uint32_t)commandBuffers.size();

        if (vkAllocateCommandBuffers(device, &allocInfo, commandBuffers.data()) != VK_SUCCESS)
        {
            ErrMsg("failed to allocate command buffers!");
        }
    }

    void GraphicsEngine::recordCommandBuffer(VkCommandBuffer commandBuffer, uint32_t imageIndex)
    {

        VkCommandBufferBeginInfo beginInfo{};
        beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

        if (vkBeginCommandBuffer(commandBuffer, &beginInfo) != VK_SUCCESS)
        {
            ErrMsg("failed to begin recording command buffer!");
        }

		// -------------------------------------------------------

        VkRenderPassBeginInfo renderPassInfo{};
        renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
        renderPassInfo.renderPass = renderPass;
        renderPassInfo.framebuffer = swapChainFramebuffers[imageIndex];
        renderPassInfo.renderArea.offset = {0, 0};
        renderPassInfo.renderArea.extent = swapChainExtent;

		// -------------------------------------------------------

        std::array<VkClearValue, 2> clearValues{};
        clearValues[0].color = {{0.0f, 0.0f, 0.0f, 1.0f}};
        clearValues[1].depthStencil = {1.0f, 0};

        renderPassInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
        renderPassInfo.pClearValues = clearValues.data();

		// -------------------------------------------------------

        vkCmdBeginRenderPass(commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

		{
			vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, graphicsPipeline);

			VkBuffer vertexBuffers[] = {vertexBuffer};
			VkDeviceSize offsets[] = {0};
			vkCmdBindVertexBuffers(commandBuffer, 0, 1, vertexBuffers, offsets);

			vkCmdBindIndexBuffer(commandBuffer, indexBuffer, 0, VK_INDEX_TYPE_UINT32);

			vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, 1, &descriptorSets[currentFrame], 0, nullptr);

			vkCmdDrawIndexed(commandBuffer, static_cast<uint32_t>(indices.size()), 1, 0, 0, 0);

			// -------------------------------------------------------

			// ImGui
            //UI guiInstance;
            //guiInstance.newFrame();

			ImGui_ImplVulkan_NewFrame();
            ImGui_ImplGlfw_NewFrame();
            ImGui::NewFrame();

            ImGui::ShowDemoWindow();

            ImGui::Render();
            ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), commandBuffer, 0);

			// -------------------------------------------------------
        }

        vkCmdEndRenderPass(commandBuffer);

        if (vkEndCommandBuffer(commandBuffer) != VK_SUCCESS)
        {
            ErrMsg("failed to record command buffer!");
        }
    }

    void GraphicsEngine::createSyncObjects()
    {
        imageAvailableSemaphores.resize(framesInFlight);
        renderFinishedSemaphores.resize(framesInFlight);
        inFlightFences.resize(framesInFlight);

        VkSemaphoreCreateInfo semaphoreInfo{};
        semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

        VkFenceCreateInfo fenceInfo{};
        fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
        fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

        for (size_t i = 0; i < framesInFlight; i++)
        {
            if (vkCreateSemaphore(device, &semaphoreInfo, allocator, &imageAvailableSemaphores[i]) != VK_SUCCESS ||
                vkCreateSemaphore(device, &semaphoreInfo, allocator, &renderFinishedSemaphores[i]) != VK_SUCCESS ||
                vkCreateFence(device, &fenceInfo, allocator, &inFlightFences[i]) != VK_SUCCESS)
            {
                ErrMsg("failed to create synchronization objects for a frame!");
            }
        }
    }

	void GraphicsEngine::recreateSwapChain()
    {
        EDITOR_LOG_INFO("Beginning swap chain recreation");

        int width = 0, height = 0;
        glfwGetFramebufferSize(window, &width, &height);
        EDITOR_LOG_INFO("Current framebuffer size: {}x{}", width, height);

        while (width == 0 || height == 0)
        {
            EDITOR_LOG_INFO("Window minimized, waiting for restore");
            glfwGetFramebufferSize(window, &width, &height);
            glfwWaitEvents();
        }

		EDITOR_LOG_INFO("Window class dimensions: {}x{}", Window::GetWidth(), Window::GetHeight());

		EDITOR_LOG_INFO("Waiting for device to be idle");
        vkDeviceWaitIdle(device);

        EDITOR_LOG_INFO("Destroying old swap chain");
        cleanupSwapChain(device, physDevice.GetSurface());

		EDITOR_LOG_INFO("Creating new swap chain with updated dimensions");
        createSwapChain();
        createImageViews();
        createRenderPass();
        createGraphicsPipeline();
        createFramebuffers();

		EDITOR_LOG_INFO("Swap chain recreation completed");

		// Reset the framebuffer resized flag
        framebufferResized = false;
    }

	void GraphicsEngine::createGraphicsPipeline()
    {
        EDITOR_LOG_INFO("Creating graphics pipeline...");

        // Set up pipeline configuration
        PipelineConfigInfo pipelineConfig{};

        // Configure dynamic states to allow viewport and scissors to be updated dynamically
        pipelineConfig.dynamicStateEnables = {VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR};

        // Set up dynamic state info
        pipelineConfig.dynamicStateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
        pipelineConfig.dynamicStateInfo.dynamicStateCount =
            static_cast<uint32_t>(pipelineConfig.dynamicStateEnables.size());
        pipelineConfig.dynamicStateInfo.pDynamicStates = pipelineConfig.dynamicStateEnables.data();
        pipelineConfig.dynamicStateInfo.flags = 0;

        // Configure viewport info
        pipelineConfig.viewportInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
        pipelineConfig.viewportInfo.viewportCount = 1;
        pipelineConfig.viewportInfo.pViewports = nullptr; // Dynamic state will set this
        pipelineConfig.viewportInfo.scissorCount = 1;
        pipelineConfig.viewportInfo.pScissors = nullptr; // Dynamic state will set this
        pipelineConfig.viewportInfo.flags = 0;

        // Configure rasterization info
        pipelineConfig.rasterizationInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
        pipelineConfig.rasterizationInfo.depthClampEnable = VK_FALSE;
        pipelineConfig.rasterizationInfo.rasterizerDiscardEnable = VK_FALSE;
        pipelineConfig.rasterizationInfo.polygonMode = VK_POLYGON_MODE_FILL;
        pipelineConfig.rasterizationInfo.lineWidth = 1.0f;
        pipelineConfig.rasterizationInfo.cullMode = VK_CULL_MODE_BACK_BIT;
        pipelineConfig.rasterizationInfo.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
        pipelineConfig.rasterizationInfo.depthBiasEnable = VK_FALSE;
        pipelineConfig.rasterizationInfo.depthBiasConstantFactor = 0.0f;
        pipelineConfig.rasterizationInfo.depthBiasClamp = 0.0f;
        pipelineConfig.rasterizationInfo.depthBiasSlopeFactor = 0.0f;

        // Configure multisampling
        pipelineConfig.multisampleInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
        pipelineConfig.multisampleInfo.sampleShadingEnable = VK_FALSE;
        pipelineConfig.multisampleInfo.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
        pipelineConfig.multisampleInfo.minSampleShading = 1.0f;
        pipelineConfig.multisampleInfo.pSampleMask = nullptr;
        pipelineConfig.multisampleInfo.alphaToCoverageEnable = VK_FALSE;
        pipelineConfig.multisampleInfo.alphaToOneEnable = VK_FALSE;

        // Configure color blend attachment
        pipelineConfig.colorBlendAttachment.colorWriteMask =
            VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
        pipelineConfig.colorBlendAttachment.blendEnable = VK_FALSE;
        pipelineConfig.colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_ONE;
        pipelineConfig.colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ZERO;
        pipelineConfig.colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;
        pipelineConfig.colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
        pipelineConfig.colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
        pipelineConfig.colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;

        // Configure color blending
        pipelineConfig.colorBlendInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
        pipelineConfig.colorBlendInfo.logicOpEnable = VK_FALSE;
        pipelineConfig.colorBlendInfo.logicOp = VK_LOGIC_OP_COPY;
        pipelineConfig.colorBlendInfo.attachmentCount = 1;
        pipelineConfig.colorBlendInfo.pAttachments = &pipelineConfig.colorBlendAttachment;
        pipelineConfig.colorBlendInfo.blendConstants[0] = 0.0f;
        pipelineConfig.colorBlendInfo.blendConstants[1] = 0.0f;
        pipelineConfig.colorBlendInfo.blendConstants[2] = 0.0f;
        pipelineConfig.colorBlendInfo.blendConstants[3] = 0.0f;

        // Configure depth-stencil state
        pipelineConfig.depthStencilInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
        pipelineConfig.depthStencilInfo.depthTestEnable = VK_TRUE;
        pipelineConfig.depthStencilInfo.depthWriteEnable = VK_TRUE;
        pipelineConfig.depthStencilInfo.depthCompareOp = VK_COMPARE_OP_LESS;
        pipelineConfig.depthStencilInfo.depthBoundsTestEnable = VK_FALSE;
        pipelineConfig.depthStencilInfo.minDepthBounds = 0.0f;
        pipelineConfig.depthStencilInfo.maxDepthBounds = 1.0f;
        pipelineConfig.depthStencilInfo.stencilTestEnable = VK_FALSE;

        // Configure input assembly
        pipelineConfig.inputAssemblyInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
        pipelineConfig.inputAssemblyInfo.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
        pipelineConfig.inputAssemblyInfo.primitiveRestartEnable = VK_FALSE;

        // Set pipeline layout and render pass
        pipelineConfig.pipelineLayout = pipelineLayout;
        pipelineConfig.renderPass = renderPass;
        pipelineConfig.subpass = 0;

        // Path to shaders - using default shaders initially
        EditorConfig config;
        std::string shaderPath(config.shaderFolder.data());
        std::string vertShaderPath = shaderPath + "/vert.spv";
        std::string fragShaderPath = shaderPath + "/frag.spv";

        EDITOR_LOG_INFO("Creating pipeline with shaders: {} and {}", vertShaderPath, fragShaderPath);

        // Create pipeline using the GFXPipeline class
        std::unique_ptr<GFXPipeline> pipeline =
            std::make_unique<GFXPipeline>(physDeviceManager, vertShaderPath, fragShaderPath, pipelineConfig);

        // Store the created graphics pipeline
        graphicsPipeline = pipeline->GetGraphicsPipeline();

        // Log success
        if (graphicsPipeline != VK_NULL_HANDLE)
        {
            EDITOR_LOG_INFO("Graphics pipeline created successfully");
        }
        else
        {
            EDITOR_LOG_ERROR("Failed to create graphics pipeline - pipeline handle is null");
        }
    }

	// -------------------------------------------------------

    void GraphicsEngine::renderFrame()
    {
        GUI guiInstance;

        vkWaitForFences(device, 1, &inFlightFences[currentFrame], VK_TRUE, UINT64_MAX);

		// -------------------------------------------------------

        uint32_t imageIndex;
        VkResult result = vkAcquireNextImageKHR(device, swapChain, UINT64_MAX, imageAvailableSemaphores[currentFrame], VK_NULL_HANDLE, &imageIndex);
        if (result == VK_ERROR_OUT_OF_DATE_KHR)
        {
            EDITOR_LOG_INFO("VK_ERROR_OUT_OF_DATE_KHR returned from vkAcquireNextImageKHR - recreating swap chain");
            recreateSwapChain();
            return;
        }
        else if (result == VK_SUBOPTIMAL_KHR)
        {
            EDITOR_LOG_INFO("VK_SUBOPTIMAL_KHR returned from vkAcquireNextImageKHR - continuing with render");
        }
        else if (result != VK_SUCCESS)
        {
            EDITOR_LOG_ERROR("Failed to acquire swap chain image: {}", ToString(result));
            ErrMsg("Failed to acquire swap chain image!");
        }

        // Check if a resize has been requested through the Window class
        if (Window::GetFramebufferResized())
        {
            EDITOR_LOG_INFO("Framebuffer resize detected from Window class");
            recreateSwapChain();
            return;
        }

        updateUniformBuffer(currentFrame);

        vkResetFences(device, 1, &inFlightFences[currentFrame]);

        vkResetCommandBuffer(commandBuffers[currentFrame], /*VkCommandBufferResetFlagBits*/ 0);
        recordCommandBuffer(commandBuffers[currentFrame], imageIndex);

		// -------------------------------------------------------

        VkSubmitInfo submitInfo{};
        submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

        VkSemaphore waitSemaphores[] = {imageAvailableSemaphores[currentFrame]};
        VkPipelineStageFlags waitStages[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
        submitInfo.waitSemaphoreCount = 1;
        submitInfo.pWaitSemaphores = waitSemaphores;
        submitInfo.pWaitDstStageMask = waitStages;
        submitInfo.commandBufferCount = 1;
        submitInfo.pCommandBuffers = &commandBuffers[currentFrame];

        VkSemaphore signalSemaphores[] = {renderFinishedSemaphores[currentFrame]};
        submitInfo.signalSemaphoreCount = 1;
        submitInfo.pSignalSemaphores = signalSemaphores;

        if (vkQueueSubmit(graphicsQueue, 1, &submitInfo, inFlightFences[currentFrame]) != VK_SUCCESS)
        {
            EDITOR_LOG_ERROR("Failed to submit draw command buffer");
            throw std::runtime_error("failed to submit draw command buffer!");
        }

		// -------------------------------------------------------

        VkPresentInfoKHR presentInfo{};
        presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
        presentInfo.waitSemaphoreCount = 1;
        presentInfo.pWaitSemaphores = signalSemaphores;

        VkSwapchainKHR swapChains[] = {swapChain};
        presentInfo.swapchainCount = 1;
        presentInfo.pSwapchains = swapChains;
        presentInfo.pImageIndices = &imageIndex;

		// -------------------------------------------------------

        result = vkQueuePresentKHR(presentQueue, &presentInfo);

        if (result == VK_ERROR_OUT_OF_DATE_KHR)
        {
            EDITOR_LOG_INFO("VK_ERROR_OUT_OF_DATE_KHR returned from vkQueuePresentKHR - recreating swap chain");
            framebufferResized = false;
            recreateSwapChain();
        }
        else if (result == VK_SUBOPTIMAL_KHR)
        {
            EDITOR_LOG_INFO("VK_SUBOPTIMAL_KHR returned from vkQueuePresentKHR - recreating swap chain");
            framebufferResized = false;
            recreateSwapChain();
        }
        else if (Window::GetFramebufferResized())
        {
            EDITOR_LOG_INFO("Window framebuffer resize flag set - recreating swap chain");
            framebufferResized = false;
            recreateSwapChain();
        }
        else if (result != VK_SUCCESS)
        {
            EDITOR_LOG_ERROR("Failed to present swap chain image: {}", ToString(result));
            ErrMsg("Failed to present swap chain image!");
        }

        currentFrame = (currentFrame + 1) % framesInFlight;
    }

	// -------------------------------------------------------

	/**
	 * @brief Updates the uniform buffer for the current frame
	 * 
	 * This method updates the model-view-projection matrices for rendering.
	 * It calculates:
	 * 1. The model matrix with rotation based on elapsed time
	 * 2. The view matrix (camera position/orientation)
	 * 3. The projection matrix with aspect ratio correction
	 * 
	 * The updated matrices are uploaded to the GPU in the uniform buffer
	 * corresponding to the current frame being rendered.
	 * 
	 * @param currentImage Index of the current frame's uniform buffer to update
	 */
	void GraphicsEngine::updateUniformBuffer(uint32_t currentImage)
    {
        static auto startTime = std::chrono::high_resolution_clock::now();

        auto currentTime = std::chrono::high_resolution_clock::now();
        float time = std::chrono::duration<float, std::chrono::seconds::period>(currentTime - startTime).count();

        UniformBufferObject ubo{};
        ubo.model = glm::rotate(glm::mat4(1.0f), time * glm::radians(90.0f), glm::vec3(0.0f, 0.0f, 1.0f));
        ubo.view =  glm::lookAt(glm::vec3(2.0f, 2.0f, 2.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f));
        ubo.proj =  glm::perspective(glm::radians(45.0f), swapChainExtent.width / (float)swapChainExtent.height, 0.1f, 10.0f);
        ubo.proj[1][1] *= -1;

        void *data;
		vkMapMemory(device, uniformBuffersMemory[currentImage], 0, sizeof(ubo), 0, &data);
			memcpy(data, &ubo, sizeof(ubo));
        vkUnmapMemory(device, uniformBuffersMemory[currentImage]);
    }

	void GraphicsEngine::recreateSurfaceFormats()
    {

        // Get the surface capabilities, formats, and present modes for the current physical device
        VkSurfaceCapabilitiesKHR capabilities;
        vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physicalDevice, physDeviceManager.GetSurface(), &capabilities);

        // Query formats
        uint32_t formatCount;
        vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, physDeviceManager.GetSurface(), &formatCount, nullptr);

        if (formatCount != 0)
        {
            availableSurfaceFormats.resize(formatCount);
            vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, physDeviceManager.GetSurface(), &formatCount, availableSurfaceFormats.data());

            EDITOR_LOG_INFO("Recreated surface formats, found {} formats", formatCount);
        }
        else
        {
            EDITOR_LOG_ERROR("Failed to find any surface formats!");
        }

        // Query present modes
        uint32_t presentModeCount;
        vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, physDeviceManager.GetSurface(), &presentModeCount, nullptr);

        if (presentModeCount != 0)
        {
            availablePresentModes.resize(presentModeCount);
            vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, physDeviceManager.GetSurface(), &presentModeCount, availablePresentModes.data());

            EDITOR_LOG_INFO("Recreated present modes, found {} modes", presentModeCount);
        }
        else
        {
            EDITOR_LOG_ERROR("Failed to find any present modes!");
        }

        // Store updated surface capabilities
        gpuDevice.surfaceCapabilities = capabilities;
    }

	void GraphicsEngine::generateMipmaps(VkImage image, VkFormat imageFormat, int32_t texWidth, int32_t texHeight, uint32_t mipLevels)
    {
        // Check if image format supports linear blitting
        VkFormatProperties formatProperties;
        vkGetPhysicalDeviceFormatProperties(physicalDevice, imageFormat, &formatProperties);

        if (!(formatProperties.optimalTilingFeatures & VK_FORMAT_FEATURE_SAMPLED_IMAGE_FILTER_LINEAR_BIT))
        {
            throw std::runtime_error("texture image format does not support linear blitting!");
        }

        VkCommandBuffer commandBuffer = beginSingleTimeCommands();

        VkImageMemoryBarrier barrier{};
        barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        barrier.image = image;
        barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        barrier.subresourceRange.baseArrayLayer = 0;
        barrier.subresourceRange.layerCount = 1;
        barrier.subresourceRange.levelCount = 1;

        int32_t mipWidth = texWidth;
        int32_t mipHeight = texHeight;

        for (uint32_t i = 1; i < mipLevels; i++)
        {
            barrier.subresourceRange.baseMipLevel = i - 1;
            barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
            barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
            barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
            barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;

            vkCmdPipelineBarrier(commandBuffer,
                                 VK_PIPELINE_STAGE_TRANSFER_BIT,
                                 VK_PIPELINE_STAGE_TRANSFER_BIT,
                                 0,
                                 0,
                                 nullptr,
                                 0,
                                 nullptr,
                                 1,
                                 &barrier);

            VkImageBlit blit{};
            blit.srcOffsets[0] = {0, 0, 0};
            blit.srcOffsets[1] = {mipWidth, mipHeight, 1};
            blit.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            blit.srcSubresource.mipLevel = i - 1;
            blit.srcSubresource.baseArrayLayer = 0;
            blit.srcSubresource.layerCount = 1;
            blit.dstOffsets[0] = {0, 0, 0};
            blit.dstOffsets[1] = {mipWidth > 1 ? mipWidth / 2 : 1, mipHeight > 1 ? mipHeight / 2 : 1, 1};
            blit.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            blit.dstSubresource.mipLevel = i;
            blit.dstSubresource.baseArrayLayer = 0;
            blit.dstSubresource.layerCount = 1;

            vkCmdBlitImage(commandBuffer,
                           image,
                           VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                           image,
                           VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                           1,
                           &blit,
                           VK_FILTER_LINEAR);

            barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
            barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
            barrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
            barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

            vkCmdPipelineBarrier(commandBuffer,
                                 VK_PIPELINE_STAGE_TRANSFER_BIT,
                                 VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
                                 0,
                                 0,
                                 nullptr,
                                 0,
                                 nullptr,
                                 1,
                                 &barrier);

            if (mipWidth > 1)
                mipWidth /= 2;
            if (mipHeight > 1)
                mipHeight /= 2;
        }

        barrier.subresourceRange.baseMipLevel = mipLevels - 1;
        barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

        vkCmdPipelineBarrier(commandBuffer,
                             VK_PIPELINE_STAGE_TRANSFER_BIT,
                             VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
                             0,
                             0,
                             nullptr,
                             0,
                             nullptr,
                             1,
                             &barrier);

        endSingleTimeCommands(commandBuffer);
    }

	void GraphicsEngine::createColorResources()
    {
        VkFormat colorFormat = swapChainImageFormat;

        createImage(swapChainExtent.width,
                    swapChainExtent.height,
                    1,
                    msaaSamples,
                    colorFormat,
                    VK_IMAGE_TILING_OPTIMAL,
                    VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
                    VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                    colorImage,
                    colorImageMemory);
        colorImageView = createImageView(colorImage, colorFormat, VK_IMAGE_ASPECT_COLOR_BIT, 1);
    }

	VkSampleCountFlagBits GraphicsEngine::getMaxUsableSampleCount()
    {
        VkPhysicalDeviceProperties physicalDeviceProperties;
        vkGetPhysicalDeviceProperties(physicalDevice, &physicalDeviceProperties);

        VkSampleCountFlags counts = physicalDeviceProperties.limits.framebufferColorSampleCounts &
                                    physicalDeviceProperties.limits.framebufferDepthSampleCounts;
        if (counts & VK_SAMPLE_COUNT_64_BIT)
        {
            return VK_SAMPLE_COUNT_64_BIT;
        }
        if (counts & VK_SAMPLE_COUNT_32_BIT)
        {
            return VK_SAMPLE_COUNT_32_BIT;
        }
        if (counts & VK_SAMPLE_COUNT_16_BIT)
        {
            return VK_SAMPLE_COUNT_16_BIT;
        }
        if (counts & VK_SAMPLE_COUNT_8_BIT)
        {
            return VK_SAMPLE_COUNT_8_BIT;
        }
        if (counts & VK_SAMPLE_COUNT_4_BIT)
        {
            return VK_SAMPLE_COUNT_4_BIT;
        }
        if (counts & VK_SAMPLE_COUNT_2_BIT)
        {
            return VK_SAMPLE_COUNT_2_BIT;
        }

        return VK_SAMPLE_COUNT_1_BIT;
    }

	// -------------------------------------------------------

	void GraphicsEngine::copyBufferToImage(VkBuffer buffer, VkImage image, uint32_t width, uint32_t height)
    {
        VkCommandBuffer commandBuffer = beginSingleTimeCommands();

        VkBufferImageCopy region{};
        region.bufferOffset = 0;
        region.bufferRowLength = 0;
        region.bufferImageHeight = 0;
        region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        region.imageSubresource.mipLevel = 0;
        region.imageSubresource.baseArrayLayer = 0;
        region.imageSubresource.layerCount = 1;
        region.imageOffset = {0, 0, 0};
        region.imageExtent = {width, height, 1};

        vkCmdCopyBufferToImage(commandBuffer, buffer, image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);

        endSingleTimeCommands(commandBuffer);
    }

	void GraphicsEngine::copyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size)
    {
        VkCommandBuffer commandBuffer = beginSingleTimeCommands();

        VkBufferCopy copyRegion{};
        copyRegion.size = size;
        vkCmdCopyBuffer(commandBuffer, srcBuffer, dstBuffer, 1, &copyRegion);

        endSingleTimeCommands(commandBuffer);
    }

	// -------------------------------------------------------

	void GraphicsEngine::createTextureImage()
    {
        // Get editor configuration
        EditorConfig config;

		std::string texturePath = config.textureFolder + "/texture.png";

        EDITOR_LOG_INFO("Loading texture shader from: {}", texturePath);

        int texWidth, texHeight, texChannels;

        stbi_uc *pixels = stbi_load(texturePath.c_str(), &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);
        VkDeviceSize imageSize = texWidth * texHeight * 4;
        mipLevels = static_cast<uint32_t>(std::floor(std::log2(std::max(texWidth, texHeight)))) + 1;

        if (!pixels)
        {
            ErrMsg("Failed to load texture image!");
        }

        VkBuffer stagingBuffer;
        VkDeviceMemory stagingBufferMemory;
        createBuffer(imageSize,
                     VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                     VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                     stagingBuffer,
                     stagingBufferMemory);

        void *data;

        vkMapMemory(device, stagingBufferMemory, 0, imageSize, 0, &data);
        if (pixels != nullptr)
        {
            memcpy(data, pixels, static_cast<size_t>(imageSize));
        }
        else
        {
            ErrMsg("Failed to load texture image: pixels is null");
        }

        vkUnmapMemory(device, stagingBufferMemory);

        stbi_image_free(pixels);

        createImage(texWidth, texHeight, mipLevels,
                    VK_SAMPLE_COUNT_1_BIT, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_TILING_OPTIMAL,
					VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                    textureImage, textureImageMemory);

        transitionImageLayout(textureImage, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, mipLevels);
        copyBufferToImage(stagingBuffer, textureImage, static_cast<uint32_t>(texWidth), static_cast<uint32_t>(texHeight));
        transitionImageLayout(textureImage, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, mipLevels);

        vkDestroyBuffer(device, stagingBuffer, allocator);
        vkFreeMemory(device, stagingBufferMemory, allocator);
    }

    void GraphicsEngine::createTextureImageView()
    {
        textureImageView = createImageView(textureImage, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_ASPECT_COLOR_BIT, 1);
    }

    void GraphicsEngine::createTextureSampler()
    {
        VkPhysicalDeviceProperties properties{};
        vkGetPhysicalDeviceProperties(physicalDevice, &properties);

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

        if (vkCreateSampler(device, &samplerInfo, allocator, &textureSampler) != VK_SUCCESS)
        {
            ErrMsg("failed to create texture sampler!");
        }
    }

	void GraphicsEngine::loadModel()
    {
        // Get editor configuration
        EditorConfig config;

        // Construct the model path using the modelFolder from config
        std::string modelPath = config.modelFolder + "/viking_room.obj";

        EDITOR_LOG_INFO("Loading 3D model from: {}", modelPath);

        tinyobj::attrib_t attrib;
        std::vector<tinyobj::shape_t> shapes;
        std::vector<tinyobj::material_t> materials;
        std::string warn, err;

        if (!tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err, modelPath.c_str()))
        {
            EDITOR_LOG_ERROR("Failed to load model: {}", modelPath);
            EDITOR_LOG_ERROR("Error details: {} {}", warn, err);
            throw std::runtime_error(warn + err);
        }

    EDITOR_LOG_INFO("Model loaded successfully: {} vertices, {} shapes", attrib.vertices.size() / 3, shapes.size());

        std::unordered_map<Vertex, uint32_t> uniqueVertices{};

        for (const auto &shape : shapes)
        {
            for (const auto &index : shape.mesh.indices)
            {
                Vertex vertex{};

                vertex.pos = {
					attrib.vertices[3 * index.vertex_index + 0],
                    attrib.vertices[3 * index.vertex_index + 1],
                    attrib.vertices[3 * index.vertex_index + 2]
				};

                // Check if the model has texture coordinates
                vertex.texCoord = {
					attrib.texcoords[2 * index.texcoord_index + 0],
					1.0f - attrib.texcoords[2 * index.texcoord_index + 1]
				};

                vertex.color = {1.0f, 1.0f, 1.0f};

                if (uniqueVertices.count(vertex) == 0)
                {
                    uniqueVertices[vertex] = static_cast<uint32_t>(vertices.size());
                    vertices.push_back(vertex);
                }

                indices.push_back(uniqueVertices[vertex]);
            }
        }

        EDITOR_LOG_INFO("Model processing complete: {} unique vertices, {} indices", vertices.size(), indices.size());
    }

	// -------------------------------------------------------

    VkShaderModule GraphicsEngine::createShaderModule(const std::vector<char> &code)
    {
        VkShaderModuleCreateInfo createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
        createInfo.codeSize = code.size();
        createInfo.pCode = reinterpret_cast<const uint32_t *>(code.data());

        VkShaderModule shaderModule;
        if (vkCreateShaderModule(device, &createInfo, allocator, &shaderModule) != VK_SUCCESS)
        {
            EDITOR_LOG_ERROR("Failed to create shader module!");
            ErrMsg("failed to create shader module!");
        }

        return shaderModule;
    }

    VkSurfaceFormatKHR GraphicsEngine::chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR> &availableFormats)
    {
        for (const auto &availableFormat : availableFormats)
        {
            if (availableFormat.format == VK_FORMAT_B8G8R8A8_SRGB &&
                availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
            {
                return availableFormat;
            }
        }

        return availableFormats[0];
    }

    VkPresentModeKHR GraphicsEngine::chooseSwapPresentMode(const std::vector<VkPresentModeKHR> &availablePresentModes)
    {
        for (const auto &availablePresentMode : availablePresentModes)
        {
            if (availablePresentMode == VK_PRESENT_MODE_MAILBOX_KHR)
            {
                return availablePresentMode;
            }
        }

        return VK_PRESENT_MODE_FIFO_KHR;
    }

    VkExtent2D GraphicsEngine::chooseSwapExtent(const VkSurfaceCapabilitiesKHR &capabilities)
    {
        // If the current extent width is the max value, it means the window manager
        // allows us to set dimensions other than the current window size
        if (capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max())
        {
            EDITOR_LOG_INFO("Using surface extent: {}x{}", capabilities.currentExtent.width, capabilities.currentExtent.height);
            return capabilities.currentExtent;
        }
        else
        {
            // Get the actual framebuffer size from GLFW directly for consistency
            int width, height;
            glfwGetFramebufferSize(window, &width, &height);

            EDITOR_LOG_INFO("Window framebuffer size: {}x{}", width, height);

            // Create the extent using the retrieved dimensions
            VkExtent2D actualExtent = {static_cast<uint32_t>(width), static_cast<uint32_t>(height)};

            // Clamp to the allowed min/max extents from the surface capabilities
            actualExtent.width = std::clamp(actualExtent.width, capabilities.minImageExtent.width, capabilities.maxImageExtent.width);
            actualExtent.height = std::clamp(actualExtent.height, capabilities.minImageExtent.height, capabilities.maxImageExtent.height);

            EDITOR_LOG_INFO("Using calculated extent: {}x{}", actualExtent.width, actualExtent.height);
            return actualExtent;
        }
    }

    SwapChainSupportDetails GraphicsEngine::querySwapChainSupport(VkPhysicalDevice device)
    {
        SwapChainSupportDetails details;
        const GPUDevice &selectedDevice = physDeviceManager.Selected();

        // Copy the data that's already available
        details.capabilities = selectedDevice.surfaceCapabilities;
        details.formats      = selectedDevice.surfaceFormats;
        details.presentModes = selectedDevice.presentModes;

        return details;
    }

	// -------------------------------------------------------

	VkCommandBuffer GraphicsEngine::beginSingleTimeCommands()
    {
        VkCommandBufferAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        allocInfo.commandPool = commandPool;
        allocInfo.commandBufferCount = 1;

        VkCommandBuffer commandBuffer;
        vkAllocateCommandBuffers(device, &allocInfo, &commandBuffer);

        VkCommandBufferBeginInfo beginInfo{};
        beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

        vkBeginCommandBuffer(commandBuffer, &beginInfo);

        return commandBuffer;
    }

	void GraphicsEngine::endSingleTimeCommands(VkCommandBuffer commandBuffer)
    {
        vkEndCommandBuffer(commandBuffer);

        VkSubmitInfo submitInfo{};
        submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        submitInfo.commandBufferCount = 1;
        submitInfo.pCommandBuffers = &commandBuffer;

        vkQueueSubmit(graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE);
        vkQueueWaitIdle(graphicsQueue);

        vkFreeCommandBuffers(device, commandPool, 1, &commandBuffer);
    }

	// -------------------------------------------------------

    uint32_t GraphicsEngine::findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties)
    {
        // Get memory properties from the physical device
        VkPhysicalDeviceMemoryProperties memProperties;
        vkGetPhysicalDeviceMemoryProperties(physicalDevice, &memProperties);

        // Find a memory type that satisfies both the type filter and the property requirements
        for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++)
        {
            if ((typeFilter & (1 << i)) && (memProperties.memoryTypes[i].propertyFlags & properties) == properties)
            {
                return i;
            }
        }

        EDITOR_LOG_ERROR("Failed to find suitable memory type!");
        ErrMsg("Failed to find suitable memory type!");
        return 0; // Return a default value to avoid undefined behavior
    }

	// -------------------------------------------------------

	bool GraphicsEngine::hasStencilComponent(VkFormat format)
    {
        return format == VK_FORMAT_D32_SFLOAT_S8_UINT || format == VK_FORMAT_D24_UNORM_S8_UINT;
    }

} // namespace SceneryEditorX

// -------------------------------------------------------
