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
 * descriptor.cpp
 * -------------------------------------------------------
 * Created: 09/02/2026
 * -------------------------------------------------------
 */
#include "descriptor.h"

#include "render_context.h"

#include <iostream>
#include <vector>
#include <volk/volk.h>

// -------------------------------------------------------

namespace SceneryEditorX
{
	Descriptor::Descriptor()
	{
        m_Device = RenderContext::Get()->GetDevice();
	}

	Descriptor::~Descriptor()
	{
	    if (m_Device.IsValid())
	    {
	        if (m_Pool != VK_NULL_HANDLE)
	        {
	            vkDestroyDescriptorPool(m_Device->GetLogicalDevice(), m_Pool, nullptr);
	            m_Pool = VK_NULL_HANDLE;
	        }
	        if (m_Layout != VK_NULL_HANDLE)
	        {
	            vkDestroyDescriptorSetLayout(m_Device->GetLogicalDevice(), m_Layout, nullptr);
	            m_Layout = VK_NULL_HANDLE;
	        }
	
	        m_Device = VK_NULL_HANDLE;
	    }
	}
	
	VkDescriptorSetLayout Descriptor::CreateLayout(uint32_t bindingCount) const
	{
	    VkDescriptorSetLayoutBinding binding{};
	    binding.binding = 0;
	    binding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	    binding.descriptorCount = bindingCount;
	    binding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
	    binding.pImmutableSamplers = nullptr;
	
	    VkDescriptorSetLayoutCreateInfo ci{};
	    ci.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	    ci.bindingCount = 1;
	    ci.pBindings = &binding;
	
	    VkDescriptorSetLayout layout = VK_NULL_HANDLE;
	    if (VkResult r = vkCreateDescriptorSetLayout(m_Device->GetLogicalDevice(), &ci, nullptr, &layout); r != VK_SUCCESS)
	    {
	        SEDX_CORE_ERROR_TAG("Descriptor", "vkCreateDescriptorSetLayout failed: {}", r);
	        return VK_NULL_HANDLE;
	    }
	    return layout;
	}
	
	VkDescriptorPool Descriptor::CreatePool(uint32_t descriptorCount) const
	{
	    VkDescriptorPoolSize poolSize{};
	    poolSize.type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	    poolSize.descriptorCount = descriptorCount;
	
	    VkDescriptorPoolCreateInfo ci{};
	    ci.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	    ci.maxSets = 1;
	    ci.poolSizeCount = 1;
	    ci.pPoolSizes = &poolSize;
	
	    VkDescriptorPool pool = VK_NULL_HANDLE;
	    if (VkResult r = vkCreateDescriptorPool(m_Device->GetLogicalDevice(), &ci, nullptr, &pool); r != VK_SUCCESS)
	    {
	        SEDX_CORE_ERROR_TAG("Descriptor","vkCreateDescriptorPool failed: {} ", r);
	        return VK_NULL_HANDLE;
	    }
	    return pool;
	}
	
	VkDescriptorSet Descriptor::AllocateAndWrite(VkDescriptorPool pool, VkDescriptorSetLayout layout, const std::vector<::VkDescriptorImageInfo> &imageInfos) const
	{
	    if (pool == VK_NULL_HANDLE)
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
	    allocInfo.descriptorPool = pool;
	    allocInfo.descriptorSetCount = 1;
	    allocInfo.pSetLayouts = &layout;
	
	    if (VkResult r = vkAllocateDescriptorSets(m_Device->GetLogicalDevice(), &allocInfo, &descriptorSet); r != VK_SUCCESS)
	    {
	        SEDX_CORE_ERROR_TAG("Descriptor", "vkAllocateDescriptorSets failed: {}", r);
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

	Descriptor::Descriptor(Descriptor &&other) noexcept : m_Device(other.m_Device), m_Pool(other.m_Pool), m_Layout(other.m_Layout)
	{
	    other.m_Device = VK_NULL_HANDLE;
	    other.m_Pool = VK_NULL_HANDLE;
	    other.m_Layout = VK_NULL_HANDLE;
	}
	
	Descriptor &Descriptor::operator=(Descriptor &&other) noexcept
	{
	    if (this != &other)
	    {
	        // destroy our existing
	        if (m_Device.IsValid())
	        {
	            if (m_Pool != VK_NULL_HANDLE)
	                vkDestroyDescriptorPool(m_Device->GetLogicalDevice(), m_Pool, nullptr);
	            if (m_Layout != VK_NULL_HANDLE)
	                vkDestroyDescriptorSetLayout(m_Device->GetLogicalDevice(), m_Layout, nullptr);
	        }
	        m_Device = other.m_Device;
	        m_Pool = other.m_Pool;
	        m_Layout = other.m_Layout;
	        other.m_Device = VK_NULL_HANDLE;
	        other.m_Pool = VK_NULL_HANDLE;
	        other.m_Layout = VK_NULL_HANDLE;
	    }
	    return *this;
	}
	
	/*
	VkDescriptorSetLayout Descriptor::createLayout(VkDevice device, uint32_t bindingCount) const
	{
	    VkDescriptorSetLayoutBinding binding{};
	    binding.binding = 0;
	    binding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	    binding.descriptorCount = bindingCount;
	    binding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
	    binding.pImmutableSamplers = nullptr;
	
	/*
	VkDescriptorSetLayout Descriptor::createLayout(VkDevice device, uint32_t bindingCount) const
	{
	    VkDescriptorSetLayoutBinding binding{};
	    binding.binding = 0;
	    binding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	    binding.descriptorCount = bindingCount;
	    binding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
	    binding.pImmutableSamplers = nullptr;
	
	    VkDescriptorSetLayoutCreateInfo ci{};
	    ci.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	    ci.bindingCount = 1;
	    ci.pBindings = &binding;
	    VkDescriptorSetLayoutCreateInfo ci{};
	    ci.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	    ci.bindingCount = 1;
	    ci.pBindings = &binding;
	
	    VkDescriptorSetLayout layout = VK_NULL_HANDLE;
	    VkResult r = vkCreateDescriptorSetLayout(device, &ci, nullptr, &layout);
	    if (r != VK_SUCCESS) {
	        std::cerr << "vkCreateDescriptorSetLayout failed: " << r << std::endl;
	        return VK_NULL_HANDLE;
	    }
	    return layout;
	}
	*/
}


