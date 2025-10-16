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

/// -------------------------------------------------------

namespace SceneryEditorX
{

    Sampler::Sampler(const SamplerSpec &samplerSpec, const std::string &debugName)
    {
		m_samplerSpec = samplerSpec;
        samplerName = debugName;
        CreateResource();
    }

    Sampler::~Sampler()
    {
    }

    /*
	Sampler::~Sampler()
	{
	    VulkanDevice::DeletionQueueAdd(ResourceType::Sampler, m_resource);
	}
	*/

	void Sampler::CreateResource()
    {
        VkDevice device = RenderContext::Get()->GetLogicDevice()->GetDevice();
        VkSamplerCreateInfo samplerInfo = {};
        samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
        samplerInfo.magFilter = VULKAN_FILTER[static_cast<uint32_t>(m_samplerSpec.filterMag)];
        samplerInfo.minFilter = VULKAN_FILTER[static_cast<uint32_t>(m_samplerSpec.filterMin)];
        samplerInfo.mipmapMode = VULKAN_MIPMAP_MODE[static_cast<uint32_t>(m_samplerSpec.filterMipMap)];
        samplerInfo.addressModeU = VULKAN_SAMPLER_ADDRESS_MODE[static_cast<uint32_t>(m_samplerSpec.samplerAddressMode)];
        samplerInfo.addressModeV = VULKAN_SAMPLER_ADDRESS_MODE[static_cast<uint32_t>(m_samplerSpec.samplerAddressMode)];
        samplerInfo.addressModeW = VULKAN_SAMPLER_ADDRESS_MODE[static_cast<uint32_t>(m_samplerSpec.samplerAddressMode)];
        samplerInfo.anisotropyEnable = m_samplerSpec.anisotropy != 0;
        samplerInfo.maxAnisotropy = m_samplerSpec.anisotropy;
        samplerInfo.compareEnable = m_samplerSpec.comparisonEnabled ? VK_TRUE : VK_FALSE;
        samplerInfo.compareOp = VULKAN_COMPARE_OPERATOR[static_cast<uint32_t>(m_samplerSpec.comparisonFunction)];
        samplerInfo.borderColor = VK_BORDER_COLOR_INT_TRANSPARENT_BLACK;
        samplerInfo.mipLodBias = m_samplerSpec.mipBias;
        samplerInfo.minLod = 0.0f;
        samplerInfo.maxLod = FLT_MAX;

        VK_CHECK_RESULT(vkCreateSampler(device, &samplerInfo, nullptr, reinterpret_cast<VkSampler *>(&m_resource)))
    }

}


/// -------------------------------------------------------
