/**
* -------------------------------------------------------
* Scenery Editor X
* -------------------------------------------------------
* Copyright (c) 2025 Thomas Ray
* Copyright (c) 2025 Coalition of Freeware Developers
* -------------------------------------------------------
* hash.h
* -------------------------------------------------------
* Created: 11/7/2025
* -------------------------------------------------------
*/
#pragma once
#include <cstdint>
#include <functional>
#include <string>
#include <string_view>

/// ---------------------------------------------

namespace SceneryEditorX
{

    /*
    #define DECL_HASH(Type)                                                                                            \
    template <>                                                                                                        \
    inline size_t GetHash<Type>(const Type &value)                                                                     \
    {                                                                                                                  \
        return std::hash<Type>()(value);                                                                               \
    }

    /**
     * @class Hash
     * @brief Provides cryptographic and non-cryptographic hash functions for the Scenery Editor X application.
     *
     * The Hash class contains static methods for generating hash values from strings and raw data.
     * It supports both FNV-1a and CRC32 hashing algorithms for different use cases within the
     * application architecture.
     *
     * @note All hash functions are implemented as constexpr where possible for compile-time
     *       evaluation and improved performance.
     *
     * @see UUID, Identifier classes for usage examples
     #1#
    class Hash
	{
	public:

        /**
         * @brief Generates a 32-bit FNV-1a hash from a string and length at compile time.
         *
         * This function implements the FNV-1a (Fowler-Noll-Vo) hash algorithm variant 1a,
         * which is a fast, non-cryptographic hash function. The algorithm processes each
         * character in the input string, XORing it with the current hash value before
         * multiplying by the FNV prime number.
         *
         * The FNV-1a algorithm steps:
         * 1. Initialize hash to FNV offset basis (2166136261u)
         * 2. For each byte: XOR hash with byte, then multiply by FNV prime (16777619u)
         * 3. XOR with null terminator and multiply by FNV prime (for C-string compatibility)
         *
         * @param str Pointer to the character data to hash
         * @param length Number of characters to process
         * @return uint32_t 32-bit hash value suitable for hash tables and quick comparisons
         *
         * @note This is a constexpr function, allowing compile-time hash calculation
         *       for constant string expressions.
         *
         * @warning This is not a cryptographically secure hash function. Do not use
         *          for security-sensitive applications.
         *
         * @example
         * @code
         * constexpr uint32_t shaderHash = Hash::GenerateFNVHash("vertex_shader", 13);
         * uint32_t dynamicHash = Hash::GenerateFNVHash(shaderName.c_str(), shaderName.length());
         * @endcode
         #1#
        static constexpr uint32_t GenerateFNVHash(const char* str, size_t length)
		{
			constexpr uint32_t FNV_PRIME = 16777619u;
			constexpr uint32_t OFFSET_BASIS = 2166136261u;

			const char* data = str;
			uint32_t hash = OFFSET_BASIS;
			for (size_t i = 0; i < length; ++i)
			{
				hash ^= *data++;
				hash *= FNV_PRIME;
			}
			hash ^= '\0';
			hash *= FNV_PRIME;

			return hash;
		}

        /**
         * @brief Generates a 32-bit FNV-1a hash from a std::string.
         *
         * This is a convenience wrapper around the primary FNV hash function.
         * It automatically extracts the data pointer and length from the std::string
         * and delegates to the main FNV implementation.
         *
         * @param str Standard string object to hash
         * @return uint32_t 32-bit FNV-1a hash value
         *
         * @note This function internally calls GenerateFNVHash(str.c_str(), str.length())
         *
         * @example
         * @code
         * std::string shaderName = "fragment_shader";
         * uint32_t nameHash = Hash::GenerateFNVHash(shaderName);
         * @endcode
         #1#
        static uint32_t GenerateFNVHash(const std::string& str)
        {
            return GenerateFNVHash(str.c_str(), str.length());
        }

        /**
         * @brief Generates a 32-bit CRC32 hash from a C-style string.
         *
         * This function implements the CRC-32 (Cyclic Redundancy Check) algorithm using
         * the IEEE 802.3 polynomial (0xEDB88320). CRC32 is commonly used for error detection
         * and provides good distribution properties for hash table applications.
         *
         * The algorithm uses a precomputed lookup table for efficient computation and
         * processes the string character by character until the null terminator is reached.
         *
         * @param str Null-terminated C-style string to hash
         * @return uint32_t 32-bit CRC32 hash value
         *
         * @note Uses IEEE 802.3 polynomial with initial value 0xFFFFFFFF
         * @note Final result is bitwise inverted (~crc) as per CRC32 standard
         *
         * @warning Input string must be null-terminated to avoid buffer overrun
         *
         * @example
         * @code
         * uint32_t fileHash = Hash::CRC32("scene_data.edx");
         * uint32_t nameHash = Hash::CRC32(objectName.c_str());
         * @endcode
         #1#
		static uint32_t CRC32(const char* str);

        /**
         * @brief Generates a 32-bit CRC32 hash from a std::string.
         *
         * This is a convenience wrapper around the C-style string CRC32 function.
         * It automatically extracts the C-string representation from the std::string
         * and delegates to the primary CRC32 implementation.
         *
         * @param string Standard string object to hash
         * @return uint32_t 32-bit CRC32 hash value
         *
         * @note This function internally calls CRC32(string.c_str())
         *
         * @example
         * @code
         * std::string assetPath = "models/building.obj";
         * uint32_t pathHash = Hash::CRC32(assetPath);
         * @endcode
         #1#
		static uint32_t CRC32(const std::string& string);
	};

	template<typename T>
	class Array;

	struct Hash128
	{
		uint64_t high64;
        uint64_t low64;
	};

	Hash128 CalculateHash128(const void *data, size_t length);

	size_t CalculateHash(const void *data, size_t length);

    /// Default implementation does not have any 'special' code other than for pointers. Specializations do all the work.
    template <typename T>
    size_t GetHash(const T &value)
    {
        constexpr bool isPointer = std::is_pointer_v<T>;
        
        typedef std::remove_pointer_t<T> WithoutPtrType;
		typedef THasGetHashFunction<T> GetHashFuncType;
        
        if constexpr (isPointer)
        {
            return (size_t)(WithoutPtrType *)value;
        }
		else if constexpr (GetHashFuncType::Value)
		{
			return GetHashFuncType::GetHash(&value);
		}
        else
        {
			return std::hash<T>()(value);
        }
    }

	template <class T>
    inline void CombineHash(size_t &seed, const T &v)
	{
		seed ^= GetHash<T>(v) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
	}

    inline size_t GetCombinedHash(size_t hashA, size_t hashB)
	{
		hashA ^= (hashB + 0x9e3779b9 + (hashA << 6) + (hashA >> 2));
		return hashA;
	}

	uint32_t CalculateCRC(const void *data, size_t size);
    uint32_t CalculateCRC(const void *data, size_t size, uint32_t crc);
	
	size_t GetCombinedHashes(const Array<size_t> &hashes);

	template<typename... Args>
    inline size_t GetHashes(const Args &...values)
	{
		return GetCombinedHashes({ GetHash<Args>(values)... });
	}

	DECL_HASH(uint8_t)
	DECL_HASH(uint16_t)
	DECL_HASH(uint32_t)
	DECL_HASH(uint64_t)

	DECL_HASH(int8_t)
	DECL_HASH(int16_t)
	DECL_HASH(int32_t)
	DECL_HASH(int64_t)

	DECL_HASH(f32)
	DECL_HASH(f64)

	DECL_HASH(bool)
	DECL_HASH(char)

	template<typename T>
	struct HashFunc
	{
		size_t operator()(const T& value) const
		{
			return GetHash(value);
		}
	};
	*/



}

/// ---------------------------------------------
