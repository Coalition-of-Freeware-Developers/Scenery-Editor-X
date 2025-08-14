/**
* -------------------------------------------------------
* Scenery Editor X
* -------------------------------------------------------
* Copyright (c) 2025 Thomas Ray
* Copyright (c) 2025 Coalition of Freeware Developers
* -------------------------------------------------------
* pressure.h
* -------------------------------------------------------
* Created: 12/8/2025
* -------------------------------------------------------
*/
#pragma once

/// -----------------------------------------------------

namespace SceneryEditorX::Convert
{

	enum class PressureUnit : uint8_t
	{
		None = 0,
		KPa,
		Bar,
		PSI,
		InHg,
		KgfpCm2
	};

	/// Base conversions centered on kilopascals (kPa)
	float ToKPa(float value, PressureUnit inUnit);

    /// Header-only implementation (base = kPa)
    namespace detail
    {
		constexpr float KPA_PER_Pa   = 0.001f;
		constexpr float KPA_PER_kPa  = 1.0f;
		constexpr float KPA_PER_MPa  = 1000.0f;
		constexpr float KPA_PER_Bar  = 100.0f;			/// 1 bar = 100 kPa
		constexpr float KPA_PER_PSI  = 6.89475729f;		/// 1 psi = 6.894757 kPa
		constexpr float KPA_PER_InHg = 3.386389f;		/// 1 inHg ≈ 3.386389 kPa
		constexpr float KPA_PER_KgfpCm2 = 98.0665f;		/// 1 kgf/cm² ≈ 98.0665 kPa

		inline float toKPaFactor(PressureUnit u)
		{
			switch (u)
			{
			case PressureUnit::None: return 1.0f; /// Treat None as identity (value in kPa)
			case PressureUnit::KPa: return KPA_PER_kPa;
			case PressureUnit::Bar: return KPA_PER_Bar;
			case PressureUnit::PSI: return KPA_PER_PSI;
			case PressureUnit::InHg:return KPA_PER_InHg;
			case PressureUnit::KgfpCm2: return KPA_PER_KgfpCm2;
			default: return 1.0f;
			}
		}
    }

	inline float ToKPa(const float value, const PressureUnit unit)
	{
		return value * detail::toKPaFactor(unit);
	}

	inline float FromKPa(const float kpa, const PressureUnit unit)
	{
		return kpa / detail::toKPaFactor(unit);
	}

	/// convenience helpers
	inline float KPaToPSI(const float kpa) { return FromKPa(kpa, PressureUnit::PSI); }
	inline float PSIToKPa(const float psi) { return ToKPa(psi, PressureUnit::PSI); }
	inline float KPaToBar(const float kpa) { return FromKPa(kpa, PressureUnit::Bar); }
	inline float BarToKPa(const float bar) { return ToKPa(bar, PressureUnit::Bar); }
	inline float KPaToInHg(const float kpa) { return FromKPa(kpa, PressureUnit::InHg); }
	inline float InHgToKPa(const float inHg) { return ToKPa(inHg, PressureUnit::InHg); }
	inline float KPaToKgfpCm2(const float kpa) { return FromKPa(kpa, PressureUnit::KgfpCm2); }
	inline float KgfpCm2ToKPa(const float kgfpcm2) { return ToKPa(kgfpcm2, PressureUnit::KgfpCm2); }

}

/// -----------------------------------------------------
