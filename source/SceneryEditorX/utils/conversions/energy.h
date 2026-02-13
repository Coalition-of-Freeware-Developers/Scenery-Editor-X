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
 * energy.h
 * -------------------------------------------------------
 * Created: 12/8/2025
 * -------------------------------------------------------
 */
#pragma once

// -----------------------------------------------------

namespace SceneryEditorX::Convert
{

	/// Energy conversions centered on joules (J) - header-only
	namespace detail
    {
		constexpr float J_PER_WH   = 3600.0f;			/// 1 Wh = 3600 J
		constexpr float J_PER_KWH  = 3'600'000.0f;		/// 1 kWh = 3.6e6 J
		constexpr float J_PER_BTU  = 1055.05585262f;	/// 1 BTU (IT) = 1055.05585262 J
	}

	inline float FromKJ(const float kilojoules) { return kilojoules * 1000.0f; }
	inline float ToKJ(const float joules)       { return joules / 1000.0f; }

	inline float FromWh(const float wh)         { return wh * detail::J_PER_WH; }
	inline float ToWh(const float joules)       { return joules / detail::J_PER_WH; }

	inline float FromKWh(const float kwh)       { return kwh * detail::J_PER_KWH; }
	inline float ToKWh(const float joules)      { return joules / detail::J_PER_KWH; }

	inline float FromBTU(const float btu)       { return btu * detail::J_PER_BTU; }
	inline float ToBTU(const float joules)      { return joules / detail::J_PER_BTU; }

}

// -----------------------------------------------------
