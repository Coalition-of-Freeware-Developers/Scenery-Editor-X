/**
* -------------------------------------------------------
* Scenery Editor X
* -------------------------------------------------------
* Copyright (c) 2025 Thomas Ray
* Copyright (c) 2025 Coalition of Freeware Developers
* -------------------------------------------------------
* force.h
* -------------------------------------------------------
* Created: 12/8/2025
* -------------------------------------------------------
*/
#pragma once

/// -----------------------------------------------------

namespace SceneryEditorX::Convert
{

    /// Force conversions centered on newtons (N) - header-only
    namespace detail
    {
        constexpr float N_PER_LBF = 4.4482216152605f; /// exact (pound-force)
        constexpr float N_PER_KGF = 9.80665f;         /// kilogram-force (standard gravity)
    }

    inline float FromLbf(const float lbf) { return lbf * detail::N_PER_LBF; }
    inline float ToLbf(const float n)     { return n / detail::N_PER_LBF; }

    inline float FromKgf(const float kgf) { return kgf * detail::N_PER_KGF; }
    inline float ToKgf(const float n)     { return n / detail::N_PER_KGF; }

}


/// -----------------------------------------------------
