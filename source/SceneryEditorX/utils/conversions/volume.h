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
 * volume.h
 * -------------------------------------------------------
 * Created: 12/8/2025
 * -------------------------------------------------------
 */
#pragma once

// -----------------------------------------------------

namespace SceneryEditorX::Convert
{

    /// Volume conversions centered on cubic meters (m^3) - header-only
    namespace detail
    {
        constexpr float FT3_PER_M3 = 35.3146667215f;		/// 1 m^3 = 35.3146667 ft^3
        constexpr float L_PER_M3   = 1000.0f;				/// 1 m^3 = 1000 L
        constexpr float ML_PER_M3  = L_PER_M3 * 1000.0f;	/// 1 m^3 = 1,000,000 mL
        constexpr float L_PER_GAL_US = 3.785411784f;		/// 1 US gal = 3.785411784 L
        constexpr float L_PER_GAL_IMP = 4.54609f;			/// 1 Imp gal = 4.54609 L
    }

    /// Feet^3 <-> m^3
    inline float FromFt3(const float cubicFeet) { return cubicFeet / detail::FT3_PER_M3; }
    inline float ToFt3(const float cubicMeters) { return cubicMeters * detail::FT3_PER_M3; }

    /// Liters/mL <-> m^3
    inline float FromL(const float liters)      { return liters / detail::L_PER_M3; }
    inline float ToL(const float cubicMeters)   { return cubicMeters * detail::L_PER_M3; }
    inline float FrommL(const float milliliters){ return milliliters / detail::ML_PER_M3; }
    inline float TomL(const float cubicMeters)  { return cubicMeters * detail::ML_PER_M3; }

    /// US gallons <-> m^3
    inline float FromGalUS(const float gallons)
    {
        return (gallons * detail::L_PER_GAL_US) / detail::L_PER_M3;
    }

    inline float ToGalUS(const float cubicMeters)
    {
        const float liters = ToL(cubicMeters);
        return liters / detail::L_PER_GAL_US;
    }

    /// Imperial gallons <-> m^3
    inline float FromGalImp(const float gallons)
    {
        return (gallons * detail::L_PER_GAL_IMP) / detail::L_PER_M3;
    }

    inline float ToGalImp(const float cubicMeters)
    {
        const float liters = ToL(cubicMeters);
        return liters / detail::L_PER_GAL_IMP;
    }

}

// -----------------------------------------------------
