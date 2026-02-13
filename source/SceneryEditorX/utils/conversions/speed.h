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
 * speed.h
 * -------------------------------------------------------
 * Created: 12/8/2025
 * -------------------------------------------------------
 */
#pragma once
// -----------------------------------------------------

namespace SceneryEditorX::Convert
{

	/// Speed conversions centered on meters per second (m/s) - header-only
	namespace detail
    {
		constexpr float MPS_PER_MPH = 0.44704f;      /// 1 mph = 0.44704 m/s
		constexpr float MPS_PER_KPH = 0.2777777778f; /// 1 km/h = 0.277777... m/s
		constexpr float MPS_PER_KT  = 0.514444f;     /// 1 knot = 0.514444 m/s
	}

	inline float FromMpH(const float mph) { return mph * detail::MPS_PER_MPH; }
	inline float ToMpH(const float mps)   { return mps / detail::MPS_PER_MPH; }
	inline float FromKpH(const float kph) { return kph * detail::MPS_PER_KPH; }
	inline float ToKpH(const float mps)   { return mps / detail::MPS_PER_KPH; }
	inline float FromKt(const float kn)   { return kn * detail::MPS_PER_KT; }
	inline float ToKt(const float mps)    { return mps / detail::MPS_PER_KT; }

	/// Helper: present/display based on metric flag (km/h or mph)
	inline float FromMpS(const float speedMps, const bool isMetric)
	{
		return isMetric ? ToKpH(speedMps) : ToMpH(speedMps);
	}

	inline float ToMpS(const float displayValue, const bool isMetric)
	{
		return isMetric ? FromKpH(displayValue) : FromMpH(displayValue);
	}

}

// -----------------------------------------------------
