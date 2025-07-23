/**
* -------------------------------------------------------
* Scenery Editor X
* -------------------------------------------------------
* Copyright (c) 2025 Thomas Ray
* Copyright (c) 2025 Coalition of Freeware Developers
* -------------------------------------------------------
* uuid.h
* -------------------------------------------------------
* Created: 13/7/2025
* -------------------------------------------------------
*
* This file provides UUID (Universally Unique Identifier) functionality for the
* Scenery Editor X application. It implements 32-bit, 64-bit, and 128-bit UUID variants
* to optimize memory usage and performance for different use cases.
*
* Key Features:
* - Thread-safe UUID generation using cryptographically secure RNG
* - STL hash specializations for use in hash-based containers
* - Explicit conversion operators to prevent accidental type confusion
* - Minimal memory footprint with optimized storage
* - Support for deterministic construction from explicit values
* - Standard 128-bit UUID support for maximum uniqueness
* - Base64 encoding/decoding utilities for UUID serialization
* - Hash combination utilities for creating composite identifiers
*
* Usage Examples:
* @code
* // Generate random UUIDs
* UUID entityId;           // 64-bit UUID for persistent entities
* UUID32 eventToken;       // 32-bit UUID for temporary objects
* UUID128 standardId;      // 128-bit standard UUID for maximum uniqueness
*
* // Create from explicit values (e.g., for serialization)
* UUID loadedEntity(0x123456789ABCDEF0);
* UUID32 savedToken(0x12345678);
* UUID128 loadedStandardId({0x12345678, 0x9ABCDEF0, 0x11111111, 0x22222222});
*
* // Use in containers
* std::unordered_map<UUID, Entity> entityMap;
* std::unordered_set<UUID32> activeTokens;
* std::unordered_map<UUID128, Asset> assetRegistry;
*
* // Convert for serialization
* uint64_t serializedId = static_cast<uint64_t>(entityId);
* std::string base64Id = UUID::EncodeBase64(reinterpret_cast<const unsigned char*>(&entityId), sizeof(entityId));
* @endcode
*
* @see uuid-system-documentation.md for comprehensive usage guide
* @see Identifier class for string-based identifiers
* @see Hash class for additional hashing utilities
* -------------------------------------------------------
*/
#pragma once
#include <cstdint>
#include <functional>
#include <vector>
#include <string>
#include <array>

#include <SceneryEditorX/asset/asset.h>

/// -------------------------------------------------------

namespace SceneryEditorX
{
	///< Forward declarations
	class UUID;
	class UUID32;
	class UUID128;

    namespace Utils
    {
        /**
         * @class UUID
         * @brief Utility functions for UUID operations including encoding, decoding, and hashing.
         *
         * This class provides static utility functions for various UUID-related operations
         * including Base64 encoding/decoding and hash combination utilities that can be
         * used with all UUID types.
         *
         * @thread_safety All methods are thread-safe unless otherwise noted.
         */
	    class UUID
	    {
	    public:
	        /**
             * @brief Encodes a byte array to a base64 string.
             *
             * Converts binary data to a base64-encoded string representation suitable
             * for text-based transmission or storage. The encoding follows the standard
             * base64 alphabet and padding conventions.
             *
             * @param input Pointer to the input byte array to encode
             * @param len Length of the input array in bytes
             *
             * @return std::string The base64 encoded string representation
             *
             * @pre input must not be nullptr if len > 0
             * @pre len must accurately represent the size of the input buffer
             *
             * @thread_safety Thread-safe. No shared state is accessed.
             *
             * @complexity O(n) where n is the length of the input
             *
             * @example
             * @code
             * const unsigned char data[] = {0x48, 0x65, 0x6C, 0x6C, 0x6F}; // "Hello"
             * std::string encoded = UUIDUtilities::EncodeBase64(data, sizeof(data));
             * // Result: "SGVsbG8="
             * @endcode
             */
	        static std::string EncodeBase64(const unsigned char* input, size_t len);

	        /**
             * @brief Decodes a base64 string to a byte array.
             *
             * Converts a base64-encoded string back to its original binary representation.
             * The function handles standard base64 padding and validates input format.
             *
             * @param input The base64 encoded string to decode
             *
             * @return std::vector<uint8_t> containing the decoded bytes
             *
             * @throws std::invalid_argument if the input string contains invalid base64 characters
             * @throws std::runtime_error if the input string has invalid padding
             *
             * @thread_safety Thread-safe. No shared state is accessed.
             *
             * @complexity O(n) where n is the length of the input string
             *
             * @example
             * @code
             * std::string encoded = "SGVsbG8=";
             * auto decoded = UUIDUtilities::DecodeBase64(encoded);
             * // Result: {0x48, 0x65, 0x6C, 0x6C, 0x6F} // "Hello"
             * @endcode
             */
	        static std::vector<uint8_t> DecodeBase64(const std::string& input);

	        /**
             * @brief Generates a hash from a vector of 32-bit integers.
             *
             * Creates a 32-bit hash value from a vector of unsigned 32-bit integers
             * using a hash algorithm suitable for UUID-like identifier generation.
             * The function processes all elements in the vector to produce a
             * deterministic hash value.
             *
             * @param vec Vector of 32-bit integers to hash
             *
             * @return uint32_t The computed hash value
             *
             * @pre The vector should not be empty for meaningful results
             *
             * @thread_safety Thread-safe. No shared state is accessed.
             *
             * @complexity O(n) where n is the size of the input vector
             *
             * @example
             * @code
             * std::vector<uint32_t> data = {0x12345678, 0x9ABCDEF0, 0x11111111};
             * uint32_t hash = UUIDUtilities::HashUUID(data);
             * @endcode
             */
	        static uint32_t HashUUID(const std::vector<uint32_t>& vec);

	        /**
             * @brief Combines a hash value with another value using the FNV-1a algorithm.
             *
             * This template function uses a modified FNV-1a hashing algorithm to combine an existing
             * hash value with another hashable value, producing a new combined hash. This is useful
             * for building composite hash values from multiple data elements.
             *
             * @tparam T Type of the value to combine with the hash (must be hashable by std::hash<T>)
             * @param h Reference to the hash value to be updated in-place
             * @param v The value to combine with the hash
             *
             * @thread_safety Thread-safe if std::hash<T> is thread-safe for type T.
             *
             * @complexity O(1) plus the complexity of std::hash<T> for type T
             *
             * @example
             * @code
             * uint32_t combinedHash = 0;
             * UUIDUtilities::HashCombine(combinedHash, std::string("hello"));
             * UUIDUtilities::HashCombine(combinedHash, 42);
             * UUIDUtilities::HashCombine(combinedHash, 3.14f);
             * @endcode
             */
	        template <typename T>
            static void HashCombine(uint32_t& h, const T& v)
	        {
	            std::hash<T> hash;
	            const uint64_t temp = static_cast<uint64_t>(hash(v)) + 0x9e3779b9 + (static_cast<uint64_t>(h) << 6) + (static_cast<uint64_t>(h) >> 2);
	            h ^= static_cast<uint32_t>(temp); // Cast back to uint32_t after the calculation
	        }

	        /**
             * @brief Combines a hash value with a memory block's hash.
             *
             * This function creates a hash from an arbitrary memory block and combines it with
             * an existing hash value using a string_view-based approach. This allows hashing
             * of any contiguous memory region, making it useful for hashing structs, arrays,
             * or other binary data.
             *
             * @param h Reference to the hash value to be updated with the memory block's hash
             * @param ptr Pointer to the memory block to hash
             * @param size Size of the memory block in bytes
             *
             * @pre ptr must not be nullptr if size > 0
             * @pre size must accurately represent the accessible memory size at ptr
             * @pre The memory block must remain valid for the duration of the call
             *
             * @warning This function reads raw memory. Ensure the memory block does not
             *          contain uninitialized padding bytes that could affect hash consistency.
             *
             * @thread_safety Thread-safe if the memory block is not modified concurrently.
             *
             * @complexity O(n) where n is the size of the memory block
             *
             * @example
             * @code
             * struct Data { int x; float y; };
             * Data data = {42, 3.14f};
             *
             * uint32_t hash = 0;
             * UUIDUtilities::HashCombine(hash, &data, sizeof(data));
             * @endcode
             */
	        static void HashCombine(uint32_t& h, void* ptr, const uint32_t size);
	    };
    }

	/**
	 * @class UUID128
	 * @brief 128-bit Universally Unique Identifier following RFC 4122 standard.
	 *
	 * The UUID128 class provides a full 128-bit UUID implementation that follows
	 * the RFC 4122 standard for maximum uniqueness and compatibility with other
	 * systems. This is the most robust UUID variant but uses more memory than
	 * the 32-bit and 64-bit variants.
	 *
	 * This class is ideal for scenarios where maximum uniqueness is required,
	 * such as persistent asset identifiers, database keys, or distributed
	 * system identifiers that need to be globally unique.
	 *
	 * @note The probability of collision is astronomically low (approximately 1 in 2^122)
	 *       making it suitable for any application requiring guaranteed uniqueness.
	 *
	 * @thread_safety Thread-safe for construction and conversion operations.
	 *
	 * @example
	 * @code
	 * // Generate random UUID128
	 * UUID128 assetId;
	 *
	 * // Create from explicit value
	 * std::array<uint32_t, 4> data = {0x12345678, 0x9ABCDEF0, 0x11111111, 0x22222222};
	 * UUID128 knownId(data);
	 *
	 * // Use in containers requiring maximum uniqueness
	 * std::unordered_map<UUID128, AssetData> globalAssets;
	 * globalAssets[assetId] = myAsset;
	 * @endcode
	 */
	class UUID128
	{
	public:
		/**
		 * @brief Default constructor that generates a random 128-bit UUID.
		 *
		 * Creates a new UUID128 with a randomly generated 128-bit value using a
		 * cryptographically secure random number generator. Each call to this
		 * constructor will produce a unique identifier with astronomically high probability.
		 *
		 * @thread_safety Thread-safe. Multiple threads can safely create UUID128s
		 *                simultaneously without synchronization.
		 *
		 * @complexity O(1) - constant time operation
		 *
		 * @example
		 * @code
		 * UUID128 assetId;      // Generates random UUID128
		 * UUID128 anotherId;    // Generates different random UUID128
		 * @endcode
		 */
		UUID128();

		/**
		 * @brief Explicit constructor that creates a UUID128 from four 32-bit values.
		 *
		 * Creates a UUID128 with the specified 128-bit value represented as an array
		 * of four 32-bit integers. This constructor is marked as explicit to prevent
		 * accidental implicit conversions.
		 *
		 * @param uuid Array of four 32-bit values to use for this UUID128
		 *
		 * @warning Using this constructor with non-random values may increase the
		 *          probability of collisions. Use with caution.
		 *
		 * @thread_safety Thread-safe. No shared state is accessed.
		 *
		 * @complexity O(1) - constant time operation
		 *
		 * @example
		 * @code
		 * // Create UUID128 from saved values
		 * std::array<uint32_t, 4> savedId = {0x12345678, 0x9ABCDEF0, 0x11111111, 0x22222222};
		 * UUID128 restoredId(savedId);
		 * @endcode
		 */
		explicit UUID128(const std::array<uint32_t, 4>& uuid);

		/**
		 * @brief Copy constructor that creates a UUID128 from another UUID128.
		 *
		 * Creates a new UUID128 that is an exact copy of the provided UUID128. This is
		 * the standard copy constructor that allows UUID128s to be copied safely.
		 *
		 * @param other The UUID128 to copy from
		 *
		 * @thread_safety Thread-safe. No shared state is accessed.
		 *
		 * @complexity O(1) - constant time operation
		 *
		 * @example
		 * @code
		 * UUID128 original;
		 * UUID128 copy(original);  // copy has same value as original
		 * @endcode
		 */
		UUID128(const UUID128& other);

		/**
		 * @brief Explicit conversion operator to array of uint32_t.
		 *
		 * Provides explicit conversion to the underlying 128-bit value represented
		 * as an array of four 32-bit integers. This is marked as explicit to prevent
		 * accidental implicit conversions and to make serialization operations explicit.
		 *
		 * @return The underlying 128-bit value as an array of four uint32_t
		 *
		 * @thread_safety Thread-safe. No shared state is accessed.
		 *
		 * @complexity O(1) - constant time operation
		 *
		 * @example
		 * @code
		 * UUID128 id;
		 * auto values = static_cast<std::array<uint32_t, 4>>(id);  // Explicit conversion
		 * @endcode
		 */
		explicit operator std::array<uint32_t, 4>() const { return m_UUID; }

		/**
		 * @brief Equality comparison operator.
		 *
		 * Compares two UUID128 instances for equality by comparing their underlying
		 * 128-bit values. Two UUID128s are considered equal if they have the same
		 * internal values in all four 32-bit components.
		 *
		 * @param other The UUID128 to compare with
		 * @return true if the UUID128s are equal, false otherwise
		 *
		 * @thread_safety Thread-safe. No shared state is accessed.
		 *
		 * @complexity O(1) - constant time operation
		 *
		 * @example
		 * @code
		 * UUID128 id1;
		 * UUID128 id2(id1);
		 * bool areEqual = (id1 == id2);  // true
		 * @endcode
		 */
		bool operator==(const UUID128& other) const { return m_UUID == other.m_UUID; }

		/**
		 * @brief Inequality comparison operator.
		 *
		 * Compares two UUID128 instances for inequality by comparing their underlying
		 * 128-bit values. Two UUID128s are considered not equal if they have different
		 * internal values in any of the four 32-bit components.
		 *
		 * @param other The UUID128 to compare with
		 * @return true if the UUID128s are not equal, false otherwise
		 *
		 * @thread_safety Thread-safe. No shared state is accessed.
		 *
		 * @complexity O(1) - constant time operation
		 *
		 * @example
		 * @code
		 * UUID128 id1;
		 * UUID128 id2;
		 * bool areNotEqual = (id1 != id2);  // almost certainly true (randomly generated)
		 * @endcode
		 */
		bool operator!=(const UUID128& other) const { return m_UUID != other.m_UUID; }

		/**
		 * @brief Converts the UUID128 to a Base64 encoded string.
		 *
		 * Provides a convenient method to serialize the UUID128 to a Base64 string
		 * for text-based storage or transmission.
		 *
		 * @return std::string containing the Base64 encoded representation
		 *
		 * @thread_safety Thread-safe. No shared state is accessed.
		 *
		 * @complexity O(1) - constant time operation
		 *
		 * @example
		 * @code
		 * UUID128 id;
		 * std::string encoded = id.ToBase64();
		 * @endcode
		 */
		std::string ToBase64() const;

		/**
		 * @brief Creates a UUID128 from a Base64 encoded string.
		 *
		 * Static method to deserialize a UUID128 from a Base64 string created
		 * by the ToBase64() method.
		 *
		 * @param base64 The Base64 encoded string representation
		 * @return UUID128 instance created from the decoded data
		 *
		 * @throws std::invalid_argument if the input string is not valid Base64
		 * @throws std::runtime_error if the decoded data is not the correct size
		 *
		 * @thread_safety Thread-safe. No shared state is accessed.
		 *
		 * @complexity O(1) - constant time operation
		 *
		 * @example
		 * @code
		 * std::string encoded = "base64_encoded_uuid";
		 * UUID128 id = UUID128::FromBase64(encoded);
		 * @endcode
		 */
		static UUID128 FromBase64(const std::string& base64);

	private:
		friend struct std::hash<UUID128>;  ///< Allow hash specialization access
		std::array<uint32_t, 4> m_UUID;   ///< The underlying 128-bit identifier value
	};

	/**
	 * @class UUID
	 * @brief 64-bit Universally Unique Identifier for persistent entities and objects.
	 *
	 * The UUID class provides a robust mechanism for generating unique 64-bit identifiers
	 * suitable for persistent entities, assets, and other objects that require long-term
	 * identification. It uses cryptographically secure random number generation to ensure
	 * uniqueness across application instances.
	 *
	 * This class implements explicit conversion operators to prevent accidental type
	 * confusion and provides STL hash specialization for use in hash-based containers.
	 *
	 * @note While called "UUID", this implementation currently uses a 64-bit random
	 *       integer rather than a full 128-bit UUID. This provides sufficient uniqueness
	 *       for most applications while maintaining optimal performance.
	 *
	 * @warning The probability of collision is extremely low (approximately 1 in 2^64)
	 *          but not zero. Consider this in critical applications.
	 *
	 * @thread_safety Thread-safe for construction and conversion operations.
	 *
	 * @example
	 * @code
	 * // Generate random UUID
	 * UUID entityId;
	 *
	 * // Create from explicit value (e.g., for deserialization)
	 * UUID loadedEntity(0x123456789ABCDEF0);
	 *
	 * // Use in containers
	 * std::unordered_map<UUID, Entity> entities;
	 * entities[entityId] = myEntity;
	 *
	 * // Serialize for storage
	 * uint64_t serialized = static_cast<uint64_t>(entityId);
	 * @endcode
	 */
	class UUID
	{
	public:
		/**
		 * @brief Default constructor that generates a random UUID.
		 *
		 * Creates a new UUID with a randomly generated 64-bit value using a
		 * cryptographically secure random number generator. Each call to this
		 * constructor will produce a unique identifier with extremely high probability.
		 *
		 * @thread_safety Thread-safe. Multiple threads can safely create UUIDs
		 *                simultaneously without synchronization.
		 *
		 * @complexity O(1) - constant time operation
		 *
		 * @example
		 * @code
		 * UUID entityId;  // Generates random UUID
		 * UUID anotherId; // Generates different random UUID
		 * @endcode
		 */
		UUID();

		/**
		 * @brief Explicit constructor that creates a UUID from a specific value.
		 *
		 * Creates a UUID with the specified 64-bit value. This constructor is marked
		 * as explicit to prevent accidental implicit conversions. It's primarily used
		 * for deserialization or when you need to recreate a UUID from a known value.
		 *
		 * @param uuid The 64-bit value to use for this UUID
		 *
		 * @warning Using this constructor with non-random values may increase the
		 *          probability of collisions. Use with caution.
		 *
		 * @thread_safety Thread-safe. No shared state is accessed.
		 *
		 * @complexity O(1) - constant time operation
		 *
		 * @example
		 * @code
		 * // Create UUID from saved value
		 * uint64_t savedId = 0x123456789ABCDEF0;
		 * UUID restoredId(savedId);
		 *
		 * // Cannot do this (explicit constructor):
		 * // UUID badId = 12345;  // Compilation error
		 * @endcode
		 */
		explicit UUID(uint64_t uuid);

		/**
		 * @brief Copy constructor that creates a UUID from another UUID.
		 *
		 * Creates a new UUID that is an exact copy of the provided UUID. This is
		 * the standard copy constructor that allows UUIDs to be copied safely.
		 *
		 * @param other The UUID to copy from
		 *
		 * @thread_safety Thread-safe. No shared state is accessed.
		 *
		 * @complexity O(1) - constant time operation
		 *
		 * @example
		 * @code
		 * UUID original;
		 * UUID copy(original);  // copy has same value as original
		 * @endcode
		 */
		UUID(const UUID& other);

		/**
		 * @brief Explicit conversion operator to uint64_t.
		 *
		 * Provides explicit conversion to the underlying 64-bit integer value.
		 * This is marked as explicit to prevent accidental implicit conversions
		 * and to make serialization/deserialization operations more explicit.
		 *
		 * @return The underlying 64-bit integer value of this UUID
		 *
		 * @thread_safety Thread-safe. No shared state is accessed.
		 *
		 * @complexity O(1) - constant time operation
		 *
		 * @example
		 * @code
		 * UUID id;
		 * uint64_t value = static_cast<uint64_t>(id);  // Explicit conversion
		 *
		 * // Cannot do this (explicit operator):
		 * // uint64_t bad = id;  // Compilation error
		 * @endcode
		 */
		explicit operator uint64_t () const { return m_UUID; }

		/**
		 * @brief Equality comparison operator.
		 *
		 * Compares two UUID instances for equality by comparing their underlying
		 * 64-bit values. Two UUIDs are considered equal if they have the same
		 * internal value.
		 *
		 * @param other The UUID to compare with
		 * @return true if the UUIDs are equal, false otherwise
		 *
		 * @thread_safety Thread-safe. No shared state is accessed.
		 *
		 * @complexity O(1) - constant time operation
		 *
		 * @example
		 * @code
		 * UUID id1;
		 * UUID id2(id1);
		 * bool areEqual = (id1 == id2);  // true
		 * @endcode
		 */
		bool operator==(const UUID& other) const { return m_UUID == other.m_UUID; }

		/**
		 * @brief Inequality comparison operator.
		 *
		 * Compares two UUID instances for inequality by comparing their underlying
		 * 64-bit values. Two UUIDs are considered not equal if they have different
		 * internal values.
		 *
		 * @param other The UUID to compare with
		 * @return true if the UUIDs are not equal, false otherwise
		 *
		 * @thread_safety Thread-safe. No shared state is accessed.
		 *
		 * @complexity O(1) - constant time operation
		 *
		 * @example
		 * @code
		 * UUID id1;
		 * UUID id2;
		 * bool areNotEqual = (id1 != id2);  // likely true (randomly generated)
		 * @endcode
		 */
		bool operator!=(const UUID& other) const { return m_UUID != other.m_UUID; }

	private:
		friend struct std::hash<UUID>;  ///< Allow hash specialization access
		uint64_t m_UUID;               ///< The underlying 64-bit identifier value
	};

	/**
	 * @class UUID32
	 * @brief 32-bit Universally Unique Identifier for temporary objects and performance-critical contexts.
	 *
	 * The UUID32 class provides a compact 32-bit identifier suitable for temporary objects,
	 * UI elements, event tokens, and other contexts where memory usage and performance are
	 * critical. It uses the same secure random number generation as UUID but with a smaller
	 * value space.
	 *
	 * This class is ideal for scenarios where you need unique identifiers but don't require
	 * the full uniqueness guarantees of a 64-bit UUID. The smaller size makes it more
	 * cache-friendly and reduces memory usage in collections.
	 *
	 * @note The probability of collision is higher than UUID (approximately 1 in 2^32)
	 *       but still very low for most applications. Consider this when choosing between
	 *       UUID and UUID32.
	 *
	 * @thread_safety Thread-safe for construction and conversion operations.
	 *
	 * @example
	 * @code
	 * // Generate random UUID32
	 * UUID32 eventToken;
	 *
	 * // Create from explicit value
	 * UUID32 knownId(0x12345678);
	 *
	 * // Use in performance-critical containers
	 * std::unordered_map<UUID32, std::string> eventNames;
	 * eventNames[eventToken] = "ButtonClick";
	 * @endcode
	 */
	class UUID32
	{
	public:
		/**
		 * @brief Default constructor that generates a random UUID32.
		 *
		 * Creates a new UUID32 with a randomly generated 32-bit value using a
		 * cryptographically secure random number generator. Each call to this
		 * constructor will produce a unique identifier with high probability.
		 *
		 * @thread_safety Thread-safe. Multiple threads can safely create UUID32s
		 *                simultaneously without synchronization.
		 *
		 * @complexity O(1) - constant time operation
		 *
		 * @example
		 * @code
		 * UUID32 token;        // Generates random UUID32
		 * UUID32 anotherToken; // Generates different random UUID32
		 * @endcode
		 */
		UUID32();

		/**
		 * @brief Explicit constructor that creates a UUID32 from a specific value.
		 *
		 * Creates a UUID32 with the specified 32-bit value. This constructor is marked
		 * as explicit to prevent accidental implicit conversions. It's primarily used
		 * for deserialization or when you need to recreate a UUID32 from a known value.
		 *
		 * @param uuid The 32-bit value to use for this UUID32
		 *
		 * @warning Using this constructor with non-random values may increase the
		 *          probability of collisions. Use with caution.
		 *
		 * @thread_safety Thread-safe. No shared state is accessed.
		 *
		 * @complexity O(1) - constant time operation
		 *
		 * @example
		 * @code
		 * // Create UUID32 from saved value
		 * uint32_t savedToken = 0x12345678;
		 * UUID32 restoredToken(savedToken);
		 *
		 * // Cannot do this (explicit constructor):
		 * // UUID32 badToken = 12345;  // Compilation error
		 * @endcode
		 */
		explicit UUID32(uint32_t uuid);

		/**
		 * @brief Copy constructor that creates a UUID32 from another UUID32.
		 *
		 * Creates a new UUID32 that is an exact copy of the provided UUID32. This is
		 * the standard copy constructor that allows UUID32s to be copied safely.
		 *
		 * @param other The UUID32 to copy from
		 *
		 * @thread_safety Thread-safe. No shared state is accessed.
		 *
		 * @complexity O(1) - constant time operation
		 *
		 * @example
		 * @code
		 * UUID32 original;
		 * UUID32 copy(original);  // copy has same value as original
		 * @endcode
		 */
		UUID32(const UUID32& other);

		/**
		 * @brief Explicit conversion operator to uint32_t.
		 *
		 * Provides explicit conversion to the underlying 32-bit integer value.
		 * This is marked as explicit to prevent accidental implicit conversions
		 * and to make serialization/deserialization operations more explicit.
		 *
		 * @return The underlying 32-bit integer value of this UUID32
		 *
		 * @thread_safety Thread-safe. No shared state is accessed.
		 *
		 * @complexity O(1) - constant time operation
		 *
		 * @example
		 * @code
		 * UUID32 token;
		 * uint32_t value = static_cast<uint32_t>(token);  // Explicit conversion
		 *
		 * // Cannot do this (explicit operator):
		 * // uint32_t bad = token;  // Compilation error
		 * @endcode
		 */
		explicit operator uint32_t () const { return m_UUID; }

		/**
		 * @brief Equality comparison operator.
		 *
		 * Compares two UUID32 instances for equality by comparing their underlying
		 * 32-bit values. Two UUID32s are considered equal if they have the same
		 * internal value.
		 *
		 * @param other The UUID32 to compare with
		 * @return true if the UUID32s are equal, false otherwise
		 *
		 * @thread_safety Thread-safe. No shared state is accessed.
		 *
		 * @complexity O(1) - constant time operation
		 *
		 * @example
		 * @code
		 * UUID32 token1;
		 * UUID32 token2(token1);
		 * bool areEqual = (token1 == token2);  // true
		 * @endcode
		 */
		bool operator==(const UUID32& other) const { return m_UUID == other.m_UUID; }

		/**
		 * @brief Inequality comparison operator.
		 *
		 * Compares two UUID32 instances for inequality by comparing their underlying
		 * 32-bit values. Two UUID32s are considered not equal if they have different
		 * internal values.
		 *
		 * @param other The UUID32 to compare with
		 * @return true if the UUID32s are not equal, false otherwise
		 *
		 * @thread_safety Thread-safe. No shared state is accessed.
		 *
		 * @complexity O(1) - constant time operation
		 *
		 * @example
		 * @code
		 * UUID32 token1;
		 * UUID32 token2;
		 * bool areNotEqual = (token1 != token2);  // likely true (randomly generated)
		 * @endcode
		 */
		bool operator!=(const UUID32& other) const { return m_UUID != other.m_UUID; }


        bool operator==(const UUID & uuid) const;

    private:
		friend struct std::hash<UUID32>;  ///< Allow hash specialization access
		uint32_t m_UUID;                 ///< The underlying 32-bit identifier value
	};

}

/**
 * @namespace std
 * @brief STL hash specializations for UUID classes.
 *
 * This namespace contains template specializations of std::hash for all UUID
 * classes (UUID, UUID32, and UUID128), enabling them to be used as keys in STL
 * hash-based containers such as std::unordered_map and std::unordered_set.
 *
 * The hash implementations are optimized for performance and provide good
 * distribution characteristics for hash table usage.
 */
namespace std
{
	/**
	 * @class hash<SceneryEditorX::UUID>
	 * @brief Hash specialization for 64-bit UUID class.
	 *
	 * This specialization allows UUID objects to be used as keys in STL hash-based
	 * containers. Since UUIDs are already randomly distributed values, the hash
	 * implementation simply casts the UUID to std::size_t, which provides excellent
	 * hash distribution characteristics.
	 *
	 * @performance O(1) - constant time hashing
	 * @distribution Excellent - UUIDs are already random
	 * @collisions Extremely rare - inherits UUID collision probability
	 *
	 * @example
	 * @code
	 * std::unordered_map<UUID, std::string> entityNames;
	 * std::unordered_set<UUID> activeEntities;
	 *
	 * UUID entityId;
	 * entityNames[entityId] = "Player";
	 * activeEntities.insert(entityId);
	 * @endcode
	 */
	template <>
	struct hash<SceneryEditorX::UUID>
	{
		/**
		 * @brief Hash function operator for UUID.
		 *
		 * Computes a hash value for the given UUID. Since UUIDs are already
		 * randomly distributed, this function simply converts the UUID to
		 * std::size_t, which provides optimal hash distribution.
		 *
		 * @param uuid The UUID to hash
		 * @return Hash value suitable for hash table indexing
		 *
		 * @complexity O(1) - constant time operation
		 * @thread_safety Thread-safe. No shared state is accessed.
		 *
		 * @note The hash value is deterministic - the same UUID will always
		 *       produce the same hash value within a single program execution.
		 */
		std::size_t operator()(const SceneryEditorX::UUID &uuid) const
		{
			// uuid is already a randomly generated number, and is suitable as a hash key as-is.
			// this may change in future, in which case return hash<uint64_t>{}(uuid); might be more appropriate
			return static_cast<std::size_t>(static_cast<uint64_t>(uuid));
		}
	};

	/**
	 * @class hash<SceneryEditorX::UUID32>
	 * @brief Hash specialization for 32-bit UUID32 class.
	 *
	 * This specialization allows UUID32 objects to be used as keys in STL hash-based
	 * containers. The implementation delegates to the standard uint32_t hash function
	 * to ensure consistent behavior with other 32-bit integer types.
	 *
	 * @performance O(1) - constant time hashing
	 * @distribution Good - delegates to std::hash<uint32_t>
	 * @collisions Rare - inherits UUID32 collision probability
	 *
	 * @example
	 * @code
	 * std::unordered_map<UUID32, std::function<void()>> eventHandlers;
	 * std::unordered_set<UUID32> activeTokens;
	 *
	 * UUID32 eventToken;
	 * eventHandlers[eventToken] = []() { return; };
	 * activeTokens.insert(eventToken);
	 * @endcode
	 */
	template <>
	struct hash<SceneryEditorX::UUID32>
	{
		/**
		 * @brief Hash function operator for UUID32.
		 *
		 * Computes a hash value for the given UUID32 by delegating to the
		 * standard hash function for uint32_t. This ensures consistent
		 * behavior with other 32-bit integer types and provides good
		 * hash distribution.
		 *
		 * @param uuid The UUID32 to hash
		 * @return Hash value suitable for hash table indexing
		 *
		 * @complexity O(1) - constant time operation
		 * @thread_safety Thread-safe. No shared state is accessed.
		 *
		 * @note The hash value is deterministic - the same UUID32 will always
		 *       produce the same hash value within a single program execution.
		 */
		std::size_t operator()(const SceneryEditorX::UUID32 &uuid) const
		{
			return hash<uint32_t>()(static_cast<uint32_t>(uuid));
		}
	};

	/**
	 * @class hash<SceneryEditorX::UUID128>
	 * @brief Hash specialization for 128-bit UUID128 class.
	 *
	 * This specialization allows UUID128 objects to be used as keys in STL hash-based
	 * containers. The implementation combines the hash values of all four 32-bit
	 * components using a hash combination algorithm to produce a well-distributed
	 * hash value.
	 *
	 * @performance O(1) - constant time hashing
	 * @distribution Excellent - combines multiple random components
	 * @collisions Extremely rare - inherits UUID128 collision probability
	 *
	 * @example
	 * @code
	 * std::unordered_map<UUID128, AssetData> globalAssets;
	 * std::unordered_set<UUID128> uniqueIdentifiers;
	 *
	 * UUID128 assetId;
	 * globalAssets[assetId] = myAsset;
	 * uniqueIdentifiers.insert(assetId);
	 * @endcode
	 */
	template <>
	struct hash<SceneryEditorX::UUID128>
	{
		/**
		 * @brief Hash function operator for UUID128.
		 *
		 * Computes a hash value for the given UUID128 by combining the hash
		 * values of all four 32-bit components. This provides excellent
		 * distribution characteristics for hash table usage.
		 *
		 * @param uuid The UUID128 to hash
		 * @return Hash value suitable for hash table indexing
		 *
		 * @complexity O(1) - constant time operation
		 * @thread_safety Thread-safe. No shared state is accessed.
		 *
		 * @note The hash value is deterministic - the same UUID128 will always
		 *       produce the same hash value within a single program execution.
		 */
		std::size_t operator()(const SceneryEditorX::UUID128 &uuid) const
		{
			const auto& components = static_cast<std::array<uint32_t, 4>>(uuid);

			// Combine hash values of all components using a hash combination algorithm
			std::size_t result = hash<uint32_t>()(components[0]);
			result ^= hash<uint32_t>()(components[1]) + 0x9e3779b9 + (result << 6) + (result >> 2);
			result ^= hash<uint32_t>()(components[2]) + 0x9e3779b9 + (result << 6) + (result >> 2);
			result ^= hash<uint32_t>()(components[3]) + 0x9e3779b9 + (result << 6) + (result >> 2);

			return result;
		}
	};
}

/// -------------------------------------------------------
