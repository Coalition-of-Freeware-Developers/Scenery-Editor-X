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

		// What the clear layers already give at the ground (green, about 550 nm), the haze the rest
		const double ground = std::max(groundElevationM, 0.0);
		const double rayleigh = atmosphere.rayleighScattering[1] * std::exp(-ground / atmosphere.rayleighScaleHeight);
		const double mie = atmosphere.mieExtinction * std::exp(-ground / atmosphere.mieScaleHeight);
		out.hazeExtinction = std::max(VisibilityExtinction(visibilityM, atmosphere.visibilityContrast) - rayleigh - mie * atmosphere.mieSpectral[1], 0.0);
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
		return atmosphere.hazeBandM > 0.0 ? std::max(0.0, 1.0 - above / atmosphere.hazeBandM) : 0.0;
	}

	Rgb MediumSample::Extinction() const
	{
		return { rayleighScattering[0] + mieExtinction[0] + ozoneAbsorption[0], rayleighScattering[1] + mieExtinction[1] + ozoneAbsorption[1],
				 rayleighScattering[2] + mieExtinction[2] + ozoneAbsorption[2] };
	}

	Rgb MediumSample::Scattering() const
	{
		return { rayleighScattering[0] + mieScattering[0], rayleighScattering[1] + mieScattering[1], rayleighScattering[2] + mieScattering[2] };
	}

	MediumSample SampleMedium(const AtmosphereParams &a, const double altitudeMslM)
	{
		const double h = std::max(altitudeMslM, 0.0);
		const double rayleigh = std::exp(-h / a.rayleighScaleHeight);
		const double mie = std::exp(-h / a.mieScaleHeight);
		const double ozone = std::max(0.0, 1.0 - std::abs(h - a.ozoneCenter) / a.ozoneHalfWidth);
		const double haze = a.hazeExtinction * HazeProfile(a, h);

		MediumSample s;
		for (int c = 0; c < 3; ++c)
		{
			s.rayleighScattering[c] = a.rayleighScattering[c] * rayleigh;
			s.ozoneAbsorption[c] = a.ozoneAbsorption[c] * ozone;
			s.mieScattering[c] = a.mieScattering * mie * a.mieSpectral[c] + haze * a.hazeScatteringAlbedo;
			s.mieExtinction[c] = a.mieExtinction * mie * a.mieSpectral[c] + haze;
		}
		return s;
	}
}

// -------------------------------------------------------
