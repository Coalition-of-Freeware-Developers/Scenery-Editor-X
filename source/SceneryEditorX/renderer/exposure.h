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
 * exposure.h
 * -------------------------------------------------------
 * Created: 20/10/2026
 * -------------------------------------------------------
 */
#pragma once
#include <array>
#include <cstdint>

// -------------------------------------------------------

namespace SceneryEditorX
{
	/**
	 * X-Plane 12's camera exposure (plan REQ-006, TASK-038): an EV100 camera with reflected-light meter constant K,
	 * metered by a centre-weighted histogram of log2 luminance and clamped to EV100 5-15, then shifted by an art-directed
	 * bias table. Standard library only: exposure_adapt.slang does the same on the GPU, these are the reference.
	 */
	struct ExposureSettings
	{
		bool automatic = true;				// false: manualEv100 (photometric/ev100, the parity scenarios' ev100_lock)
		float manualEv100 = 12.0f;
		float evMin = 5.0f;					// photometric/ev100_min
		float evMax = 15.0f;				// photometric/ev100_max
		float meterK = 12.5f;				// photometric/K
		float speedUp = 2.0f;				// exposure/speed_up (EV/s toward a brighter scene's higher EV)
		float speedDown = 5.0f;				// exposure/speed_down
		float trimLow = 0.5f;				// autoexposure/trim_lo: the darkest half of the histogram doesn't meter (ASSUMPTION)
		float trimHigh = 0.98f;				// autoexposure/trim_hi: nor the brightest 2% (ASSUMPTION)
		float centerRadius = 0.33f;			// _exp_radius: pixels within this many screen heights of the centre weigh 4x
		float compensation = 0.0f;			// photometric/ev100_bias on top of the table (EV, positive brightens)

		/**
		 * Bias by metered EV100, linear between the points and flat beyond them (positive brightens: night scenes, held
		 * at the EV floor, come back up). ASSUMPTION: the 12.05 community script's table, sign read as compensation.
		 */
		std::array<float, 5> biasEv = { 5.0f, 8.0f, 11.0f, 12.0f, 12.33f };
		std::array<float, 5> bias = { 3.5f, 3.0f, 0.0f, 0.0f, -0.3f };
	};

	inline constexpr int EXPOSURE_HISTOGRAM_BINS = 256;

	/* @brief The floats of the GPU exposure state (Renderer_Buffer::ExposureState; EXPOSURE_STATE_* in resources.slang). */
	enum ExposureStateIndex : int
	{
		EXPOSURE_STATE_EV = 0,			// EV100 the frame is exposed at (adapted, bias applied)
		EXPOSURE_STATE_METERED = 1,		// Metered EV100, clamped to the range (before the bias)
		EXPOSURE_STATE_TARGET = 2,		// EV100 the adaptation moves toward
		EXPOSURE_STATE_MEAN_LOG2 = 3,	// Mean log2 luminance (nits) between the trims
		EXPOSURE_STATE_VALID = 4,		// 1 once the first frame has been metered
		EXPOSURE_STATE_RAW = 5,			// Metered EV100 before the clamp
		EXPOSURE_STATE_FLOATS = 8
	};
	inline constexpr float EXPOSURE_LOG2_MIN = -2.0f;	// Histogram range: log2 luminance (nits)
	inline constexpr float EXPOSURE_LOG2_MAX = 18.0f;

	/* @brief EV100 a reflected-light meter reads for an average luminance (nits): log2(L x 100 / K). */
	float EvFromLuminance(float nits, float meterK = 12.5f);

	/* @brief What the frame is multiplied by at an exposure EV100: 1 / (1.2 x 2^EV) (saturation-based, q = 0.65). */
	float ExposureScaleFromEv(float ev100);

	/* @brief The bias table at a metered EV100. */
	float GetExposureBias(const ExposureSettings &settings, float meteredEv100);

	/* @brief The EV100 a frame is exposed at for a metered EV100 (clamped to the range, then the bias applied). */
	float GetTargetEv(const ExposureSettings &settings, float meteredEv100);

	/* @brief One step toward the target EV100 at the adaptation speeds. */
	float AdaptEv(const ExposureSettings &settings, float currentEv100, float targetEv100, float deltaSeconds);

	/* @brief The histogram bin of a luminance (nits; 0 and below go in bin 0). */
	int GetExposureBin(float nits);

	/**
	 * @brief Mean log2 luminance of the histogram's counts between the trims (fractions of the total count), using
	 * each bin's centre. Returns false for an empty histogram.
	 */
	bool MeterHistogram(const std::array<uint32_t, EXPOSURE_HISTOGRAM_BINS> &bins, float trimLow, float trimHigh, float &meanLog2);
}

// -------------------------------------------------------
