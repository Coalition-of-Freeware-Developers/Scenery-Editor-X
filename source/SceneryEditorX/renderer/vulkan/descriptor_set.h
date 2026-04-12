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
#include "SceneryEditorX/utils/inheritance.h"
#include <unordered_map>
#include <vector>
#include <SceneryEditorX/core/identifiers/flag.h>

// -----------------------------------------------------------------

namespace SceneryEditorX
{
	class PipelineState;
	class ImageResource;
	class Buffer;
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
		 * @brief Construct a new Descriptor Set object.
		 * @param descriptors Pointer to an array of descriptors.
		 * @param count The number of descriptors in the array.
		 * @param name The name of the descriptor set.
		 */
		DescriptorSet(const Descriptor* descriptors, size_t count, const char* name);

		/**
		 * @brief Construct a new Descriptor Set object.
		 * @param combined Vector of DescriptorWithBinding objects to initialize the descriptor set with.
		 * @param name The name of the descriptor set.
		 */
		DescriptorSet(const std::vector<DescriptorWithBinding>& combined, const char* name);

		DescriptorSet() = default;

		/* @brief Destroy the Descriptor Set object. */
		virtual ~DescriptorSet() override;

		DescriptorSet(DescriptorSet&&) noexcept;
		DescriptorSet& operator=(DescriptorSet&&) noexcept;

		// binding api - O(1) slot lookup

		/**
		 * @brief Set a constant buffer for the descriptor set.
		 * @param slot The slot to bind the constant buffer to.
		 * @param constantBuffer Pointer to the constant buffer resource.
		 */
		void SetConstantBuffer(uint32_t slot, Buffer* constantBuffer);

		/**
		 * @brief Set a buffer for the descriptor set.
		 * @param slot The slot to bind the buffer to.
		 * @param buffer Pointer to the buffer resource.
		 */
		void SetBuffer(uint32_t slot, Buffer* buffer);

		/**
		 * @brief Set a texture for the descriptor set.
		 * @param slot The slot to bind the texture to.
		 * @param img Pointer to the image resource.
		 * @param mipIndex The mip level index.
		 * @param mipRange The range of mip levels.
		 * @param layer The layer index (default is ALL_MIPS).
		 */
		void SetTexture(uint32_t slot, ImageResource* img, uint32_t mipIndex, uint32_t mipRange, uint32_t layer = ALL_MIPS);

		/**
		 * @brief Get the dynamic offsets for the bound buffers.
		 * @param offsets Pointer to an array to store the dynamic offsets.
		 * @param count Pointer to a variable to store the number of dynamic offsets.
		 */
		void GetDynamicOffsets(std::array<uint32_t, 10>* offsets, uint32_t* count);

		/**
		 * @brief Find a descriptor binding by its slot.
		 * @param slot The slot of the descriptor binding to find.
		 * @return Pointer to the DescriptorBinding if found, nullptr otherwise.
		 */
		DescriptorBinding *FindBinding(uint32_t slot);

		/* @brief Clear all bindings in the descriptor set. */
		void ClearBindings();

		/**
		 * @brief Get or create the Vulkan descriptor set.
		 * @return Pointer to the Vulkan descriptor set handle.
		 */
		void *GetOrCreateDescriptorSet();

		/**
		 * @brief Destroy the descriptor set and free the associated resources. 
		 * @note After calling this method, the DescriptorSet instance should not be used. 
		 */
		void Destroy();

		/**
		 * @brief Get the descriptors associated with the descriptor set.
		 * @return A constant reference to the vector of descriptors.
		 */
		const std::vector<Descriptor>& GetDescriptors() const { return m_Descriptors; }

		/**
		 * @brief Get the Vulkan descriptor set layout handle.
		 * @return The VkDescriptorSetLayout handle.
		 */
		VkDescriptorSetLayout GetLayout() const { return m_Layout; }

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
		 * @brief Get the current bindings of the descriptor set.
		 * @return A constant reference to the vector of descriptor bindings.
		 */
		const std::vector<DescriptorBinding>& GetBindings() const { return m_Bindings; }

		/**
		 * @brief Get the hash of the descriptor set layout.
		 * @return The hash value of the layout.
		 */
		uint64_t GetLayoutHash() const { return m_LayoutHash; }

		/**
		 * @brief Check if the descriptor set is referring to a specific resource.
		 * @param resource Pointer to the resource to check.
		 * @return True if the descriptor set is referring to the resource, false otherwise.
		 */
		bool IsReferringToResource(void* resource) const;

		/**
		 * @brief Compute the hash of the descriptor set bindings.
		 * @return The hash value of the bindings.
		 */
		uint64_t ComputeBindingHash() const;

		/**
		 * @brief 
		 * @param pso 
		 * @param cmdBuffer 
		 * @param pipelineLayout 
		 * @param descriptorSet 
		 */
		static void SetDynamicDescriptor(const PipelineState& pso, VkCommandBuffer cmdBuffer, VkPipelineLayout pipelineLayout, DescriptorSet* descriptorSet);

		/**
		 * @brief 
		 * @param pso 
		 * @param resource 
		 * @param pipeline_layout 
		 */
		void SetBindless(PipelineState pso, void *resource, void *pipeline_layout);

	private:
		Ref<Device> m_Device;
		void Create();

		// layout info (immutable after construction)
		std::vector<Descriptor> m_Descriptors;
		std::unordered_map<uint32_t, size_t> m_SlotToIndex; // slot -> index in m_Descriptors
		uint64_t m_LayoutHash = 0;

		VkDescriptorSetLayout m_Layout = VK_NULL_HANDLE;
		VkDescriptorSet m_Set = VK_NULL_HANDLE;

		// binding state (mutable)
		std::vector<DescriptorBinding> m_Bindings; // parallel to m_Descriptors
		uint64_t m_BindingHash = 0;
		Flag m_Dirty;
		bool m_Destroyed = false;
	};

} // namespace SceneryEditorX

// -----------------------------------------------------------------
