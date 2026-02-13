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
 * area.h
 * -------------------------------------------------------
 * Created: 12/8/2025
 * -------------------------------------------------------
 */
#pragma once

// -----------------------------------------------------

namespace SceneryEditorX::Convert
{

	/// Area conversions centered on square meters (m^2) - header-only
	namespace detail
    {
		constexpr float M2_PER_FT2 = 0.09290304f;    /// (0.3048 m)^2
		constexpr float M2_PER_IN2 = 0.00064516f;    /// (0.0254 m)^2
		constexpr float M2_PER_YD2 = 0.83612736f;    /// (0.9144 m)^2
		constexpr float M2_PER_ACRE = 4046.8564224f; /// 1 acre in m^2
		constexpr float M2_PER_HA = 10000.0f;        /// 1 hectare in m^2
	}

	inline float FromFt2(const float squareFeet)		{ return squareFeet * detail::M2_PER_FT2; }
	inline float ToFt2(const float squareMeters)		{ return squareMeters / detail::M2_PER_FT2; }
	inline float FromIn2(const float squareInches)		{ return squareInches * detail::M2_PER_IN2; }
	inline float ToIn2(const float squareMeters)		{ return squareMeters / detail::M2_PER_IN2; }
	inline float FromYd2(const float squareYards)		{ return squareYards * detail::M2_PER_YD2; }
	inline float ToYd2(const float squareMeters)		{ return squareMeters / detail::M2_PER_YD2; }
	inline float FromAcre(const float acres)			{ return acres * detail::M2_PER_ACRE; }
	inline float ToAcre(const float squareMeters)		{ return squareMeters / detail::M2_PER_ACRE; }
	inline float FromHectare(const float hectares)		{ return hectares * detail::M2_PER_HA; }
	inline float ToHectare(const float squareMeters)	{ return squareMeters / detail::M2_PER_HA; }

}

// -----------------------------------------------------
