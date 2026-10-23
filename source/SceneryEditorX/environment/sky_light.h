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
 * sky_light.h
 * -------------------------------------------------------
 * Created: 20/10/2026
 * -------------------------------------------------------
 */
#pragma once
#include "atmosphere_params.h"
#include <array>

// -------------------------------------------------------

namespace SceneryEditorX::Environment
{
	/**
	 * What lights the scene, at the observer: the sun and moon after the atmosphere, and the sky's radiance in a few
	 * directions plus its irradiance on a level surface. Units: lux (illuminance) and nits (radiance), linear sRGB.
	 *
	 * Single scattering plus an isotropic estimate of the higher orders (the diffuse field the sky and the ground make,
	 * re-scattered as a geometric series): the interim model until the atmosphere LUTs of plan Phase 6.
	 */
	struct SkyLight
	{
		Rgb sunTransmittance = { 0.0, 0.0, 0.0 };	// Toward the sun (0 below the horizon)
		Rgb sunIlluminance = { 0.0, 0.0, 0.0 };		// On a surface facing the sun
		Rgb moonIlluminance = { 0.0, 0.0, 0.0 };
		// Sky radiance without the Mie lobe (Rayleigh plus every higher order), and the Mie single scattering with its
		// phase factored out: radiance = base + MiePhaseCornetteShanks(cos(view, sun), g) x mie. Straight up, at the
		// horizon (toward the sun's azimuth, 90 degrees off, away) and 30 degrees up (same azimuths).
		Rgb zenith = { 0.0, 0.0, 0.0 };
		std::array<Rgb, 3> horizon{};
		std::array<Rgb, 3> mid{};
		Rgb zenithMie = { 0.0, 0.0, 0.0 };
		std::array<Rgb, 3> horizonMie{};
		std::array<Rgb, 3> midMie{};
		Rgb irradiance = { 0.0, 0.0, 0.0 };			// From the sky on a level surface
	};

	/* @brief The extraterrestrial sun's colour, linear sRGB scaled to unit luminance (ASSUMPTION: about 5900 K). */
	constexpr Rgb SUN_COLOR = { 1.0236, 0.9961, 0.9693 };

	/* @brief Fraction of light passing from altitude (m) to space along a ray at cosZenith (0 when it meets the ground). */
	Rgb ComputeTransmittance(const AtmosphereParams &atmosphere, double altitudeM, double cosZenith, int steps = 40);

	/**
	 * @brief Single-scattered sky radiance (nits) seen from altitudeM along a direction (elevation above the horizon,
	 * azimuth from the sun's azimuth), lit by a sun at sunElevationDeg with illuminance sunLux outside the atmosphere.
	 */
	Rgb ComputeSkyRadiance(const AtmosphereParams &atmosphere, double altitudeM, double viewElevationDeg, double viewAzimuthFromSunDeg,
		double sunElevationDeg, double sunLux, int steps = 32);

	/* @brief The Mie phase function the sky uses (Cornette-Shanks), per steradian. */
	double MiePhaseCornetteShanks(double cosTheta, double g);

	/**
	 * @brief Everything SkyLight holds. sunLux / moonLux: illuminance outside the atmosphere (lighting/E_sun_lx, and
	 * lighting/E_moon_lx x illuminated fraction). A few milliseconds: cache it on the inputs.
	 */
	SkyLight ComputeSkyLight(const AtmosphereParams &atmosphere, double altitudeM, double sunElevationDeg, double sunLux, double moonElevationDeg, double moonLux);
}

// -------------------------------------------------------
