/**
* -------------------------------------------------------
* Scenery Editor X
* -------------------------------------------------------
* Copyright (c) 2025 Thomas Ray
* Copyright (c) 2025 Coalition of Freeware Developers
* -------------------------------------------------------
* vk_device.h
* -------------------------------------------------------
* Created: 17/3/2025
* -------------------------------------------------------
*/

#pragma once
#include <vulkan/vulkan.h>
#include <GLFW/glfw3.h>

// -------------------------------------------------------

class GraphicsEngine; // Forward declaration

// -------------------------------------------------------

struct GPUDevice
{
    VkPhysicalDevice m_PhysicalDevice;
    VkPhysicalDeviceProperties DeviceInfo_;
    std::vector<VkQueueFamilyProperties> QueueFamilyInfo_;
    std::vector<VkBool32> QueueSupportPresent_;
    std::vector<VkSurfaceFormatKHR> SurfaceFormats_;
    VkSurfaceCapabilitiesKHR SurfaceCapabilities_;
    VkPhysicalDeviceMemoryProperties MemoryInfo_;
    std::vector<VkPresentModeKHR> PresentModes_;
    VkPhysicalDeviceFeatures GFXFeatures_;
    VkFormat DepthFormat_;

    GPUDevice() :
        m_PhysicalDevice(VK_NULL_HANDLE),
        DeviceInfo_({}),
        SurfaceCapabilities_({}),
        MemoryInfo_({}),
        GFXFeatures_({}),
        DepthFormat_(VK_FORMAT_UNDEFINED)
    {
    }
};

class VulkanDevice
{
public:
    VulkanDevice();
    ~VulkanDevice();

	// -----------------------------------------
    void initPhysicalDevice(const VkInstance &m_Instance, const VkSurfaceKHR &m_Surface);
    uint32_t SelectDevice(VkQueueFlags QueueType, bool SupportPresent);

    const GPUDevice &Selected() const;

    void GetGPUProps(VkPhysicalDeviceProperties &GPUprops);

    bool isValidationLayersEnabled();

    bool checkValidationLayerSupport();

    bool IsDeviceSuitable(VkPhysicalDevice device);

private:
    std::vector<GPUDevice> Devices_;
    int DeviceIndex_ = -1;

};
