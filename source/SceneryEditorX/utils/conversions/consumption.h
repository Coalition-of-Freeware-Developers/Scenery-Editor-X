/**
* -------------------------------------------------------
* Scenery Editor X
* -------------------------------------------------------
* Copyright (c) 2025 Thomas Ray
* Copyright (c) 2025 Coalition of Freeware Developers
* -------------------------------------------------------
* consumption.h
* -------------------------------------------------------
* Created: 12/8/2025
* -------------------------------------------------------
*/
#pragma once

/// --------------------------------------------

namespace SceneryEditorX::Convert
{
    /// Fuel consumption/economy conversions - header-only
    /// We'll treat L/100km (consumption) and mpgUS/mpgImp/kmL (economy)
    /// Provide both directions centered around an internal base of L/100km.

    /// Use unique namespace to avoid colliding with volume.h's detail constants
    namespace econ_detail
    {
        constexpr float KM_PER_MI = 1.609344f;      /// exact relationship
    }

    /// Normalize economy to consumption (L/100km)
    inline float LPer100Km_From_MpgUS(const float mpg)
    {
        /// L/100km = 100 * L / (km)
        /// miles/gal -> km/L, then invert & scale
        const float km_per_l = (mpg * econ_detail::KM_PER_MI) / detail::L_PER_GAL_US;
        return km_per_l > 0.0f ? (100.0f / km_per_l) : 0.0f;
    }

    inline float LPer100Km_From_MpgImp(const float mpg)
    {
        const float km_per_l = (mpg * econ_detail::KM_PER_MI) / detail::L_PER_GAL_IMP;
        return km_per_l > 0.0f ? (100.0f / km_per_l) : 0.0f;
    }

    inline float LPer100Km_From_KmPerL(const float km_per_l)
    {
        return km_per_l > 0.0f ? (100.0f / km_per_l) : 0.0f;
    }

    /// From L/100km to economy units
    inline float MpgUS_From_LPer100Km(const float l_per_100km)
    {
        if (l_per_100km <= 0.0f)
			return 0.0f;

        const float km_per_l = 100.0f / l_per_100km;
        const float miles_per_l = km_per_l / econ_detail::KM_PER_MI;
        return miles_per_l * (detail::L_PER_GAL_US);
    }

    inline float MpgImp_From_LPer100Km(const float l_per_100km)
    {
        if (l_per_100km <= 0.0f)
			return 0.0f;

        const float km_per_l = 100.0f / l_per_100km;
        const float miles_per_l = km_per_l / econ_detail::KM_PER_MI;
        return miles_per_l * (detail::L_PER_GAL_IMP);
    }

    inline float KmPerL_From_LPer100Km(const float l_per_100km)
    {
        return l_per_100km > 0.0f ? (100.0f / l_per_100km) : 0.0f;
    }

    /// --- Direct cross-converters (economy <-> economy) ---
    /// Note: Use guards for non-positive inputs to avoid nonsensical/negative results.

    /// mpg(US) <-> mpg(Imp)
    inline float MpgImp_From_MpgUS(const float mpg_us)
    {
        if (mpg_us <= 0.0f)
			return 0.0f;

        return mpg_us * (detail::L_PER_GAL_IMP / detail::L_PER_GAL_US);
    }
    inline float MpgUS_From_MpgImp(const float mpg_imp)
    {
        if (mpg_imp <= 0.0f)
			return 0.0f;

        return mpg_imp * (detail::L_PER_GAL_US / detail::L_PER_GAL_IMP);
    }

    /// km/L <-> mpg(US)
    inline float KmPerL_From_MpgUS(const float mpg_us)
    {
        if (mpg_us <= 0.0f)
			return 0.0f;

        return (mpg_us * econ_detail::KM_PER_MI) / detail::L_PER_GAL_US;
    }

    inline float MpgUS_From_KmPerL(const float km_per_l)
    {
        if (km_per_l <= 0.0f)
			return 0.0f;

        return (km_per_l / econ_detail::KM_PER_MI) * detail::L_PER_GAL_US;
    }

    /// km/L <-> mpg(Imp)
    inline float KmPerL_From_MpgImp(const float mpg_imp)
    {
        if (mpg_imp <= 0.0f)
			return 0.0f;

        return (mpg_imp * econ_detail::KM_PER_MI) / detail::L_PER_GAL_IMP;
    }

    inline float MpgImp_From_KmPerL(const float km_per_l)
    {
        if (km_per_l <= 0.0f)
			return 0.0f;

        return (km_per_l / econ_detail::KM_PER_MI) * detail::L_PER_GAL_IMP;
    }

}


/// -----------------------------------------------------
