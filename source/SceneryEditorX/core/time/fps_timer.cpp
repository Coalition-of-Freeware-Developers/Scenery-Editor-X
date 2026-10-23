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
 * fps_timer.cpp
 * -------------------------------------------------------
 * Created: 28/02/2026
 * -------------------------------------------------------
 */
// ReSharper disable IdentifierTypo
// ReSharper disable CppInconsistentNaming
#include "fps_timer.h"
#include "SceneryEditorX/core/window/monitor_data.h"

// -------------------------------------------------------

namespace SceneryEditorX
{

#pragma region Static FPSTimer Properties

		// accumulation
		constexpr uint32_t FRAMES_TO_ACCUMULATE = 15;
		constexpr double WEIGHT_DELTA           = 1.0 / static_cast<float>(FRAMES_TO_ACCUMULATE);

		// frame time
		static double s_Time_ms					= 0.0f;
		static double s_DeltaT_ms				= 0.0f;
		static double s_DeltaT_Smoothed_ms		= 0.0f;

		// fps
		static float s_FPS_Min          = 30.0f;
		static float s_FPS_Max          = 10000.0f;
		static float s_FPS_Limit        = s_FPS_Min;
		static float s_FPS_Prevlimit	= s_FPS_Limit;

		static std::chrono::steady_clock::time_point s_last_TickTime;

#pragma endregion

	void FPSTimer::Init()
	{
		s_FPS_Limit = static_cast<float>(MonitorData::GetRefreshRate());
		s_last_TickTime = std::chrono::steady_clock::now();
	}

	void FPSTimer::PostTick()
	{
		// if this is not the first tick, we calculate the delta time
		if (s_last_TickTime.time_since_epoch() != std::chrono::steady_clock::duration::zero())
		{
			s_DeltaT_ms = static_cast<double>(
				std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - s_last_TickTime)
					.count());
		}

		// fps limit
		double target_ms = 1000.0 / s_FPS_Limit;
		while (s_DeltaT_ms < target_ms)
		{
			s_DeltaT_ms =
				std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - s_last_TickTime)
					.count();
		}

		// compute delta time based timings
		s_DeltaT_Smoothed_ms = s_DeltaT_Smoothed_ms * (1.0 - WEIGHT_DELTA) + s_DeltaT_ms * WEIGHT_DELTA;
		s_Time_ms += s_DeltaT_ms;

		// end
		s_last_TickTime = std::chrono::steady_clock::now();
	}

	float FPSTimer::GetFpsLimit() { return s_FPS_Limit; }

	FrameLimits FPSTimer::GetFpsLimitType()
	{
		if (s_FPS_Limit == MonitorData::GetRefreshRate())
		{
			return FrameLimits::FixedToMonitor;
		}

		if (s_FPS_Limit == s_FPS_Max)
		{
			return FrameLimits::Unlocked;
		}

		return FrameLimits::Fixed;
	}

	void FPSTimer::OnVSyncToggled(const bool enabled)
	{
		if (enabled)
		{
			s_FPS_Prevlimit = s_FPS_Limit;
			SetFpsLimit(static_cast<float>(MonitorData::GetRefreshRate()));
		}
		else
		{
			SetFpsLimit(s_FPS_Prevlimit);
		}
	}

	void FPSTimer::SetFpsLimit(float fpsIn)
	{
		if (fpsIn < 0.0f) // negative -> match monitor's refresh rate
		{
			fpsIn = MonitorData::GetRefreshRate();
		}

		// clamp to a minimum of 10 FPS to avoid unresponsiveness
		fpsIn = std::ranges::clamp(fpsIn, s_FPS_Min, s_FPS_Max);

		if (s_FPS_Limit == fpsIn)
		{
			return;
		}

		s_FPS_Limit = fpsIn;
		SEDX_CORE_INFO("Set to %.2f FPS", s_FPS_Limit);
	}

	double FPSTimer::GetTimeMs() { return s_Time_ms; }

	double FPSTimer::GetTimeSec() { return s_Time_ms / 1000.0; }

	double FPSTimer::GetDeltaTimeMs() { return s_DeltaT_ms; }

	double FPSTimer::GetDeltaTimeSec() { return s_DeltaT_ms / 1000.0; }

	double FPSTimer::GetDeltaTimeSmoothedMs() { return s_DeltaT_Smoothed_ms; }

	double FPSTimer::GetDeltaTimeSmoothedSec() { return s_DeltaT_Smoothed_ms / 1000.0; }

} // namespace SceneryEditorX

// -------------------------------------------------------
