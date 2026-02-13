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
 * Created: 16/7/2025
 * -------------------------------------------------------
 */
#include "hash.h"

// ---------------------------------------------

namespace SceneryEditorX
{

    /*
    Hash128 CalculateHash128(const void *data, size_t length)
	{
		XXH128_hash_t hash = XXH3_128bits(data, length);
		Hash128 out;
		out.high64 = hash.high64;
		out.low64 = hash.low64;
		return out;
	}
	*/

	/*
	size_t CalculateHash(const void *data, size_t length)
	{
#if IS_64BIT
		return XXH64(data, length, 0);
#else
		return XXH32(data, length, 0);
#endif
	}
	*/

	/*
	uint32_t CalculateCRC(const void *data, size_t size)
	{
		return CRC::Calculate(data, size, CRC::CRC_32());
	}

    uint32_t CalculateCRC(const void *data, size_t size, uint32_t crc)
	{
		return CRC::Calculate(data, size, CRC::CRC_32(), crc);
	}
	*/

    /*
    size_t GetCombinedHashes(const Array<size_t> &hashes)
	{
		if (hashes.GetSize() == 0)
			return 0;
		
		size_t hash = hashes[0];

		for (int i = 1; i < hashes.GetSize(); i++)
		{
			hash = GetCombinedHash(hash, hashes[i]);
		}

		return hash;
	} 
	*/


}

// ---------------------------------------------
