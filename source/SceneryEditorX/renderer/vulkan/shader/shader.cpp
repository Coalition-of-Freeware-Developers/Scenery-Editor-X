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
 * shader.cpp
 * -------------------------------------------------------
 * Created: 24/02/2026
 * -------------------------------------------------------
 */
#include "shader.h"
#include "shader_stage.h"
#include <SceneryEditorX/renderer/vulkan/render_context.h>
#include <volk/volk.h>

// -------------------------------------------------------

namespace SceneryEditorX
{

	Shader::~Shader()
	{        
	    const Ref<Device> device = RenderContext::Get()->GetDevice();
        SEDX_CORE_ASSERT(device.IsValid(), "Invalid device");

		for (VkDescriptorSetLayout layout : m_DescriptorSetLayouts)
		{
		    vkDestroyDescriptorSetLayout(device->GetLogicalDevice(), layout, nullptr);
		}
		m_Stages.clear();
	}

	void Shader::CreateDescriptorSetLayouts()
	{
		std::map<uint32_t, std::vector<VkDescriptorSetLayoutBinding>> bindings = GetDescriptorSetLayoutBindings();
	
		const Ref<Device> device = RenderContext::Get()->GetDevice();
        SEDX_CORE_ASSERT(device.IsValid(), "Invalid device");

		for (VkDescriptorSetLayout layout : m_DescriptorSetLayouts)
		{
		    vkDestroyDescriptorSetLayout(device->GetLogicalDevice(), layout, nullptr);
		}
		m_DescriptorSetLayouts.clear();
	
		for (auto& [set, descLayoutBindings] : bindings)
		{
			m_DescriptorSetLayouts.emplace_back();
			VkDescriptorSetLayout& descLayout = m_DescriptorSetLayouts.back();
	
			VkDescriptorSetLayoutCreateInfo createInfo = {};
			createInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
			createInfo.bindingCount = (uint32_t)descLayoutBindings.size();
			createInfo.pBindings = descLayoutBindings.data();
			createInfo.pNext = nullptr;
	
			VkDescriptorSetLayoutBindingFlagsCreateInfoEXT extendedInfo = {};
			VkDescriptorBindingFlags bindlessFlags = VK_DESCRIPTOR_BINDING_PARTIALLY_BOUND_BIT_EXT | VK_DESCRIPTOR_BINDING_VARIABLE_DESCRIPTOR_COUNT_BIT_EXT | VK_DESCRIPTOR_BINDING_UPDATE_AFTER_BIND_BIT_EXT;
	
			if (m_BindlessSets.contains(set))
			{
				extendedInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_BINDING_FLAGS_CREATE_INFO_EXT;
				extendedInfo.bindingCount = 1;
				extendedInfo.pBindingFlags = &bindlessFlags;
				extendedInfo.pNext = nullptr;
	
				createInfo.flags = VK_DESCRIPTOR_SET_LAYOUT_CREATE_UPDATE_AFTER_BIND_POOL_BIT_EXT;
				createInfo.pNext = &extendedInfo;
			}
	
			SEDX_VK_RESULT_ASSERT(vkCreateDescriptorSetLayout(device->GetLogicalDevice(), &createInfo, nullptr, &descLayout), "Unable to create descriptor set layout");
		}
	}
	
	void Shader::AddShaderStage(Stage stage, const std::string& filepath)
	{
		if (m_Stages.contains(stage))
		{
		    return;
		}
	
		m_Stages[stage] = CreateRef<ShaderStage>(stage, filepath);
	
		for (const ShaderInput& input : m_Stages[stage]->GetInput())
		{
		    m_Input[input.set].push_back(input);
		}
	}
	
	Ref<ShaderStage> Shader::GetShaderStage(const Stage stage)
	{
		SEDX_CORE_ASSERT(m_Stages.contains(stage) == false, "Stage is not present");
		return m_Stages[stage];
	}
	
	bool Shader::HasStage(const Stage stage)
	{
		if (!m_Stages.contains(stage))
		{
		    return false;
		}

		return true;
	}
	
	std::map<uint32_t, std::vector<VkDescriptorSetLayoutBinding>> Shader::GetDescriptorSetLayoutBindings()
	{
		std::map<uint32_t, std::vector<VkDescriptorSetLayoutBinding>> bindings;
	
		for (auto& [set, input] : m_Input)
		{
			bindings[set] = std::vector<VkDescriptorSetLayoutBinding>();
	
			// double pass to match the same input on multiple stages
			std::unordered_map<std::string, VkDescriptorSetLayoutBinding> inputs;
	
			for (const ShaderInput& i : input)
			{
				if (inputs.contains(i.debugName))
				{
					inputs[i.debugName].stageFlags |= GetStage(i.stage);
					continue;
				}
	
				VkDescriptorSetLayoutBinding layoutBinding = {};
				layoutBinding.binding = i.binding;
				layoutBinding.descriptorType = GetInputType(i.type);
	
				if (i.count == 0)
				{
					m_BindlessSets.insert(set);
					layoutBinding.descriptorCount = MAX_BINDLESS;
				}
				else
				{
				    layoutBinding.descriptorCount = i.count;
				}
	
				layoutBinding.stageFlags = GetStage(i.stage);
				layoutBinding.pImmutableSamplers = nullptr;
				inputs[i.debugName] = layoutBinding;
			}
	
			for (auto &layoutBinding : inputs | std::views::values)
			{
			    bindings[set].push_back(layoutBinding);
			}
		}
	
		return bindings;
	}

}

// -------------------------------------------------------
