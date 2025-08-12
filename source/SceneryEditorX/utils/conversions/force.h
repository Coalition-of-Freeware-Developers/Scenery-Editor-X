/**
* -------------------------------------------------------
* Scenery Editor X
* -------------------------------------------------------
* force.h
* -------------------------------------------------------
*/
#pragma once

#ifdef __cplusplus
namespace SceneryEditorX { namespace Convert {

    // Force conversions centered on newtons (N) - header-only
    namespace detail {
        constexpr float N_PER_LBF = 4.4482216152605f; // exact (pound-force)
        constexpr float N_PER_KGF = 9.80665f;         // kilogram-force (standard gravity)
    }

    inline float FromLbf(const float lbf) { return lbf * detail::N_PER_LBF; }
    inline float ToLbf(const float n)     { return n / detail::N_PER_LBF; }

    inline float FromKgf(const float kgf) { return kgf * detail::N_PER_KGF; }
    inline float ToKgf(const float n)     { return n / detail::N_PER_KGF; }

}} // namespace SceneryEditorX::Convert
#endif // __cplusplus

/// -----------------------------------------------------
