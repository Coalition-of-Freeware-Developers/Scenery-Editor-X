/**
* -------------------------------------------------------
* Scenery Editor X
* -------------------------------------------------------
* Copyright (c) 2025 Thomas Ray 
* Copyright (c) 2025 Coalition of Freeware Developers
* -------------------------------------------------------
* bindless_descriptor_manager.cpp
* -------------------------------------------------------
* Created: 27/8/2025
* -------------------------------------------------------
*/
#include "bindless_descriptor_manager.h"
#include "render_context.h"
#include "vulkan/vk_device.h"
#include "vulkan/vk_util.h"
#include <algorithm>

/// -------------------------------------------------------

namespace SceneryEditorX
{
	
	static VkDescriptorSetLayoutBinding MakeBinding(uint32_t binding, VkDescriptorType type, uint32_t count, VkShaderStageFlags stages)
	{
	    VkDescriptorSetLayoutBinding b{};
	    b.binding = binding;
	    b.descriptorType = type;
	    b.descriptorCount = count;
	    b.stageFlags = stages;
	    b.pImmutableSamplers = nullptr;
	    return b;
	}
	
	void BindlessDescriptorManager::Init(uint32_t maxSampledImages, uint32_t maxSamplers, uint32_t maxStorageImages, uint32_t maxStorageBuffers, uint32_t maxUniformBuffers)
	{
	    if (s_Instance) return;
	    s_Instance = CreateRef<BindlessDescriptorManager>();
	
	    s_MaxSampledImages  = std::max(1u, maxSampledImages);
	    s_MaxSamplers       = std::max(1u, maxSamplers);
	    s_MaxStorageImages  = std::max(1u, maxStorageImages);
	    s_MaxStorageBuffers = std::max(1u, maxStorageBuffers);
	    s_MaxUniformBuffers = std::max(1u, maxUniformBuffers);
	
	    AllocateSet();
	    SEDX_CORE_INFO_TAG("BINDLESS", "Bindless descriptor set initialized: SI={} SAMP={} STO_IMG={} STO_BUF={} UBO={} ",
	                       s_MaxSampledImages, s_MaxSamplers, s_MaxStorageImages, s_MaxStorageBuffers, s_MaxUniformBuffers);
	}
	
	void BindlessDescriptorManager::Shutdown()
	{
	    if (!s_Instance)
			return;

	    auto device = RenderContext::GetCurrentDevice()->GetDevice();

	    if (s_DescriptorSet)
	    {
	        /* freed with pool */
	    }

	    if (s_Pool)
	    {
	        vkDestroyDescriptorPool(device, s_Pool, nullptr);
	        s_Pool = VK_NULL_HANDLE;
	    }

	    if (s_Layout)
	    {
	        vkDestroyDescriptorSetLayout(device, s_Layout, nullptr);
	        s_Layout = VK_NULL_HANDLE;
	    }

	    s_Instance.Reset();
	    s_PendingWrites.clear();
	}
	
	void BindlessDescriptorManager::EnsureInitialized()
	{
	    SEDX_CORE_ASSERT(s_Instance != nullptr, "BindlessDescriptorManager not initialized");
	}

    /// -------------------------------------------------------

	void BindlessDescriptorManager::AllocateSet()
	{
	    auto device = RenderContext::GetCurrentDevice()->GetDevice();
	
	    std::vector<VkDescriptorSetLayoutBinding> bindings;
	    bindings.reserve(5);
	    bindings.push_back(MakeBinding(0, VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE,  s_MaxSampledImages,  VK_SHADER_STAGE_ALL));
	    bindings.push_back(MakeBinding(1, VK_DESCRIPTOR_TYPE_SAMPLER,        s_MaxSamplers,       VK_SHADER_STAGE_ALL));
	    bindings.push_back(MakeBinding(2, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,  s_MaxStorageImages,  VK_SHADER_STAGE_ALL));
	    bindings.push_back(MakeBinding(3, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, s_MaxStorageBuffers, VK_SHADER_STAGE_ALL));
	    bindings.push_back(MakeBinding(4, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, s_MaxUniformBuffers, VK_SHADER_STAGE_ALL));
	
	    // Descriptor indexing flags (assumes device created with appropriate features).
	    std::vector<VkDescriptorBindingFlags> bindingFlags(bindings.size(),
	        VK_DESCRIPTOR_BINDING_PARTIALLY_BOUND_BIT | VK_DESCRIPTOR_BINDING_UPDATE_AFTER_BIND_BIT | VK_DESCRIPTOR_BINDING_UPDATE_UNUSED_WHILE_PENDING_BIT);
	
	    VkDescriptorSetLayoutBindingFlagsCreateInfo bindingFlagsInfo{};
	    bindingFlagsInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_BINDING_FLAGS_CREATE_INFO;
	    bindingFlagsInfo.bindingCount = static_cast<uint32_t>(bindingFlags.size());
	    bindingFlagsInfo.pBindingFlags = bindingFlags.data();
	
	    VkDescriptorSetLayoutCreateInfo layoutInfo{};
	    layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	    layoutInfo.flags = VK_DESCRIPTOR_SET_LAYOUT_CREATE_UPDATE_AFTER_BIND_POOL_BIT;
	    layoutInfo.bindingCount = static_cast<uint32_t>(bindings.size());
	    layoutInfo.pBindings    = bindings.data();
	    layoutInfo.pNext        = &bindingFlagsInfo;
	
	    VK_CHECK_RESULT(vkCreateDescriptorSetLayout(device, &layoutInfo, nullptr, &s_Layout))

        /// -------------------------------------------------------

	    std::vector<VkDescriptorPoolSize> poolSizes;
	    poolSizes.reserve(5);
	    poolSizes.push_back({.type = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE,	.descriptorCount = s_MaxSampledImages });
	    poolSizes.push_back({.type = VK_DESCRIPTOR_TYPE_SAMPLER,			.descriptorCount = s_MaxSamplers });
	    poolSizes.push_back({.type = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,	.descriptorCount = s_MaxStorageImages });
	    poolSizes.push_back({.type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,	.descriptorCount = s_MaxStorageBuffers });
	    poolSizes.push_back({.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,	.descriptorCount = s_MaxUniformBuffers });
	
	    VkDescriptorPoolCreateInfo poolInfo{};
	    poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	    poolInfo.flags = VK_DESCRIPTOR_POOL_CREATE_UPDATE_AFTER_BIND_BIT;
	    poolInfo.poolSizeCount = (uint32_t)poolSizes.size();
	    poolInfo.pPoolSizes    = poolSizes.data();
	    poolInfo.maxSets       = 1; // single giant bindless set
	    VK_CHECK_RESULT(vkCreateDescriptorPool(device, &poolInfo, nullptr, &s_Pool))
	
	    VkDescriptorSetAllocateInfo allocInfo{};
	    allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	    allocInfo.descriptorSetCount = 1;
	    allocInfo.pSetLayouts = &s_Layout;
	    allocInfo.descriptorPool = s_Pool;
	    VK_CHECK_RESULT(vkAllocateDescriptorSets(device, &allocInfo, &s_DescriptorSet))
	}
	
	uint32_t BindlessDescriptorManager::RegisterSampledImage(VkImageView view, VkImageLayout layout)
	{
	    EnsureInitialized();
	    std::scoped_lock lock(s_Mutex);
	    SEDX_CORE_ASSERT(s_AllocatedSampledImages < s_MaxSampledImages, "Bindless sampled image overflow");
	    uint32_t index = s_AllocatedSampledImages++;

	    PendingWrite pw{};
	    pw.imageInfo.imageLayout = layout;
	    pw.imageInfo.imageView = view;
	    pw.write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	    pw.write.dstSet = s_DescriptorSet;
	    pw.write.dstBinding = 0;
	    pw.write.dstArrayElement = index;
	    pw.write.descriptorCount = 1;
	    pw.write.descriptorType = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
	    pw.write.pImageInfo = &pw.imageInfo;
	    s_PendingWrites.emplace_back(pw);

	    FlushPending();
	    return index;
	}
	
	uint32_t BindlessDescriptorManager::RegisterSampler(VkSampler sampler)
	{
	    EnsureInitialized();
	    std::scoped_lock lock(s_Mutex);
	    SEDX_CORE_ASSERT(s_AllocatedSamplers < s_MaxSamplers, "Bindless sampler overflow");
	    uint32_t index = s_AllocatedSamplers++;

	    PendingWrite pw{};
	    pw.imageInfo.sampler = sampler;
	    pw.write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	    pw.write.dstSet = s_DescriptorSet;
	    pw.write.dstBinding = 1;
	    pw.write.dstArrayElement = index;
	    pw.write.descriptorCount = 1;
	    pw.write.descriptorType = VK_DESCRIPTOR_TYPE_SAMPLER;
	    pw.write.pImageInfo = &pw.imageInfo;
	    s_PendingWrites.emplace_back(pw);

	    FlushPending();
	    return index;
	}
	
	uint32_t BindlessDescriptorManager::RegisterStorageImage(VkImageView view, VkImageLayout layout)
	{
	    EnsureInitialized();
	    std::scoped_lock lock(s_Mutex);
	    SEDX_CORE_ASSERT(s_AllocatedStorageImages < s_MaxStorageImages, "Bindless storage image overflow");
	    uint32_t index = s_AllocatedStorageImages++;

	    PendingWrite pw{};
	    pw.imageInfo.imageLayout = layout;
	    pw.imageInfo.imageView = view;
	    pw.write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	    pw.write.dstSet = s_DescriptorSet;
	    pw.write.dstBinding = 2;
	    pw.write.dstArrayElement = index;
	    pw.write.descriptorCount = 1;
	    pw.write.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
	    pw.write.pImageInfo = &pw.imageInfo;
	    s_PendingWrites.emplace_back(pw);

	    FlushPending();
	    return index;
	}
	
	uint32_t BindlessDescriptorManager::RegisterStorageBuffer(VkBuffer buffer, VkDeviceSize size, VkDeviceSize offset)
	{
	    EnsureInitialized();
	    std::scoped_lock lock(s_Mutex);
	    SEDX_CORE_ASSERT(s_AllocatedStorageBuffers < s_MaxStorageBuffers, "Bindless storage buffer overflow");
	    uint32_t index = s_AllocatedStorageBuffers++;

	    PendingWrite pw{};
	    pw.bufferInfo.buffer = buffer;
	    pw.bufferInfo.offset = offset;
	    pw.bufferInfo.range  = size;
	    pw.write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	    pw.write.dstSet = s_DescriptorSet;
	    pw.write.dstBinding = 3;
	    pw.write.dstArrayElement = index;
	    pw.write.descriptorCount = 1;
	    pw.write.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
	    pw.write.pBufferInfo = &pw.bufferInfo;
	    s_PendingWrites.emplace_back(pw);

	    FlushPending();
	    return index;
	}
	
	uint32_t BindlessDescriptorManager::RegisterUniformBuffer(VkBuffer buffer, VkDeviceSize size, VkDeviceSize offset)
	{
	    EnsureInitialized();
	    std::scoped_lock lock(s_Mutex);
	    SEDX_CORE_ASSERT(s_AllocatedUniformBuffers < s_MaxUniformBuffers, "Bindless uniform buffer overflow");
	    uint32_t index = s_AllocatedUniformBuffers++;

	    PendingWrite pw{};
	    pw.bufferInfo.buffer = buffer;
	    pw.bufferInfo.offset = offset;
	    pw.bufferInfo.range  = size;
	    pw.write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	    pw.write.dstSet = s_DescriptorSet;
	    pw.write.dstBinding = 4;
	    pw.write.dstArrayElement = index;
	    pw.write.descriptorCount = 1;
	    pw.write.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	    pw.write.pBufferInfo = &pw.bufferInfo;
	    s_PendingWrites.emplace_back(pw);

	    FlushPending();
	    return index;
	}
	
	void BindlessDescriptorManager::FlushPending()
	{
	    if (s_PendingWrites.empty()) return;
	    auto device = RenderContext::GetCurrentDevice()->GetDevice();
	
	    // Copy because vector elements contain pointers to their own members.
	    std::vector<VkWriteDescriptorSet> writes;
	    writes.reserve(s_PendingWrites.size());
	    for (auto &pw : s_PendingWrites) writes.push_back(pw.write);
	
	    vkUpdateDescriptorSets(device, static_cast<uint32_t>(writes.size()), writes.data(), 0, nullptr);
	    s_PendingWrites.clear();
	}
	
	void BindlessDescriptorManager::UpdateSampledImage(uint32_t index, VkImageView view, VkImageLayout layout)
	{
	    EnsureInitialized();
	    std::scoped_lock lock(s_Mutex);
	    SEDX_CORE_ASSERT(index < s_AllocatedSampledImages, "Bindless sampled image update out of range");

	    PendingWrite pw{};
	    pw.imageInfo.imageLayout = layout;
	    pw.imageInfo.imageView = view;
	    pw.write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	    pw.write.dstSet = s_DescriptorSet;
	    pw.write.dstBinding = 0;
	    pw.write.dstArrayElement = index;
	    pw.write.descriptorCount = 1;
	    pw.write.descriptorType = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
	    pw.write.pImageInfo = &pw.imageInfo;
	    s_PendingWrites.emplace_back(pw);

	    FlushPending();
	}
	
	void BindlessDescriptorManager::UpdateSampler(uint32_t index, VkSampler sampler)
	{
	    EnsureInitialized();
	    std::scoped_lock lock(s_Mutex);
	    SEDX_CORE_ASSERT(index < s_AllocatedSamplers, "Bindless sampler update out of range");

	    PendingWrite pw{};
	    pw.imageInfo.sampler = sampler;
	    pw.write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	    pw.write.dstSet = s_DescriptorSet;
	    pw.write.dstBinding = 1;
	    pw.write.dstArrayElement = index;
	    pw.write.descriptorCount = 1;
	    pw.write.descriptorType = VK_DESCRIPTOR_TYPE_SAMPLER;
	    pw.write.pImageInfo = &pw.imageInfo;
	    s_PendingWrites.emplace_back(pw);

	    FlushPending();
	}
	
}

/// -------------------------------------------------------
