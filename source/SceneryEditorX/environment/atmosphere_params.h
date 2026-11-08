/**
 * -------------------------------------------------------
 * Scenery Editor X
 * -------------------------------------------------------
 * Copyright (c) 2026 Thomas Ray
 * Copyright (c) 2026 Coalition of Freeware Developers
 * -------------------------------------------------------
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 * -------------------------------------------------------
 * atmosphere_params.h
 * -------------------------------------------------------
 * Created: 20/10/2026
 * -------------------------------------------------------
 */
#pragma once
#include <array>

// -------------------------------------------------------

namespace SceneryEditorX::Environment
{
	using Rgb = std::array<double, 3>;

	/**
	 * The atmosphere's medium (plan REQ-008, TASK-046), Bruneton / Hillaire parameterization plus X-Plane 12.4's surface
	 * haze: Rayleigh and Mie layers falling off exponentially with altitude, an ozone tent, and the visibility's layer
	 * from the ground up to hazeTopM that eases to clear air over hazeBandM: extra Mie, and grey fog for short
	 * visibilities. Units: meters and 1/m.
	 * The defaults are X-Plane 12.4.4's clear atmosphere as the reference probe dumped it (atmo/*, scattering/*), which
	 * resources/environment/xp12_environment.json repeats. The GPU (atmosphere_common.slang) samples the same medium
	 * from the environment buffer.
	 */
	struct AtmosphereParams
	{
		Rgb rayleighScattering = { 5.596e-6, 11.75e-6, 27.68e-6 };	// atmo/rayleigh_*
		double rayleighScaleHeight = 8000.0;
		double mieScattering = 3.996e-6;		// Green; Hillaire 2020 (X-Plane's scattering/mie_rat 1 scales a base it doesn't show)
		double mieExtinction = 4.44e-6;			// Scattering / scattering/mie_albedo 0.9
		Rgb mieSpectral = { 0.7859, 1.0, 1.3211 };	// Per channel on the clear Mie, green 1: MieSpectralFactors(rayleigh, -1.3)
		double mieScaleHeight = 1200.0;
		double mieG = 0.76;						// scattering/g_cornette_shanks
		double multipleScattering = 1.25;		// scattering/multi_rat: on the multiple-scattering estimate
		Rgb ozoneAbsorption = { 2.427e-6, 1.508e-6, 0.105e-6 };	// atmo/ozone_*
		double ozoneCenter = 22300.0;			// atmo/ozone_center
		double ozoneHalfWidth = 17800.0;		// atmo/ozone_width (35600) / 2
		double groundRadius = 6360000.0;
		double topRadius = 6460000.0;
		double groundAlbedo = 0.15;				// scattering/earth_albedo

		// The visibility's layer (WithVisibility): extra Mie (X-Plane 12.4.4 raises its turbidity: spectral, the
		// Cornette-Shanks lobe, in the sun's way too) and, for short visibilities, X-Plane's fog (scatter_compute_*
		// behaviour): grey, scattering isotropically, and only on view paths (the sun's light reaches every point
		// without it). The fog's share of the layer grows from none at fogStartVisibilityM to all at fogFullVisibilityM
		double hazeExtinction = 0.0;			// 1/m: the extra Mie, green; per channel the Mie's Angstrom factors (mieSpectral)
		double hazeScatteringAlbedo = 0.9;		// scattering/mie_albedo, like the clear Mie
		double fogExtinction = 0.0;				// 1/m: the fog, grey
		double fogScatteringAlbedo = 0.9;		// atmo/fog_albedo
		double fogStartVisibilityM = 20000.0;	// sedx/fog_start_visibility_m: no fog at this visibility and more
		double fogFullVisibilityM = 8000.0;		// sedx/fog_full_visibility_m: all fog at this visibility and less
		double hazeTopM = 1500.0;				// Above the ground
		double hazeBandM = 1000.0;				// Fade to clear air above the top
		double visibilityContrast = 0.01;		// scattering/vis_cutoff_ratio (VisibilityExtinction)
		double groundElevationM = 0.0;			// MSL of the ground the layer sits on
		double hazeMieScale = 1.0;				// sedx/haze_mie_scale: on the extra Mie WithVisibility gives (not the fog)

		// The sun's colour over SUN_COLOR (sky_light.h), red and blue over green (sedx/sun_tint_*): SunColor()
		Rgb sunTint = { 1.0, 1.0, 1.0 };
	};

	/* @brief X-Plane's scattering/vis_cutoff_ratio: visibility is where contrast falls to 1% (Koschmieder's 2% is 3.912 / V). */
	inline constexpr double VISIBILITY_CONTRAST = 0.01;

	/* @brief Extinction (1/m) that gives a visibility: -ln(contrast) / V (4.605 / V); 0 for no visibility limit. */
	double VisibilityExtinction(double visibilityM, double contrast = VISIBILITY_CONTRAST);

	/**
	 * @brief Per-channel Mie factors (green 1) for an Angstrom exponent (X-Plane scattering/mie_scattering_exponent
	 * -1.3: Mie ~ wavelength^-1.3), with the channels' wavelengths implied by the Rayleigh coefficients (~ wavelength^-4).
	 */
	Rgb MieSpectralFactors(const Rgb &rayleighScattering, double angstromExponent);

	/**
	 * @brief Top of the haze layer above the ground for a visibility: at least minimumTopM, deeper for very clear air
	 * whose few aerosols mix higher (ASSUMPTION: 2% of the visibility).
	 */
	double HazeTopFromVisibility(double visibilityM, double minimumTopM = 1500.0);

	/**
	 * @brief The medium for a visibility at the ground: the extinction the clear Rayleigh and Mie layers don't give in
	 * the bluest channel (never negative), where contrast falls to the cutoff first, shared between extra Mie and grey
	 * fog (none at fogStartVisibilityM and more, all at fogFullVisibilityM and less, linear in between). The layer's
	 * top is HazeTopFromVisibility with the given atmosphere's hazeTopM as the minimum.
	 */
	AtmosphereParams WithVisibility(const AtmosphereParams &atmosphere, double visibilityM, double groundElevationM = 0.0);

	/* @brief The sky-condition multipliers (X-Plane skyc ratios for Rayleigh and Mie) applied to the scattering. */
	AtmosphereParams WithSkyMultipliers(const AtmosphereParams &atmosphere, double rayleigh, double mie);

	/* @brief The haze and fog layer's density at an altitude (MSL): 1 inside, easing to 0 over the band above it (smoothstep). */
	double HazeProfile(const AtmosphereParams &atmosphere, double altitudeMslM);

	/* @brief The medium at an altitude above the ground sphere (meters; negative is treated as 0). */
	struct MediumSample
	{
		Rgb rayleighScattering{};
		Rgb mieScattering{};				// The clear Mie and the haze
		Rgb mieExtinction{};
		Rgb ozoneAbsorption{};
		double fogScattering = 0.0;			// The fog (grey, isotropic)
		double fogExtinction = 0.0;

		[[nodiscard]] Rgb Extinction() const;			// Everything: what a view ray crosses
		[[nodiscard]] Rgb ExtinctionWithoutFog() const;	// What the sun's light crosses on its way in
		[[nodiscard]] Rgb Scattering() const;			// Everything
	};
	MediumSample SampleMedium(const AtmosphereParams &atmosphere, double altitudeMslM);
}

// -------------------------------------------------------
