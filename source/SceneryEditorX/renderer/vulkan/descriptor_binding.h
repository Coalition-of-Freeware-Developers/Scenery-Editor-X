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
 * descriptor_binding.h
 * -------------------------------------------------------
 * Created: 01/04/2026
 * -------------------------------------------------------
 */
#pragma once
#include "descriptor.h"
#include "render_data.h"

// -------------------------------------------------------

namespace SceneryEditorX
{
	/**
	 * @class DescriptorBinding
	 * @brief binding state, mutable, set at runtime
	 */
	class DescriptorBinding
	{
	public:
		/**
		 * @brief Reset the binding state
		 */
		void Reset();

		/**
		 * @brief Check if the binding is currently bound
		 * @return True if bound, false otherwise
		 */
		[[nodiscard]] bool IsBound() const { return m_Resource != nullptr; }

		/**
		 * @brief Get the hash of the binding state
		 * @return The hash value
		 */
		[[nodiscard]] uint64_t GetHash() const;

		/**
		 * @brief Get the dynamic offset of the binding
		 * @return The dynamic offset value
		 */
		[[nodiscard]] uint32_t GetDynamicOffset() const { return m_DynamicOffset; }

		/**
		 * @brief Get the range of the binding
		 * @return The range value
		 */
		[[nodiscard]] int64_t GetRange() const { return m_Range; }

		/**
		 * @brief Get the resource of the binding
		 * @return The resource pointer
		 */
		[[nodiscard]] void* GetResource() const { return m_Resource; }

		/**
		 * @brief Get the layout of the binding
		 * @return The layout value
		 */
		[[nodiscard]] Layout::ImageLayout GetLayout() const { return m_Layout; }

		/**
		 * @brief Get the mip level of the binding
		 * @return The mip level value
		 */
		[[nodiscard]] uint32_t GetMip() const { return m_Mip; }

		/**
		 * @brief Get the mip range of the binding
		 * @return The mip range value
		 */
		[[nodiscard]] uint32_t GetMipRange() const { return m_MipRange; }

		/**
		 * @brief Get the array layer of the binding
		 * @return The array layer value
		 */
		[[nodiscard]] uint32_t GetArrayLayer() const { return m_ArrayLayer; }

		/**
		 * @brief Set the resource of the binding
		 * @param res The resource pointer
		 */
		void SetResource(void* res) { m_Resource = res; }

		/**
		 * @brief Set the range of the binding
		 * @param range The range value
		 */
		void SetRange(int64_t range) { m_Range = range; }

		/**
		 * @brief Set the dynamic offset of the binding
		 * @param offset The dynamic offset value
		 */
		void SetDynamicOffset(uint32_t offset) { m_DynamicOffset = offset; }

		/**
		 * @brief Set the layout of the binding
		 * @param layout The layout value
		 */
		void SetLayout(Layout::ImageLayout layout) { m_Layout = layout; }

		/**
		 * @brief Set the mip level of the binding
		 * @param mip The mip level value
		 */
		void SetMip(uint32_t mip) { m_Mip = mip; }

		/**
		 * @brief Set the mip range of the binding
		 * @param mipRange The mip range value
		 */
		void SetMipRange(uint32_t mipRange) { m_MipRange = mipRange; }

		/**
		 * @brief Set the array layer of the binding
		 * @param layer The array layer value
		 */
		void SetArrayLayer(uint32_t layer) { m_ArrayLayer = layer; }

	private:
		// layout properties (from reflection)
		int64_t m_Range				 = 0;
		uint32_t m_DynamicOffset	 = 0;
		uint32_t m_Mip				 = 0;
		uint32_t m_MipRange			 = 0;
		uint32_t m_ArrayLayer		 = ALL_MIPS; // ALL_MIPS = all layers (default), otherwise specific layer index
		Layout::ImageLayout m_Layout = Layout::ImageLayout::MaxEnum;
		void* m_Resource			 = nullptr;
	};

	/**
	 * @class DescriptorWithBinding
	 * @brief combined descriptor with binding for descriptor set creation
	 */
	class DescriptorWithBinding
	{
	public:
		DescriptorWithBinding() = default;

		/**
		 * @brief Construct a DescriptorWithBinding
		 * @param desc The descriptor
		 * @param binding The descriptor binding
		 */
		DescriptorWithBinding(Descriptor desc, const DescriptorBinding& binding);

		/**
		 * @brief Get the slot of the descriptor
		 * @return The slot value
		 */
		[[nodiscard]] uint32_t GetSlot() const;

		/**
		 * @brief Get the type of the descriptor
		 * @return The descriptor type
		 */
		[[nodiscard]] DescriptorType GetType() const;

		/**
		 * @brief Get the descriptor
		 * @return The descriptor reference
		 */
		Descriptor& GetDescriptor() { return m_Descriptor; }

		/**
		 * @brief Get the descriptor
		 * @return The descriptor reference
		 */
		[[nodiscard]] const Descriptor& GetDescriptor() const { return m_Descriptor; }

		/**
		 * @brief Get the binding of the descriptor
		 * @return The binding reference
		 */
		DescriptorBinding& GetBinding() { return m_Binding; }

		/**
		 * @brief Get the binding of the descriptor
		 * @return The binding reference
		 */
		[[nodiscard]] const DescriptorBinding& GetBinding() const { return m_Binding; }

		/**
		 * @brief Check if the descriptor is bound
		 * @return True if the descriptor is bound, false otherwise
		 */
		[[nodiscard]] bool IsBound() const { return m_Binding.IsBound(); }

	private:
		Descriptor m_Descriptor;
		DescriptorBinding m_Binding;
	};
}

// -------------------------------------------------------
