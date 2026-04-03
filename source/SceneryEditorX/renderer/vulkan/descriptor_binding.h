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
		void Reset();
		[[nodiscard]] bool IsBound() const { return m_Resource != nullptr; }
		[[nodiscard]] uint64_t GetHash() const;
		[[nodiscard]] uint32_t GetDynamicOffset() const { return m_DynamicOffset; }
		[[nodiscard]] int64_t GetRange() const { return m_Range; }
		[[nodiscard]] void* GetResource() const { return m_Resource; }
		[[nodiscard]] Layout::ImageLayout GetLayout() const { return m_Layout; }
		[[nodiscard]] uint32_t GetMip() const { return m_Mip; }
		[[nodiscard]] uint32_t GetMipRange() const { return m_MipRange; }
		[[nodiscard]] uint32_t GetArrayLayer() const { return m_ArrayLayer; }

		void SetResource(void* res) { m_Resource = res; }
		void SetRange(int64_t range) { m_Range = range; }
		void SetDynamicOffset(uint32_t offset) { m_DynamicOffset = offset; }
		void SetLayout(Layout::ImageLayout layout) { m_Layout = layout; }
		void SetMip(uint32_t mip) { m_Mip = mip; }
		void SetMipRange(uint32_t mipRange) { m_MipRange = mipRange; }
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
		DescriptorWithBinding(Descriptor desc, const DescriptorBinding& binding);

		uint32_t GetSlot() const;
		DescriptorType GetType() const;
		Descriptor& GetDescriptor() { return m_Descriptor; }
		const Descriptor& GetDescriptor() const { return m_Descriptor; }
		DescriptorBinding& GetBinding() { return m_Binding; }
		const DescriptorBinding& GetBinding() const { return m_Binding; }
		bool IsBound() const { return m_Binding.IsBound(); }

	private:
		Descriptor m_Descriptor;
		DescriptorBinding m_Binding;
	};
}

// -------------------------------------------------------
