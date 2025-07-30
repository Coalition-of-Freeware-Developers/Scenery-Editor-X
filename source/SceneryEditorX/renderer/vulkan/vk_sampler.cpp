/**
* -------------------------------------------------------
* Scenery Editor X
* -------------------------------------------------------
* Copyright (c) 2025 Thomas Ray 
* Copyright (c) 2025 Coalition of Freeware Developers
* -------------------------------------------------------
* vk_sampler.cpp
* -------------------------------------------------------
* Created: 26/7/2025
* -------------------------------------------------------
*/
#include <SceneryEditorX/renderer/render_context.h>
#include <SceneryEditorX/renderer/vulkan/vk_sampler.h>
#include <SceneryEditorX/renderer/vulkan/vk_util.h>

/// -------------------------------------------------------

namespace SceneryEditorX
{

	VkDescriptorSetAllocateInfo DescriptorSetAllocInfo(const VkDescriptorSetLayout* layouts, uint32_t count, VkDescriptorPool pool)
	{
		VkDescriptorSetAllocateInfo info{};
		info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
		info.pSetLayouts = layouts;
		info.descriptorSetCount = count;
		info.descriptorPool = pool;
		return info;
	}

	VkSampler CreateSampler(const VkSamplerCreateInfo &samplerCreateInfo)
	{
        const VkDevice vulkanDevice = RenderContext::GetCurrentDevice()->GetDevice();
		VkSampler sampler;
		VK_CHECK_RESULT(vkCreateSampler(vulkanDevice, &samplerCreateInfo, nullptr, &sampler))

		Utils::GetResourceAllocationCounts().Samplers++;
		return sampler;
	}

	void DestroySampler(VkSampler sampler)
	{
        const VkDevice vulkanDevice = RenderContext::GetCurrentDevice()->GetDevice();
		vkDestroySampler(vulkanDevice, sampler, nullptr);

		Utils::GetResourceAllocationCounts().Samplers--;
	}

}

/// -------------------------------------------------------
