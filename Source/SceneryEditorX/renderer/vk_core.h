/**
* -------------------------------------------------------
* Scenery Editor X
* -------------------------------------------------------
* Copyright (c) 2025 Thomas Ray 
* Copyright (c) 2025 Coalition of Freeware Developers
* -------------------------------------------------------
* vk_core.h
* -------------------------------------------------------
* Created: 17/3/2025
* -------------------------------------------------------
*/

#pragma once

#define VMA_IMPLEMENTATION

#include <vulkan/vulkan.h>
#include <vk_mem_alloc.h>
#include <SceneryEditorX/renderer/vk_device.h>
#include <SceneryEditorX/core/window.h>


// -------------------------------------------------------

class GraphicsEngine
{
public:
    GraphicsEngine(GLFWwindow &window);
	~GraphicsEngine();

	// -----------------------------------------

    void initEngine();


private:
    // -----------------------------------------
    void create_instance(GLFWwindow *window);
	void create_debug_callback();
    void create_device();
    void create_logic_device();
	void create_destruction_handler();
	void create_command_pool();
	void create_sync_objects();
	void create_descriptors();

	// -----------------------------------------
    void destroy_instance();

    // -----------------------------------------

	VkApplicationInfo m_AppInfo;
	VkInstanceCreateInfo m_InstanceInfo;
	VkDebugUtilsMessengerEXT m_DebugMessenger;

    VkDevice g_Device = VK_NULL_HANDLE;

	VkInstance m_Instance = VK_NULL_HANDLE;
	VkSurfaceKHR m_Surface = VK_NULL_HANDLE;
    //VulkanDevice m_PhysicalDevice;
    VkPhysicalDevice g_PhysicalDevice = VK_NULL_HANDLE;
	//VkDevice m_Device = VK_NULL_HANDLE;
    VkSampleCountFlagBits maxSamples = VK_SAMPLE_COUNT_1_BIT;
    VkSampleCountFlags sampleCounts;

    VkAllocationCallbacks *allocator = VK_NULL_HANDLE;
    VmaAllocator vmaAllocator;
    VkPhysicalDeviceMemoryProperties memoryProperties;

    Scope<VkDevice> m_Device;
    Scope<VkCommandPool> CmdBuffPool_;
    Scope<VkCommandBuffer> CpyCmdBuff;

    Ref<VulkanDevice> m_PhysicalDevice;
    std::vector<Ref<VkImage>> Images;
    std::vector<Ref<VkImageView>> ImageViews;

    uint32_t apiVersion;
    std::vector<bool> activeLayers;
    std::vector<const char *> activeLayersNames;
    std::vector<VkLayerProperties> layers;
    std::vector<bool> activeExtensions;
    std::vector<const char *> activeExtensionsNames;
    std::vector<VkExtensionProperties> instanceExtensions;

    bool enableValidationLayers = true;

	uint32_t m_QueueFamily = 0;
	VkSurfaceFormatKHR m_SurfaceFormat = {};
    std::vector<int32_t> availableBufferRID;
    std::vector<int32_t> availableImageRID;
    std::vector<int32_t> availableTLASRID;
    VkSampler genericSampler;

	//std::vector<VkImage> Images;
	//std::vector<VkImageView> ImageViews;

	VkQueue m_GraphicsQueue;
	VkQueue m_PresentQueue;

    VkPhysicalDeviceFeatures physicalFeatures{};
    VkSurfaceCapabilitiesKHR surfaceCapabilities{};
    VkPhysicalDeviceProperties physicalProperties{};

    std::vector<VkPresentModeKHR> availablePresentModes;
    std::vector<VkSurfaceFormatKHR> availableSurfaceFormats;
    std::vector<VkExtensionProperties> availableExtensions;
    std::vector<VkQueueFamilyProperties> availableFamilies;

	//VkCommandPool CmdBuffPool_ = VK_NULL_HANDLE;
	//VkCommandBuffer CpyCmdBuff = VK_NULL_HANDLE;
	//RenderQueue ImgQueue;

	// -----------------------------------------

protected:
	GLFWwindow& m_Window;
	uint32_t m_CurrentImageIndex = 0;
	uint32_t m_CurrentFrame = 0;
	uint64_t m_FrameCount = 0;
    VkSampler CreateSampler(float maxLod);
};
