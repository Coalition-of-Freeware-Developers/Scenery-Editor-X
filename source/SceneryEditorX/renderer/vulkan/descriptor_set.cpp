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

#include "bindless_manager.h"
#include "descriptor.h"
#include "render_context.h"
#include "image_resource.h"
#include "debug/graphics_debug.h"

#include <volk/volk.h>

// -------------------------------------------------------

namespace SceneryEditorX
{
	static uint64_t HashCombine(const uint64_t a, const uint64_t b)
	{
		return a * 31 + b;
	}

	static uint32_t ShaderTypeToMask(Stage type)
	{
		switch (type)
		{
			case Stage::Vertex:					return BIT(0);
			case Stage::TessellationControl:    return BIT(1);
			case Stage::TessellationEvaluation: return BIT(2);
			case Stage::Fragment:				return BIT(3);
			case Stage::Compute:				return BIT(4);
			default:							return 0;
		}
	}

	bool Descriptor::Init(uint32_t bindingCount, uint32_t descriptorCount)
	{
		m_Device = RenderContext::Get()->GetDevice();

		// Create layout
		VkDescriptorSetLayoutBinding binding;
		binding.binding = 0;
		binding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		binding.descriptorCount = bindingCount;
		binding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
		binding.pImmutableSamplers = nullptr;
	
		VkDescriptorSetLayoutCreateInfo layoutCI{};
		layoutCI.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
		layoutCI.bindingCount = 1;
		layoutCI.pBindings = &binding;
	
		SEDX_VK_RESULT_ASSERT(vkCreateDescriptorSetLayout(m_Device->GetLogicalDevice(), &layoutCI, nullptr, &m_Layout), "Attempt to create a DescriptorSetLayout failed");
	
		// Create pool
		VkDescriptorPoolSize poolSize;
		poolSize.type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		poolSize.descriptorCount = descriptorCount;
	
		VkDescriptorPoolCreateInfo poolCI{};
		poolCI.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
		poolCI.maxSets = 1;
		poolCI.poolSizeCount = 1;
		poolCI.pPoolSizes = &poolSize;
	
		VkResult r = vkCreateDescriptorPool(m_Device->GetLogicalDevice(), &poolCI, nullptr, &m_Pool);
		if (r != VK_SUCCESS) 
		{
			SEDX_CORE_ERROR_TAG("DescriptorSet", "Attempt to create a DescriptorPool failed: {}", r);
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
		SEDX_VK_RESULT_ASSERT(vkAllocateDescriptorSets(m_Device->GetLogicalDevice(), &allocInfo, &descriptorSet), "Attempt to allocate descriptor sets failed");
	
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

		const uint32_t count = static_cast<uint32_t>(textureDescriptors.size());
		DescriptorSpec spec{};
		spec.name        = "DescriptorSet";
		spec.type        = DescriptorType::Image;
		spec.layout      = Layout::ImageLayout::MaxEnum;
		spec.slot        = 0;
		spec.stage       = static_cast<uint32_t>(VK_SHADER_STAGE_FRAGMENT_BIT);
		spec.structSize  = 0;
		spec.asArray     = true;
		spec.arrayLength = count;
		Descriptor helper(spec);
		m_Layout = helper.CreateLayout(count);
		m_DescriptorPool = helper.CreatePool(count);
		SEDX_CORE_ASSERT(m_DescriptorPool != VK_NULL_HANDLE, "DescriptorSet: failed to create descriptor pool");

		m_Set = helper.AllocateAndWrite(m_DescriptorPool, m_Layout, textureDescriptors);
		SEDX_CORE_ASSERT(m_Set != VK_NULL_HANDLE, "DescriptorSet: failed to allocate descriptor set");
	}

	DescriptorSet::DescriptorSet(const Descriptor *descriptors, size_t count, const char *name)
	{
		m_ObjectName = name;
		m_Descriptors.reserve(count);
		m_Bindings.reserve(count);

		// build slot -> index map for O(1) lookups
		for (size_t i = 0; i < count; ++i)
		{
			m_Descriptors.push_back(descriptors[i]);
			m_Bindings.emplace_back(); // default binding
			m_SlotToIndex[descriptors[i].GetSlot()] = i;
		}

		// compute layout hash (immutable - based on slots and stages)
		for (const Descriptor& descriptor : m_Descriptors)
		{
			m_LayoutHash = HashCombine(m_LayoutHash, static_cast<uint64_t>(descriptor.GetSlot()));
			m_LayoutHash = HashCombine(m_LayoutHash, static_cast<uint64_t>(descriptor.GetStage()));
		}

	}

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

	DescriptorSet::DescriptorSet(DescriptorSet&& other) noexcept
	{
		m_Device = other.m_Device;
		m_Layout = other.m_Layout;
		m_DescriptorPool = other.m_DescriptorPool;
		m_Set = other.m_Set;
		m_Destroyed = other.m_Destroyed;

		other.m_Device = nullptr;
		other.m_Layout = VK_NULL_HANDLE;
		other.m_DescriptorPool = VK_NULL_HANDLE;
		other.m_Set = VK_NULL_HANDLE;
		other.m_Destroyed = true;
	}

	DescriptorSet& DescriptorSet::operator=(DescriptorSet&& other) noexcept
	{
		if (this == &other)
			return *this;

		Destroy();

		m_Device = other.m_Device;
		m_Layout = other.m_Layout;
		m_DescriptorPool = other.m_DescriptorPool;
		m_Set = other.m_Set;
		m_Destroyed = other.m_Destroyed;

		other.m_Device = nullptr;
		other.m_Layout = VK_NULL_HANDLE;
		other.m_DescriptorPool = VK_NULL_HANDLE;
		other.m_Set = VK_NULL_HANDLE;
		other.m_Destroyed = true;

		return *this;
	}

	void DescriptorSet::SetConstantBuffer(uint32_t slot, Buffer *constantBuffer)
	{
		uint32_t actualSlot = slot + SHADER_REGISTER_SHIFT_B;
		if (DescriptorBinding* binding = FindBinding(actualSlot))
		{
			// get descriptor for validation
			const Descriptor& descriptor = m_Descriptors[m_SlotToIndex[actualSlot]];

			binding->SetResource(static_cast<void*>(constantBuffer));
			binding->SetRange(static_cast<int64_t>(constantBuffer->GetStride()));
			binding->SetDynamicOffset(constantBuffer->GetOffset());

			SEDX_CORE_ASSERT(constantBuffer->GetStrideUnaligned() == descriptor.GetStructSize(), "Size mismatch between CPU and GPU side constant buffer");
			SEDX_CORE_ASSERT(binding->GetDynamicOffset() % binding->GetRange() == 0, "Incorrect dynamic offset");

			m_Dirty.SetDirty();
		}
	}

	void DescriptorSet::SetBuffer(uint32_t slot, Buffer *buffer)
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

	void DescriptorSet::SetTexture(uint32_t slot, ImageResource *img, uint32_t mipIndex, uint32_t mipRange, uint32_t layer)
	{
		bool mipSpecified = (mipIndex != ALL_MIPS); (void)mipSpecified;
		bool isStorage = img->IsUnorderedAccessView();
		Layout::ImageLayout layout = isStorage ? Layout::ImageLayout::General : Layout::ImageLayout::ShaderRead;

		SEDX_CORE_ASSERT(layout == Layout::ImageLayout::General || layout == Layout::ImageLayout::ShaderRead);

		uint32_t shift     = isStorage ? SHADER_REGISTER_SHIFT_U : SHADER_REGISTER_SHIFT_T;
		uint32_t actualSlot = slot + shift;

		if (DescriptorBinding* binding = FindBinding(actualSlot))
		{
			binding->SetResource(static_cast<void*>(img));
			binding->SetLayout(layout);
			binding->SetMip(mipIndex);
			binding->SetMipRange(mipRange);
			binding->SetArrayLayer(layer);
			m_Dirty.SetDirty();
		}
	}

	void DescriptorSet::GetDynamicOffsets(std::array<uint32_t, 10> *offsets, uint32_t *count)
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

	void *DescriptorSet::GetOrCreateDescriptorSet()
	{
		// compute hash only if dirty
		if (m_Dirty.IsDirty())
		{
			m_BindingHash = ComputeBindingHash();
			m_Dirty.Check();
		}

		// look up or create descriptor set
		std::unordered_map<uint64_t, DescriptorSet>& descriptor_sets = BindlessManager::GetDescriptorSets();
		auto it = descriptor_sets.find(m_BindingHash);

		if (it == descriptor_sets.end())
		{
			// build combined descriptors with bindings for descriptor set creation
			std::vector<DescriptorWithBinding> combined;
			combined.reserve(m_Descriptors.size());

			for (size_t i = 0; i < m_Descriptors.size(); ++i)
			{
				DescriptorWithBinding dwb(m_Descriptors[i], m_Bindings[i]);
				combined.push_back(dwb);
				// built via DescriptorWithBinding ctor
			}

			descriptor_sets[m_BindingHash] = DescriptorSet(combined, m_ObjectName.c_str());
			it = descriptor_sets.find(m_BindingHash);
		}

		return it->second.GetResource();
	}

	DescriptorSet::~DescriptorSet()
	{
		if (!m_Destroyed)
			Destroy();
	}

	void DescriptorSet::Destroy()
	{
		if (!m_Destroyed)
		{
			if (m_DescriptorPool != VK_NULL_HANDLE)
			{
				QueueManager::AddDeletionQueue(ResourceType::DescriptorPool, m_DescriptorPool);
				//vkDestroyDescriptorPool(m_Device->GetLogicalDevice(), m_DescriptorPool, nullptr);
				m_DescriptorPool = VK_NULL_HANDLE;
			}

			if (m_Layout != VK_NULL_HANDLE)
			{
				QueueManager::AddDeletionQueue(ResourceType::DescriptorSetLayout, m_Layout);
				//vkDestroyDescriptorSetLayout(m_Device->GetLogicalDevice(), m_Layout, nullptr);
				m_Layout = VK_NULL_HANDLE;
			}

			m_Destroyed = true;
		}
	}

	bool DescriptorSet::IsReferringToResource(void *resource) const
	{
		for (const auto &m_Binding : m_Bindings)
		{
			if (m_Binding.GetResource() == resource)
			{
				return true;
			}
		}

		return false;
	}

	uint64_t DescriptorSet::ComputeBindingHash() const
	{
		uint64_t hash = m_LayoutHash;
		for (const DescriptorBinding& binding : m_Bindings)
		{
			hash = HashCombine(hash, binding.GetHash());
		}

		return hash;
	}

	void DescriptorSet::Create()
	{
		SEDX_CORE_ASSERT(m_Layout == VK_NULL_HANDLE);

		// filter descriptors - exclude push constants and bindless arrays
		std::vector<Descriptor> filtered;
		filtered.reserve(m_Descriptors.size());
		for (const Descriptor& desc : m_Descriptors)
		{
			if (desc.GetType() == DescriptorType::PushConstantBuffer)
			{
				continue;
			}

			if (desc.IsArray() && desc.GetArrayLength() == MAX_ARRAY_SIZE)
			{
				continue;
			}

			filtered.push_back(desc);
		}

		if (filtered.empty())
		{
			// create empty layout
			VkDescriptorSetLayoutCreateInfo createInfo = {};
			createInfo.sType        = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
			createInfo.bindingCount = 0;
			createInfo.pBindings    = nullptr;

			SEDX_VK_RESULT_ASSERT(vkCreateDescriptorSetLayout(m_Device->GetLogicalDevice(), &createInfo, nullptr, reinterpret_cast<VkDescriptorSetLayout*>(&m_Layout)));
			Debugging::SetResourceName(m_Layout, ResourceType::DescriptorSetLayout, m_ObjectName.c_str());
			return;
		}

		// validate unique bindings
		{
			std::unordered_set<uint32_t> uniqueBindings;
			for (const Descriptor& desc : filtered)
			{
				bool inserted = uniqueBindings.insert(desc.GetSlot()).second;
				SEDX_CORE_ASSERT(inserted, "Duplicate binding slot detected");
			}
		}

		// build vulkan layout bindings
		constexpr uint32_t maxBindings = 255;
		std::array<VkDescriptorSetLayoutBinding, maxBindings> layoutBindings = {};
		std::array<VkDescriptorBindingFlags, maxBindings> bindingFlags = {};

		for (size_t i = 0; i < filtered.size(); ++i)
		{
			const Descriptor& desc = filtered[i];

			// convert stage mask to vulkan flags
			VkShaderStageFlags stageFlags = 0;
			if (desc.GetStage() & ShaderTypeToMask(Stage::Vertex))					stageFlags |= VK_SHADER_STAGE_VERTEX_BIT;
			if (desc.GetStage() & ShaderTypeToMask(Stage::TessellationControl))     stageFlags |= VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT;
			if (desc.GetStage() & ShaderTypeToMask(Stage::TessellationEvaluation))  stageFlags |= VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT;
			if (desc.GetStage() & ShaderTypeToMask(Stage::Fragment))				stageFlags |= VK_SHADER_STAGE_FRAGMENT_BIT;
			if (desc.GetStage() & ShaderTypeToMask(Stage::Compute))					stageFlags |= VK_SHADER_STAGE_COMPUTE_BIT;

			auto& binding              = layoutBindings[i];
			binding.binding            = desc.GetSlot();
			binding.descriptorType     = static_cast<VkDescriptorType>(static_cast<uint32_t>(desc.GetType()));
			binding.descriptorCount    = desc.IsArray() ? desc.GetArrayLength() : 1;
			binding.stageFlags         = stageFlags;
			binding.pImmutableSamplers = nullptr;

			bindingFlags[i] = VK_DESCRIPTOR_BINDING_PARTIALLY_BOUND_BIT;
		}

		VkDescriptorSetLayoutBindingFlagsCreateInfoEXT flags_info = {};
		flags_info.sType         = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_BINDING_FLAGS_CREATE_INFO_EXT;
		flags_info.bindingCount  = static_cast<uint32_t>(filtered.size());
		flags_info.pBindingFlags = bindingFlags.data();

		VkDescriptorSetLayoutCreateInfo create_info = {};
		create_info.sType        = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
		create_info.pNext        = &flags_info;
		create_info.bindingCount = static_cast<uint32_t>(filtered.size());
		create_info.pBindings    = layoutBindings.data();

		SEDX_VK_RESULT_ASSERT(vkCreateDescriptorSetLayout(m_Device->GetLogicalDevice(), &create_info, nullptr, reinterpret_cast<VkDescriptorSetLayout*>(&m_Layout)));
		Debugging::SetResourceName(m_Layout, ResourceType::DescriptorSetLayout, m_ObjectName.c_str());
	}

} // namespace SceneryEditorX

// -------------------------------------------------------
