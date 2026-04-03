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
 * descriptor_set.h
 * -------------------------------------------------------
 * Created: 09/02/2026
 * -------------------------------------------------------
 */
#pragma once
#include "descriptor.h"
#include "descriptor_binding.h"
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include <SceneryEditorX/core/identifiers/flag.h>

// -----------------------------------------------------------------

namespace SceneryEditorX
{
	class Device;

	/**
	 * @class DescriptorSet
	 * @brief Class representing a Vulkan descriptor set.
	 * 
	 * A descriptor set is a collection of resources (such as textures, buffers, and samplers)
	 * that shaders can access during rendering. This class encapsulates the creation, management,
	 * and destruction of a Vulkan descriptor set.
	 */
	class DescriptorSet : public SharedObject
	{
	public:
		/**
		 * @brief Construct a new Descriptor Set object.
		 * @param textureDescriptors The texture descriptors to initialize the descriptor set with.
		 */
		DescriptorSet(const std::vector<::VkDescriptorImageInfo>& textureDescriptors);

		/**
		 * @brief 
		 * @param descriptors 
		 * @param count 
		 * @param name 
		 */
		DescriptorSet(const Descriptor* descriptors, size_t count, const char* name);
		DescriptorSet(const std::vector<DescriptorWithBinding>& combined, const char* name);
		DescriptorSet() = default;
		~DescriptorSet();
		DescriptorSet(DescriptorSet&&) noexcept;
		DescriptorSet& operator=(DescriptorSet&&) noexcept;

		// binding api - O(1) slot lookup

		/**
		 * @brief 
		 * @param slot 
		 * @param constantBuffer 
		 */
		void SetConstantBuffer(uint32_t slot, Buffer* constantBuffer);

		/**
		 * @brief 
		 * @param slot 
		 * @param buffer 
		 */
		void SetBuffer(uint32_t slot, Buffer* buffer);

		/**
		 * @brief 
		 * @param slot 
		 * @param img 
		 * @param mipIndex 
		 * @param mipRange 
		 * @param layer 
		 */
		void SetTexture(uint32_t slot, ImageResource* img, uint32_t mipIndex, uint32_t mipRange, uint32_t layer = ALL_MIPS);

		// dynamic offsets for bound buffers

		/**
		 * @brief 
		 * @param offsets 
		 * @param count 
		 */
		void GetDynamicOffsets(std::array<uint32_t, 10>* offsets, uint32_t* count);

		/**
		 * @brief 
		 * @param slot 
		 * @return 
		 */
		DescriptorBinding *FindBinding(uint32_t slot);

		// state management

		/**
		 * @brief 
		 */
		void ClearBindings();

		/**
		 * @brief 
		 * @return 
		 */
		void* GetOrCreateDescriptorSet();

		/**
		 * @brief Destroy the descriptor set and free the associated resources. After calling this method, the DescriptorSet instance should not be used. 
		 */
		void Destroy();

		/**
		 * @brief 
		 * @return 
		 */
		const std::vector<Descriptor>& GetDescriptors() const { return m_Descriptors; }

		/**
		 * @brief Get the Vulkan descriptor set layout handle.
		 * @return The VkDescriptorSetLayout handle.
		 */
		VkDescriptorSetLayout GetLayout() const { return m_Layout; }

		/**
		 * @brief Get the Vulkan descriptor pool handle.
		 * @return The VkDescriptorPool handle.
		 */
		VkDescriptorPool GetPool() const { return m_DescriptorPool; }

		/**
		 * @brief Get the Vulkan descriptor set handle.
		 * @return The VkDescriptorSet handle.
		 */
		VkDescriptorSet GetSet() const { return m_Set; }

		/**
		 * @brief Get the underlying VkDescriptorSet as a void pointer.
		 * @return Pointer to the VkDescriptorSet handle.
		 */
		void* GetResource() const { return reinterpret_cast<void*>(m_Set); }

		/**
		 * @brief 
		 * @return 
		 */
		const std::vector<DescriptorBinding>& GetBindings() const { return m_Bindings; }

		/**
		 * @brief 
		 * @return 
		 */
		uint64_t GetLayoutHash() const { return m_LayoutHash; }

		/**
		 * @brief 
		 * @param resource 
		 * @return 
		 */
		bool IsReferringToResource(void* resource) const;

		/**
		 * @brief 
		 * @return 
		 */
		uint64_t ComputeBindingHash() const;

	private:
		Ref<Device> m_Device;
		void Create();

		// layout info (immutable after construction)
		std::vector<Descriptor> m_Descriptors;
		std::unordered_map<uint32_t, size_t> m_SlotToIndex; // slot -> index in m_Descriptors
		uint64_t m_LayoutHash = 0;

		VkDescriptorSetLayout m_Layout = VK_NULL_HANDLE;
		VkDescriptorPool m_DescriptorPool = VK_NULL_HANDLE;
		VkDescriptorSet m_Set = VK_NULL_HANDLE;
		
		std::string m_ObjectName;

		// binding state (mutable)
		std::vector<DescriptorBinding> m_Bindings; // parallel to m_Descriptors
		uint64_t m_BindingHash = 0;
		Flag m_Dirty;
		bool m_Destroyed = false;
	};

} // namespace SceneryEditorX

// -----------------------------------------------------------------
