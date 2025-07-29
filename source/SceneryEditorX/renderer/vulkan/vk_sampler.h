/**
* -------------------------------------------------------
* Scenery Editor X
* -------------------------------------------------------
* Copyright (c) 2025 Thomas Ray 
* Copyright (c) 2025 Coalition of Freeware Developers
* -------------------------------------------------------
* vk_sampler.h
* -------------------------------------------------------
* Created: 26/7/2025
* -------------------------------------------------------
*/
#pragma once
#include <vulkan/vulkan.h>

/// -------------------------------------------------------

namespace SceneryEditorX
{

	VkDescriptorSetAllocateInfo DescriptorSetAllocInfo(const VkDescriptorSetLayout* layouts, uint32_t count = 1, VkDescriptorPool pool = nullptr);

	VkSampler CreateSampler(const VkSamplerCreateInfo &samplerCreateInfo);
	void DestroySampler(VkSampler sampler);

}

/// -------------------------------------------------------
