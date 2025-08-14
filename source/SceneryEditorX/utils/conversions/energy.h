/**
* -------------------------------------------------------
* Scenery Editor X
* -------------------------------------------------------
* Copyright (c) 2025 Thomas Ray
* Copyright (c) 2025 Coalition of Freeware Developers
* -------------------------------------------------------
* energy.h
* -------------------------------------------------------
* Created: 12/8/2025
* -------------------------------------------------------
*/
#pragma once

/// -----------------------------------------------------

namespace SceneryEditorX::Convert
{

	/// Energy conversions centered on joules (J) - header-only
	namespace detail
    {
		constexpr float J_PER_WH   = 3600.0f;			/// 1 Wh = 3600 J
		constexpr float J_PER_KWH  = 3'600'000.0f;		/// 1 kWh = 3.6e6 J
		constexpr float J_PER_BTU  = 1055.05585262f;	/// 1 BTU (IT) = 1055.05585262 J
	}

	inline float FromKJ(const float kilojoules) { return kilojoules * 1000.0f; }
	inline float ToKJ(const float joules)       { return joules / 1000.0f; }

	inline float FromWh(const float wh)         { return wh * detail::J_PER_WH; }
	inline float ToWh(const float joules)       { return joules / detail::J_PER_WH; }

	inline float FromKWh(const float kwh)       { return kwh * detail::J_PER_KWH; }
	inline float ToKWh(const float joules)      { return joules / detail::J_PER_KWH; }

	inline float FromBTU(const float btu)       { return btu * detail::J_PER_BTU; }
	inline float ToBTU(const float joules)      { return joules / detail::J_PER_BTU; }

}

/// -----------------------------------------------------
