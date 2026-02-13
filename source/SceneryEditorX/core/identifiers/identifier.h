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

// ---------------------------------------------

namespace SceneryEditorX
{
    class Identifier
	{
	public:
		constexpr Identifier() = default;

        //explicit constexpr Identifier(const std::string_view name) noexcept : hash(Hash::GenerateFNVHash(name.data())), dbgName(name) {}
        explicit constexpr Identifier(const uint32_t hash) noexcept : hash(hash) {}

		constexpr bool operator==(const Identifier& other) const noexcept { return hash == other.hash; }
		constexpr bool operator!=(const Identifier& other) const noexcept { return hash != other.hash; }

        explicit constexpr operator uint32_t() const noexcept { return hash; }
        [[nodiscard]] constexpr std::string_view GetDBGName() const { return dbgName; }

	private:
		friend struct std::hash<Identifier>;
		uint32_t hash = 0;
		std::string_view dbgName;
	};

}

// ---------------------------------------------
