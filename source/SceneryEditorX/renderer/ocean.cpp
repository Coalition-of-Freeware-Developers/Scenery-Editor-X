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
 * ocean.cpp
 * -------------------------------------------------------
 * Created: 22/10/2026
 * -------------------------------------------------------
 */
#include "ocean.h"
#include <algorithm>
#include <cmath>

// -------------------------------------------------------

namespace SceneryEditorX
{
	namespace
	{
		float Saturate(const float value)
		{
			return std::clamp(value, 0.0f, 1.0f);
		}

		/* @brief ln of the cos-2s spreading's normalization, Q(s) = 2^(2s - 1) Gamma(s + 1)^2 / (pi Gamma(2s + 1)). */
		double LogSpreadNormalization(const double s)
		{
			return (2.0 * s - 1.0) * std::log(2.0) - std::log(static_cast<double>(OCEAN_PI)) + 2.0 * std::lgamma(s + 1.0) - std::lgamma(2.0 * s + 1.0);
		}
	}

	std::array<OceanCascade, OCEAN_CASCADES> ComputeOceanCascades(const OceanSettings &settings)
	{
		std::array<OceanCascade, OCEAN_CASCADES> cascades;
		const float n = static_cast<float>(OCEAN_FFT_SIZE);
		for (uint32_t c = 0; c < OCEAN_CASCADES; ++c)
		{
			OceanCascade &cascade = cascades[c];
			cascade.sizeM = std::max(settings.cascadeSizeM[c], 0.01f);
			cascade.scale = settings.cascadeScale[c];
			const float nyquist = OCEAN_PI * n / cascade.sizeM;
			cascade.kMax = nyquist;
			if (c == 0)
			{
				cascade.kMin = 0.0f;	// Every wave the largest tile holds (k = 0 itself is left out: no wave)
			}
			else
			{
				// The smaller tile takes over at a few waves across it, never past what the larger one can hold
				const float start = settings.cascadeMinCycles * 2.0f * OCEAN_PI / cascade.sizeM;
				cascade.kMin = std::min(start, cascades[c - 1].kMax);
				cascades[c - 1].kMax = cascade.kMin;
			}
		}
		return cascades;
	}

	OceanSpectrum ComputeOceanSpectrum(const float waveAmplitudeM, const float waveDirectionDegT, const OceanSettings &settings)
	{
		OceanSpectrum spectrum;
		spectrum.significantHeightM = std::max(waveAmplitudeM, 0.0f) * settings.heightFromAmplitude;
		spectrum.spreadBlend = Saturate(settings.spreadBlend);
		spectrum.shortWavesFade = std::max(settings.shortWavesFade, 0.0f);
		spectrum.depthM = std::max(settings.spectrumDepthM, 0.1f);
		spectrum.gamma = std::max(settings.gamma, 1.0f);

		// Travel direction: away from where the waves come from (deg true: 0 north, 90 east). Local frame: x east, z south.
		const float from = waveDirectionDegT * OCEAN_PI / 180.0f;
		spectrum.directionRad = std::atan2(std::cos(from), -std::sin(from));

		const float hs = spectrum.significantHeightM;
		const float heightSpan = std::max(settings.heightHiM - settings.heightLoM, 1e-3f);
		const float t = Saturate((hs - settings.heightLoM) / heightSpan);
		spectrum.fetchM = (settings.fetchLoM + (settings.fetchHiM - settings.fetchLoM) * t) * std::max(settings.fetchMultiplier, 0.0f);
		spectrum.fetchM = std::max(spectrum.fetchM, 10.0f);
		const float swellSpan = std::max(settings.heightSwellM - settings.heightNoSwellM, 1e-3f);
		spectrum.swell = settings.maxSwell * Saturate((hs - settings.heightNoSwellM) / swellSpan);

		// The wind that raises that sea over that fetch, at least the one whose fully developed sea is that high
		const float g = OCEAN_GRAVITY;
		float wind = hs / (0.0016f * std::sqrt(spectrum.fetchM / g));
		wind = std::max(wind, std::sqrt(hs * g / 0.21f));
		spectrum.windSpeed = std::clamp(wind, 0.5f, 40.0f);

		const float u = spectrum.windSpeed;
		spectrum.alpha = 0.076f * std::pow(u * u / (spectrum.fetchM * g), 0.22f);
		spectrum.peakOmega = 22.0f * std::cbrt(g * g / (u * spectrum.fetchM));
		spectrum.peakOmega = std::max(spectrum.peakOmega, 0.855f * g / u);	// No lower than the fully developed sea's peak
		return spectrum;
	}

	float OceanDispersion(const float k, const float depthM)
	{
		return std::sqrt(OCEAN_GRAVITY * k * std::tanh(std::min(k * depthM, 20.0f)));
	}

	float OceanDispersionDerivative(const float k, const float depthM)
	{
		const float kd = std::min(k * depthM, 20.0f);
		const float th = std::tanh(kd);
		const float ch = std::cosh(kd);
		const float omega = std::sqrt(OCEAN_GRAVITY * k * th);
		return omega > 0.0f ? OCEAN_GRAVITY * (th + kd / (ch * ch)) / (2.0f * omega) : 0.0f;
	}

	float JonswapSpectrum(const float omega, const OceanSpectrum &spectrum)
	{
		if (omega <= 0.0f || spectrum.peakOmega <= 0.0f)
			return 0.0f;
		const float g = OCEAN_GRAVITY;
		const float wp = spectrum.peakOmega;
		const float sigma = omega <= wp ? 0.07f : 0.09f;
		const float d = omega - wp;
		const float r = std::exp(-(d * d) / (2.0f * sigma * sigma * wp * wp));
		const float ratio = wp / omega;
		return spectrum.alpha * g * g / std::pow(omega, 5.0f) * std::exp(-1.25f * ratio * ratio * ratio * ratio) * std::pow(spectrum.gamma, r);
	}

	float TmaAttenuation(const float omega, const float depthM)
	{
		const float wh = omega * std::sqrt(depthM / OCEAN_GRAVITY);
		if (wh <= 1.0f)
			return 0.5f * wh * wh;
		if (wh < 2.0f)
			return 1.0f - 0.5f * (2.0f - wh) * (2.0f - wh);
		return 1.0f;
	}

	float OceanDirectionalSpread(const float omega, const float theta, const OceanSpectrum &spectrum)
	{
		const float uniform = 1.0f / (2.0f * OCEAN_PI);
		if (spectrum.spreadBlend <= 0.0f || omega <= 0.0f || spectrum.peakOmega <= 0.0f)
			return uniform;

		// Hasselmann et al. 1980's spread exponent, then Horvath's swell term
		const float wp = spectrum.peakOmega;
		const float ratio = omega / wp;
		float s;
		if (omega <= wp)
			s = 6.97f * std::pow(ratio, 4.06f);
		else
			s = 9.77f * std::pow(ratio, -2.33f - 1.45f * (spectrum.windSpeed * wp / OCEAN_GRAVITY - 1.17f));
		s += 16.0f * std::tanh(wp / omega) * spectrum.swell * spectrum.swell;

		const double logQ = LogSpreadNormalization(s);
		const float c = std::abs(std::cos(0.5f * theta));
		const float directional = c > 0.0f ? static_cast<float>(std::exp(logQ + 2.0 * s * std::log(static_cast<double>(c)))) : 0.0f;
		return uniform + (directional - uniform) * spectrum.spreadBlend;
	}

	float OceanSpectrumEnergy(const float kx, const float kz, const OceanSpectrum &spectrum)
	{
		const float k = std::sqrt(kx * kx + kz * kz);
		if (k <= 1e-6f)
			return 0.0f;
		const float omega = OceanDispersion(k, spectrum.depthM);
		const float theta = std::atan2(kz, kx) - spectrum.directionRad;
		const float fade = std::exp(-k * k * spectrum.shortWavesFade * spectrum.shortWavesFade);
		return JonswapSpectrum(omega, spectrum) * TmaAttenuation(omega, spectrum.depthM) * OceanDirectionalSpread(omega, theta, spectrum) *
			   OceanDispersionDerivative(k, spectrum.depthM) / k * fade;
	}

	float OceanGridWaveNumber(const uint32_t n, const float cascadeSizeM, const uint32_t size)
	{
		const int wrapped = n < size / 2 ? static_cast<int>(n) : static_cast<int>(n) - static_cast<int>(size);
		return 2.0f * OCEAN_PI * static_cast<float>(wrapped) / cascadeSizeM;
	}

	float OceanCascadesSignificantHeight(const OceanSpectrum &spectrum, const std::array<OceanCascade, OCEAN_CASCADES> &cascades, const uint32_t size)
	{
		double variance = 0.0;
		for (const OceanCascade &cascade : cascades)
		{
			const double dk = 2.0 * OCEAN_PI / cascade.sizeM;
			double sum = 0.0;
			for (uint32_t m = 0; m < size; ++m)
			{
				const float kz = OceanGridWaveNumber(m, cascade.sizeM, size);
				for (uint32_t n = 0; n < size; ++n)
				{
					const float kx = OceanGridWaveNumber(n, cascade.sizeM, size);
					const float k = std::sqrt(kx * kx + kz * kz);
					if (k <= 0.0f || k < cascade.kMin || k >= cascade.kMax)
						continue;
					sum += OceanSpectrumEnergy(kx, kz, spectrum);
				}
			}
			variance += sum * dk * dk * cascade.scale * cascade.scale;
		}
		return static_cast<float>(4.0 * std::sqrt(variance));
	}

	void NormalizeOceanSpectrum(OceanSpectrum &spectrum, const std::array<OceanCascade, OCEAN_CASCADES> &cascades, const uint32_t size)
	{
		spectrum.heightScale = 0.0f;
		if (spectrum.significantHeightM <= 0.0f)
			return;
		const float height = OceanCascadesSignificantHeight(spectrum, cascades, size);
		if (height > 1e-9f)
			spectrum.heightScale = spectrum.significantHeightM / height;
	}

	uint32_t OceanBitReverse(uint32_t x, const uint32_t bits)
	{
		uint32_t reversed = 0;
		for (uint32_t i = 0; i < bits; ++i)
		{
			reversed = (reversed << 1) | (x & 1u);
			x >>= 1;
		}
		return reversed;
	}

	void OceanInverseFft(std::vector<std::complex<float>> &data)
	{
		const uint32_t n = static_cast<uint32_t>(data.size());
		uint32_t bits = 0;
		while ((1u << bits) < n)
			++bits;
		if (n < 2 || (1u << bits) != n)
			return;

		std::vector<std::complex<float>> reordered(n);
		for (uint32_t i = 0; i < n; ++i)
			reordered[i] = data[OceanBitReverse(i, bits)];
		data.swap(reordered);

		// One butterfly per thread and stage on the GPU: thread t of n / 2 pairs element (t / half) x 2 half + t % half
		for (uint32_t half = 1; half < n; half *= 2)
		{
			for (uint32_t t = 0; t < n / 2; ++t)
			{
				const uint32_t j = t % half;
				const uint32_t i0 = (t / half) * 2 * half + j;
				const uint32_t i1 = i0 + half;
				const float angle = OCEAN_PI * static_cast<float>(j) / static_cast<float>(half);
				const std::complex<float> w(std::cos(angle), std::sin(angle));
				const std::complex<float> a = data[i0];
				const std::complex<float> b = data[i1] * w;
				data[i0] = a + b;
				data[i1] = a - b;
			}
		}
	}

	float WaterClarityDistance(const float tileAlpha, const OceanSettings &settings)
	{
		return std::max(settings.turbidity / std::max(1.0f - Saturate(tileAlpha), 0.01f), settings.clarityMinM);
	}

	float WaterTransmittance(const float pathM, const float clarityDistanceM, const OceanSettings &settings)
	{
		if (!settings.enableTurbidity)
			return 1.0f;
		return std::exp(settings.turbidityCutoff * std::max(pathM, 0.0f) / std::max(clarityDistanceM, 1e-3f));
	}

	float WaterWaveScale(const float fetchRatio, const float floorDepthM, const OceanSettings &settings)
	{
		const float fetch = settings.useFetch ? settings.pondWaveScale + (1.0f - settings.pondWaveScale) * Saturate(fetchRatio) : 1.0f;
		const float shore = settings.shoreDepthM > 0.0f ? Saturate(floorDepthM / settings.shoreDepthM) : 1.0f;
		return fetch * shore * shore * (3.0f - 2.0f * shore);	// smoothstep
	}

	float WaterTessellationFactor(const float edgeLengthM, const float distanceM, const float pixelAngle, const OceanSettings &settings)
	{
		const float target = std::max(distanceM, 0.1f) * std::max(pixelAngle, 1e-6f) * std::max(settings.tessellationPixels, 1.0f);
		return std::clamp(edgeLengthM / target, 1.0f, std::max(settings.maxTessellation, 1.0f));
	}
}

// -------------------------------------------------------
