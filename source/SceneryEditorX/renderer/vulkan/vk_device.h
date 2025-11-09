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
#include "vk_allocator.h"
#include "vk_data.h"
#include <optional>
#include <vulkan/vulkan.h>
// PCH normally provides these in AppCore; include here for standalone TUs (tests)
#include <map>
#include <mutex>
#include <string>
#include <thread>
#include <unordered_set>

// -------------------------------------------------------

namespace SceneryEditorX
{
	class CommandPool;

	struct GPUDevice
	{
        VkFormat depthFormat;
        VkFormat tilingFormat;
        VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
        VkFormatProperties formatProperties;
        VkPhysicalDeviceLimits GFXLimits;
        VkPhysicalDeviceFeatures deviceFeatures;
        VkSurfaceCapabilitiesKHR surfaceCapabilities;
        VkPhysicalDeviceProperties2 deviceProperties;
        VkPhysicalDeviceMemoryProperties memoryProperties;

	    // -------------------------------------------------------

        std::vector<VkBool32> queueSupportPresent;
        std::vector<VkPresentModeKHR> presentModes;
        std::vector<VkSurfaceFormatKHR> surfaceFormats;
	    std::vector<VkQueueFamilyProperties> queueFamilyInfo;
        std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;

	    // -------------------------------------------------------

	    GPUDevice() : depthFormat(), tilingFormat(), formatProperties(), GFXLimits({}), deviceFeatures(), surfaceCapabilities(), deviceProperties(), memoryProperties() {}
	};

    // -----------------------------------------------------------

	class VulkanPhysicalDevice : public RefCounted
    {
    public:
        explicit VulkanPhysicalDevice(VkInstance &instance);
        virtual ~VulkanPhysicalDevice() override;

		// Delete copy constructor and assignment operator/
        VulkanPhysicalDevice(const VulkanPhysicalDevice &) = delete;
        VulkanPhysicalDevice &operator=(const VulkanPhysicalDevice &) = delete;

        // Allow move operations if needed
        VulkanPhysicalDevice(VulkanPhysicalDevice &&) noexcept = default;
        VulkanPhysicalDevice &operator=(VulkanPhysicalDevice &&) noexcept = default;

		/**
		* @brief Structure to hold indices of different queue families.
		* This structure contains optional indices for graphics, present, compute, and transfer queue families.
		*/
		struct QueueFamilyIndices
		{
		    std::optional<std::pair<Queue, uint32_t>> graphicsFamily;
		    std::optional<std::pair<Queue, uint32_t>> presentFamily;
		    std::optional<std::pair<Queue, uint32_t>> computeFamily;
		    std::optional<std::pair<Queue, uint32_t>> transferFamily;

            /**
             * @brief Check if all required queue families are initialized.
             * @return True if all required families are set, false otherwise.
             */
            [[nodiscard]] bool IsComplete() const;

            /**
             * @brief Get the family index.
             * @return The family index, or 0 if not initialized.
             */
            [[nodiscard]] uint32_t GetGraphicsFamily() const;
            [[nodiscard]] uint32_t GetPresentFamily() const;
            [[nodiscard]] uint32_t GetComputeFamily() const;
            [[nodiscard]] uint32_t GetTransferFamily() const;
        };

		/**
		 * @brief Select a physical device based on the best available options.
		 * @param instance The Vulkan instance to use.
		 * @return A reference to the selected physical device.
		 * @throws Error if no suitable device is found.
		 */
	    static Ref<VulkanPhysicalDevice> Select(VkInstance &instance);

		/**
         * @brief Select a physical device based on queue requirements.
         * @param queueType The required queue type flags.
         * @param supportPresent Whether presentation support is required.
         * @return The queue family index for the selected device.
         */
        //uint32_t SelectDevice(VkQueueFlags queueType, bool supportPresent);

		/**
         * @brief Get the currently selected GPU device.
         * @return Reference to the selected GPU device.
         * @throws Error if no device is selected.
         */
        [[nodiscard]] const GPUDevice& Selected() const;

        // Accessor methods
        [[nodiscard]] VkPhysicalDevice GetGPUDevices() const;

		[[nodiscard]] const QueueFamilyIndices &GetQueueFamilyIndices() const { return QFamilyIndices; }
		[[nodiscard]] const VkPhysicalDeviceLimits &GetLimits() const { return devices.at(deviceIndex).GFXLimits; }
        [[nodiscard]] const VkPhysicalDeviceMemoryProperties &GetMemoryProperties() const { return devices.at(deviceIndex).memoryProperties; }
		[[nodiscard]] VkFormat GetDepthFormat() const { return  devices.at(deviceIndex).depthFormat;}
		[[nodiscard]] const VkPhysicalDeviceFeatures &GetDeviceFeatures() const { return devices.at(deviceIndex).deviceFeatures; }
        [[nodiscard]] VkPhysicalDeviceProperties2 GetDeviceProperties() const { return devices.at(deviceIndex).deviceProperties; }
		[[nodiscard]] const std::vector<VkSurfaceFormatKHR> &GetSurfaceFormats() const { return devices.at(deviceIndex).surfaceFormats; }
		[[nodiscard]] const std::vector<VkPresentModeKHR> &GetPresentModes() const { return devices.at(deviceIndex).presentModes; }
		[[nodiscard]] const std::vector<VkQueueFamilyProperties> &GetQueueFamilyProperties() const { return devices.at(deviceIndex).queueFamilyInfo; }

        /**
         * @brief Find queue families that meet specified criteria in the physical device.
         * @param device The physical device to examine.
         * @return Queue family indices for different queue types.
         */
        //QueueFamilyIndices FindQueueFamilies(VkPhysicalDevice device) const;

    private:
        VkFormat depthFormat = VK_FORMAT_UNDEFINED;
        VkInstance *vkInstance;
        VkPhysicalDevice physicalDevice = nullptr;
        QueueFamilyIndices QFamilyIndices;

        int deviceIndex = -1;
        std::vector<GPUDevice> devices;
        std::unordered_set<std::string> supportedExtensions;

        static VkFormat FindDepthFormat(const GPUDevice& device);
		static VkFormat FindSupportedFormat(VkPhysicalDevice physicalDevice, const std::vector<VkFormat>& candidates, VkImageTiling tiling, VkFormatFeatureFlags features);

	    /**
         * @brief Find queue families that match specified queue flags.
         * @param qFlags The queue flags to search for.
         * @return Queue family indices for different queue types.
         */
        [[nodiscard]] QueueFamilyIndices GetQueueFamilyIndices(VkQueueFlags qFlags) const;

		friend class VulkanDevice;
		friend class VulkanChecks;
        friend class VulkanQueue;
    };

	// ---------------------------------------------------------

	class VulkanDevice : public RefCounted
    {
    public:
        /**
         * @brief Create a logical device from a physical device.
         * @param physDevice The physical device to use.
         * @param enabledFeatures Device features to enable.
         */
        explicit VulkanDevice(const Ref<VulkanPhysicalDevice> &physDevice);
        virtual ~VulkanDevice() override;

        void Tick(uint64_t frame_count) const;
        //Ref<MemoryAllocator> GetValue() const;
        VmaAllocator GetMemoryAllocator() const;
        VkCommandBuffer GetCommandBuffer(bool cond);

        // Immediate submit helper (one-shot command buffer already ended)
        void ImmediateSubmit(VkCommandBuffer cmd, VkQueue queue = VK_NULL_HANDLE)
        {
            if (!cmd) return;
            if (queue == VK_NULL_HANDLE)
                queue = GetGraphicsQueue();
            FlushCmdBuffer(cmd, queue);
        }

        bool IsValidResolution(uint32_t width, uint32_t height);

        /// Delete copy constructor and assignment operator.
        VulkanDevice(const VulkanDevice &) = delete;
        VulkanDevice &operator=(const VulkanDevice &) = delete;

		/// Allow move operations if needed.
        VulkanDevice(VulkanDevice &&) noexcept;
        VulkanDevice &operator=(VulkanDevice &&) noexcept;

        /**
         * @brief Clean up resources and destroy the logical device.
         */
        void Destroy();

		/// Accessor methods.
	    [[nodiscard]] const VkDevice &Selected() const { return GetDevice(); }
        [[nodiscard]] VkQueue GetGraphicsQueue() const { return GraphicsQueue; }
        [[nodiscard]] VkQueue GetComputeQueue() const { return ComputeQueue; }
        [[nodiscard]] VkQueue GetPresentQueue() const { return PresentQueue; }
        [[nodiscard]] const VkDevice& GetDevice() const { return device; }
		[[nodiscard]] const Ref<VulkanPhysicalDevice> &GetPhysicalDevice() const {return vkPhysicalDevice;}
        [[nodiscard]] uint32_t FindMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties) const;
        void SetResourceName(void *resource, const ResourceType resourceType, const char *name);

	    // -------------------------------------------------------
        /// Function pointers for Vulkan extensions
        // -------------------------------------------------------
        PFN_vkGetBufferDeviceAddressKHR vkGetBufferDeviceAddressKHR = nullptr;
        PFN_vkSetDebugUtilsObjectNameEXT vkSetDebugUtilsObjectNameEXT = nullptr;
        PFN_vkCreateAccelerationStructureKHR vkCreateAccelerationStructureKHR = nullptr;
        PFN_vkDestroyAccelerationStructureKHR vkDestroyAccelerationStructureKHR = nullptr;
        PFN_vkCmdBuildAccelerationStructuresKHR vkCmdBuildAccelerationStructuresKHR = nullptr;
        PFN_vkGetAccelerationStructureBuildSizesKHR vkGetAccelerationStructureBuildSizesKHR = nullptr;
        PFN_vkGetAccelerationStructureDeviceAddressKHR vkGetAccelerationStructureDeviceAddressKHR = nullptr;

        /**
         * @brief Create a staging buffer for data transfer.
         * @param size Size of the buffer in bytes.
         * @param name Debug name for the buffer.
         * @return Buffer object configured for staging.
         */
        static Buffer CreateStagingBuffer(uint64_t size, const std::string& name = "Staging Buffer");

	    /**
         * @brief Get the maximum usable MSAA sample count supported by the device.
         * @return The maximum sample count as a VkSampleCountFlagBits value.
         */
        [[nodiscard]] VkSampleCountFlagBits GetMaxUsableSampleCount() const;

		/**
         * @brief Lock a queue for exclusive access.
         * @param compute Whether to lock the compute queue (true) or graphics queue (false).
         */
		void LockQueue(bool compute = false);

        /**
         * @brief Unlock a previously locked queue.
         * @param compute Whether to unlock the compute queue (true) or graphics queue (false).
         */
        void UnlockQueue(bool compute = false);

        /**
         * @brief Create a secondary command buffer for recording commands.
         * @param debugName Name for debugging purposes.
         * @return A new command buffer.
         */
        VkCommandBuffer CreateUICmdBuffer(const char *debugName);

        Ref<CommandPool> GetThreadLocalCommandPool();
        Ref<CommandPool> GetOrCreateThreadLocalCommandPool();

        /**
         * @brief Submit and wait for a command buffer to complete execution.
         * @param cmdBuffer The command buffer to submit.
         */
        void FlushCmdBuffer(VkCommandBuffer cmdBuffer);

        /**
         * @brief Submit a command buffer to a specific queue and wait for completion.
         * @param cmdBuffer The command buffer to submit.
         * @param queue The queue to submit to.
         */
        void FlushCmdBuffer(VkCommandBuffer cmdBuffer, VkQueue queue);

        /**
         * @brief Get the scratch buffer address.
         * @return The device address of the scratch buffer.
         */
        VkSampler GetSampler() const { return textureSampler; }

        /**
         * @brief Get the bindless resources associated with this device.
         * @return The bindless resources associated with this device.
         */
        //BindlessResources GetBindlessResources() const { return bindlessResources; }

		/**
         * @brief Initialize the memory allocator for this device.
         */
        //void InitializeMemoryAllocator();

    private:
        Layers vkLayers;
        VkDevice device = nullptr;
        //BindlessResources bindlessResources;
		Ref<MemoryAllocator> memoryAllocator;
        VkSampler textureSampler = nullptr;
        Ref<CommandPool> LocalCommandPool();
        Ref<CommandPool> CreateLocalCommandPool();
        Ref<VulkanPhysicalDevice> vkPhysicalDevice;
        VkPhysicalDeviceFeatures vkEnabledFeatures = {};
        uint32_t initialScratchBufferSize = 64 * 1024 * 1024;
		Buffer scratchBuffer = {};
		uint64_t scratchAddress;

		// -------------------------------------------------------

        VkQueue GraphicsQueue = VK_NULL_HANDLE;
        VkQueue ComputeQueue = VK_NULL_HANDLE;
        VkQueue PresentQueue = VK_NULL_HANDLE;
        VkQueue TransferQueue = VK_NULL_HANDLE;

        std::mutex GraphicsQueueMutex;
        std::mutex ComputeQueueMutex;
        std::mutex PresentQueueMutex;
        std::mutex TransferQueueMutex;

		// -------------------------------------------------------

        /// Command pool management
        std::map<std::thread::id, Ref<CommandPool>> CmdPools;

        /**
         * @brief Create a texture sampler with specified parameters
         * @param maxLOD Maximum LOD level for mipmapping
         * @return VkSampler handle
         */
        [[nodiscard]] VkSampler CreateSampler(float maxLOD) const;

        /**
         * @brief Load function pointers for extension functions
         *
         * @param device
         */
        void LoadExtensionFunctions();

        // -------------------------------------------------------

	    VkQueue vkQueue = VK_NULL_HANDLE;
        VkDevice vkDevice = VK_NULL_HANDLE;
	    VkSurfaceKHR vkSurface = VK_NULL_HANDLE;
        VkCommandBuffer vkCommandBuffer = VK_NULL_HANDLE;
    };

}

// -------------------------------------------------------
