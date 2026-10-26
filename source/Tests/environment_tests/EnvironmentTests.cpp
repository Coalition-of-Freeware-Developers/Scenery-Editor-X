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
 * EnvironmentTests.cpp
 * -------------------------------------------------------
 * Created: 20/10/2026
 * -------------------------------------------------------
 * The world environment model (source/SceneryEditorX/environment): sun and moon positions, runway friction states and the surface
 * integrator, seasons, sky-condition weights, directions in the world frame and the World object.
 * -------------------------------------------------------
 */
#include <catch2/catch_all.hpp>
#include <SceneryEditorX/environment/atmosphere_params.h>
#include <SceneryEditorX/environment/environment.h>
#include <SceneryEditorX/environment/parity_scenario.h>
#include <SceneryEditorX/environment/sky_light.h>
#include <SceneryEditorX/renderer/cacao.h>
#include <SceneryEditorX/renderer/csm.h>
#include <SceneryEditorX/renderer/env_probes.h>
#include <SceneryEditorX/renderer/exposure.h>
#include <SceneryEditorX/renderer/ocean.h>
#include <SceneryEditorX/renderer/renderer_features.h>
#include <SceneryEditorX/renderer/renderer_quality.h>
#include <SceneryEditorX/renderer/ssr.h>
#include <algorithm>
#include <cfloat>
#include <cmath>
#include <cstddef>
#include <numbers>
#include <numeric>
#include <set>
#include <string>

using namespace SceneryEditorX::Environment;
using Catch::Approx;

namespace
{
	constexpr double DEG = 3.14159265358979323846 / 180.0;

	/**
	 * The Astronomical Almanac's low-precision solar coordinates (good to 0.01 degrees, 1950-2050): an independent
	 * formulation of the sun's position, to check the NOAA equations against.
	 */
	void AlmanacSun(const double jd, const double latitude, const double longitude, double &elevation, double &azimuth)
	{
		const double n = jd - 2451545.0;
		const double l = std::fmod(280.460 + 0.9856474 * n, 360.0);
		const double g = std::fmod(357.528 + 0.9856003 * n, 360.0) * DEG;
		const double lambda = (l + 1.915 * std::sin(g) + 0.020 * std::sin(2.0 * g)) * DEG;
		const double epsilon = (23.439 - 0.0000004 * n) * DEG;
		const double alpha = std::atan2(std::cos(epsilon) * std::sin(lambda), std::cos(lambda));
		const double delta = std::asin(std::sin(epsilon) * std::sin(lambda));
		const double gmstHours = std::fmod(18.697374558 + 24.06570982441908 * n, 24.0);
		const double hourAngle = gmstHours * 15.0 * DEG + longitude * DEG - alpha;
		const double phi = latitude * DEG;
		elevation = std::asin(std::sin(phi) * std::sin(delta) + std::cos(phi) * std::cos(delta) * std::cos(hourAngle)) / DEG;
		azimuth = std::atan2(-std::sin(hourAngle), std::tan(delta) * std::cos(phi) - std::sin(phi) * std::cos(hourAngle)) / DEG;
		azimuth = std::fmod(azimuth + 360.0, 360.0);
	}

	double AngleDifference(const double a, const double b)
	{
		return std::abs(std::remainder(a - b, 360.0));
	}
}

TEST_CASE("Environment: Julian day and sidereal time (Meeus examples)", "[environment][solar]")
{
	CHECK(JulianDay(1957, 10, 4, 0.81 * 24.0) == Approx(2436116.31).margin(1e-6));	// Example 7.a
	CHECK(JulianDay(2000, 1, 1, 12.0) == Approx(2451545.0).margin(1e-9));
	CHECK(JulianDay(1987, 6, 19, 12.0) == Approx(2446966.0).margin(1e-9));			// Example 7 list
	CHECK(GreenwichSiderealDeg(2446895.5) == Approx(197.693195).margin(1e-5));		// Example 12.a
	CHECK(DeltaTSeconds(2026.0) == Approx(75.1).margin(1.0));							// Polynomial value (actual ~69 s)
}

TEST_CASE("Environment: the sun matches an independent almanac within 0.05 deg", "[environment][solar]")
{
	struct Date { int year, month, day; };
	const Date dates[] = { { 2026, 3, 20 }, { 2026, 6, 21 }, { 2026, 9, 23 }, { 2026, 12, 21 } };
	const double latitudes[] = { 0.0, 47.45, -33.9, 64.8 };
	const double utcHours[] = { 6.0, 12.0, 18.0 };
	int checked = 0;
	for (const Date &date : dates)
	{
		for (const double latitude : latitudes)
		{
			for (const double utc : utcHours)
			{
				const double longitude = -122.31;
				const double jd = JulianDay(date.year, date.month, date.day, utc);
				const SunPosition sun = ComputeSun(jd, latitude, longitude);
				double elevation = 0.0, azimuth = 0.0;
				AlmanacSun(jd, latitude, longitude, elevation, azimuth);
				INFO(date.year << "-" << date.month << "-" << date.day << " " << utc << " UTC, lat " << latitude);
				CHECK(std::abs(sun.elevationDeg - elevation) < 0.05);
				if (std::abs(elevation) < 85.0)
					CHECK(AngleDifference(sun.azimuthDeg, azimuth) < 0.1);
				++checked;
			}
		}
	}
	CHECK(checked == 48);
}

TEST_CASE("Environment: sun sanity at solstices and equinoxes", "[environment][solar]")
{
	// June solstice: declination near +23.44, local noon elevation 90 - lat + 23.44 at 47.45 N
	const SunPosition june = ComputeSun(JulianDay(2026, 6, 21, 12.0), 47.45, 0.0);
	CHECK(june.declinationDeg == Approx(23.44).margin(0.02));
	CHECK(june.elevationDeg == Approx(90.0 - 47.45 + 23.44).margin(0.2));
	CHECK(AngleDifference(june.azimuthDeg, 180.0) < 1.5);	// South at noon (equation of time keeps it a little off)

	// March equinox at the equator, 12:00 UTC on the prime meridian: nearly overhead (the equation of time, -7.5 min,
	// leaves the sun about 1.9 deg east of the meridian)
	const SunPosition march = ComputeSun(JulianDay(2026, 3, 20, 12.0), 0.0, 0.0);
	CHECK(std::abs(march.declinationDeg) < 0.4);
	CHECK(march.elevationDeg == Approx(88.1).margin(0.2));

	// Refraction lifts the sun about 0.57 deg at the horizon, nothing near the zenith
	CHECK(RefractionDeg(0.0) == Approx(0.482).margin(0.01));	// 1735" at the horizon (NOAA)
	CHECK(RefractionDeg(89.0) == 0.0);
	const SunPosition dawn = ComputeSun(JulianDay(2026, 7, 15, 12.5), 47.45, -122.31);
	CHECK(dawn.apparentElevationDeg >= dawn.elevationDeg);
	CHECK(june.distanceAu == Approx(1.0163).margin(0.0005));	// Aphelion is early July
}

TEST_CASE("Environment: the moon matches Meeus examples 47.a and 48.a", "[environment][moon]")
{
	// 1992 April 12, 0h TD
	const double jdUT = 2448724.5 - DeltaTSeconds(1992.28) / 86400.0;
	const MoonPosition moon = ComputeMoon(jdUT, 0.0, 0.0);
	CHECK(moon.eclipticLongitudeDeg == Approx(133.162655).margin(0.02));
	CHECK(moon.eclipticLatitudeDeg == Approx(-3.229126).margin(0.01));
	CHECK(moon.distanceKm == Approx(368409.7).margin(50.0));
	CHECK(moon.rightAscensionDeg == Approx(134.688470).margin(0.02));
	CHECK(moon.declinationDeg == Approx(13.768368).margin(0.02));
	CHECK(moon.phaseAngleDeg == Approx(69.0756).margin(0.1));
	CHECK(moon.illuminatedFraction == Approx(0.6786).margin(0.002));
	CHECK(moon.waxing);
}

TEST_CASE("Environment: the moon's parallax and phases", "[environment][moon]")
{
	// Parallax lowers the moon by up to about a degree near the horizon (less overhead)
	const double jd = JulianDay(2026, 7, 15, 6.0);
	const MoonPosition seattle = ComputeMoon(jd, 47.45, -122.31);
	CHECK(seattle.elevationDeg > -90.0);
	CHECK(seattle.elevationDeg < 90.0);
	CHECK(seattle.distanceKm > 356000.0);
	CHECK(seattle.distanceKm < 407000.0);

	// New moon 2026-01-18 (dark), full moon 2026-01-03 (lit)
	CHECK(ComputeMoon(JulianDay(2026, 1, 18, 19.9), 0.0, 0.0).illuminatedFraction < 0.02);
	CHECK(ComputeMoon(JulianDay(2026, 1, 3, 10.1), 0.0, 0.0).illuminatedFraction > 0.98);
}

TEST_CASE("Environment: runway friction states", "[environment][surface]")
{
	// Levels in quarters: X-Plane 12.4.4 reports rain 0.517 and puddles 0.511 for puddly (medium)
	CHECK(DecodeRunwayFriction(0) == SurfaceWeather{});
	CHECK(DecodeRunwayFriction(1).wetness == Approx(0.25f));
	CHECK(DecodeRunwayFriction(2).wetness == Approx(0.5f));
	CHECK(DecodeRunwayFriction(3).wetness == Approx(0.75f));
	for (int f = 4; f <= 6; ++f)
	{
		const SurfaceWeather s = DecodeRunwayFriction(f);
		CHECK(s.wetness == Approx((f - 3) / 4.0f));
		CHECK(s.puddles == Approx((f - 3) / 4.0f));
	}
	for (int f = 7; f <= 9; ++f)
	{
		const SurfaceWeather s = DecodeRunwayFriction(f);
		CHECK(s.snow == Approx((f - 6) / 4.0f));
		CHECK(s.ice == Approx(s.snow * 2.0f / 3.0f));	// snowy (heavy): ice 0.4-0.5 in X-Plane
	}
	for (int f = 10; f <= 12; ++f)
		CHECK(DecodeRunwayFriction(f).ice == Approx((f - 9) / 4.0f));
	for (int f = 13; f <= 15; ++f)
	{
		const SurfaceWeather s = DecodeRunwayFriction(f);
		CHECK(s.snow == Approx((f - 12) / 4.0f));
		CHECK(s.ice == Approx((f - 12) / 4.0f));
		CHECK(s.wetness == 0.0f);
	}
	CHECK(DecodeRunwayFriction(-3) == SurfaceWeather{});
	CHECK(DecodeRunwayFriction(40) == DecodeRunwayFriction(15));

	// Every state survives encoding back
	for (int f = 0; f <= 15; ++f)
	{
		INFO(f);
		CHECK(EncodeRunwayFriction(DecodeRunwayFriction(f)) == f);
	}
	CHECK(std::string(GetRunwayFrictionName(5)) == "puddly (medium)");
}

TEST_CASE("Environment: weather thresholds follow X-Plane's keyframes", "[environment][surface]")
{
	// None of a kind: a threshold no receptivity passes; all of it: nearly everything passes
	CHECK(SNOW_THRESHOLDS.Evaluate(0.0f) == Approx(1.25f));
	CHECK(SNOW_THRESHOLDS.Evaluate(1.0f) == Approx(0.07f));
	CHECK(RAIN_THRESHOLDS.Evaluate(0.0f) > 1.0f);
	CHECK(ICE_THRESHOLDS.Evaluate(0.0f) > 1.0f);
	CHECK(PUDDLE_THRESHOLDS.Evaluate(0.0f) > 1.0f);

	// Linear between keyframes, held past the ends
	CHECK(SNOW_THRESHOLDS.Evaluate(0.125f) == Approx(0.8f));
	CHECK(RAIN_THRESHOLDS.Evaluate(0.75f) == Approx((0.3125f + 0.05f) * 0.5f));
	CHECK(PUDDLE_THRESHOLDS.Evaluate(-1.0f) == Approx(1.1f));
	CHECK(ICE_THRESHOLDS.Evaluate(3.0f) == Approx(0.195f));

	// More of a kind never raises its threshold
	for (const WeatherThresholdCurve *curve : { &RAIN_THRESHOLDS, &PUDDLE_THRESHOLDS, &SNOW_THRESHOLDS, &ICE_THRESHOLDS })
	{
		for (int i = 1; i <= 100; ++i)
			CHECK(curve->Evaluate(i / 100.0f) <= curve->Evaluate((i - 1) / 100.0f));
	}
}

TEST_CASE("Environment: wetness builds in rain and dries afterwards", "[environment][surface]")
{
	SurfaceWeatherIntegrator integrator;
	for (int i = 0; i < 600; ++i)
		integrator.Step(1.0f, 1.0f, 12.0f);	// 10 minutes of heavy rain
	CHECK(integrator.Get().wetness > 0.85f);
	CHECK(integrator.Get().puddles > 0.1f);

	const SurfaceWeather soaked = integrator.Get();
	for (int i = 0; i < 1800; ++i)
		integrator.Step(1.0f, 0.0f, 12.0f);	// 30 minutes dry
	CHECK(integrator.Get().wetness < soaked.wetness * 0.5f);
	CHECK(integrator.Get().puddles < soaked.puddles);

	// Below freezing nothing dries
	integrator.Reset({ 1.0f, 0.5f, 0.0f, 0.0f });
	integrator.Step(3600.0f, 0.0f, -5.0f);
	CHECK(integrator.Get().wetness == 1.0f);
}

TEST_CASE("Environment: seasons from date and latitude", "[environment][season]")
{
	// Northern hemisphere
	CHECK(ComputeSeason(15, 47.45).season == Season::Winter);
	CHECK(ComputeSeason(105, 47.45).season == Season::Spring);
	CHECK(ComputeSeason(196, 47.45).season == Season::Summer);
	CHECK(ComputeSeason(288, 47.45).season == Season::Fall);
	CHECK(ComputeSeason(350, 47.45).season == Season::Winter);
	CHECK(ComputeSeason(196, 47.45).bits == 2);

	// The ratio rises over the last 15 days only
	CHECK(ComputeSeason(130, 47.45).ratio == 0.0f);
	CHECK(ComputeSeason(151, 47.45).ratio == Approx(1.0f));		// Last day of spring
	CHECK(ComputeSeason(144, 47.45).ratio == Approx(8.0f / 15.0f));
	CHECK(ComputeSeason(152, 47.45).ratio == 0.0f);				// First day of summer

	// Southern hemisphere: half a year apart (January is summer at YSSY)
	CHECK(ComputeSeason(15, -33.95).season == Season::Summer);
	CHECK(ComputeSeason(196, -33.95).season == Season::Winter);

	// Tropics: always summer
	CHECK(ComputeSeason(15, 10.0).season == Season::Summer);
	CHECK(ComputeSeason(196, -20.0).ratio == 0.0f);

	// Leap day 366
	CHECK(ComputeSeason(366, 47.45).season == Season::Winter);

	Season parsed;
	CHECK(ParseSeasonCode("fal", parsed));
	CHECK(parsed == Season::Fall);
	CHECK_FALSE(ParseSeasonCode("autumn", parsed));
	CHECK(NextSeason(Season::Winter) == Season::Spring);
}

TEST_CASE("Environment: seasons from DSF ranges", "[environment][season]")
{
	DsfSeasonRanges ranges;
	// Spring 80-150, summer 151-250, fall 260-320, winter 330-70 (wraps the year)
	ranges.start = { 80, 151, 260, 330 };
	ranges.end = { 150, 250, 320, 70 };
	ranges.valid = { true, true, true, true };

	CHECK(ComputeSeason(100, 47.0, &ranges).season == Season::Spring);
	CHECK(ComputeSeason(200, 47.0, &ranges).season == Season::Summer);
	CHECK(ComputeSeason(300, 47.0, &ranges).season == Season::Fall);
	CHECK(ComputeSeason(10, 47.0, &ranges).season == Season::Winter);
	CHECK(ComputeSeason(350, 47.0, &ranges).season == Season::Winter);

	// The gap 251-259 belongs to summer and moves toward fall across it
	const SeasonResult gap = ComputeSeason(255, 47.0, &ranges);
	CHECK(gap.season == Season::Summer);
	CHECK(gap.ratio == Approx(5.0f / 10.0f));
	CHECK(ComputeSeason(75, 47.0, &ranges).season == Season::Winter);	// Gap 71-79 after winter
	CHECK(ComputeSeason(75, 47.0, &ranges).ratio > 0.0f);

	// Spring ends the day before summer begins: the ratio rises inside spring's last days
	CHECK(ComputeSeason(150, 47.0, &ranges).ratio == Approx(1.0f));
	CHECK(ComputeSeason(120, 47.0, &ranges).ratio == 0.0f);

	// A tile without spring (2-state terrain): winter runs into summer
	DsfSeasonRanges twoState;
	twoState.start = { 0, 120, 0, 300 };
	twoState.end = { 0, 299, 0, 119 };
	twoState.valid = { false, true, false, true };
	CHECK(ComputeSeason(60, 47.0, &twoState).season == Season::Winter);
	CHECK(ComputeSeason(200, 47.0, &twoState).season == Season::Summer);
}

TEST_CASE("Environment: sky-condition weights", "[environment][sky]")
{
	EnvironmentState state;
	const auto sum = [](const SkyWeights &w) { return std::accumulate(w.begin(), w.end(), 0.0f); };
	const auto weight = [](const SkyWeights &w, SkyCondition c) { return w[static_cast<size_t>(c)]; };

	state.visibilityM = 80000.0f;
	SkyWeights w = ComputeSkyWeights(state, 100.0f, 0.0f);
	CHECK(sum(w) == Approx(1.0f));
	CHECK(weight(w, SkyCondition::Clean) == Approx(1.0f));

	state.visibilityM = 10000.0f;
	w = ComputeSkyWeights(state, 100.0f, 0.0f);
	CHECK(weight(w, SkyCondition::Hazy) == Approx(1.0f));

	state.visibilityM = 800.0f;
	w = ComputeSkyWeights(state, 100.0f, 0.0f);
	CHECK(weight(w, SkyCondition::Foggy) > 0.5f);
	CHECK(sum(w) == Approx(1.0f));

	state.visibilityM = 100.0f;
	w = ComputeSkyWeights(state, 100.0f, 0.0f);
	CHECK(weight(w, SkyCondition::Socked) == Approx(1.0f));

	// Full low stratus
	state.visibilityM = 30000.0f;
	state.clouds[0] = { 600.0f, 1300.0f, 1.0f, 1.0f };
	w = ComputeSkyWeights(state, 100.0f, 100.0f);
	CHECK(weight(w, SkyCondition::Stratus) == Approx(1.0f));
	state.clouds[0].type = 2.0f;	// Cumulus deck
	w = ComputeSkyWeights(state, 100.0f, 100.0f);
	CHECK(weight(w, SkyCondition::Overcast) == Approx(1.0f));
	state.clouds[0].baseMslM = 3000.0f;	// High deck: not overcast
	w = ComputeSkyWeights(state, 100.0f, 100.0f);
	CHECK(weight(w, SkyCondition::Overcast) == 0.0f);

	// Snowfall
	state.clouds = {};
	state.rainPercent = 0.6f;
	state.temperatureC = -5.0f;
	w = ComputeSkyWeights(state, 100.0f, 0.0f);
	CHECK(weight(w, SkyCondition::Snowy) == Approx(1.0f));

	// Altitude
	state.rainPercent = 0.0f;
	w = ComputeSkyWeights(state, 12000.0f, 0.0f);
	CHECK(weight(w, SkyCondition::HighAltitude) == Approx(1.0f));
	w = ComputeSkyWeights(state, 40000.0f, 0.0f);
	CHECK(weight(w, SkyCondition::Orbit) == Approx(1.0f));

	// A sweep always sums to 1
	for (float visibility : { 50.0f, 300.0f, 1200.0f, 4000.0f, 20000.0f, 39000.0f })
		for (float altitude : { 0.0f, 7000.0f, 25000.0f })
			for (float coverage : { 0.0f, 0.8f, 1.0f })
			{
				state.visibilityM = visibility;
				state.clouds[0] = { 500.0f, 900.0f, coverage, 1.5f };
				CHECK(sum(ComputeSkyWeights(state, altitude, 0.0f)) == Approx(1.0f));
			}

	SkyConditionTable table;
	REQUIRE(table.Parse(R"({ "classes": { "clean": { "ambient_ratio": 2.0 }, "hazy": { "ambient_ratio": 4.0 } } })"));
	SkyWeights half{};
	half[static_cast<size_t>(SkyCondition::Clean)] = 0.5f;
	half[static_cast<size_t>(SkyCondition::Hazy)] = 0.5f;
	CHECK(table.Blend(half).ambient == Approx(3.0f));
	CHECK(table.Blend(half).direct == Approx(1.0f));
	CHECK_FALSE(table.Parse("{ }"));
}

TEST_CASE("Environment: world-frame directions and local time", "[environment][state]")
{
	// +X east, +Y up, +Z south
	const Direction east = DirectionFromAzimuth(90.0, 0.0);
	CHECK(east.x == Approx(1.0f).margin(1e-6));
	CHECK(east.z == Approx(0.0f).margin(1e-6));
	const Direction north = DirectionFromAzimuth(0.0, 0.0);
	CHECK(north.z == Approx(-1.0f).margin(1e-6));
	const Direction up = DirectionFromAzimuth(123.0, 90.0);
	CHECK(up.y == Approx(1.0f).margin(1e-6));

	EnvironmentState state;
	state.year = 2026;
	state.month = 7;
	state.day = 15;
	state.localTimeHours = 23.5;
	state.utcOffsetHours = -7.0f;
	int y = 0, m = 0, d = 0;
	double utc = 0.0;
	LocalToUtc(state, y, m, d, utc);
	CHECK((y == 2026 && m == 7 && d == 16));
	CHECK(utc == Approx(6.5));

	state.month = 12;
	state.day = 31;
	LocalToUtc(state, y, m, d, utc);
	CHECK((y == 2027 && m == 1 && d == 1));

	state.month = 1;
	state.day = 1;
	state.localTimeHours = 2.0;
	state.utcOffsetHours = 11.0f;
	LocalToUtc(state, y, m, d, utc);
	CHECK((y == 2025 && m == 12 && d == 31));
	CHECK(utc == Approx(15.0));
}

TEST_CASE("Environment: derived values follow the state", "[environment][state]")
{
	EnvironmentState state;
	state.latitudeDeg = 47.4499;
	state.longitudeDeg = -122.3118;
	state.utcOffsetHours = -7.0f;
	state.localTimeHours = 13.2;	// Near solar noon at KSEA in July
	EnvironmentDerived d = Derive(state);
	CHECK(d.sun.apparentElevationDeg > 60.0);
	CHECK(d.sunDirection.y > 0.85f);
	CHECK(d.sunDirection.z > 0.0f);		// The noon sun is south (+Z)
	CHECK(d.daylight == Approx(1.0f));
	CHECK(d.nightFactor == 0.0f);
	CHECK(d.dayOfYear == 196);
	CHECK(d.season.season == Season::Summer);

	state.localTimeHours = 23.5;
	d = Derive(state);
	CHECK(d.sun.apparentElevationDeg < -6.0);
	CHECK(d.daylight == 0.0f);
	CHECK(d.nightFactor == Approx(1.0f));

	state.runwayFriction = 5;
	state.snowCover = 0.4f;
	d = Derive(state);
	CHECK(d.surface.puddles == Approx(0.5f));
	CHECK(d.surface.snow == Approx(0.4f));

	state.season = SeasonOverride::Winter;
	state.seasonRatioOverride = 0.25f;
	d = Derive(state);
	CHECK(d.season.season == Season::Winter);
	CHECK(d.season.ratio == Approx(0.25f));
}

TEST_CASE("Environment: the world versions its changes and takes scenarios", "[environment][world]")
{
	World &world = Get();
	const uint64_t before = world.GetVersion();
	world.SetState(world.GetState());
	CHECK(world.GetVersion() == before);	// No change, no new version

	world.Edit([](EnvironmentState &s) { s.visibilityM = 1234.0f; });
	CHECK(world.GetVersion() == before + 1);
	CHECK(world.GetSnapshot().state.visibilityM == 1234.0f);
	CHECK(world.GetSnapshot().version == world.GetVersion());

	ParityScenarioFile file;
	REQUIRE(ParseParityScenarios(R"({ "scenarios": [ { "id": "x", "date": "2026-01-15", "local_time": "12:00", "utc_offset": -8,
		"lat": 47.45, "lon": -122.31, "visibility_m": 800, "runway_friction": 9, "snow_cover": 1, "temperature_c": -5 } ] })", file));
	world.ApplyScenario(file.scenarios[0]);
	CHECK(world.GetState().month == 1);
	CHECK(world.GetState().visibilityM == 800.0f);
	CHECK(world.GetDerived().surface.snow == Approx(1.0f));
	CHECK(world.GetDerived().season.season == Season::Winter);
	CHECK(world.GetDerived().scenerySeason.season == Season::Winter);

	// Scenery loaded on another date keeps that date's seasonal art; the season itself (sun, trees) follows the date
	ParityScenarioFile stale;
	REQUIRE(ParseParityScenarios(R"({ "scenarios": [ { "id": "y", "date": "2026-07-15", "scenery_date": "2026-01-15",
		"lat": 53.43, "lon": -6.27 } ] })", stale));
	world.ApplyScenario(stale.scenarios[0]);
	CHECK(world.GetDerived().season.season == Season::Summer);
	CHECK(world.GetDerived().scenerySeason.season == Season::Winter);
	CHECK(world.GetSnapshot().derived.scenerySeason.bits == GetSeasonBit(Season::Winter));
	// A season the user forces applies to both
	world.Edit([](EnvironmentState &s) { s.season = SeasonOverride::Fall; });
	CHECK(world.GetDerived().scenerySeason.season == Season::Fall);
	world.Edit([](EnvironmentState &s) { s.season = SeasonOverride::Auto; });
	CHECK(world.GetDerived().scenerySeason.season == Season::Winter);
	world.SetSceneryDate(std::nullopt);
	CHECK(world.GetDerived().scenerySeason.season == Season::Summer);
	// A scenario without one follows its own date again
	ParityScenarioFile fresh;
	REQUIRE(ParseParityScenarios(R"({ "scenarios": [ { "id": "z", "date": "2026-07-15", "lat": 53.43, "lon": -6.27 } ] })", fresh));
	world.SetSceneryDate(std::array<int, 3>{ 2026, 1, 15 });
	world.ApplyScenario(fresh.scenarios[0]);
	CHECK(world.GetDerived().scenerySeason.season == Season::Summer);

	// Following the airport: the datum replaces the place; a manual place ignores it
	world.Edit([](EnvironmentState &s) { s.followAirport = true; });
	world.SetAirportLocation(-33.946, 151.177, 6.0f);
	CHECK(world.GetState().latitudeDeg == Approx(-33.946));
	world.Edit([](EnvironmentState &s) { s.followAirport = false; });
	world.SetAirportLocation(10.0, 10.0, 0.0f);
	CHECK(world.GetState().latitudeDeg == Approx(-33.946));

	// The integrated surface dries over time once it is on
	world.Edit([](EnvironmentState &s) { s.runwayFriction = 3; s.integrateSurface = true; s.rainPercent = 0.0f; s.temperatureC = 20.0f; });
	const float wet = world.GetDerived().surface.wetness;
	world.Tick(1800.0);
	CHECK(world.GetDerived().surface.wetness < wet);
}

TEST_CASE("Renderer quality levels match X-Plane's settings.txt", "[environment][quality]")
{
	using namespace SceneryEditorX;
	// renopt_shadow_quality_04
	CHECK_FALSE(GetShadowQuality(0).sceneryShadows);
	CHECK(GetShadowQuality(0).fadeRatio == Approx(0.999f));
	CHECK(GetShadowQuality(1).exteriorCascades == 2);
	CHECK(GetShadowQuality(1).exteriorFarM == 120.0f);
	CHECK(GetShadowQuality(1).mapSize == 1024u);
	CHECK(GetShadowQuality(2).exteriorCascades == 3);
	CHECK(GetShadowQuality(2).exteriorFarM == 250.0f);
	CHECK(GetShadowQuality(2).mapSize == 2048u);
	CHECK(GetShadowQuality(3).mapSize == 4096u);
	CHECK(GetShadowQuality(4).exteriorCascades == 4);
	CHECK(GetShadowQuality(4).exteriorFarM == 1000.0f);
	CHECK(GetShadowQuality(4).interiorFarM == 500.0f);
	CHECK(GetShadowQuality(4).fadeRatio == Approx(0.5f));
	for (int level = 0; level < SHADOW_LEVELS; ++level)
		CHECK(GetShadowQuality(level).interiorCascades == 3);
	CHECK(GetShadowQuality(99).mapSize == GetShadowQuality(4).mapSize);	// Clamped

	// renopt_SSAO_04
	CHECK_FALSE(GetAoQuality(0).enabled);
	CHECK(GetAoQuality(2).cacaoQuality == 3);
	CHECK(GetAoQuality(3).halfResolution);
	CHECK_FALSE(GetAoQuality(4).halfResolution);

	// renopt_clouds_02
	CHECK(GetCloudQuality(0).resolutionScale == Approx(0.20f));
	CHECK(GetCloudQuality(1).resolutionMax == 980u);
	CHECK(GetCloudQuality(2).resolutionMax == 1920u);
	CHECK(GetCloudQuality(3).shadowSteps == 8);
	CHECK_FALSE(GetCloudQuality(0).color);

	// renopt_FSR_04 and renopt_draw_distance04
	CHECK_FALSE(GetFsrQuality(0).enabled);
	CHECK(GetFsrQuality(1).textureLodBias == Approx(-0.38f));
	CHECK(GetFsrQuality(4).textureLodBias == Approx(-1.0f));
	CHECK(GetDrawDistanceQuality(0).lodBiasRatio == Approx(5.0f));
	CHECK(GetDrawDistanceQuality(1).lodBiasRatio == Approx(3.375f));
	CHECK(GetDrawDistanceQuality(4).lodBiasRatio == Approx(1.0f));
	CHECK(GetTextureResolution(4) == 5);

	// vkmtl_async defaults: shadows 2, SSAO 2, clouds 2, FXAA on, FSR off, anisotropy 16, texture resolution 4
	const RendererQualityPreset preset = GetDefaultQualityPreset();
	CHECK(preset.shadows == 2);
	CHECK(preset.ao == 2);
	CHECK(preset.clouds == 2);
	CHECK(preset.fxaa);
	CHECK(preset.fsr == 0);
	CHECK(preset.anisotropy == 16);
	CHECK(preset.textureResolution == 4);
}

TEST_CASE("Renderer feature switches", "[environment][quality]")
{
	using namespace SceneryEditorX;
	const RendererFeatures defaults;
	CHECK(RendererFeatures::FromBits(defaults.ToBits()) == defaults);
	CHECK(RendererFeatures::AllOff().ToBits() == 0u);
	CHECK_FALSE(defaults.fsr1);
	CHECK_FALSE(defaults.screenSpaceShadows);

	std::set<std::string> names;
	for (const RendererFeatureInfo &feature : GetRendererFeatureTable())
	{
		CHECK(names.insert(feature.name).second);	// --no-<name> spellings are unique
		CHECK(FindRendererFeature(feature.name) == &feature);
	}
	CHECK(names.size() == GetRendererFeatureTable().size());
	CHECK(GetRendererFeatureTable().size() <= 24);	// Bits fit the environment buffer's float exactly
	REQUIRE(FindRendererFeature("csm"));
	RendererFeatures features;
	features.*FindRendererFeature("csm")->member = false;
	CHECK_FALSE(features.csmShadows);
	CHECK(FindRendererFeature("nonsense") == nullptr);
	CHECK(std::string(GetDebugViewName(Renderer_DebugView::Normals)) == "Normals");

	// Shaders test these bits by their index in the table (FEATURE_* in environment_lighting, csm_sample and ssr_common.slang)
	const auto bit = [](const char *name) { return FindRendererFeature(name) - GetRendererFeatureTable().data(); };
	CHECK(bit("atmosphere") == 0);
	CHECK(bit("aerial-perspective") == 1);
	CHECK(bit("env-probes") == 2);
	CHECK(bit("cloud-shadows") == 3);
	CHECK(bit("csm") == 4);
	CHECK(bit("ssr") == 11);
	CHECK(bit("water") == 14);	// ocean_common.slang FEATURE_WATER
	REQUIRE(FindRendererFeature("terrain-scenery"));
	REQUIRE(FindRendererFeature("autogen"));
	CHECK(defaults.terrainScenery);
	CHECK(defaults.autogen);
}

// -------------------------------------------------------
// Sky light (environment/sky_light.*): the interim atmosphere that lights the scene in plan Phase 5
// -------------------------------------------------------

namespace
{
	double Lum(const Rgb &c) { return 0.2126 * c[0] + 0.7152 * c[1] + 0.0722 * c[2]; }
}

TEST_CASE("Sky light: the sun's transmittance follows the air mass", "[environment][sky-light]")
{
	const AtmosphereParams clear;

	// Straight up through X-Plane 12.4.4's clean atmosphere: Rayleigh, ozone and Mie optical depth about 0.09 in red
	// (half of it ozone), 0.23 in blue
	const Rgb zenith = ComputeTransmittance(clear, 0.0, 1.0);
	CHECK(zenith[0] == Catch::Approx(0.912).margin(0.01));
	CHECK(zenith[2] == Catch::Approx(0.794).margin(0.01));
	CHECK(zenith[0] > zenith[1]);
	CHECK(zenith[1] > zenith[2]);	// blue scatters most: the sun reddens

	// Lower suns go through more air; below the horizon the ground blocks them
	double previous = 1.0;
	for (const double elevation : { 60.0, 30.0, 10.0, 3.0, 0.5 })
	{
		const double t = Lum(ComputeTransmittance(clear, 0.0, std::sin(elevation * std::numbers::pi / 180.0)));
		CHECK(t < previous);
		previous = t;
	}
	CHECK(Lum(ComputeTransmittance(clear, 0.0, std::sin(-2.0 * std::numbers::pi / 180.0))) == 0.0);

	// Higher up there is less air above
	CHECK(Lum(ComputeTransmittance(clear, 10000.0, 0.2)) > Lum(ComputeTransmittance(clear, 0.0, 0.2)));
}

TEST_CASE("Sky light: a clear day's sun and sky", "[environment][sky-light]")
{
	const AtmosphereParams hazy = WithVisibility(AtmosphereParams{}, 80000.0);	// the parity scenarios' default

	const SkyLight noon = ComputeSkyLight(hazy, 0.0, 60.0, 115000.0, -20.0, 0.25);
	const double sun = Lum(noon.sunIlluminance);
	const double sky = Lum(noon.irradiance);
	CHECK(sun > 80000.0);
	CHECK(sun < 110000.0);
	CHECK(sky > 8000.0);			// clear-sky diffuse illuminance is 10-20 klx with a high sun
	CHECK(sky < 25000.0);
	CHECK(sky / (sun * std::sin(60.0 * std::numbers::pi / 180.0) + sky) < 0.25);	// mostly direct light on a clear day

	// Blue overhead, brighter toward the horizon, the sun's side brighter than the opposite one at 30 degrees up
	CHECK(noon.zenith[2] > noon.zenith[0]);
	CHECK(Lum(noon.horizon[1]) > Lum(noon.zenith));
	const double phaseToward = MiePhaseCornetteShanks(std::cos(30.0 * std::numbers::pi / 180.0), hazy.mieG);
	const double phaseAway = MiePhaseCornetteShanks(std::cos(150.0 * std::numbers::pi / 180.0), hazy.mieG);
	CHECK(Lum(noon.mid[0]) + phaseToward * Lum(noon.midMie[0]) > Lum(noon.mid[2]) + phaseAway * Lum(noon.midMie[2]));

	// The moon is below the horizon
	CHECK(Lum(noon.moonIlluminance) == 0.0);

	// Twilight: no direct sun, a dim sky; night: nothing
	const SkyLight dusk = ComputeSkyLight(hazy, 0.0, -3.0, 115000.0, 30.0, 0.25);
	CHECK(Lum(dusk.sunIlluminance) == 0.0);
	CHECK(Lum(dusk.irradiance) > 10.0);
	CHECK(Lum(dusk.irradiance) < 1000.0);
	CHECK(Lum(dusk.moonIlluminance) > 0.1);
	CHECK(Lum(dusk.moonIlluminance) < 0.25);
	const SkyLight night = ComputeSkyLight(hazy, 0.0, -30.0, 115000.0, 30.0, 0.25);
	CHECK(Lum(night.irradiance) == 0.0);

	// Thicker haze: less direct sun, never more light on the ground than arrives at the top, down to where fog starts
	const double sunUp = std::sin(60.0 * std::numbers::pi / 180.0);
	double previousSun = sun;
	for (const double visibility : { 40000.0, 30000.0, AtmosphereParams{}.fogStartVisibilityM })
	{
		const SkyLight hazier = ComputeSkyLight(WithVisibility(AtmosphereParams{}, visibility), 0.0, 60.0, 115000.0, -20.0, 0.25);
		const double direct = Lum(hazier.sunIlluminance);
		CHECK(direct < previousSun);
		CHECK(Lum(hazier.irradiance) + direct * sunUp < 115000.0 * sunUp);
		previousSun = direct;
	}

	// All fog: it dims the sun no more than clear air (its light goes round X-Plane 12.4.4's fog); the sky sends down no
	// more than 85% of what the fog would take from the beam
	const double clearSun = Lum(ComputeSkyLight(WithVisibility(AtmosphereParams{}, 1.0e6), 0.0, 60.0, 115000.0, -20.0, 0.25).sunIlluminance);
	for (const double visibility : { 8000.0, 3000.0 })
	{
		const AtmosphereParams medium = WithVisibility(AtmosphereParams{}, visibility);
		const SkyLight foggier = ComputeSkyLight(medium, 0.0, 60.0, 115000.0, -20.0, 0.25);
		CHECK(Lum(foggier.sunIlluminance) == Catch::Approx(clearSun));
		CHECK(Lum(foggier.irradiance) > 0.0);
		const double through = ComputeTransmittance(medium, 0.0, sunUp, 40, true)[1];
		CHECK(through < ComputeTransmittance(medium, 0.0, sunUp)[1]);
		CHECK(foggier.irradiance[1] <= 0.85 * 115000.0 * SUN_COLOR[1] * sunUp * (1.0 - through) * (1.0 + 1e-9));
	}
}

TEST_CASE("Atmosphere: visibility to haze", "[environment][atmosphere]")
{
	// X-Plane's scattering/vis_cutoff_ratio: 1% contrast at the visibility (Koschmieder's 2% on request)
	CHECK(VisibilityExtinction(10000.0) == Catch::Approx(4.60517e-4).epsilon(1e-5));
	CHECK(VisibilityExtinction(10000.0, 0.02) == Catch::Approx(3.912e-4).epsilon(1e-4));
	CHECK(VisibilityExtinction(0.0) == 0.0);

	// What the clear layers don't give at the ground in blue (contrast falls there first): extra Mie, spectral like the
	// clear Mie, as X-Plane 12.4.4 draws no fog at 20 km and more
	const AtmosphereParams clear;
	const AtmosphereParams hazy = WithVisibility(clear, 40000.0);
	const double cutoff = -std::log(0.01);
	CHECK(hazy.hazeExtinction == Catch::Approx((cutoff / 40000.0 - 27.68e-6 - 4.44e-6 * clear.mieSpectral[2]) / clear.mieSpectral[2]).epsilon(1e-9));
	CHECK(hazy.fogExtinction == 0.0);
	CHECK(WithVisibility(clear, 20000.0).fogExtinction == 0.0);
	CHECK(hazy.hazeScatteringAlbedo == Catch::Approx(0.9));
	CHECK(hazy.mieExtinction == clear.mieExtinction);
	CHECK(WithVisibility(clear, 1.0e6).hazeExtinction == 0.0);	// never cleaner than clear air
	const MediumSample ground = SampleMedium(hazy, 0.0);
	CHECK(ground.Extinction()[2] == Catch::Approx(cutoff / 40000.0).epsilon(1e-6));	// blue: the visibility
	CHECK(ground.Extinction()[1] < ground.Extinction()[2]);
	CHECK(ground.ExtinctionWithoutFog()[2] == ground.Extinction()[2]);

	// Shorter visibilities turn it into grey fog, all of it at 8 km (X-Plane's fog-white haze), half of it at 14 km
	const AtmosphereParams thick = WithVisibility(clear, 8000.0);
	CHECK(thick.hazeExtinction == 0.0);
	CHECK(thick.fogExtinction == Catch::Approx(cutoff / 8000.0 - 27.68e-6 - 4.44e-6 * clear.mieSpectral[2]).epsilon(1e-9));
	CHECK(thick.fogScatteringAlbedo == Catch::Approx(0.9));
	const AtmosphereParams between = WithVisibility(clear, 14000.0);
	CHECK(between.fogExtinction == Catch::Approx(between.hazeExtinction * clear.mieSpectral[2]).epsilon(1e-9));
	CHECK(WithVisibility(clear, 3000.0).hazeExtinction == 0.0);
	const MediumSample foggy = SampleMedium(thick, 0.0);
	CHECK(foggy.Extinction()[2] == Catch::Approx(cutoff / 8000.0).epsilon(1e-6));
	CHECK(foggy.fogScattering == Catch::Approx(0.9 * thick.fogExtinction));
	CHECK(foggy.Extinction()[1] - foggy.ExtinctionWithoutFog()[1] == Catch::Approx(thick.fogExtinction));	// the sun's light goes round it
	CHECK(foggy.Extinction()[0] - foggy.ExtinctionWithoutFog()[0] == Catch::Approx(foggy.Extinction()[2] - foggy.ExtinctionWithoutFog()[2]));	// grey

	// Mie and haze per channel: Angstrom -1.3 with the wavelengths the Rayleigh coefficients imply (blue scatters 1.32x)
	const Rgb spectral = MieSpectralFactors(clear.rayleighScattering, -1.3);
	CHECK(spectral[0] == Catch::Approx(0.786).margin(0.002));
	CHECK(spectral[1] == 1.0);
	CHECK(spectral[2] == Catch::Approx(1.321).margin(0.002));
	CHECK(spectral[2] == Catch::Approx(clear.mieSpectral[2]).margin(1e-3));
	CHECK(MieSpectralFactors(clear.rayleighScattering, 0.0)[2] == 1.0);
	CHECK(SampleMedium(clear, 0.0).mieExtinction[2] / SampleMedium(clear, 0.0).mieExtinction[1] == Catch::Approx(clear.mieSpectral[2]));
	CHECK(ground.mieExtinction[2] / ground.mieExtinction[1] == Catch::Approx(clear.mieSpectral[2]));	// the haze is spectral too

	// The layer: uniform to its top above the ground, easing out over the band, gone above it; on an elevated airport too
	const AtmosphereParams fog = WithVisibility(clear, 1000.0, 400.0);
	CHECK(fog.hazeTopM == Catch::Approx(1500.0));
	CHECK(HazeTopFromVisibility(200000.0) == Catch::Approx(4000.0));
	CHECK(HazeProfile(fog, 400.0) == 1.0);
	CHECK(HazeProfile(fog, 1899.0) == 1.0);
	CHECK(HazeProfile(fog, 1900.0 + fog.hazeBandM * 0.5) == Catch::Approx(0.5));
	CHECK(HazeProfile(fog, 1900.0 + fog.hazeBandM * 0.25) == Catch::Approx(0.84375));	// smoothstep
	CHECK(HazeProfile(fog, 1900.0 + fog.hazeBandM) == 0.0);
	CHECK(SampleMedium(fog, 400.0).Extinction()[2] == Catch::Approx(cutoff / 1000.0).epsilon(1e-6));
	CHECK(SampleMedium(fog, 5000.0).fogExtinction == 0.0);
	CHECK(SampleMedium(fog, 5000.0).mieExtinction[1] < 1e-5);

	// Sky-condition multipliers scale the scattering, not the absorption
	const AtmosphereParams doubled = WithSkyMultipliers(hazy, 2.0, 2.0);
	CHECK(doubled.rayleighScattering[2] == Catch::Approx(2.0 * clear.rayleighScattering[2]));
	CHECK(doubled.mieScattering == Catch::Approx(2.0 * clear.mieScattering));
	CHECK(doubled.mieExtinction - doubled.mieScattering == Catch::Approx(clear.mieExtinction - clear.mieScattering));
	CHECK(doubled.hazeExtinction * doubled.hazeScatteringAlbedo == Catch::Approx(2.0 * hazy.hazeExtinction * hazy.hazeScatteringAlbedo));
	CHECK(WithSkyMultipliers(thick, 2.0, 2.0).fogExtinction == thick.fogExtinction);	// the fog isn't the Mie

	// sedx/haze_mie_scale thins the extra Mie, not the fog
	AtmosphereParams thinner = clear;
	thinner.hazeMieScale = 0.55;
	CHECK(WithVisibility(thinner, 40000.0).hazeExtinction == Catch::Approx(0.55 * hazy.hazeExtinction).epsilon(1e-12));
	CHECK(WithVisibility(thinner, 14000.0).hazeExtinction == Catch::Approx(0.55 * between.hazeExtinction).epsilon(1e-12));
	CHECK(WithVisibility(thinner, 14000.0).fogExtinction == between.fogExtinction);
	CHECK(WithVisibility(thinner, 8000.0).fogExtinction == thick.fogExtinction);
}

TEST_CASE("Sky light: the sun's colour and its tint", "[environment][sky-light]")
{
	const auto luminance = [](const Rgb &c) { return 0.2126 * c[0] + 0.7152 * c[1] + 0.0722 * c[2]; };
	CHECK(luminance(SUN_COLOR) == Catch::Approx(1.0).epsilon(1e-4));
	const AtmosphereParams clear;
	for (int c = 0; c < 3; ++c)
		CHECK(SunColor(clear)[c] == Catch::Approx(SUN_COLOR[c] / luminance(SUN_COLOR)).epsilon(1e-12));

	// A tint shifts the colour at the same luminance, and lights the sky model with it
	AtmosphereParams warm = clear;
	warm.sunTint = { 1.08, 1.0, 0.98 };
	const Rgb color = SunColor(warm);
	CHECK(luminance(color) == Catch::Approx(1.0).epsilon(1e-12));
	CHECK(color[0] / color[1] == Catch::Approx(1.08 * SUN_COLOR[0] / SUN_COLOR[1]).epsilon(1e-12));
	CHECK(color[2] / color[1] == Catch::Approx(0.98 * SUN_COLOR[2] / SUN_COLOR[1]).epsilon(1e-12));
	const SkyLight plain = ComputeSkyLight(clear, 0.0, 40.0, 133000.0, -20.0, 0.0);
	const SkyLight warmer = ComputeSkyLight(warm, 0.0, 40.0, 133000.0, -20.0, 0.0);
	CHECK(warmer.sunIlluminance[0] / warmer.sunIlluminance[1] == Catch::Approx(1.08 * plain.sunIlluminance[0] / plain.sunIlluminance[1]).epsilon(1e-9));
	CHECK(warmer.zenith[0] / warmer.zenith[1] > plain.zenith[0] / plain.zenith[1]);
	CHECK(luminance(warmer.sunIlluminance) == Catch::Approx(luminance(plain.sunIlluminance)).epsilon(0.02));
}

TEST_CASE("Sky light: the Mie phase function is normalized", "[environment][sky-light]")
{
	// Integral over the sphere: 2 pi x integral of p(cos) d(cos) over -1..1
	for (const double g : { 0.0, 0.5, 0.8 })
	{
		double sum = 0.0;
		constexpr int N = 20000;
		for (int i = 0; i < N; ++i)
		{
			const double mu = -1.0 + (i + 0.5) * 2.0 / N;
			sum += MiePhaseCornetteShanks(mu, g) * 2.0 / N;
		}
		CHECK(sum * 2.0 * std::numbers::pi == Catch::Approx(1.0).margin(0.01));
	}
}

// -------------------------------------------------------
// Exposure (renderer/exposure.*): the reference for exposure_adapt.slang
// -------------------------------------------------------

namespace
{
	// The luminance a meter with constant K reads as an EV100
	float EvToNits(const float ev100, const float meterK) { return std::exp2(ev100) * meterK / 100.0f; }
}

TEST_CASE("Exposure: EV100 camera, X-Plane 12.4.4's target and adaptation", "[environment][exposure]")
{
	using namespace SceneryEditorX;
	const ExposureSettings settings;

	// A meter reading EV 12 sees 512 nits; exposed at EV 12 they become middle grey
	CHECK(EvFromLuminance(512.0f) == Catch::Approx(12.0f).margin(1e-4));
	CHECK(EvFromLuminance(512.0f) - EvFromLuminance(256.0f) == Catch::Approx(1.0f).margin(1e-4));
	CHECK(ExposureScaleFromEv(12.0f) * 512.0f == Catch::Approx(0.18f).margin(1e-4));
	CHECK(ExposureScaleFromEv(9.0f, 14.0f) * EvToNits(9.0f, 14.0f) == Catch::Approx(0.18f).margin(1e-4));

	// photometric/ev100_mid and ev100_bias by the sun's elevation: the readouts they were fitted to, flat beyond
	CHECK(GetExposureMid(settings, 40.19f) == Catch::Approx(14.62f));
	CHECK(GetExposureMid(settings, -1.93f) == Catch::Approx(10.425f));
	CHECK(GetExposureMid(settings, -30.0f) == Catch::Approx(5.0f));
	CHECK(GetExposureMid(settings, 30.0f) > 14.5f);
	CHECK(GetExposureMid(settings, 30.0f) < 14.62f);
	CHECK(GetExposureBias(settings, -17.0f) == Catch::Approx(2.59f));
	CHECK(GetExposureBias(settings, 40.19f) == Catch::Approx(-0.34f));

	// X-Plane's one genuine automatic readout: sun -11.8 deg, metered 1.02 -> EV 4.31 (5 + 0.5 x (1.02 + 2.59 - 5))
	CHECK(GetTargetEv(settings, 1.02f, -11.82f) == Catch::Approx(4.305f).margin(0.01f));
	// Darker than the target: gain_lo; brighter: gain_hi; then the 3.5-14.8 range
	CHECK(GetTargetEv(settings, 14.0f, 40.19f) == Catch::Approx(14.62f + 0.5f * (14.0f - 0.34f - 14.62f)));
	CHECK(GetTargetEv(settings, 15.2f, 40.19f) == Catch::Approx(14.62f + 0.4f * (15.2f - 0.34f - 14.62f)));
	CHECK(GetTargetEv(settings, 15.9f, 40.19f) == Catch::Approx(14.8f));
	CHECK(GetTargetEv(settings, -10.0f, -30.0f) == Catch::Approx(3.5f));
	ExposureSettings brighter = settings;
	brighter.compensation = 1.0f;
	CHECK(GetTargetEv(brighter, 14.0f, 40.19f) == Catch::Approx(GetTargetEv(settings, 14.0f, 40.19f) - 1.0f));
	ExposureSettings manual = settings;
	manual.automatic = false;
	manual.manualEv100 = 9.0f;
	CHECK(GetTargetEv(manual, 14.0f, 40.0f) == Catch::Approx(9.0f));

	// Adaptation: at most speed x dt per frame, faster toward a darker scene (speed_down 5, speed_up 3)
	CHECK(AdaptEv(settings, 10.0f, 14.0f, 0.5f) == Catch::Approx(11.5f));
	CHECK(AdaptEv(settings, 10.0f, 4.0f, 0.5f) == Catch::Approx(7.5f));
	CHECK(AdaptEv(settings, 10.0f, 10.2f, 0.5f) == Catch::Approx(10.2f));
}

TEST_CASE("Exposure: histogram bins and metering", "[environment][exposure]")
{
	using namespace SceneryEditorX;
	CHECK(GetExposureBin(0.0f) == 0);
	CHECK(GetExposureBin(-1.0f) == 0);
	CHECK(GetExposureBin(0.25f) == 0);					// log2 = -2: the range's start
	CHECK(GetExposureBin(1.0e9f) == EXPOSURE_HISTOGRAM_BINS - 1);
	CHECK(GetExposureBin(1.0f) == 25);					// (0 + 2) / 20 x 256 = 25.6

	// One luminance everywhere: the mean is its bin's centre
	std::array<uint32_t, EXPOSURE_HISTOGRAM_BINS> bins{};
	bins[static_cast<size_t>(GetExposureBin(512.0f))] = 1000;
	float mean = 0.0f;
	REQUIRE(MeterHistogram(bins, 0.5f, 0.98f, mean));
	CHECK(mean == Catch::Approx(std::log2(512.0f)).margin(20.0f / EXPOSURE_HISTOGRAM_BINS));

	// Half the pixels black: the lower trim leaves them out entirely
	bins[0] = 1000;
	REQUIRE(MeterHistogram(bins, 0.5f, 0.98f, mean));
	CHECK(mean == Catch::Approx(std::log2(512.0f)).margin(20.0f / EXPOSURE_HISTOGRAM_BINS));

	// A few very bright pixels (the sun) are trimmed off the top
	bins[0] = 0;
	bins[EXPOSURE_HISTOGRAM_BINS - 1] = 10;
	REQUIRE(MeterHistogram(bins, 0.5f, 0.98f, mean));
	CHECK(mean == Catch::Approx(std::log2(512.0f)).margin(20.0f / EXPOSURE_HISTOGRAM_BINS));

	std::array<uint32_t, EXPOSURE_HISTOGRAM_BINS> empty{};
	CHECK_FALSE(MeterHistogram(empty, 0.5f, 0.98f, mean));
}

// -------------------------------------------------------
// Environment probes (renderer/env_probes.*): when probe 0 is captured again
// -------------------------------------------------------

TEST_CASE("Environment probes: capture rules", "[environment][probes]")
{
	using namespace SceneryEditorX;

	// The camera snaps to the 50 m grid
	const ProbeVec3 snapped = SnapProbePosition({ 124.0, 26.0, -76.0 });
	CHECK(snapped[0] == 100.0);
	CHECK(snapped[1] == 50.0);
	CHECK(snapped[2] == -100.0);

	EnvProbeInputs inputs;
	inputs.camera = { 10.0, 2.0, 10.0 };
	inputs.sun = { 0.0, 1.0, 0.0 };
	inputs.mediumHash = HashCombine(0, 40000.0);
	CHECK(EnvProbeNeedsCapture(EnvProbeState{}, inputs));	// never captured

	const EnvProbeState state = CaptureEnvProbe(inputs);
	CHECK_FALSE(EnvProbeNeedsCapture(state, inputs));

	// The camera moving within 200 m of the probe: no; further: yes
	EnvProbeInputs moved = inputs;
	moved.camera = { 150.0, 2.0, 10.0 };
	CHECK_FALSE(EnvProbeNeedsCapture(state, moved));
	moved.camera = { 250.0, 2.0, 10.0 };
	CHECK(EnvProbeNeedsCapture(state, moved));

	// The sun or moon turning more than half a degree
	const double small = 0.4 * std::numbers::pi / 180.0;
	const double large = 0.6 * std::numbers::pi / 180.0;
	EnvProbeInputs turned = inputs;
	turned.sun = { std::sin(small), std::cos(small), 0.0 };
	CHECK_FALSE(EnvProbeNeedsCapture(state, turned));
	turned.sun = { std::sin(large), std::cos(large), 0.0 };
	CHECK(EnvProbeNeedsCapture(state, turned));
	turned = inputs;
	turned.moon = { 0.0, std::cos(large), std::sin(large) };
	CHECK(EnvProbeNeedsCapture(state, turned));

	// The medium changing (visibility, sky condition, light)
	EnvProbeInputs hazier = inputs;
	hazier.mediumHash = HashCombine(0, 20000.0);
	CHECK(hazier.mediumHash != inputs.mediumHash);
	CHECK(EnvProbeNeedsCapture(state, hazier));
}

namespace
{
	/* @brief A camera at a position looking along yaw (from -Z toward +X) and pitch, 60 degrees wide, 16:9. */
	SceneryEditorX::CsmCamera MakeCsmCamera(const SceneryEditorX::CsmVec3 &position, const double yawDeg, const double pitchDeg)
	{
		SceneryEditorX::CsmCamera camera;
		camera.position = position;
		const double yaw = yawDeg * DEG, pitch = pitchDeg * DEG;
		camera.forward = { std::cos(pitch) * std::sin(yaw), std::sin(pitch), -std::cos(pitch) * std::cos(yaw) };
		camera.tanHalfFovX = std::tan(30.0 * DEG);
		camera.tanHalfFovY = camera.tanHalfFovX * 9.0 / 16.0;
		camera.nearPlane = 0.1;
		return camera;
	}

	/* @brief The 8 corners of the camera's view-frustum slice between two view depths. */
	std::array<SceneryEditorX::CsmVec3, 8> SliceCorners(const SceneryEditorX::CsmCamera &camera, const double depthNear, const double depthFar)
	{
		const SceneryEditorX::CsmVec3 &f = camera.forward;
		const double length = std::sqrt(f[2] * f[2] + f[0] * f[0]);
		const SceneryEditorX::CsmVec3 right{ -f[2] / length, 0.0, f[0] / length };	// forward x world up
		const SceneryEditorX::CsmVec3 up{ right[1] * f[2] - right[2] * f[1], right[2] * f[0] - right[0] * f[2], right[0] * f[1] - right[1] * f[0] };
		std::array<SceneryEditorX::CsmVec3, 8> corners{};
		size_t n = 0;
		for (const double depth : { depthNear, depthFar })
		{
			for (const double sx : { -1.0, 1.0 })
			{
				for (const double sy : { -1.0, 1.0 })
				{
					SceneryEditorX::CsmVec3 &c = corners[n++];
					for (size_t axis = 0; axis < 3; ++axis)
						c[axis] = camera.position[axis] + depth * (f[axis] + sx * camera.tanHalfFovX * right[axis] + sy * camera.tanHalfFovY * up[axis]);
				}
			}
		}
		return corners;
	}
}

TEST_CASE("Sun shadows: cascade splits", "[environment][csm]")
{
	using namespace SceneryEditorX;

	// X-Plane 12.4.4's controls (3 cascades, 4 - 250 m, lambda 1): logarithmic splits, the first cascade from the camera
	CsmSettings settings;
	auto splits = ComputeCsmSplits(settings, 0.1);
	CHECK(splits[0] == Approx(0.1));
	CHECK(splits[1] == Approx(4.0 * std::pow(62.5, 1.0 / 3.0)));
	CHECK(splits[2] == Approx(4.0 * std::pow(62.5, 2.0 / 3.0)));
	CHECK(splits[3] == Approx(250.0));

	// lambda 0: uniform between the limits
	settings.lambda = 0.0;
	splits = ComputeCsmSplits(settings, 0.1);
	CHECK(splits[1] == Approx(4.0 + 246.0 / 3.0));
	CHECK(splits[2] == Approx(4.0 + 2.0 * 246.0 / 3.0));

	// Monotonic for every level, count and lambda
	for (int level = 0; level < SHADOW_LEVELS; ++level)
	{
		const ShadowQuality quality = GetShadowQuality(level);
		for (const double lambda : { 0.0, 0.5, 0.75, 1.0 })
		{
			settings.cascadeCount = static_cast<uint32_t>(quality.exteriorCascades);
			settings.farLimitM = quality.exteriorFarM;
			settings.lambda = lambda;
			splits = ComputeCsmSplits(settings, 0.25);
			for (uint32_t i = 0; i < settings.cascadeCount; ++i)
				CHECK(splits[i] < splits[i + 1]);
			CHECK(splits[settings.cascadeCount] == Approx(quality.exteriorFarM));
		}
	}
}

TEST_CASE("Sun shadows: a slice's bounding sphere holds its corners", "[environment][csm]")
{
	using namespace SceneryEditorX;
	const CsmCamera camera = MakeCsmCamera({ 0.0, 0.0, 0.0 }, 0.0, 0.0);
	for (const auto &[n, f] : { std::pair{ 0.1, 15.9 }, std::pair{ 15.9, 63.0 }, std::pair{ 63.0, 250.0 }, std::pair{ 0.1, 1000.0 } })
	{
		double centerDepth = 0.0, radius = 0.0;
		CsmSliceSphere(n, f, camera.tanHalfFovX, camera.tanHalfFovY, centerDepth, radius);
		CHECK(centerDepth >= n);
		CHECK(centerDepth <= f);
		double furthest = 0.0;
		for (const CsmVec3 &corner : SliceCorners(camera, n, f))
		{
			const CsmVec3 d{ corner[0] - camera.forward[0] * centerDepth, corner[1] - camera.forward[1] * centerDepth, corner[2] - camera.forward[2] * centerDepth };
			furthest = std::max(furthest, std::sqrt(d[0] * d[0] + d[1] * d[1] + d[2] * d[2]));
		}
		CHECK(furthest <= radius * (1.0 + 1e-9));
		CHECK(furthest >= radius * 0.999);	// and is no larger than it needs to be
	}
}

TEST_CASE("Sun shadows: cascades cover the view and stay put while the camera moves", "[environment][csm]")
{
	using namespace SceneryEditorX;
	const CsmSettings settings;
	const CsmVec3 towardSun = { 0.5 * std::cos(40.0 * DEG), std::sin(40.0 * DEG), 0.866 * std::cos(40.0 * DEG) };

	CsmSetup setup;
	const CsmCamera camera = MakeCsmCamera({ 120.0, 2.0, -40.0 }, 161.0, -5.0);
	REQUIRE(FitCsm(settings, camera, towardSun, setup));
	REQUIRE(setup.count == 3);
	CHECK(setup.fadeStart == Approx(0.7 * 250.0));
	CHECK(setup.fadeEnd == Approx(250.0));

	for (uint32_t i = 0; i < setup.count; ++i)
	{
		const CsmCascade &cascade = setup.cascades[i];
		CHECK(cascade.texelSize == Approx(2.0 * cascade.radius / settings.mapSize));

		// Every corner of the slice it serves lands inside its square, with room for the filter, and in its depth range
		for (const CsmVec3 &corner : SliceCorners(camera, cascade.depthNear, cascade.depthFar))
		{
			const CsmVec3 clip = cascade.Project(corner);
			CHECK(std::abs(clip[0]) <= 1.0 - 3.0 / settings.mapSize);
			CHECK(std::abs(clip[1]) <= 1.0 - 3.0 / settings.mapSize);
			CHECK(clip[2] >= 0.0);
			CHECK(clip[2] <= 1.0);
			CHECK(cascade.CanCast(corner, corner));
		}

		// Reverse-Z: points toward the light are nearer (larger depth)
		const CsmVec3 lifted{ cascade.center[0] + towardSun[0] * 10.0, cascade.center[1] + towardSun[1] * 10.0, cascade.center[2] + towardSun[2] * 10.0 };
		CHECK(cascade.Project(lifted)[2] > cascade.Project(cascade.center)[2]);
	}

	// The blend band: the next cascade starts that far before this one ends
	CHECK(setup.cascades[1].depthNear == Approx(setup.splits[1] - setup.blendBand[0]));
	CHECK(setup.blendBand[0] == Approx(0.01 * (setup.splits[1] - setup.splits[0])));
	CHECK(setup.blendBand[2] == 0.0);

	// Panning and turning: the radii stay the same, and a fixed world point moves by whole texels in every cascade
	const CsmVec3 world{ 100.0, 0.0, -30.0 };
	for (const auto &[offset, yaw] : { std::pair{ CsmVec3{ 0.37, 0.0, 0.11 }, 161.0 }, std::pair{ CsmVec3{ -3.3, 0.2, 7.9 }, 150.0 }, std::pair{ CsmVec3{ 25.0, 0.0, -14.0 }, 200.0 } })
	{
		CsmSetup moved;
		const CsmCamera other = MakeCsmCamera({ camera.position[0] + offset[0], camera.position[1] + offset[1], camera.position[2] + offset[2] }, yaw, -5.0);
		REQUIRE(FitCsm(settings, other, towardSun, moved));
		for (uint32_t i = 0; i < setup.count; ++i)
		{
			CHECK(moved.cascades[i].radius == Approx(setup.cascades[i].radius).epsilon(1e-12));
			const double before = (setup.cascades[i].Project(world)[0] * 0.5 + 0.5) * settings.mapSize;
			const double after = (moved.cascades[i].Project(world)[0] * 0.5 + 0.5) * settings.mapSize;
			const double shift = after - before;
			CHECK(std::abs(shift - std::round(shift)) < 1e-6);
			const double beforeY = (setup.cascades[i].Project(world)[1] * 0.5 + 0.5) * settings.mapSize;
			const double afterY = (moved.cascades[i].Project(world)[1] * 0.5 + 0.5) * settings.mapSize;
			CHECK(std::abs((afterY - beforeY) - std::round(afterY - beforeY)) < 1e-6);
		}
	}
}

TEST_CASE("Sun shadows: orthographic views split evenly around the pivot", "[environment][csm]")
{
	using namespace SceneryEditorX;
	const CsmSettings settings;	// 3 cascades, 250 m
	const CsmVec3 towardSun = { 0.5 * std::cos(40.0 * DEG), std::sin(40.0 * DEG), 0.866 * std::cos(40.0 * DEG) };

	// A top view from 600 m up (beyond the perspective far limit): 300 x 168.75 m of ground around the pivot
	CsmCamera camera;
	camera.position = { 50.0, 600.0, -20.0 };
	camera.forward = { 0.0, -1.0, 0.0 };
	camera.orthographic = true;
	camera.orthoHalfWidth = 150.0;
	camera.orthoHalfHeight = 84.375;
	camera.focusDepth = 600.0;

	CsmSetup setup;
	REQUIRE(FitCsm(settings, camera, towardSun, setup));
	REQUIRE(setup.count == 3);
	CHECK(setup.splits[0] == Approx(475.0));
	CHECK(setup.splits[1] == Approx(475.0 + 250.0 / 3.0));
	CHECK(setup.splits[3] == Approx(725.0));
	CHECK(setup.fadeEnd == Approx(725.0));
	CHECK(setup.fadeStart == Approx(475.0 + 0.7 * 250.0));

	// Every corner of each slice's box lies inside its cascade (the ground at the pivot included)
	for (uint32_t i = 0; i < setup.count; ++i)
	{
		const CsmCascade &cascade = setup.cascades[i];
		for (const double depth : { cascade.depthNear, cascade.depthFar })
		{
			for (const double sx : { -1.0, 1.0 })
			{
				for (const double sz : { -1.0, 1.0 })
				{
					// Looking straight down, right is east (+X) and up the screen is north (-Z)
					const CsmVec3 corner{ camera.position[0] + sx * camera.orthoHalfWidth, camera.position[1] - depth, camera.position[2] + sz * camera.orthoHalfHeight };
					const CsmVec3 clip = cascade.Project(corner);
					CHECK(std::abs(clip[0]) <= 1.0);
					CHECK(std::abs(clip[1]) <= 1.0);
					CHECK(clip[2] >= 0.0);
					CHECK(clip[2] <= 1.0);
				}
			}
		}
	}
}

TEST_CASE("Sun shadows: which boxes cast into a cascade", "[environment][csm]")
{
	using namespace SceneryEditorX;
	const CsmSettings settings;
	const CsmVec3 towardSun = { 0.0, std::sin(30.0 * DEG), std::cos(30.0 * DEG) };	// low in the south (+Z)

	CsmSetup setup;
	const CsmCamera camera = MakeCsmCamera({ 0.0, 2.0, 0.0 }, 0.0, -10.0);	// looking north (-Z)
	REQUIRE(FitCsm(settings, camera, towardSun, setup));
	const CsmCascade &last = setup.cascades[setup.count - 1];

	// A hangar between the view and the sun, outside the view itself, casts; so does one far toward the sun (clamped
	// onto the near plane while drawn); one beside the cascade's square or beyond its far plane doesn't
	CHECK(last.CanCast({ -10.0, 0.0, 20.0 }, { 10.0, 15.0, 40.0 }));
	CHECK(last.CanCast({ -10.0, 1000.0, 1700.0 }, { 10.0, 1010.0, 1720.0 }));
	CHECK_FALSE(last.CanCast({ 900.0, 0.0, -100.0 }, { 920.0, 15.0, -80.0 }));
	CHECK_FALSE(last.CanCast({ -10.0, -2000.0, -3500.0 }, { 10.0, -1990.0, -3480.0 }));
	CHECK((setup.CascadeMask({ -1.0, 0.0, -6.0 }, { 1.0, 1.0, -5.0 }) & 1u) != 0);

	// The sun below the horizon: no shadows
	CsmSetup night;
	CHECK_FALSE(FitCsm(settings, camera, { 0.0, -0.1, 0.995 }, night));
	CHECK(night.count == 0);
}

TEST_CASE("Cloud shadows: cascades around the camera", "[environment][csm]")
{
	using namespace SceneryEditorX;

	// X-Plane 12.4.4: two 1024 px cascades, 64 km and 256 km wide
	const CloudShadowCascade fine = FitCloudShadowCascade(0, CLOUD_SHADOW_CASCADES, 64000.0, 256000.0, 1234.0, -987.0);
	const CloudShadowCascade coarse = FitCloudShadowCascade(1, CLOUD_SHADOW_CASCADES, 64000.0, 256000.0, 1234.0, -987.0);
	CHECK(fine.halfExtentM == Approx(32000.0));
	CHECK(coarse.halfExtentM == Approx(128000.0));
	CHECK(fine.texelM == Approx(62.5));
	CHECK(coarse.texelM == Approx(250.0));

	// Centers on whole texels, within half a texel of the camera
	CHECK(std::abs(fine.centerX / fine.texelM - std::round(fine.centerX / fine.texelM)) < 1e-9);
	CHECK(std::abs(fine.centerX - 1234.0) <= 0.5 * fine.texelM);
	CHECK(std::abs(coarse.centerZ - -987.0) <= 0.5 * coarse.texelM);

	// One cascade: the smallest width
	CHECK(FitCloudShadowCascade(0, 1, 64000.0, 256000.0, 0.0, 0.0).halfExtentM == Approx(32000.0));
}

TEST_CASE("Ambient occlusion: CACAO's buffer sizes", "[environment][cacao]")
{
	using namespace SceneryEditorX;

	// Half resolution (X-Plane's downsample 1): 4 deinterleaved layers of a quarter of the output, importance at an eighth
	const CacaoBufferSizes half = ComputeCacaoBufferSizes(1920, 1080, true);
	CHECK(half.ssaoWidth == 480u);
	CHECK(half.ssaoHeight == 270u);
	CHECK(half.deinterleavedDepthWidth == 480u);
	CHECK(half.deinterleavedDepthHeight == 270u);
	CHECK(half.importanceWidth == 240u);
	CHECK(half.importanceHeight == 135u);
	CHECK(half.downsampled);

	// Full resolution (renopt_SSAO_04 level 4): layers of half the output
	const CacaoBufferSizes full = ComputeCacaoBufferSizes(1920, 1080, false);
	CHECK(full.ssaoWidth == 960u);
	CHECK(full.ssaoHeight == 540u);
	CHECK(full.importanceWidth == 480u);
	CHECK(full.importanceHeight == 270u);

	// Odd sizes round up at every halving
	const CacaoBufferSizes odd = ComputeCacaoBufferSizes(1281, 721, true);
	CHECK(odd.ssaoWidth == 321u);
	CHECK(odd.ssaoHeight == 181u);
	CHECK(odd.importanceWidth == 161u);
	CHECK(odd.importanceHeight == 91u);
}

TEST_CASE("Ambient occlusion: CACAO's constants for SEDX's reverse-Z camera", "[environment][cacao]")
{
	using namespace SceneryEditorX;

	CacaoCamera camera;
	camera.tanHalfFovX = std::tan(0.5 * 60.0 * 3.14159265358979323846 / 180.0);
	camera.tanHalfFovY = camera.tanHalfFovX / (1920.0 / 1080.0);
	camera.nearPlane = 0.1;
	camera.farPlane = 10000.0;
	const CacaoSettings settings;	// X-Plane 12.4.4's
	const CacaoBufferSizes sizes = ComputeCacaoBufferSizes(1920, 1080, true);
	const CacaoConstants c = ComputeCacaoConstants(settings, CacaoQuality::High, sizes, camera);

	// The shaders' view depth of a reverse-Z depth: 1 at the near plane, 0 at the far one
	const double n = camera.nearPlane;
	const double f = camera.farPlane;
	for (const double z : { 0.1, 0.5, 2.0, 10.0, 250.0, 1000.0, 10000.0 })
	{
		const double depth = n * (f / z - 1.0) / (f - n);
		CHECK(CacaoViewDepth(c, depth) == Approx(z).epsilon(1e-4));
	}
	CHECK(CacaoViewDepth(c, 1.0) == Approx(n).epsilon(1e-4));
	CHECK(CacaoViewDepth(c, 0.0) == Approx(f).epsilon(1e-4));

	// Screen uv (y down) to view space (y up): the center is on the axis, the top right corner at (tan x, tan y)
	const auto toView = [&c](const double u, const double v, const double z)
	{
		return std::array<double, 2>{ (c.ndcToView[0] * u + c.ndcToView[2]) * z, (c.ndcToView[1] * v + c.ndcToView[3]) * z };
	};
	CHECK(toView(0.5, 0.5, 10.0)[0] == Approx(0.0).margin(1e-6));
	CHECK(toView(0.5, 0.5, 10.0)[1] == Approx(0.0).margin(1e-6));
	CHECK(toView(1.0, 0.0, 10.0)[0] == Approx(camera.tanHalfFovX * 10.0).epsilon(1e-5));
	CHECK(toView(1.0, 0.0, 10.0)[1] == Approx(camera.tanHalfFovY * 10.0).epsilon(1e-5));

	// X-Plane's controls: strength x 4.3, fade-out 1 km -> 10 km, fade-in 0.5 m -> 2 m
	CHECK(c.effect[0] == Approx(2.1f));
	CHECK(c.effect[1] == Approx(1.4f * 4.3f));
	CHECK(c.effect[2] == Approx(2.4f));
	const auto fadeOut = [&c](const float z) { return std::clamp(z * c.fade[0] + c.fade[1], 0.0f, 1.0f); };
	const auto fadeIn = [&c](const float z) { return 1.0f - std::clamp(z * c.fade[2] + c.fade[3], 0.0f, 1.0f); };
	CHECK(fadeOut(500.0f) == Approx(1.0f));
	CHECK(fadeOut(5500.0f) == Approx(0.5f));
	CHECK(fadeOut(20000.0f) == Approx(0.0f));
	CHECK(fadeIn(0.25f) == Approx(0.0f));
	CHECK(fadeIn(1.25f) == Approx(0.5f));
	CHECK(fadeIn(10.0f) == Approx(1.0f));

	// AMD's derived values
	CHECK(c.sampling[1] == Approx(static_cast<float>(camera.tanHalfFovY) / (2.1f * 1.2f)));	// 1 / sampling radius near limit
	CHECK(c.sampling[2] == Approx(0.9992f));
	CHECK(c.sampling[3] == Approx(-1.0f / 2.1f));
	CHECK(c.adaptive[0] == Approx(9.0 / (240.0 * 135.0 * 255.0)));
	CHECK(c.adaptive[2] == Approx(0.0f));	// sharpness 1
	CHECK(c.bilateral[2] == Approx(3.0f));	// blur passes
	CHECK(c.perPassFullResUvOffset[3][0] == Approx(1.0f / 480.0f));
	CHECK(c.perPassFullResUvOffset[3][1] == Approx(1.0f / 270.0f));
	CHECK(c.output[1] == Approx(static_cast<float>(CacaoQuality::High)));
	CHECK(c.output[2] == Approx(1.0f));

	// The sampling pattern: a rotation and mirror scaled by 1 + (layer - 1.5 + (sub-pass - 2) / 5) x 0.07
	const CacaoVec4 &first = c.patternRotScale[0];
	CHECK(first[0] == Approx(0.867f));
	CHECK(first[1] == Approx(0.0f).margin(1e-6));
	CHECK(first[3] == Approx(-0.867f));
	for (uint32_t layer = 0; layer < CACAO_LAYERS; ++layer)
	{
		for (uint32_t subPass = 0; subPass < CACAO_PATTERN_SUBPASSES; ++subPass)
		{
			const CacaoVec4 &m = c.patternRotScale[layer * CACAO_PATTERN_SUBPASSES + subPass];
			const int b = std::array<int, 5>{ 0, 1, 4, 3, 2 }[subPass];
			const double scale = 1.0 + (layer - 1.5 + (b - 2.0) / 5.0) * 0.07;
			CHECK(m[0] * m[3] - m[1] * m[2] == Approx(-scale * scale).epsilon(1e-5));
		}
	}

	// The layout cacao.slang's std140 cbuffer reads (spirv-dis: member offsets 224, 288, 608, 656)
	CHECK(offsetof(CacaoConstants, output) == 208u);
	CHECK(offsetof(CacaoConstants, perPassFullResUvOffset) == 224u);
	CHECK(offsetof(CacaoConstants, patternRotScale) == 288u);
	CHECK(offsetof(CacaoConstants, normalsToView) == 608u);
	CHECK(offsetof(CacaoConstants, padding) == 656u);
}

TEST_CASE("Ambient occlusion: X-Plane's dynamic values follow the camera's height", "[environment][cacao]")
{
	using namespace SceneryEditorX;

	// X-Plane 12.4.4 (probe dumps): 2 m and 8 m above the ground radius 2.1 m and fade-in to 2 m, 600 m up 22 m and 100 m
	const CacaoSettings base;
	for (const double height : { 0.0, 2.0, 8.0 })
	{
		const CacaoSettings at = ApplyCacaoDynamicValues(base, height);
		CHECK(at.radius == Approx(2.1f));
		CHECK(at.fadeInToM == Approx(2.0f));
		CHECK(at.fadeInFromM == Approx(0.5f));
	}
	const CacaoSettings aerial = ApplyCacaoDynamicValues(base, 600.0);
	CHECK(aerial.radius == Approx(22.0f));
	CHECK(aerial.fadeInToM == Approx(100.0f));
	CHECK(aerial.shadowMultiplier == Approx(base.shadowMultiplier));

	// Monotonic in between, untouched with the control off
	CHECK(ApplyCacaoDynamicValues(base, 300.0).radius > ApplyCacaoDynamicValues(base, 100.0).radius);
	CacaoSettings fixed = base;
	fixed.dynamicValues = false;
	CHECK(ApplyCacaoDynamicValues(fixed, 600.0).radius == Approx(2.1f));
}

TEST_CASE("Ambient occlusion: CACAO's dispatches for X-Plane's presets", "[environment][cacao]")
{
	using namespace SceneryEditorX;

	const auto kernels = [](const std::vector<CacaoDispatch> &plan)
	{
		std::vector<CacaoKernel> result;
		for (const CacaoDispatch &d : plan)
			result.push_back(d.kernel);
		return result;
	};
	const CacaoBufferSizes half = ComputeCacaoBufferSizes(1920, 1080, true);

	// High at half resolution (X-Plane's default, renopt_SSAO_04 2)
	const std::vector<CacaoDispatch> high = PlanCacaoDispatches(CacaoQuality::High, half, 3);
	CHECK(kernels(high) == std::vector{ CacaoKernel::PrepareDepths, CacaoKernel::PrepareNormals, CacaoKernel::Generate, CacaoKernel::Blur, CacaoKernel::Upscale });
	CHECK(high[0].groupsX == 60u);
	CHECK(high[0].groupsY == 34u);
	CHECK(high[2].groupsZ == 4u);
	CHECK(high[3].groupsX == 9u);	// 480 / (64 - 2 x 3)
	CHECK(high[3].groupsY == 7u);	// 270 / (48 - 2 x 3)
	CHECK(high[4].groupsX == 120u);	// 2 x 2 output pixels per thread
	CHECK(high[4].groupsY == 68u);
	CHECK(high[4].readsBlurred);

	// Medium: the sparse kernel, every fifth pixel in 5 slices per layer
	const std::vector<CacaoDispatch> medium = PlanCacaoDispatches(CacaoQuality::Medium, half, 3);
	REQUIRE(medium.size() == 5u);
	CHECK(medium[2].groupsX == 24u);
	CHECK(medium[2].groupsY == 17u);
	CHECK(medium[2].groupsZ == 20u);

	// Highest: the base pass and the importance map before the adaptive pass
	CHECK(kernels(PlanCacaoDispatches(CacaoQuality::Highest, half, 3)) ==
		  std::vector{ CacaoKernel::PrepareDepths, CacaoKernel::PrepareNormals, CacaoKernel::GenerateBase, CacaoKernel::ImportanceMap, CacaoKernel::ImportanceMapA,
					   CacaoKernel::ImportanceMapB, CacaoKernel::Generate, CacaoKernel::Blur, CacaoKernel::Upscale });

	// Full resolution applies instead of upscaling; no blur reads the unblurred buffer
	const std::vector<CacaoDispatch> full = PlanCacaoDispatches(CacaoQuality::Highest, ComputeCacaoBufferSizes(1920, 1080, false), 0);
	CHECK(full.back().kernel == CacaoKernel::Apply);
	CHECK(full.back().groupsX == 240u);
	CHECK(full.back().groupsY == 135u);
	CHECK_FALSE(full.back().readsBlurred);
	CHECK(std::ranges::none_of(full, [](const CacaoDispatch &d) { return d.kernel == CacaoKernel::Blur; }));

	// X-Plane's presets: every enabled level runs CACAO 2-4 with the resolution it asks for; lower qualities run as medium
	for (int level = 1; level < AO_LEVELS; ++level)
	{
		const AoQuality quality = GetAoQuality(level);
		const CacaoQuality cacao = static_cast<CacaoQuality>(quality.cacaoQuality);
		CHECK(GetCacaoRunQuality(cacao) == cacao);
		const std::vector<CacaoDispatch> plan = PlanCacaoDispatches(cacao, ComputeCacaoBufferSizes(1280, 720, quality.halfResolution), 3);
		CHECK(plan.back().kernel == (quality.halfResolution ? CacaoKernel::Upscale : CacaoKernel::Apply));
	}
	CHECK(GetCacaoRunQuality(CacaoQuality::Lowest) == CacaoQuality::Medium);
	CHECK(GetAoQuality(1).cacaoQuality == 2);
	CHECK(GetAoQuality(3).cacaoQuality == 4);
}

namespace
{
	/**
	 * A camera 2 m above level ground looking along it at a wall 30 m ahead (view space: x right, y up, -z ahead), drawn
	 * into a reverse-Z depth buffer the way SEDX's projection writes it (horizontal field of view 90 degrees).
	 */
	struct SsrTestScene
	{
		static constexpr uint32_t WIDTH = 320;
		static constexpr uint32_t HEIGHT = 180;
		static constexpr float NEAR_PLANE = 0.5f;
		static constexpr float FAR_PLANE = 10000.0f;
		float xScale = 1.0f;
		float yScale = 1.0f;
		std::vector<float> depth;

		SsrTestScene()
		{
			yScale = xScale * static_cast<float>(WIDTH) / static_cast<float>(HEIGHT);
			depth.resize(static_cast<size_t>(WIDTH) * HEIGHT);
			for (uint32_t y = 0; y < HEIGHT; ++y)
			{
				for (uint32_t x = 0; x < WIDTH; ++x)
				{
					const float u = (static_cast<float>(x) + 0.5f) / WIDTH;
					const float v = (static_cast<float>(y) + 0.5f) / HEIGHT;
					depth[static_cast<size_t>(y) * WIDTH + x] = Cast(u, v);
				}
			}
		}

		static float DepthOf(const float distance)
		{
			return NEAR_PLANE * (FAR_PLANE - distance) / ((FAR_PLANE - NEAR_PLANE) * distance);
		}

		SceneryEditorX::SsrScreenVector Project(const float x, const float y, const float z) const
		{
			const float d = -z;
			return { xScale * x / d * 0.5f + 0.5f, -yScale * y / d * 0.5f + 0.5f, DepthOf(d) };
		}

		// The reverse-Z depth of the first surface through a screen point: the ground (y = -2) or the wall (z = -30, x
		// -10..10, y -2..6); 0 where there's nothing
		float Cast(const float u, const float v) const
		{
			const float dx = (u * 2.0f - 1.0f) / xScale;
			const float dy = (1.0f - v * 2.0f) / yScale;
			float distance = FLT_MAX;	// along -z (the view depth)
			if (dy < 0.0f)
				distance = -2.0f / dy;
			const float wallX = dx * 30.0f;
			const float wallY = dy * 30.0f;
			if (wallX >= -10.0f && wallX <= 10.0f && wallY >= -2.0f && wallY <= 6.0f)
				distance = std::min(distance, 30.0f);
			return distance < FAR_PLANE ? DepthOf(distance) : 0.0f;
		}

		static float DistanceOf(const float depth)
		{
			return NEAR_PLANE * FAR_PLANE / (NEAR_PLANE + depth * (FAR_PLANE - NEAR_PLANE));
		}

		/**
		 * A mirror ray off the ground the way ssr_common.slang starts one: from the centre of the pixel the ground point
		 * (x, -2, z) falls in, at the depth the buffer holds there, lifted SSR_ORIGIN_OFFSET_PIXELS pixel widths along the
		 * normal, toward the reflection of the view direction (a step of half the point's distance). surface: the reflecting
		 * point (view space) and the mirrored direction, for the expected hit.
		 */
		void GroundReflection(const float x, const float z, SceneryEditorX::SsrScreenVector &origin, SceneryEditorX::SsrScreenVector &direction,
							  float surface[3], float mirrored[3]) const
		{
			const SceneryEditorX::SsrScreenVector projected = Project(x, -2.0f, z);
			const uint32_t px = std::min(static_cast<uint32_t>(projected.u * WIDTH), WIDTH - 1);
			const uint32_t py = std::min(static_cast<uint32_t>(projected.v * HEIGHT), HEIGHT - 1);
			const float u = (static_cast<float>(px) + 0.5f) / WIDTH;
			const float v = (static_cast<float>(py) + 0.5f) / HEIGHT;
			const float d = DistanceOf(depth[static_cast<size_t>(py) * WIDTH + px]);
			surface[0] = (u * 2.0f - 1.0f) / xScale * d;
			surface[1] = (1.0f - v * 2.0f) / yScale * d;
			surface[2] = -d;

			const float length = std::sqrt(surface[0] * surface[0] + surface[1] * surface[1] + surface[2] * surface[2]);
			mirrored[0] = surface[0] / length;
			mirrored[1] = -surface[1] / length;
			mirrored[2] = surface[2] / length;

			const float lift = SceneryEditorX::SSR_ORIGIN_OFFSET_PIXELS * 2.0f * d / (yScale * HEIGHT);
			const float step = 0.5f * length;
			origin = Project(surface[0], surface[1] + lift, surface[2]);
			const SceneryEditorX::SsrScreenVector end = Project(surface[0] + mirrored[0] * step, surface[1] + lift + mirrored[1] * step, surface[2] + mirrored[2] * step);
			direction = { end.u - origin.u, end.v - origin.v, end.z - origin.z };
		}
	};
}

TEST_CASE("Screen-space reflections: gloss, weight and lobe footprint", "[environment][ssr]")
{
	using namespace SceneryEditorX;

	// X-Plane's gloss: roughness 1 - 0.96875 gloss; ssr/min_gloss 0.5 is roughness 0.515625
	CHECK(RoughnessFromGloss(0.5f) == Approx(0.515625f));
	CHECK(GlossFromRoughness(RoughnessFromGloss(0.73f)) == Approx(0.73f));
	CHECK(GlossFromRoughness(1.0f) == 0.0f);
	CHECK(GlossFromRoughness(0.0f) == 1.0f);

	// The weight: none below min_gloss, all of it from the fade's end, smooth between
	const SsrSettings settings;
	CHECK(ReflectionGlossWeight(RoughnessFromGloss(0.49f), settings) == 0.0f);
	CHECK(ReflectionGlossWeight(RoughnessFromGloss(0.55f), settings) == Approx(0.5f).margin(1e-4));
	CHECK(ReflectionGlossWeight(RoughnessFromGloss(0.6f), settings) == 1.0f);
	CHECK(ReflectionGlossWeight(0.0f, settings) == 1.0f);
	SsrSettings hard = settings;
	hard.glossFade = 0.0f;
	CHECK(ReflectionGlossWeight(RoughnessFromGloss(0.5f), hard) == 1.0f);

	// The lobe's history mip: 0 for mirrors and for footprints under a pixel, one more per doubling of distance
	CHECK(ReflectionConeMip(0.0f, 100.0f, 0.01f, settings) == 0.0f);
	CHECK(ReflectionConeMip(0.3f, 0.1f, 0.05f, settings) == 0.0f);
	const float tenMetres = ReflectionConeMip(0.4f, 10.0f, 0.01f, settings);
	CHECK(tenMetres > 1.0f);
	CHECK(ReflectionConeMip(0.4f, 20.0f, 0.01f, settings) == Approx(tenMetres + 1.0f));
	SsrSettings biased = settings;
	biased.lodBias = 0.5f;
	CHECK(ReflectionConeMip(0.4f, 10.0f, 0.01f, biased) == Approx(tenMetres + 0.5f));
	SsrSettings sharp = settings;
	sharp.usePrefiltered = false;
	CHECK(ReflectionConeMip(0.4f, 10.0f, 0.01f, sharp) == 0.0f);
}

TEST_CASE("Screen-space reflections: the min / max depth pyramid", "[environment][ssr]")
{
	using namespace SceneryEditorX;

	// The editor viewport's render size: 11 levels, halves rounded down, to 1 x 1
	const std::vector<PyramidLevel> levels = ComputeDepthPyramidLevels(1117, 642);
	REQUIRE(levels.size() == 11u);
	CHECK(levels[1].width == 558u);
	CHECK(levels[1].height == 321u);
	CHECK(levels[2].height == 160u);
	CHECK(levels[9].width == 2u);
	CHECK(levels[9].height == 1u);
	CHECK(levels[10].width == 1u);
	CHECK(levels[10].height == 1u);
	CHECK(ComputeDepthPyramidLevels(1, 1).size() == 1u);

	// Every level-0 texel lies within the farthest / nearest bounds of the texel min(p >> level, size - 1) on every level,
	// odd sizes included, and the bounds are reached
	constexpr uint32_t width = 37;
	constexpr uint32_t height = 23;
	std::vector<float> depth(static_cast<size_t>(width) * height);
	uint32_t state = 12345u;
	for (float &d : depth)
	{
		state = state * 1664525u + 1013904223u;
		d = static_cast<float>(state >> 8) / static_cast<float>(1u << 24);
	}
	const DepthPyramid pyramid = BuildDepthPyramid(depth, width, height);
	REQUIRE(pyramid.levels.size() == 6u);
	for (uint32_t level = 0; level < pyramid.levels.size(); ++level)
	{
		const PyramidLevel size = pyramid.levels[level];
		std::vector<float> lowest(static_cast<size_t>(size.width) * size.height, FLT_MAX);
		std::vector<float> highest(lowest.size(), -FLT_MAX);
		bool inside = true;
		for (uint32_t y = 0; y < height; ++y)
		{
			for (uint32_t x = 0; x < width; ++x)
			{
				const uint32_t tx = std::min(x >> level, size.width - 1);
				const uint32_t ty = std::min(y >> level, size.height - 1);
				const float d = depth[static_cast<size_t>(y) * width + x];
				inside = inside && pyramid.Farthest(level, static_cast<int32_t>(tx), static_cast<int32_t>(ty)) <= d &&
						 d <= pyramid.Nearest(level, static_cast<int32_t>(tx), static_cast<int32_t>(ty));
				const size_t t = static_cast<size_t>(ty) * size.width + tx;
				lowest[t] = std::min(lowest[t], d);
				highest[t] = std::max(highest[t], d);
			}
		}
		CHECK(inside);
		CHECK(pyramid.farthest[level] == lowest);
		CHECK(pyramid.nearest[level] == highest);
	}
}

TEST_CASE("Screen-space reflections: the hierarchical-Z march finds what a mirror sees", "[environment][ssr]")
{
	using namespace SceneryEditorX;

	const SsrTestScene scene;
	const DepthPyramid pyramid = BuildDepthPyramid(scene.depth, SsrTestScene::WIDTH, SsrTestScene::HEIGHT);
	const SsrSettings settings;
	const float texelU = 1.0f / SsrTestScene::WIDTH;
	const float texelV = 1.0f / SsrTestScene::HEIGHT;

	// Where the mirror at a ground point meets the wall plane (z = -30)
	const auto wall_hit = [](const float surface[3], const float mirrored[3], float &x, float &y)
	{
		const float k = (-30.0f - surface[2]) / mirrored[2];
		x = surface[0] + k * mirrored[0];
		y = surface[1] + k * mirrored[1];
	};

	// The ground near (3, -2, -12) mirrors the wall near (7.5, 1, -30): the march ends there, within a texel, in few steps
	{
		SsrScreenVector origin, direction;
		float surface[3], mirrored[3], wallX, wallY;
		scene.GroundReflection(3.0f, -12.0f, origin, direction, surface, mirrored);
		wall_hit(surface, mirrored, wallX, wallY);
		CHECK(wallX == Approx(7.5f).margin(0.2f));
		CHECK(wallY == Approx(1.0f).margin(0.2f));
		const SsrMarchResult march = MarchHiZ(pyramid, origin, direction, settings.maxSteps);
		const SsrScreenVector expected = scene.Project(wallX, wallY, -30.0f);
		REQUIRE(march.hit);
		CHECK(std::abs(march.position.u - expected.u) <= texelU);
		CHECK(std::abs(march.position.v - expected.v) <= texelV);
		CHECK(march.position.z == Approx(expected.z).epsilon(1e-3));
		CHECK(march.iterations < 40u);
	}

	// Mirrored points all over the ground in front of the wall: each ray ends on the wall where geometry says
	uint32_t checked = 0;
	for (float x = -6.0f; x <= 6.0f; x += 1.5f)
	{
		for (float z = -6.0f; z >= -26.0f; z -= 2.5f)
		{
			SsrScreenVector origin, direction;
			float surface[3], mirrored[3], wallX, wallY;
			scene.GroundReflection(x, z, origin, direction, surface, mirrored);
			wall_hit(surface, mirrored, wallX, wallY);
			if (wallX < -9.0f || wallX > 9.0f || wallY > 5.0f)
				continue;
			const SsrMarchResult march = MarchHiZ(pyramid, origin, direction, settings.maxSteps);
			const SsrScreenVector expected = scene.Project(wallX, wallY, -30.0f);
			INFO("ground point " << x << ", " << z);
			REQUIRE(march.hit);
			CHECK(std::abs(march.position.u - expected.u) <= 1.5f * texelU);
			CHECK(std::abs(march.position.v - expected.v) <= 1.5f * texelV);
			++checked;
		}
	}
	CHECK(checked > 20u);

	// Near the camera the mirror looks over the wall into the sky: the ray runs out past the far plane, no hit
	{
		SsrScreenVector origin, direction;
		float surface[3], mirrored[3];
		scene.GroundReflection(0.0f, -4.0f, origin, direction, surface, mirrored);
		const SsrMarchResult march = MarchHiZ(pyramid, origin, direction, settings.maxSteps);
		CHECK_FALSE(march.hit);
	}

	// The wall mirrors the space behind the camera: the ray comes toward it and leaves the screen, no hit
	{
		const SsrScreenVector origin = scene.Project(0.0f, 2.0f, -30.0f);
		const float length = std::sqrt(4.0f + 900.0f);
		const SsrScreenVector end = scene.Project(0.0f, 2.0f + 2.0f / length * 10.0f, -30.0f + 30.0f / length * 10.0f);
		const SsrMarchResult march = MarchHiZ(pyramid, origin, { end.u - origin.u, end.v - origin.v, end.z - origin.z }, settings.maxSteps);
		CHECK_FALSE(march.hit);
	}
}

// -------------------------------------------------------
// Water (renderer/ocean.*): the FFT ocean's sea state, spectrum and cascades, the CPU twin of its FFT, the water's optics
// -------------------------------------------------------

TEST_CASE("Water: the cascades hold every wave number once", "[environment][ocean]")
{
	using namespace SceneryEditorX;
	const OceanSettings settings;
	const std::array<OceanCascade, OCEAN_CASCADES> cascades = ComputeOceanCascades(settings);
	CHECK(cascades[0].kMin == 0.0f);
	for (uint32_t c = 0; c < OCEAN_CASCADES; ++c)
	{
		INFO("cascade " << c);
		CHECK(cascades[c].sizeM == settings.cascadeSizeM[c]);
		CHECK(cascades[c].kMin < cascades[c].kMax);
		// No wave shorter than two of its grid's cells
		CHECK(cascades[c].kMax <= OCEAN_PI * static_cast<float>(OCEAN_FFT_SIZE) / cascades[c].sizeM * 1.0001f);
		if (c > 0)
		{
			CHECK(cascades[c].kMin == cascades[c - 1].kMax);	// Contiguous: no band left out or counted twice
			// The smaller tile takes over a few waves across it: its longest wave fits its tile cascadeMinCycles times
			CHECK_THAT(2.0f * OCEAN_PI / cascades[c].kMin, Catch::Matchers::WithinRel(cascades[c].sizeM / settings.cascadeMinCycles, 1e-4f));
		}
	}
	// The last cascade ends at its Nyquist limit: ~5 cm waves for a 7 m tile of 256 cells
	CHECK_THAT(2.0f * OCEAN_PI / cascades[OCEAN_CASCADES - 1].kMax, Catch::Matchers::WithinRel(2.0f * 7.0f / 256.0f, 1e-4f));
	CHECK(OceanGridWaveNumber(1, 500.0f) == Approx(2.0f * OCEAN_PI / 500.0f));
	CHECK(OceanGridWaveNumber(255, 500.0f) == Approx(-2.0f * OCEAN_PI / 500.0f));	// The FFT order: the top half is negative
	CHECK(OceanGridWaveNumber(128, 500.0f) == Approx(-128.0f * 2.0f * OCEAN_PI / 500.0f));
}

TEST_CASE("Water: the sea state for a wave amplitude", "[environment][ocean]")
{
	using namespace SceneryEditorX;
	const OceanSettings settings;

	// From the west: the waves travel east (+x); from the north: south (+z)
	CHECK(ComputeOceanSpectrum(0.5f, 270.0f, settings).directionRad == Approx(0.0f).margin(1e-5f));
	CHECK(ComputeOceanSpectrum(0.5f, 0.0f, settings).directionRad == Approx(0.5f * OCEAN_PI).margin(1e-5f));

	// Height picks the fetch (fetch_lo_m .. fetch_hi_m, times fetch_multiplier) and the swell (0 .. max_swell)
	const OceanSpectrum calm = ComputeOceanSpectrum(0.05f, 270.0f, settings);
	CHECK(calm.significantHeightM == Approx(0.1f));
	CHECK(calm.fetchM == Approx(settings.fetchLoM * settings.fetchMultiplier));
	CHECK(calm.swell == 0.0f);
	const OceanSpectrum rough = ComputeOceanSpectrum(1.5f, 270.0f, settings);
	CHECK(rough.fetchM == Approx(settings.fetchHiM * settings.fetchMultiplier));
	CHECK(rough.swell == Approx(settings.maxSwell));

	// The wind raises that height over that fetch (JONSWAP: Hs = 0.0016 U sqrt(F / g)), never less than a fully developed
	// sea of that height needs (Hs = 0.21 U^2 / g)
	for (const float amplitude : { 0.1f, 0.25f, 0.5f, 1.0f })
	{
		const OceanSpectrum sea = ComputeOceanSpectrum(amplitude, 270.0f, settings);
		INFO("amplitude " << amplitude);
		const float fetchLimited = 0.0016f * sea.windSpeed * std::sqrt(sea.fetchM / OCEAN_GRAVITY);
		const float developed = 0.21f * sea.windSpeed * sea.windSpeed / OCEAN_GRAVITY;
		CHECK(std::max(fetchLimited, developed) >= sea.significantHeightM * 0.999f);
		CHECK(sea.peakOmega >= 0.855f * OCEAN_GRAVITY / sea.windSpeed * 0.999f);
		CHECK(sea.alpha > 0.0f);
	}
	// Longer waves for a higher sea
	CHECK(ComputeOceanSpectrum(1.0f, 270.0f, settings).peakOmega < ComputeOceanSpectrum(0.25f, 270.0f, settings).peakOmega);
	CHECK(ComputeOceanSpectrum(0.0f, 270.0f, settings).significantHeightM == 0.0f);
}

TEST_CASE("Water: JONSWAP, TMA and the dispersion relation", "[environment][ocean]")
{
	using namespace SceneryEditorX;
	const OceanSpectrum sea = ComputeOceanSpectrum(0.5f, 270.0f, OceanSettings());

	// The spectrum peaks at the peak frequency
	float best = 0.0f, bestOmega = 0.0f;
	for (float omega = 0.2f * sea.peakOmega; omega < 4.0f * sea.peakOmega; omega += 0.001f)
	{
		const float s = JonswapSpectrum(omega, sea);
		if (s > best)
		{
			best = s;
			bestOmega = omega;
		}
	}
	CHECK(bestOmega == Approx(sea.peakOmega).epsilon(0.01));
	CHECK(JonswapSpectrum(0.0f, sea) == 0.0f);

	// TMA: nothing changes in deep water, the low-frequency limit is w^2 h / 2g
	CHECK(TmaAttenuation(2.0f, 100.0f) == 1.0f);
	CHECK(TmaAttenuation(0.1f, 1.0f) == Approx(0.5f * 0.01f / OCEAN_GRAVITY));

	// Deep water: w^2 = g k and dw / dk = g / 2w; shallow water: w = k sqrt(g h)
	CHECK(OceanDispersion(1.0f, 1000.0f) == Approx(std::sqrt(OCEAN_GRAVITY)));
	CHECK(OceanDispersionDerivative(1.0f, 1000.0f) == Approx(OCEAN_GRAVITY / (2.0f * std::sqrt(OCEAN_GRAVITY))));
	CHECK(OceanDispersion(0.01f, 1.0f) == Approx(0.01f * std::sqrt(OCEAN_GRAVITY)).epsilon(0.001));
	// The derivative against a finite difference
	const float k = 0.3f, h = 5.0f;
	CHECK(OceanDispersionDerivative(k, h) == Approx((OceanDispersion(k + 1e-3f, h) - OceanDispersion(k - 1e-3f, h)) / 2e-3f).epsilon(0.002));
}

TEST_CASE("Water: directional spreading", "[environment][ocean]")
{
	using namespace SceneryEditorX;
	OceanSpectrum sea = ComputeOceanSpectrum(1.0f, 270.0f, OceanSettings());
	const auto integral = [&sea](const float omega)
	{
		double sum = 0.0;
		constexpr int steps = 4000;
		for (int i = 0; i < steps; ++i)
		{
			const float theta = -OCEAN_PI + (static_cast<float>(i) + 0.5f) * 2.0f * OCEAN_PI / steps;
			sum += OceanDirectionalSpread(omega, theta, sea) * 2.0 * OCEAN_PI / steps;
		}
		return sum;
	};
	for (const float ratio : { 0.5f, 1.0f, 2.0f, 5.0f })
	{
		INFO("w / wp " << ratio);
		const float omega = ratio * sea.peakOmega;
		CHECK(integral(omega) == Approx(1.0).epsilon(1e-3));
		// Most along the sea's direction, least against it
		CHECK(OceanDirectionalSpread(omega, 0.0f, sea) > OceanDirectionalSpread(omega, 0.5f * OCEAN_PI, sea));
		CHECK(OceanDirectionalSpread(omega, 0.5f * OCEAN_PI, sea) >= OceanDirectionalSpread(omega, OCEAN_PI, sea));
	}
	// Swell narrows the spreading; a spread blend of 0 spreads evenly
	OceanSpectrum swell = sea;
	swell.swell = 1.0f;
	sea.swell = 0.0f;
	CHECK(OceanDirectionalSpread(sea.peakOmega, 0.0f, swell) > OceanDirectionalSpread(sea.peakOmega, 0.0f, sea));
	sea.spreadBlend = 0.0f;
	CHECK(OceanDirectionalSpread(sea.peakOmega, 1.0f, sea) == Approx(1.0f / (2.0f * OCEAN_PI)));
}

TEST_CASE("Water: the cascades make the requested significant height", "[environment][ocean]")
{
	using namespace SceneryEditorX;
	const OceanSettings settings;
	const std::array<OceanCascade, OCEAN_CASCADES> cascades = ComputeOceanCascades(settings);
	for (const float amplitude : { 0.1f, 0.5f, 1.5f })
	{
		OceanSpectrum sea = ComputeOceanSpectrum(amplitude, 200.0f, settings);
		NormalizeOceanSpectrum(sea, cascades);
		INFO("amplitude " << amplitude);
		REQUIRE(sea.heightScale > 0.0f);
		CHECK(OceanCascadesSignificantHeight(sea, cascades) * sea.heightScale == Approx(sea.significantHeightM).epsilon(1e-4));
		CHECK(sea.heightScale == Approx(1.0f).margin(0.5f));	// The JONSWAP sea of that wind and fetch is about that high already
	}
	OceanSpectrum flat = ComputeOceanSpectrum(0.0f, 0.0f, settings);
	NormalizeOceanSpectrum(flat, cascades);
	CHECK(flat.heightScale == 0.0f);
}

TEST_CASE("Water: the inverse FFT", "[environment][ocean]")
{
	using namespace SceneryEditorX;
	CHECK(OceanBitReverse(1, 8) == 128u);
	CHECK(OceanBitReverse(6, 3) == 3u);

	// ocean.slang's 256-point transform against the direct sum (no 1 / n)
	std::vector<std::complex<float>> data(OCEAN_FFT_SIZE), direct(OCEAN_FFT_SIZE);
	for (uint32_t i = 0; i < OCEAN_FFT_SIZE; ++i)
		data[i] = { std::sin(0.37f * static_cast<float>(i)) + 0.25f, std::cos(1.3f * static_cast<float>(i * i % 17)) };
	for (uint32_t x = 0; x < OCEAN_FFT_SIZE; ++x)
	{
		std::complex<double> sum = 0.0;
		for (uint32_t k = 0; k < OCEAN_FFT_SIZE; ++k)
			sum += std::complex<double>(data[k]) * std::polar(1.0, 2.0 * std::numbers::pi * static_cast<double>(k * x % OCEAN_FFT_SIZE) / OCEAN_FFT_SIZE);
		direct[x] = std::complex<float>(sum);
	}
	OceanInverseFft(data);
	float error = 0.0f;
	for (uint32_t i = 0; i < OCEAN_FFT_SIZE; ++i)
		error = std::max(error, std::abs(data[i] - direct[i]));
	CHECK(error < 2e-3f);

	// A single wave number gives a real cosine when its conjugate partner is set (the surface is real)
	std::vector<std::complex<float>> wave(16);
	wave[3] = { 0.5f, 0.0f };
	wave[13] = { 0.5f, 0.0f };
	OceanInverseFft(wave);
	for (uint32_t x = 0; x < 16; ++x)
	{
		CHECK(wave[x].real() == Approx(std::cos(2.0f * OCEAN_PI * 3.0f * static_cast<float>(x) / 16.0f)).margin(1e-5f));
		CHECK(wave[x].imag() == Approx(0.0f).margin(1e-5f));
	}
}

TEST_CASE("Water: optics, wave scale and tessellation", "[environment][ocean]")
{
	using namespace SceneryEditorX;
	OceanSettings settings;

	// X-Plane's open water (any.png alpha 191) is clear for about 8 m; the floor fades to 1% there
	const float clarity = WaterClarityDistance(191.0f / 255.0f, settings);
	CHECK(clarity == Approx(settings.turbidity / (1.0f - 191.0f / 255.0f)));
	CHECK(WaterTransmittance(clarity, clarity, settings) == Approx(0.01f).epsilon(1e-3));
	CHECK(WaterTransmittance(0.0f, clarity, settings) == 1.0f);
	CHECK(WaterClarityDistance(1.0f, settings) >= settings.clarityMinM);
	settings.enableTurbidity = false;
	CHECK(WaterTransmittance(100.0f, clarity, settings) == 1.0f);

	// Ponds are calm, the open sea has the full height; the shore keeps still
	settings = OceanSettings();
	CHECK(WaterWaveScale(1.0f, 50.0f, settings) == Approx(1.0f));
	CHECK(WaterWaveScale(0.0f, 50.0f, settings) == Approx(settings.pondWaveScale));
	CHECK(WaterWaveScale(1.0f, 0.0f, settings) == 0.0f);
	CHECK(WaterWaveScale(1.0f, 0.5f * settings.shoreDepthM, settings) == Approx(0.5f));
	settings.useFetch = false;
	CHECK(WaterWaveScale(0.0f, 50.0f, settings) == Approx(1.0f));

	// Edges as long as tessellationPixels pixels stay whole; longer ones are cut, up to the hardware limit
	settings = OceanSettings();
	const float pixelAngle = 0.001f;
	CHECK(WaterTessellationFactor(settings.tessellationPixels * 0.1f, 100.0f, pixelAngle, settings) == Approx(1.0f));
	CHECK(WaterTessellationFactor(settings.tessellationPixels * 0.1f * 10.0f, 100.0f, pixelAngle, settings) == Approx(10.0f));
	CHECK(WaterTessellationFactor(64.0f, 0.5f, pixelAngle, settings) == settings.maxTessellation);
	CHECK(WaterTessellationFactor(1.0f, 10000.0f, pixelAngle, settings) == 1.0f);
}
