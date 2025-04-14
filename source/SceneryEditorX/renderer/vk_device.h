/**
* -------------------------------------------------------
* Scenery Editor X
* -------------------------------------------------------
* Copyright (c) 2025 Thomas Ray 
* Copyright (c) 2025 Coalition of Freeware Developers
* -------------------------------------------------------
* vk_device.h
* -------------------------------------------------------
* Created: 21/3/2025
* -------------------------------------------------------
*/
#pragma once

#include <optional>
#include <SceneryEditorX/core/ref.h>
#include <vector>
#include <vulkan/vulkan.h>

// -------------------------------------------------------

namespace SceneryEditorX
{
	class CommandPool;
	
	struct GPUDevice
	{
        VkFormat depthFormat;
        VkPhysicalDevice physicalDevice;
        VkPhysicalDeviceFeatures GFXFeatures;
        VkPhysicalDeviceLimits GFXLimits;
        VkSurfaceCapabilitiesKHR surfaceCapabilities;
	    VkPhysicalDeviceProperties deviceInfo;
        VkPhysicalDeviceMemoryProperties memoryInfo;

        std::vector<VkBool32> queueSupportPresent;
        std::vector<VkPresentModeKHR> presentModes;
        std::vector<VkSurfaceFormatKHR> surfaceFormats;
	    std::vector<VkQueueFamilyProperties> queueFamilyInfo;

	    GPUDevice() :
			physicalDevice(VK_NULL_HANDLE),
			deviceInfo({}),
			surfaceCapabilities({}),
			memoryInfo({}),
			GFXFeatures({}),
            GFXLimits({}),
			depthFormat(VK_FORMAT_UNDEFINED)
	    {
	    }
	};

	struct QueueFamilyIndices
	{
        std::optional<uint32_t> graphicsFamily;
        std::optional<uint32_t> presentFamily;

		int32_t Graphics = -1;
		int32_t Compute = -1;
		int32_t Transfer = -1;

		bool IsComplete() const
        {
	        return graphicsFamily.has_value() && presentFamily.has_value();
	    }
	};

	// -------------------------------------------------------

	class VulkanPhysicalDevice : public RefCounted
    {
    public:
        explicit VulkanPhysicalDevice();
        virtual ~VulkanPhysicalDevice() override;

        uint32_t SelectDevice(VkQueueFlags queueType, bool supportPresent);

        const GPUDevice& Selected() const;
		const QueueFamilyIndices& GetQueueFamilyIndices() const { return QFamilyIndices; }
		const VkPhysicalDeviceLimits& GetLimits() const { return devices.at(deviceIndex).GFXLimits; }
        const VkPhysicalDeviceMemoryProperties& GetMemoryProperties() const { return devices.at(deviceIndex).memoryInfo; }

		VkFormat GetDepthFormat() const { return devices.at(deviceIndex).depthFormat; }
		VkPhysicalDevice GetGPUDevice() const { return devices.at(deviceIndex).physicalDevice; }
		VkPhysicalDeviceProperties GetDeviceInfo() const { return devices.at(deviceIndex).deviceInfo; }

    private:
        std::vector<GPUDevice> devices;
        std::vector<VkQueueFamilyProperties> QFamilyProperties;

        VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
        VkInstance instance;
        QueueFamilyIndices QFamilyIndices;
        QueueFamilyIndices GetQueueFamilyIndices(int qFlags) const;

        QueueFamilyIndices FindQueueFamilies(VkPhysicalDevice device) const;

        int deviceIndex = -1;

		friend class VulkanDevice;
		friend class VulkanChecks;
        friend class VulkanQueue;
    };

	// ---------------------------------------------------------

	class VulkanDevice : public RefCounted
    {
    public:
        VulkanDevice(const Ref<VulkanPhysicalDevice> &physDevice, VkPhysicalDeviceFeatures enabledFeatures);
        virtual ~VulkanDevice() override;

        void Destroy();

        const VkDevice &Selected() const;

        VkQueue GetGraphicsQueue() const { return GraphicsQueue; }
        VkQueue GetPresentQueue() const { return PresentQueue; }

		VkDevice GetDevice() const {return device;}

		const Ref<VulkanPhysicalDevice>& GetPhysicalDevice() const {return PhysicalDevice;}

		void LockQueue(bool compute = false);
        void UnlockQueue(bool compute = false);

		// -------------------------------------------------------

        VkCommandBuffer CreateSecondaryCommandBuffer(const char *debugName);
		void FlushCmdBuffer(VkCommandBuffer cmdBuffer);
		void FlushCmdBuffer(VkCommandBuffer cmdBuffer, VkQueue queue);

    private:
        VkDevice device = VK_NULL_HANDLE;
        VkInstance instance = VK_NULL_HANDLE;
        Ref<VulkanPhysicalDevice> PhysicalDevice;
        VkPhysicalDeviceFeatures EnabledFeatures;

		// -------------------------------------------------------

        VkQueue GraphicsQueue = VK_NULL_HANDLE;
        VkQueue PresentQueue = VK_NULL_HANDLE;

        std::mutex GraphicsQueueMutex;
        std::mutex ComputeQueueMutex;

		// -------------------------------------------------------

		Ref<CommandPool> GetThreadLocalCmdPool();
		Ref<CommandPool> GetOrCreateThreadLocalCmdPool();

        //bool IsDeviceSuitable(Ref<VulkanPhysicalDevice> device);

		std::map<std::thread::id, Ref<CommandPool>> CmdPools;

    };

	// ---------------------------------------------------------

	class CommandPool : public RefCounted
    {
    public:
        CommandPool();
        virtual ~CommandPool() override;

		VkCommandBuffer AllocateCommandBuffer(bool begin, bool compute = false) const;
		void FlushCmdBuffer(VkCommandBuffer cmdBuffer) const;
        void FlushCmdBuffer(VkCommandBuffer cmdBuffer, VkQueue queue) const;

		VkCommandPool GetGraphicsCmdPool() const { return GraphicsCmdPool; }
		VkCommandPool GetComputeCmdPool() const { return ComputeCmdPool; }

        Ref<VulkanDevice> GetCurrentDevice() const;

    private:

        VkCommandPool GraphicsCmdPool;
        VkCommandPool ComputeCmdPool;
	};

	// ---------------------------------------------------------

} // namespace SceneryEditorX

// -------------------------------------------------------
