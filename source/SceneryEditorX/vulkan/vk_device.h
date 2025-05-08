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
#include <map>
#include <memory>
#include <mutex>
#include <SceneryEditorX/renderer/buffer_data.h>
#include <SceneryEditorX/renderer/image_data.h>
#include <vulkan/vulkan.h>

// -------------------------------------------------------

namespace SceneryEditorX
{
	class CommandPool;

	struct GPUDevice
	{
        VkFormat depthFormat;
        VkFormat tilingFormat;
        VkFormatProperties formatProperties;

        VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
        VkPhysicalDeviceFeatures GFXFeatures = {};
        VkPhysicalDeviceLimits GFXLimits = {};
        VkSurfaceCapabilitiesKHR surfaceCapabilities;
        VkPhysicalDeviceFeatures deviceInfo = {};
        VkPhysicalDeviceProperties deviceProperties = {};
        VkPhysicalDeviceMemoryProperties memoryInfo = {};

        std::vector<VkBool32> queueSupportPresent;
        std::vector<VkPresentModeKHR> presentModes;
        std::vector<VkSurfaceFormatKHR> surfaceFormats;
	    std::vector<VkQueueFamilyProperties> queueFamilyInfo;
        std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;

	    GPUDevice() :
			depthFormat(),
            tilingFormat(),
            formatProperties(),
			physicalDevice(VK_NULL_HANDLE),
			GFXFeatures({}),
	        GFXLimits({}),
			surfaceCapabilities(),
	        deviceInfo({}),
	        deviceProperties({}),
	        memoryInfo({})
	    {
	    }
	};

    /// -----------------------------------------------------------

    struct QueueFamilyIndices
    {
		uint32_t Graphics = UINT32_MAX;
		uint32_t Compute = UINT32_MAX;
		uint32_t Transfer = UINT32_MAX;
		uint32_t Present = UINT32_MAX;

        [[nodiscard]] bool isComplete() const
        {
            return (graphicsFamily.has_value() && presentFamily.has_value() && transferFamily.has_value()) || computeFamily.has_value();
        }
        [[nodiscard]] uint32_t GetGraphicsFamily() const
        {
            return graphicsFamily.value().second;
        }
        [[nodiscard]] uint32_t GetPresentFamily() const
        {
            return presentFamily.value().second;
        }
        [[nodiscard]] uint32_t GetComputeFamily() const
        {
            return computeFamily.value().second;
        }
        [[nodiscard]] uint32_t GetTransferFamily() const
        {
            return transferFamily.value().second;
        }
    };

    /// -----------------------------------------------------------

	class VulkanPhysicalDevice
    {
    public:

        VulkanPhysicalDevice();
        ~VulkanPhysicalDevice();

		/// Delete copy constructor and assignment operator
        VulkanPhysicalDevice(const VulkanPhysicalDevice &) = delete;
        VulkanPhysicalDevice &operator=(const VulkanPhysicalDevice &) = delete;

        /// Allow move operations if needed
        VulkanPhysicalDevice(VulkanPhysicalDevice &&) noexcept = default;
        VulkanPhysicalDevice &operator=(VulkanPhysicalDevice &&) noexcept = default;

		/**
		 * @brief Select a physical device based on the best available options
		 * @return A reference to the selected physical device
		 * @throws Error if no suitable device is found
		 */
	    static Ref<VulkanPhysicalDevice> Select();

		/**
         * @brief Select a physical device based on queue requirements
         * @param queueType The required queue type flags
         * @param supportPresent Whether presentation support is required
         * @return The queue family index for the selected device
         */
        uint32_t SelectDevice(VkQueueFlags queueType, bool supportPresent);

		/**
         * @brief Get the currently selected GPU device
         * @return Reference to the selected GPU device
         * @throws Error if no device is selected
         */
        [[nodiscard]] const GPUDevice& Selected() const;

        /// Accessor methods
		[[nodiscard]] const QueueFamilyIndices &GetQueueFamilyIndices() const { return QFamilyIndices; }
		[[nodiscard]] const VkPhysicalDeviceLimits &GetLimits() const { return devices.at(deviceIndex).GFXLimits; }
        [[nodiscard]] const VkPhysicalDeviceMemoryProperties &GetMemoryProperties() const { return devices.at(deviceIndex).memoryInfo; }
		[[nodiscard]] VkFormat GetDepthFormat() const { return devices.at(deviceIndex).depthFormat; }
		[[nodiscard]] VkPhysicalDevice GetGPUDevice() const { return devices.at(deviceIndex).physicalDevice; }
		[[nodiscard]] VkPhysicalDeviceFeatures GetDeviceFeatures() const { return devices.at(deviceIndex).deviceInfo; }
        [[nodiscard]] VkPhysicalDeviceProperties GetDeviceProperties() const { return devices.at(deviceIndex).deviceProperties; }
		[[nodiscard]] const std::vector<VkSurfaceFormatKHR> &GetSurfaceFormats() const { return devices.at(deviceIndex).surfaceFormats; }
		[[nodiscard]] const std::vector<VkPresentModeKHR> &GetPresentModes() const { return devices.at(deviceIndex).presentModes; }
		[[nodiscard]] const std::vector<VkQueueFamilyProperties>& GetQueueFamilyProperties() const { return devices.at(deviceIndex).queueFamilyInfo; }

        /**
         * @brief Find queue families that meet specified criteria in the physical device
         * @param device The physical device to examine
         * @return Queue family indices for different queue types
         */
        QueueFamilyIndices FindQueueFamilies(VkPhysicalDevice device) const;

    private:
        std::vector<GPUDevice> devices;
        std::unordered_set<std::string> supportedExtensions;
        VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
        VkInstance instance = VK_NULL_HANDLE;
        QueueFamilyIndices QFamilyIndices;
        int deviceIndex = -1;

		INTERNAL VkFormat FindDepthFormat(const GPUDevice& device);
		INTERNAL VkFormat FindSupportedFormat(VkPhysicalDevice physicalDevice, const std::vector<VkFormat>& candidates, VkImageTiling tiling, VkFormatFeatureFlags features);

	    /**
         * @brief Find queue families that match specified queue flags
         * @param qFlags The queue flags to search for
         * @return Queue family indices for different queue types
         */
        [[nodiscard]] QueueFamilyIndices GetQueueFamilyIndices(VkQueueFlags qFlags) const;


		/**
         * @brief Enumerate and populate device information
         * @param instance The Vulkan instance to use
         */
        void EnumerateDevices();

		friend class VulkanDevice;
		friend class VulkanChecks;
        friend class VulkanQueue;
    };

	// ---------------------------------------------------------

	class VulkanDevice
    {
    public:

        /**
         * @brief Create a logical device from a physical device
         * @param physDevice The physical device to use
         * @param enabledFeatures Device features to enable
         */
        VulkanDevice(const Ref<VulkanPhysicalDevice> &physDevice, VkPhysicalDeviceFeatures enabledFeatures);
        virtual ~VulkanDevice();
        Ref<MemoryAllocator> GetValue() const;
        VmaAllocator GetMemoryAllocator() const;

        /// Delete copy constructor and assignment operator
        VulkanDevice(const VulkanDevice &) = delete;
        VulkanDevice &operator=(const VulkanDevice &) = delete;

		/// Allow move operations if needed
        //VulkanDevice(VulkanDevice &&) noexcept;
        //VulkanDevice &operator=(VulkanDevice &&) noexcept;

        /**
         * @brief Clean up resources and destroy the logical device
         */
        void Destroy();

		/// Accessor methods
        [[nodiscard]] const VkDevice &Selected() const;
        [[nodiscard]] VkQueue GetGraphicsQueue() const { return GraphicsQueue; }
        [[nodiscard]] VkQueue GetComputeQueue() const { return ComputeQueue; }
        //[[nodiscard]] VkQueue GetPresentQueue() const { return PresentQueue; }
		[[nodiscard]] VkDevice GetDevice() const {return device;}
		[[nodiscard]] const Ref<VulkanPhysicalDevice> &GetPhysicalDevice() const {return vkPhysDevice;}
        uint32_t FindMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties) const;

        /**
         * @brief Create a buffer with specified properties
         * @param size Size of the buffer in bytes
         * @param usage How the buffer will be used
         * @param memory Memory type for allocation
         * @param name Debug name for the buffer
         * @return Buffer object
         */
        Buffer CreateBuffer(uint32_t size, BufferUsageFlags usage, MemoryFlags memory = MemoryType::GPU, const std::string& name = "") const;

		/**
         * @brief Create a staging buffer for data transfer
         * @param size Size of the buffer in bytes
         * @param name Debug name for the buffer
         * @return Buffer object configured for staging
         */
        Buffer CreateStagingBuffer(uint32_t size, const std::string& name = "Staging Buffer");

		/**
         * @brief Lock a queue for exclusive access
         * @param compute Whether to lock the compute queue (true) or graphics queue (false)
         */
		void LockQueue(bool compute = false);

        /**
         * @brief Unlock a previously locked queue
         * @param compute Whether to unlock the compute queue (true) or graphics queue (false)
         */
        void UnlockQueue(bool compute = false);

        /**
         * @brief Create a secondary command buffer for recording commands
         * @param debugName Name for debugging purposes
         * @return A new command buffer
         */
        VkCommandBuffer CreateSecondaryCommandBuffer(const char *debugName);

        /**
         * @brief Submit and wait for a command buffer to complete execution
         * @param cmdBuffer The command buffer to submit
         */
        void FlushCmdBuffer(VkCommandBuffer cmdBuffer);

        /**
         * @brief Submit a command buffer to a specific queue and wait for completion
         * @param cmdBuffer The command buffer to submit
         * @param queue The queue to submit to
         */
        void FlushCmdBuffer(VkCommandBuffer cmdBuffer, VkQueue queue);

        /**
         * @brief Get the maximum usable MSAA sample count supported by the device
         * @return The maximum sample count as a VkSampleCountFlagBits value
         */
        [[nodiscard]] VkSampleCountFlagBits GetMaxUsableSampleCount() const;

        VkSampler GetSampler() const { return textureSampler; }

    private:
        RenderData renderData;
        ImageID textureImageID;
        Extensions vkExtensions;
        Layers vkLayers;
        BindlessResources bindlessResources;

        Buffer scratchBuffer;
        VkDeviceAddress scratchAddress = 0;

		Ref<MemoryAllocator> memoryAllocator;
        VkSampler textureSampler = VK_NULL_HANDLE;
        VkDevice device = VK_NULL_HANDLE;
        VkInstance vkInstance = VK_NULL_HANDLE;
        Ref<VulkanPhysicalDevice> vkPhysDevice;
        VkPhysicalDeviceFeatures vkEnabledFeatures = {};
        const uint32_t initialScratchBufferSize = 64 * 1024 * 1024;

		// -------------------------------------------------------

        /// Function pointers for Vulkan extensions
        PFN_vkSetDebugUtilsObjectNameEXT vkSetDebugUtilsObjectNameEXT = nullptr;
        PFN_vkGetAccelerationStructureBuildSizesKHR vkGetAccelerationStructureBuildSizesKHR = nullptr;
        PFN_vkCreateAccelerationStructureKHR vkCreateAccelerationStructureKHR = nullptr;
        PFN_vkGetBufferDeviceAddressKHR vkGetBufferDeviceAddressKHR = nullptr;
        PFN_vkCmdBuildAccelerationStructuresKHR vkCmdBuildAccelerationStructuresKHR = nullptr;
        PFN_vkGetAccelerationStructureDeviceAddressKHR vkGetAccelerationStructureDeviceAddressKHR = nullptr;
        PFN_vkDestroyAccelerationStructureKHR vkDestroyAccelerationStructureKHR = nullptr;

		// -------------------------------------------------------

        VkQueue GraphicsQueue = VK_NULL_HANDLE;
        VkQueue ComputeQueue = VK_NULL_HANDLE;
        //VkQueue PresentQueue = VK_NULL_HANDLE;
        std::mutex GraphicsQueueMutex;
        std::mutex ComputeQueueMutex;

		// -------------------------------------------------------

        /// Command pool management
        std::map<std::thread::id, Ref<CommandPool>> CmdPools;
        Ref<CommandPool> GetThreadLocalCmdPool();
        //Ref<CommandPool> GetOrCreateThreadLocalCmdPool();

        /**
         * @brief Create Vulkan 1.2+ features structure and load device extensions
         */
        void CreateDeviceFeatures2();

        /**
         * @brief Initialize the memory allocator for this device
         */
        void InitializeMemoryAllocator();

        /**
         * @brief Create a texture sampler with specified parameters
         * @param maxLOD Maximum LOD level for mipmapping
         * @return VkSampler handle
         */
        [[nodiscard]] VkSampler CreateSampler(float maxLOD) const;

        /**
         * @brief Initialize bindless resources for the device
         */
        void InitializeBindlessResources();

        /**
         * @brief Load function pointers for extension functions
         */
        void LoadExtensionFunctions();

        VkPhysicalDevice vkPhysicalDevice = VK_NULL_HANDLE;
        VkDevice vkDevice = VK_NULL_HANDLE;
	    VkCommandBuffer vkCommandBuffer = VK_NULL_HANDLE;
	    VkQueue vkQueue = VK_NULL_HANDLE;
	    VkSurfaceKHR vkSurface = VK_NULL_HANDLE;
    };

	// ---------------------------------------------------------

	class CommandPool
    {
    public:
        /**
         * @brief Create command pools for a device
         * @param vulkanDevice The device to create command pools for
         */
        explicit CommandPool(Ref<VulkanDevice> vulkanDevice);
        virtual ~CommandPool();

        /**
         * @brief Allocate a command buffer from the pool
         * @param begin Whether to begin the command buffer
         * @param compute Whether to allocate from the compute pool
         * @return A new command buffer
         */
        [[nodiscard]] VkCommandBuffer AllocateCommandBuffer(bool begin, bool compute = false) const;

        /**
         * @brief Submit a command buffer to the graphics queue and wait for completion
         * @param cmdBuffer The command buffer to submit
         */
        void FlushCmdBuffer(VkCommandBuffer cmdBuffer) const;

        /**
         * @brief Submit a command buffer to a specific queue and wait for completion
         * @param cmdBuffer The command buffer to submit
         * @param queue The queue to submit to
         */
        void FlushCmdBuffer(VkCommandBuffer cmdBuffer, VkQueue queue) const;

        /// Accessor methods
        [[nodiscard]] VkCommandPool GetGraphicsCmdPool() const { return GraphicsCmdPool; }
        [[nodiscard]] VkCommandPool GetComputeCmdPool() const { return ComputeCmdPool; }
        [[nodiscard]] Ref<VulkanDevice> GetDevice() const { return device; }

    private:
        Ref<VulkanDevice> device;
        VkCommandPool GraphicsCmdPool = VK_NULL_HANDLE;
        VkCommandPool ComputeCmdPool = VK_NULL_HANDLE;
	};

	// ---------------------------------------------------------

} // namespace SceneryEditorX

// -------------------------------------------------------
