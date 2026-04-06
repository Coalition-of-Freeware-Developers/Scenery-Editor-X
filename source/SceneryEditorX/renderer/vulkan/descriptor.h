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
#include "device.h"
#include <vector>

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
		TextureStorage,
		PushConstantBuffer,
		ConstantBuffer,
		StructuredBuffer,
		AccelerationStructure,
		MaxEnum
	};

	/**
	 * @struct DescriptorSpec
	 * @brief Specification for a descriptor, including type, layout, and other properties.
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
	 * @brief A helper class for managing Vulkan descriptor sets, layouts, and pools.
	 */
	class Descriptor 
	{
	public:
		Descriptor() = default;

		/**
		 * @brief Construct a new Descriptor object based on the provided specification.
		 * @param spec The specification for the descriptor.
		 */
		Descriptor(const DescriptorSpec &spec);

		/**
		 * @brief Destroy the Descriptor object and release any associated resources.
		 */
		~Descriptor();

		Descriptor(const Descriptor&);
		Descriptor& operator=(const Descriptor&);
		Descriptor(Descriptor&&) noexcept;
		Descriptor& operator=(Descriptor&&) noexcept;

		/**
		 * @brief Create a descriptor set layout with the specified number of bindings.
		 * @param bindingCount The number of bindings in the descriptor set layout.
		 * @return The VkDescriptorSetLayout handle.
		 */
		VkDescriptorSetLayout CreateLayout(uint32_t bindingCount) const;

		/**
		 * @brief Create a descriptor pool with the specified number of descriptors.
		 * @param descriptorCount The number of descriptors in the pool.
		 * @return The VkDescriptorPool handle.
		 */
		VkDescriptorPool CreatePool(uint32_t descriptorCount) const;

		/**
		 * @brief Allocate a descriptor set from the specified pool and layout, and update it with imageInfos.
		 * @param pool The descriptor pool from which to allocate the descriptor set.
		 * @param layout The descriptor set layout to use for the allocation.
		 * @param imageInfos Vector of VkDescriptorImageInfo structs to write into the allocated descriptor set.
		 * @return Returns VK_NULL_HANDLE on failure or the allocated VkDescriptorSet handle on success.
		 */
		VkDescriptorSet AllocateAndWrite(VkDescriptorPool pool, VkDescriptorSetLayout layout, const std::vector<::VkDescriptorImageInfo>& imageInfos) const;

		/**
		 * @brief Initialize the descriptor helper: create a layout for bindingCount combined image samplers and a pool sized for descriptorCount.
		 * @param bindingCount The number of bindings in the descriptor set layout.
		 * @param descriptorCount The number of descriptors in the descriptor pool.
		 * @return Returns true on success, false on failure.
		 */
		bool Init(uint32_t bindingCount = 1, uint32_t descriptorCount = 1);

		/**
		 * @brief Allocate a descriptor set from the internally owned pool/layout and update it with imageInfos.
		 * @param imageInfos Vector of VkDescriptorImageInfo structs to write into the allocated descriptor set.
		 * @return Returns VK_NULL_HANDLE on failure or the allocated VkDescriptorSet handle on success.
		 */
		VkDescriptorSet AllocateAndWrite(const std::vector<::VkDescriptorImageInfo>& imageInfos) const;

		/**
		 * @brief Get the internally owned descriptor pool.
		 * @return The VkDescriptorPool handle.
		 */
		VkDescriptorPool GetPool() const { return m_Pool; }

		/**
		 * @brief Get the internally owned descriptor set layout.
		 * @return The VkDescriptorSetLayout handle.
		 */
		VkDescriptorSetLayout GetLayout() const { return m_Layout; }

		/**
		 * @brief Check if the descriptor is a storage type.
		 * @return True if the descriptor is a storage type, false otherwise.
		 */
		bool IsStorage() const { return m_Type == DescriptorType::TextureStorage; }

		uint32_t GetSlot() const { return m_Slot; }
		uint32_t GetStage() const { return m_Stage; }
		uint32_t GetStructSize() const { return m_StructSize; }
		uint32_t GetArrayLength() const { return m_ArrayLength; }
		DescriptorType GetType() const { return m_Type; }
		bool IsArray() const { return m_AsArray; }
		void SetStage(uint32_t stage) { m_Stage = stage; }

	private:
		Ref<Device> m_Device;
		DescriptorSpec m_Spec;

		uint32_t m_Slot					= 0;
		uint32_t m_Stage				= 0;
		uint32_t m_StructSize			= 0;
		uint32_t m_ArrayLength			= 0;
		DescriptorType m_Type			= DescriptorType::MaxEnum;
		Layout::ImageLayout m_ImgLayout	= Layout::ImageLayout::MaxEnum;

		VkDescriptorPool m_Pool			= VK_NULL_HANDLE;
		VkDescriptorSetLayout m_Layout	= VK_NULL_HANDLE;
		bool m_AsArray					= false;
		std::string m_Name;
	};

}

// -------------------------------------------------------
