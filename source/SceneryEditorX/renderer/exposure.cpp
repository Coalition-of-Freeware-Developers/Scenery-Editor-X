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
 * exposure.cpp
 * -------------------------------------------------------
 * Created: 20/10/2026
 * -------------------------------------------------------
 */
#include "exposure.h"
#include <algorithm>
#include <cmath>

// -------------------------------------------------------

namespace SceneryEditorX
{
	float EvFromLuminance(const float nits, const float meterK)
	{
		return std::log2(std::max(nits, 1e-9f) * 100.0f / meterK);
	}

	float ExposureScaleFromEv(const float ev100, const float meterK)
	{
		return 18.0f / (std::max(meterK, 1e-3f) * std::exp2(ev100));
	}

	namespace
	{
		/* @brief Linear between the points, flat beyond them. */
		template <size_t N>
		float Interpolate(const std::array<float, N> &x, const std::array<float, N> &y, const float at)
		{
			if (at <= x.front())
				return y.front();
			for (size_t i = 1; i < N; ++i)
			{
				if (at <= x[i])
				{
					const float t = (at - x[i - 1]) / std::max(x[i] - x[i - 1], 1e-6f);
					return y[i - 1] + (y[i] - y[i - 1]) * t;
				}
			}
			return y.back();
		}
	}

	float GetExposureMid(const ExposureSettings &settings, const float sunElevationDeg)
	{
		return Interpolate(settings.midElevation, settings.midEv, sunElevationDeg);
	}

	float GetExposureBias(const ExposureSettings &settings, const float sunElevationDeg)
	{
		return Interpolate(settings.biasElevation, settings.biasEv, sunElevationDeg);
	}

	float GetTargetEv(const ExposureSettings &settings, const float meteredEv100, const float sunElevationDeg)
	{
		if (!settings.automatic)
			return settings.manualEv100;
		const float mid = GetExposureMid(settings, sunElevationDeg);
		const float offset = meteredEv100 + GetExposureBias(settings, sunElevationDeg) - mid;
		const float ev = mid + (offset < 0.0f ? settings.gainLow : settings.gainHigh) * offset;
		return std::clamp(ev, settings.evMin, settings.evMax) - settings.compensation;
	}

	float AdaptEv(const ExposureSettings &settings, const float currentEv100, const float targetEv100, const float deltaSeconds)
	{
		const float delta = targetEv100 - currentEv100;
		const float dt = std::max(deltaSeconds, 0.0f);
		return currentEv100 + std::clamp(delta, -settings.speedDown * dt, settings.speedUp * dt);
	}

	int GetExposureBin(const float nits)
	{
		if (!(nits > 0.0f))
			return 0;
		const float t = (std::log2(nits) - EXPOSURE_LOG2_MIN) / (EXPOSURE_LOG2_MAX - EXPOSURE_LOG2_MIN);
		return std::clamp(static_cast<int>(t * EXPOSURE_HISTOGRAM_BINS), 0, EXPOSURE_HISTOGRAM_BINS - 1);
	}

	bool MeterHistogram(const std::array<uint32_t, EXPOSURE_HISTOGRAM_BINS> &bins, const float trimLow, const float trimHigh, float &meanLog2)
	{
		double total = 0.0;
		for (const uint32_t count : bins)
			total += count;
		if (total <= 0.0)
			return false;

		// The counts between the two trims, bin by bin (a bin straddling a trim counts in part)
		const double low = total * std::clamp(static_cast<double>(trimLow), 0.0, 1.0);
		const double high = total * std::clamp(static_cast<double>(trimHigh), 0.0, 1.0);
		double cumulative = 0.0;
		double sum = 0.0;
		double weight = 0.0;
		constexpr double binWidth = (EXPOSURE_LOG2_MAX - EXPOSURE_LOG2_MIN) / EXPOSURE_HISTOGRAM_BINS;
		for (int b = 0; b < EXPOSURE_HISTOGRAM_BINS; ++b)
		{
			const double n = bins[static_cast<size_t>(b)];
			const double part = std::max(0.0, std::min(cumulative + n, high) - std::max(cumulative, low));
			sum += part * (EXPOSURE_LOG2_MIN + (b + 0.5) * binWidth);
			weight += part;
			cumulative += n;
		}
		if (weight <= 0.0)
			return false;
		meanLog2 = static_cast<float>(sum / weight);
		return true;
	}
}

// -------------------------------------------------------
