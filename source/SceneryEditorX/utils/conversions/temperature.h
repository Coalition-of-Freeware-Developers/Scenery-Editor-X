/**
* -------------------------------------------------------
* Scenery Editor X
* -------------------------------------------------------
* Copyright (c) 2025 Thomas Ray
* Copyright (c) 2025 Coalition of Freeware Developers
* -------------------------------------------------------
* temperature.h
* -------------------------------------------------------
* Created: 12/8/2025
* -------------------------------------------------------
*/
#pragma once
#pragma once
#ifdef __cplusplus

/// -----------------------------------------------------

namespace SceneryEditorX::Convert
{

	/// Temperature conversions among Celsius, Fahrenheit, Kelvin (header-only)
	inline float CToF(const float c) { return (c * 9.0f / 5.0f) + 32.0f; }
	inline float FToC(const float f) { return (f - 32.0f) * 5.0f / 9.0f; }
	inline float CToK(const float c) { return c + 273.15f; }
	inline float KToC(const float k) { return k - 273.15f; }
	inline float FToK(const float f) { return CToK(FToC(f)); }
	inline float KToF(const float k) { return CToF(KToC(k)); }

}
#endif // __cplusplus

/// -----------------------------------------------------
