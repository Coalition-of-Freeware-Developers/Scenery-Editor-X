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
	 * metered by a centre-weighted histogram of log2 luminance. As X-Plane 12.4.4 behaves (fitted to the reference
	 * probe's photometric/* readouts): a target EV100 for the sun's elevation (photometric/ev100_mid) that the meter
	 * (ev100_mtr, plus the elevation's ev100_bias) pulls on part of the way, gain_lo below the target and gain_hi above,
	 * clamped to ev100_min..ev100_max. Standard library only: exposure_adapt.slang does the same on the GPU, these are
	 * the reference.
	 */
	struct ExposureSettings
	{
		bool automatic = true;				// false: manualEv100 (photometric/ev100, the parity scenarios' ev100_lock)
		float manualEv100 = 12.0f;
		float evMin = 3.5f;					// photometric/ev100_min
		float evMax = 14.8f;				// photometric/ev100_max
		float meterK = 12.5f;				// photometric/K
		float gainLow = 0.5f;				// autoexposure/gain_lo: share of a darker reading (than the target) taken
		float gainHigh = 0.4f;				// autoexposure/gain_hi: share of a brighter one
		float speedUp = 3.0f;				// exposure/speed_up (EV/s toward a brighter scene's higher EV)
		float speedDown = 5.0f;				// exposure/speed_down
		float trimLow = 0.5f;				// SEDX metering: the darkest half of the histogram doesn't meter
		float trimHigh = 0.98f;				// nor the brightest 2% (reads within 0.3 EV of X-Plane's ev100_mtr)
		float centerRadius = 0.33f;			// _exp_radius: pixels within this many screen heights of the centre weigh 4x
		float compensation = 0.0f;			// SEDX exposure compensation (EV, positive brightens)

		/**
		 * photometric/ev100_mid by the sun's apparent elevation (deg), linear between the points and flat beyond them.
		 * X-Plane 12.4.4 readouts at KSEA in July (and the season scenarios); it leaves its 5.0 floor somewhere between
		 * -11.8 and -5.8 deg: -10 is an ASSUMPTION.
		 */
		std::array<float, 10> midElevation = { -10.0f, -5.84f, -1.93f, 1.92f, 8.12f, 20.18f, 40.19f, 58.15f, 77.66f, 90.0f };
		std::array<float, 10> midEv = { 5.0f, 7.76f, 10.425f, 12.754f, 13.848f, 14.5f, 14.62f, 14.758f, 14.911f, 15.0f };

		/**
		 * photometric/ev100_bias by the sun's elevation, added to the metered EV100 (positive darkens). X-Plane 12.4.4
		 * readouts in clear air, runway and aerial views averaged; X-Plane also moves it with the view's contrast (fog,
		 * haze), which this leaves out.
		 */
		std::array<float, 9> biasElevation = { -12.0f, -5.84f, -1.93f, 1.92f, 8.12f, 20.18f, 40.19f, 58.15f, 77.66f };
		std::array<float, 9> biasEv = { 2.59f, 2.115f, -1.10f, -1.695f, -0.585f, -0.365f, -0.34f, -0.32f, -0.21f };
	};

	inline constexpr int EXPOSURE_HISTOGRAM_BINS = 256;

	/* @brief The floats of the GPU exposure state (Renderer_Buffer::ExposureState; EXPOSURE_STATE_* in resources.slang). */
	enum ExposureStateIndex : int
	{
		EXPOSURE_STATE_EV = 0,			// EV100 the frame is exposed at (adapted)
		EXPOSURE_STATE_METERED = 1,		// Metered EV100 (ev100_mtr)
		EXPOSURE_STATE_TARGET = 2,		// EV100 the adaptation moves toward
		EXPOSURE_STATE_MEAN_LOG2 = 3,	// Mean log2 luminance (nits) between the trims
		EXPOSURE_STATE_VALID = 4,		// 1 once the first frame has been metered
		EXPOSURE_STATE_RAW = 5,			// Metered EV100 (same as METERED since the 12.4.4 model)
		EXPOSURE_STATE_FLOATS = 8
	};
	inline constexpr float EXPOSURE_LOG2_MIN = -2.0f;	// Histogram range: log2 luminance (nits)
	inline constexpr float EXPOSURE_LOG2_MAX = 18.0f;

	/* @brief EV100 a reflected-light meter reads for an average luminance (nits): log2(L x 100 / K). */
	float EvFromLuminance(float nits, float meterK = 12.5f);

	/**
	 * @brief What the frame is multiplied by at an exposure EV100: the luminance a meter reads as that EV100 becomes
	 * middle grey, 0.18 x 100 / (K x 2^EV) (ISO 2720 meter, standard-output-sensitivity exposure). X-Plane 12.4.4's
	 * references sit 0.8 EV brighter than the saturation-based 1 / (1.2 x 2^EV) the first phases used.
	 */
	float ExposureScaleFromEv(float ev100, float meterK = 12.5f);

	/* @brief photometric/ev100_mid for the sun's apparent elevation (deg). */
	float GetExposureMid(const ExposureSettings &settings, float sunElevationDeg);

	/* @brief photometric/ev100_bias for the sun's apparent elevation (deg). */
	float GetExposureBias(const ExposureSettings &settings, float sunElevationDeg);

	/**
	 * @brief The EV100 a frame is exposed at for a metered EV100 and the sun's elevation: mid + gain x (metered + bias -
	 * mid), gain_lo below mid and gain_hi above, clamped to the range, then the compensation.
	 */
	float GetTargetEv(const ExposureSettings &settings, float meteredEv100, float sunElevationDeg);

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
