/**
* -------------------------------------------------------
* Scenery Editor X
* -------------------------------------------------------
* Copyright (c) 2025 Thomas Ray
* Copyright (c) 2025 Coalition of Freeware Developers
* -------------------------------------------------------
* speed.h
* -------------------------------------------------------
* Created: 12/8/2025
* -------------------------------------------------------
*/
#pragma once
/// -----------------------------------------------------

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

/// -----------------------------------------------------
