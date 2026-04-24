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
 * descriptor.h
 * -------------------------------------------------------
 * Created: 09/02/2026
 * -------------------------------------------------------
 */
#pragma once
#include "enums.h"

// -------------------------------------------------------

namespace SceneryEditorX
{
	/**
	 * @enum DescriptorType
	 * @brief Defines the types of descriptors that can be used in the rendering system.
	 */
	enum class DescriptorType
	{
		Image,
		Sampler,
		TextureStorage,
		PushConstantBuffer,
		ConstantBuffer,
		StructuredBuffer,
		//AccelerationStructure,
		MaxEnum
	};

	/**
	 * @brief Convert a DescriptorType to the corresponding VkDescriptorType.
	 *
	 * Returns VK_DESCRIPTOR_TYPE_MAX_ENUM for types that have no direct Vulkan counterpart
	 * (e.g. PushConstantBuffer) — callers should filter those out before creating bindings.
	 *
	 * @param type The DescriptorType to convert.
	 * @return The corresponding VkDescriptorType value.
	 */
	[[nodiscard]] inline VkDescriptorType ToVkDescriptorType(DescriptorType type)
	{
		switch (type)
		{
			case DescriptorType::Image:                return VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
			case DescriptorType::TextureStorage:       return VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
			case DescriptorType::ConstantBuffer:       return VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
			case DescriptorType::StructuredBuffer:     return VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
			//case DescriptorType::AccelerationStructure:return VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR;
			default:                                   return VK_DESCRIPTOR_TYPE_MAX_ENUM;
		}
	}

	/**
	 * @struct DescriptorSpec
	 * @brief POD specification for a single shader binding (slot, type, stage mask, array info, name).
	 *
	 * DescriptorSpec is the authoritative description of a binding.  It carries no Vulkan
	 * resource handles — use DescriptorLayout to create the corresponding VkDescriptorSetLayout.
	 */
	struct DescriptorSpec
	{
		std::string name;
		DescriptorType type;
		Layout::ImageLayout layout;
		uint32_t slot;
		uint32_t stage;
		uint32_t structSize;
		bool asArray;
		uint32_t arrayLength;
	};

	/**
	 * @class Descriptor
	 * @brief Spec-only description of a single shader binding.
	 *
	 * A Descriptor carries the slot, type, stage mask, and optional array info for one
	 * binding.  It does NOT own any Vulkan resource handles.  Use DescriptorLayout to
	 * create the VkDescriptorSetLayout, and DescriptorPoolManager to allocate sets.
	 */
	class Descriptor
	{
	public:
		Descriptor() = default;

		/**
		 * @brief Construct a Descriptor from the provided specification.
		 * @param spec The binding specification.
		 */
	    Descriptor(const DescriptorSpec& spec);

		~Descriptor() = default;

		Descriptor(const Descriptor&)            = default;
		Descriptor& operator=(const Descriptor&) = default;
		Descriptor(Descriptor&&) noexcept        = default;
		Descriptor& operator=(Descriptor&&) noexcept = default;

		/**
		 * @brief Check if the descriptor is a storage image type.
		 * @return True if the descriptor type is TextureStorage.
		 */
		[[nodiscard]] bool IsStorage() const { return m_Spec.type == DescriptorType::TextureStorage; }

		[[nodiscard]] uint32_t       GetSlot()        const { return m_Spec.slot; }
		[[nodiscard]] uint32_t       GetStage()       const { return m_Spec.stage; }
		[[nodiscard]] uint32_t       GetStructSize()  const { return m_Spec.structSize; }
		[[nodiscard]] uint32_t       GetArrayLength() const { return m_Spec.arrayLength; }
		[[nodiscard]] DescriptorType GetType()        const { return m_Spec.type; }
		[[nodiscard]] bool           IsArray()        const { return m_Spec.asArray; }
		[[nodiscard]] const std::string& GetName()    const { return m_Spec.name; }

		/**
		 * @brief Merge stage flags from another descriptor for the same slot (used by shader reflection merging).
		 * @param stage Additional stage flags to OR into this descriptor's stage mask.
		 */
		void SetStage(const uint32_t stage) { m_Spec.stage = stage; }

	private:
		DescriptorSpec m_Spec;
	};

} // namespace SceneryEditorX

// -------------------------------------------------------
