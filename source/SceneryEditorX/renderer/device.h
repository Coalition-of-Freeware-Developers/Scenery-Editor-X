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
#include "memory_allocator.h"
#include "vulkan_data.h"
#include <optional>
#include <vulkan/vulkan.h>
// PCH normally provides these in AppCore; include here for standalone TUs (tests)
#include "enums.h"
#include <map>
#include <mutex>
#include <string>
#include <thread>
#include <unordered_set>

// -------------------------------------------------------

namespace SceneryEditorX
{
	//class CommandPool;

    enum GPUQueueType : uint8_t
    {
        GPU_QUEUE_GRAPHICS	= 0,
        GPU_QUEUE_COMPUTE	= 1,
        GPU_QUEUE_TRANSFER	= 2,
        GPU_QUEUES_COUNT	= 3,
        GPU_QUEUE_UNKNOWN	= static_cast<uint8_t>(-1)
    };

    struct GPUQueue
    {
        VkQueue handle = VK_NULL_HANDLE;
        uint32_t familyIndex = GPU_QUEUE_UNKNOWN;
    };

    // -----------------------------------------------------------

	class VulkanPhysicalDevice : public RefCounted
    {
    public:
	    VulkanPhysicalDevice();
        virtual ~VulkanPhysicalDevice();

	    /**
		 * @brief Get the underlying VkPhysicalDevice handle.
		 * @return The VkPhysicalDevice handle.
		 */
        VkPhysicalDevice GetDevice() const { return m_PhysicalDevice; }

		/**
		 * @struct QueueFamilyIndices
		 * @brief Structure to hold indices of different queue families.
		 *
		 * @details This structure encapsulates the indices of various queue families
		 * available on a Vulkan physical device. It includes indices for graphics,
		 * present, compute, and transfer queues. The structure provides methods
		 * to check if all required queue families are initialized and to access
		 * the indices based on queue type.
		 *
		 * @note - The INVALID constant is used to represent uninitialized indices.
		 * @note - Raw indices with UINT32_MAX as the invalid sentinel.
		 */
		struct QueueFamilyIndices
		{
            uint32_t graphics;
            uint32_t present;
            uint32_t compute;
            uint32_t transfer;

			/** @brief Sentinel value representing an invalid index. */
            static constexpr uint32_t INVALID = static_cast<uint32_t>(-1);

            /** @brief Constructor initializes all family indices to INVALID. */
            QueueFamilyIndices() : graphics(INVALID), present(INVALID), compute(INVALID), transfer(INVALID) {}

            /**
             * @brief Check if all required queue families are initialized.
             * @return True if all required families are set, false otherwise.
             */
            [[nodiscard]] bool IsComplete() const;

			/**
			 * @brief Unified accessor, get the family index based on the specified type.
			 * @param type The queue type (graphics, present, compute, transfer).
			 * @return The corresponding family index, or INVALID if not found.
			 */
			uint32_t Get(uint32_t type) const;

            /**
             * @brief Get the family index.
             * @return The family index, or 0 if not initialized.
             */
            [[nodiscard]] uint32_t GetGraphicsFamily() const noexcept;
            [[nodiscard]] uint32_t GetPresentFamily() const noexcept;
            [[nodiscard]] uint32_t GetComputeFamily() const noexcept;
            [[nodiscard]] uint32_t GetTransferFamily() const noexcept;
        };

        /**
		 * @brief Select a physical device based on the best available options.
		 * @return A reference to the selected physical device.
		 * @throws Error if no suitable device is found.
		 */
	    static Ref<VulkanPhysicalDevice> Select();

        //////////////////////////////////////////////////////////////////////////////////////////////////////////
		// Accessor Methods																						//
        //////////////////////////////////////////////////////////////////////////////////////////////////////////
        const VkPhysicalDeviceLimits &GetLimits() const							{ return m_Limits; }
        VkPhysicalDeviceFeatures2 GetFeatures() const							{ return m_Features; }
		VkPhysicalDeviceMemoryProperties2 GetMemoryProperties() const			{ return m_MemProperties; }
		VkSurfaceCapabilitiesKHR GetSurfaceCapabilities() const					{ return m_SurfaceCapabilities; }
        VkPhysicalDeviceProperties2 GetDeviceProperties() const					{ return m_DeviceProperties; }
        std::vector<VkPresentModeKHR> GetPresentModes() const					{ return m_PresentModes; }
        std::vector<VkSurfaceFormatKHR> GetSurfaceFormats() const				{ return m_SurfaceFormats; }
        const QueueFamilyIndices &GetQueueFamilyIndices() const					{ return m_QueueFamilyIndices; }

		/**
		 * @brief Find a suitable memory type index based on type filter and properties.
		 *
		 * @param typeFilter Bitmask of acceptable memory types.
		 * @param properties Desired memory property flags.
		 * @return Index of the suitable memory type.
		 */
        uint32_t FindMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties);

		/**
		 * @brief Find a supported depth format for depth buffering.
		 * @return The chosen VkFormat for depth buffering.
		 */
        VkFormat FindDepthFormat() const;

	    bool IsExtensionSupported(const std::string &extensionName);

    private:
        int m_DeviceIndex = -1;
        VkInstance *m_Instance;
        VkPhysicalDevice m_PhysicalDevice = nullptr;
        QueueFamilyIndices m_QueueFamilyIndices;
        VkFormatProperties m_FormatProperties;

        VkPhysicalDeviceLimits m_Limits;
        VkPhysicalDeviceFeatures2 m_Features;
        VkSurfaceCapabilitiesKHR m_SurfaceCapabilities;
        VkPhysicalDeviceProperties2 m_DeviceProperties;
        VkPhysicalDeviceMemoryProperties2 m_MemProperties;

        // -------------------------------------------------------

	    VkFormat m_DepthFormat	= VK_FORMAT_UNDEFINED;
        VkFormat m_TilingFormat = VK_FORMAT_UNDEFINED;

        // -------------------------------------------------------

        std::vector<VkBool32> m_QueueSupportPresent;
        std::vector<VkPresentModeKHR> m_PresentModes;
        std::vector<VkSurfaceFormatKHR> m_SurfaceFormats;
        std::vector<VkExtensionProperties> m_Extensions;
        std::vector<VkQueueFamilyProperties2> m_QueueFamilyInfo;
        std::vector<VkDeviceQueueCreateInfo> m_QueueCreateInfos;
        std::unordered_set<std::string> m_SupportedExtensions;

        /**
         * @brief Find queue families that match specified queue flags.
         * @param flags The queue flags to search for.
         * @return Queue family indices for different queue types.
         */
        QueueFamilyIndices GetQueueFamilyIndices(int flags);
        uint32_t GetMemoryTypeIndex(uint32_t typeBits, VkMemoryPropertyFlags properties) const;

        static VkFormat FindSupportedFormat(VkPhysicalDevice physicalDevice, const std::vector<VkFormat>& candidates, VkImageTiling tiling, VkFormatFeatureFlags features);

		friend class VulkanDevice;
		friend class VulkanChecks;
        friend class VulkanQueue;
    };

	// ---------------------------------------------------------

	class VulkanDevice : public RefCounted
    {
    public:
	    VulkanDevice(const Ref<VulkanPhysicalDevice> &physDevice);
        virtual ~VulkanDevice() override;

        // Delete copy constructor and assignment operator.
        VulkanDevice(const VulkanDevice &) = delete;
        VulkanDevice &operator=(const VulkanDevice &) = delete;
        // Allow move operations if needed.
        VulkanDevice(VulkanDevice &&) noexcept;
        VulkanDevice &operator=(VulkanDevice &&) noexcept;

	    //////////////////////////////////////////////////////////////////////////////////////////////////////////

        void Tick(uint64_t frameCount) const;
        //Ref<MemoryAllocator> GetValue() const;
        VmaAllocator GetMemoryAllocator() const;
        //VkCommandBuffer GetCommandBuffer(bool cond);

        //bool IsValidResolution(uint32_t width, uint32_t height);

        void Destroy(); // Clean up resources and destroy the logical device.

		uint32_t GetGPUQueues(const Ref<VulkanPhysicalDevice> &physDevice);
		
	    //////////////////////////////////////////////////////////////////////////////////////////////////////////
		// Accessor Methods																						//
        //////////////////////////////////////////////////////////////////////////////////////////////////////////
        [[nodiscard]] VkQueue GetGraphicsQueue() const { return m_GraphicsQueue; }
        [[nodiscard]] VkQueue GetComputeQueue() const { return m_ComputeQueue; }
        [[nodiscard]] VkQueue GetPresentQueue() const { return m_PresentQueue; }
        [[nodiscard]] VkQueue GetTransferQueue() const { return m_TransferQueue; }
        //////////////////////////////////////////////////////////////////////////////////////////////////////////
        [[nodiscard]] const VkDevice &Selected() const { return GetDevice(); }
        [[nodiscard]] const VkDevice &GetDevice() const { return m_Device; }
        VkFormat GetDepthFormat() const { return m_DepthFormat; }
		[[nodiscard]] const Ref<VulkanPhysicalDevice> &GetPhysicalDevice() const {return m_PhysicalDevice;}
        //[[nodiscard]] uint32_t FindMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties) const;
        void SetDebugName(void *resource, ResourceType resourceType, const char *name);

        //////////////////////////////////////////////////////////////////////////////////////////////////////////
        // Function pointers for Vulkan extensions																//
        //////////////////////////////////////////////////////////////////////////////////////////////////////////
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
        //static Buffer CreateStagingBuffer(uint64_t size, const std::string& name = "Staging Buffer");

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
         * @brief Get the scratch buffer address.
         * @return The device address of the scratch buffer.
         */
        // VkSampler GetSampler() const { return textureSampler; }

        /**
         * @brief Get the bindless resources associated with this device.
         * @return The bindless resources associated with this device.
         */
        // BindlessResources GetBindlessResources() const { return bindlessResources; }

    private:
        Layers m_Layers;
        VkQueue m_Queue = VK_NULL_HANDLE;
        VkDevice m_Device = VK_NULL_HANDLE;
        VkSurfaceKHR m_Surface = VK_NULL_HANDLE;
        VkCommandBuffer m_CmdBuffer = VK_NULL_HANDLE;
        //BindlessResources bindlessResources;
		Ref<MemoryAllocator> m_MemoryAlloc;
        VkSampler m_TextureSampler = nullptr;
        VkFormat m_DepthFormat = VK_FORMAT_UNDEFINED;
        Ref<VulkanPhysicalDevice> m_PhysicalDevice;
        VkPhysicalDeviceFeatures m_EnabledFeatures = {};
        uint32_t m_InitScratchBufferSize = 64 * 1024 * 1024;
		//Buffer m_ScratchBuffer = {};
		uint64_t m_ScratchAddress;

		// -------------------------------------------------------

        VkQueue m_GraphicsQueue;
        VkQueue m_ComputeQueue;
        VkQueue m_PresentQueue;
        VkQueue m_TransferQueue;

        uint32_t m_GraphicsQueueFamilyIndex = UINT32_MAX;
        uint32_t m_ComputeQueueFamilyIndex = UINT32_MAX;
        uint32_t m_TransferQueueFamilyIndex = UINT32_MAX;
        uint32_t m_PresentQueueFamilyIndex = UINT32_MAX;

        std::mutex m_GraphicsQueueMutex;
        std::mutex m_ComputeQueueMutex;
        std::mutex m_PresentQueueMutex;
        std::mutex m_TransferQueueMutex;

		// -------------------------------------------------------

        // Command pool management
        //std::map<std::thread::id, Ref<CommandPool>> m_CmdPools;

        /**
         * @brief Create a texture sampler with specified parameters
         * @param maxLOD Maximum LOD level for mipmapping
         * @return VkSampler handle
         */
        //[[nodiscard]] VkSampler CreateSampler(float maxLOD) const;

        /** @brief Load function pointers for extension functions */
        void LoadExtensionFunctions();

    };

}

// -------------------------------------------------------
