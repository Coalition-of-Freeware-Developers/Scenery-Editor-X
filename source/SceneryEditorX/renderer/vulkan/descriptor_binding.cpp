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
 * descriptor_binding.cpp
 * -------------------------------------------------------
 * Created: 01/04/2026
 * -------------------------------------------------------
 */
#include "descriptor_binding.h"

// -------------------------------------------------------

namespace SceneryEditorX
{

	static uint64_t HashCombine(uint64_t a, uint64_t b)
	{
		return a * 31 + b;
	}
	
	void DescriptorBinding::Reset()
	{
		m_Resource			= nullptr;
		m_Range				= 0;
		m_DynamicOffset		= 0;
		m_Mip				= 0;
		m_MipRange			= 0;
		m_ArrayLayer		= ALL_MIPS;
		m_Layout			= Layout::ImageLayout::MaxEnum;
	}

	uint64_t DescriptorBinding::GetHash() const
	{
		uint64_t hash = reinterpret_cast<uint64_t>(m_Resource);
		hash = HashCombine(hash, m_Range);
		hash = HashCombine(hash, static_cast<uint64_t>(m_Mip));
		hash = HashCombine(hash, static_cast<uint64_t>(m_MipRange));
		hash = HashCombine(hash, static_cast<uint64_t>(m_ArrayLayer));
		return hash;
	}

	DescriptorWithBinding::DescriptorWithBinding(Descriptor desc, const DescriptorBinding &binding) : m_Descriptor(std::move(desc)), m_Binding(binding)
	{
	}

	uint32_t DescriptorWithBinding::GetSlot() const
	{
		return m_Descriptor.GetSlot();
	}
	
	DescriptorType DescriptorWithBinding::GetType() const
	{
		return m_Descriptor.GetType();
	}
	
} // namespace SceneryEditorX

// -------------------------------------------------------
