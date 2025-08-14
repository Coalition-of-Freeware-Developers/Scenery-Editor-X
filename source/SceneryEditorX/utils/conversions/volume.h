/**
* -------------------------------------------------------
* Scenery Editor X
* -------------------------------------------------------
* Copyright (c) 2025 Thomas Ray
* Copyright (c) 2025 Coalition of Freeware Developers
* -------------------------------------------------------
* volume.h
* -------------------------------------------------------
* Created: 12/8/2025
* -------------------------------------------------------
*/
#pragma once

/// -----------------------------------------------------

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

/// -----------------------------------------------------
