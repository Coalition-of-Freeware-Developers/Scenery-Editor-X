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
 * descriptor_impl.cpp
 * -------------------------------------------------------
 * Created: 09/02/2026
 * -------------------------------------------------------
 */
#include "descriptor_set.h"
#include "descriptor.h"
#include "render_context.h"
#include <iostream>
#include <vector>
#include <volk/volk.h>

// -------------------------------------------------------

namespace SceneryEditorX
{
	bool Descriptor::Init(uint32_t bindingCount, uint32_t descriptorCount)
	{
        m_Device = RenderContext::Get()->GetDevice();

	    // Create layout
	    VkDescriptorSetLayoutBinding binding{};
	    binding.binding = 0;
	    binding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	    binding.descriptorCount = bindingCount;
	    binding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
	    binding.pImmutableSamplers = nullptr;
	
	    VkDescriptorSetLayoutCreateInfo layoutCI{};
	    layoutCI.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	    layoutCI.bindingCount = 1;
	    layoutCI.pBindings = &binding;
	
	    VkResult r = vkCreateDescriptorSetLayout(m_Device->GetLogicalDevice(), &layoutCI, nullptr, &m_Layout);
	    if (r != VK_SUCCESS) 
	    {
	        SEDX_CORE_ERROR_TAG("DescriptorSet", "vkCreateDescriptorSetLayout failed: {}", r);
	        return false;
	    }
	
	    // Create pool
	    VkDescriptorPoolSize poolSize{};
	    poolSize.type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	    poolSize.descriptorCount = descriptorCount;
	
	    VkDescriptorPoolCreateInfo poolCI{};
	    poolCI.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	    poolCI.maxSets = 1;
	    poolCI.poolSizeCount = 1;
	    poolCI.pPoolSizes = &poolSize;
	
	    r = vkCreateDescriptorPool(m_Device->GetLogicalDevice(), &poolCI, nullptr, &m_Pool);
	    if (r != VK_SUCCESS) 
	    {
	        SEDX_CORE_ERROR_TAG("DescriptorSet", "vkCreateDescriptorPool failed: {}", r);
            vkDestroyDescriptorSetLayout(m_Device->GetLogicalDevice(), m_Layout, nullptr);
	        m_Layout = VK_NULL_HANDLE;
	        return false;
	    }
	
	    return true;
	}
	
	VkDescriptorSet Descriptor::AllocateAndWrite(const std::vector<VkDescriptorImageInfo>& imageInfos) const
	{
	    if (m_Pool == VK_NULL_HANDLE || m_Layout == VK_NULL_HANDLE)
			return VK_NULL_HANDLE;

	    uint32_t variableDescCount = static_cast<uint32_t>(imageInfos.size());
	
	    VkDescriptorSetVariableDescriptorCountAllocateInfo variableDescCountAI{};
	    variableDescCountAI.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_VARIABLE_DESCRIPTOR_COUNT_ALLOCATE_INFO_EXT;
	    variableDescCountAI.descriptorSetCount = 1;
	    variableDescCountAI.pDescriptorCounts = &variableDescCount;
	
	    VkDescriptorSet descriptorSet = VK_NULL_HANDLE;
	    VkDescriptorSetAllocateInfo allocInfo{};
	    allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	    allocInfo.pNext = &variableDescCountAI;
	    allocInfo.descriptorPool = m_Pool;
	    allocInfo.descriptorSetCount = 1;
	    allocInfo.pSetLayouts = &m_Layout;
	
	    VkResult r = vkAllocateDescriptorSets(m_Device->GetLogicalDevice(), &allocInfo, &descriptorSet);
	    if (r != VK_SUCCESS)
	    {
	        SEDX_CORE_ERROR_TAG("DescriptorSet", "vkAllocateDescriptorSets failed: {}", r);
	        return VK_NULL_HANDLE;
	    }
	
	    VkWriteDescriptorSet writeDescSet{};
	    writeDescSet.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	    writeDescSet.dstSet = descriptorSet;
	    writeDescSet.dstBinding = 0;
	    writeDescSet.descriptorCount = variableDescCount;
	    writeDescSet.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	    writeDescSet.pImageInfo = imageInfos.data();
	    vkUpdateDescriptorSets(m_Device->GetLogicalDevice(), 1, &writeDescSet, 0, nullptr);
	
	    return descriptorSet;
	}

    DescriptorSet::DescriptorSet(const std::vector<VkDescriptorImageInfo> &textureDescriptors)
    {
        Ref<Device> device = RenderContext::Get()->GetDevice();
        m_Device = device;

        Descriptor helper;
        m_Layout = helper.CreateLayout(static_cast<uint32_t>(textureDescriptors.size()));
        m_DescriptorPool = helper.CreatePool(static_cast<uint32_t>(textureDescriptors.size()));
		SEDX_CORE_ASSERT(m_DescriptorPool != VK_NULL_HANDLE, "DescriptorSet: failed to create descriptor pool");

        m_Set = helper.AllocateAndWrite(m_DescriptorPool, m_Layout, textureDescriptors);
        SEDX_CORE_ASSERT(m_Set != VK_NULL_HANDLE, "DescriptorSet: failed to allocate descriptor set");
    }

    DescriptorSet::~DescriptorSet()
    {
        if (!m_Destroyed)
        {
            // best-effort; explicit destroy(device) preferred
            Destroy();
        }
    }

    void DescriptorSet::Destroy()
    {
        if (!m_Destroyed)
        {
            if (m_DescriptorPool != VK_NULL_HANDLE)
            {
                vkDestroyDescriptorPool(m_Device->GetLogicalDevice(), m_DescriptorPool, nullptr);
                m_DescriptorPool = VK_NULL_HANDLE;
            }

            if (m_Layout != VK_NULL_HANDLE)
            {
                vkDestroyDescriptorSetLayout(m_Device->GetLogicalDevice(), m_Layout, nullptr);
                m_Layout = VK_NULL_HANDLE;
            }

            m_Destroyed = true;
        }
    }

} // namespace SceneryEditorX

// -------------------------------------------------------
