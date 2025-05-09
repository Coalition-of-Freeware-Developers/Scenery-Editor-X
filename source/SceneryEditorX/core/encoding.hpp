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

/// -------------------------------------------------------

namespace SceneryEditorX
{
	/**
	 * @struct UUID
	 * @brief Represents a universally unique identifier for scene objects.
	 * 
	 * The UUID structure provides a 64-bit identifier that can be used
	 * to uniquely identify objects within the scenery editor.
	 */
	struct UUID
	{
        uint64_t ID;  ///< 64-bit unique identifier value
	};

	/**
	 * @class Encoding
	 * @brief Provides encoding and hashing utilities for the SceneryEditorX application.
	 * 
	 * This class contains methods for encoding and decoding data in various formats,
	 * as well as hashing utilities for generating unique identifiers and combining hash values.
	 */
	class Encoding
	{
    public:
	    /**
	     * @brief Encodes a byte array to a base64 string.
	     * @param input Pointer to the input byte array
	     * @param len Length of the input array in bytes
	     * @return std::string The base64 encoded string
	     */
	    std::string EncodeBase64(const unsigned char *input, size_t len);
	    
	    /**
	     * @brief Decodes a base64 string to a byte array.
	     * @param input The base64 encoded string
	     * @return std::vector<uint8_t> Vector containing the decoded bytes
	     */
	    std::vector<uint8_t> DecodeBase64(const std::string &input);

		//static uint32_t HashUUID(const std::vector<uint32_t>& vec);

		/**
		 * @brief Combines a hash value with another value using the FNV-1a algorithm.
		 * @tparam T Type of the value to combine with the hash
		 * @param h Reference to the hash value to be updated
		 * @param v The value to combine with the hash
		 * 
		 * This template function uses the FNV-1a hashing algorithm to combine an existing
		 * hash value with another hashable value, producing a new combined hash.
		 */
		template <typename T>
        void HashCombine(uint32_t &h, const T &v)
        {
            std::hash<T> hash;
            h ^= hash(v) + 0x9e3779b9 + (h << 6) + (h >> 2);
        }

        /**
         * @brief Combines a hash value with a memory block's hash.
         * @param h Reference to the hash value to be updated
         * @param ptr Pointer to the memory block
         * @param size Size of the memory block in bytes
         * 
         * This function creates a hash from a memory block and combines it with
         * an existing hash value using a string_view-based approach.
         */
        static void HashCombine(uint32_t &h, void *ptr, const uint32_t size)
        {
            h = std::hash<std::string_view>()(std::string_view(static_cast<char *>(ptr), size));
        }

	    /**
	     * @brief Static ID used for generating unique identifiers.
	     * 
	     * This static variable may be used as a counter or base for
	     * generating unique identifiers within the application.
	     */
	    static uint64_t ID;
	};

} // namespace SceneryEditorX

// -------------------------------------------------------
