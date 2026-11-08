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
#include <array>
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
	 * 13-15 snowy and icy. Within a group the level rises in quarters (light 0.25, medium 0.5, heavy 0.75): X-Plane
	 * 12.4.4 reports rain 0.517 and puddles 0.511 for puddly (medium) (wxr/rain_now, puddles_now; reference probe dumps,
	 * 2026-10-25), so puddly ground is as wet as its puddles, and snowy (heavy) ice 0.4-0.5, so snow carries ice at two
	 * thirds of its level. Light and heavy are ASSUMPTIONS from the medium step.
	 */
	SurfaceWeather DecodeRunwayFriction(int friction);

	/* @brief The friction state closest to a surface (0-15), the inverse of DecodeRunwayFriction for its outputs. */
	int EncodeRunwayFriction(const SurfaceWeather &surface);

	/* @brief The X-Plane name of a friction state ("dry", "wet (light)" ... "snowy and icy (heavy)"). */
	const char *GetRunwayFrictionName(int friction);

	/**
	 * @struct WeatherThresholdCurve
	 * @brief How X-Plane 12 turns an amount of rain, puddles, snow or ice (0-1) into the threshold a surface's
	 * receptivity must pass to show it: a curve through five keyframes (its hack/petr/wxr/kf/<kind>_in_N / _out_N art
	 * controls). A threshold above 1 lets nothing through; a low one lets nearly everything through.
	 */
	struct WeatherThresholdCurve
	{
		std::array<float, 5> amount{};		// Rising
		std::array<float, 5> threshold{};

		/* @brief The threshold for an amount, linear between keyframes, held past the ends. */
		[[nodiscard]] float Evaluate(float value) const;
	};

	/* @brief X-Plane 12.4.4's keyframes (reference probe dumps, 2026-10-25). */
	inline constexpr WeatherThresholdCurve RAIN_THRESHOLDS = { { 0.0f, 0.1f, 0.25f, 0.5f, 1.0f }, { 1.1f, 0.68f, 0.47f, 0.3125f, 0.05f } };
	inline constexpr WeatherThresholdCurve PUDDLE_THRESHOLDS = { { 0.0f, 0.15f, 0.35f, 0.65f, 1.0f }, { 1.1f, 0.7736f, 0.5016f, 0.3384f, 0.2295f } };
	inline constexpr WeatherThresholdCurve SNOW_THRESHOLDS = { { 0.0f, 0.25f, 0.7f, 0.85f, 1.0f }, { 1.25f, 0.35f, 0.12f, 0.0865f, 0.07f } };
	inline constexpr WeatherThresholdCurve ICE_THRESHOLDS = { { 0.0f, 0.1f, 0.25f, 0.5f, 1.0f }, { 2.0f, 0.8267f, 0.556f, 0.3755f, 0.195f } };

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
