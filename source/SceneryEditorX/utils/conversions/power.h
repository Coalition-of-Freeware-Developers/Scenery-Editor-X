/**
* -------------------------------------------------------
* Scenery Editor X
* -------------------------------------------------------
* power.h
* -------------------------------------------------------
*/
#pragma once

#ifdef __cplusplus
namespace SceneryEditorX { namespace Convert {

    // Power conversions centered on watts (W) - header-only
    namespace detail {
        constexpr float W_PER_KW = 1000.0f;
        constexpr float W_PER_MW = 1'000'000.0f;
        constexpr float W_PER_HP_MECH = 745.69987158227022f; // mechanical hp
        constexpr float W_PER_HP_METRIC = 735.49875f;        // metric hp (PS)
    }

    inline float FromKW(const float kw)         { return kw * detail::W_PER_KW; }
    inline float ToKW(const float w)            { return w / detail::W_PER_KW; }
    inline float FromMW(const float mw)         { return mw * detail::W_PER_MW; }
    inline float ToMW(const float w)            { return w / detail::W_PER_MW; }

    inline float FromHPMech(const float hp)     { return hp * detail::W_PER_HP_MECH; }
    inline float ToHPMech(const float w)        { return w / detail::W_PER_HP_MECH; }
    inline float FromHPMetric(const float hp)   { return hp * detail::W_PER_HP_METRIC; }
    inline float ToHPMetric(const float w)      { return w / detail::W_PER_HP_METRIC; }

}} // namespace SceneryEditorX::Convert
#endif // __cplusplus

/// -----------------------------------------------------
