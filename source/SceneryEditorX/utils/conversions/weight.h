/**
* -------------------------------------------------------
* Scenery Editor X
* -------------------------------------------------------
* Copyright (c) 2025 Thomas Ray
* Copyright (c) 2025 Coalition of Freeware Developers
* -------------------------------------------------------
* weight.h
* -------------------------------------------------------
* Created: 12/8/2025
* -------------------------------------------------------
*/
#pragma once

/// -----------------------------------------------------

#ifdef __cplusplus
namespace SceneryEditorX { namespace Convert {

	// Mass conversions centered on kilograms (kg) - header-only
	namespace detail {
		constexpr float KG_PER_LB    = 0.45359237f;   // exact
		constexpr float KG_PER_OZ    = KG_PER_LB / 16.0f;
		constexpr float KG_PER_TON_US= 907.18474f;    // short ton (2000 lb)
		constexpr float KG_PER_TON_UK= 1016.0469088f; // long ton (2240 lb)
		constexpr float KG_PER_TONNE = 1000.0f;       // metric tonne
	}

	inline float FromLb(const float lb)   { return lb * detail::KG_PER_LB; }
	inline float ToLb(const float kg)     { return kg / detail::KG_PER_LB; }
	inline float FromOz(const float oz)   { return oz * detail::KG_PER_OZ; }
	inline float ToOz(const float kg)     { return kg / detail::KG_PER_OZ; }
	inline float FromTUS(const float t)   { return t * detail::KG_PER_TON_US; }
	inline float ToTUS(const float kg)    { return kg / detail::KG_PER_TON_US; }
	inline float FromTUK(const float t)   { return t * detail::KG_PER_TON_UK; }
	inline float ToTUK(const float kg)    { return kg / detail::KG_PER_TON_UK; }
	inline float FromTonne(const float t) { return t * detail::KG_PER_TONNE; }
	inline float ToTonne(const float kg)  { return kg / detail::KG_PER_TONNE; }

}} // namespace SceneryEditorX::Convert
#endif // __cplusplus

/// -----------------------------------------------------
