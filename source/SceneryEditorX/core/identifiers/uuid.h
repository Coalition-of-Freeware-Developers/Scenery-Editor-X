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
* Scenery Editor X application. It implements both 64-bit and 32-bit UUID variants
* to optimize memory usage and performance for different use cases.
*
* Key Features:
* - Thread-safe UUID generation using cryptographically secure RNG
* - STL hash specializations for use in hash-based containers
* - Explicit conversion operators to prevent accidental type confusion
* - Minimal memory footprint with optimized storage
* - Support for deterministic construction from explicit values
*
* Usage Examples:
* @code
* // Generate random UUIDs
* UUID entityId;           // 64-bit UUID for persistent entities
* UUID32 eventToken;       // 32-bit UUID for temporary objects
*
* // Create from explicit values (e.g., for serialization)
* UUID loadedEntity(0x123456789ABCDEF0);
* UUID32 savedToken(0x12345678);
*
* // Use in containers
* std::unordered_map<UUID, Entity> entityMap;
* std::unordered_set<UUID32> activeTokens;
*
* // Convert for serialization
* uint64_t serializedId = static_cast<uint64_t>(entityId);
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

/// -------------------------------------------------------

namespace SceneryEditorX
{
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
	 *  entityId;
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

	private:
		friend struct std::hash<UUID32>;  ///< Allow hash specialization access
		uint32_t m_UUID;                 ///< The underlying 32-bit identifier value
	};

}

/**
 * @namespace std
 * @brief STL hash specializations for UUID classes.
 *
 * This namespace contains template specializations of std::hash for both UUID
 * and UUID32 classes, enabling them to be used as keys in STL hash-based containers
 * such as std::unordered_map and std::unordered_set.
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
}

/// -------------------------------------------------------
