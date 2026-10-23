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
 * sky_light.cpp
 * -------------------------------------------------------
 * Created: 20/10/2026
 * -------------------------------------------------------
 */
#include "sky_light.h"
#include <algorithm>
#include <cmath>
#include <numbers>

// -------------------------------------------------------

namespace SceneryEditorX::Environment
{
	namespace
	{
		constexpr double PI = std::numbers::pi;
		constexpr double DEG = PI / 180.0;

		struct Vec
		{
			double x = 0.0, y = 0.0, z = 0.0;
			Vec operator+(const Vec &o) const { return { x + o.x, y + o.y, z + o.z }; }
			Vec operator*(const double s) const { return { x * s, y * s, z * s }; }
			[[nodiscard]] double Dot(const Vec &o) const { return x * o.x + y * o.y + z * o.z; }
			[[nodiscard]] double Length() const { return std::sqrt(Dot(*this)); }
		};

		/* @brief A direction from elevation above the horizon and azimuth (y up; azimuth 0 along +x). */
		Vec Direction(const double elevationDeg, const double azimuthDeg)
		{
			const double e = elevationDeg * DEG;
			const double a = azimuthDeg * DEG;
			return { std::cos(e) * std::cos(a), std::sin(e), std::cos(e) * std::sin(a) };
		}

		/* @brief Distance along d from p to the sphere of radius, the far crossing; negative when it misses. */
		double ExitDistance(const Vec &p, const Vec &d, const double radius)
		{
			const double b = p.Dot(d);
			const double c = p.Dot(p) - radius * radius;
			const double disc = b * b - c;
			return disc < 0.0 ? -1.0 : -b + std::sqrt(disc);
		}

		/* @brief Distance along d to the ground, or -1 when the ray clears it. */
		double GroundDistance(const Vec &p, const Vec &d, const double radius)
		{
			const double b = p.Dot(d);
			const double c = p.Dot(p) - radius * radius;
			const double disc = b * b - c;
			if (b >= 0.0 || disc < 0.0)
				return -1.0;
			const double t = -b - std::sqrt(disc);
			return t >= -1e-6 ? std::max(t, 0.0) : -1.0;	// 0: leaving the ground downward (an observer standing on it)
		}

		/* @brief Optical depth (extinction x length) accumulated along a path. */
		struct Depth
		{
			Rgb tau{};
			void Add(const MediumSample &m, const double length)
			{
				const Rgb e = m.Extinction();
				for (int c = 0; c < 3; ++c)
					tau[c] += e[c] * length;
			}
		};

		Rgb Exp(const Rgb &tau)
		{
			return { std::exp(-tau[0]), std::exp(-tau[1]), std::exp(-tau[2]) };
		}

		double RayleighPhase(const double cosTheta)
		{
			return 3.0 / (16.0 * PI) * (1.0 + cosTheta * cosTheta);
		}

		double MiePhase(const double cosTheta, const double g)
		{
			// Cornette-Shanks
			const double g2 = g * g;
			return 3.0 / (8.0 * PI) * (1.0 - g2) * (1.0 + cosTheta * cosTheta) / ((2.0 + g2) * std::pow(1.0 + g2 - 2.0 * g * cosTheta, 1.5));
		}
	}

	Rgb ComputeTransmittance(const AtmosphereParams &atmosphere, const double altitudeM, const double cosZenith, const int steps)
	{
		const Vec p{ 0.0, atmosphere.groundRadius + std::max(altitudeM, 0.0), 0.0 };
		const double mu = std::clamp(cosZenith, -1.0, 1.0);
		const Vec d{ std::sqrt(std::max(0.0, 1.0 - mu * mu)), mu, 0.0 };
		if (GroundDistance(p, d, atmosphere.groundRadius) >= 0.0)
			return { 0.0, 0.0, 0.0 };
		const double length = ExitDistance(p, d, atmosphere.topRadius);
		if (length <= 0.0)
			return { 1.0, 1.0, 1.0 };

		Depth depth;
		const double dt = length / steps;
		for (int i = 0; i < steps; ++i)
		{
			const Vec q = p + d * ((i + 0.5) * dt);
			depth.Add(SampleMedium(atmosphere, q.Length() - atmosphere.groundRadius), dt);
		}
		return Exp(depth.tau);
	}

	namespace
	{
		/* @brief One view ray: Rayleigh in-scattering (with its phase), Mie in-scattering without its phase, and G. */
		struct Marched
		{
			Rgb rayleigh{};
			Rgb mie{};
			double gather = 0.0;	// How much of an isotropic field the ray picks up (green): the multiple-scattering estimate
			double cosTheta = 1.0;	// Between the view and the sun
		};

		Marched March(const AtmosphereParams &atmosphere, const double altitudeM, const double viewElevationDeg, const double viewAzimuthFromSunDeg,
			const double sunElevationDeg, const double sunLux, const int steps)
		{
			Marched out;
			const Vec p{ 0.0, atmosphere.groundRadius + std::max(altitudeM, 1.0), 0.0 };
			const Vec d = Direction(viewElevationDeg, viewAzimuthFromSunDeg);
			const Vec s = Direction(sunElevationDeg, 0.0);
			out.cosTheta = d.Dot(s);

			double length = ExitDistance(p, d, atmosphere.topRadius);
			const double ground = GroundDistance(p, d, atmosphere.groundRadius);
			if (ground > 0.0)
				length = ground;
			if (length <= 0.0)
				return out;

			const double phaseR = RayleighPhase(out.cosTheta);
			Depth depth;	// observer -> sample
			const double dt = length / steps;
			for (int i = 0; i < steps; ++i)
			{
				const Vec q = p + d * ((i + 0.5) * dt);
				const double r = q.Length();
				const MediumSample m = SampleMedium(atmosphere, r - atmosphere.groundRadius);
				Depth toSample = depth;
				toSample.Add(m, dt * 0.5);
				const Rgb toObserver = Exp(toSample.tau);
				const Rgb toSun = ComputeTransmittance(atmosphere, r - atmosphere.groundRadius, q.Dot(s) / r, 16);
				for (int c = 0; c < 3; ++c)
				{
					const double light = toObserver[c] * toSun[c] * sunLux * SUN_COLOR[c] * dt;
					out.rayleigh[c] += light * m.rayleighScattering[c] * phaseR;
					out.mie[c] += light * m.mieScattering[c];
				}
				out.gather += toObserver[1] * (m.rayleighScattering[1] + m.mieScattering[1]) * dt;
				depth.Add(m, dt);
			}
			return out;
		}
	}

	double MiePhaseCornetteShanks(const double cosTheta, const double g)
	{
		return MiePhase(cosTheta, g);
	}

	Rgb ComputeSkyRadiance(const AtmosphereParams &atmosphere, const double altitudeM, const double viewElevationDeg, const double viewAzimuthFromSunDeg,
		const double sunElevationDeg, const double sunLux, const int steps)
	{
		const Marched m = March(atmosphere, altitudeM, viewElevationDeg, viewAzimuthFromSunDeg, sunElevationDeg, sunLux, steps);
		const double phaseM = MiePhase(m.cosTheta, atmosphere.mieG);
		return { m.rayleigh[0] + m.mie[0] * phaseM, m.rayleigh[1] + m.mie[1] * phaseM, m.rayleigh[2] + m.mie[2] * phaseM };
	}

	SkyLight ComputeSkyLight(const AtmosphereParams &atmosphere, const double altitudeM, const double sunElevationDeg, const double sunLux,
		const double moonElevationDeg, const double moonLux)
	{
		SkyLight light;
		light.sunTransmittance = ComputeTransmittance(atmosphere, altitudeM, std::sin(sunElevationDeg * DEG));
		const Rgb moonTransmittance = ComputeTransmittance(atmosphere, altitudeM, std::sin(moonElevationDeg * DEG));
		for (int c = 0; c < 3; ++c)
		{
			light.sunIlluminance[c] = sunLux * SUN_COLOR[c] * light.sunTransmittance[c];
			light.moonIlluminance[c] = moonLux * SUN_COLOR[c] * moonTransmittance[c];
		}

		// A sun far below the horizon lights nothing (and saves the integration)
		if (sunElevationDeg < -18.0 || sunLux <= 0.0)
			return light;

		// Single scattering, split into Rayleigh and Mie (phase factored out), and how much of an isotropic field each
		// direction gathers (G)
		const auto sky = [&](const double elevation, const double azimuth) { return March(atmosphere, altitudeM, elevation, azimuth, sunElevationDeg, sunLux, 32); };
		const Marched zenith = sky(90.0, 0.0);
		constexpr double AZIMUTHS[3] = { 0.0, 90.0, 180.0 };
		std::array<Marched, 3> horizon;
		std::array<Marched, 3> mid;
		for (int i = 0; i < 3; ++i)
		{
			horizon[i] = sky(0.5, AZIMUTHS[i]);
			mid[i] = sky(30.0, AZIMUTHS[i]);
		}

		// Irradiance on a level surface: the radiance over the upper hemisphere, weighted by cos(zenith). The sky is
		// symmetric about the sun's vertical plane, so azimuths 0-180 count twice.
		constexpr int ELEVATIONS = 8;
		constexpr int HALF_AZIMUTHS = 6;
		const double dElevation = 90.0 / ELEVATIONS * DEG;
		const double dAzimuth = 180.0 / HALF_AZIMUTHS * DEG;
		Rgb single{};
		double gatherCos = 0.0;		// cos-weighted hemisphere integral of G (pi for a uniform G of 1)
		double gatherMean = 0.0;	// G averaged over the hemisphere
		for (int e = 0; e < ELEVATIONS; ++e)
		{
			const double elevation = (e + 0.5) * 90.0 / ELEVATIONS;
			const double solid = std::cos(elevation * DEG) * dElevation * dAzimuth * 2.0;
			const double weight = std::sin(elevation * DEG) * solid;
			for (int a = 0; a < HALF_AZIMUTHS; ++a)
			{
				const Marched m = sky(elevation, (a + 0.5) * 180.0 / HALF_AZIMUTHS);
				const double phaseM = MiePhase(m.cosTheta, atmosphere.mieG);
				for (int c = 0; c < 3; ++c)
					single[c] += (m.rayleigh[c] + m.mie[c] * phaseM) * weight;
				gatherCos += m.gather * weight;
				gatherMean += m.gather * solid / (2.0 * PI);
			}
		}

		// Higher orders: the sky and the sunlit ground make a diffuse field L_d = (E_down + E_up) / 2 pi; a direction
		// gathers L_d x G of it, and what it gathers scatters again (a geometric series in the mean G)
		const double sunUp = std::max(std::sin(sunElevationDeg * DEG), 0.0);
		const double series = 1.0 / std::max(1.0 - gatherMean, 0.2);
		Rgb diffuse{};
		for (int c = 0; c < 3; ++c)
		{
			const double up = atmosphere.groundAlbedo * (light.sunIlluminance[c] * sunUp + single[c]);
			diffuse[c] = (single[c] + up) / (2.0 * PI) * series * atmosphere.multipleScattering;
		}
		const auto base = [&](const Marched &m)
		{
			Rgb out{};
			for (int c = 0; c < 3; ++c)
				out[c] = m.rayleigh[c] + diffuse[c] * m.gather;
			return out;
		};
		light.zenith = base(zenith);
		light.zenithMie = zenith.mie;
		for (int i = 0; i < 3; ++i)
		{
			light.horizon[i] = base(horizon[i]);
			light.horizonMie[i] = horizon[i].mie;
			light.mid[i] = base(mid[i]);
			light.midMie[i] = mid[i].mie;
		}
		for (int c = 0; c < 3; ++c)
			light.irradiance[c] = single[c] + diffuse[c] * gatherCos;

		// Energy: the sky can't send down more than the beam lost on its way, of which forward-scattering haze sends
		// most down (ASSUMPTION: at most 85%); the isotropic estimate overshoots in thick haze
		const double lost = std::max(sunLux * SUN_COLOR[1] * sunUp - light.sunIlluminance[1] * sunUp, 0.0);
		const double cap = 0.85 * lost;
		if (sunUp > 0.0 && light.irradiance[1] > cap)
		{
			const double scale = cap / light.irradiance[1];
			for (int c = 0; c < 3; ++c)
			{
				light.irradiance[c] *= scale;
				light.zenith[c] *= scale;
				light.zenithMie[c] *= scale;
				for (int i = 0; i < 3; ++i)
				{
					light.horizon[i][c] *= scale;
					light.horizonMie[i][c] *= scale;
					light.mid[i][c] *= scale;
					light.midMie[i][c] *= scale;
				}
			}
		}
		return light;
	}
}

// -------------------------------------------------------
