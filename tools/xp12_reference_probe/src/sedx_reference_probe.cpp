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
 * sedx_reference_probe.cpp
 * -------------------------------------------------------
 * Created: 20/10/2026
 * -------------------------------------------------------
 *
 * SEDX Reference Probe: an X-Plane 12 plugin that renders the parity scenarios (scenarios.json, the same file SEDX
 * reads with --env-scenario) and dumps X-Plane's art-control values, so the editor's XP12 emulation can be compared
 * with and calibrated against X-Plane itself (plan TASK-001 / TASK-002 / TASK-102). A second set, airports.json
 * (scenarios/xp12_airports.json), takes the same kinds of views at the standard test airports.
 *
 * For each scenario it sets the date, time and weather through the public datarefs, takes the camera to a fixed pose,
 * waits settle_frames for exposure and weather to settle, saves a screenshot as Output/sedx_probe/<id>.png and writes
 * Output/sedx_probe/<id>.json with what it applied and the live value of every art control in art_controls.txt.
 *
 * Start it from Plugins > SEDX Reference Probe, or put a file named "autorun" (containing "quit" to exit X-Plane
 * afterwards, "airports" to run the test-airport set, "resume" to skip the scenarios already captured, "snow" to take
 * only the snowy scenarios again) into
 * Output/sedx_probe/.
 *
 * Reads and writes only public SDK datarefs plus the sim/private/controls/ values X-Plane exposes to plugins; it never
 * reads X-Plane's own files. Living Scenery Technology is switched off for a run and back on after it.
 */

#include <XPLMCamera.h>
#include <XPLMDataAccess.h>
#include <XPLMDisplay.h>
#include <XPLMGraphics.h>
#include <XPLMMenus.h>
#include <XPLMNavigation.h>
#include <XPLMPlanes.h>
#include <XPLMPlugin.h>
#include <XPLMProcessing.h>
#include <XPLMScenery.h>
#include <XPLMUtilities.h>

#include <algorithm>
#include <array>
#include <chrono>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <ctime>
#include <filesystem>
#include <fstream>
#include <optional>
#include <utility>
#include <sstream>
#include <string>
#include <vector>
#include <nlohmann/json.hpp>

// -------------------------------------------------------

namespace
{
	namespace fs = std::filesystem;
	using Json = nlohmann::ordered_json;

	constexpr const char *PROBE_NAME = "SEDX Reference Probe";
	constexpr const char *PROBE_SIGNATURE = "org.cofd.sedx.reference_probe";
	constexpr const char *PROBE_DESCRIPTION = "Renders Scenery Editor X parity scenarios and dumps art-control values.";

	constexpr int SCENERY_LOAD_FRAMES = 300;		// At another airport: terrain under the camera and X-Plane not loading, this long
	constexpr int SCENERY_TIMEOUT_FRAMES = 7200;	// Not loaded after this long: a new flight there (no terrain), or shoot anyway
	constexpr const char *SCENERY_LOADING = "sim/graphics/scenery/async_scenery_load_in_progress";
	// X-Plane writes a screenshot in 4 KB pieces between compression bursts, so a size that held still for 10 frames
	// (0.1 s at 100 fps) was no proof: four night shots of the first airport run were cut off at a multiple of 4096 bytes.
	// A screenshot is done when it ends in its IEND chunk and kept its size this many frames and this long.
	constexpr int SCREENSHOT_STABLE_FRAMES = 10;
	constexpr double SCREENSHOT_STABLE_SECONDS = 1.0;
	constexpr double SCREENSHOT_TIMEOUT_SECONDS = 60.0;
	// After the camera arrives X-Plane goes on loading LODs, textures and objects for a while (600 frames passed in about
	// 6 s at 100 fps), so a view settles at least this long and settle_frames frames, then until X-Plane hasn't loaded
	// scenery for SETTLE_QUIET_FRAMES frames in a row; still loading after SETTLE_TIMEOUT_SECONDS, the shot is taken anyway
	constexpr double SETTLE_SECONDS = 15.0;
	constexpr int SETTLE_QUIET_FRAMES = 120;
	constexpr double SETTLE_TIMEOUT_SECONDS = 180.0;
	// X-Plane builds a new ground snow cover over minutes, and its readback is no guide: in the first 15-airport run
	// region/snow_cover read 0 for the first two snowy shots after the probe set 1, and the terrain turned white in steps
	// every few seconds; in the snow-only run it stopped at 0.86 while the airport was fully white about 120 s after the
	// probe set 1 (the runway's own snow, wxr/snow_now, is there at once). So the wait is timed: when a scenario's snow
	// cover differs from the one the probe last let X-Plane build (at the start of a run: what X-Plane reads), the probe
	// sets it and the temperature again every frame for SNOW_BUILD_SECONDS, then settles; later scenarios with the same
	// cover don't wait, whatever X-Plane reads back.
	constexpr const char *SNOW_COVER = "sim/weather/region/snow_cover";
	constexpr const char *SEALEVEL_TEMPERATURE = "sim/weather/region/sealevel_temperature_c";
	constexpr float SNOW_COVER_TOLERANCE = 0.02f;
	constexpr double SNOW_BUILD_SECONDS = 150.0;
	// The status window (top left): shown while a run moves and settles, hidden this many frames before each screenshot
	// until the file is written (X-Plane's screenshots include plugin windows), "finished" stays up this long
	constexpr int STATUS_HIDDEN_FRAMES = 5;
	constexpr double STATUS_FINISHED_SECONDS = 20.0;
	constexpr int STATUS_WIDTH = 480;
	constexpr int STATUS_LINE_HEIGHT = 18;
	constexpr int STATUS_LINES = 4;
	constexpr double METERS_PER_STATUTE_MILE = 1609.344;
	constexpr double PI = 3.14159265358979323846;

	/* @brief A scenario file installed with the plugin (a copy in Output/sedx_probe/ overrides it for a run). */
	struct ScenarioSet
	{
		const char *file;
		const char *runAll;
		const char *runMissing;
		const char *runFirst;
		const char *runSnowy;
	};
	constexpr ScenarioSet SCENARIO_SETS[] = {
		{ "scenarios.json", "Run all parity scenarios (KSEA)", "Resume the parity scenarios (skip captured ones)", "Run the first parity scenario",
		  "Run the snowy parity scenarios only (retakes them)" },
		{ "airports.json", "Run all test-airport scenarios (15 airports)", "Resume the test-airport scenarios (skip captured ones)",
		  "Run the first test-airport scenario", "Run the snowy test-airport scenarios only (retakes them)" },
	};

	/**
	 * @brief What a run covers: every scenario, the ones without a screenshot and dump yet, the first one, or the snowy
	 * ones (taken again, without the rest of the set).
	 */
	enum class RunMode : intptr_t
	{
		All = 1,
		First = 2,
		Missing = 3,
		Snowy = 4
	};
	constexpr intptr_t MENU_STOP = 0xFF;	// Menu item refs: set index << 8 | RunMode; MENU_STOP stops a run

	// The user aircraft is held out of the shot through these (see HoldUser)
	constexpr const char *OVERRIDE_PLANE_PATH = "sim/operation/override/override_planepath";
	constexpr double USER_BEHIND_CAMERA_M = 6000.0;
	constexpr double USER_ABOVE_CAMERA_M = 1200.0;

	// Plugins switched off for a run (see DisablePlugins): Living Scenery Technology
	constexpr const char *PLUGINS_OFF_DURING_RUN[] = { "com.x-codrdesigns.livingscenerytech" };

	// -------------------------------------------------------

	struct CloudLayer
	{
		float baseMslM = 0.0f;
		float topMslM = 0.0f;
		float coverage = 0.0f;
		float type = 0.0f;
	};

	/* @brief One entry of scenarios.json (the schema of tools/xp12_reference_probe/scenarios/xp12_parity.json). */
	struct Scenario
	{
		std::string id;
		std::string icao;
		double lat = 0.0, lon = 0.0;
		float altMAgl = 2.0f, heading = 0.0f, pitch = 0.0f, fov = 60.0f;
		int month = 7, day = 15, year = 2026;
		std::optional<std::array<int, 3>> sceneryDate;	// "scenery_date": the date the airport's scenery loads on (else the date)
		double localTimeHours = 12.0;
		float utcOffset = 0.0f;
		float visibilityM = 80000.0f;
		std::vector<CloudLayer> clouds;
		float rainPercent = 0.0f, snowCover = 0.0f, temperatureC = 15.0f;
		float runwayFriction = 0.0f;
		std::optional<float> waveAmplitudeM;	// sim/weather/region/wave_amplitude; none: X-Plane's own sea
		std::optional<float> waveDirectionDeg;	// sim/weather/region/wave_dir
		int settleFrames = 600;
		std::optional<float> ev100Lock;
		std::vector<std::pair<std::string, float>> artControls;	// "art_controls": set for this shot only (sim/private/controls/)
		Json source;		// The entry as read, echoed into the dump
	};

	enum class Stage
	{
		Idle,
		MoveToAirport,		// Aircraft and camera moved to the airport, waiting for its scenery
		WaitSnow,			// Camera, time and weather set; letting X-Plane build a new snow cover (SNOW_BUILD_SECONDS)
		Settle,				// Camera, time and weather set; exposure, weather and scenery settling
		HideStatus,			// Status window hidden; a few frames before the screenshot so it isn't in it
		WaitScreenshot,		// Screenshot command sent; waiting for the file to be complete
		Finished
	};

	using Clock = std::chrono::steady_clock;

	struct Probe
	{
		XPLMMenuID menu = nullptr;
		XPLMWindowID statusWindow = nullptr;
		XPLMProbeRef terrain = nullptr;
		fs::path pluginFolder;		// SEDXReferenceProbe/ (holding 64/)
		fs::path xplaneFolder;
		fs::path outputFolder;		// Output/sedx_probe/
		std::ofstream log;

		std::vector<Scenario> scenarios;
		std::string runFile;		// The scenario set being run (scenarios.json, airports.json)
		size_t index = 0;
		bool runAll = true;
		bool quitWhenDone = false;
		Stage stage = Stage::Idle;
		int framesLeft = 0;
		Clock::time_point stageStart{};		// When the move, the settling or the screenshot began
		Clock::time_point finishedAt{};
		int quietFrames = 0;				// Frames in a row X-Plane loaded no scenery while settling
		double settledSeconds = 0.0;		// How long the shot's view settled
		bool settledLoading = false;		// X-Plane was still loading scenery when the shot was taken
		double snowWaitSeconds = 0.0;		// How long the shot waited for X-Plane's snow to build (0: no change)
		std::optional<float> builtSnowCover;	// The snow cover X-Plane last had time to build (see SNOW_BUILD_SECONDS)
		int sceneryTimeout = 0;		// Frames left for the scenery of the airport being moved to
		bool newFlight = false;		// The move fell back to a new flight at that airport
		std::string loadedAirport;
		std::string sceneryDate;	// The date the loaded airport's scenery was loaded on ("YYYY-MM-DD"; empty: not by this run)
		bool reloadPending = false;	// XPLMReloadScenery() in the next frame (see Stage::MoveToAirport)

		// Camera pose in world coordinates, resolved to local coordinates every frame
		double cameraLat = 0.0, cameraLon = 0.0, cameraMsl = 0.0;
		float cameraHeading = 0.0f, cameraPitch = 0.0f;
		bool cameraActive = false;

		// Where the user aircraft is held (behind and above the camera), while the run overrides its flight path
		double userLat = 0.0, userLon = 0.0, userMsl = 0.0;
		float userHeading = 0.0f;
		bool userHeld = false;

		// Screenshot detection
		fs::file_time_type screenshotsBefore{};
		fs::path screenshotFile;
		std::uintmax_t screenshotSize = 0;
		int screenshotStable = 0;
		Clock::time_point screenshotStableSince{};

		// Values to restore after the run
		std::optional<float> savedEv100Min, savedEv100Max, savedFov;
		std::vector<std::pair<std::string, float>> savedArtControls;	// What the current shot's art_controls replaced
		std::optional<int> savedUseSystemTime;
		std::optional<int> savedOverridePlanePath;
		bool disablePluginsPending = false;		// Switched off in the run's first flight loop
		std::vector<std::pair<XPLMPluginID, std::string>> disabledPlugins;	// To switch back on (id, signature)
		std::vector<std::string> statusLines;	// What the status window shows, built by the flight loop

		std::vector<std::string> artControls;
	};

	Probe g;

	// -------------------------------------------------------

	void Log(const std::string &message)
	{
		const std::string line = std::string("SEDX Probe: ") + message + "\n";
		XPLMDebugString(line.c_str());
		if (g.log.is_open())
		{
			// probe_log.txt lines carry the local time, so slow moves and settles show
			const std::time_t now = std::time(nullptr);
			std::tm local{};
#ifdef _WIN32
			localtime_s(&local, &now);
#else
			localtime_r(&now, &local);
#endif
			char stamp[16] = {};
			std::strftime(stamp, sizeof(stamp), "%H:%M:%S ", &local);
			g.log << stamp << message << '\n';
			g.log.flush();
		}
	}

	double SecondsSince(const Clock::time_point start)
	{
		return std::chrono::duration<double>(Clock::now() - start).count();
	}

	XPLMDataRef Find(const char *name)
	{
		return XPLMFindDataRef(name);
	}

	bool SetFloat(const char *name, const float value)
	{
		XPLMDataRef ref = Find(name);
		if (!ref || !XPLMCanWriteDataRef(ref))
			return false;
		const XPLMDataTypeID types = XPLMGetDataRefTypes(ref);
		if (types & xplmType_Float)
			XPLMSetDataf(ref, value);
		else if (types & xplmType_Double)
			XPLMSetDatad(ref, value);
		else if (types & xplmType_Int)
			XPLMSetDatai(ref, static_cast<int>(std::lround(value)));
		else
			return false;
		return true;
	}

	bool SetInt(const char *name, const int value)
	{
		return SetFloat(name, static_cast<float>(value));
	}

	bool SetDouble(const char *name, const double value)
	{
		XPLMDataRef ref = Find(name);
		if (!ref || !XPLMCanWriteDataRef(ref))
			return false;
		const XPLMDataTypeID types = XPLMGetDataRefTypes(ref);
		if (types & xplmType_Double)
			XPLMSetDatad(ref, value);
		else if (types & xplmType_Float)
			XPLMSetDataf(ref, static_cast<float>(value));
		else
			return false;
		return true;
	}

	/* @brief One element of an int array dataref; nullopt where there is no such array. */
	std::optional<int> GetIntElement(const char *name, const int index)
	{
		XPLMDataRef ref = Find(name);
		if (!ref || !(XPLMGetDataRefTypes(ref) & xplmType_IntArray) || XPLMGetDatavi(ref, nullptr, 0, 0) <= index)
			return std::nullopt;
		int value = 0;
		XPLMGetDatavi(ref, &value, index, 1);
		return value;
	}

	bool SetIntElement(const char *name, const int index, int value)
	{
		XPLMDataRef ref = Find(name);
		if (!ref || !XPLMCanWriteDataRef(ref) || !(XPLMGetDataRefTypes(ref) & xplmType_IntArray))
			return false;
		XPLMSetDatavi(ref, &value, index, 1);
		return true;
	}

	std::optional<float> GetFloat(const char *name)
	{
		XPLMDataRef ref = Find(name);
		if (!ref)
			return std::nullopt;
		const XPLMDataTypeID types = XPLMGetDataRefTypes(ref);
		if (types & xplmType_Float)
			return XPLMGetDataf(ref);
		if (types & xplmType_Double)
			return static_cast<float>(XPLMGetDatad(ref));
		if (types & xplmType_Int)
			return static_cast<float>(XPLMGetDatai(ref));
		return std::nullopt;
	}

	bool SetFloatArray(const char *name, const std::vector<float> &values)
	{
		XPLMDataRef ref = Find(name);
		if (!ref || !XPLMCanWriteDataRef(ref) || !(XPLMGetDataRefTypes(ref) & xplmType_FloatArray))
			return false;
		XPLMSetDatavf(ref, const_cast<float *>(values.data()), 0, static_cast<int>(values.size()));
		return true;
	}

	/* @brief The current value of a dataref as JSON (number, array, or string for byte data). */
	Json ReadValue(XPLMDataRef ref)
	{
		const XPLMDataTypeID types = XPLMGetDataRefTypes(ref);
		if (types & xplmType_Double)
			return XPLMGetDatad(ref);
		if (types & xplmType_Float)
			return XPLMGetDataf(ref);
		if (types & xplmType_Int)
			return XPLMGetDatai(ref);
		if (types & xplmType_FloatArray)
		{
			const int count = std::min(XPLMGetDatavf(ref, nullptr, 0, 0), 4096);
			std::vector<float> values(static_cast<size_t>(std::max(count, 0)));
			if (count > 0)
				XPLMGetDatavf(ref, values.data(), 0, count);
			return values;
		}
		if (types & xplmType_IntArray)
		{
			const int count = std::min(XPLMGetDatavi(ref, nullptr, 0, 0), 4096);
			std::vector<int> values(static_cast<size_t>(std::max(count, 0)));
			if (count > 0)
				XPLMGetDatavi(ref, values.data(), 0, count);
			return values;
		}
		if (types & xplmType_Data)
		{
			const int count = std::min(XPLMGetDatab(ref, nullptr, 0, 0), 4096);
			std::string bytes(static_cast<size_t>(std::max(count, 0)), '\0');
			if (count > 0)
				XPLMGetDatab(ref, bytes.data(), 0, count);
			bytes.erase(std::find(bytes.begin(), bytes.end(), '\0'), bytes.end());
			return bytes;
		}
		return nullptr;
	}

	// -------------------------------------------------------

	bool ParseDate(const std::string &text, int &year, int &month, int &day)
	{
		return std::sscanf(text.c_str(), "%d-%d-%d", &year, &month, &day) == 3 && month >= 1 && month <= 12 && day >= 1 && day <= 31;
	}

	bool ParseTime(const std::string &text, double &hours)
	{
		int h = 0, m = 0, s = 0;
		const int fields = std::sscanf(text.c_str(), "%d:%d:%d", &h, &m, &s);
		if (fields < 2)
			return false;
		hours = h + m / 60.0 + s / 3600.0;
		return hours >= 0.0 && hours <= 24.0;
	}

	int DayOfYearZeroBased(const int year, const int month, const int day)
	{
		static constexpr int days[12] = { 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 };
		const bool leap = (year % 4 == 0 && year % 100 != 0) || year % 400 == 0;
		int total = day - 1;
		for (int m = 1; m < month; ++m)
			total += days[m - 1] + (m == 2 && leap ? 1 : 0);
		return total;
	}

	template <typename T>
	void ReadField(const Json &object, const char *key, T &value)
	{
		const auto it = object.find(key);
		if (it != object.end() && !it->is_null())
			value = it->get<T>();
	}

	void ApplyFields(const Json &object, Scenario &s)
	{
		ReadField(object, "id", s.id);
		ReadField(object, "icao", s.icao);
		ReadField(object, "lat", s.lat);
		ReadField(object, "lon", s.lon);
		ReadField(object, "alt_m_agl", s.altMAgl);
		ReadField(object, "heading", s.heading);
		ReadField(object, "pitch", s.pitch);
		ReadField(object, "fov", s.fov);
		ReadField(object, "utc_offset", s.utcOffset);
		ReadField(object, "visibility_m", s.visibilityM);
		ReadField(object, "rain_percent", s.rainPercent);
		ReadField(object, "snow_cover", s.snowCover);
		ReadField(object, "temperature_c", s.temperatureC);
		ReadField(object, "runway_friction", s.runwayFriction);
		ReadField(object, "settle_frames", s.settleFrames);
		if (const auto it = object.find("date"); it != object.end() && it->is_string())
			ParseDate(it->get<std::string>(), s.year, s.month, s.day);
		if (const auto it = object.find("scenery_date"); it != object.end())
		{
			std::array<int, 3> date{};
			if (it->is_string() && ParseDate(it->get<std::string>(), date[0], date[1], date[2]))
				s.sceneryDate = date;
			else
				s.sceneryDate.reset();
		}
		if (const auto it = object.find("local_time"); it != object.end() && it->is_string())
			ParseTime(it->get<std::string>(), s.localTimeHours);
		if (const auto it = object.find("ev100_lock"); it != object.end())
			s.ev100Lock = it->is_null() ? std::nullopt : std::optional<float>(it->get<float>());
		if (const auto it = object.find("wave_amplitude_m"); it != object.end())
			s.waveAmplitudeM = it->is_null() ? std::nullopt : std::optional<float>(it->get<float>());
		if (const auto it = object.find("wave_direction_deg"); it != object.end())
			s.waveDirectionDeg = it->is_null() ? std::nullopt : std::optional<float>(it->get<float>());
		if (const auto it = object.find("art_controls"); it != object.end() && it->is_object())
		{
			s.artControls.clear();
			for (const auto &[name, value] : it->items())
			{
				if (value.is_number())
					s.artControls.emplace_back(name, value.get<float>());
			}
		}
		if (const auto it = object.find("clouds"); it != object.end() && it->is_array())
		{
			s.clouds.clear();
			for (const Json &layer : *it)
			{
				CloudLayer cloud;
				ReadField(layer, "base_msl_m", cloud.baseMslM);
				ReadField(layer, "top_msl_m", cloud.topMslM);
				ReadField(layer, "coverage", cloud.coverage);
				ReadField(layer, "type", cloud.type);
				s.clouds.push_back(cloud);
			}
		}
	}

	/* @brief A snow scenario: snow cover, or rain below freezing (the last group of a run, and a snowy-only run's set). */
	bool Snowy(const Scenario &s)
	{
		return s.snowCover > 0.0f || (s.temperatureC <= 0.0f && s.rainPercent > 0.0f);
	}

	/**
	 * @brief Weather X-Plane can't take back at once runs last: snow cover melts and wet runways dry over minutes of sim
	 * time, and the first run's calibration and season shots, taken after the snowy ones, were still white. Clear and
	 * dry scenarios first, then haze, fog and overcast, then rain and wet runways, then snow. Within each group an
	 * airport's scenarios run together (airports in the order the file first names them, then file order), so X-Plane
	 * loads each airport's scenery once per group instead of flying back and forth.
	 */
	void OrderForRun(std::vector<Scenario> &scenarios)
	{
		const auto rank = [](const Scenario &s)
		{
			if (Snowy(s))
				return 3;
			if (s.rainPercent > 0.0f || s.runwayFriction > 0.0f)
				return 2;
			bool cloudy = false;
			for (const CloudLayer &layer : s.clouds)
				cloudy |= layer.coverage > 0.0f;
			return (cloudy || s.visibilityM < 50000.0f) ? 1 : 0;
		};
		std::vector<std::string> airports;
		for (const Scenario &s : scenarios)
		{
			if (std::find(airports.begin(), airports.end(), s.icao) == airports.end())
				airports.push_back(s.icao);
		}
		const auto airport = [&](const Scenario &s) { return std::find(airports.begin(), airports.end(), s.icao) - airports.begin(); };
		std::stable_sort(scenarios.begin(), scenarios.end(), [&](const Scenario &a, const Scenario &b)
		{
			const int rankA = rank(a), rankB = rank(b);
			return rankA != rankB ? rankA < rankB : airport(a) < airport(b);
		});
	}

	bool LoadScenarios(const std::string &fileName)
	{
		g.scenarios.clear();
		// Output/sedx_probe/<file> first (per-run sets), then the copy installed with the plugin
		for (const fs::path &path : { g.outputFolder / fileName, g.pluginFolder / fileName })
		{
			std::ifstream file(path);
			if (!file)
				continue;
			try
			{
				const Json root = Json::parse(file);
				Scenario defaults;
				if (const auto it = root.find("defaults"); it != root.end())
					ApplyFields(*it, defaults);
				for (const Json &entry : root.at("scenarios"))
				{
					Scenario s = defaults;
					s.id.clear();
					ApplyFields(entry, s);
					s.source = entry;
					if (!s.id.empty())
						g.scenarios.push_back(std::move(s));
				}
				OrderForRun(g.scenarios);
				Log("Loaded " + std::to_string(g.scenarios.size()) + " scenarios from " + path.string());
				return !g.scenarios.empty();
			}
			catch (const std::exception &e)
			{
				Log("Could not read " + path.string() + ": " + e.what());
				return false;
			}
		}
		Log("No " + fileName + " in " + g.outputFolder.string() + " or " + g.pluginFolder.string());
		return false;
	}

	void LoadArtControls()
	{
		g.artControls.clear();
		for (const fs::path &path : { g.outputFolder / "art_controls.txt", g.pluginFolder / "art_controls.txt" })
		{
			std::ifstream file(path);
			if (!file)
				continue;
			std::string line;
			while (std::getline(file, line))
			{
				while (!line.empty() && (line.back() == '\r' || line.back() == ' ' || line.back() == '\t'))
					line.pop_back();
				if (line.empty() || line[0] == '#')
					continue;
				g.artControls.push_back(line);
			}
			Log("Art-control names: " + std::to_string(g.artControls.size()) + " from " + path.string());
			return;
		}
		Log("No art_controls.txt: the dumps will hold no art-control values");
	}

	// -------------------------------------------------------

	/* @brief Ground elevation (MSL meters) under a point, from X-Plane's terrain; nullopt where no terrain is loaded. */
	std::optional<double> GroundMsl(const double lat, const double lon)
	{
		if (!g.terrain)
			return std::nullopt;
		double x = 0.0, y = 0.0, z = 0.0;
		XPLMWorldToLocal(lat, lon, 0.0, &x, &y, &z);
		XPLMProbeInfo_t info{};
		info.structSize = sizeof(info);
		if (XPLMProbeTerrainXYZ(g.terrain, static_cast<float>(x), static_cast<float>(y), static_cast<float>(z), &info) != xplm_ProbeHitTerrain)
			return std::nullopt;
		double hitLat = 0.0, hitLon = 0.0, hitAlt = 0.0;
		XPLMLocalToWorld(info.locationX, info.locationY, info.locationZ, &hitLat, &hitLon, &hitAlt);
		return hitAlt;
	}

	int CameraCallback(XPLMCameraPosition_t *position, const int isLosingControl, void *)
	{
		if (isLosingControl || !position || !g.cameraActive)
		{
			g.cameraActive = false;
			return 0;
		}
		double x = 0.0, y = 0.0, z = 0.0;
		XPLMWorldToLocal(g.cameraLat, g.cameraLon, g.cameraMsl, &x, &y, &z);
		position->x = static_cast<float>(x);
		position->y = static_cast<float>(y);
		position->z = static_cast<float>(z);
		position->heading = g.cameraHeading;
		position->pitch = g.cameraPitch;
		position->roll = 0.0f;
		position->zoom = 1.0f;
		return 1;
	}

	/* @brief A point some distance from (lat, lon) along a true bearing (spherical earth; meters). */
	void Offset(const double lat, const double lon, const double bearingDeg, const double meters, double &outLat, double &outLon)
	{
		const double r = 6371000.0;
		const double d = meters / r;
		const double b = bearingDeg * PI / 180.0;
		const double p1 = lat * PI / 180.0;
		const double l1 = lon * PI / 180.0;
		const double p2 = std::asin(std::sin(p1) * std::cos(d) + std::cos(p1) * std::sin(d) * std::cos(b));
		const double l2 = l1 + std::atan2(std::sin(b) * std::sin(d) * std::cos(p1), std::cos(d) - std::sin(p1) * std::sin(p2));
		outLat = p2 * 180.0 / PI;
		outLon = l2 * 180.0 / PI;
	}

	void ApplyDate(const std::array<int, 3> &date)
	{
		SetInt("sim/time/use_system_time", 0);
		SetInt("sim/time/local_date_days", DayOfYearZeroBased(date[0], date[1], date[2]));
	}

	void ApplyTime(const Scenario &s)
	{
		double zulu = std::fmod(s.localTimeHours - s.utcOffset, 24.0);
		if (zulu < 0.0)
			zulu += 24.0;
		ApplyDate({ s.year, s.month, s.day });
		SetFloat("sim/time/zulu_time_sec", static_cast<float>(zulu * 3600.0));
	}

	/* @brief The date the scenario's scenery loads on: its "scenery_date", else its date. */
	std::array<int, 3> SceneryDateOf(const Scenario &s)
	{
		return s.sceneryDate.value_or(std::array<int, 3>{ s.year, s.month, s.day });
	}

	std::string FormatDate(const std::array<int, 3> &date)
	{
		char text[16] = {};
		std::snprintf(text, sizeof(text), "%04d-%02d-%02d", date[0], date[1], date[2]);
		return text;
	}

	void ApplyExposureLock(const Scenario &s)
	{
		if (!s.ev100Lock)
			return;
		// Clamp auto exposure to the locked value. photometric/ev100 itself is auto exposure's output: once a plugin writes
		// it, X-Plane stops driving it, and every later scenario would be shot at that value (the first run's references
		// were), so it is only read, never written.
		SetFloat("sim/private/controls/photometric/ev100_min", *s.ev100Lock);
		SetFloat("sim/private/controls/photometric/ev100_max", *s.ev100Lock);
	}

	/* @brief The shot's art controls (an A/B such as ssao/enable 0), keeping what they replaced for RestoreArtControls. */
	void ApplyArtControls(const Scenario &s)
	{
		for (const auto &[name, value] : s.artControls)
		{
			const std::string path = "sim/private/controls/" + name;
			const std::optional<float> previous = GetFloat(path.c_str());
			if (!previous || !SetFloat(path.c_str(), value))
			{
				Log(s.id + ": art control " + name + " is not writable here, left as it is");
				continue;
			}
			g.savedArtControls.emplace_back(path, *previous);
		}
	}

	void RestoreArtControls()
	{
		for (const auto &[path, value] : g.savedArtControls)
			SetFloat(path.c_str(), value);
		g.savedArtControls.clear();
	}

	void ApplyWeather(const Scenario &s)
	{
		SetInt("sim/weather/region/change_mode", 3);	// Static
		SetFloat("sim/weather/region/visibility_reported_sm", static_cast<float>(s.visibilityM / METERS_PER_STATUTE_MILE));
		std::vector<float> base(3, 0.0f), tops(3, 0.0f), coverage(3, 0.0f), type(3, 0.0f);
		for (size_t i = 0; i < std::min<size_t>(3, s.clouds.size()); ++i)
		{
			base[i] = s.clouds[i].baseMslM;
			tops[i] = s.clouds[i].topMslM;
			coverage[i] = s.clouds[i].coverage;
			type[i] = s.clouds[i].type;
		}
		SetFloatArray("sim/weather/region/cloud_base_msl_m", base);
		SetFloatArray("sim/weather/region/cloud_tops_msl_m", tops);
		SetFloatArray("sim/weather/region/cloud_coverage_percent", coverage);
		SetFloatArray("sim/weather/region/cloud_type", type);
		SetFloat("sim/weather/region/rain_percent", s.rainPercent);
		SetFloat("sim/weather/region/runway_friction", s.runwayFriction);
		if (!SetFloat(SNOW_COVER, s.snowCover))
			Log("sim/weather/region/snow_cover is not writable here: the snow scenario keeps X-Plane's own cover");
		SetFloat(SEALEVEL_TEMPERATURE, s.temperatureC);
		if (s.waveAmplitudeM)
			SetFloat("sim/weather/region/wave_amplitude", *s.waveAmplitudeM);
		if (s.waveDirectionDeg)
			SetFloat("sim/weather/region/wave_dir", *s.waveDirectionDeg);
		SetInt("sim/weather/region/update_immediately", 1);
	}

	/**
	 * @brief Keeps the user aircraft where the run put it (g.userLat / userLon / userMsl), out of the shot, for the whole
	 * run: shots and airport changes alike. Moving it with XPLMPlaceUserAtLocation / XPLMPlaceUserAtAirport starts a
	 * new flight (X-Plane reloads the aircraft and its plugins and tells every plugin the user moved), and X-Coder's
	 * Living Scenery Technology (v1.11.6) doesn't survive new flights: X-Plane destroys one of its flight loops in the
	 * first one, and LST deletes that loop again later (at the second new flight at KDEN, or when the move from KBOS to
	 * KBTV took it out of KBOS's range), so X-Plane's plugin manager stops the sim ("Contains(list_head, who)"). Here the
	 * run overrides the plane's flight path (override_planepath[0], put back by ReleaseUser) and moves it through its
	 * position datarefs, the way plugins move a plane mid-flight; called every frame, as X-Plane may shift its local
	 * frame while we wait.
	 */
	void HoldUser()
	{
		if (!g.userHeld)
		{
			g.savedOverridePlanePath = GetIntElement(OVERRIDE_PLANE_PATH, 0);
			if (!SetIntElement(OVERRIDE_PLANE_PATH, 0, 1))
				Log("sim/operation/override/override_planepath is not writable here: the aircraft may drift into the shot");
			g.userHeld = true;
		}
		double x = 0.0, y = 0.0, z = 0.0;
		XPLMWorldToLocal(g.userLat, g.userLon, g.userMsl, &x, &y, &z);
		SetDouble("sim/flightmodel/position/local_x", x);
		SetDouble("sim/flightmodel/position/local_y", y);
		SetDouble("sim/flightmodel/position/local_z", z);
		for (const char *velocity : { "sim/flightmodel/position/local_vx", "sim/flightmodel/position/local_vy", "sim/flightmodel/position/local_vz" })
			SetFloat(velocity, 0.0f);
		// Level, facing the camera's heading (X-Plane's quaternion with pitch and roll 0)
		const double halfPsi = g.userHeading * PI / 360.0;
		SetFloatArray("sim/flightmodel/position/q", { static_cast<float>(std::cos(halfPsi)), 0.0f, 0.0f, static_cast<float>(std::sin(halfPsi)) });
	}

	/**
	 * @brief Gives the plane's flight path back to X-Plane; parked at the run's last airport when asked (a new flight, so
	 * not on shutdown, nor when X-Plane quits after the run).
	 */
	void ReleaseUser(const bool parkAtAirport)
	{
		if (!g.userHeld)
			return;
		SetIntElement(OVERRIDE_PLANE_PATH, 0, g.savedOverridePlanePath.value_or(0));
		g.userHeld = false;
		if (parkAtAirport && !g.loadedAirport.empty())
			XPLMPlaceUserAtAirport(g.loadedAirport.c_str());	// At the airport: not left hanging in the air without a flight path
	}

	/* @brief The scenario's camera pose over ground at groundMsl, and the user aircraft's spot behind and above it. */
	void SetPose(const Scenario &s, const double groundMsl)
	{
		g.cameraLat = s.lat;
		g.cameraLon = s.lon;
		g.cameraMsl = groundMsl + s.altMAgl;
		g.cameraHeading = s.heading;
		g.cameraPitch = s.pitch;

		Offset(s.lat, s.lon, s.heading + 180.0, USER_BEHIND_CAMERA_M, g.userLat, g.userLon);
		g.userMsl = g.cameraMsl + USER_ABOVE_CAMERA_M;
		g.userHeading = s.heading;
	}

	/* @brief The scenario airport's elevation from X-Plane's nav database (MSL), 0 m where it isn't there. */
	double AirportElevation(const Scenario &s)
	{
		if (s.icao.empty())
			return 0.0;
		float lat = static_cast<float>(s.lat), lon = static_cast<float>(s.lon);
		const XPLMNavRef airport = XPLMFindNavAid(nullptr, s.icao.c_str(), &lat, &lon, nullptr, xplm_Nav_Airport);
		if (airport == XPLM_NAV_NOT_FOUND)
			return 0.0;
		float height = 0.0f;
		XPLMGetNavAidInfo(airport, nullptr, nullptr, nullptr, &height, nullptr, nullptr, nullptr, nullptr, nullptr);
		return std::max(height, 0.0f);
	}

	/**
	 * @brief Takes the held aircraft and the camera to the scenario's airport without a new flight (see HoldUser):
	 * X-Plane loads the scenery around the aircraft as it does for a plane flying there. Until the terrain is in, the
	 * airport's elevation stands in for the ground.
	 */
	void MoveHeldUser(const Scenario &s)
	{
		SetPose(s, AirportElevation(s));
		HoldUser();
	}

	/* @brief The terrain under the scenario's camera is in and X-Plane isn't loading or unloading scenery. */
	bool SceneryReady(const Scenario &s)
	{
		if (!GroundMsl(s.lat, s.lon))
			return false;
		const std::optional<float> loading = GetFloat(SCENERY_LOADING);
		return !loading || *loading == 0.0f;
	}

	std::string Fixed(const double value, const int decimals)
	{
		char text[32] = {};
		std::snprintf(text, sizeof(text), "%.*f", decimals, value);
		return text;
	}

	/* @brief The scenario's snow cover isn't the one X-Plane last had time to build (see SNOW_BUILD_SECONDS). */
	bool SnowChanges(const Scenario &s)
	{
		return g.builtSnowCover && std::fabs(*g.builtSnowCover - s.snowCover) > SNOW_COVER_TOLERANCE;
	}

	/* @brief Places the camera and moves the user aircraft out of sight (behind and above the camera). */
	void PlaceCameraAndUser(const Scenario &s)
	{
		const std::optional<double> ground = GroundMsl(s.lat, s.lon);
		if (!ground)
			Log(s.id + ": no terrain under the camera yet; using 0 m MSL as the ground");
		SetPose(s, ground.value_or(0.0));
		HoldUser();

		SetFloat("sim/graphics/view/field_of_view_deg", s.fov);
		g.cameraActive = true;
		XPLMControlCamera(xplm_ControlCameraForever, CameraCallback, nullptr);
	}

	// -------------------------------------------------------

	fs::path NewestScreenshot()
	{
		fs::path newest;
		fs::file_time_type newestTime{};
		std::error_code ec;
		for (const fs::directory_entry &entry : fs::directory_iterator(g.xplaneFolder / "Output" / "screenshots", ec))
		{
			if (!entry.is_regular_file(ec) || entry.path().extension() != ".png")
				continue;
			const fs::file_time_type time = entry.last_write_time(ec);
			if (newest.empty() || time > newestTime)
			{
				newest = entry.path();
				newestTime = time;
			}
		}
		return newest;
	}

	/* @brief The file is a whole PNG: it ends in the IEND chunk (X-Plane writes screenshots in pieces). */
	bool PngComplete(const fs::path &path)
	{
		static constexpr unsigned char IEND[12] = { 0, 0, 0, 0, 'I', 'E', 'N', 'D', 0xAE, 0x42, 0x60, 0x82 };
		std::ifstream file(path, std::ios::binary | std::ios::ate);
		if (!file || file.tellg() < static_cast<std::streamoff>(sizeof(IEND)))
			return false;
		char tail[sizeof(IEND)] = {};
		file.seekg(-static_cast<std::streamoff>(sizeof(IEND)), std::ios::end);
		file.read(tail, sizeof(tail));
		return file && std::memcmp(tail, IEND, sizeof(IEND)) == 0;
	}

	void WriteDump(const Scenario &s, const fs::path &screenshot)
	{
		Json dump;
		int xplaneVersion = 0, xplmVersion = 0;
		XPLMHostApplicationID host = 0;
		XPLMGetVersions(&xplaneVersion, &xplmVersion, &host);
		dump["id"] = s.id;
		dump["xplane_version"] = xplaneVersion;
		dump["xplm_version"] = xplmVersion;
		dump["screenshot"] = screenshot.filename().string();
		dump["scenario"] = s.source;

		Json applied;
		applied["camera_lat"] = g.cameraLat;
		applied["camera_lon"] = g.cameraLon;
		applied["camera_msl_m"] = g.cameraMsl;
		applied["camera_heading"] = g.cameraHeading;
		applied["camera_pitch"] = g.cameraPitch;
		applied["snow_wait_seconds"] = g.snowWaitSeconds;
		applied["settle_seconds"] = g.settledSeconds;
		applied["scenery_still_loading"] = g.settledLoading;
		applied["scenery_date"] = g.sceneryDate;	// The date the airport's scenery (and its seasonal art) was loaded on
		dump["applied"] = applied;

		Json sim;
		for (const char *name : { "sim/graphics/scenery/sun_pitch_degrees", "sim/graphics/scenery/sun_heading_degrees", "sim/time/local_time_sec",
								  "sim/time/zulu_time_sec", "sim/time/local_date_days", "sim/graphics/view/field_of_view_deg",
								  "sim/graphics/view/window_width", "sim/graphics/view/window_height", "sim/graphics/view/visibility_effective_m",
								  "sim/weather/region/visibility_reported_sm", "sim/weather/region/runway_friction", "sim/weather/region/rain_percent",
								  "sim/weather/region/snow_cover", "sim/weather/region/cloud_coverage_percent", "sim/weather/region/cloud_base_msl_m",
								  "sim/weather/view/rain_ratio", "sim/weather/view/snow_ratio", "sim/weather/region/wave_amplitude",
								  "sim/weather/region/wave_dir", "sim/weather/region/wave_length", "sim/weather/region/wave_speed",
								  "sim/weather/region/wind_speed_msc", "sim/weather/region/wind_direction_degt", SEALEVEL_TEMPERATURE,
								  "sim/weather/aircraft/temperature_ambient_deg_c" })
		{
			if (XPLMDataRef ref = Find(name))
				sim[name] = ReadValue(ref);
		}
		dump["sim"] = sim;

		// Art controls (sim/private/controls/<name>), else render statistics (sim/private/stats/<name>)
		Json controls = Json::object();
		Json stats = Json::object();
		int found = 0;
		for (const std::string &name : g.artControls)
		{
			if (XPLMDataRef ref = Find(("sim/private/controls/" + name).c_str()))
			{
				controls[name] = ReadValue(ref);
				++found;
			}
			else if (XPLMDataRef stat = Find(("sim/private/stats/" + name).c_str()))
			{
				stats[name] = ReadValue(stat);
				++found;
			}
		}
		dump["controls"] = controls;
		dump["stats"] = stats;

		std::ofstream file(g.outputFolder / (s.id + ".json"));
		file << dump.dump(1, '\t') << '\n';
		Log(s.id + ": dumped " + std::to_string(found) + " of " + std::to_string(g.artControls.size()) + " art controls");
	}

	// -------------------------------------------------------

	/**
	 * @brief Switches off the plugins in PLUGINS_OFF_DURING_RUN for the run. Living Scenery Technology (v1.11.6) stops
	 * X-Plane after a new flight (see HoldUser), and its moving vehicles, boats and people are art SEDX doesn't draw.
	 * Done in the run's first flight loop, before anything moves: once a new flight has destroyed one of LST's flight
	 * loops, its own clean-up (which disabling runs) would trip over it too.
	 */
	void DisablePlugins()
	{
		for (const char *signature : PLUGINS_OFF_DURING_RUN)
		{
			const XPLMPluginID id = XPLMFindPluginBySignature(signature);
			if (id == XPLM_NO_PLUGIN_ID || !XPLMIsPluginEnabled(id))
				continue;
			XPLMDisablePlugin(id);
			g.disabledPlugins.emplace_back(id, signature);
			Log(std::string("Disabled ") + signature + " for the run");
		}
	}

	/* @brief Switches DisablePlugins' plugins back on: before the aircraft is parked (a new flight), never when quitting. */
	void EnablePlugins()
	{
		for (const auto &[id, signature] : g.disabledPlugins)
			Log((XPLMEnablePlugin(id) ? "Enabled " : "Could not enable ") + signature + " again");
		g.disabledPlugins.clear();
	}

	void Restore(const bool parkUser)
	{
		ReleaseUser(parkUser);
		if (g.cameraActive)
		{
			g.cameraActive = false;
			XPLMDontControlCamera();
		}
		if (g.savedEv100Min)
			SetFloat("sim/private/controls/photometric/ev100_min", *g.savedEv100Min);
		if (g.savedEv100Max)
			SetFloat("sim/private/controls/photometric/ev100_max", *g.savedEv100Max);
		if (g.savedFov)
			SetFloat("sim/graphics/view/field_of_view_deg", *g.savedFov);
		if (g.savedUseSystemTime)
			SetInt("sim/time/use_system_time", *g.savedUseSystemTime);
		RestoreArtControls();
	}

	void StartRun(const ScenarioSet &set, const RunMode mode)
	{
		if (g.stage != Stage::Idle && g.stage != Stage::Finished)
		{
			Log("A run is already in progress");
			return;
		}
		if (!LoadScenarios(set.file))
			return;
		if (mode == RunMode::Missing)
		{
			// Resume after a crash or a stop: the scenarios with both their screenshot and their dump are done
			const size_t before = g.scenarios.size();
			std::erase_if(g.scenarios, [](const Scenario &s)
			{
				std::error_code ec;
				return fs::exists(g.outputFolder / (s.id + ".png"), ec) && fs::exists(g.outputFolder / (s.id + ".json"), ec);
			});
			Log("Resuming: " + std::to_string(before - g.scenarios.size()) + " scenarios already captured, " + std::to_string(g.scenarios.size()) + " to go");
			if (g.scenarios.empty())
				return;
		}
		else if (mode == RunMode::Snowy)
		{
			// Only the snow group, taken again over what is there (the first one waits for X-Plane's snow, see WaitSnow)
			std::erase_if(g.scenarios, [](const Scenario &s) { return !Snowy(s); });
			Log("Snowy scenarios only: " + std::to_string(g.scenarios.size()) + " to take");
			if (g.scenarios.empty())
				return;
		}
		LoadArtControls();
		std::error_code ec;
		fs::create_directories(g.outputFolder, ec);
		if (!fs::exists(g.xplaneFolder / "Custom Scenery" / "SEDX_Calibration"))
			Log("Custom Scenery/SEDX_Calibration is not installed: the calibration panels won't be in the references");

		g.savedEv100Min = GetFloat("sim/private/controls/photometric/ev100_min");
		g.savedEv100Max = GetFloat("sim/private/controls/photometric/ev100_max");
		g.savedFov = GetFloat("sim/graphics/view/field_of_view_deg");
		if (const std::optional<float> useSystem = GetFloat("sim/time/use_system_time"))
			g.savedUseSystemTime = static_cast<int>(*useSystem);
		g.builtSnowCover = GetFloat(SNOW_COVER);	// What X-Plane has when the run starts counts as built

		g.index = 0;
		g.runFile = set.file;
		g.runAll = mode != RunMode::First;
		g.loadedAirport.clear();
		g.sceneryDate.clear();
		g.reloadPending = false;
		g.disablePluginsPending = true;
		g.stage = Stage::MoveToAirport;
		g.framesLeft = -1;	// Not placed yet
		const char *what = mode == RunMode::All ? "all scenarios" : mode == RunMode::Missing ? "the scenarios not captured yet" : mode == RunMode::Snowy ? "the snowy scenarios" : "the first scenario";
		Log(std::string("Run started: ") + what + " of " + set.file);
	}

	void FinishRun()
	{
		if (!g.quitWhenDone)
			EnablePlugins();
		Restore(!g.quitWhenDone);
		g.stage = Stage::Finished;
		g.finishedAt = Clock::now();
		Log("Run finished: " + std::to_string(g.index) + " scenarios in " + g.outputFolder.string());
		if (g.quitWhenDone)
		{
			if (XPLMCommandRef quit = XPLMFindCommand("sim/operation/quit"))
				XPLMCommandOnce(quit);
		}
	}

	void StartSettle(const Scenario &s)
	{
		g.framesLeft = s.settleFrames;
		g.stageStart = Clock::now();
		g.quietFrames = 0;
		g.stage = Stage::Settle;
		Log(s.id + ": settling for at least " + std::to_string(static_cast<int>(SETTLE_SECONDS)) + " s and " + std::to_string(s.settleFrames) + " frames");
	}

	void NextScenario()
	{
		// Restore the exposure clamp between scenarios so an unlocked one meters freely
		if (g.savedEv100Min)
			SetFloat("sim/private/controls/photometric/ev100_min", *g.savedEv100Min);
		if (g.savedEv100Max)
			SetFloat("sim/private/controls/photometric/ev100_max", *g.savedEv100Max);
		RestoreArtControls();
		++g.index;
		if (!g.runAll || g.index >= g.scenarios.size())
		{
			FinishRun();
			return;
		}
		g.stage = Stage::MoveToAirport;
		g.framesLeft = -1;
	}

	// -------------------------------------------------------

	std::string Seconds(const double seconds)
	{
		return std::to_string(static_cast<int>(std::ceil(std::max(seconds, 0.0)))) + " s";
	}

	/* @brief The status window's lines: the run and scenario, what the probe waits for, and the countdown to the shot. */
	std::vector<std::string> StatusLines()
	{
		if (g.stage == Stage::Finished)
			return { std::string(PROBE_NAME) + ": run finished", std::to_string(g.index) + " scenarios of " + g.runFile,
					 "Screenshots and dumps in Output/sedx_probe", "Details in Output/sedx_probe/probe_log.txt" };
		if (g.index >= g.scenarios.size())
			return {};

		const Scenario &s = g.scenarios[g.index];
		std::vector<std::string> lines;
		lines.push_back(std::string(PROBE_NAME) + "  -  " + g.runFile + "  " + std::to_string(g.index + 1) + " / " + std::to_string(g.scenarios.size()));
		lines.push_back(s.id + (s.icao.empty() ? std::string() : "  (" + s.icao + ")"));

		const std::optional<float> loading = GetFloat(SCENERY_LOADING);
		const bool xplaneLoading = loading && *loading != 0.0f;
		if (g.stage == Stage::MoveToAirport)
		{
			std::string waiting = "applying time and weather";
			if (g.framesLeft > 0 && !GroundMsl(s.lat, s.lon))
				waiting = "waiting for the terrain";
			else if (g.framesLeft > 0 && xplaneLoading)
				waiting = "X-Plane is loading scenery";
			else if (g.framesLeft > 0)
				waiting = "steady for " + std::to_string(g.framesLeft) + " more frames";
			lines.push_back("Loading " + s.icao + " (" + Seconds(SecondsSince(g.stageStart)) + "): " + waiting);
		}
		else if (g.stage == Stage::WaitSnow)
		{
			lines.push_back("Letting X-Plane build the snow: " + Seconds(SNOW_BUILD_SECONDS - SecondsSince(g.stageStart)) + " left (cover reads " +
							Fixed(GetFloat(SNOW_COVER).value_or(0.0f), 2) + "), then settling");
		}
		else
		{
			const double settled = SecondsSince(g.stageStart);
			if (settled < SETTLE_SECONDS)
				lines.push_back("Capture in " + Seconds(SETTLE_SECONDS - settled) + (xplaneLoading ? "  (X-Plane is loading scenery)" : ""));
			else if (g.framesLeft >= 0)
				lines.push_back("Capture in " + std::to_string(g.framesLeft + 1) + " frames");
			else
				lines.push_back("Capture once X-Plane stops loading scenery (" + Seconds(SETTLE_TIMEOUT_SECONDS - settled) + " at most)");
		}
		lines.push_back(std::string(g.disabledPlugins.empty() ? "" : "Living Scenery Technology off for the run  |  ") + "Stop: Plugins > " + PROBE_NAME);
		return lines;
	}

	/* @brief Shows the status window while the run moves and settles; hidden around screenshots (see STATUS_HIDDEN_FRAMES). */
	void UpdateStatusWindow()
	{
		if (!g.statusWindow)
			return;
		const bool running = g.stage == Stage::MoveToAirport || g.stage == Stage::WaitSnow || g.stage == Stage::Settle;
		const bool finished = g.stage == Stage::Finished && SecondsSince(g.finishedAt) < STATUS_FINISHED_SECONDS;
		const int visible = running || finished ? 1 : 0;
		if (visible)
			g.statusLines = StatusLines();
		if (XPLMGetWindowIsVisible(g.statusWindow) != visible)
			XPLMSetWindowIsVisible(g.statusWindow, visible);
	}

	void DrawStatus(XPLMWindowID window, void *)
	{
		int left = 0, top = 0, right = 0, bottom = 0;
		XPLMGetWindowGeometry(window, &left, &top, &right, &bottom);
		XPLMDrawTranslucentDarkBox(left, top, right, bottom);
		float title[3] = { 1.0f, 1.0f, 1.0f };
		float text[3] = { 0.78f, 0.78f, 0.78f };
		float countdown[3] = { 1.0f, 0.8f, 0.3f };
		int y = top - STATUS_LINE_HEIGHT;
		for (size_t i = 0; i < g.statusLines.size(); ++i, y -= STATUS_LINE_HEIGHT)
			XPLMDrawString(i == 0 ? title : i == 2 ? countdown : text, left + 10, y, g.statusLines[i].c_str(), nullptr, xplmFont_Proportional);
	}

	/* @brief The status window, top left of the main screen below the menu bar; it ignores the mouse and the keyboard. */
	void CreateStatusWindow()
	{
		int screenLeft = 0, screenTop = 0, screenRight = 0, screenBottom = 0;
		XPLMGetScreenBoundsGlobal(&screenLeft, &screenTop, &screenRight, &screenBottom);
		XPLMCreateWindow_t params{};
		params.structSize = sizeof(params);
		params.left = screenLeft + 20;
		params.top = screenTop - 60;
		params.right = params.left + STATUS_WIDTH;
		params.bottom = params.top - (STATUS_LINES * STATUS_LINE_HEIGHT + 12);
		params.visible = 0;
		params.drawWindowFunc = DrawStatus;
		params.handleMouseClickFunc = [](XPLMWindowID, int, int, XPLMMouseStatus, void *) { return 0; };
		params.handleRightClickFunc = [](XPLMWindowID, int, int, XPLMMouseStatus, void *) { return 0; };
		params.handleKeyFunc = [](XPLMWindowID, char, XPLMKeyFlags, char, void *, int) {};
		params.handleCursorFunc = [](XPLMWindowID, int, int, void *) { return static_cast<XPLMCursorStatus>(xplm_CursorDefault); };
		params.handleMouseWheelFunc = [](XPLMWindowID, int, int, int, int, void *) { return 0; };
		params.decorateAsFloatingWindow = xplm_WindowDecorationNone;
		params.layer = xplm_WindowLayerFloatingWindows;
		g.statusWindow = XPLMCreateWindowEx(&params);
	}

	float FlightLoop(float, float, int, void *)
	{
		UpdateStatusWindow();
		if (g.stage == Stage::Idle || g.stage == Stage::Finished)
			return -1.0f;
		if (g.disablePluginsPending)
		{
			g.disablePluginsPending = false;
			DisablePlugins();
		}
		const Scenario &s = g.scenarios[g.index];

		switch (g.stage)
		{
			case Stage::MoveToAirport:
			{
				if (g.framesLeft < 0)
				{
					// X-Plane picks a tile's seasonal library art (EXPORT_SEASON terrain, autogen, airport art) as it
					// loads the tile and keeps it when the date changes: the first 15-airport run moved first and set the
					// date after, so most July shots had the terrain of the January shot before them. The scenery date
					// goes in before the move, and a new date at the same airport (or the run's first airport, which may
					// be loaded already) reloads the scenery
					const std::string sceneryDate = FormatDate(SceneryDateOf(s));
					const bool move = s.icao != g.loadedAirport;
					if (move || sceneryDate != g.sceneryDate)
					{
						ApplyDate(SceneryDateOf(s));
						if (move)
						{
							Log(s.id + ": moving to " + s.icao + " (scenery date " + sceneryDate + ")");
							MoveHeldUser(s);
							g.loadedAirport = s.icao;
							g.newFlight = false;
						}
						g.reloadPending = !move || g.sceneryDate.empty();
						g.sceneryDate = sceneryDate;
						g.stageStart = Clock::now();
						g.sceneryTimeout = SCENERY_TIMEOUT_FRAMES;
						g.framesLeft = move ? SCENERY_LOAD_FRAMES : 0;
					}
					else
						g.framesLeft = 0;
					return -1.0f;
				}
				if (g.userHeld)
					HoldUser();
				if (g.framesLeft > 0 && !SceneryReady(s))
				{
					// The scenery must be in for SCENERY_LOAD_FRAMES frames in a row
					g.framesLeft = SCENERY_LOAD_FRAMES;
					if (g.sceneryTimeout-- > 0)
						return -1.0f;
					if (!g.newFlight && !GroundMsl(s.lat, s.lon))
					{
						// X-Plane loaded nothing around the held aircraft: let it place the user at the airport itself
						Log(s.id + ": no terrain at " + s.icao + " after " + std::to_string(SCENERY_TIMEOUT_FRAMES) + " frames; starting a new flight there");
						ReleaseUser(false);
						XPLMPlaceUserAtAirport(s.icao.c_str());
						g.newFlight = true;
						g.sceneryTimeout = SCENERY_TIMEOUT_FRAMES;
						return -1.0f;
					}
					Log(s.id + ": scenery at " + s.icao + " still loading after " + std::to_string(SCENERY_TIMEOUT_FRAMES) + " frames; going on");
					g.framesLeft = 0;
				}
				if (g.framesLeft-- > 0)
					return -1.0f;
				if (g.reloadPending)
				{
					// Once the scenery is in: load it again on the scenery date, and wait for it as for a move
					Log(s.id + ": reloading the scenery at " + s.icao + " for " + g.sceneryDate);
					g.reloadPending = false;
					XPLMReloadScenery();
					g.sceneryTimeout = SCENERY_TIMEOUT_FRAMES;
					g.framesLeft = SCENERY_LOAD_FRAMES;
					return -1.0f;
				}
				ApplyTime(s);
				ApplyWeather(s);
				ApplyExposureLock(s);
				ApplyArtControls(s);
				PlaceCameraAndUser(s);
				g.snowWaitSeconds = 0.0;
				if (SnowChanges(s))
				{
					g.stageStart = Clock::now();
					g.stage = Stage::WaitSnow;
					Log(s.id + ": snow cover " + Fixed(*g.builtSnowCover, 2) + " -> " + Fixed(s.snowCover, 2) + "; letting X-Plane build it for " +
						std::to_string(static_cast<int>(SNOW_BUILD_SECONDS)) + " s");
					return -1.0f;
				}
				StartSettle(s);
				return -1.0f;
			}

			case Stage::WaitSnow:
			{
				ApplyTime(s);
				ApplyExposureLock(s);
				HoldUser();
				// Set again every frame: X-Plane takes a new cover late
				SetFloat(SNOW_COVER, s.snowCover);
				SetFloat(SEALEVEL_TEMPERATURE, s.temperatureC);
				const double waited = SecondsSince(g.stageStart);
				if (waited < SNOW_BUILD_SECONDS)
					return -1.0f;
				Log(s.id + ": snow built for " + std::to_string(static_cast<int>(waited)) + " s; X-Plane reads a cover of " +
					Fixed(GetFloat(SNOW_COVER).value_or(0.0f), 2));
				g.snowWaitSeconds = waited;
				g.builtSnowCover = s.snowCover;
				StartSettle(s);
				return -1.0f;
			}

			case Stage::Settle:
			{
				// Hold the sun, the exposure and the plane still while everything else settles
				ApplyTime(s);
				ApplyExposureLock(s);
				HoldUser();
				if (g.framesLeft % 120 == 0)
				{
					// Terrain under the camera may have loaded in the meantime
					if (const std::optional<double> ground = GroundMsl(s.lat, s.lon))
						g.cameraMsl = *ground + s.altMAgl;
				}
				--g.framesLeft;
				const std::optional<float> loading = GetFloat(SCENERY_LOADING);
				g.quietFrames = loading && *loading != 0.0f ? 0 : g.quietFrames + 1;
				const double settled = SecondsSince(g.stageStart);
				if (g.framesLeft >= 0 || settled < SETTLE_SECONDS)
					return -1.0f;
				if (g.quietFrames < SETTLE_QUIET_FRAMES)
				{
					if (settled < SETTLE_TIMEOUT_SECONDS)
						return -1.0f;
					Log(s.id + ": scenery still loading after " + std::to_string(static_cast<int>(settled)) + " s; taking the shot anyway");
				}
				g.settledSeconds = settled;
				g.settledLoading = g.quietFrames < SETTLE_QUIET_FRAMES;
				g.framesLeft = STATUS_HIDDEN_FRAMES;
				g.stage = Stage::HideStatus;
				return -1.0f;
			}

			case Stage::HideStatus:
			{
				ApplyTime(s);
				ApplyExposureLock(s);
				HoldUser();
				if (g.framesLeft-- > 0)
					return -1.0f;

				std::error_code ec;
				const fs::path before = NewestScreenshot();
				g.screenshotsBefore = before.empty() ? fs::file_time_type{} : fs::last_write_time(before, ec);
				g.screenshotFile.clear();
				g.screenshotSize = 0;
				g.screenshotStable = 0;
				g.stageStart = Clock::now();
				if (XPLMCommandRef screenshot = XPLMFindCommand("sim/operation/screenshot"))
					XPLMCommandOnce(screenshot);
				else
					Log("sim/operation/screenshot not found");
				g.stage = Stage::WaitScreenshot;
				return -1.0f;
			}

			case Stage::WaitScreenshot:
			{
				ApplyTime(s);
				ApplyExposureLock(s);
				HoldUser();
				std::error_code ec;
				const fs::path newest = NewestScreenshot();
				if (!newest.empty() && fs::last_write_time(newest, ec) > g.screenshotsBefore)
				{
					const std::uintmax_t size = fs::file_size(newest, ec);
					if (newest == g.screenshotFile && size == g.screenshotSize && size > 0)
						++g.screenshotStable;
					else
					{
						g.screenshotFile = newest;
						g.screenshotSize = size;
						g.screenshotStable = 0;
						g.screenshotStableSince = Clock::now();
					}
					if (g.screenshotStable >= SCREENSHOT_STABLE_FRAMES && SecondsSince(g.screenshotStableSince) >= SCREENSHOT_STABLE_SECONDS &&
						PngComplete(newest))
					{
						// A copy cut short (X-Plane still had the file open) is tried again next frame
						const fs::path target = g.outputFolder / (s.id + ".png");
						fs::copy_file(newest, target, fs::copy_options::overwrite_existing, ec);
						if (!ec && PngComplete(target))
						{
							Log(s.id + ": " + target.string());
							WriteDump(s, newest);
							NextScenario();
							return -1.0f;
						}
					}
				}
				if (SecondsSince(g.stageStart) > SCREENSHOT_TIMEOUT_SECONDS)
				{
					// Without its png the scenario counts as not captured: Resume takes it again
					if (g.screenshotFile.empty())
						Log(s.id + ": no screenshot appeared in Output/screenshots; skipped");
					else
						Log(s.id + ": " + g.screenshotFile.string() + " never became a complete png (or could not be copied); skipped");
					std::error_code removeError;
					fs::remove(g.outputFolder / (s.id + ".png"), removeError);
					WriteDump(s, fs::path());
					NextScenario();
				}
				return -1.0f;
			}

			default:
				return -1.0f;
		}
	}

	void MenuHandler(void *, void *item)
	{
		const intptr_t command = reinterpret_cast<intptr_t>(item);
		const size_t set = static_cast<size_t>(command >> 8);
		if (command != MENU_STOP && set < std::size(SCENARIO_SETS))
			StartRun(SCENARIO_SETS[set], static_cast<RunMode>(command & 0xFF));
		else if (command == MENU_STOP && g.stage != Stage::Idle && g.stage != Stage::Finished)
		{
			Log("Run stopped");
			g.disablePluginsPending = false;
			EnablePlugins();
			Restore(true);
			g.stage = Stage::Finished;
			g.finishedAt = Clock::now();
		}
	}
}

// -------------------------------------------------------

PLUGIN_API int XPluginStart(char *outName, char *outSig, char *outDesc)
{
	std::snprintf(outName, 256, "%s", PROBE_NAME);
	std::snprintf(outSig, 256, "%s", PROBE_SIGNATURE);
	std::snprintf(outDesc, 256, "%s", PROBE_DESCRIPTION);
	XPLMEnableFeature("XPLM_USE_NATIVE_PATHS", 1);

	char systemPath[2048] = {};
	XPLMGetSystemPath(systemPath);
	g.xplaneFolder = fs::path(systemPath);
	g.outputFolder = g.xplaneFolder / "Output" / "sedx_probe";

	char pluginPath[2048] = {};
	XPLMGetPluginInfo(XPLMGetMyID(), nullptr, pluginPath, nullptr, nullptr);
	g.pluginFolder = fs::path(pluginPath).parent_path().parent_path();	// <plugin>/64/win.xpl -> <plugin>

	std::error_code ec;
	fs::create_directories(g.outputFolder, ec);
	g.log.open(g.outputFolder / "probe_log.txt", std::ios::app);
	Log(std::string("Started; output in ") + g.outputFolder.string());

	g.terrain = XPLMCreateProbe(xplm_ProbeY);
	XPLMRegisterFlightLoopCallback(FlightLoop, -1.0f, nullptr);
	return 1;
}

PLUGIN_API void XPluginStop(void)
{
	XPLMUnregisterFlightLoopCallback(FlightLoop, nullptr);
	if (g.terrain)
		XPLMDestroyProbe(g.terrain);
	g.terrain = nullptr;
	if (g.log.is_open())
		g.log.close();
}

PLUGIN_API int XPluginEnable(void)
{
	const int item = XPLMAppendMenuItem(XPLMFindPluginsMenu(), PROBE_NAME, nullptr, 0);
	g.menu = XPLMCreateMenu(PROBE_NAME, XPLMFindPluginsMenu(), item, MenuHandler, nullptr);
	for (size_t set = 0; set < std::size(SCENARIO_SETS); ++set)
	{
		const intptr_t base = static_cast<intptr_t>(set << 8);
		XPLMAppendMenuItem(g.menu, SCENARIO_SETS[set].runAll, reinterpret_cast<void *>(base | static_cast<intptr_t>(RunMode::All)), 0);
		XPLMAppendMenuItem(g.menu, SCENARIO_SETS[set].runMissing, reinterpret_cast<void *>(base | static_cast<intptr_t>(RunMode::Missing)), 0);
		XPLMAppendMenuItem(g.menu, SCENARIO_SETS[set].runFirst, reinterpret_cast<void *>(base | static_cast<intptr_t>(RunMode::First)), 0);
		XPLMAppendMenuItem(g.menu, SCENARIO_SETS[set].runSnowy, reinterpret_cast<void *>(base | static_cast<intptr_t>(RunMode::Snowy)), 0);
	}
	XPLMAppendMenuItem(g.menu, "Stop", reinterpret_cast<void *>(MENU_STOP), 0);
	CreateStatusWindow();

	// Unattended runs: Output/sedx_probe/autorun ("quit" inside: exit X-Plane when done; "airports": the test-airport set;
	// "resume": skip the scenarios already captured; "snow": only the snowy scenarios, taken again)
	std::ifstream autorun(g.outputFolder / "autorun");
	if (autorun)
	{
		std::string text((std::istreambuf_iterator<char>(autorun)), std::istreambuf_iterator<char>());
		g.quitWhenDone = text.find("quit") != std::string::npos;
		const ScenarioSet &set = SCENARIO_SETS[text.find("airports") != std::string::npos ? 1 : 0];
		Log(std::string("autorun found: ") + set.file + (g.quitWhenDone ? " (quit when done)" : ""));
		StartRun(set, text.find("snow") != std::string::npos ? RunMode::Snowy : text.find("resume") != std::string::npos ? RunMode::Missing : RunMode::All);
	}
	return 1;
}

PLUGIN_API void XPluginDisable(void)
{
	// Shutting down or switched off mid-run: other plugins are left as they are (Plugin Admin switches them back on)
	for (const auto &[id, signature] : g.disabledPlugins)
		Log(signature + " stays disabled until X-Plane restarts or Plugin Admin enables it");
	g.disabledPlugins.clear();
	g.disablePluginsPending = false;
	Restore(false);
	if (g.menu)
		XPLMDestroyMenu(g.menu);
	g.menu = nullptr;
	if (g.statusWindow)
		XPLMDestroyWindow(g.statusWindow);
	g.statusWindow = nullptr;
	g.stage = Stage::Idle;
}

PLUGIN_API void XPluginReceiveMessage(XPLMPluginID, int, void *)
{
}

// -------------------------------------------------------
