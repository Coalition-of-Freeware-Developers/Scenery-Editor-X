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
#include <unordered_map>
#include <SceneryEditorX/renderer/vulkan/descriptor.h>
#include <SceneryEditorX/renderer/vulkan/push_constant_buffer.h>
#include <SceneryEditorX/renderer/vulkan/render_context.h>
#include <volk/volk.h>

// -------------------------------------------------------

namespace SceneryEditorX
{
	/**
	 * @brief Converts ShaderInputType to text for diagnostics.
	 * @param type The ShaderInputType to convert.
	 * @return A string representing the shader input type.
	 */
	static const char* ShaderInputTypeToString(const ShaderInputType type)
	{
		switch (type)
		{
			case ShaderInputType::Texture:               return "Texture";
			case ShaderInputType::Sampler:               return "Sampler";
			case ShaderInputType::StorageImage:          return "StorageImage";
			case ShaderInputType::StorageBuffer:         return "StorageBuffer";
			case ShaderInputType::UniformBuffer:         return "UniformBuffer";
			case ShaderInputType::StorageBufferSet:      return "StorageBufferSet";
			case ShaderInputType::UniformBufferSet:      return "UniformBufferSet";
			case ShaderInputType::CombinedImageSampler:  return "CombinedImageSampler";
			case ShaderInputType::PushConstant:          return "PushConstant";
			default:                                     return "Unknown";
		}
	}

	/**
	 * @brief Returns true when reflection emitted a synthetic fallback name such as set0_binding7.
	 * @param name The reflection name to check.
	 * @return True if the name matches the synthetic pattern, false otherwise.
	 */
	static bool IsSyntheticReflectionName(const std::string& name)
	{
		return name.rfind("set", 0) == 0 && name.find("_binding") != std::string::npos;
	}

	/**
	 * @brief Converts DescriptorType to text for diagnostics.
	 * @param type The DescriptorType to convert.
	 * @return A string representing the descriptor type.
	 */
	static const char* DescriptorTypeToString(const DescriptorType type)
	{
		switch (type)
		{
			case DescriptorType::Sampler:             return "Sampler";
			case DescriptorType::Image:               return "Image";
			case DescriptorType::TextureStorage:      return "TextureStorage";
			case DescriptorType::ConstantBuffer:      return "ConstantBuffer";
			case DescriptorType::StructuredBuffer:    return "StructuredBuffer";
			case DescriptorType::PushConstantBuffer:  return "PushConstantBuffer";
			default:                                  return "Unknown";
		}
	}

	/**
	 * @brief Converts DescriptorType to Vulkan descriptor type text for diagnostics.
	 * @param type The DescriptorType to convert.
	 * @return A string representing the corresponding Vulkan descriptor type.
	 */
	static const char* DescriptorTypeToVkString(const DescriptorType type)
	{
		switch (type)
		{
			case DescriptorType::Sampler:             return "VK_DESCRIPTOR_TYPE_SAMPLER";
			case DescriptorType::Image:               return "VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER";
			case DescriptorType::TextureStorage:      return "VK_DESCRIPTOR_TYPE_STORAGE_IMAGE";
			case DescriptorType::ConstantBuffer:      return "VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER";
			case DescriptorType::StructuredBuffer:    return "VK_DESCRIPTOR_TYPE_STORAGE_BUFFER";
			default:                                  return "VK_DESCRIPTOR_TYPE_MAX_ENUM";
		}
	}

	Shader::Shader(const char *shaderName) : SharedObject(), m_Name(shaderName ? shaderName : "UnnamedShader"), m_CompilationState(ShaderCompiler::State::Idle)
	{
		SetObjectName(m_Name);
	}

	Shader::Shader(const Renderer_Shader shader) : SharedObject(), m_CompilationState(ShaderCompiler::State::Idle)
	{
	    m_Name = RendererShaderStageToString(shader);
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

	void Shader::AddShaderStage(const StageType stage, const std::string& filepath, const VertexType vertexType)
	{
		SetCompilationState(ShaderCompiler::State::Compiling);

		if (stage == StageType::Vertex)
		{
			m_VertexType = vertexType;
		}

		m_Stages[stage] = CreateRef<ShaderStage>(stage, filepath);
		if (m_Stages[stage] == nullptr)
		{
			SetCompilationState(ShaderCompiler::State::Failed);
			SEDX_CORE_ERROR_TAG("Shader", "Failed to create shader stage for '{}'", filepath);
			return;
		}

		if (m_Stages[stage]->GetHandle() == VK_NULL_HANDLE)
		{
			m_Stages.erase(stage);
			SetCompilationState(ShaderCompiler::State::Failed);
			SEDX_CORE_ERROR_TAG("Shader", "Shader stage '{}' produced null VkShaderModule for '{}'", static_cast<uint32_t>(stage), filepath);
			return;
		}

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

		// Dynamic descriptor set creation is for set 0 only. Bindless sets (space1+)
		// are provided by BindlessManager and bound separately.
		std::unordered_map<uint64_t, DescriptorSpec> bySetBindingAndType;
		std::unordered_map<uint64_t, DescriptorType> firstTypePerSetBinding;

		auto remap_known_set0_binding = [](const std::string& name, const uint32_t reflectedBinding) -> uint32_t
		{
			// Keep shader-side set-0 ABI stable even when reflection emits synthetic names/slots.
			if (name == "BufferFrame") return 200u;

			if (name == "tex_uav") return 100u;
			if (name == "tex_uav2") return 101u;
			if (name == "tex_uav3") return 102u;
			if (name == "tex_uav4") return 103u;
			if (name == "tex3d_uav") return 104u;
			if (name == "tex_uav_sss") return 105u;
			if (name == "tex_uav_mips") return 108u;
			if (name == "tex_uav_uint") return 130u;

			if (name == "indirect_draw_args") return 131u;
			if (name == "indirect_draw_data") return 132u;
			if (name == "indirect_draw_args_out") return 133u;
			if (name == "indirect_draw_data_out") return 134u;
			if (name == "indirect_draw_count") return 135u;

			if (name == "particle_buffer_a") return 136u;
			if (name == "particle_buffer_b") return 137u;
			if (name == "particle_counter") return 138u;
			if (name == "particle_emitter") return 139u;

			if (name == "tex_compress_in") return 140u;
			if (name == "tex_compress_out") return 141u;
			if (name == "tex_compress_out_bc1") return 142u;

			if (name == "visibility") return 143u;
			if (name == "g_atomic_counter") return 144u;

			return reflectedBinding;
		};

		for (auto& [set, inputs] : m_Input)
		{
			if (set != 0)
				continue;

			for (const ShaderInput& input : inputs)
			{
				DescriptorType descType = DescriptorType::MaxEnum;
				switch (input.type)
				{
					case ShaderInputType::UniformBuffer:
					case ShaderInputType::UniformBufferSet:
						descType = DescriptorType::ConstantBuffer;
						break;
					case ShaderInputType::StorageBuffer:
					case ShaderInputType::StorageBufferSet:
						descType = DescriptorType::StructuredBuffer;
						break;
					case ShaderInputType::CombinedImageSampler:
					case ShaderInputType::Texture:
						descType = DescriptorType::Image;
						break;
					case ShaderInputType::StorageImage:
						descType = DescriptorType::TextureStorage;
						break;
					default:
						break;
				}

				if (descType == DescriptorType::MaxEnum)
					continue;

				if (input.debugName == "BufferFrame")
				{
					descType = DescriptorType::ConstantBuffer;
				}
				else if (input.debugName.rfind("tex_uav", 0) == 0)
				{
					descType = DescriptorType::TextureStorage;
				}

				DescriptorSpec spec{};
				spec.name        = input.debugName;
				spec.type        = descType;
				spec.layout      = Layout::ImageLayout::MaxEnum;
				spec.slot        = remap_known_set0_binding(input.debugName, input.binding);
				spec.stage       = static_cast<uint32_t>(GetStage(input.stage));
				spec.structSize  = 0;
				spec.asArray     = input.count > 1;
				spec.arrayLength = input.count;

				if (m_Name == "grid" && spec.name == "BufferFrame" && spec.slot == 200)
				{
					// Compatibility bridge: some cached/legacy SPIR-V variants still expose
					// BufferFrame at set 0 binding 0. Emit both binding 200 and binding 0
					// for grid only so pipeline layout creation remains valid while caches converge.
					DescriptorSpec legacySpec = spec;
					legacySpec.slot = 0;

					const uint64_t legacyMergeKey = (static_cast<uint64_t>(set) << 32ull) | static_cast<uint64_t>(legacySpec.slot);
					const uint64_t legacyMergeKeyWithType = (legacyMergeKey << 8ull) | static_cast<uint64_t>(legacySpec.type);

					auto legacyTypeIt = firstTypePerSetBinding.find(legacyMergeKey);
					if (legacyTypeIt == firstTypePerSetBinding.end())
					{
						firstTypePerSetBinding.emplace(legacyMergeKey, legacySpec.type);
					}
					else if (legacyTypeIt->second != legacySpec.type)
					{
						SEDX_CORE_ERROR_TAG("Shader", "Descriptor reflection type conflict: shader='{}' set={} binding={} existingType='{}' incomingType='{}' (preserving conflict for hard fail)",
							m_Name,
							set,
							legacySpec.slot,
							DescriptorTypeToString(legacyTypeIt->second),
							DescriptorTypeToString(legacySpec.type));
					}

					const auto legacyIt = bySetBindingAndType.find(legacyMergeKeyWithType);
					if (legacyIt == bySetBindingAndType.end())
					{
						bySetBindingAndType.emplace(legacyMergeKeyWithType, legacySpec);
					}
					else
					{
						DescriptorSpec& existingLegacy = legacyIt->second;
						existingLegacy.stage |= legacySpec.stage;
						existingLegacy.asArray = existingLegacy.asArray || legacySpec.asArray;
						existingLegacy.arrayLength = std::max(existingLegacy.arrayLength, legacySpec.arrayLength);
					}
				}

				const uint64_t mergeKey = (static_cast<uint64_t>(set) << 32ull) | static_cast<uint64_t>(spec.slot);
				const uint64_t mergeKeyWithType = (mergeKey << 8ull) | static_cast<uint64_t>(spec.type);

				auto firstTypeIt = firstTypePerSetBinding.find(mergeKey);
				if (firstTypeIt == firstTypePerSetBinding.end())
				{
					firstTypePerSetBinding.emplace(mergeKey, spec.type);
				}
				else if (firstTypeIt->second != spec.type)
				{
					SEDX_CORE_ERROR_TAG("Shader", "Descriptor reflection type conflict: shader='{}' set={} binding={} existingType='{}' incomingType='{}' (preserving conflict for hard fail)",
						m_Name,
						set,
						spec.slot,
						DescriptorTypeToString(firstTypeIt->second),
						DescriptorTypeToString(spec.type));
				}

				const auto it = bySetBindingAndType.find(mergeKeyWithType);
				if (it == bySetBindingAndType.end())
				{
					SEDX_CORE_TRACE_TAG("Shader", "Descriptor map: shader='{}' stage={} set={} binding={} name='{}' reflected={} -> {} ({})",
						m_Name,
						static_cast<uint32_t>(input.stage),
						set,
						spec.slot,
						spec.name,
						ShaderInputTypeToString(input.type),
						DescriptorTypeToString(spec.type),
						DescriptorTypeToVkString(spec.type));
					bySetBindingAndType.emplace(mergeKeyWithType, spec);
					continue;
				}

				DescriptorSpec& existing = it->second;
				existing.stage |= spec.stage;
				existing.asArray = existing.asArray || spec.asArray;
				existing.arrayLength = std::max(existing.arrayLength, spec.arrayLength);

				if (IsSyntheticReflectionName(existing.name) && !IsSyntheticReflectionName(spec.name))
				{
					existing.name = spec.name;
				}
			}
		}

		result.reserve(bySetBindingAndType.size());
		for (const DescriptorSpec& spec : bySetBindingAndType | std::views::values)
		{
			result.emplace_back(spec);
		}

		std::ranges::sort(result, [](const Descriptor& lhs, const Descriptor& rhs)
		{
			return lhs.GetSlot() < rhs.GetSlot();
		});

		return result;
	}

} // namespace SceneryEditorX

// -------------------------------------------------------
