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
#include <SceneryEditorX/environment/environment.h>
#include <SceneryEditorX/environment/parity_scenario.h>
#include <cmath>
#include <numeric>

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
