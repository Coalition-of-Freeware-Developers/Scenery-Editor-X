/**
* -------------------------------------------------------
* Scenery Editor X
* -------------------------------------------------------
* Copyright (c) 2025 Thomas Ray 
* Copyright (c) 2025 Coalition of Freeware Developers
* -------------------------------------------------------
* bindless_descriptor_manager.h
* -------------------------------------------------------
* Created: 27/8/2025
* -------------------------------------------------------
*/
#pragma once
#include <cstdint>
#include <mutex>
#include <unordered_map>
#include <vector>
#include <vulkan/vulkan.h>

/// -------------------------------------------------------

namespace SceneryEditorX
{
	/**
	 * @brief Minimal bindless descriptor set manager (Spartan-style foundation).
	 *
	 * Creates a single large descriptor set using descriptor indexing (VK_EXT_descriptor_indexing)
	 * so shaders can index resources (textures/samplers/storage buffers) by 32-bit index.
	 * This is an initial scaffold to migrate toward a Spartan-like rendering architecture
	 * without disturbing existing renderer code. Existing explicit descriptor allocations
	 * continue to function in parallel until the migration completes.
	 */
	class BindlessDescriptorManager : public RefCounted
	{
	public:
	    static void Init(uint32_t maxSampledImages	= 4096, uint32_t maxSamplers       = 512,
	                     uint32_t maxStorageImages  = 512, uint32_t maxStorageBuffers = 1024, uint32_t maxUniformBuffers = 256);
	    static void Shutdown();
	
	    // Allocation returns integer index usable in shaders.
	    static uint32_t RegisterSampledImage(VkImageView view, VkImageLayout layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
	    static uint32_t RegisterSampler(VkSampler sampler);
	    static uint32_t RegisterStorageImage(VkImageView view, VkImageLayout layout = VK_IMAGE_LAYOUT_GENERAL);
	    static uint32_t RegisterStorageBuffer(VkBuffer buffer, VkDeviceSize size, VkDeviceSize offset = 0);
	    static uint32_t RegisterUniformBuffer(VkBuffer buffer, VkDeviceSize size, VkDeviceSize offset = 0);
	
	    // Update existing slot (used when a texture/view or sampler is recreated without wanting a new index)
	    static void UpdateSampledImage(uint32_t index, VkImageView view, VkImageLayout layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
	    static void UpdateSampler(uint32_t index, VkSampler sampler);
	
	    static VkDescriptorSet GetDescriptorSet() { return s_DescriptorSet; }
	    static VkDescriptorSetLayout GetLayout() { return s_Layout; }
	
	    // Flush batched writes (called automatically on allocations, can be forced).
	    static void FlushPending();
	
	private:
	    struct PendingWrite
	    {
	        VkWriteDescriptorSet write{};
	        VkDescriptorImageInfo imageInfo{};
	        VkDescriptorBufferInfo bufferInfo{};
	    };
	
	    static void EnsureInitialized();
	    static void AllocateSet();
	
	    static inline Ref<BindlessDescriptorManager> s_Instance = nullptr;
	    static inline VkDescriptorSetLayout s_Layout = VK_NULL_HANDLE;
	    static inline VkDescriptorPool      s_Pool   = VK_NULL_HANDLE;
	    static inline VkDescriptorSet       s_DescriptorSet = VK_NULL_HANDLE;
	
	    static inline uint32_t s_MaxSampledImages  = 0;
	    static inline uint32_t s_MaxSamplers       = 0;
	    static inline uint32_t s_MaxStorageImages  = 0;
	    static inline uint32_t s_MaxStorageBuffers = 0;
	    static inline uint32_t s_MaxUniformBuffers = 0;
	
	    static inline uint32_t s_AllocatedSampledImages  = 0;
	    static inline uint32_t s_AllocatedSamplers       = 0;
	    static inline uint32_t s_AllocatedStorageImages  = 0;
	    static inline uint32_t s_AllocatedStorageBuffers = 0;
	    static inline uint32_t s_AllocatedUniformBuffers = 0;
	
	    static inline std::vector<PendingWrite> s_PendingWrites;
	    static inline std::mutex s_Mutex;
	};
	
}

/// -------------------------------------------------------
