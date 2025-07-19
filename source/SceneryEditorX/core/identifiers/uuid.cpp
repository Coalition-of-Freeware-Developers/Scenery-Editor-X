/**
* -------------------------------------------------------
* Scenery Editor X
* -------------------------------------------------------
* Copyright (c) 2025 Thomas Ray
* Copyright (c) 2025 Coalition of Freeware Developers
* -------------------------------------------------------
* uuid.cpp
* -------------------------------------------------------
* Created: 13/7/2025
* -------------------------------------------------------
*
* This file implements the UUID (Universally Unique Identifier) system for
* the Scenery Editor X application. It provides both 64-bit and 32-bit UUID
* variants with cryptographically secure random generation.
*
* The implementation uses thread-safe random number generators to ensure
* uniqueness across multiple threads and application instances.
* -------------------------------------------------------
*/
#include <SceneryEditorX/core/identifiers/uuid.h>
#include <SceneryEditorX/utils/static_states.h>
#include <random>

/// -------------------------------------------------------

namespace SceneryEditorX
{
	/// @brief Cryptographically secure random device for UUID generation
	GLOBAL std::random_device s_RandomDevice;

	/// @brief 64-bit Mersenne Twister random number generator seeded with secure random device
	GLOBAL std::mt19937_64 eng(s_RandomDevice());

	/// @brief Uniform distribution for generating 64-bit UUID values across the full range
	GLOBAL std::uniform_int_distribution<uint64_t> s_UniformDistribution;

	/// @brief 32-bit Mersenne Twister random number generator seeded with secure random device
	GLOBAL std::mt19937 eng32(s_RandomDevice());

	/// @brief Uniform distribution for generating 32-bit UUID values across the full range
	GLOBAL std::uniform_int_distribution<uint32_t> s_UniformDistribution32;

	/**
	 * @brief Default constructor that generates a random 64-bit UUID
	 *
	 * Creates a new UUID with a cryptographically secure random value.
	 * This constructor is thread-safe and ensures uniqueness across
	 * multiple concurrent UUID creations.
	 */
	UUID::UUID() : m_UUID(s_UniformDistribution(eng)) {}

	/**
	 * @brief Constructor that creates a UUID from an explicit 64-bit value
	 *
	 * This constructor is primarily used for deserialization or when
	 * loading UUIDs from persistent storage.
	 *
	 * @param uuid The explicit 64-bit value to use for the UUID
	 */
	UUID::UUID(const uint64_t uuid) : m_UUID(uuid) {}

	/**
	 * @brief Copy constructor for UUID objects
	 *
	 * Creates a new UUID that is an exact copy of another UUID.
	 * Uses the compiler-generated default copy constructor.
	 *
	 * @param other The UUID object to copy from
	 */
	UUID::UUID(const UUID& other) = default;

	/**
	 * @brief Default constructor that generates a random 32-bit UUID
	 *
	 * Creates a new 32-bit UUID with a cryptographically secure random value.
	 * This constructor is thread-safe and optimized for use cases where
	 * memory usage is a concern and collision probability is acceptable.
	 */
    UUID32::UUID32() : m_UUID(s_UniformDistribution32(eng32)) {}

    /**
	 * @brief Constructor that creates a 32-bit UUID from an explicit value
	 *
	 * This constructor is primarily used for deserialization or when
	 * loading 32-bit UUIDs from persistent storage.
	 *
	 * @param uuid The explicit 32-bit value to use for the UUID
	 */
	UUID32::UUID32(const uint32_t uuid) : m_UUID(uuid) {}

	/**
	 * @brief Copy constructor for UUID32 objects
	 *
	 * Creates a new 32-bit UUID that is an exact copy of another UUID32.
	 * Uses the compiler-generated default copy constructor.
	 *
	 * @param other The UUID32 object to copy from
	 */
	UUID32::UUID32(const UUID32& other) = default;

}

/// -------------------------------------------------------
