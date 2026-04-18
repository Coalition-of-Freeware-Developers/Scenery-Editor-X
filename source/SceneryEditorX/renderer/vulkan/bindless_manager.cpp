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
 * bindless_manager.h
 * -------------------------------------------------------
 * Created: 06/04/2026
 * -------------------------------------------------------
 */
#include "bindless_manager.h"
#include "descriptor_pool_manager.h"
#include "debug/graphics_debug.h"
#include <algorithm>
#include <SceneryEditorX/renderer/renderer.h>
#include <SceneryEditorX/renderer/vulkan/render_context.h>
#include <SceneryEditorX/renderer/vulkan/shader/shader.h>

 // --------------------------------------------------------------

namespace SceneryEditorX
{
	static std::vector<VkDescriptorPool> s_DescriptorPools;
	static uint32_t s_Allocated_DescriptorSets = 0;

	// lookup table indexed by BindlessResource
	static const BindlessManager::ResourceConfig CONFIGS[] =
	{
		{ VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE,  SHADER_REGISTER_SHIFT_T, 15, MAX_ARRAY_SIZE,		"material_textures"   }, // MaterialTextures
		{ VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, SHADER_REGISTER_SHIFT_T, 16, 1,                  "material_parameters" }, // MaterialParameters
		{ VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, SHADER_REGISTER_SHIFT_T, 17, 1,                  "light_parameters"    }, // LightParameters
		{ VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, SHADER_REGISTER_SHIFT_T, 18, 1,                  "aabbs"               }, // Aabbs
		{ VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, SHADER_REGISTER_SHIFT_T, 19, 1,                  "draw_data"           }, // DrawData
		{ VK_DESCRIPTOR_TYPE_SAMPLER,        SHADER_REGISTER_SHIFT_S, 0,  1,                  "samplers_comparison" }, // SamplersComparison
		{ VK_DESCRIPTOR_TYPE_SAMPLER,        SHADER_REGISTER_SHIFT_S, 1,  8,                  "samplers_regular"    }, // SamplersRegular
		{ VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, SHADER_REGISTER_SHIFT_T, 20, 1,                  "geometry_vertices"   }, // GeometryVertices
		{ VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, SHADER_REGISTER_SHIFT_T, 22, 1,                  "geometry_indices"    }, // GeometryIndices
		{ VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, SHADER_REGISTER_SHIFT_T, 23, 1,                  "instances"           }, // Instances
	};

	/**
	 * @brief Descriptor pool sizes for the bindless manager.
	 */
	static std::array<VkDescriptorPoolSize, 7> s_PoolSizes =
	{
		VkDescriptorPoolSize{ VK_DESCRIPTOR_TYPE_SAMPLER,                    32 * MAX_DESCRIPTOR_SET_COUNT },
		VkDescriptorPoolSize{ VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE,              MAX_ARRAY_SIZE + 32 * MAX_DESCRIPTOR_SET_COUNT },
		VkDescriptorPoolSize{ VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,              MAX_ARRAY_SIZE * MAX_DESCRIPTOR_SET_COUNT },
		VkDescriptorPoolSize{ VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,             32 * MAX_DESCRIPTOR_SET_COUNT },
		VkDescriptorPoolSize{ VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC,     32 * MAX_DESCRIPTOR_SET_COUNT },
		VkDescriptorPoolSize{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC,     32 * MAX_DESCRIPTOR_SET_COUNT },
		VkDescriptorPoolSize{ VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR, 32 * MAX_DESCRIPTOR_SET_COUNT }
	};

	static_assert(std::size(CONFIGS) == static_cast<size_t>(BindlessResource::MaxEnum), "config table size mismatch");
	
	// Storage
	static std::array<VkDescriptorSet, static_cast<uint32_t>(BindlessResource::MaxEnum)> s_Sets          = {};
	static std::array<VkDescriptorSetLayout, static_cast<uint32_t>(BindlessResource::MaxEnum)> s_Layouts = {};
	static std::unordered_map<uint64_t, DescriptorSet> s_DescriptorSets;
	static std::unordered_map<uint64_t, std::vector<Descriptor>> s_DescriptorCache;

	BindlessManager::BindlessManager() : InheritanceBundle<RefCounted, IResource>(ResourceType::DescriptorSet)
	{
		Ref<RenderContext> context = RenderContext::Get();
		m_Device = context->GetLogicalDevice();
	}
	
	BindlessManager::~BindlessManager()
	{

	}
	
	void BindlessManager::Init()
	{
		if (!s_DescriptorPools.empty())
			return;

		AddToDescriptorPool();

		for (uint32_t i = 0; i < static_cast<uint32_t>(BindlessResource::MaxEnum); i++)
		{
			CreateSetLayout(static_cast<BindlessResource>(i));
		}
	}

	void BindlessManager::AddToDescriptorPool()
	{
		const Ref<Device> device = RenderContext::Get()->GetLogicalDevice();
		SEDX_CORE_ASSERT(device != nullptr, "BindlessManager requires a valid device to create descriptor pools");

		// Initialise the shared DescriptorPoolManager if it hasn't been done yet.
		// The pool created here is also used by DescriptorSet for dynamic sets.
		if (s_DescriptorPools.empty())
		{
			DescriptorPoolManager::Init();
			s_DescriptorPools.push_back(DescriptorPoolManager::Get().GetPool());
			s_Allocated_DescriptorSets = 0;
		}
	}

	void BindlessManager::Shutdown()
	{
		const Ref<Device> device = RenderContext::Get()->GetLogicalDevice();
		if (!device.IsValid())
			return;

		for (VkDescriptorSetLayout& layout : s_Layouts)
		{
			if (layout != VK_NULL_HANDLE)
			{
				vkDestroyDescriptorSetLayout(device->GetLogicalDevice(), layout, nullptr);
				layout = VK_NULL_HANDLE;
			}
		}

		s_Sets.fill(VK_NULL_HANDLE);

		// The shared pool is owned by DescriptorPoolManager; shut it down here so it is
		// destroyed before the device is torn down.
		DescriptorPoolManager::Shutdown();

		s_DescriptorPools.clear();
		s_Allocated_DescriptorSets = 0;
	}
	
	uint32_t BindlessManager::Allocate(BindlessResource type)
	{
		const uint32_t typeIndex = static_cast<uint32_t>(type);
		const ResourceConfig& cfg = CONFIGS[typeIndex];
		ResourceAllocationState& state = m_Resources[typeIndex];

		if (!state.freeIndices.empty())
		{
			const uint32_t index = state.freeIndices.back();
			state.freeIndices.pop_back();
			return index;
		}

		if (state.nextIndex >= cfg.count)
		{
			SEDX_CORE_ERROR_TAG("BindlessManager", "Bindless allocation overflow for '{}' (count={})", cfg.name, cfg.count);
			return INVALID_VK_INDEX; 
		}

		return state.nextIndex++;
	}

	std::vector<Descriptor> BindlessManager::GetDescriptorCache(uint64_t hash)
	{
		(void)hash;
		return {};
	}

	void BindlessManager::GetDescriptorsFromPipelineState(PipelineState& pipelineState, SceneryEditorX::Descriptor* outDescriptors, size_t& outCount)
	{
		pipelineState.Prepare();
		
		uint64_t pipeline_state_hash = pipelineState.GetHash();
		auto cached_descriptors_it = s_DescriptorCache.find(pipeline_state_hash);
		
		static Descriptor static_buffer[256];
		size_t static_size = 0;
		
		if (cached_descriptors_it != s_DescriptorCache.end())
		{
			const auto& cached = cached_descriptors_it->second;
			SEDX_CORE_ASSERT(cached.size() <= 256);
		
			for (size_t i = 0; i < cached.size(); ++i)
			{
				static_buffer[i] = cached[i];
			}
			static_size = cached.size();
		}
		else
		{
			static_size = 0;
		
			auto merge_descriptors = [&](const std::vector<Descriptor>& src)
			{
				for (const auto& d : src)
				{
					bool merged = false;

					for (size_t i = 0; i < static_size; ++i)
					{
						if (static_buffer[i].GetSlot() == d.GetSlot())
						{
							static_buffer[i].SetStage(static_buffer[i].GetStage() | d.GetStage());
							merged = true;
							break;
						}
					}
		
					if (!merged)
					{
						SEDX_CORE_ASSERT(static_size < 256);
						static_buffer[static_size++] = d;
					}
				}
			};
		
			if (pipelineState.IsCompute())
			{
				SEDX_CORE_ASSERT(pipelineState.shaders[static_cast<uint32_t>(StageType::Compute)] && pipelineState.shaders[static_cast<uint32_t>(StageType::Compute)]->IsCompiled());
				merge_descriptors(pipelineState.shaders[static_cast<uint32_t>(StageType::Compute)]->GetDescriptors());
			}
			else if (pipelineState.IsGraphics())
			{
				SEDX_CORE_ASSERT(pipelineState.shaders[static_cast<uint32_t>(StageType::Vertex)] && pipelineState.shaders[static_cast<uint32_t>(StageType::Vertex)]->IsCompiled());
				merge_descriptors(pipelineState.shaders[static_cast<uint32_t>(StageType::Vertex)]->GetDescriptors());
				
				if (pipelineState.shaders[static_cast<uint32_t>(StageType::Fragment)] && pipelineState.shaders[static_cast<uint32_t>(StageType::Fragment)]->IsCompiled())
				{
					merge_descriptors(pipelineState.shaders[static_cast<uint32_t>(StageType::Fragment)]->GetDescriptors());
				}
		
				if (pipelineState.shaders[static_cast<uint32_t>(StageType::TessellationControl)] && pipelineState.shaders[static_cast<uint32_t>(StageType::TessellationControl)]->IsCompiled())
				{
					merge_descriptors(pipelineState.shaders[static_cast<uint32_t>(StageType::TessellationControl)]->GetDescriptors());
				}
		
				if (pipelineState.shaders[static_cast<uint32_t>(StageType::TessellationEvaluation)] && pipelineState.shaders[static_cast<uint32_t>(StageType::TessellationEvaluation)]->IsCompiled())
				{
					merge_descriptors(pipelineState.shaders[static_cast<uint32_t>(StageType::TessellationEvaluation)]->GetDescriptors());
				}
			}
		
			// sort by slot
			std::sort(static_buffer, static_buffer + static_size, [](const Descriptor& a, const Descriptor& b)
			{
				return a.GetSlot() < b.GetSlot();
			});
		
			// cache as vector (first-time allocation unavoidable)
			s_DescriptorCache[pipeline_state_hash] = std::vector<Descriptor>(static_buffer, static_buffer + static_size);
		}
		
		// copy to output array
		outCount = static_size;
		for (size_t i = 0; i < static_size; ++i)
		{
			outDescriptors[i] = static_buffer[i];
		}
	}

	void BindlessManager::Free(BindlessResource type, uint32_t index)
	{
		const uint32_t typeIndex = static_cast<uint32_t>(type);
		const ResourceConfig& cfg = CONFIGS[typeIndex];
		ResourceAllocationState& state = m_Resources[typeIndex];

		if (index == INVALID_VK_INDEX || index >= cfg.count)
			return;

		if (std::ranges::find(state.freeIndices, index) != state.freeIndices.end())
			return;

		state.freeIndices.push_back(index);
	}
	
	void BindlessManager::UpdateBuffer(BindlessResource type, const Buffer *buffer)
	{
		const Ref<Device> device = RenderContext::Get()->GetDevice();
		if (!buffer)
			return;

		if (!device.IsValid())
			return;

		const uint32_t index = static_cast<uint32_t>(type);
		if (s_Sets[index] == VK_NULL_HANDLE)
			return;

		VkDescriptorBufferInfo buffer_info = {};
		buffer_info.buffer = buffer->Get();
		buffer_info.offset = 0;
		buffer_info.range  = buffer->GetObjectSize();

		VkWriteDescriptorSet write = {};
		write.sType           = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		write.dstSet          = s_Sets[index];
		write.dstBinding      = GetBinding(type);
		write.dstArrayElement = 0;
		write.descriptorType  = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
		write.descriptorCount = 1;
		write.pBufferInfo     = &buffer_info;

		vkUpdateDescriptorSets(device->GetLogicalDevice(), 1, &write, 0, nullptr);
	}
	
	void BindlessManager::UpdateSamplers(BindlessResource type, const Ref<Sampler> *samplers, uint32_t count)
	{
		const Ref<Device> device = RenderContext::Get()->GetDevice();
		if (samplers == nullptr || count == 0)
			return;

		if (!device.IsValid())
			return;

		const uint32_t index = static_cast<uint32_t>(type);
		if (s_Sets[index] == VK_NULL_HANDLE)
			return;

		// max sampler count is small enough for the stack
		constexpr uint32_t maxSamplers = 16;
		SEDX_CORE_ASSERT(count <= maxSamplers);
		VkDescriptorImageInfo imageInfos[maxSamplers] = {};
		for (uint32_t i = 0; i < count; ++i)
		{
			if (!samplers[i])
				continue;

			imageInfos[i].sampler = samplers[i]->Get();
		}

		VkWriteDescriptorSet write = {};
		write.sType           = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		write.dstSet          = s_Sets[index];
		write.dstBinding      = GetBinding(type);
		write.dstArrayElement = 0;
		write.descriptorType  = VK_DESCRIPTOR_TYPE_SAMPLER;
		write.descriptorCount = count;
		write.pImageInfo      = imageInfos;

		vkUpdateDescriptorSets(device->GetLogicalDevice(), 1, &write, 0, nullptr);
	}
	
	void BindlessManager::UpdateImages(const std::array<ImageResource *, MAX_ARRAY_SIZE> *imageArrays)
	{
		const Ref<Device> device = RenderContext::Get()->GetDevice();
		if (!imageArrays)
			return;

		if (!device.IsValid())
			return;

		const uint32_t index      = static_cast<uint32_t>(BindlessResource::MaterialTextures);
		const ResourceConfig& cfg = CONFIGS[index];
		if (s_Sets[index] == VK_NULL_HANDLE)
			return;

		thread_local std::vector<VkDescriptorImageInfo> imageInfos;
		imageInfos.resize(cfg.count);

		void* fallback = Renderer::GetStandardTexture(Renderer_StandardTexture::Checkerboard)->GetShaderResourceView();

		for (uint32_t i = 0; i < cfg.count; ++i)
		{
			ImageResource* img			= (*imageArrays)[i];
			void* srv					= (img && img->GetShaderResourceView()) ? img->GetShaderResourceView() : fallback;
			imageInfos[i].imageView		= static_cast<VkImageView>(srv);
			imageInfos[i].imageLayout	= VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		}

		VkWriteDescriptorSet write = {};
		write.sType           = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		write.dstSet          = s_Sets[index];
		write.dstBinding      = GetBinding(BindlessResource::MaterialTextures);
		write.dstArrayElement = 0;
		write.descriptorType  = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
		write.descriptorCount = cfg.count;
		write.pImageInfo      = imageInfos.data();

		vkUpdateDescriptorSets(device->GetLogicalDevice(), 1, &write, 0, nullptr);
	}
	
	void BindlessManager::CreateSetLayout(BindlessResource type)
	{
		const Ref<Device> device = RenderContext::Get()->GetLogicalDevice();
		SEDX_CORE_ASSERT(device != nullptr, "BindlessManager requires a valid device to create set layouts");

		if (s_DescriptorPools.empty())
			AddToDescriptorPool();

		uint32_t index              = static_cast<uint32_t>(type);
		const ResourceConfig& cfg   = CONFIGS[index];
		uint32_t binding            = cfg.registerShift + cfg.slot;

		// layout
		VkDescriptorSetLayoutBinding layout_binding = {};
		layout_binding.binding                      = binding;
		layout_binding.descriptorType               = cfg.type;
		layout_binding.descriptorCount              = cfg.count;
		layout_binding.stageFlags                   = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT |
													  VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT |
													  VK_SHADER_STAGE_FRAGMENT_BIT | VK_SHADER_STAGE_COMPUTE_BIT;

		VkDescriptorBindingFlags binding_flags = VK_DESCRIPTOR_BINDING_PARTIALLY_BOUND_BIT | VK_DESCRIPTOR_BINDING_VARIABLE_DESCRIPTOR_COUNT_BIT;
		/*if (cfg.count > 1)
		{
			binding_flags |= VK_DESCRIPTOR_BINDING_VARIABLE_DESCRIPTOR_COUNT_BIT;
		}*/

		VkDescriptorSetLayoutBindingFlagsCreateInfo binding_flags_info = {};
		binding_flags_info.sType         = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_BINDING_FLAGS_CREATE_INFO;
		binding_flags_info.bindingCount  = 1;
		binding_flags_info.pBindingFlags = &binding_flags;

		VkDescriptorSetLayoutCreateInfo layout_info = {};
		layout_info.sType        = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
		layout_info.bindingCount = 1;
		layout_info.pBindings    = &layout_binding;
		layout_info.flags        = VK_DESCRIPTOR_SET_LAYOUT_CREATE_UPDATE_AFTER_BIND_POOL_BIT_EXT;
		layout_info.pNext        = &binding_flags_info;

		SEDX_VK_RESULT_ASSERT(vkCreateDescriptorSetLayout(device->GetLogicalDevice(), &layout_info, nullptr, &s_Layouts[index]), "Failed to create bindless descriptor set layout");
		//SEDX_CORE_ASSERT(layoutResult == VK_SUCCESS, "Failed to create bindless descriptor set layout");
		Debugging::SetResourceName(s_Layouts[index], ResourceType::DescriptorSetLayout, cfg.name);

		// set
		VkDescriptorSetVariableDescriptorCountAllocateInfoEXT count_info = {};
		count_info.sType              = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_VARIABLE_DESCRIPTOR_COUNT_ALLOCATE_INFO_EXT;
		count_info.descriptorSetCount = 1;
		count_info.pDescriptorCounts  = &cfg.count;

		VkDescriptorSetAllocateInfo alloc_info = {};
		alloc_info.sType              = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
		alloc_info.descriptorPool     = DescriptorPoolManager::Get().GetPool();
		alloc_info.descriptorSetCount = 1;
		alloc_info.pSetLayouts        = &s_Layouts[index];
		alloc_info.pNext              = &count_info;
		
		SEDX_VK_RESULT_ASSERT(vkAllocateDescriptorSets(device->GetLogicalDevice(), &alloc_info, &s_Sets[index]), "Failed to allocate bindless descriptor set");
		//SEDX_CORE_ASSERT(allocResult == VK_SUCCESS, "Failed to allocate bindless descriptor set");
		Debugging::SetResourceName(s_Sets[index], ResourceType::DescriptorSet, cfg.name);
		++s_Allocated_DescriptorSets;
			
	}
	
	uint32_t BindlessManager::GetBinding(BindlessResource type)
	{
		const ResourceConfig& cfg = CONFIGS[static_cast<uint32_t>(type)];
		return cfg.registerShift + cfg.slot;
	}

	VkDescriptorSetLayout BindlessManager::GetLayoutForType(BindlessResource type)
	{
		return s_Layouts[static_cast<uint32_t>(type)];
	}

	VkDescriptorSet BindlessManager::GetSetForType(BindlessResource type)
	{
		return s_Sets[static_cast<uint32_t>(type)];
	}

	std::unordered_map<uint64_t, DescriptorSet>& BindlessManager::GetDescriptorSets()
	{
		return s_DescriptorSets;
	}

} // namespace SceneryEditorX

// --------------------------------------------------------------
