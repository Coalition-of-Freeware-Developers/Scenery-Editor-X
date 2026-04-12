/**
 * -------------------------------------------------------
 * Scenery Editor X
 * -------------------------------------------------------
 * Copyright (c) 2026 Thomas Ray
 * Copyright (c) 2026 Coalition of Freeware Developers
 * -------------------------------------------------------
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 * -------------------------------------------------------
 * descriptor_pool_manager.cpp
 * -------------------------------------------------------
 * Created: 06/04/2026
 * -------------------------------------------------------
 */
#include "descriptor_pool_manager.h"
#include "queue_manager.h"
#include "render_context.h"

#include <SceneryEditorX/logging/logging.hpp>
#include <volk/volk.h>

// -------------------------------------------------------

namespace SceneryEditorX
{
	static DescriptorPoolManager s_Instance;

	void DescriptorPoolManager::Init()
	{
		const Ref<Device> device = RenderContext::Get()->GetDevice();
		SEDX_CORE_ASSERT(device.IsValid(), "DescriptorPoolManager::Init requires a valid device");
		s_Instance.m_Device = device;
		s_Instance.CreatePool();
		SEDX_CORE_INFO_TAG("DescriptorPoolManager", "Shared descriptor pool created");
	}

	void DescriptorPoolManager::Shutdown()
	{
		if (s_Instance.m_Pool != VK_NULL_HANDLE && s_Instance.m_Device.IsValid())
		{
			vkDestroyDescriptorPool(s_Instance.m_Device->GetLogicalDevice(), s_Instance.m_Pool, nullptr);
			s_Instance.m_Pool = VK_NULL_HANDLE;
			SEDX_CORE_INFO_TAG("DescriptorPoolManager", "Shared descriptor pool destroyed");
		}
		s_Instance.m_Device = nullptr;
	}

	DescriptorPoolManager& DescriptorPoolManager::Get()
	{
		SEDX_CORE_ASSERT(s_Instance.m_Pool != VK_NULL_HANDLE, "DescriptorPoolManager::Get() called before Init()");
		return s_Instance;
	}

	VkDescriptorSet DescriptorPoolManager::Allocate(VkDescriptorSetLayout layout, uint32_t variableDescriptorCount)
	{
		// Defensive runtime checks: avoid dereferencing null device/pool in release builds.
		if (!m_Device.IsValid())
		{
			SEDX_CORE_ERROR_TAG("DescriptorPoolManager", "Allocate called before Init(): device is null");
			return VK_NULL_HANDLE;
		}

		if (m_Pool == VK_NULL_HANDLE)
		{
			SEDX_CORE_ERROR_TAG("DescriptorPoolManager", "Allocate called before Init(): descriptor pool is null");
			return VK_NULL_HANDLE;
		}

		SEDX_CORE_ASSERT(layout != VK_NULL_HANDLE, "Cannot allocate descriptor set from null layout");

		VkDescriptorSetVariableDescriptorCountAllocateInfo variableCountInfo{};
		variableCountInfo.sType              = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_VARIABLE_DESCRIPTOR_COUNT_ALLOCATE_INFO_EXT;
		variableCountInfo.descriptorSetCount = 1;
		variableCountInfo.pDescriptorCounts  = &variableDescriptorCount;

		VkDescriptorSetAllocateInfo allocInfo{};
		allocInfo.sType              = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
		allocInfo.descriptorPool     = m_Pool;
		allocInfo.descriptorSetCount = 1;
		allocInfo.pSetLayouts        = &layout;
		if (variableDescriptorCount > 0)
			allocInfo.pNext = &variableCountInfo;

		VkDescriptorSet set = VK_NULL_HANDLE;
		const VkResult result = vkAllocateDescriptorSets(m_Device->GetLogicalDevice(), &allocInfo, &set);
		if (result != VK_SUCCESS)
		{
			SEDX_CORE_ERROR_TAG("DescriptorPoolManager", "vkAllocateDescriptorSets failed: {}", static_cast<int>(result));
			return VK_NULL_HANDLE;
		}

		return set;
	}

	void DescriptorPoolManager::Free(VkDescriptorSet set)
	{
		SEDX_CORE_ASSERT(m_Device.IsValid(), "DescriptorPoolManager::Free() called with null device");
		if (set == VK_NULL_HANDLE || m_Pool == VK_NULL_HANDLE)
			return;

		if (!m_Device.IsValid())
			return;

		vkFreeDescriptorSets(m_Device->GetLogicalDevice(), m_Pool, 1, &set);
	}

	void DescriptorPoolManager::CreatePool()
	{
		SEDX_CORE_ASSERT(m_Device.IsValid(), "DescriptorPoolManager::CreatePool() called with null device");

		// Generous per-type counts sized for the whole application lifetime.
		// Covers dynamic descriptor sets, per-object material sets, and fixed-count bindings.
		constexpr uint32_t kSetCount = MAX_DESCRIPTOR_SET_COUNT;

		const VkDescriptorPoolSize poolSizes[] =
		{
			{.type = VK_DESCRIPTOR_TYPE_SAMPLER,						.descriptorCount = 32  * kSetCount },
			{.type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,		.descriptorCount = kSetCount + MAX_ARRAY_SIZE },
			{.type = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE,				.descriptorCount = MAX_ARRAY_SIZE + 32 * kSetCount },
			{.type = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,				.descriptorCount = MAX_ARRAY_SIZE },
			{.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,				.descriptorCount = 32  * kSetCount },
			{.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC,		.descriptorCount = 32  * kSetCount },
			{.type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,				.descriptorCount = 32  * kSetCount },
			{.type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC,		.descriptorCount = 32  * kSetCount },
			//{.type = VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR,	.descriptorCount = 32  * kSetCount },
		};

		VkDescriptorPoolCreateInfo poolInfo{};
		poolInfo.sType         = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
		poolInfo.flags         = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT |
								 VK_DESCRIPTOR_POOL_CREATE_UPDATE_AFTER_BIND_BIT;
		poolInfo.maxSets       = kSetCount;
		poolInfo.poolSizeCount = static_cast<uint32_t>(std::size(poolSizes));
		poolInfo.pPoolSizes    = poolSizes;

		const VkResult result = vkCreateDescriptorPool(m_Device->GetLogicalDevice(), &poolInfo, nullptr, &m_Pool);
		SEDX_CORE_ASSERT(result == VK_SUCCESS, "DescriptorPoolManager: failed to create shared descriptor pool");
	}

} // namespace SceneryEditorX

// -------------------------------------------------------
