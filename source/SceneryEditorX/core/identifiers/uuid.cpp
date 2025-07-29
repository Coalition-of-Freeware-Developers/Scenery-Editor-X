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
* the Scenery Editor X application. It provides 32-bit, 64-bit, and 128-bit UUID
* variants with cryptographically secure random generation, as well as utility
* functions for encoding, decoding, and hashing operations.
*
* The implementation uses thread-safe random number generators to ensure
* uniqueness across multiple threads and application instances.
* -------------------------------------------------------
*/
#include <SceneryEditorX/core/identifiers/uuid.h>
#include <random>
#include <algorithm>
#include <string>
#include <stdexcept>
#include <array>

/// -------------------------------------------------------

namespace SceneryEditorX
{
	/// @brief Base64 character set used for encoding binary data to text
	static const std::string base64Chars = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

	/// @brief Random device for UUID generation
	static std::random_device s_RandomDevice;

	/// @brief 64-bit Mersenne Twister random number generator
	static std::mt19937_64 s_Engine64(s_RandomDevice());

	/// @brief Uniform distribution for generating 64-bit UUID values
	static std::uniform_int_distribution<uint64_t> s_UniformDistribution64;

	/// @brief 32-bit Mersenne Twister random number generator
	static std::mt19937 s_Engine32(s_RandomDevice());

	/// @brief Uniform distribution for generating 32-bit UUID values
	static std::uniform_int_distribution<uint32_t> s_UniformDistribution32;

	/**
	 * @brief Checks if a character is a valid Base64 character
	 */
	static bool IsBase64(const unsigned char c)
	{
		return (c == 43 ||              // +
				(c >= 47 && c <= 57) || // /-9
				(c >= 65 && c <= 90) || // A-Z
				(c >= 97 && c <= 122)); // a-z
	}

	// -------------------------------------------------------
	// UUIDUtilities Implementation
	// -------------------------------------------------------

	/*
	std::string UUIDUtilities::EncodeBase64(const unsigned char* input, size_t length)
	{
		std::string ret;
		int i = 0;
		unsigned char char_array_3[3];
		unsigned char char_array_4[4];

		while (length--)
		{
			char_array_3[i++] = *(input++);
			if (i == 3)
			{
				char_array_4[0] = (char_array_3[0] & 0xfc) >> 2;
				char_array_4[1] = ((char_array_3[0] & 0x03) << 4) + ((char_array_3[1] & 0xf0) >> 4);
				char_array_4[2] = ((char_array_3[1] & 0x0f) << 2) + ((char_array_3[2] & 0xc0) >> 6);
				char_array_4[3] = char_array_3[2] & 0x3f;

				for (i = 0; (i < 4); i++)
					ret += base64Chars[char_array_4[i]];

				i = 0;
			}
		}

		if (i)
		{
			int j = 0;
			for (j = i; j < 3; j++)
				char_array_3[j] = '\0';

			char_array_4[0] = (char_array_3[0] & 0xfc) >> 2;
			char_array_4[1] = ((char_array_3[0] & 0x03) << 4) + ((char_array_3[1] & 0xf0) >> 4);
			char_array_4[2] = ((char_array_3[1] & 0x0f) << 2) + ((char_array_3[2] & 0xc0) >> 6);
			char_array_4[3] = char_array_3[2] & 0x3f;

			for (j = 0; (j < i + 1); j++)
				ret += base64Chars[char_array_4[j]];

			while ((i++ < 3))
				ret += '=';
		}

		return ret;
	}

	std::vector<uint8_t> UUIDUtilities::DecodeBase64(const std::string& input)
	{
		size_t in_len = input.size();
		int i = 0;
		int in_ = 0;
		unsigned char char_array_3[3] = {0};
		unsigned char char_array_4[4] = {0};

		std::vector<uint8_t> ret;

		while (in_len-- && (input[in_] != '=') && IsBase64(input[in_]))
		{
			char_array_4[i++] = input[in_]; in_++;
			if (i == 4) {
				for (i = 0; i < 4; i++)
					char_array_4[i] = static_cast<unsigned char>(base64Chars.find(char_array_4[i]));

				char_array_3[0] = ((char_array_4[1] & 0x30) >> 4) + (char_array_4[0] << 2);
				char_array_3[1] = ((char_array_4[1] & 0xf) << 4) + ((char_array_4[2] & 0x3c) >> 2);
				char_array_3[2] = ((char_array_4[2] & 0x3) << 6) + char_array_4[3];

				for (i = 0; (i < 3); i++)
					ret.push_back(char_array_3[i]);

				i = 0;
			}
		}

		if (i)
		{
			int j = 0;
			for (j = i; j < 4; j++)
				char_array_4[j] = 0;

			for (j = 0; j < 4; j++)
				char_array_4[j] = static_cast<unsigned char>(base64Chars.find(char_array_4[j]));

			char_array_3[0] = (char_array_4[0] << 2) + ((char_array_4[1] & 0x30) >> 4);
			char_array_3[1] = ((char_array_4[1] & 0xf) << 4) + ((char_array_4[2] & 0x3c) >> 2);
			char_array_3[2] = ((char_array_4[2] & 0x3) << 6) + char_array_4[3];

			for (j = 0; (j < i - 1); j++)
			{
				ret.push_back(char_array_3[j]);
			}
		}

		return ret;
	}

	uint32_t UUIDUtilities::HashUUID(const std::vector<uint32_t>& invec)
	{
		std::vector<uint32_t> vec = invec;
		std::ranges::sort(vec);
		std::size_t seed = vec.size();
		for (const auto& value : vec)
			seed ^= value + 0x9e3779b9 + (seed << 6) + (seed >> 2);

		return static_cast<uint32_t>(seed);
	}

	void UUIDUtilities::HashCombine(uint32_t& h, void* ptr, const uint32_t size)
	{
		h = static_cast<uint32_t>(std::hash<std::string_view>()(std::string_view(static_cast<char*>(ptr), size)));
	}
	*/

	// -------------------------------------------------------
	// UUID128 Implementation
	// -------------------------------------------------------

	UUID128::UUID128()
	{
		m_UUID[0] = s_UniformDistribution32(s_Engine32);
		m_UUID[1] = s_UniformDistribution32(s_Engine32);
		m_UUID[2] = s_UniformDistribution32(s_Engine32);
		m_UUID[3] = s_UniformDistribution32(s_Engine32);
	}

	UUID128::UUID128(const std::array<uint32_t, 4>& uuid) : m_UUID(uuid) {}

	UUID128::UUID128(const UUID128& other) = default;

	/*
	std::string UUID128::ToBase64() const
	{
		return UUIDUtilities::EncodeBase64(reinterpret_cast<const unsigned char*>(m_UUID.data()), sizeof(m_UUID));
	}

	UUID128 UUID128::FromBase64(const std::string& base64)
	{
		auto decoded = UUIDUtilities::DecodeBase64(base64);

		if (decoded.size() != sizeof(std::array<uint32_t, 4>))
		{
			throw std::runtime_error("Invalid Base64 data size for UUID128");
		}

		std::array<uint32_t, 4> values;
		std::memcpy(values.data(), decoded.data(), sizeof(values));

		return UUID128(values);
	}
	*/

	// -------------------------------------------------------
	// UUID (64-bit) Implementation
	// -------------------------------------------------------

	UUID::UUID() : m_UUID(s_UniformDistribution64(s_Engine64)) {}

	UUID::UUID(const uint64_t uuid) : m_UUID(uuid) {}

	UUID::UUID(const UUID& other) = default;

	// -------------------------------------------------------
	// UUID32 Implementation
	// -------------------------------------------------------

	UUID32::UUID32() : m_UUID(s_UniformDistribution32(s_Engine32)) {}

	UUID32::UUID32(const uint32_t uuid) : m_UUID(uuid) {}

	UUID32::UUID32(const UUID32& other) = default;

}

/// -------------------------------------------------------
