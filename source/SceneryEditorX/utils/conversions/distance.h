/**
* -------------------------------------------------------
* Scenery Editor X
* -------------------------------------------------------
* Copyright (c) 2025 Thomas Ray
* Copyright (c) 2025 Coalition of Freeware Developers
* -------------------------------------------------------
* distance.h
* -------------------------------------------------------
* Created: 12/8/2025
* -------------------------------------------------------
*/
#pragma once

/// -----------------------------------------------------

#ifdef __cplusplus
namespace SceneryEditorX { namespace Convert {
	// Distance conversions centered on meters as the base unit (header-only)

	namespace detail
	{
		constexpr float M_PER_MI = 1609.344f;
		constexpr float M_PER_NM = 1852.0f;
		constexpr float M_PER_KM = 1000.0f;
		constexpr float M_PER_YD = 0.9144f;
		constexpr float M_PER_FT = 0.3048f;
		constexpr float M_PER_IN = 0.0254f;
		constexpr float M_PER_CM = 0.01f;
		constexpr float M_PER_MM = 0.001f;
	}

	// Miles (statute) <-> meters
	inline float FromMi(const float miles) { return miles * detail::M_PER_MI; }
	inline float ToMi(const float meters) { return meters / detail::M_PER_MI; }

	// Kilometers <-> meters
	inline float FromKiloM(const float km) { return km * detail::M_PER_KM; }
	inline float ToKiloM(const float meters) { return meters / detail::M_PER_KM; }

	// Yards <-> meters
	inline float FromYd(const float yards) { return yards * detail::M_PER_YD; }
	inline float ToYd(const float meters) { return meters / detail::M_PER_YD; }

	// Feet <-> meters
	inline float FromFt(const float feet) { return feet * detail::M_PER_FT; }
	inline float ToFt(const float meters) { return meters / detail::M_PER_FT; }

	// Inches <-> meters
	inline float FromIn(const float inches) { return inches * detail::M_PER_IN; }
	inline float ToIn(const float meters) { return meters / detail::M_PER_IN; }

	// Nautical miles <-> meters
	inline float FromNm(const float nauticalMiles) { return nauticalMiles * detail::M_PER_NM; }
	inline float ToNm(const float meters) { return meters / detail::M_PER_NM; }

	// Centimeters / millimeters <-> meters
	inline float FromCm(const float centimeters) { return centimeters * detail::M_PER_CM; }
	inline float ToCm(const float meters) { return meters / detail::M_PER_CM; }
	inline float FromMm(const float millimeters) { return millimeters * detail::M_PER_MM; }
	inline float ToMm(const float meters) { return meters / detail::M_PER_MM; }

	// Helper: convert from meters into km (metric) or miles (imperial)
	inline float FromM(const float distanceMeters, const bool isMetric)
	{
		return isMetric ? ToKiloM(distanceMeters) : ToMi(distanceMeters);
	}

	// Helper: convert to meters from km (metric) or miles (imperial)
	inline float ToM(const float distanceValue, const bool isMetric)
	{
		return isMetric ? FromKiloM(distanceValue) : FromMi(distanceValue);
	}

}} // namespace SceneryEditorX::Convert

/// -----------------------------------------------------
#endif // __cplusplus
