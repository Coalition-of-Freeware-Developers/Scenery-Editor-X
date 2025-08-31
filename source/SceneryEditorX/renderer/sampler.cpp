/**
* -------------------------------------------------------
* Scenery Editor X
* -------------------------------------------------------
* Copyright (c) 2025 Thomas Ray 
* Copyright (c) 2025 Coalition of Freeware Developers
* -------------------------------------------------------
* sampler.cpp
* -------------------------------------------------------
* Created: 31/8/2025
* -------------------------------------------------------
*/
#include "sampler.h"

#include "render_context.h"

#include "vulkan/vk_util.h"

namespace SceneryEditorX
{
	void Sampler::CreateResource()
	{
        VkDevice device = RenderContext::Get()->GetLogicDevice()->GetDevice();
	    VkSamplerCreateInfo sampler_info = {};
	    sampler_info.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
	    sampler_info.magFilter = vulkan_filter[static_cast<uint32_t>(m_filter_mag)];
	    sampler_info.minFilter = vulkan_filter[static_cast<uint32_t>(m_filter_min)];
	    sampler_info.mipmapMode = vulkan_mipmap_mode[static_cast<uint32_t>(m_filter_mipmap)];
	    sampler_info.addressModeU = vulkan_sampler_address_mode[static_cast<uint32_t>(m_sampler_address_mode)];
	    sampler_info.addressModeV = vulkan_sampler_address_mode[static_cast<uint32_t>(m_sampler_address_mode)];
	    sampler_info.addressModeW = vulkan_sampler_address_mode[static_cast<uint32_t>(m_sampler_address_mode)];
	    sampler_info.anisotropyEnable = m_anisotropy != 0;
	    sampler_info.maxAnisotropy = m_anisotropy;
	    sampler_info.compareEnable = m_comparison_enabled ? VK_TRUE : VK_FALSE;
	    sampler_info.compareOp = vulkan_compare_operator[static_cast<uint32_t>(m_comparison_function)];
	    sampler_info.borderColor = VK_BORDER_COLOR_INT_TRANSPARENT_BLACK;
	    sampler_info.mipLodBias = m_mip_lod_bias;
	    sampler_info.minLod = 0.0f;
	    sampler_info.maxLod = FLT_MAX;
	
	    VK_CHECK_RESULT(vkCreateSampler(device, &sampler_info, nullptr, reinterpret_cast<VkSampler *>(&m_resource)))
	}
	
	Sampler::~Sampler()
	{
	    VulkanDevice::DeletionQueueAdd(ResourceType::Sampler, m_resource);
	}
}
