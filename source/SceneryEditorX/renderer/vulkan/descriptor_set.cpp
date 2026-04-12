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
 * descriptor_set.cpp
 * -------------------------------------------------------
 * Created: 09/02/2026
 * -------------------------------------------------------
 */
#include "descriptor_set.h"
#include "bindless_manager.h"
#include "descriptor.h"
#include "descriptor_layout.h"
#include "descriptor_pool_manager.h"
#include "image_resource.h"
#include "queue_manager.h"
#include "render_context.h"
#include "debug/graphics_debug.h"
#include <unordered_set>
#include <volk/volk.h>

// -------------------------------------------------------

namespace SceneryEditorX
{
	
	/**
	 * @brief Combine two 64-bit hashes into one.
	 * @param a The first hash value.
	 * @param b The second hash value.
	 * @return The combined hash value.
	 */
	static uint64_t HashCombine(const uint64_t a, const uint64_t b)
	{
	    return a * 31 + b;
	}
	
	/**
	 * @brief Convert a shader stage to a corresponding bitmask.
	 * @param type The shader stage.
	 * @return The bitmask representing the shader stage.
	 */
	static uint32_t ShaderTypeToMask(StageType type)
	{
		switch (type)
		{
			case StageType::Vertex:                  return BIT(0);
			case StageType::TessellationControl:     return BIT(1);
			case StageType::TessellationEvaluation:  return BIT(2);
			case StageType::Fragment:                return BIT(3);
			case StageType::Compute:                 return BIT(4);
			default:                             return 0;
		}
	}
	
	// -----------------------------------------------------------------------
	// DescriptorSet — texture array constructor
	// -----------------------------------------------------------------------
	
	DescriptorSet::DescriptorSet(const std::vector<VkDescriptorImageInfo>& textureDescriptors)
	{
		m_Device = RenderContext::Get()->GetDevice();
		
		const uint32_t count = static_cast<uint32_t>(textureDescriptors.size());
		
		// Build a simple combined-image-sampler layout for `count` descriptors
		VkDescriptorSetLayoutBinding binding{};
		binding.binding            = 0;
		binding.descriptorType     = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		binding.descriptorCount    = count;
		binding.stageFlags         = VK_SHADER_STAGE_FRAGMENT_BIT;
		binding.pImmutableSamplers = nullptr;
		
		VkDescriptorBindingFlags bindingFlag =
		VK_DESCRIPTOR_BINDING_PARTIALLY_BOUND_BIT |
		VK_DESCRIPTOR_BINDING_VARIABLE_DESCRIPTOR_COUNT_BIT;
		
		VkDescriptorSetLayoutBindingFlagsCreateInfoEXT flagsInfo{};
		flagsInfo.sType         = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_BINDING_FLAGS_CREATE_INFO_EXT;
		flagsInfo.bindingCount  = 1;
		flagsInfo.pBindingFlags = &bindingFlag;
		
		VkDescriptorSetLayoutCreateInfo layoutCI{};
		layoutCI.sType        = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
		layoutCI.pNext        = &flagsInfo;
		layoutCI.bindingCount = 1;
		layoutCI.pBindings    = &binding;
		
		Ref<DescriptorLayout> descLayout = DescriptorLayout::Create(layoutCI, "DescriptorSet-TextureArray");
		SEDX_CORE_ASSERT(descLayout && descLayout->IsValid(), "DescriptorSet: failed to create texture-array layout");
		m_Layout = descLayout->GetLayout();
		
		// Allocate from the shared pool with variable count
		m_Set = DescriptorPoolManager::Get().Allocate(m_Layout, count);
		SEDX_CORE_ASSERT(m_Set != VK_NULL_HANDLE, "DescriptorSet: failed to allocate descriptor set from shared pool");
		
		// Write image descriptors
		VkWriteDescriptorSet write{};
		write.sType           = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		write.dstSet          = m_Set;
		write.dstBinding      = 0;
		write.descriptorCount = count;
		write.descriptorType  = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		write.pImageInfo      = textureDescriptors.data();
		vkUpdateDescriptorSets(m_Device->GetLogicalDevice(), 1, &write, 0, nullptr);
	}
	
	// -----------------------------------------------------------------------
	// DescriptorSet — spec array constructor
	// -----------------------------------------------------------------------
	
	DescriptorSet::DescriptorSet(const Descriptor* descriptors, size_t count, const char* name)
	{
		m_ObjectName = name ? name : "";
		m_Descriptors.reserve(count);
		m_Bindings.reserve(count);
		
		for (size_t i = 0; i < count; ++i)
		{
			m_Descriptors.push_back(descriptors[i]);
			m_Bindings.emplace_back();
			m_SlotToIndex[descriptors[i].GetSlot()] = i;
		}
		
		for (const Descriptor& descriptor : m_Descriptors)
		{
		    m_LayoutHash = HashCombine(m_LayoutHash, static_cast<uint64_t>(descriptor.GetSlot()));
		    m_LayoutHash = HashCombine(m_LayoutHash, static_cast<uint64_t>(descriptor.GetStage()));
		}
	
	    m_Device = RenderContext::Get()->GetDevice();
	}
	
	// -----------------------------------------------------------------------
	// DescriptorSet — combined DescriptorWithBinding constructor (cache path)
	// -----------------------------------------------------------------------
	
	DescriptorSet::DescriptorSet(const std::vector<DescriptorWithBinding>& combined, const char* name)
	{
		m_ObjectName = name ? name : "";
		m_Descriptors.reserve(combined.size());
		m_Bindings.reserve(combined.size());
		
		for (size_t i = 0; i < combined.size(); ++i)
		{
			m_Descriptors.push_back(combined[i].GetDescriptor());
			m_Bindings.push_back(combined[i].GetBinding());
			m_SlotToIndex[combined[i].GetDescriptor().GetSlot()] = i;
		}
		
		for (const Descriptor& d : m_Descriptors)
		{
		    m_LayoutHash = HashCombine(m_LayoutHash, static_cast<uint64_t>(d.GetSlot()));
		    m_LayoutHash = HashCombine(m_LayoutHash, static_cast<uint64_t>(d.GetStage()));
		}
		
		m_Device = RenderContext::Get()->GetDevice();
		Create();
	}
	
	// -----------------------------------------------------------------------
	// DescriptorSet — move ctor / move assign
	// -----------------------------------------------------------------------
	
	DescriptorSet::DescriptorSet(DescriptorSet&& other) noexcept
	{
		m_Device        = other.m_Device;
		m_Layout        = other.m_Layout;
		m_Set           = other.m_Set;
		m_Destroyed     = other.m_Destroyed;
		m_Descriptors   = std::move(other.m_Descriptors);
		m_Bindings      = std::move(other.m_Bindings);
		m_SlotToIndex   = std::move(other.m_SlotToIndex);
		m_LayoutHash    = other.m_LayoutHash;
		m_BindingHash   = other.m_BindingHash;
		m_Dirty         = other.m_Dirty;
		m_ObjectName    = std::move(other.m_ObjectName);
		
		other.m_Device     = nullptr;
		other.m_Layout     = VK_NULL_HANDLE;
		other.m_Set        = VK_NULL_HANDLE;
		other.m_Destroyed  = true;
	}
	
	DescriptorSet& DescriptorSet::operator=(DescriptorSet&& other) noexcept
	{
	    if (this == &other)
			return *this;
	
	    Destroy();
	
		m_Device        = other.m_Device;
		m_Layout        = other.m_Layout;
		m_Set           = other.m_Set;
		m_Destroyed     = other.m_Destroyed;
		m_Descriptors   = std::move(other.m_Descriptors);
		m_Bindings      = std::move(other.m_Bindings);
		m_SlotToIndex   = std::move(other.m_SlotToIndex);
		m_LayoutHash    = other.m_LayoutHash;
		m_BindingHash   = other.m_BindingHash;
		m_Dirty         = other.m_Dirty;
		m_ObjectName    = std::move(other.m_ObjectName);
	
		other.m_Device     = nullptr;
		other.m_Layout     = VK_NULL_HANDLE;
		other.m_Set        = VK_NULL_HANDLE;
		other.m_Destroyed  = true;
	
	    return *this;
	}
	
	// -----------------------------------------------------------------------
	// DescriptorSet — binding API
	// -----------------------------------------------------------------------
	
	void DescriptorSet::SetConstantBuffer(uint32_t slot, Buffer* constantBuffer)
	{
		uint32_t actualSlot = slot + SHADER_REGISTER_SHIFT_B;
		if (DescriptorBinding* binding = FindBinding(actualSlot))
		{
		    const Descriptor& descriptor = m_Descriptors[m_SlotToIndex[actualSlot]];
		
			binding->SetResource(static_cast<void*>(constantBuffer));
			binding->SetRange(static_cast<int64_t>(constantBuffer->GetStride()));
			binding->SetDynamicOffset(constantBuffer->GetOffset());
			
			SEDX_CORE_ASSERT(constantBuffer->GetStrideUnaligned() == descriptor.GetStructSize(), "Size mismatch between CPU and GPU side constant buffer");
			SEDX_CORE_ASSERT(binding->GetDynamicOffset() % binding->GetRange() == 0, "Incorrect dynamic offset");
			
			m_Dirty.SetDirty();
		}
	}
	
	void DescriptorSet::SetBuffer(uint32_t slot, Buffer* buffer)
	{
		uint32_t actualSlot = slot + SHADER_REGISTER_SHIFT_U;
		if (DescriptorBinding* binding = FindBinding(actualSlot))
		{
			binding->SetResource(static_cast<void*>(buffer));
			binding->SetRange(static_cast<int64_t>(buffer->GetObjectSize()));
			binding->SetDynamicOffset(buffer->GetOffset());
			m_Dirty.SetDirty();
		}
	}
	
	void DescriptorSet::SetTexture(uint32_t slot, ImageResource* img, uint32_t mipIndex, uint32_t mipRange, uint32_t layer)
	{
	    bool isStorage = img->IsUnorderedAccessView();
	    Layout::ImageLayout layout = isStorage ? Layout::ImageLayout::General : Layout::ImageLayout::ShaderRead;
	
	    SEDX_CORE_ASSERT(layout == Layout::ImageLayout::General || layout == Layout::ImageLayout::ShaderRead);
	
		uint32_t shift      = isStorage ? SHADER_REGISTER_SHIFT_U : SHADER_REGISTER_SHIFT_T;
		uint32_t actualSlot = slot + shift;
	
		if (DescriptorBinding* binding = FindBinding(actualSlot))
		{
			binding->SetResource(img);
			binding->SetLayout(layout);
			binding->SetMip(mipIndex);
			binding->SetMipRange(mipRange);
			binding->SetArrayLayer(layer);
			m_Dirty.SetDirty();
		}

	}
	
	void DescriptorSet::GetDynamicOffsets(std::array<uint32_t, 10>* offsets, uint32_t* count)
	{
		*count = 0;
		for (size_t i = 0; i < m_Descriptors.size(); ++i)
		{
			const Descriptor& descriptor = m_Descriptors[i];
			if (descriptor.GetType() == DescriptorType::StructuredBuffer || descriptor.GetType() == DescriptorType::ConstantBuffer)
			{
			    (*offsets)[(*count)++] = m_Bindings[i].GetDynamicOffset();
			}
		}
	}
	
	DescriptorBinding* DescriptorSet::FindBinding(uint32_t slot)
	{
		if (auto it = m_SlotToIndex.find(slot); it != m_SlotToIndex.end())
		{
		    return &m_Bindings[it->second];
		}

		return nullptr;
	}
	
	void DescriptorSet::ClearBindings()
	{
	    for (DescriptorBinding& binding : m_Bindings)
	    {
	        binding.Reset();
	    }

	    m_Dirty.SetDirty();
	}
	
	// -----------------------------------------------------------------------
	// DescriptorSet — cache-based VkDescriptorSet retrieval
	// -----------------------------------------------------------------------
	
	void* DescriptorSet::GetOrCreateDescriptorSet()
	{
		if (m_Dirty.IsDirty())
		{
			m_BindingHash = ComputeBindingHash();
			m_Dirty.Check();
		}
		
		std::unordered_map<uint64_t, DescriptorSet>& descriptorSets = BindlessManager::GetDescriptorSets();
		auto it = descriptorSets.find(m_BindingHash);
		
		if (it == descriptorSets.end())
		{
			std::vector<DescriptorWithBinding> combined;
			combined.reserve(m_Descriptors.size());
			
			for (size_t i = 0; i < m_Descriptors.size(); ++i)
			{
			    combined.emplace_back(m_Descriptors[i], m_Bindings[i]);
			}
			
			descriptorSets[m_BindingHash] = DescriptorSet(combined, m_ObjectName.c_str());
			it = descriptorSets.find(m_BindingHash);
		}
	
	    return it->second.GetResource();
	}
	
	// -----------------------------------------------------------------------
	// DescriptorSet — lifetime
	// -----------------------------------------------------------------------
	
	DescriptorSet::~DescriptorSet()
	{
		if (!m_Destroyed)
			Destroy();
	}
	
	void DescriptorSet::Destroy()
	{
		if (!m_Destroyed)
		{
			if (m_Set != VK_NULL_HANDLE && m_Device.IsValid())
			{
				DescriptorPoolManager::Get().Free(m_Set);
				m_Set = VK_NULL_HANDLE;
			}
			
			if (m_Layout != VK_NULL_HANDLE)
			{
				QueueManager::AddDeletionQueue(ResourceType::DescriptorSetLayout, m_Layout);
				m_Layout = VK_NULL_HANDLE;
			}
		
		    m_Destroyed = true;
		}
	}
	
	// -----------------------------------------------------------------------
	// DescriptorSet — queries
	// -----------------------------------------------------------------------
	
	bool DescriptorSet::IsReferringToResource(void* resource) const
	{
		for (const auto& binding : m_Bindings)
		{
			if (binding.GetResource() == resource)
			return true;
		}

		return false;
	}
	
	uint64_t DescriptorSet::ComputeBindingHash() const
	{
		uint64_t hash = m_LayoutHash;
		for (const DescriptorBinding& binding : m_Bindings)
		hash = HashCombine(hash, binding.GetHash());
		return hash;
	}
	
	// -----------------------------------------------------------------------
	// DescriptorSet — dynamic binding API
	// -----------------------------------------------------------------------
	
	void DescriptorSet::SetDynamicDescriptor(const PipelineState& pso, VkCommandBuffer cmdBuffer, VkPipelineLayout pipelineLayout, DescriptorSet* descriptorSet)
	{
		SEDX_CORE_ASSERT(descriptorSet != nullptr, "SetDynamicDescriptor: descriptorSet must not be null");
		
		void* rawSet = descriptorSet->GetOrCreateDescriptorSet();
		
		std::array<uint32_t, 10> dynamicOffsets;
		uint32_t dynamicOffsetCount = 0;
		descriptorSet->GetDynamicOffsets(&dynamicOffsets, &dynamicOffsetCount);
		
		const VkPipelineBindPoint bindPoint = pso.IsGraphics() ? VK_PIPELINE_BIND_POINT_GRAPHICS : VK_PIPELINE_BIND_POINT_COMPUTE;
		
		const VkDescriptorSet vkSet = static_cast<VkDescriptorSet>(rawSet);
		vkCmdBindDescriptorSets(cmdBuffer, bindPoint, pipelineLayout, 0, 1,&vkSet,
			dynamicOffsetCount, dynamicOffsets.data());
	}
	
	// -----------------------------------------------------------------------
	// DescriptorSet — bindless binding
	// -----------------------------------------------------------------------
	
	void DescriptorSet::SetBindless(const PipelineState pso, void* resource, void* pipeline_layout)
	{
		std::array<void*, static_cast<size_t>(BindlessResource::MaxEnum)> resources;
		for (size_t i = 0; i < resources.size(); ++i)
		{
		    resources[i] = BindlessManager::GetSetForType(static_cast<BindlessResource>(i));
		}
		
		const VkPipelineBindPoint bindPoint = pso.IsGraphics() ? VK_PIPELINE_BIND_POINT_GRAPHICS : VK_PIPELINE_BIND_POINT_COMPUTE;
		
		vkCmdBindDescriptorSets(static_cast<VkCommandBuffer>(resource), bindPoint, static_cast<VkPipelineLayout>(pipeline_layout),
		1, static_cast<uint32_t>(resources.size()), reinterpret_cast<VkDescriptorSet*>(resources.data()),
		0, nullptr);
	}
	
	// -----------------------------------------------------------------------
	// DescriptorSet::Create — layout-only (VkDescriptorSet allocated lazily)
	// -----------------------------------------------------------------------
	
	void DescriptorSet::Create()
	{
		SEDX_CORE_ASSERT(m_Layout == VK_NULL_HANDLE, "DescriptorSet::Create called with existing layout");
		
		// Filter out push constants and bindless arrays
		std::vector<const Descriptor*> filtered;
		filtered.reserve(m_Descriptors.size());
		for (const Descriptor& desc : m_Descriptors)
		{
		    if (desc.GetType() == DescriptorType::PushConstantBuffer)
		        continue;

			if (desc.IsArray() && desc.GetArrayLength() == MAX_ARRAY_SIZE)
			    continue;

		    filtered.push_back(&desc);
		}
		
		if (filtered.empty())
		{
			VkDescriptorSetLayoutCreateInfo createInfo{};
			createInfo.sType        = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
			createInfo.bindingCount = 0;
			createInfo.pBindings    = nullptr;
			
			Ref<DescriptorLayout> emptyLayout = DescriptorLayout::Create(createInfo, m_ObjectName.c_str());
			SEDX_CORE_ASSERT(emptyLayout && emptyLayout->IsValid(), "DescriptorSet: failed to create empty layout");
			m_Layout = emptyLayout->GetLayout();
			return;
		}
		
		// Validate unique bindings
		{
			std::unordered_set<uint32_t> uniqueBindings;
			for (const Descriptor* desc : filtered)
			{
				bool inserted = uniqueBindings.insert(desc->GetSlot()).second;
				SEDX_CORE_ASSERT(inserted, "Duplicate binding slot detected");
			}
		}
		
		constexpr uint32_t maxBindings = 255;
		std::array<VkDescriptorSetLayoutBinding, maxBindings> layoutBindings{};
		std::array<VkDescriptorBindingFlags, maxBindings>     bindingFlags{};
		
		for (size_t i = 0; i < filtered.size(); ++i)
		{
			const Descriptor& desc = *filtered[i];
			
			VkShaderStageFlags stageFlags = 0;
			if (desc.GetStage() & ShaderTypeToMask(StageType::Vertex))                 stageFlags |= VK_SHADER_STAGE_VERTEX_BIT;
			if (desc.GetStage() & ShaderTypeToMask(StageType::TessellationControl))    stageFlags |= VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT;
			if (desc.GetStage() & ShaderTypeToMask(StageType::TessellationEvaluation)) stageFlags |= VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT;
			if (desc.GetStage() & ShaderTypeToMask(StageType::Fragment))               stageFlags |= VK_SHADER_STAGE_FRAGMENT_BIT;
			if (desc.GetStage() & ShaderTypeToMask(StageType::Compute))                stageFlags |= VK_SHADER_STAGE_COMPUTE_BIT;
			
			auto& binding              = layoutBindings[i];
			binding.binding            = desc.GetSlot();
			binding.descriptorType     = ToVkDescriptorType(desc.GetType());
			binding.descriptorCount    = desc.IsArray() ? desc.GetArrayLength() : 1;
			binding.stageFlags         = stageFlags;
			binding.pImmutableSamplers = nullptr;
			
			bindingFlags[i] = VK_DESCRIPTOR_BINDING_PARTIALLY_BOUND_BIT;
		}
		
		VkDescriptorSetLayoutBindingFlagsCreateInfoEXT flagsInfo{};
		flagsInfo.sType         = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_BINDING_FLAGS_CREATE_INFO_EXT;
		flagsInfo.bindingCount  = static_cast<uint32_t>(filtered.size());
		flagsInfo.pBindingFlags = bindingFlags.data();
		
		VkDescriptorSetLayoutCreateInfo createInfo{};
		createInfo.sType        = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
		createInfo.pNext        = &flagsInfo;
		createInfo.bindingCount = static_cast<uint32_t>(filtered.size());
		createInfo.pBindings    = layoutBindings.data();
		
		Ref<DescriptorLayout> descLayout = DescriptorLayout::Create(createInfo, m_ObjectName.c_str());
		SEDX_CORE_ASSERT(descLayout && descLayout->IsValid(), "DescriptorSet::Create: failed to create VkDescriptorSetLayout");
		m_Layout = descLayout->GetLayout();
		
		// Allocate a VkDescriptorSet from the shared pool immediately
		m_Set = DescriptorPoolManager::Get().Allocate(m_Layout);
		SEDX_CORE_ASSERT(m_Set != VK_NULL_HANDLE, "DescriptorSet::Create: failed to allocate VkDescriptorSet");
	}
	
} // namespace SceneryEditorX

// -------------------------------------------------------
