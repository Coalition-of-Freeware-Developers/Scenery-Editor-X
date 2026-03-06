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
 * identifier.h
 * -------------------------------------------------------
 * Created: 13/7/2025
 * -------------------------------------------------------
 */
#pragma once
#include "hash.h"

// ---------------------------------------------

namespace SceneryEditorX
{
/**
     * @class Identifier
     * @brief A unique identifier class that encapsulates a hash and an optional debug name.
     *
     * The Identifier class provides a way to uniquely identify objects using a hash value.
     * It also optionally stores a debug name for easier identification during development.
     */
    class Identifier
	{
	public:
		constexpr Identifier() = default;

        /**
         * @brief Constructs an Identifier with a debug name.
         * @param name The debug name for the identifier.
         */
        explicit constexpr Identifier(const char* name) noexcept : m_Hash(Hash::CreateFNV1A(name)), m_Name(name) {}

        /**
         * @brief Constructs an Identifier with a hash value.
         * @param hash The hash value for the identifier.
         */
        explicit constexpr Identifier(Hash hash) noexcept : m_Hash(std::move(hash)) {}

		constexpr bool operator==(const Identifier& other) const noexcept { return m_Hash == other.m_Hash; }
		constexpr bool operator!=(const Identifier& other) const noexcept { return m_Hash != other.m_Hash; }

        /**
         * @brief Converts the Identifier to its underlying hash value.
         * @return The hash value of the identifier.
         */
        explicit constexpr operator Hash() const noexcept { return m_Hash; }
        [[nodiscard]] constexpr const char* GetDebugName() const { return m_Name; }

	private:
		friend Hash<Identifier>; // Allow the Hash struct to access the private m_Hash member for hashing.
		Hash m_Hash;
		const char* m_Name = nullptr;
	};

}

// ---------------------------------------------
