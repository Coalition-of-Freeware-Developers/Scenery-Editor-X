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
 * environment_state.h
 * -------------------------------------------------------
 * Created: 20/10/2026
 * -------------------------------------------------------
 */
#pragma once
#include "season_model.h"
#include "sky_conditions.h"
#include "solar_position.h"
#include "surface_weather.h"
#include <array>
#include <cstdint>
#include <string>

// -------------------------------------------------------

namespace SceneryEditorX::Environment
{
	/* @brief A cloud layer (sim/weather/region/cloud_*). Coverage 0 = no layer. */
	struct CloudLayer
	{
		float baseMslM = 0.0f;
		float topMslM = 0.0f;
		float coverage = 0.0f;			// 0-1
		float type = 0.0f;				// 0 cirrus, 1 stratus, 2 cumulus, 3 cumulonimbus (blends allowed, like X-Plane)

		bool operator==(const CloudLayer &) const = default;
	};

	/* @brief The season: from the date and place, or one the user forces. */
	enum class SeasonOverride : uint8_t
	{
		Auto,
		Spring,
		Summer,
		Fall,
		Winter
	};

	/**
	 * @brief The world the viewport shows: date and time, place, weather, surface and season (plan TASK-010). What X-Plane
	 * keeps in its time, location and sim/weather/region datarefs; one copy per editor, saved with the project.
	 */
	struct EnvironmentState
	{
		// Date and local time (local = UTC + utcOffsetHours)
		int year = 2026;
		int month = 7;
		int day = 15;
		double localTimeHours = 12.0;
		float utcOffsetHours = 0.0f;

		// Place (the open airport's datum by default)
		double latitudeDeg = 47.449889;		// KSEA until an airport opens
		double longitudeDeg = -122.311778;
		float elevationM = 132.0f;
		bool followAirport = true;			// Take the place from the open airport

		// Weather
		float visibilityM = 40000.0f;
		std::array<CloudLayer, 3> clouds = {};
		float rainPercent = 0.0f;			// 0-1
		float temperatureC = 15.0f;
		float windDirectionDegT = 270.0f;	// Wind blows from
		float windSpeedKt = 8.0f;
		float waveAmplitudeM = 0.5f;
		float waveDirectionDegT = 270.0f;

		// Surface
		int runwayFriction = 0;				// 0-15, X-Plane's runway_friction
		float snowCover = 0.0f;				// 0-1
		bool integrateSurface = false;		// Wetness builds and dries over time instead of following the friction state

		// Season
		SeasonOverride season = SeasonOverride::Auto;
		float seasonRatioOverride = -1.0f;	// 0-1, negative = from the date

		bool operator==(const EnvironmentState &) const = default;
	};

	/* @brief A direction in the editor's world frame: +X east, +Y up, +Z south (unit length). */
	struct Direction
	{
		float x = 0.0f;
		float y = 1.0f;
		float z = 0.0f;
	};

	/* @brief What follows from the state: sun and moon, surface, season, sky classes (plan TASK-010). */
	struct EnvironmentDerived
	{
		double julianDay = 0.0;
		double utcHours = 12.0;
		int dayOfYear = 1;

		SunPosition sun;
		MoonPosition moon;
		Direction sunDirection;				// Toward the sun (apparent position)
		Direction moonDirection;

		SurfaceWeather surface;
		SeasonResult season;
		SkyWeights skyWeights{};
		SkyMultipliers sky;

		float daylight = 1.0f;				// 0 night .. 1 day (the sun 6 degrees below .. 8 degrees above the horizon)
		float nightFactor = 0.0f;			// 0 day .. 1 lights fully on (the sun 3 degrees above .. 6 below)
	};

	/* @brief The world-frame direction of an azimuth (true, clockwise from north) and elevation, both in degrees. */
	Direction DirectionFromAzimuth(double azimuthDeg, double elevationDeg);

	/* @brief UTC date and hours for a local date and time (handles the day crossing). */
	void LocalToUtc(const EnvironmentState &state, int &year, int &month, int &day, double &utcHours);

	/**
	 * @brief Derives everything from a state. The surface comes from the friction state unless an integrated one is
	 * given (EnvironmentState::integrateSurface); X-Plane's snow_cover raises the snow level too.
	 * @param observerAltitudeMslM Camera altitude, for the high-altitude / orbit sky classes.
	 * @param table Sky-class multipliers; nullptr blends to 1.
	 */
	EnvironmentDerived Derive(const EnvironmentState &state, float observerAltitudeMslM = 0.0f, const SkyConditionTable *table = nullptr,
		const SurfaceWeather *integratedSurface = nullptr);
}

// -------------------------------------------------------
