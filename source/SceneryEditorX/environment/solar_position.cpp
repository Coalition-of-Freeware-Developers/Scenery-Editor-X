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
 * solar_position.cpp
 * -------------------------------------------------------
 * Created: 20/10/2026
 * -------------------------------------------------------
 *
 * Sun: the equations of NOAA's solar calculator (after Meeus, Astronomical Algorithms), good to about 0.01 degrees
 * between 1800 and 2100. Moon: Meeus chapter 47 with the largest periodic terms of tables 47.A and 47.B (a few
 * hundredths of a degree), then the topocentric parallax, which moves the moon by up to a degree near the horizon.
 */
#include "solar_position.h"
#include <algorithm>
#include <cmath>

// -------------------------------------------------------

namespace SceneryEditorX::Environment
{
	namespace
	{
		constexpr double PI = 3.14159265358979323846;
		constexpr double DEG = PI / 180.0;
		constexpr double KM_PER_AU = 149597870.7;
		constexpr double EARTH_EQUATORIAL_RADIUS_KM = 6378.14;

		double Wrap360(double degrees)
		{
			degrees = std::fmod(degrees, 360.0);
			return degrees < 0.0 ? degrees + 360.0 : degrees;
		}

		double SinD(const double degrees) { return std::sin(degrees * DEG); }
		double CosD(const double degrees) { return std::cos(degrees * DEG); }

		/* @brief Mean obliquity of the ecliptic corrected for nutation (NOAA), degrees. */
		double Obliquity(const double t)
		{
			const double mean = 23.0 + (26.0 + (21.448 - t * (46.815 + t * (0.00059 - t * 0.001813))) / 60.0) / 60.0;
			return mean + 0.00256 * CosD(125.04 - 1934.136 * t);
		}

		/* @brief Elevation and azimuth (from north, clockwise) of an equatorial position for an observer. */
		void Horizontal(const double rightAscensionDeg, const double declinationDeg, const double julianDay, const double latitudeDeg, const double longitudeDeg,
			double &elevationDeg, double &azimuthDeg)
		{
			const double hourAngle = Wrap360(GreenwichSiderealDeg(julianDay) + longitudeDeg - rightAscensionDeg);
			const double sinElevation = SinD(latitudeDeg) * SinD(declinationDeg) + CosD(latitudeDeg) * CosD(declinationDeg) * CosD(hourAngle);
			elevationDeg = std::asin(std::clamp(sinElevation, -1.0, 1.0)) / DEG;
			// Azimuth from south, westward (Meeus 13.5), turned to from north, clockwise
			const double fromSouth = std::atan2(SinD(hourAngle), CosD(hourAngle) * SinD(latitudeDeg) - std::tan(declinationDeg * DEG) * CosD(latitudeDeg)) / DEG;
			azimuthDeg = Wrap360(fromSouth + 180.0);
		}

		struct Term
		{
			signed char d, m, mp, f;
			double coefficient;
		};

		// Meeus table 47.A: longitude (1e-6 degrees) and distance (1e-3 km), arguments D M M' F
		constexpr Term LONGITUDE_TERMS[] = {
			{ 0, 0, 1, 0, 6288774 }, { 2, 0, -1, 0, 1274027 }, { 2, 0, 0, 0, 658314 }, { 0, 0, 2, 0, 213618 },
			{ 0, 1, 0, 0, -185116 }, { 0, 0, 0, 2, -114332 }, { 2, 0, -2, 0, 58793 }, { 2, -1, -1, 0, 57066 },
			{ 2, 0, 1, 0, 53322 }, { 2, -1, 0, 0, 45758 }, { 0, 1, -1, 0, -40923 }, { 1, 0, 0, 0, -34720 },
			{ 0, 1, 1, 0, -30383 }, { 2, 0, 0, -2, 15327 }, { 0, 0, 1, 2, -12528 }, { 0, 0, 1, -2, 10980 },
			{ 4, 0, -1, 0, 10675 }, { 0, 0, 3, 0, 10034 }, { 4, 0, -2, 0, 8548 }, { 2, 1, -1, 0, -7888 },
			{ 2, 1, 0, 0, -6766 }, { 1, 0, -1, 0, -5163 }, { 1, 1, 0, 0, 4987 }, { 2, -1, 1, 0, 4036 },
			{ 2, 0, 2, 0, 3994 }, { 4, 0, 0, 0, 3861 }, { 2, 0, -3, 0, 3665 }, { 0, 1, -2, 0, -2689 },
			{ 2, 0, -1, 2, -2602 }, { 2, -1, -2, 0, 2390 }, { 1, 0, 1, 0, -2348 }, { 2, -2, 0, 0, 2236 },
			{ 0, 1, 2, 0, -2120 }, { 0, 2, 0, 0, -2069 }, { 2, -2, -1, 0, 2048 }, { 2, 0, 1, -2, -1773 },
			{ 2, 0, 0, 2, -1595 }, { 4, -1, -1, 0, 1215 }, { 0, 0, 2, 2, -1110 }, { 3, 0, -1, 0, -892 },
		};
		constexpr Term DISTANCE_TERMS[] = {
			{ 0, 0, 1, 0, -20905355 }, { 2, 0, -1, 0, -3699111 }, { 2, 0, 0, 0, -2955968 }, { 0, 0, 2, 0, -569925 },
			{ 0, 1, 0, 0, 48888 }, { 0, 0, 0, 2, -3149 }, { 2, 0, -2, 0, 246158 }, { 2, -1, -1, 0, -152138 },
			{ 2, 0, 1, 0, -170733 }, { 2, -1, 0, 0, -204586 }, { 0, 1, -1, 0, -129620 }, { 1, 0, 0, 0, 108743 },
			{ 0, 1, 1, 0, 104755 }, { 2, 0, 0, -2, 10321 }, { 0, 0, 1, -2, 79661 }, { 4, 0, -1, 0, -34782 },
			{ 0, 0, 3, 0, -23210 }, { 4, 0, -2, 0, -21636 }, { 2, 1, -1, 0, 24208 }, { 2, 1, 0, 0, 30824 },
			{ 1, 0, -1, 0, -8379 }, { 1, 1, 0, 0, -16675 }, { 2, -1, 1, 0, -12831 }, { 2, 0, 2, 0, -10445 },
			{ 4, 0, 0, 0, -11650 }, { 2, 0, -3, 0, 14403 }, { 0, 1, -2, 0, -7003 }, { 2, -1, -2, 0, 10056 },
			{ 1, 0, 1, 0, 6322 }, { 2, -2, 0, 0, -9884 }, { 0, 1, 2, 0, 5751 }, { 2, -2, -1, 0, -4950 },
			{ 2, 0, 1, -2, 4130 }, { 4, -1, -1, 0, -3958 }, { 3, 0, -1, 0, 3258 }, { 2, 1, 1, 0, 2616 },
			{ 4, -1, -2, 0, -1897 },
		};
		// Meeus table 47.B: latitude (1e-6 degrees)
		constexpr Term LATITUDE_TERMS[] = {
			{ 0, 0, 0, 1, 5128122 }, { 0, 0, 1, 1, 280602 }, { 0, 0, 1, -1, 277693 }, { 2, 0, 0, -1, 173237 },
			{ 2, 0, -1, 1, 55413 }, { 2, 0, -1, -1, 46271 }, { 2, 0, 0, 1, 32573 }, { 0, 0, 2, 1, 17198 },
			{ 2, 0, 1, -1, 9266 }, { 0, 0, 2, -1, 8822 }, { 2, -1, 0, -1, 8216 }, { 2, 0, -2, -1, 4324 },
			{ 2, 0, 1, 1, 4200 }, { 2, 1, 0, -1, -3359 }, { 2, -1, -1, 1, 2463 }, { 2, -1, 0, 1, 2211 },
			{ 2, -1, -1, -1, 2065 }, { 0, 1, -1, -1, -1870 }, { 4, 0, -1, -1, 1828 }, { 0, 1, 0, 1, -1794 },
			{ 0, 0, 0, 3, -1749 }, { 0, 1, -1, 1, -1565 }, { 1, 0, 0, 1, -1491 }, { 0, 1, 1, 1, -1475 },
		};

		/* @brief Sum of periodic terms; E scales the terms with the sun's anomaly M (once per |M|). */
		template <size_t N>
		double SumTerms(const Term (&terms)[N], const double d, const double m, const double mp, const double f, const double e, const bool cosine)
		{
			double sum = 0.0;
			for (const Term &term : terms)
			{
				const double argument = term.d * d + term.m * m + term.mp * mp + term.f * f;
				double coefficient = term.coefficient;
				if (term.m == 1 || term.m == -1)
					coefficient *= e;
				else if (term.m == 2 || term.m == -2)
					coefficient *= e * e;
				sum += coefficient * (cosine ? CosD(argument) : SinD(argument));
			}
			return sum;
		}
	}

	// -------------------------------------------------------

	double JulianDay(int year, int month, const int day, const double utcHours)
	{
		if (month <= 2)
		{
			year -= 1;
			month += 12;
		}
		const int a = year / 100;
		const int b = 2 - a + a / 4;
		return std::floor(365.25 * (year + 4716)) + std::floor(30.6001 * (month + 1)) + day + b - 1524.5 + utcHours / 24.0;
	}

	double DeltaTSeconds(const double year)
	{
		// Espenak & Meeus polynomial expressions (NASA eclipse site)
		if (year >= 2005.0 && year < 2050.0)
		{
			const double t = year - 2000.0;
			return 62.92 + 0.32217 * t + 0.005589 * t * t;
		}
		if (year >= 1986.0 && year < 2005.0)
		{
			const double t = year - 2000.0;
			return 63.86 + t * (0.3345 + t * (-0.060374 + t * (0.0017275 + t * (0.000651814 + t * 0.00002373599))));
		}
		const double u = (year - 1820.0) / 100.0;
		if (year >= 2050.0 && year < 2150.0)
			return -20.0 + 32.0 * u * u - 0.5628 * (2150.0 - year);
		return -20.0 + 32.0 * u * u;
	}

	double RefractionDeg(const double elevationDeg)
	{
		if (elevationDeg > 85.0)
			return 0.0;
		const double te = std::tan(elevationDeg * DEG);
		double seconds;
		if (elevationDeg > 5.0)
			seconds = 58.1 / te - 0.07 / (te * te * te) + 0.000086 / std::pow(te, 5.0);
		else if (elevationDeg > -0.575)
			seconds = 1735.0 + elevationDeg * (-518.2 + elevationDeg * (103.4 + elevationDeg * (-12.79 + elevationDeg * 0.711)));
		else
			seconds = -20.772 / te;
		return seconds / 3600.0;
	}

	double GreenwichSiderealDeg(const double julianDay)
	{
		const double t = (julianDay - 2451545.0) / 36525.0;
		return Wrap360(280.46061837 + 360.98564736629 * (julianDay - 2451545.0) + t * t * (0.000387933 - t / 38710000.0));
	}

	SunPosition ComputeSun(const double julianDay, const double latitudeDeg, const double longitudeDeg)
	{
		SunPosition sun;
		const double t = (julianDay - 2451545.0) / 36525.0;
		const double l0 = Wrap360(280.46646 + t * (36000.76983 + t * 0.0003032));
		const double m = 357.52911 + t * (35999.05029 - 0.0001537 * t);
		const double e = 0.016708634 - t * (0.000042037 + 0.0000001267 * t);
		const double center = SinD(m) * (1.914602 - t * (0.004817 + 0.000014 * t)) + SinD(2.0 * m) * (0.019993 - 0.000101 * t) + SinD(3.0 * m) * 0.000289;
		const double trueLongitude = l0 + center;
		const double trueAnomaly = m + center;
		sun.distanceAu = 1.000001018 * (1.0 - e * e) / (1.0 + e * CosD(trueAnomaly));
		const double omega = 125.04 - 1934.136 * t;
		const double apparentLongitude = trueLongitude - 0.00569 - 0.00478 * SinD(omega);
		const double obliquity = Obliquity(t);
		sun.apparentLongitudeDeg = Wrap360(apparentLongitude);
		sun.rightAscensionDeg = Wrap360(std::atan2(CosD(obliquity) * SinD(apparentLongitude), CosD(apparentLongitude)) / DEG);
		sun.declinationDeg = std::asin(SinD(obliquity) * SinD(apparentLongitude)) / DEG;

		const double y = std::pow(std::tan(obliquity * 0.5 * DEG), 2.0);
		sun.equationOfTimeMinutes = 4.0 / DEG * (y * SinD(2.0 * l0) - 2.0 * e * SinD(m) + 4.0 * e * y * SinD(m) * CosD(2.0 * l0) - 0.5 * y * y * SinD(4.0 * l0) -
			1.25 * e * e * SinD(2.0 * m));

		// Hour angle from the true solar time (NOAA), which folds in the equation of time
		const double utcMinutes = (julianDay + 0.5 - std::floor(julianDay + 0.5)) * 1440.0;
		const double trueSolarMinutes = std::fmod(utcMinutes + sun.equationOfTimeMinutes + 4.0 * longitudeDeg + 1440.0 * 4.0, 1440.0);
		const double hourAngle = trueSolarMinutes / 4.0 - 180.0;

		const double cosZenith = SinD(latitudeDeg) * SinD(sun.declinationDeg) + CosD(latitudeDeg) * CosD(sun.declinationDeg) * CosD(hourAngle);
		const double zenith = std::acos(std::clamp(cosZenith, -1.0, 1.0)) / DEG;
		sun.elevationDeg = 90.0 - zenith;
		sun.apparentElevationDeg = sun.elevationDeg + RefractionDeg(sun.elevationDeg);

		const double denominator = CosD(latitudeDeg) * SinD(zenith);
		if (std::abs(denominator) > 1e-9)
		{
			const double cosAzimuth = std::clamp((SinD(latitudeDeg) * CosD(zenith) - SinD(sun.declinationDeg)) / denominator, -1.0, 1.0);
			const double angle = std::acos(cosAzimuth) / DEG;
			sun.azimuthDeg = hourAngle > 0.0 ? Wrap360(angle + 180.0) : Wrap360(540.0 - angle);
		}
		else
			sun.azimuthDeg = latitudeDeg > 0.0 ? 180.0 : 0.0;	// Sun at the zenith or observer at a pole
		return sun;
	}

	MoonPosition ComputeMoon(const double julianDay, const double latitudeDeg, const double longitudeDeg)
	{
		MoonPosition moon;
		// Terrestrial time for the theory, universal time for the earth's rotation
		const double year = 2000.0 + (julianDay - 2451545.0) / 365.25;
		const double jde = julianDay + DeltaTSeconds(year) / 86400.0;
		const double t = (jde - 2451545.0) / 36525.0;

		const double lp = Wrap360(218.3164477 + t * (481267.88123421 + t * (-0.0015786 + t * (1.0 / 538841.0 - t / 65194000.0))));
		const double d = Wrap360(297.8501921 + t * (445267.1114034 + t * (-0.0018819 + t * (1.0 / 545868.0 - t / 113065000.0))));
		const double m = Wrap360(357.5291092 + t * (35999.0502909 + t * (-0.0001536 + t / 24490000.0)));
		const double mp = Wrap360(134.9633964 + t * (477198.8675055 + t * (0.0087414 + t * (1.0 / 69699.0 - t / 14712000.0))));
		const double f = Wrap360(93.2720950 + t * (483202.0175233 + t * (-0.0036539 + t * (-1.0 / 3526000.0 + t / 863310000.0))));
		const double e = 1.0 - t * (0.002516 + 0.0000074 * t);
		const double a1 = 119.75 + 131.849 * t;
		const double a2 = 53.09 + 479264.290 * t;
		const double a3 = 313.45 + 481266.484 * t;

		double sumL = SumTerms(LONGITUDE_TERMS, d, m, mp, f, e, false);
		double sumB = SumTerms(LATITUDE_TERMS, d, m, mp, f, e, false);
		const double sumR = SumTerms(DISTANCE_TERMS, d, m, mp, f, e, true);
		sumL += 3958.0 * SinD(a1) + 1962.0 * SinD(lp - f) + 318.0 * SinD(a2);
		sumB += -2235.0 * SinD(lp) + 382.0 * SinD(a3) + 175.0 * SinD(a1 - f) + 175.0 * SinD(a1 + f) + 127.0 * SinD(lp - mp) - 115.0 * SinD(lp + mp);

		// Apparent longitude: nutation in longitude, main term
		const double omega = 125.04452 - 1934.136261 * t;
		const double nutation = -17.20 / 3600.0 * SinD(omega) - 1.32 / 3600.0 * SinD(2.0 * (280.4665 + 36000.7698 * t)) - 0.23 / 3600.0 * SinD(2.0 * lp);
		moon.eclipticLongitudeDeg = Wrap360(lp + sumL / 1e6 + nutation);
		moon.eclipticLatitudeDeg = sumB / 1e6;
		moon.distanceKm = 385000.56 + sumR / 1000.0;

		const double obliquity = Obliquity(t);
		const double lambda = moon.eclipticLongitudeDeg;
		const double beta = moon.eclipticLatitudeDeg;
		moon.rightAscensionDeg = Wrap360(std::atan2(SinD(lambda) * CosD(obliquity) - std::tan(beta * DEG) * SinD(obliquity), CosD(lambda)) / DEG);
		moon.declinationDeg = std::asin(std::clamp(SinD(beta) * CosD(obliquity) + CosD(beta) * SinD(obliquity) * SinD(lambda), -1.0, 1.0)) / DEG;

		double geocentricElevation = 0.0;
		Horizontal(moon.rightAscensionDeg, moon.declinationDeg, julianDay, latitudeDeg, longitudeDeg, geocentricElevation, moon.azimuthDeg);
		// Topocentric: seen from the earth's surface the moon sits lower by the parallax (up to ~1 degree)
		const double parallax = std::asin(EARTH_EQUATORIAL_RADIUS_KM / moon.distanceKm) / DEG;
		moon.elevationDeg = geocentricElevation - parallax * CosD(geocentricElevation);
		moon.apparentElevationDeg = moon.elevationDeg + RefractionDeg(moon.elevationDeg);

		// Phase from the elongation (Meeus 48.2, 48.3) against the sun's apparent position
		const SunPosition sun = ComputeSun(jde, 0.0, 0.0);
		const double sunDistanceKm = sun.distanceAu * KM_PER_AU;
		const double cosElongation = std::clamp(CosD(beta) * CosD(lambda - sun.apparentLongitudeDeg), -1.0, 1.0);
		const double elongation = std::acos(cosElongation);
		moon.phaseAngleDeg = std::atan2(sunDistanceKm * std::sin(elongation), moon.distanceKm - sunDistanceKm * cosElongation) / DEG;
		moon.illuminatedFraction = (1.0 + CosD(moon.phaseAngleDeg)) * 0.5;
		moon.waxing = Wrap360(lambda - sun.apparentLongitudeDeg) < 180.0;
		return moon;
	}
}

// -------------------------------------------------------
