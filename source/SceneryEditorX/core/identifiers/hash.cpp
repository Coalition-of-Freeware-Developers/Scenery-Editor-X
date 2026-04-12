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
 * hash.cpp
 * -------------------------------------------------------
 * Created: 19/02/2026
 * -------------------------------------------------------
 */
// ReSharper disable CppInconsistentNaming
#include "hash.h"
#include <cstdint>

// -------------------------------------------------------

namespace SceneryEditorX
{
	Hash::Hash() : m_Hash(0)
	{
	}
	
	Hash::~Hash()
	{
		m_Hash = 0;
	}
	
	Hash::Hash(const uint64_t value) : m_Hash(value)
	{

	}

	Hash::Hash(const Hash &other)
	{
		m_Hash = other.m_Hash;
	}

	Hash &Hash::operator=(const Hash& other)
	{
		if (this != &other)
		{
			m_Hash = other.m_Hash;
		}
		return *this;
	}

	Hash::Hash(Hash &&other) noexcept : m_Hash(other.m_Hash)
	{
		other.m_Hash = 0;
	}

	Hash &Hash::operator=(Hash &&other) noexcept
	{
		if (this != &other)
		{
			m_Hash = other.m_Hash;
			other.m_Hash = 0;
		}
		return *this;
	}

	bool Hash::operator==(const Hash &other) const
	{
		return m_Hash == other.m_Hash;
	}

	bool Hash::operator!=(const Hash &other) const
	{
		return m_Hash != other.m_Hash;
	}

	uint64_t Hash::GenerateFNV1A(std::string_view str)
	{
		return GenerateFNV1A(str.data(), str.size());
	}

	uint64_t Hash::GenerateFNV1A(const void *data, size_t size)
	{
		uint64_t hash = FNV1A_OFFSET_BASIS;
		const uint8_t *bytes = static_cast<const uint8_t *>(data);
	
		for (size_t i = 0; i < size; ++i)
		{
			hash ^= static_cast<uint64_t>(bytes[i]);
			hash *= FNV1A_PRIME;
		}
	
		return hash;
	}

	Hash Hash::CreateFNV1A(std::string_view str)
	{
		return Hash(GenerateFNV1A(str));
	}

	Hash Hash::CreateFNV1A(const void *data, size_t size)
	{
		return Hash(GenerateFNV1A(data, size));
	}

	uint64_t Hash::Combine(uint64_t a, uint64_t b)
	{
		// Use multiplication-based combination for good distribution
		return a * 31 + b;
	}

	uint64_t Hash::CombineXOR(uint64_t a, uint64_t b)
	{
		// XOR-based combination with bit rotation for better mixing
		return a ^ RotateLeft(b, 17);
	}

	uint64_t Hash::RotateLeft(uint64_t hash, uint32_t bits)
	{
		bits %= 64; // Ensure bits is in valid range
		return (hash << bits) | (hash >> (64 - bits));
	}

	uint64_t Hash::RotateRight(uint64_t hash, uint32_t bits)
	{
		bits %= 64; // Ensure bits is in valid range
		return (hash >> bits) | (hash << (64 - bits));
	}

	uint64_t Hash::Mix(uint64_t hash)
	{
		// MurmurHash3 finalizer - provides good avalanche properties
		hash ^= hash >> 33;
		hash *= 0xff51afd7ed558ccdULL;
		hash ^= hash >> 33;
		hash *= 0xc4ceb9fe1a85ec53ULL;
		hash ^= hash >> 33;
		return hash;
	}

} // namespace SceneryEditorX

// -------------------------------------------------------
