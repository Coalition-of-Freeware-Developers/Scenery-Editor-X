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
#include "buffer.h"
#include "descriptor.h"
#include "descriptor_layout.h"
#include "descriptor_pool_manager.h"
#include "image_resource.h"
#include "queue_manager.h"
#include "render_context.h"
#include "debug/graphics_debug.h"
#include <algorithm>
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
			case StageType::Vertex:                  return VK_SHADER_STAGE_VERTEX_BIT;
			case StageType::TessellationControl:     return VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT;
			case StageType::TessellationEvaluation:  return VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT;
			case StageType::Geometry:                return VK_SHADER_STAGE_GEOMETRY_BIT;
			case StageType::Fragment:                return VK_SHADER_STAGE_FRAGMENT_BIT;
			case StageType::Compute:                 return VK_SHADER_STAGE_COMPUTE_BIT;
			default:                             return 0;
		}
	}

	/**
	 * @brief Picks the image view a binding refers to: a single mip, a single array layer or the whole resource.
	 * @param img The bound image.
	 * @param binding The binding (mip / layer selection).
	 * @param element Offset from the first selected mip, for descriptor arrays that expose consecutive mips.
	 * @return The view, or VK_NULL_HANDLE if the image has no matching view.
	 */
	static VkImageView SelectImageView(const ImageResource* img, const DescriptorBinding& binding, const bool isStorage, const uint32_t element = 0)
	{
		// storage image views address a single mip, so an "all mips" storage binding resolves to mip 0
		const uint32_t firstMip = binding.GetMip() != ALL_MIPS ? binding.GetMip() : (isStorage ? 0u : ALL_MIPS);
		if (firstMip != ALL_MIPS && img->HasPerMipViews())
		{
			const uint32_t mip = firstMip + element;
			return mip < img->GetMipCount() ? img->GetShaderView_Mip(mip) : VK_NULL_HANDLE;
		}

		if (binding.GetArrayLayer() != ALL_MIPS)
			return img->GetShaderView_Layer(binding.GetArrayLayer());

		return img->GetShaderResourceView();
	}

	/**
	 * @brief Returns the Vulkan handle a binding writes into the set (image view or buffer), or 0 when unbound.
	 */
	static uint64_t ResolveDescriptorHandle(const Descriptor& descriptor, const DescriptorBinding& binding)
	{
		if (!binding.IsBound())
			return 0;

		switch (descriptor.GetType())
		{
			case DescriptorType::Image:
			case DescriptorType::TextureStorage:
				return reinterpret_cast<uint64_t>(SelectImageView(static_cast<const ImageResource*>(binding.GetResource()), binding,
					descriptor.GetType() == DescriptorType::TextureStorage));
			case DescriptorType::ConstantBuffer:
			case DescriptorType::StructuredBuffer:
				return reinterpret_cast<uint64_t>(static_cast<const Buffer*>(binding.GetResource())->Get());
			default:
				return 0;
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
		
		m_LayoutObject = DescriptorLayout::Create(layoutCI, "DescriptorSet-TextureArray");
		SEDX_CORE_ASSERT(m_LayoutObject && m_LayoutObject->IsValid(), "DescriptorSet: failed to create texture-array layout");
		m_Layout = m_LayoutObject->GetLayout();
		
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
		bool hasSlotTypeConflict = false;
		
		for (size_t i = 0; i < count; ++i)
		{
			const uint32_t slot = descriptors[i].GetSlot();
			if (auto it = m_SlotToIndex.find(slot); it == m_SlotToIndex.end())
			{
				const size_t newIndex = m_Descriptors.size();
				m_Descriptors.push_back(descriptors[i]);
				m_Bindings.emplace_back();
				m_SlotToIndex[slot] = newIndex;
			}
			else
			{
				const size_t existingIndex = it->second;
				const DescriptorType existingType = m_Descriptors[existingIndex].GetType();
				const DescriptorType newType = descriptors[i].GetType();

				if (existingType != newType)
				{
					hasSlotTypeConflict = true;
					SEDX_CORE_ERROR_TAG("DescriptorSet", "Descriptor slot/type conflict while creating '{}': slot={} existingType={} incomingType={} (hard fail)",
						m_ObjectName.c_str(),
						slot,
						static_cast<uint32_t>(existingType),
						static_cast<uint32_t>(newType));
					continue;
				}

				m_Descriptors[existingIndex].SetStage(m_Descriptors[existingIndex].GetStage() | descriptors[i].GetStage());
			}
		}
		
		for (const Descriptor& descriptor : m_Descriptors)
		{
		    m_LayoutHash = HashCombine(m_LayoutHash, static_cast<uint64_t>(descriptor.GetSlot()));
		    m_LayoutHash = HashCombine(m_LayoutHash, static_cast<uint64_t>(descriptor.GetStage()));
			m_LayoutHash = HashCombine(m_LayoutHash, static_cast<uint64_t>(descriptor.GetType()));
			m_LayoutHash = HashCombine(m_LayoutHash, static_cast<uint64_t>(descriptor.IsArray() ? descriptor.GetArrayLength() : 1));
		}
	
	    m_Device = RenderContext::Get()->GetDevice();
		if (hasSlotTypeConflict)
			return;
		Create();
	}
	
	// -----------------------------------------------------------------------
	// DescriptorSet — combined DescriptorWithBinding constructor (cache path)
	// -----------------------------------------------------------------------
	
	DescriptorSet::DescriptorSet(const std::vector<DescriptorWithBinding>& combined, const char* name)
	{
		m_ObjectName = name ? name : "";
		m_Descriptors.reserve(combined.size());
		m_Bindings.reserve(combined.size());
		bool hasSlotTypeConflict = false;
		
		for (size_t i = 0; i < combined.size(); ++i)
		{
			const Descriptor& incomingDescriptor = combined[i].GetDescriptor();
			const uint32_t slot = incomingDescriptor.GetSlot();
			if (auto it = m_SlotToIndex.find(slot); it == m_SlotToIndex.end())
			{
				const size_t newIndex = m_Descriptors.size();
				m_Descriptors.push_back(incomingDescriptor);
				m_Bindings.push_back(combined[i].GetBinding());
				m_SlotToIndex[slot] = newIndex;
			}
			else
			{
				const size_t existingIndex = it->second;
				if (m_Descriptors[existingIndex].GetType() != incomingDescriptor.GetType())
				{
					hasSlotTypeConflict = true;
					SEDX_CORE_ERROR_TAG("DescriptorSet", "Descriptor slot/type conflict while creating '{}': slot={} existingType={} incomingType={} (hard fail)",
						m_ObjectName.c_str(),
						slot,
						static_cast<uint32_t>(m_Descriptors[existingIndex].GetType()),
						static_cast<uint32_t>(incomingDescriptor.GetType()));
					continue;
				}

				m_Descriptors[existingIndex].SetStage(m_Descriptors[existingIndex].GetStage() | incomingDescriptor.GetStage());
			}
		}
		
		for (const Descriptor& d : m_Descriptors)
		{
		    m_LayoutHash = HashCombine(m_LayoutHash, static_cast<uint64_t>(d.GetSlot()));
		    m_LayoutHash = HashCombine(m_LayoutHash, static_cast<uint64_t>(d.GetStage()));
			m_LayoutHash = HashCombine(m_LayoutHash, static_cast<uint64_t>(d.GetType()));
			m_LayoutHash = HashCombine(m_LayoutHash, static_cast<uint64_t>(d.IsArray() ? d.GetArrayLength() : 1));
		}

		m_Device = RenderContext::Get()->GetDevice();
		if (hasSlotTypeConflict)
			return;
		Create();
		Write();
	}
	
	// -----------------------------------------------------------------------
	// DescriptorSet — move ctor / move assign
	// -----------------------------------------------------------------------
	
	DescriptorSet::DescriptorSet(DescriptorSet&& other) noexcept
	{
		m_Device        = other.m_Device;
		m_LayoutObject  = std::move(other.m_LayoutObject);
		m_Layout        = other.m_Layout;
		m_Set           = other.m_Set;
		m_Destroyed     = other.m_Destroyed;
		m_Descriptors   = std::move(other.m_Descriptors);
		m_Bindings      = std::move(other.m_Bindings);
		m_WrittenHandles = std::move(other.m_WrittenHandles);
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
		m_LayoutObject  = std::move(other.m_LayoutObject);
		m_Layout        = other.m_Layout;
		m_Set           = other.m_Set;
		m_Destroyed     = other.m_Destroyed;
		m_Descriptors   = std::move(other.m_Descriptors);
		m_Bindings      = std::move(other.m_Bindings);
		m_WrittenHandles = std::move(other.m_WrittenHandles);
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
		uint32_t actualSlot = slot;
		DescriptorBinding* binding = FindBinding(actualSlot);
		if (!binding)
		{
			actualSlot = slot + SHADER_REGISTER_SHIFT_B; // legacy fallback
			binding = FindBinding(actualSlot);
		}

		if (binding)
		{
		    const Descriptor& descriptor = m_Descriptors[m_SlotToIndex[actualSlot]];
		
			binding->SetResource(static_cast<void*>(constantBuffer));
			binding->SetRange(static_cast<int64_t>(constantBuffer->GetStride()));
			binding->SetDynamicOffset(constantBuffer->GetOffset());

			// Reflection does not report struct sizes yet (0), so only compare when one is known.
			SEDX_CORE_ASSERT(descriptor.GetStructSize() == 0 || constantBuffer->GetStrideUnaligned() == descriptor.GetStructSize(), "Size mismatch between CPU and GPU side constant buffer");
			SEDX_CORE_ASSERT(binding->GetDynamicOffset() % binding->GetRange() == 0, "Incorrect dynamic offset");
			
			m_Dirty.SetDirty();
		}
	}
	
	void DescriptorSet::SetBuffer(uint32_t slot, Buffer* buffer)
	{
		auto matches_structured_buffer = [&](const uint32_t candidateSlot) -> bool
		{
			auto slotIt = m_SlotToIndex.find(candidateSlot);
			if (slotIt == m_SlotToIndex.end())
				return false;

			const size_t descriptorIndex = slotIt->second;
			return descriptorIndex < m_Descriptors.size() && m_Descriptors[descriptorIndex].GetType() == DescriptorType::StructuredBuffer;
		};

		uint32_t actualSlot = slot;
		if (!matches_structured_buffer(actualSlot))
		{
			actualSlot = slot + SHADER_REGISTER_SHIFT_U;
			if (!matches_structured_buffer(actualSlot))
				return;
		}

		DescriptorBinding* binding = FindBinding(actualSlot);
		if (!binding)
			return;

		binding->SetResource(static_cast<void*>(buffer));
		binding->SetRange(static_cast<int64_t>(buffer->GetObjectSize()));
		binding->SetDynamicOffset(buffer->GetOffset());
		m_Dirty.SetDirty();
	}
	
	void DescriptorSet::SetTexture(uint32_t slot, ImageResource* img, uint32_t mipIndex, uint32_t mipRange, uint32_t layer)
	{
		// Resolve the binding against the image's capabilities: a UAV-capable image prefers a storage slot,
		// but can also be bound to a sampled slot. The layout follows the descriptor type that matched.
		bool isStorage = img->IsUnorderedAccessView();
		{
			const auto slotIt = m_SlotToIndex.find(slot);
			if (slotIt != m_SlotToIndex.end() && slotIt->second < m_Descriptors.size())
			{
				const DescriptorType type = m_Descriptors[slotIt->second].GetType();
				if (type == DescriptorType::Image)
					isStorage = false;
				else if (type == DescriptorType::TextureStorage)
					isStorage = true;
			}
		}
		Layout::ImageLayout layout = isStorage ? Layout::ImageLayout::General : Layout::ImageLayout::ShaderRead;

		auto matches_texture_type = [&](const uint32_t candidateSlot) -> bool
		{
			auto slotIt = m_SlotToIndex.find(candidateSlot);
			if (slotIt == m_SlotToIndex.end())
				return false;

			const size_t descriptorIndex = slotIt->second;
			if (descriptorIndex >= m_Descriptors.size())
				return false;

			const DescriptorType type = m_Descriptors[descriptorIndex].GetType();
			return isStorage ? (type == DescriptorType::TextureStorage) : (type == DescriptorType::Image);
		};

		const uint32_t shift = isStorage ? SHADER_REGISTER_SHIFT_U : SHADER_REGISTER_SHIFT_T;
		uint32_t actualSlot = slot;
		if (!matches_texture_type(actualSlot))
		{
			actualSlot = slot + shift;
			if (!matches_texture_type(actualSlot))
				return;
		}

		DescriptorBinding* binding = FindBinding(actualSlot);
		if (!binding)
			return;

		binding->SetResource(img);
		binding->SetLayout(layout);
		binding->SetMip(mipIndex);
		binding->SetMipRange(mipRange);
		binding->SetArrayLayer(layer);
		m_Dirty.SetDirty();

	}
	
	void DescriptorSet::GetDynamicOffsets(std::array<uint32_t, 10>* offsets, uint32_t* count)
	{
		*count = 0;
		for (size_t i = 0; i < m_Descriptors.size(); ++i)
		{
			const Descriptor& descriptor = m_Descriptors[i];
			if (descriptor.GetType() == DescriptorType::ConstantBuffer)
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
			// Once the shared pool is destroyed its sets are already freed; only return the set while it lives.
			if (m_Set != VK_NULL_HANDLE && m_Device.IsValid() && DescriptorPoolManager::IsInitialized())
				DescriptorPoolManager::Get().Free(m_Set);
			m_Set = VK_NULL_HANDLE;
			
			// The layout is owned by m_LayoutObject; releasing it queues the VkDescriptorSetLayout for deletion exactly once.
			m_LayoutObject.Reset();
			m_Layout = VK_NULL_HANDLE;
		
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

		// The deletion queue reports the raw Vulkan handle (image view / buffer) that is being destroyed.
		const uint64_t handle = reinterpret_cast<uint64_t>(resource);
		return std::ranges::find(m_WrittenHandles, handle) != m_WrittenHandles.end();
	}

	uint64_t DescriptorSet::ComputeBindingHash() const
	{
		uint64_t hash = m_LayoutHash;
		for (size_t i = 0; i < m_Bindings.size(); ++i)
		{
			hash = HashCombine(hash, m_Bindings[i].GetHash());

			// Include the Vulkan handle: a resource recreated at the same address (e.g. render targets on resize)
			// must not reuse a cached set that still points at the old, pending-deletion views.
			hash = HashCombine(hash, ResolveDescriptorHandle(m_Descriptors[i], m_Bindings[i]));
		}
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
			
			m_LayoutObject = DescriptorLayout::Create(createInfo, m_ObjectName.c_str());
			SEDX_CORE_ASSERT(m_LayoutObject && m_LayoutObject->IsValid(), "DescriptorSet: failed to create empty layout");
			m_Layout = m_LayoutObject->GetLayout();
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
			if (desc.GetStage() & ShaderTypeToMask(StageType::Geometry))               stageFlags |= VK_SHADER_STAGE_GEOMETRY_BIT;
			if (desc.GetStage() & ShaderTypeToMask(StageType::Fragment))               stageFlags |= VK_SHADER_STAGE_FRAGMENT_BIT;
			if (desc.GetStage() & ShaderTypeToMask(StageType::Compute))                stageFlags |= VK_SHADER_STAGE_COMPUTE_BIT;
			SEDX_CORE_ASSERT(stageFlags != 0, "DescriptorSet::Create: descriptor slot {} has zero stage flags", desc.GetSlot());
			
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
		
		m_LayoutObject = DescriptorLayout::Create(createInfo, m_ObjectName.c_str());
		SEDX_CORE_ASSERT(m_LayoutObject && m_LayoutObject->IsValid(), "DescriptorSet::Create: failed to create VkDescriptorSetLayout");
		m_Layout = m_LayoutObject->GetLayout();
		
		// Allocate a VkDescriptorSet from the shared pool immediately
		m_Set = DescriptorPoolManager::Get().Allocate(m_Layout);
		SEDX_CORE_ASSERT(m_Set != VK_NULL_HANDLE, "DescriptorSet::Create: failed to allocate VkDescriptorSet");
	}

	// -----------------------------------------------------------------------
	// DescriptorSet::Write — resource writes for a cached binding set
	// -----------------------------------------------------------------------

	void DescriptorSet::Write()
	{
		m_WrittenHandles.clear();
		if (m_Set == VK_NULL_HANDLE || !m_Device.IsValid())
			return;

		// Reserve up front: the writes keep pointers into these arrays.
		size_t imageInfoCount  = 0;
		size_t bufferInfoCount = 0;
		for (const Descriptor& descriptor : m_Descriptors)
		{
			const DescriptorType type = descriptor.GetType();
			if (type == DescriptorType::Image || type == DescriptorType::TextureStorage)
				imageInfoCount += descriptor.IsArray() ? std::max(1u, descriptor.GetArrayLength()) : 1u;
			else if (type == DescriptorType::ConstantBuffer || type == DescriptorType::StructuredBuffer)
				++bufferInfoCount;
		}

		std::vector<VkDescriptorImageInfo> imageInfos;
		std::vector<VkDescriptorBufferInfo> bufferInfos;
		std::vector<VkWriteDescriptorSet> writes;
		imageInfos.reserve(imageInfoCount);
		bufferInfos.reserve(bufferInfoCount);
		writes.reserve(m_Descriptors.size());

		for (size_t i = 0; i < m_Descriptors.size(); ++i)
		{
			const Descriptor& descriptor = m_Descriptors[i];
			const DescriptorBinding& binding = m_Bindings[i];

			// Bindless arrays live in their own sets.
			if (descriptor.IsArray() && descriptor.GetArrayLength() == MAX_ARRAY_SIZE)
				continue;

			const DescriptorType type = descriptor.GetType();

			VkWriteDescriptorSet write{};
			write.sType           = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
			write.dstSet          = m_Set;
			write.dstBinding      = descriptor.GetSlot();
			write.dstArrayElement = 0;
			write.descriptorType  = ToVkDescriptorType(type);

			// A slot the pass left unbound gets null descriptors (nullDescriptor: reads return 0, writes are dropped).
			// Unwritten, it keeps what the recycled pool memory held, often the descriptor of a destroyed render target:
			// a shader reading it faults the GPU (VK_ERROR_DEVICE_LOST, and the reset takes every app on it down).
			// Dynamic uniform buffers need a real buffer for their offsets; the per-frame ones are always bound.
			if (!binding.IsBound())
			{
				if (type == DescriptorType::Image || type == DescriptorType::TextureStorage)
				{
					const uint32_t elementCount = descriptor.IsArray() ? std::max(1u, descriptor.GetArrayLength()) : 1u;
					const size_t first = imageInfos.size();
					for (uint32_t element = 0; element < elementCount; ++element)
						imageInfos.push_back(VkDescriptorImageInfo{ .sampler = VK_NULL_HANDLE, .imageView = VK_NULL_HANDLE, .imageLayout = VK_IMAGE_LAYOUT_UNDEFINED });
					write.descriptorCount = elementCount;
					write.pImageInfo      = imageInfos.data() + first;
				}
				else if (type == DescriptorType::StructuredBuffer)
				{
					bufferInfos.push_back(VkDescriptorBufferInfo{ .buffer = VK_NULL_HANDLE, .offset = 0, .range = VK_WHOLE_SIZE });
					write.descriptorCount = 1;
					write.pBufferInfo     = &bufferInfos.back();
				}
				else
				{
					continue;
				}

				writes.push_back(write);
				continue;
			}

			switch (type)
			{
				case DescriptorType::Image:
				case DescriptorType::TextureStorage:
				{
					const auto* img = static_cast<const ImageResource*>(binding.GetResource());
					const bool isStorage = type == DescriptorType::TextureStorage;
					const VkImageLayout layout = isStorage ? VK_IMAGE_LAYOUT_GENERAL : VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

					// A descriptor array bound to a single image exposes consecutive mips (e.g. FidelityFX SPD).
					const uint32_t elementCount = (descriptor.IsArray() && binding.GetMip() != ALL_MIPS)
						? std::min(std::max(1u, descriptor.GetArrayLength()), std::max(1u, binding.GetMipRange())) : 1u;

					const size_t first = imageInfos.size();
					for (uint32_t element = 0; element < elementCount; ++element)
					{
						const VkImageView view = SelectImageView(img, binding, isStorage, element);
						if (view == VK_NULL_HANDLE)
							break;

						imageInfos.push_back(VkDescriptorImageInfo{ .sampler = VK_NULL_HANDLE, .imageView = view, .imageLayout = layout });
						m_WrittenHandles.push_back(reinterpret_cast<uint64_t>(view));
					}

					const uint32_t written = static_cast<uint32_t>(imageInfos.size() - first);
					if (written == 0)
						continue;

					write.descriptorCount = written;
					write.pImageInfo      = imageInfos.data() + first;
					break;
				}

				case DescriptorType::ConstantBuffer:
				case DescriptorType::StructuredBuffer:
				{
					const auto* buffer = static_cast<const Buffer*>(binding.GetResource());
					if (buffer->Get() == VK_NULL_HANDLE)
						continue;

					// A dynamic uniform buffer exposes one element (the dynamic offset picks it); storage buffers expose all of it.
					VkDescriptorBufferInfo info{};
					info.buffer = buffer->Get();
					info.offset = 0;
					info.range  = type == DescriptorType::ConstantBuffer ? static_cast<VkDeviceSize>(binding.GetRange()) : VK_WHOLE_SIZE;
					bufferInfos.push_back(info);
					m_WrittenHandles.push_back(reinterpret_cast<uint64_t>(buffer->Get()));

					write.descriptorCount = 1;
					write.pBufferInfo     = &bufferInfos.back();
					break;
				}

				default:
					continue;
			}

			writes.push_back(write);
		}

		if (!writes.empty())
			vkUpdateDescriptorSets(m_Device->GetLogicalDevice(), static_cast<uint32_t>(writes.size()), writes.data(), 0, nullptr);
	}
	
} // namespace SceneryEditorX

// -------------------------------------------------------
