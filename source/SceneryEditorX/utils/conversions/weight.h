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
 * weight.h
 * -------------------------------------------------------
 * Created: 12/8/2025
 * -------------------------------------------------------
 */
#pragma once

// -----------------------------------------------------

namespace SceneryEditorX::Convert
{
	/// Mass conversions centered on kilograms (kg) - header-only
	namespace detail
    {
		constexpr float KG_PER_LB    = 0.45359237f;			/// exact
		constexpr float KG_PER_OZ    = KG_PER_LB / 16.0f;
		constexpr float KG_PER_TON_US= 907.18474f;			/// short ton (2000 lb)
		constexpr float KG_PER_TON_UK= 1016.0469088f;		/// long ton (2240 lb)
		constexpr float KG_PER_TONNE = 1000.0f;				/// metric tonne
	}

	inline float FromLb(const float lb)   { return lb * detail::KG_PER_LB; }
	inline float ToLb(const float kg)     { return kg / detail::KG_PER_LB; }
	inline float FromOz(const float oz)   { return oz * detail::KG_PER_OZ; }
	inline float ToOz(const float kg)     { return kg / detail::KG_PER_OZ; }
	inline float FromTUS(const float t)   { return t * detail::KG_PER_TON_US; }
	inline float ToTUS(const float kg)    { return kg / detail::KG_PER_TON_US; }
	inline float FromTUK(const float t)   { return t * detail::KG_PER_TON_UK; }
	inline float ToTUK(const float kg)    { return kg / detail::KG_PER_TON_UK; }
	inline float FromTonne(const float t) { return t * detail::KG_PER_TONNE; }
	inline float ToTonne(const float kg)  { return kg / detail::KG_PER_TONNE; }

}

// -----------------------------------------------------
