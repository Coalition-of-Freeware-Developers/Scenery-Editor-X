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
 * surface_weather.cpp
 * -------------------------------------------------------
 * Created: 20/10/2026
 * -------------------------------------------------------
 */
#include "surface_weather.h"
#include <algorithm>
#include <cmath>

// -------------------------------------------------------

namespace SceneryEditorX::Environment
{
	namespace
	{
		constexpr float LEVEL_STEPS = 4.0f;		// 1, 2, 3 -> 0.25, 0.5, 0.75

		float Level(const int stepInGroup)
		{
			return static_cast<float>(stepInGroup) / LEVEL_STEPS;
		}

		/* @brief First-order approach of value toward target with a time constant (exact for a constant target). */
		float Approach(const float value, const float target, const float dt, const float timeConstant)
		{
			if (timeConstant <= 0.0f)
				return target;
			return target + (value - target) * std::exp(-dt / timeConstant);
		}
	}

	SurfaceWeather DecodeRunwayFriction(int friction)
	{
		friction = std::clamp(friction, 0, 15);
		SurfaceWeather surface;
		if (friction == 0)
			return surface;
		const int group = (friction - 1) / 3;		// 0 wet, 1 puddly, 2 snowy, 3 icy, 4 snowy and icy
		const float level = Level((friction - 1) % 3 + 1);
		switch (group)
		{
			case 0: surface.wetness = level; break;
			case 1: surface.wetness = level; surface.puddles = level; break;
			case 2: surface.snow = level; surface.ice = level * (2.0f / 3.0f); break;
			case 3: surface.ice = level; break;
			default: surface.snow = level; surface.ice = level; break;
		}
		return surface;
	}

	int EncodeRunwayFriction(const SurfaceWeather &surface)
	{
		const auto step = [](const float value) { return std::clamp(static_cast<int>(std::lround(value * LEVEL_STEPS)), 0, 3); };
		const int snow = step(surface.snow);
		const int ice = step(surface.ice);
		// Snowy ground carries some ice of its own (DecodeRunwayFriction): only ice beyond that makes it snowy and icy
		if (snow > 0 && ice > 0 && surface.ice > surface.snow * (2.0f / 3.0f) + 0.01f)
			return 12 + std::max(snow, ice);
		if (snow > 0)
			return 6 + snow;
		if (ice > 0)
			return 9 + ice;
		if (const int puddles = step(surface.puddles); puddles > 0)
			return 3 + puddles;
		if (const int wet = step(surface.wetness); wet > 0)
			return wet;
		return 0;
	}

	float WeatherThresholdCurve::Evaluate(const float value) const
	{
		if (value <= amount.front())
			return threshold.front();
		for (size_t i = 1; i < amount.size(); ++i)
		{
			if (value <= amount[i])
			{
				const float span = amount[i] - amount[i - 1];
				const float t = span > 0.0f ? (value - amount[i - 1]) / span : 1.0f;
				return threshold[i - 1] + (threshold[i] - threshold[i - 1]) * t;
			}
		}
		return threshold.back();
	}

	const char *GetRunwayFrictionName(const int friction)
	{
		static constexpr const char *names[16] = {
			"dry",
			"wet (light)", "wet (medium)", "wet (heavy)",
			"puddly (light)", "puddly (medium)", "puddly (heavy)",
			"snowy (light)", "snowy (medium)", "snowy (heavy)",
			"icy (light)", "icy (medium)", "icy (heavy)",
			"snowy and icy (light)", "snowy and icy (medium)", "snowy and icy (heavy)",
		};
		return names[std::clamp(friction, 0, 15)];
	}

	const SurfaceWeather &SurfaceWeatherIntegrator::Step(const float dtSeconds, const float rainPercent, const float temperatureC)
	{
		const float dt = std::max(dtSeconds, 0.0f);
		const float rain = std::clamp(rainPercent, 0.0f, 1.0f);
		if (rain > 0.0f)
		{
			// Rain soaks toward its own intensity; heavier rain soaks faster
			const float timeConstant = WET_TIME_CONSTANT_S / std::max(rain, 0.05f);
			m_State.wetness = std::max(m_State.wetness, Approach(m_State.wetness, rain, dt, timeConstant));
			// Puddles form once the ground is soaked
			const float puddleTarget = std::clamp((m_State.wetness - 0.66f) / 0.34f, 0.0f, 1.0f) * rain;
			m_State.puddles = std::max(m_State.puddles, Approach(m_State.puddles, puddleTarget, dt, timeConstant * PUDDLE_LAG));
		}
		else if (temperatureC > 0.0f)
		{
			// Drying; puddles go first
			m_State.puddles = Approach(m_State.puddles, 0.0f, dt, DRY_TIME_CONSTANT_S / PUDDLE_LAG);
			m_State.wetness = Approach(m_State.wetness, 0.0f, dt, DRY_TIME_CONSTANT_S);
		}
		m_State.wetness = std::clamp(m_State.wetness, 0.0f, 1.0f);
		m_State.puddles = std::clamp(m_State.puddles, 0.0f, 1.0f);
		return m_State;
	}
}

// -------------------------------------------------------
