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
 * hash.h
 * -------------------------------------------------------
 * Created: 19/02/2026
 * -------------------------------------------------------
 */
// ReSharper disable CppInconsistentNaming
#pragma once

// -------------------------------------------------------

namespace SceneryEditorX
{
	/**
	 * @brief Hash utility class for generating and manipulating 64-bit hash values
	 * 
	 * This class provides FNV-1a hash generation and various hash operations.
	 * It is non-copyable but supports move semantics for efficient ownership transfer.
	 * 
	 * Supports:
	 * - FNV-1a hash generation from strings and byte arrays
	 * - Hash combination and manipulation operations
	 * - Comparison with FNV-1a prime and other hashes
	 */
	class Hash
	{
	public:
	    // FNV-1a constants for 64-bit hashing
		static constexpr uint64_t FNV1A_OFFSET_BASIS = 14695981039346656037ULL;
		static constexpr uint64_t FNV1A_PRIME = 1099511628211ULL;

        Hash();
	    ~Hash();
		explicit Hash(uint64_t value);

	    Hash(const Hash &other);					// Copyable
        Hash &operator=(const Hash &);				// Copyable
        Hash(Hash &&other) noexcept;                // Move constructor
        Hash &operator=(Hash &&other) noexcept;     // Move assignment operator

	    /**
		 * @brief Equality comparison operator
		 * @param other Hash to compare with
		 * @return true if hashes are equal
		 */
		bool operator==(const Hash &other) const;

	    /**
		 * @brief Inequality comparison operator
		 * @param other Hash to compare with
		 * @return true if hashes are not equal
		 */
		bool operator!=(const Hash &other) const;

	    /**
		 * @brief Get the current hash value
		 * @return 64-bit hash value
		 */
		[[nodiscard]] uint64_t GetHash() const { return m_Hash; }

		/**
		 * @brief Check if hash is zero
		 * @return true if hash value is 0
		 */
		[[nodiscard]] bool IsZero() const { return m_Hash == 0; }

		/**
		 * @brief Check if hash equals the FNV-1a prime constant
		 * @return true if hash equals FNV-1a prime
		 */
		[[nodiscard]] bool IsFNV1APrime() const { return m_Hash == FNV1A_PRIME; }

		/**
		 * @brief Check if hash equals the FNV-1a offset basis constant
		 * @return true if hash equals FNV-1a offset basis
		 */
		[[nodiscard]] bool IsFNV1AOffsetBasis() const { return m_Hash == FNV1A_OFFSET_BASIS; }

	    /**
		 * @brief Reset hash to zero
		 */
		void Reset() { m_Hash = 0; }

		/**
		 * @brief Set hash to explicit value
		 * @param value New hash value
		 */
		void SetHash(const uint64_t value) { m_Hash = value; }

		/**
		 * @brief Generate FNV-1a hash from string
		 * @param str String to hash
		 * @return FNV-1a hash value
		 */
		static uint64_t GenerateFNV1A(std::string_view str);

		/**
		 * @brief Generate FNV-1a hash from byte array
		 * @param data Pointer to data buffer
		 * @param size Size of data in bytes
		 * @return FNV-1a hash value
		 */
		static uint64_t GenerateFNV1A(const void* data, size_t size);

		/**
		 * @brief Generate FNV-1a hash from typed data
		 * @tparam T Type of data (must be trivially copyable)
		 * @param data Reference to data to hash
		 * @return FNV-1a hash value
		 */
		template<typename T>
		static uint64_t GenerateFNV1A(const T& data)
		{
			SEDX_TRIVIAL_STATIC_ASSERT(T);
			return GenerateFNV1A(&data, sizeof(T));
		}

		/**
		 * @brief Create Hash object with FNV-1a hash of string
		 * @param str String to hash
		 * @return Hash object containing FNV-1a hash
		 */
		static Hash CreateFNV1A(std::string_view str);

		/**
		 * @brief Create Hash object with FNV-1a hash of byte array
		 * @param data Pointer to data buffer
		 * @param size Size of data in bytes
		 * @return Hash object containing FNV-1a hash
		 */
		static Hash CreateFNV1A(const void* data, size_t size);

		/**
		 * @brief Combine two hash values using multiplication method
		 * @param a First hash value
		 * @param b Second hash value
		 * @return Combined hash value
		 */
		static uint64_t Combine(uint64_t a, uint64_t b);

		/**
		 * @brief Combine two hash values using XOR method
		 * @param a First hash value
		 * @param b Second hash value
		 * @return Combined hash value using XOR
		 */
		static uint64_t CombineXOR(uint64_t a, uint64_t b);

		/**
		 * @brief Rotate hash value left by specified bits
		 * @param hash Hash value to rotate
		 * @param bits Number of bits to rotate (0-63)
		 * @return Rotated hash value
		 */
		static uint64_t RotateLeft(uint64_t hash, uint32_t bits);

		/**
		 * @brief Rotate hash value right by specified bits
		 * @param hash Hash value to rotate
		 * @param bits Number of bits to rotate (0-63)
		 * @return Rotated hash value
		 */
		static uint64_t RotateRight(uint64_t hash, uint32_t bits);

		/**
		 * @brief Mix hash value with additional data (useful for hash finalization)
		 * @param hash Hash value to mix
		 * @return Mixed hash value
		 */
		static uint64_t Mix(uint64_t hash);

	private:
        uint64_t m_Hash = 0;
	};

}

// -------------------------------------------------------
