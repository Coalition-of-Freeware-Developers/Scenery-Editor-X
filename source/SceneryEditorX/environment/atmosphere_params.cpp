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
 * atmosphere_params.cpp
 * -------------------------------------------------------
 * Created: 20/10/2026
 * -------------------------------------------------------
 */
#include "atmosphere_params.h"
#include <algorithm>
#include <cmath>

// -------------------------------------------------------

namespace SceneryEditorX::Environment
{
	double VisibilityExtinction(const double visibilityM, const double contrast)
	{
		return visibilityM > 0.0 ? -std::log(std::clamp(contrast, 1e-6, 0.999)) / visibilityM : 0.0;
	}

	Rgb MieSpectralFactors(const Rgb &rayleighScattering, const double angstromExponent)
	{
		// (lambda_c / lambda_g)^exponent with lambda_c / lambda_g = (rayleigh_g / rayleigh_c)^(1/4)
		Rgb out{ 1.0, 1.0, 1.0 };
		const double green = rayleighScattering[1];
		if (green <= 0.0)
			return out;
		for (int c = 0; c < 3; ++c)
		{
			if (rayleighScattering[c] > 0.0)
				out[c] = std::pow(rayleighScattering[c] / green, -angstromExponent / 4.0);
		}
		return out;
	}

	double HazeTopFromVisibility(const double visibilityM, const double minimumTopM)
	{
		return std::max(minimumTopM, 0.02 * std::max(visibilityM, 0.0));
	}

	AtmosphereParams WithVisibility(const AtmosphereParams &atmosphere, const double visibilityM, const double groundElevationM)
	{
		AtmosphereParams out = atmosphere;
		out.groundElevationM = groundElevationM;
		out.hazeTopM = HazeTopFromVisibility(visibilityM, atmosphere.hazeTopM);

		// What the clear layers don't give at the ground, measured in the bluest channel: there the clear air and the
		// Mie are thickest, and contrast falls to the cutoff first. X-Plane 12.4.4 draws no fog at 20 km and more (only a
		// thicker Mie), and its 8 km haze is fog-white (ASSUMPTION: linear in between)
		const double ground = std::max(groundElevationM, 0.0);
		const double rayleigh = atmosphere.rayleighScattering[2] * std::exp(-ground / atmosphere.rayleighScaleHeight);
		const double mie = atmosphere.mieExtinction * std::exp(-ground / atmosphere.mieScaleHeight);
		const double needed = std::max(VisibilityExtinction(visibilityM, atmosphere.visibilityContrast) - rayleigh - mie * atmosphere.mieSpectral[2], 0.0);
		const double span = atmosphere.fogStartVisibilityM - atmosphere.fogFullVisibilityM;
		double fog = 0.0;
		if (visibilityM > 0.0)
			fog = span > 0.0 ? std::clamp((atmosphere.fogStartVisibilityM - visibilityM) / span, 0.0, 1.0) : (visibilityM < atmosphere.fogStartVisibilityM ? 1.0 : 0.0);
		out.hazeExtinction = (1.0 - fog) * needed / std::max(atmosphere.mieSpectral[2], 1e-3);
		out.fogExtinction = fog * needed;
		return out;
	}

	AtmosphereParams WithSkyMultipliers(const AtmosphereParams &atmosphere, const double rayleigh, const double mie)
	{
		AtmosphereParams out = atmosphere;
		for (double &c : out.rayleighScattering)
			c *= std::max(rayleigh, 0.0);

		// Mie: scattering scales, absorption (extinction - scattering) stays
		const double m = std::max(mie, 0.0);
		const double absorption = atmosphere.mieExtinction - atmosphere.mieScattering;
		out.mieScattering = atmosphere.mieScattering * m;
		out.mieExtinction = out.mieScattering + absorption;
		const double hazeScattering = atmosphere.hazeExtinction * atmosphere.hazeScatteringAlbedo * m;
		const double hazeAbsorption = atmosphere.hazeExtinction * (1.0 - atmosphere.hazeScatteringAlbedo);
		out.hazeExtinction = hazeScattering + hazeAbsorption;
		out.hazeScatteringAlbedo = out.hazeExtinction > 0.0 ? hazeScattering / out.hazeExtinction : atmosphere.hazeScatteringAlbedo;
		return out;
	}

	double HazeProfile(const AtmosphereParams &atmosphere, const double altitudeMslM)
	{
		const double above = altitudeMslM - atmosphere.groundElevationM - atmosphere.hazeTopM;
		if (above <= 0.0)
			return 1.0;
		if (atmosphere.hazeBandM <= 0.0)
			return 0.0;
		const double x = std::clamp(1.0 - above / atmosphere.hazeBandM, 0.0, 1.0);
		return x * x * (3.0 - 2.0 * x);
	}

	Rgb MediumSample::Extinction() const
	{
		const Rgb lit = ExtinctionWithoutFog();
		return { lit[0] + fogExtinction, lit[1] + fogExtinction, lit[2] + fogExtinction };
	}

	Rgb MediumSample::ExtinctionWithoutFog() const
	{
		return { rayleighScattering[0] + mieExtinction[0] + ozoneAbsorption[0], rayleighScattering[1] + mieExtinction[1] + ozoneAbsorption[1],
				 rayleighScattering[2] + mieExtinction[2] + ozoneAbsorption[2] };
	}

	Rgb MediumSample::Scattering() const
	{
		return { rayleighScattering[0] + mieScattering[0] + fogScattering, rayleighScattering[1] + mieScattering[1] + fogScattering,
				 rayleighScattering[2] + mieScattering[2] + fogScattering };
	}

	MediumSample SampleMedium(const AtmosphereParams &a, const double altitudeMslM)
	{
		const double h = std::max(altitudeMslM, 0.0);
		const double rayleigh = std::exp(-h / a.rayleighScaleHeight);
		const double mie = std::exp(-h / a.mieScaleHeight);
		const double ozone = std::max(0.0, 1.0 - std::abs(h - a.ozoneCenter) / a.ozoneHalfWidth);
		const double layer = HazeProfile(a, h);
		const double haze = a.hazeExtinction * layer;

		MediumSample s;
		for (int c = 0; c < 3; ++c)
		{
			s.rayleighScattering[c] = a.rayleighScattering[c] * rayleigh;
			s.ozoneAbsorption[c] = a.ozoneAbsorption[c] * ozone;
			s.mieScattering[c] = (a.mieScattering * mie + haze * a.hazeScatteringAlbedo) * a.mieSpectral[c];
			s.mieExtinction[c] = (a.mieExtinction * mie + haze) * a.mieSpectral[c];
		}
		s.fogExtinction = a.fogExtinction * layer;
		s.fogScattering = s.fogExtinction * a.fogScatteringAlbedo;
		return s;
	}
}

// -------------------------------------------------------
