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
 * solar_position.h
 * -------------------------------------------------------
 * Created: 20/10/2026
 * -------------------------------------------------------
 */
#pragma once

// -------------------------------------------------------

namespace SceneryEditorX::Environment
{
	/* @brief Where the sun is for an observer (NOAA solar calculator equations). Angles in degrees. */
	struct SunPosition
	{
		double elevationDeg = 0.0;				// Geometric, above the horizon
		double apparentElevationDeg = 0.0;		// With atmospheric refraction (what an observer sees)
		double azimuthDeg = 0.0;				// True, clockwise from north
		double declinationDeg = 0.0;
		double rightAscensionDeg = 0.0;
		double apparentLongitudeDeg = 0.0;		// Ecliptic longitude
		double equationOfTimeMinutes = 0.0;
		double distanceAu = 1.0;				// Earth - sun
	};

	/* @brief Where the moon is for an observer (Meeus, Astronomical Algorithms ch. 47, main terms). */
	struct MoonPosition
	{
		double elevationDeg = 0.0;				// Topocentric (parallax applied), geometric
		double apparentElevationDeg = 0.0;		// With refraction
		double azimuthDeg = 0.0;				// True, clockwise from north
		double eclipticLongitudeDeg = 0.0;		// Geocentric
		double eclipticLatitudeDeg = 0.0;
		double rightAscensionDeg = 0.0;			// Geocentric
		double declinationDeg = 0.0;
		double distanceKm = 384400.0;			// Earth - moon centres
		double phaseAngleDeg = 0.0;				// Sun - moon - earth: 0 full, 180 new
		double illuminatedFraction = 1.0;		// 0 new .. 1 full
		bool waxing = true;
	};

	/* @brief Julian day of a Gregorian date at a UTC time (Meeus 7.1). */
	double JulianDay(int year, int month, int day, double utcHours);

	/* @brief TT - UT in seconds (polynomial fit, good to a few seconds for 1900-2100). */
	double DeltaTSeconds(double year);

	/* @brief The NOAA refraction correction (degrees) to add to a geometric elevation. */
	double RefractionDeg(double geometricElevationDeg);

	/* @brief The sun at a Julian day (UT) for an observer (latitude north +, longitude east +). */
	SunPosition ComputeSun(double julianDay, double latitudeDeg, double longitudeDeg);

	/* @brief The moon at a Julian day (UT) for an observer; low precision (a few hundredths of a degree). */
	MoonPosition ComputeMoon(double julianDay, double latitudeDeg, double longitudeDeg);

	/* @brief Greenwich mean sidereal time in degrees (0-360) at a Julian day (UT) (Meeus 12.4). */
	double GreenwichSiderealDeg(double julianDay);
}

// -------------------------------------------------------
