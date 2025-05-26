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
	     * @brief Static ID used for generating unique identifiers.
	     * 
	     * This static variable may be used as a counter or base for
	     * generating unique identifiers within the application.
	     */
        GLOBAL uint64_t ID;

	    /**
	     * @brief Encodes a byte array to a base64 string.
	     * @param input Pointer to the input byte array
	     * @param len Length of the input array in bytes
	     * @return std::string The base64 encoded string
	     */
        GLOBAL std::string EncodeBase64(const unsigned char *input, size_t len);
	    
	    /**
	     * @brief Decodes a base64 string to a byte array.
	     *
	     * @param input The base64 encoded string
	     * @return std::vector<uint8_t> containing the decoded bytes
	     */
        GLOBAL std::vector<uint8_t> DecodeBase64(const std::string &input);

        /**
         * @brief Generates a UUID from a vector of 32-bit integers.
         *
         * @param vec Vector of 32-bit integers representing the UUID
         * @return UUID The generated UUID
         */
        GLOBAL uint32_t HashUUID(const std::vector<uint32_t> &vec);

		/**
		 * @brief Combines a hash value with another value using the FNV-1a algorithm.
		 * 
		 * This template function uses the FNV-1a hashing algorithm to combine an existing
		 * hash value with another hashable value, producing a new combined hash.
		 *
		 * @tparam T Type of the value to combine with the hash
		 * @param h Reference to the hash value to be updated
		 * @param v The value to combine with the hash
		 */
		template <typename T>
        void HashCombine(uint32_t &h, const T &v)
        {
			std::hash<T> hash;
			uint64_t temp = static_cast<uint64_t>(hash(v)) + 0x9e3779b9 + (static_cast<uint64_t>(h) << 6) + (static_cast<uint64_t>(h) >> 2);
			h ^= static_cast<uint32_t>(temp); /// Cast back to uint32_t after the calculation
        }

        /**
         * @brief Combines a hash value with a memory block's hash.
         * 
         * This function creates a hash from a memory block and combines it with
         * an existing hash value using a string_view-based approach.
         *
         * @param h Reference to the hash value to be updated
         * @param ptr Pointer to the memory block
         * @param size Size of the memory block in bytes
         */
        GLOBAL void HashCombine(uint32_t &h, void *ptr, const uint32_t size)
        {
            h = std::hash<std::string_view>()(std::string_view(static_cast<char *>(ptr), size));
        }

	};

} // namespace SceneryEditorX

/// -------------------------------------------------------
