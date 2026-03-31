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
	 * @class Descriptor
	 * @brief A helper class for managing Vulkan descriptor sets, layouts, and pools.
	 */
	class Descriptor 
	{
	public:
		Descriptor();
		~Descriptor();

		VkDescriptorSetLayout CreateLayout(uint32_t bindingCount) const;
		VkDescriptorPool CreatePool(uint32_t descriptorCount) const;
		VkDescriptorSet AllocateAndWrite(VkDescriptorPool pool, VkDescriptorSetLayout layout, const std::vector<::VkDescriptorImageInfo>& imageInfos) const;

		// Non-copyable
		Descriptor(const Descriptor&) = delete;
		Descriptor& operator=(const Descriptor&) = delete;

		// Movable
		Descriptor(Descriptor&&) noexcept;
		Descriptor& operator=(Descriptor&&) noexcept;

		// Initialize the descriptor helper: create a layout for bindingCount
		// combined image samplers and a pool sized for descriptorCount.
		// Returns true on success.
		bool Init(uint32_t bindingCount = 1, uint32_t descriptorCount = 1);

		// Allocate a descriptor set from the internally owned pool/layout and
		// update it with imageInfos. Returns VK_NULL_HANDLE on failure.
		VkDescriptorSet AllocateAndWrite(const std::vector<::VkDescriptorImageInfo>& imageInfos) const;

		// Accessors
		VkDescriptorPool GetPool() const { return m_Pool; }
		VkDescriptorSetLayout GetLayout() const { return m_Layout; }

	private:
		Ref<Device> m_Device;
		VkDescriptorPool m_Pool = VK_NULL_HANDLE;
		VkDescriptorSetLayout m_Layout = VK_NULL_HANDLE;
	};

}

// -------------------------------------------------------
