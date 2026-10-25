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
 * surface_weather.h
 * -------------------------------------------------------
 * Created: 20/10/2026
 * -------------------------------------------------------
 */
#pragma once
#include <cstdint>

// -------------------------------------------------------

namespace SceneryEditorX::Environment
{
	/* @brief How wet / snowy / icy the ground is, each 0-1: what the weather surface pass reads. */
	struct SurfaceWeather
	{
		float wetness = 0.0f;		// Damp to soaked
		float puddles = 0.0f;		// Standing water
		float snow = 0.0f;			// Snow on the ground
		float ice = 0.0f;			// Ice

		bool operator==(const SurfaceWeather &) const = default;
	};

	/**
	 * @brief X-Plane's sim/weather/region/runway_friction states: 0 dry; 1-3 wet; 4-6 puddly; 7-9 snowy; 10-12 icy;
	 * 13-15 snowy and icy. Within a group the level rises in thirds (wet 1 = 0.33, 3 = 1). Puddly ground is soaked,
	 * so it carries full wetness too.
	 */
	SurfaceWeather DecodeRunwayFriction(int friction);

	/* @brief The friction state closest to a surface (0-15), the inverse of DecodeRunwayFriction for its outputs. */
	int EncodeRunwayFriction(const SurfaceWeather &surface);

	/* @brief The X-Plane name of a friction state ("dry", "wet (light)" ... "snowy and icy (heavy)"). */
	const char *GetRunwayFrictionName(int friction);

	/**
	 * @class SurfaceWeatherIntegrator
	 * @brief Lets wetness build and dry over time instead of snapping to the friction state (the ENVIRONMENT panel's
	 * "integrate over time"). X-Plane documents no accumulation rates: the time constants are ASSUMPTIONS (plan TASK-011)
	 * until calibrated: soaking takes 300 s at full rain, drying 1800 s above freezing, puddles lag wetness by 2x.
	 */
	class SurfaceWeatherIntegrator
	{
	public:
		static constexpr float WET_TIME_CONSTANT_S = 300.0f;
		static constexpr float DRY_TIME_CONSTANT_S = 1800.0f;
		static constexpr float PUDDLE_LAG = 2.0f;

		void Reset(const SurfaceWeather &surface) { m_State = surface; }

		/**
		 * @brief Advances by dt toward what the rain asks for. Snow and ice are not accumulated (they follow the
		 * friction state or the snow cover); wetness freezes below 0 C (stops drying).
		 * @param rainPercent 0-1.
		 */
		const SurfaceWeather &Step(float dtSeconds, float rainPercent, float temperatureC);

		[[nodiscard]] const SurfaceWeather &Get() const { return m_State; }

	private:
		SurfaceWeather m_State;
	};
}

// -------------------------------------------------------
