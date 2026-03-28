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
 * fps_timer.h
 * -------------------------------------------------------
 * Created: 28/02/2026
 * -------------------------------------------------------
 */
#pragma once

// -------------------------------------------------------

namespace SceneryEditorX
{
	/**
	 * @enum FrameLimits
	 * @brief Enum class BitMask representing different frame limit types.
	 */
	enum class FrameLimits : uint8_t
	{
		Unlocked		= BIT(0),
		Fixed			= BIT(1),
		FixedToMonitor	= BIT(2)
	};

	/* TODO: Investigate if making this an inheritance from the Timer classes is beneficial. */

	/**
	 * @class FPSTimer
	 * @brief Utility class for managing frame timing and FPS limits. Provides functions to initialize the timer,
	 * update it each frame, and retrieve timing information such as current time, delta time, and smoothed delta time. 
	 * Also handles FPS limit settings and VSync toggling. 
	 */
	class FPSTimer
	{
	public:
		FPSTimer() = default;

		/**
		 * @brief Initialize the FPS timer. Should be called once at application startup to set initial timing values and FPS limit.
		 */
		static void Init();

		/**
		 * @brief Update the FPS timer. Should be called once per frame to update timing information.
		 * @note If this is not the first tick, we calculate the delta time.
		 */
		static void PostTick();

		/**
		 * @brief Get the current FPS limit.
		 * @return The current FPS limit.
		 */
		static float GetFpsLimit();

		/**
		 * @brief Get the current FPS limit type.
		 * @return The current FPS limit type.
		 */
		static FrameLimits GetFpsLimitType();

		/**
		 * @brief Toggle VSync on or off.
		 * @param enabled True to enable VSync, false to disable.
		 */
		static void OnVSyncToggled(const bool enabled);

		/**
		 * @brief Set the FPS limit.
		 * @param fpsIn The desired FPS limit.
		 * @note The FPS limit takes the fpsIn value and clamps it to a minimum of 10 FPS to avoid unresponsiveness.
		 */
		static void SetFpsLimit(float fpsIn);

		/**
		 * @brief Get the current time in milliseconds.
		 * @return The current time in milliseconds.
		 */
		static double GetTimeMs();

		/**
		 * @brief Get the current time in seconds.
		 * @return The current time in seconds.
		 */
		static double GetTimeSec();

		/**
		 * @brief Get the delta time in milliseconds.
		 * @return The delta time in milliseconds.
		 */
		static double GetDeltaTimeMs();

		/**
		 * @brief Get the delta time in seconds.
		 * @return The delta time in seconds.
		 */
		static double GetDeltaTimeSec();

		/**
		 * @brief Get the smoothed delta time in milliseconds.
		 * @return The smoothed delta time in milliseconds.
		 */
		static double GetDeltaTimeSmoothedMs();

		/**
		 * @brief Get the smoothed delta time in seconds.
		 * @return The smoothed delta time in seconds.
		 */
		static double GetDeltaTimeSmoothedSec();
	};

}

// -------------------------------------------------------
