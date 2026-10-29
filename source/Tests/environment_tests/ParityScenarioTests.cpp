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
 * ParityScenarioTests.cpp
 * -------------------------------------------------------
 * Created: 20/10/2026
 * -------------------------------------------------------
 * The XP12 parity scenario files (environment/parity_scenario.*): defaults, dates and times, clouds, exposure locks,
 * errors, and the scenario set the reference probe and SEDX both render.
 * -------------------------------------------------------
 */
#include <catch2/catch_all.hpp>
#include <SceneryEditorX/environment/parity_scenario.h>
#include <algorithm>
#include <cctype>
#include <cmath>
#include <filesystem>
#include <string>
#include <utility>

using namespace SceneryEditorX::Environment;
using Catch::Approx;

TEST_CASE("Parity scenarios: dates and clock times", "[environment][scenario]")
{
	int y = 0, m = 0, d = 0;
	CHECK(ParseIsoDate("2026-07-15", y, m, d));
	CHECK((y == 2026 && m == 7 && d == 15));
	CHECK(ParseIsoDate("2024-02-29", y, m, d));		// Leap year
	CHECK_FALSE(ParseIsoDate("2026-02-29", y, m, d));
	CHECK_FALSE(ParseIsoDate("2026-13-01", y, m, d));
	CHECK_FALSE(ParseIsoDate("2026-7-15", y, m, d));
	CHECK_FALSE(ParseIsoDate("15/07/2026", y, m, d));

	double hours = 0.0;
	CHECK(ParseClockTime("12:30", hours));
	CHECK(hours == Approx(12.5));
	CHECK(ParseClockTime("03:57:21", hours));
	CHECK(hours == Approx(3.0 + 57.0 / 60.0 + 21.0 / 3600.0));
	CHECK(ParseClockTime("9:05", hours));
	CHECK(hours == Approx(9.0 + 5.0 / 60.0));
	CHECK(ParseClockTime("24:00", hours));
	CHECK_FALSE(ParseClockTime("24:30", hours));
	CHECK_FALSE(ParseClockTime("12", hours));
	CHECK_FALSE(ParseClockTime("12:60", hours));
	CHECK_FALSE(ParseClockTime("12:00:00:00", hours));
	CHECK_FALSE(ParseClockTime("ab:cd", hours));

	CHECK(DayOfYear(2026, 1, 1) == 1);
	CHECK(DayOfYear(2026, 7, 15) == 196);
	CHECK(DayOfYear(2024, 12, 31) == 366);
}

TEST_CASE("Parity scenarios: defaults apply under each entry", "[environment][scenario]")
{
	const char *json = R"({
		"xp_build": "12.4.4-b4",
		"defaults": { "fov": 50, "width": 1920, "height": 1080, "visibility_m": 40000, "settle_frames": 300 },
		"scenarios": [
			{ "id": "a", "icao": "KSEA", "lat": 47.45, "lon": -122.31, "date": "2026-01-15", "local_time": "12:00", "utc_offset": -8 },
			{ "id": "b", "icao": "YSSY", "fov": 70, "visibility_m": 800, "runway_friction": 5, "rain_percent": 0.6, "ev100_lock": 12.5,
			  "clouds": [ { "base_msl_m": 700, "top_msl_m": 1400, "coverage": 1.0, "type": 1 } ],
			  "ui_mask": [ [0.0, 0.9, 1.0, 1.0] ], "project": "tools/x.edX", "art_controls": { "ssao/enable": 0, "ssao/radius": 3.5 } }
		]
	})";

	ParityScenarioFile file;
	std::string error;
	REQUIRE(ParseParityScenarios(json, file, &error));
	CHECK(file.xpBuild == "12.4.4-b4");
	REQUIRE(file.scenarios.size() == 2);

	const ParityScenario &a = file.scenarios[0];
	CHECK(a.id == "a");
	CHECK(a.fovDeg == Approx(50.0f));
	CHECK(a.width == 1920);
	CHECK(a.height == 1080);
	CHECK(a.visibilityM == Approx(40000.0f));
	CHECK(a.settleFrames == 300);
	CHECK((a.year == 2026 && a.month == 1 && a.day == 15));
	CHECK(a.localTimeHours == Approx(12.0));
	CHECK(a.utcOffsetHours == Approx(-8.0f));
	CHECK_FALSE(a.ev100Lock.has_value());
	CHECK(a.clouds[0].coverage == 0.0f);

	const ParityScenario &b = file.scenarios[1];
	CHECK(b.fovDeg == Approx(70.0f));
	CHECK(b.visibilityM == Approx(800.0f));
	CHECK(b.runwayFriction == 5);
	CHECK(b.rainPercent == Approx(0.6f));
	REQUIRE(b.ev100Lock.has_value());
	CHECK(*b.ev100Lock == Approx(12.5f));
	CHECK(b.clouds[0].baseMslM == Approx(700.0f));
	CHECK(b.clouds[0].type == 1);
	REQUIRE(b.uiMask.size() == 1);
	CHECK(b.uiMask[0][1] == Approx(0.9f));
	CHECK(b.project == "tools/x.edX");
	CHECK(b.width == 1920);		// From the defaults
	REQUIRE(b.artControls.size() == 2);
	CHECK(std::ranges::count(b.artControls, std::pair<std::string, float>{ "ssao/enable", 0.0f }) == 1);
	CHECK(std::ranges::count(b.artControls, std::pair<std::string, float>{ "ssao/radius", 3.5f }) == 1);
	CHECK(a.artControls.empty());

	CHECK(FindParityScenario(file, "b") == &file.scenarios[1]);
	CHECK(FindParityScenario(file, "c") == nullptr);
}

TEST_CASE("Parity scenarios: problems are reported, nothing half-read is kept", "[environment][scenario]")
{
	ParityScenarioFile file;
	std::string error;

	CHECK_FALSE(ParseParityScenarios("{ not json", file, &error));
	CHECK(error.find("JSON") != std::string::npos);

	CHECK_FALSE(ParseParityScenarios(R"({ "scenarios": [ { "icao": "KSEA" } ] })", file, &error));
	CHECK(error.find("no \"id\"") != std::string::npos);

	CHECK_FALSE(ParseParityScenarios(R"({ "scenarios": [ { "id": "a" }, { "id": "a" } ] })", file, &error));
	CHECK(error.find("duplicate") != std::string::npos);

	CHECK_FALSE(ParseParityScenarios(R"({ "scenarios": [ { "id": "a", "date": "2026-02-30" } ] })", file, &error));
	CHECK(error.find("date") != std::string::npos);

	CHECK_FALSE(ParseParityScenarios(R"({ "scenarios": [ { "id": "a", "fov": "wide" } ] })", file, &error));
	CHECK(error.find("value type") != std::string::npos);

	CHECK_FALSE(ParseParityScenarios(R"({ "list": [] })", file, &error));
	CHECK(file.scenarios.empty());

	// Out-of-range surface values are clamped, not rejected
	REQUIRE(ParseParityScenarios(R"({ "scenarios": [ { "id": "a", "runway_friction": 40, "rain_percent": 2 } ] })", file, &error));
	CHECK(file.scenarios[0].runwayFriction == 15);
	CHECK(file.scenarios[0].rainPercent == Approx(1.0f));
}

TEST_CASE("Parity scenarios: the shipped scenario set", "[environment][scenario]")
{
	// tools/xp12_reference_probe/scenarios/xp12_parity.json, found from the working folder or the source tree
	std::filesystem::path path = "tools/xp12_reference_probe/scenarios/xp12_parity.json";
	for (int up = 0; up < 4 && !std::filesystem::exists(path); ++up)
		path = std::filesystem::path("..") / path;
	if (!std::filesystem::exists(path))
	{
		WARN("xp12_parity.json not found from the working folder: skipped");
		return;
	}

	ParityScenarioFile file;
	std::string error;
	REQUIRE(LoadParityScenarios(path, file, &error));
	CHECK(file.xpBuild == "12.4.4-b4");
	CHECK(file.scenarios.size() >= 40);
	for (const ParityScenario &scenario : file.scenarios)
	{
		INFO(scenario.id);
		CHECK(!scenario.icao.empty());
		CHECK(scenario.width == 1280);
		CHECK(scenario.height == 720);
		CHECK(std::abs(scenario.latitude) > 1.0);
	}
	// Every sweep entry has its locked-exposure twin (TASK-102)
	for (const char *id : { "ksea_rwy16l_sunp40", "ksea_aerial_sunm12", "ksea_calib_sunp20" })
	{
		INFO(id);
		const ParityScenario *autoExposure = FindParityScenario(file, id);
		const ParityScenario *locked = FindParityScenario(file, std::string(id) + "_ev");
		REQUIRE(autoExposure);
		REQUIRE(locked);
		CHECK_FALSE(autoExposure->ev100Lock.has_value());
		CHECK(locked->ev100Lock.has_value());
		CHECK(locked->localTimeHours == Approx(autoExposure->localTimeHours));
	}
	const ParityScenario *calibration = FindParityScenario(file, "ksea_calib_sunp40");
	REQUIRE(calibration);
	CHECK(calibration->project.find("SEDX_Calibration.edX") != std::string::npos);

	// Ambient occlusion A/B (plan CRIT-008): the same shot with ssao/enable 0
	for (const char *id : { "ksea_calib_sunp40", "ksea_aerial_sunp40" })
	{
		INFO(id);
		const ParityScenario *withAo = FindParityScenario(file, id);
		const ParityScenario *withoutAo = FindParityScenario(file, std::string(id) + "_noao");
		REQUIRE(withAo);
		REQUIRE(withoutAo);
		CHECK(withAo->artControls.empty());
		REQUIRE(withoutAo->artControls.size() == 1);
		CHECK(withoutAo->artControls[0].first == "ssao/enable");
		CHECK(withoutAo->artControls[0].second == 0.0f);
		CHECK(withoutAo->latitude == Approx(withAo->latitude));
		CHECK(withoutAo->localTimeHours == Approx(withAo->localTimeHours));
	}

	// Screen-space reflections A/B (plan CRIT-009): the same shot with debug/kill_ssr 1
	for (const char *id : { "ksea_calib_sunp40", "ksea_rwy16l_wet5" })
	{
		INFO(id);
		const ParityScenario *withSsr = FindParityScenario(file, id);
		const ParityScenario *withoutSsr = FindParityScenario(file, std::string(id) + "_nossr");
		REQUIRE(withSsr);
		REQUIRE(withoutSsr);
		CHECK(withSsr->artControls.empty());
		REQUIRE(withoutSsr->artControls.size() == 1);
		CHECK(withoutSsr->artControls[0].first == "debug/kill_ssr");
		CHECK(withoutSsr->artControls[0].second == 1.0f);
		CHECK(withoutSsr->runwayFriction == withSsr->runwayFriction);
		CHECK(withoutSsr->localTimeHours == Approx(withSsr->localTimeHours));
	}

	// Snow in winter: mid-January at local solar noon (12:00 PST), below freezing
	for (const char *id : { "ksea_rwy16l_snowy", "ksea_aerial_snowy", "ksea_calib_snowy" })
	{
		INFO(id);
		const ParityScenario *snowy = FindParityScenario(file, id);
		REQUIRE(snowy);
		CHECK(snowy->snowCover == Approx(1.0f));
		CHECK(snowy->month == 1);
		CHECK(snowy->localTimeHours == Approx(12.0));
		CHECK(snowy->utcOffsetHours == Approx(-8.0f));
		CHECK(snowy->temperatureC <= 0.0f);
	}
}

TEST_CASE("Parity scenarios: the test-airport set", "[environment][scenario]")
{
	// tools/xp12_reference_probe/scenarios/xp12_airports.json: the standard test airports' views
	std::filesystem::path path = "tools/xp12_reference_probe/scenarios/xp12_airports.json";
	for (int up = 0; up < 4 && !std::filesystem::exists(path); ++up)
		path = std::filesystem::path("..") / path;
	if (!std::filesystem::exists(path))
	{
		WARN("xp12_airports.json not found from the working folder: skipped");
		return;
	}

	ParityScenarioFile file;
	std::string error;
	REQUIRE(LoadParityScenarios(path, file, &error));
	CHECK(file.xpBuild == "12.4.4-b4");

	// Field elevation (m), for the overcast layer 600 m above each field; the winter shot's month (southern fields take
	// January as summer, July as winter), the hours daylight saving adds on the summer date, and whether snow lies there
	// in winter (a snowy shot)
	struct TestAirport
	{
		const char *icao;
		float elevationM;
		int winterMonth;
		float daylightSavingHours;
		bool snow;
	};
	const TestAirport airports[] = {
		{ "KGON", 2.1f, 1, 1.0f, true },	{ "KDEN", 1656.3f, 1, 1.0f, true }, { "KORD", 203.0f, 1, 1.0f, true },
		{ "KBOS", 5.8f, 1, 1.0f, true },	{ "KBTV", 101.5f, 1, 1.0f, true },	{ "EIDW", 73.8f, 1, 1.0f, true },
		{ "LEBL", 4.3f, 1, 1.0f, false },	{ "EGLL", 25.3f, 1, 1.0f, true },	{ "LSZH", 431.6f, 1, 1.0f, true },
		{ "OMDB", 18.9f, 1, 0.0f, false },	{ "RCTP", 32.3f, 1, 0.0f, false },	{ "RJTT", 6.1f, 1, 0.0f, true },
		{ "YSSY", 6.1f, 7, 1.0f, false },	{ "SBGR", 749.2f, 7, 0.0f, false }, { "CYYZ", 164.9f, 1, 1.0f, true },
	};
	for (const auto &[icao, elevationM, winterMonth, daylightSavingHours, snow] : airports)
	{
		std::string prefix = icao;
		std::ranges::transform(prefix, prefix.begin(), [](const char c) { return static_cast<char>(std::tolower(static_cast<unsigned char>(c))); });
		INFO(icao);
		for (const char *view : { "_rwy_sunp40", "_aerial_sunp40", "_aerial_sunp08" })
		{
			INFO(view);
			const ParityScenario *autoExposure = FindParityScenario(file, prefix + view);
			const ParityScenario *locked = FindParityScenario(file, prefix + view + "_ev");
			REQUIRE(autoExposure);
			REQUIRE(locked);
			CHECK(autoExposure->icao == icao);
			CHECK_FALSE(autoExposure->ev100Lock.has_value());
			CHECK(locked->ev100Lock.has_value());
			CHECK(locked->localTimeHours == Approx(autoExposure->localTimeHours));
		}
		const ParityScenario *runway = FindParityScenario(file, prefix + "_rwy_sunp40");
		REQUIRE(runway);
		CHECK(runway->altitudeMAgl == Approx(2.0f));
		const ParityScenario *overcast = FindParityScenario(file, prefix + "_aerial_overcast");
		REQUIRE(overcast);
		CHECK(overcast->clouds[0].coverage == Approx(1.0f));
		CHECK(overcast->clouds[0].baseMslM == Approx(elevationM + 600.0f).margin(1.0f));
		const ParityScenario *winter = FindParityScenario(file, prefix + (winterMonth == 1 ? "_aerial_season01" : "_aerial_season07"));
		REQUIRE(winter);
		CHECK(winter->month == winterMonth);
		CHECK(runway->month == (winterMonth == 1 ? 7 : 1));
		CHECK(winter->utcOffsetHours == Approx(runway->utcOffsetHours - daylightSavingHours));

		// Snow only where it lies, on the winter shot's date and time, below freezing at sea level and on the field (the
		// probe gives temperature_c to X-Plane as its sea-level temperature; SEDX melts snow above 0)
		const ParityScenario *snowy = FindParityScenario(file, prefix + "_aerial_snowy");
		if (!snow)
		{
			CHECK_FALSE(snowy);
			continue;
		}
		REQUIRE(snowy);
		CHECK(snowy->snowCover == Approx(1.0f));
		CHECK(snowy->month == winter->month);
		CHECK(snowy->day == winter->day);
		CHECK(snowy->localTimeHours == Approx(winter->localTimeHours));
		CHECK(snowy->utcOffsetHours == Approx(winter->utcOffsetHours));
		CHECK(snowy->temperatureC <= 0.0f);
		CHECK(snowy->temperatureC - 0.0065f * elevationM <= 0.0f);
	}

	// The coastal fields' low views over the water carry a sea state
	for (const char *id : { "kgon_water_sunp40", "kbos_water_sunp40", "lebl_water_sunp40", "rjtt_water_sunp40", "yssy_water_sunp40" })
	{
		INFO(id);
		const ParityScenario *water = FindParityScenario(file, id);
		REQUIRE(water);
		REQUIRE(water->waveAmplitudeM.has_value());
		CHECK(*water->waveAmplitudeM == Approx(0.6f));
		REQUIRE(water->waveDirectionDeg.has_value());
	}
	CHECK_FALSE(FindParityScenario(file, "kden_water_sunp40"));
}
