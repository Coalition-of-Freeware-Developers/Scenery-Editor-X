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
#include "shader_compiler.h"
#include "shader_manager.h"
#include "shader_stage.h"
#include <SceneryEditorX/renderer/vulkan/descriptor.h>
#include <SceneryEditorX/renderer/vulkan/render_context.h>
#include <volk/volk.h>

// -------------------------------------------------------

namespace SceneryEditorX
{

	Shader::Shader(const char *shaderName) : SharedObject(), m_Name(shaderName ? shaderName : "UnnamedShader"), m_CompilationState(ShaderCompiler::State::Idle)
	{
		SetObjectName(m_Name);
	}

	Shader::Shader(const char *shaderName, const std::string &path, bool forceCompile) : SharedObject(), m_Name(shaderName ? shaderName : "UnnamedShader"), m_CompilationState(ShaderCompiler::State::Idle)
	{
		(void)forceCompile;
		SetObjectName(m_Name);
		m_Filepath = path;
	}

	ShaderCompiler::State Shader::GetCompilationState() const
	{
		return m_CompilationState.load();
	}

	bool Shader::IsCompiled() const
	{
		return m_CompilationState.load() == ShaderCompiler::State::Succeeded;
	}

	void Shader::SetCompilationState(const ShaderCompiler::State state)
	{
		m_CompilationState.store(state);
	}

	Shader::~Shader()
	{        
		// Guard against being called during the CRT static-destructor phase after
		// Renderer::Shutdown() has already torn down the RenderContext and the
		// spdlog logging infrastructure.  In that situation RenderContext::Get()
		// returns an invalid Ref, and calling any logging/assert macro would
		// dereference the already-destroyed logger (crash at offset 0x50 in
		// spdlog::logger::level_).  The VkDevice is already gone at that point,
		// so Vulkan cleanup is moot — return silently instead of crashing.
		const Ref<RenderContext> ctx = RenderContext::Get();
		if (!ctx.IsValid())
			return;

		const Ref<Device> device = ctx->GetDevice();
		if (!device.IsValid())
			return;

		for (VkDescriptorSetLayout layout : m_DescriptorSetLayouts)
		{
			if (layout != VK_NULL_HANDLE)
				vkDestroyDescriptorSetLayout(device->GetLogicalDevice(), layout, nullptr);
		}

		m_Filepath = "";
		m_Name = "";
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
			createInfo.bindingCount = static_cast<uint32_t>(descLayoutBindings.size());
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

	void Shader::AddShaderStage(StageType stage, const std::string& filepath)
	{
		m_Stages[stage] = CreateRef<ShaderStage>(stage, filepath);
		SEDX_CORE_ASSERT(m_Stages[stage] != nullptr, "Failed to create shader stage for '{}'", filepath);

		RebuildInputCache();
		SetCompilationState(ShaderCompiler::State::Succeeded);
	}
	
	Ref<ShaderStage> Shader::GetShaderStage(const StageType stage) const
	{
		SEDX_CORE_ASSERT(m_Stages.contains(stage), "Stage is not present");
		return m_Stages.at(stage);
	}
	
	bool Shader::HasStage(const StageType stage) const
	{
		return m_Stages.contains(stage);
	}

	void Shader::RebuildInputCache()
	{
		m_Input.clear();
		m_BindlessSets.clear();

		for (const auto& stageRef : m_Stages | std::views::values)
		{
			if (!stageRef)
				continue;

			for (const ShaderInput& input : stageRef->GetInput())
			{
				m_Input[input.set].push_back(input);
			}
		}
	}

	std::map<uint32_t, std::vector<VkDescriptorSetLayoutBinding>> Shader::GetDescriptorSetLayoutBindings()
	{
		std::map<uint32_t, std::vector<VkDescriptorSetLayoutBinding>> bindings;
		m_BindlessSets.clear();
	
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
	
				VkDescriptorSetLayoutBinding layoutBinding;
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

	std::vector<Descriptor> Shader::GetDescriptors()
	{
		std::vector<Descriptor> result;
		
		for (auto &inputs : m_Input | std::views::values)
		{
			for (const ShaderInput& input : inputs)
			{
				DescriptorType descType = DescriptorType::MaxEnum;
				switch (input.type)
				{
				case ShaderInputType::UniformBuffer:
				case ShaderInputType::UniformBufferSet: descType = DescriptorType::ConstantBuffer;
					break;
				case ShaderInputType::StorageBuffer:
				case ShaderInputType::StorageBufferSet: descType = DescriptorType::StructuredBuffer;
					break;
				case ShaderInputType::CombinedImageSampler:
				case ShaderInputType::Texture: descType = DescriptorType::Image;
					break;
				case ShaderInputType::StorageImage: descType = DescriptorType::TextureStorage; 
					break;
				default:	
					break;
				}
				
				if (descType == DescriptorType::MaxEnum)
					continue;
				
				DescriptorSpec spec{};
				spec.name        = input.debugName;
				spec.type        = descType;
				spec.layout      = Layout::ImageLayout::MaxEnum;
				spec.slot        = input.binding;
				spec.stage       = static_cast<uint32_t>(GetStage(input.stage));
				spec.structSize  = 0;
				spec.asArray     = input.count > 1;
				spec.arrayLength = input.count;
				result.emplace_back(spec);
			}
		}
		
		return result;
	}

} // namespace SceneryEditorX

// -------------------------------------------------------
