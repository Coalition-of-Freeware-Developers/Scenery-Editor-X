/**
* -------------------------------------------------------
* Scenery Editor X
* -------------------------------------------------------
* Copyright (c) 2025 Thomas Ray 
* Copyright (c) 2025 Coalition of Freeware Developers
* -------------------------------------------------------
* hash.cpp
* -------------------------------------------------------
* Created: 16/7/2025
* -------------------------------------------------------
*/
#include <SceneryEditorX/core/identifiers/hash.h>

/// ---------------------------------------------

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

	size_t CalculateHash(const void *data, size_t length)
	{
#if IS_64BIT
		return XXH64(data, length, 0);
#else
		return XXH32(data, length, 0);
#endif
	}

	uint32_t CalculateCRC(const void *data, size_t size)
	{
		return CRC::Calculate(data, size, CRC::CRC_32());
	}

    uint32_t CalculateCRC(const void *data, size_t size, uint32_t crc)
	{
		return CRC::Calculate(data, size, CRC::CRC_32(), crc);
	}

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

/// ---------------------------------------------
