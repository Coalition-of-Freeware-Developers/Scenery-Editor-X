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
#include <SceneryEditorX/renderer/csm.h>
#include <SceneryEditorX/renderer/env_probes.h>
#include <SceneryEditorX/renderer/exposure.h>
#include <SceneryEditorX/renderer/renderer_features.h>
#include <SceneryEditorX/renderer/renderer_quality.h>
#include <cmath>
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
	CHECK(DecodeRunwayFriction(0) == SurfaceWeather{});
	CHECK(DecodeRunwayFriction(1).wetness == Approx(1.0f / 3.0f));
	CHECK(DecodeRunwayFriction(2).wetness == Approx(2.0f / 3.0f));
	CHECK(DecodeRunwayFriction(3).wetness == Approx(1.0f));
	for (int f = 4; f <= 6; ++f)
	{
		const SurfaceWeather s = DecodeRunwayFriction(f);
		CHECK(s.wetness == 1.0f);
		CHECK(s.puddles == Approx((f - 3) / 3.0f));
	}
	for (int f = 7; f <= 9; ++f)
		CHECK(DecodeRunwayFriction(f).snow == Approx((f - 6) / 3.0f));
	for (int f = 10; f <= 12; ++f)
		CHECK(DecodeRunwayFriction(f).ice == Approx((f - 9) / 3.0f));
	for (int f = 13; f <= 15; ++f)
	{
		const SurfaceWeather s = DecodeRunwayFriction(f);
		CHECK(s.snow == Approx((f - 12) / 3.0f));
		CHECK(s.ice == Approx((f - 12) / 3.0f));
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
	CHECK(d.surface.puddles == Approx(2.0f / 3.0f));
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

	// Thicker haze: less direct sun, never more light on the ground than arrives at the top
	double previousSun = sun;
	for (const double visibility : { 40000.0, 20000.0, 10000.0 })
	{
		const SkyLight hazier = ComputeSkyLight(WithVisibility(AtmosphereParams{}, visibility), 0.0, 60.0, 115000.0, -20.0, 0.25);
		const double direct = Lum(hazier.sunIlluminance);
		const double diffuse = Lum(hazier.irradiance);
		CHECK(direct < previousSun);
		CHECK(diffuse + direct * std::sin(60.0 * std::numbers::pi / 180.0) < 115000.0 * std::sin(60.0 * std::numbers::pi / 180.0));
		previousSun = direct;
	}
}

TEST_CASE("Atmosphere: visibility to haze", "[environment][atmosphere]")
{
	// X-Plane's scattering/vis_cutoff_ratio: 1% contrast at the visibility (Koschmieder's 2% on request)
	CHECK(VisibilityExtinction(10000.0) == Catch::Approx(4.60517e-4).epsilon(1e-5));
	CHECK(VisibilityExtinction(10000.0, 0.02) == Catch::Approx(3.912e-4).epsilon(1e-4));
	CHECK(VisibilityExtinction(0.0) == 0.0);

	// The haze layer takes what the clear layers don't give at the ground, scattering with atmo/fog_albedo
	const AtmosphereParams clear;
	const AtmosphereParams hazy = WithVisibility(clear, 40000.0);
	const double cutoff = -std::log(0.01);
	CHECK(hazy.hazeExtinction == Catch::Approx(cutoff / 40000.0 - 11.75e-6 - 4.44e-6).epsilon(1e-9));
	CHECK(hazy.hazeScatteringAlbedo == Catch::Approx(0.9));
	CHECK(hazy.mieExtinction == clear.mieExtinction);
	CHECK(WithVisibility(clear, 1.0e6).hazeExtinction == 0.0);	// never cleaner than clear air
	const MediumSample ground = SampleMedium(hazy, 0.0);
	CHECK(ground.Extinction()[1] == Catch::Approx(cutoff / 40000.0).epsilon(1e-6));	// green: the visibility

	// Mie and haze per channel: Angstrom -1.3 with the wavelengths the Rayleigh coefficients imply (blue scatters 1.32x)
	const Rgb spectral = MieSpectralFactors(clear.rayleighScattering, -1.3);
	CHECK(spectral[0] == Catch::Approx(0.786).margin(0.002));
	CHECK(spectral[1] == 1.0);
	CHECK(spectral[2] == Catch::Approx(1.321).margin(0.002));
	CHECK(spectral[2] == Catch::Approx(clear.mieSpectral[2]).margin(1e-3));
	CHECK(MieSpectralFactors(clear.rayleighScattering, 0.0)[2] == 1.0);
	CHECK(SampleMedium(clear, 0.0).mieExtinction[2] / SampleMedium(clear, 0.0).mieExtinction[1] == Catch::Approx(clear.mieSpectral[2]));
	CHECK(ground.mieExtinction[2] - ground.mieExtinction[1] == Catch::Approx(SampleMedium(clear, 0.0).mieExtinction[2] - SampleMedium(clear, 0.0).mieExtinction[1]));	// the haze is grey

	// The layer: uniform to its top above the ground, fading over the band, gone above it; on an elevated airport too
	const AtmosphereParams fog = WithVisibility(clear, 1000.0, 400.0);
	CHECK(fog.hazeTopM == Catch::Approx(1500.0));
	CHECK(HazeTopFromVisibility(200000.0) == Catch::Approx(4000.0));
	CHECK(HazeProfile(fog, 400.0) == 1.0);
	CHECK(HazeProfile(fog, 1899.0) == 1.0);
	CHECK(HazeProfile(fog, 1900.0 + fog.hazeBandM * 0.5) == Catch::Approx(0.5));
	CHECK(HazeProfile(fog, 1900.0 + fog.hazeBandM) == 0.0);
	CHECK(SampleMedium(fog, 400.0).Extinction()[1] == Catch::Approx(cutoff / 1000.0).epsilon(1e-6));
	CHECK(SampleMedium(fog, 5000.0).mieExtinction[1] < 1e-5);

	// Sky-condition multipliers scale the scattering, not the absorption
	const AtmosphereParams doubled = WithSkyMultipliers(hazy, 2.0, 2.0);
	CHECK(doubled.rayleighScattering[2] == Catch::Approx(2.0 * clear.rayleighScattering[2]));
	CHECK(doubled.mieScattering == Catch::Approx(2.0 * clear.mieScattering));
	CHECK(doubled.mieExtinction - doubled.mieScattering == Catch::Approx(clear.mieExtinction - clear.mieScattering));
	CHECK(doubled.hazeExtinction * doubled.hazeScatteringAlbedo == Catch::Approx(2.0 * hazy.hazeExtinction * hazy.hazeScatteringAlbedo));
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
