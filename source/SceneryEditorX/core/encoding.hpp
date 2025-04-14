/**
* -------------------------------------------------------
* Scenery Editor X
* -------------------------------------------------------
* Copyright (c) 2025 Thomas Ray 
* Copyright (c) 2025 Coalition of Freeware Developers
* -------------------------------------------------------
* encoding.hpp
* -------------------------------------------------------
* Created: 9/4/2025
* -------------------------------------------------------
*/

#pragma once
#include <string>
#include <vector>

// -------------------------------------------------------
namespace SceneryEditorX
{
	// UUID structure
	struct UUID
	{
        uint64_t ID;
	};

	class Encoding
	{
    public:
	    // Function to encode a byte array to a base64 string
	    std::string EncodeBase64(unsigned char const *input, size_t len);
	    // Function to decode a base64 string to a byte array
	    std::vector<uint8_t> DecodeBase64(std::string const &input);

		uint32_t HashUUID(const std::vector<UUID>& vec);

		template <typename T>
        void HashCombine(uint32_t &h, const T &v)
        {
            std::hash<T> hash;
            h ^= hash(v) + 0x9e3779b9 + (h << 6) + (h >> 2);
        }

		inline void HashCombine(uint32_t &h, void *ptr, uint32_t size)
        {
            h = std::hash<std::string_view>()(std::string_view((char *)ptr, size));
        }


	};


} // namespace SceneryEditorX

// -------------------------------------------------------
