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
 * environment_state.cpp
 * -------------------------------------------------------
 * Created: 20/10/2026
 * -------------------------------------------------------
 */
#include "environment_state.h"
#include "parity_scenario.h"
#include <algorithm>
#include <cmath>

// -------------------------------------------------------

namespace SceneryEditorX::Environment
{
	namespace
	{
		constexpr double DEG = 3.14159265358979323846 / 180.0;

		bool IsLeap(const int year) { return (year % 4 == 0 && year % 100 != 0) || year % 400 == 0; }

		int DaysIn(const int year, const int month)
		{
			static constexpr int days[12] = { 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 };
			return days[std::clamp(month, 1, 12) - 1] + (month == 2 && IsLeap(year) ? 1 : 0);
		}

		void AddDays(int &year, int &month, int &day, const int delta)
		{
			day += delta;
			while (day < 1)
			{
				if (--month < 1)
				{
					month = 12;
					--year;
				}
				day += DaysIn(year, month);
			}
			while (day > DaysIn(year, month))
			{
				day -= DaysIn(year, month);
				if (++month > 12)
				{
					month = 1;
					++year;
				}
			}
		}

		float SmoothStep(const float a, const float b, const float x)
		{
			const float t = std::clamp((x - a) / (b - a), 0.0f, 1.0f);
			return t * t * (3.0f - 2.0f * t);
		}
	}

	Direction DirectionFromAzimuth(const double azimuthDeg, const double elevationDeg)
	{
		const double cosElevation = std::cos(elevationDeg * DEG);
		Direction d;
		d.x = static_cast<float>(cosElevation * std::sin(azimuthDeg * DEG));	// East
		d.y = static_cast<float>(std::sin(elevationDeg * DEG));					// Up
		d.z = static_cast<float>(-cosElevation * std::cos(azimuthDeg * DEG));	// South (north is -Z)
		return d;
	}

	void LocalToUtc(const EnvironmentState &state, int &year, int &month, int &day, double &utcHours)
	{
		year = state.year;
		month = std::clamp(state.month, 1, 12);
		day = std::clamp(state.day, 1, DaysIn(year, month));
		utcHours = state.localTimeHours - state.utcOffsetHours;
		const int shift = static_cast<int>(std::floor(utcHours / 24.0));
		utcHours -= shift * 24.0;
		if (shift != 0)
			AddDays(year, month, day, shift);
	}

	EnvironmentDerived Derive(const EnvironmentState &state, const float observerAltitudeMslM, const SkyConditionTable *table, const SurfaceWeather *integratedSurface)
	{
		EnvironmentDerived derived;
		int y = 0, m = 0, d = 0;
		LocalToUtc(state, y, m, d, derived.utcHours);
		derived.julianDay = JulianDay(y, m, d, derived.utcHours);
		derived.dayOfYear = DayOfYear(state.year, std::clamp(state.month, 1, 12), std::clamp(state.day, 1, DaysIn(state.year, std::clamp(state.month, 1, 12))));

		derived.sun = ComputeSun(derived.julianDay, state.latitudeDeg, state.longitudeDeg);
		derived.moon = ComputeMoon(derived.julianDay, state.latitudeDeg, state.longitudeDeg);
		derived.sunDirection = DirectionFromAzimuth(derived.sun.azimuthDeg, derived.sun.apparentElevationDeg);
		derived.moonDirection = DirectionFromAzimuth(derived.moon.azimuthDeg, derived.moon.apparentElevationDeg);

		// Surface: the friction state (or what the integrator built), with the snow cover as a floor for snow
		derived.surface = DecodeRunwayFriction(state.runwayFriction);
		if (state.integrateSurface && integratedSurface)
		{
			derived.surface.wetness = integratedSurface->wetness;
			derived.surface.puddles = integratedSurface->puddles;
		}
		derived.surface.snow = std::max(derived.surface.snow, std::clamp(state.snowCover, 0.0f, 1.0f));

		// Season
		if (state.season == SeasonOverride::Auto)
			derived.season = ComputeSeason(derived.dayOfYear, state.latitudeDeg, nullptr);
		else
		{
			derived.season.season = static_cast<Season>(static_cast<uint8_t>(state.season) - 1);
			derived.season.bits = GetSeasonBit(derived.season.season);
			derived.season.ratio = 0.0f;
		}
		if (state.seasonRatioOverride >= 0.0f)
			derived.season.ratio = std::clamp(state.seasonRatioOverride, 0.0f, 1.0f);
		derived.scenerySeason = derived.season;

		derived.skyWeights = ComputeSkyWeights(state, observerAltitudeMslM, state.elevationM);
		derived.sky = table ? table->Blend(derived.skyWeights) : SkyMultipliers{};

		const float elevation = static_cast<float>(derived.sun.apparentElevationDeg);
		derived.daylight = SmoothStep(-6.0f, 8.0f, elevation);
		derived.nightFactor = std::clamp((3.0f - elevation) / 9.0f, 0.0f, 1.0f);
		return derived;
	}
}

// -------------------------------------------------------
