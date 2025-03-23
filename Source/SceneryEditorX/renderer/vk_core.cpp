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

#include <optional>
#include <SceneryEditorX/core/window.h>
#include <SceneryEditorX/platform/windows/editor_config.hpp>
#include <SceneryEditorX/renderer/vk_core.h>
#include <SceneryEditorX/renderer/vk_util.h>
#include <stb_image.h>
#include <vector>

// -------------------------------------------------------

namespace SceneryEditorX
{
	static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
	                                                    VkDebugUtilsMessageTypeFlagsEXT messageType,
	                                                    const VkDebugUtilsMessengerCallbackDataEXT *pCallbackData,
	                                                    void *pUserData)
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

        if (enableValidationLayers)
        {
            extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
        }

        return extensions;
    }

    // -------------------------------------------------------

    void GraphicsEngine::initEngine(GLFWwindow *window, uint32_t width, uint32_t height)
    {
        this->window = window;

        createInstance();
        createDebugMessenger();
        createSurface(window);
        pickPhysicalDevice();
        createLogicalDevice();
        createSwapChain();
        createImageViews();
        createRenderPass();
        createDescriptorSetLayout();
        createGraphicsPipeline();
        createFramebuffers();
        createCommandPool();
        createTextureImage();
        createTextureImageView();
        createTextureSampler();
        createVertexBuffer();
        createIndexBuffer();
        createUniformBuffers();
        createDescriptorPool();
        createDescriptorSets();
        createCommandBuffers();
        createSyncObjects();
    }

    void GraphicsEngine::cleanup() 
    {
        DestroySwapChain();

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
        vkDestroyDevice(device, allocator);

        if (enableValidationLayers)
        {
            GraphicsEngine::DestroyDebugUtilsMessengerEXT(instance, debugMessenger, allocator);
        }

        vkDestroySurfaceKHR(instance, surface, allocator);
        vkDestroyInstance(instance, allocator);
    }

	void GraphicsEngine::DestroySwapChain()
    {
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

    VkResult GraphicsEngine::CreateDebugUtilsMessengerEXT(VkInstance instance,
                                                          const VkDebugUtilsMessengerCreateInfoEXT *pCreateInfo,
                                                          const VkAllocationCallbacks *pAllocator,
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

    void GraphicsEngine::DestroyDebugUtilsMessengerEXT(VkInstance instance,
                                                       VkDebugUtilsMessengerEXT debugMessenger,
                                                       const VkAllocationCallbacks *pAllocator)
    {
        auto func =
            (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT");
        if (func != nullptr)
        {
            func(instance, debugMessenger, pAllocator);
        }
    }

    void GraphicsEngine::createDebugMessenger()
    {
        if (!enableValidationLayers)
            return;

        VkDebugUtilsMessengerCreateInfoEXT createInfo;
        populateDebugMessengerCreateInfo(createInfo);

        if (CreateDebugUtilsMessengerEXT(instance, &createInfo, allocator, &debugMessenger) != VK_SUCCESS)
        {
            ErrMsg("Failed to set up debug messenger!");
        }
    }

    void GraphicsEngine::createSurface(GLFWwindow *glfwWindow)
    {
        if (glfwCreateWindowSurface(instance, glfwWindow, allocator, &surface) != VK_SUCCESS)
        {
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
        if (enableValidationLayers && !checkValidationLayerSupport())
        {
            ErrMsg("validation layers requested, but not available!");
        }

        uint32_t layerCount = 0;
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

        // get api version
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
        if (enableValidationLayers)
        {
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
            ErrMsg("failed to create instance!");
        }
    }

    /**
	 * @brief - Check if a device is suitable for rendering.
	 * 
	 * @param device - The Vulkan physical device.
	 * @return - bool True if the device is suitable, false otherwise.
	 */
    bool GraphicsEngine::isDeviceCompatible(VkPhysicalDevice device)
    {
        VkPhysicalDeviceFeatures deviceFeatures;
        VkPhysicalDeviceProperties deviceProperties;
        vkGetPhysicalDeviceFeatures(device, &deviceFeatures);
        vkGetPhysicalDeviceProperties(device, &deviceProperties);

        // Check for required features and properties
        bool isSuitable = (deviceProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU) &&
                          (deviceFeatures.geometryShader) && (deviceFeatures.tessellationShader);

        if (!isSuitable)
        {
            EDITOR_LOG_ERROR("Vulkan: Device does not meet required features or is not discrete GPU");
            ErrMsg("Vulkan: Device does not meet required features or is not discrete GPU");
            return false;
        }

        return true;
    }

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
	    
	    EDITOR_LOG_INFO("Selected physical device: {}", ToString(selectedDevice.deviceInfo.deviceName));
	    
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

    void GraphicsEngine::createLogicalDevice()
    {
        QueueFamilyIndices indices = findQueueFamilies(physicalDevice);

        // Handle potentially separate queues for graphics and presentation
        std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
        std::set<uint32_t> uniqueQueueFamilies = {indices.graphicsFamily.value(), indices.presentFamily.value()};

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

		if (!checkDeviceExtensionSupport(physicalDevice))
        {
            EDITOR_LOG_ERROR("Required device extensions not supported!");
            ErrMsg("Required device extensions not supported!");
        }

        // Enable swap chain extension
        createInfo.enabledExtensionCount = static_cast<uint32_t>(deviceExtensions.size());
        createInfo.ppEnabledExtensionNames = deviceExtensions.data();

        if (enableValidationLayers)
        {
            createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
            createInfo.ppEnabledLayerNames = validationLayers.data();
        }
        else
        {
            createInfo.enabledLayerCount = 0;
        }

        if (vkCreateDevice(physicalDevice, &createInfo, allocator, &device) != VK_SUCCESS)
        {
            EDITOR_LOG_ERROR("Failed to create logical device!");
            ErrMsg("Failed to create logical device!");
        }

        vkGetDeviceQueue(device, indices.graphicsFamily.value(), 0, &graphicsQueue);
        // Get presentation queue
        vkGetDeviceQueue(device, indices.presentFamily.value(), 0, &presentQueue);
    }

    void GraphicsEngine::createSwapChain()
    {
        // Get swap chain support details from the selected device
        const GPUDevice &selectedDevice = physDeviceManager.Selected();

        // Create our swap chain using the capabilities from the selected device
        VkSwapchainCreateInfoKHR createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
        createInfo.surface = surface;

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
            swapChainImageViews[i] = GraphicsEngine::createImageView(swapChainImages[i], swapChainImageFormat);
        }
    }

	void GraphicsEngine::createImage(uint32_t width, uint32_t height, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage,
					 VkMemoryPropertyFlags properties, VkImage& image, VkDeviceMemory& imageMemory)
	{
        VkImageCreateInfo imageInfo{};
        imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
        imageInfo.imageType = VK_IMAGE_TYPE_2D;
        imageInfo.extent.width = width;
        imageInfo.extent.height = height;
        imageInfo.extent.depth = 1;
        imageInfo.mipLevels = 1;
        imageInfo.arrayLayers = 1;
        imageInfo.format = format;
        imageInfo.tiling = tiling;
        imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        imageInfo.usage = usage;
        imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
        imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

        if (vkCreateImage(device, &imageInfo, nullptr, &image) != VK_SUCCESS) {
            ErrMsg("failed to create image!");
        }

        VkMemoryRequirements memRequirements;
        vkGetImageMemoryRequirements(device, image, &memRequirements);

        VkMemoryAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        allocInfo.allocationSize = memRequirements.size;
        allocInfo.memoryTypeIndex = findMemoryType(memRequirements.memoryTypeBits, properties);

        if (vkAllocateMemory(device, &allocInfo, nullptr, &imageMemory) != VK_SUCCESS) {
            ErrMsg("failed to allocate image memory!");
        }

        vkBindImageMemory(device, image, imageMemory, 0);
    }

    VkImageView GraphicsEngine::createImageView(VkImage image, VkFormat format)
    {
        VkImageViewCreateInfo viewInfo{};
        viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        viewInfo.image = image;
        viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
        viewInfo.format = format;
        viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        viewInfo.subresourceRange.baseMipLevel = 0;
        viewInfo.subresourceRange.levelCount = 1;
        viewInfo.subresourceRange.baseArrayLayer = 0;
        viewInfo.subresourceRange.layerCount = 1;

        VkImageView imageView;
        if (vkCreateImageView(device, &viewInfo, nullptr, &imageView) != VK_SUCCESS)
        {
            ErrMsg("failed to create texture image view!");
        }

        return imageView;
    }

	void GraphicsEngine::transitionImageLayout(VkImage image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout)
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
        barrier.subresourceRange.levelCount = 1;
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
            throw std::invalid_argument("unsupported layout transition!");
        }

        vkCmdPipelineBarrier(commandBuffer, sourceStage, destinationStage, 0, 0, nullptr, 0, nullptr, 1, &barrier);

        endSingleTimeCommands(commandBuffer);
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

	void GraphicsEngine::recreateSurfaceFormats()
    {
        //auto surface = _ctx.surface;

	}

    static std::vector<char> readFile(const std::string &filename)
    {
        std::ifstream file(filename, std::ios::ate | std::ios::binary);

        if (!file.is_open())
        {
            EDITOR_LOG_ERROR("Failed to open file: {}", filename);
            ErrMsg(std::string("Failed to open file: ") + filename);
            return {}; // Return empty vector on failure
        }

        size_t fileSize = static_cast<size_t>(file.tellg());
        if (fileSize == 0)
        {
            EDITOR_LOG_ERROR("File is empty: {}", filename);
            return {};
        }

        std::vector<char> buffer(fileSize);

        file.seekg(0);
        file.read(buffer.data(), fileSize);

        if (!file)
        {
            EDITOR_LOG_ERROR("Failed to read entire file: {}", filename);
            ErrMsg(std::string("Failed to read entire file: ") + filename);
            return {};
        }

        file.close();

        EDITOR_LOG_INFO("Successfully read file: {} ({} bytes)", filename, fileSize);
        return buffer;
    }

	void GraphicsEngine::createRenderPass()
    {
        VkAttachmentDescription colorAttachment{};
        colorAttachment.format = swapChainImageFormat;
        colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
        colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

        VkAttachmentReference colorAttachmentRef{};
        colorAttachmentRef.attachment = 0;
        colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

        VkSubpassDescription subpass{};
        subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
        subpass.colorAttachmentCount = 1;
        subpass.pColorAttachments = &colorAttachmentRef;

        VkSubpassDependency dependency{};
        dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
        dependency.dstSubpass = 0;
        dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        dependency.srcAccessMask = 0;
        dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

        VkRenderPassCreateInfo renderPassInfo{};
        renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
        renderPassInfo.attachmentCount = 1;
        renderPassInfo.pAttachments = &colorAttachment;
        renderPassInfo.subpassCount = 1;
        renderPassInfo.pSubpasses = &subpass;
        renderPassInfo.dependencyCount = 1;
        renderPassInfo.pDependencies = &dependency;

        if (vkCreateRenderPass(device, &renderPassInfo, allocator, &renderPass) != VK_SUCCESS)
        {
            ErrMsg("failed to create render pass!");
        }
    }

	// -------------------------------------------------------

	void GraphicsEngine::createBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer &buffer, VkDeviceMemory &bufferMemory)
    {
        VkBufferCreateInfo bufferInfo{};
        bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        bufferInfo.size = size;
        bufferInfo.usage = usage;
        bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

        if (vkCreateBuffer(device, &bufferInfo, nullptr, &buffer) != VK_SUCCESS)
        {
            ErrMsg("failed to create buffer!");
        }

        VkMemoryRequirements memRequirements;
        vkGetBufferMemoryRequirements(device, buffer, &memRequirements);

        VkMemoryAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        allocInfo.allocationSize = memRequirements.size;
        allocInfo.memoryTypeIndex = findMemoryType(memRequirements.memoryTypeBits, properties);

        if (vkAllocateMemory(device, &allocInfo, nullptr, &bufferMemory) != VK_SUCCESS)
        {
            ErrMsg("failed to allocate buffer memory!");
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

        vkDestroyBuffer(device, stagingBuffer, nullptr);
        vkFreeMemory(device, stagingBufferMemory, nullptr);
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

        vkDestroyBuffer(device, stagingBuffer, nullptr);
        vkFreeMemory(device, stagingBufferMemory, nullptr);
    }

	// -------------------------------------------------------

    void GraphicsEngine::createUniformBuffers()
    {
        VkDeviceSize bufferSize = sizeof(UniformBufferObject);

        uniformBuffers.resize(framesInFlight);
        uniformBuffersMemory.resize(framesInFlight);

        for (size_t i = 0; i < framesInFlight; i++)
        {
            createBuffer(bufferSize,
                         VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                         VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                         uniformBuffers[i],
                         uniformBuffersMemory[i]);
        }
    }

	void GraphicsEngine::createDescriptorPool()
    {
        VkDescriptorPoolSize poolSize{};
        poolSize.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        poolSize.descriptorCount = static_cast<uint32_t>(framesInFlight);

        VkDescriptorPoolCreateInfo poolInfo{};
        poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
        poolInfo.poolSizeCount = 1;
        poolInfo.pPoolSizes = &poolSize;
        poolInfo.maxSets = static_cast<uint32_t>(framesInFlight);

        if (vkCreateDescriptorPool(device, &poolInfo, allocator, &descriptorPool) != VK_SUCCESS)
        {
            ErrMsg("failed to create descriptor pool!");
        }
    }

	void GraphicsEngine::createDescriptorSetLayout()
    {
        VkDescriptorSetLayoutBinding uboLayoutBinding{};
        uboLayoutBinding.binding = 0;
        uboLayoutBinding.descriptorCount = 1;
        uboLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        uboLayoutBinding.pImmutableSamplers = nullptr;
        uboLayoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

        VkDescriptorSetLayoutCreateInfo layoutInfo{};
        layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
        layoutInfo.bindingCount = 1;
        layoutInfo.pBindings = &uboLayoutBinding;

        if (vkCreateDescriptorSetLayout(device, &layoutInfo, allocator, &descriptorSetLayout) != VK_SUCCESS)
        {
            ErrMsg("failed to create descriptor set layout!");
        }
    }

	void GraphicsEngine::createDescriptorSets()
    {
        std::vector<VkDescriptorSetLayout> layouts(framesInFlight, descriptorSetLayout);
        VkDescriptorSetAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
        allocInfo.descriptorPool = descriptorPool;
        allocInfo.descriptorSetCount = static_cast<uint32_t>(framesInFlight);
        allocInfo.pSetLayouts = layouts.data();

        descriptorSets.resize(framesInFlight);
        if (vkAllocateDescriptorSets(device, &allocInfo, descriptorSets.data()) != VK_SUCCESS)
        {
            throw std::runtime_error("failed to allocate descriptor sets!");
        }

        for (size_t i = 0; i < framesInFlight; i++)
        {
            VkDescriptorBufferInfo bufferInfo{};
            bufferInfo.buffer = uniformBuffers[i];
            bufferInfo.offset = 0;
            bufferInfo.range = sizeof(UniformBufferObject);

            VkWriteDescriptorSet descriptorWrite{};
            descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            descriptorWrite.dstSet = descriptorSets[i];
            descriptorWrite.dstBinding = 0;
            descriptorWrite.dstArrayElement = 0;
            descriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
            descriptorWrite.descriptorCount = 1;
            descriptorWrite.pBufferInfo = &bufferInfo;

            vkUpdateDescriptorSets(device, 1, &descriptorWrite, 0, nullptr);
        }
    }

    void GraphicsEngine::createGraphicsPipeline()
    {
        // Get editor configuration
        EditorConfig config;

		std::string shaderPath(config.shaderFolder.data());

        std::string vertShaderPath = shaderPath + "/vert.spv";
        std::string fragShaderPath = shaderPath + "/frag.spv";

        EDITOR_LOG_INFO("Loading vertex shader from: {}", vertShaderPath);
        EDITOR_LOG_INFO("Loading fragment shader from: {}", fragShaderPath);

		// -------------------------------------------------------

        auto vertShaderCode = readFile(vertShaderPath);
        auto fragShaderCode = readFile(fragShaderPath);

		VkShaderModule vertShaderModule = createShaderModule(vertShaderCode);
        VkShaderModule fragShaderModule = createShaderModule(fragShaderCode);

		// -------------------------------------------------------

        VkPipelineShaderStageCreateInfo vertShaderStageInfo{};
        vertShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        vertShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
        vertShaderStageInfo.module = vertShaderModule;
        vertShaderStageInfo.pName = "main";

        VkPipelineShaderStageCreateInfo fragShaderStageInfo{};
        fragShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        fragShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
        fragShaderStageInfo.module = fragShaderModule;
        fragShaderStageInfo.pName = "main";

        VkPipelineShaderStageCreateInfo shaderStages[] = {vertShaderStageInfo, fragShaderStageInfo};

		// Configure vertex input
		VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
        vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;

        auto bindingDescription = Vertex::getBindingDescription();
        auto attributeDescriptions = Vertex::getAttributeDescriptions();

        vertexInputInfo.vertexBindingDescriptionCount = 1;
        vertexInputInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(attributeDescriptions.size());
        vertexInputInfo.pVertexBindingDescriptions = &bindingDescription;
        vertexInputInfo.pVertexAttributeDescriptions = attributeDescriptions.data();

		// Configure input assembly
		VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
        inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
        inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
        inputAssembly.primitiveRestartEnable = VK_FALSE;

        // Configure viewport and scissor
        VkViewport viewport{};
        viewport.x = 0.0f;
        viewport.y = 0.0f;
        viewport.width = (float)swapChainExtent.width;
        viewport.height = (float)swapChainExtent.height;
        viewport.minDepth = 0.0f;
        viewport.maxDepth = 1.0f;

        VkRect2D scissor{};
        scissor.offset = {0, 0};
        scissor.extent = swapChainExtent;

        VkPipelineViewportStateCreateInfo viewportState{};
        viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
        viewportState.viewportCount = 1;
        viewportState.pViewports = &viewport;
        viewportState.scissorCount = 1;
        viewportState.pScissors = &scissor;

        // Configure rasterization
        VkPipelineRasterizationStateCreateInfo rasterizer{};
        rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
        rasterizer.depthClampEnable = VK_FALSE;
        rasterizer.rasterizerDiscardEnable = VK_FALSE;
        rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
        rasterizer.lineWidth = 1.0f;
        rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;
        rasterizer.frontFace = VK_FRONT_FACE_CLOCKWISE;
        rasterizer.depthBiasEnable = VK_FALSE;

        // Configure multisampling
        VkPipelineMultisampleStateCreateInfo multisampling{};
        multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
        multisampling.sampleShadingEnable = VK_FALSE;
        multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

        // Configure color blending
        VkPipelineColorBlendAttachmentState colorBlendAttachment{};
        colorBlendAttachment.colorWriteMask =
            VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
        colorBlendAttachment.blendEnable = VK_FALSE;

        VkPipelineColorBlendStateCreateInfo colorBlending{};
        colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
        colorBlending.logicOpEnable = VK_FALSE;
        colorBlending.logicOp = VK_LOGIC_OP_COPY;
        colorBlending.attachmentCount = 1;
        colorBlending.pAttachments = &colorBlendAttachment;
        colorBlending.blendConstants[0] = 0.0f;
        colorBlending.blendConstants[1] = 0.0f;
        colorBlending.blendConstants[2] = 0.0f;
        colorBlending.blendConstants[3] = 0.0f;

        // Create the graphics pipeline
        VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
        pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
        pipelineLayoutInfo.setLayoutCount = 1;
        pipelineLayoutInfo.pSetLayouts = &descriptorSetLayout;

        if (vkCreatePipelineLayout(device, &pipelineLayoutInfo, allocator, &pipelineLayout) != VK_SUCCESS)
        {
            ErrMsg("failed to create pipeline layout!");
        }

        VkGraphicsPipelineCreateInfo pipelineInfo{};
        pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
        pipelineInfo.stageCount = 2;
        pipelineInfo.pStages = shaderStages;
        pipelineInfo.pVertexInputState = &vertexInputInfo;
        pipelineInfo.pInputAssemblyState = &inputAssembly;
        pipelineInfo.pViewportState = &viewportState;
        pipelineInfo.pRasterizationState = &rasterizer;
        pipelineInfo.pMultisampleState = &multisampling;
        pipelineInfo.pColorBlendState = &colorBlending;
        pipelineInfo.layout = pipelineLayout;
        pipelineInfo.renderPass = renderPass;
        pipelineInfo.subpass = 0;
        pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;

        if (vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &graphicsPipeline) !=
            VK_SUCCESS)
        {
            ErrMsg("failed to create graphics pipeline!");
        }

        vkDestroyShaderModule(device, fragShaderModule, nullptr);
        vkDestroyShaderModule(device, vertShaderModule, nullptr);
    }

	void GraphicsEngine::createFramebuffers()
    {
        swapChainFramebuffers.resize(swapChainImageViews.size());

        for (size_t i = 0; i < swapChainImageViews.size(); i++)
        {
            VkImageView attachments[] = {swapChainImageViews[i]};

            VkFramebufferCreateInfo framebufferInfo{};
            framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
            framebufferInfo.renderPass = renderPass;
            framebufferInfo.attachmentCount = 1;
            framebufferInfo.pAttachments = attachments;
            framebufferInfo.width = swapChainExtent.width;
            framebufferInfo.height = swapChainExtent.height;
            framebufferInfo.layers = 1;

            if (vkCreateFramebuffer(device, &framebufferInfo, allocator, &swapChainFramebuffers[i]) != VK_SUCCESS)
            {
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

        if (vkBeginCommandBuffer(commandBuffer, &beginInfo) != VK_SUCCESS)
        {
            ErrMsg("failed to begin recording command buffer!");
        }

        VkRenderPassBeginInfo renderPassInfo{};
        renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
        renderPassInfo.renderPass = renderPass;
        renderPassInfo.framebuffer = swapChainFramebuffers[imageIndex];
        renderPassInfo.renderArea.offset = {0, 0};
        renderPassInfo.renderArea.extent = swapChainExtent;

        VkClearValue clearColor = {{{0.0f, 0.0f, 0.0f, 1.0f}}};
        renderPassInfo.clearValueCount = 1;
        renderPassInfo.pClearValues = &clearColor;

        vkCmdBeginRenderPass(commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

        vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, graphicsPipeline);

        VkBuffer vertexBuffers[] = {vertexBuffer};
        VkDeviceSize offsets[] = {0};
        vkCmdBindVertexBuffers(commandBuffer, 0, 1, vertexBuffers, offsets);

        vkCmdBindIndexBuffer(commandBuffer, indexBuffer, 0, VK_INDEX_TYPE_UINT16);

        vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, 1, &descriptorSets[currentFrame], 0, nullptr);

        vkCmdDrawIndexed(commandBuffer, static_cast<uint32_t>(indices.size()), 1, 0, 0, 0);

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
            if (vkCreateSemaphore(device, &semaphoreInfo, nullptr, &imageAvailableSemaphores[i]) != VK_SUCCESS ||
                vkCreateSemaphore(device, &semaphoreInfo, nullptr, &renderFinishedSemaphores[i]) != VK_SUCCESS ||
                vkCreateFence(device, &fenceInfo, nullptr, &inFlightFences[i]) != VK_SUCCESS)
            {
                ErrMsg("failed to create synchronization objects for a frame!");
            }
        }
    }

	void GraphicsEngine::recreateSwapChain()
    {
        int width = 0, height = 0;
        glfwGetFramebufferSize(window, &width, &height);
        while (width == 0 || height == 0)
        {
            glfwGetFramebufferSize(window, &width, &height);
            glfwWaitEvents();
        }

        vkDeviceWaitIdle(device);

        DestroySwapChain();

        createSwapChain();
        createImageViews();
        createRenderPass();
        createGraphicsPipeline();
        createFramebuffers();
    }

    void GraphicsEngine::renderFrame()
    {
        vkWaitForFences(device, 1, &inFlightFences[currentFrame], VK_TRUE, UINT64_MAX);

        uint32_t imageIndex;
        VkResult result = vkAcquireNextImageKHR(device,
                                                swapChain,
                                                UINT64_MAX,
                                                imageAvailableSemaphores[currentFrame],
                                                VK_NULL_HANDLE,
                                                &imageIndex);

        if (result == VK_ERROR_OUT_OF_DATE_KHR)
        {
            recreateSwapChain();
            return;
        }
        else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR)
        {
            throw std::runtime_error("failed to acquire swap chain image!");
        }

        updateUniformBuffer(currentFrame);

        vkResetFences(device, 1, &inFlightFences[currentFrame]);

        vkResetCommandBuffer(commandBuffers[currentFrame], /*VkCommandBufferResetFlagBits*/ 0);
        recordCommandBuffer(commandBuffers[currentFrame], imageIndex);

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
            throw std::runtime_error("failed to submit draw command buffer!");
        }

        VkPresentInfoKHR presentInfo{};
        presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;

        presentInfo.waitSemaphoreCount = 1;
        presentInfo.pWaitSemaphores = signalSemaphores;

        VkSwapchainKHR swapChains[] = {swapChain};
        presentInfo.swapchainCount = 1;
        presentInfo.pSwapchains = swapChains;

        presentInfo.pImageIndices = &imageIndex;

        result = vkQueuePresentKHR(presentQueue, &presentInfo);

        if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR || framebufferResized)
        {
            framebufferResized = false;
            recreateSwapChain();
        }
        else if (result != VK_SUCCESS)
        {
            ErrMsg("Failed to present swap chain image!");
        }

        currentFrame = (currentFrame + 1) % framesInFlight;
    }

	void GraphicsEngine::updateUniformBuffer(uint32_t currentImage)
    {
        static auto startTime = std::chrono::high_resolution_clock::now();

        auto currentTime = std::chrono::high_resolution_clock::now();
        float time = std::chrono::duration<float, std::chrono::seconds::period>(currentTime - startTime).count();

        UniformBufferObject ubo{};
        ubo.model = glm::rotate(glm::mat4(1.0f), time * glm::radians(90.0f), glm::vec3(0.0f, 0.0f, 1.0f));
        ubo.view = glm::lookAt(glm::vec3(2.0f, 2.0f, 2.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f));
        ubo.proj =
            glm::perspective(glm::radians(45.0f), swapChainExtent.width / (float)swapChainExtent.height, 0.1f, 10.0f);
        ubo.proj[1][1] *= -1;

        void *data;
        vkMapMemory(device, uniformBuffersMemory[currentImage], 0, sizeof(ubo), 0, &data);
        memcpy(data, &ubo, sizeof(ubo));
        vkUnmapMemory(device, uniformBuffersMemory[currentImage]);
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

        createImage(texWidth,
                    texHeight,
                    VK_FORMAT_R8G8B8A8_SRGB,
                    VK_IMAGE_TILING_OPTIMAL,
                    VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
                    VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                    textureImage,
                    textureImageMemory);

        transitionImageLayout(textureImage,
                              VK_FORMAT_R8G8B8A8_SRGB,
                              VK_IMAGE_LAYOUT_UNDEFINED,
                              VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
        copyBufferToImage(stagingBuffer,
                          textureImage,
                          static_cast<uint32_t>(texWidth),
                          static_cast<uint32_t>(texHeight));
        transitionImageLayout(textureImage,
                              VK_FORMAT_R8G8B8A8_SRGB,
                              VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                              VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

        vkDestroyBuffer(device, stagingBuffer, allocator);
        vkFreeMemory(device, stagingBufferMemory, allocator);
    }

    void GraphicsEngine::createTextureImageView()
    {
        textureImageView = createImageView(textureImage, VK_FORMAT_R8G8B8A8_SRGB);
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

        if (vkCreateSampler(device, &samplerInfo, nullptr, &textureSampler) != VK_SUCCESS)
        {
            ErrMsg("failed to create texture sampler!");
        }
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
        if (capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max())
        {
            return capabilities.currentExtent;
        }
        else
        {
            this->window = window;
            int width, height;

            // Check if the Window class has been properly initialized
            if (Window::GetGLFWwindow() == nullptr)
            {
                // Fall back to using raw GLFW calls if Window isn't fully initialized
                glfwGetFramebufferSize(this->window, &width, &height);
                EDITOR_LOG_WARN("Using direct GLFW calls for framebuffer size as Window class isn't initialized");
            }
            else
            {
                // Use the Window class's functionality
                Window::UpdateFramebufferSize();
                width = Window::GetWidth();
                height = Window::GetHeight();
            }

            VkExtent2D actualExtent = {static_cast<uint32_t>(width), static_cast<uint32_t>(height)};

            actualExtent.width =
                std::clamp(actualExtent.width, capabilities.minImageExtent.width, capabilities.maxImageExtent.width);
            actualExtent.height =
                std::clamp(actualExtent.height, capabilities.minImageExtent.height, capabilities.maxImageExtent.height);

            return actualExtent;
        }
    }

    SwapChainSupportDetails GraphicsEngine::querySwapChainSupport(VkPhysicalDevice device)
    {
        SwapChainSupportDetails details;
        const GPUDevice &selectedDevice = physDeviceManager.Selected();

        // Copy the data that's already available
        details.capabilities = selectedDevice.surfaceCapabilities;
        details.formats = selectedDevice.surfaceFormats;
        details.presentModes = selectedDevice.presentModes;

        return details;
    }

    bool GraphicsEngine::isDeviceSuitable(VkPhysicalDevice device)
    {
        QueueFamilyIndices indices = findQueueFamilies(device);

		bool extensionsSupported = checkDeviceExtensionSupport(device);
        bool swapChainAdequate = false;
        if (extensionsSupported)
        {
            SwapChainSupportDetails swapChainSupport = querySwapChainSupport(device);
            swapChainAdequate = !swapChainSupport.formats.empty() && !swapChainSupport.presentModes.empty();
        }

        return indices.isComplete() && extensionsSupported && swapChainAdequate;
    }

    bool GraphicsEngine::checkDeviceExtensionSupport(VkPhysicalDevice device)
    {
        uint32_t extensionCount;
        vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, nullptr);

        std::vector<VkExtensionProperties> availableExtensions(extensionCount);
        vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, availableExtensions.data());

        std::set<std::string> requiredExtensions(deviceExtensions.begin(), deviceExtensions.end());

        EDITOR_LOG_INFO("Checking for required device extensions:");
        for (const auto &extension : deviceExtensions)
        {
            EDITOR_LOG_INFO("  Required: {}", ToString(extension));
        }

        EDITOR_LOG_INFO("Available device extensions:");
        for (const auto &extension : availableExtensions)
        {
            EDITOR_LOG_INFO("  Available: {}", ToString(extension.extensionName));
            requiredExtensions.erase(extension.extensionName);
        }

        if (!requiredExtensions.empty())
        {
            EDITOR_LOG_ERROR("Missing extensions:");
            for (const auto &ext : requiredExtensions)
            {
                EDITOR_LOG_ERROR("  Missing: {}", ToString(ext));
            }
            return false;
        }

        return true;
    }

    QueueFamilyIndices GraphicsEngine::findQueueFamilies(VkPhysicalDevice device)
    {
        QueueFamilyIndices indices;

        // Instead of querying again, use the data we already have
        const GPUDevice &selectedDevice = physDeviceManager.Selected();

        for (uint32_t i = 0; i < selectedDevice.queueFamilyInfo.size(); i++)
        {
            const VkQueueFamilyProperties &queueFamily = selectedDevice.queueFamilyInfo[i];

            if (queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT)
            {
                indices.graphicsFamily = i;
            }

            // Check presentation support
            if (selectedDevice.queueSupportPresent[i])
            {
                indices.presentFamily = i;
            }

            if (indices.isComplete())
            {
                break;
            }
        }

        return indices;
    }

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

    bool GraphicsEngine::checkValidationLayerSupport()
    {
        uint32_t layerCount;
        vkEnumerateInstanceLayerProperties(&layerCount, nullptr);

        std::vector<VkLayerProperties> availableLayers(layerCount);
        vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());

        for (const char *layerName : validationLayers)
        {
            bool layerFound = false;

            for (const auto &layerProperties : availableLayers)
            {
                if (strcmp(layerName, layerProperties.layerName) == 0)
                {
                    layerFound = true;
                    break;
                }
            }

            if (!layerFound)
            {
                return false;
            }
        }

        return true;
    }

} // namespace SceneryEditorX

// -------------------------------------------------------
