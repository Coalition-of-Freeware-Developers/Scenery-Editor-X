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
 * with and calibrated against X-Plane itself (plan TASK-001 / TASK-002 / TASK-102).
 *
 * For each scenario it sets the date, time and weather through the public datarefs, takes the camera to a fixed pose,
 * waits settle_frames for exposure and weather to settle, saves a screenshot as Output/sedx_probe/<id>.png and writes
 * Output/sedx_probe/<id>.json with what it applied and the live value of every art control in art_controls.txt.
 *
 * Start it from Plugins > SEDX Reference Probe, or put a file named "autorun" (containing "quit" to exit X-Plane
 * afterwards) into Output/sedx_probe/.
 *
 * Reads and writes only public SDK datarefs plus the sim/private/controls/ values X-Plane exposes to plugins; it never
 * reads X-Plane's own files.
 */

#include <XPLMCamera.h>
#include <XPLMDataAccess.h>
#include <XPLMGraphics.h>
#include <XPLMMenus.h>
#include <XPLMPlanes.h>
#include <XPLMPlugin.h>
#include <XPLMProcessing.h>
#include <XPLMScenery.h>
#include <XPLMUtilities.h>

#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <optional>
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

	constexpr int SCENERY_LOAD_FRAMES = 300;		// After moving to another airport, before probing the terrain
	constexpr int SCREENSHOT_STABLE_FRAMES = 10;	// The screenshot file kept its size this long: X-Plane finished it
	constexpr int SCREENSHOT_TIMEOUT_FRAMES = 1200;
	constexpr double METERS_PER_STATUTE_MILE = 1609.344;
	constexpr double PI = 3.14159265358979323846;

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
		double localTimeHours = 12.0;
		float utcOffset = 0.0f;
		float visibilityM = 80000.0f;
		std::vector<CloudLayer> clouds;
		float rainPercent = 0.0f, snowCover = 0.0f, temperatureC = 15.0f;
		float runwayFriction = 0.0f;
		int settleFrames = 600;
		std::optional<float> ev100Lock;
		Json source;		// The entry as read, echoed into the dump
	};

	enum class Stage
	{
		Idle,
		MoveToAirport,		// Placed at the airport, waiting for its scenery
		Settle,				// Camera, time and weather set; exposure and weather settling
		WaitScreenshot,		// Screenshot command sent; waiting for the file to appear and stop growing
		Finished
	};

	struct Probe
	{
		XPLMMenuID menu = nullptr;
		XPLMProbeRef terrain = nullptr;
		fs::path pluginFolder;		// SEDXReferenceProbe/ (holding 64/)
		fs::path xplaneFolder;
		fs::path outputFolder;		// Output/sedx_probe/
		std::ofstream log;

		std::vector<Scenario> scenarios;
		size_t index = 0;
		bool runAll = true;
		bool quitWhenDone = false;
		Stage stage = Stage::Idle;
		int framesLeft = 0;
		std::string loadedAirport;

		// Camera pose in world coordinates, resolved to local coordinates every frame
		double cameraLat = 0.0, cameraLon = 0.0, cameraMsl = 0.0;
		float cameraHeading = 0.0f, cameraPitch = 0.0f;
		bool cameraActive = false;

		// Screenshot detection
		fs::file_time_type screenshotsBefore{};
		fs::path screenshotFile;
		std::uintmax_t screenshotSize = 0;
		int screenshotStable = 0;

		// Values to restore after the run
		std::optional<float> savedEv100, savedEv100Min, savedEv100Max, savedFov;
		std::optional<int> savedUseSystemTime;

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
			g.log << message << '\n';
			g.log.flush();
		}
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
		if (const auto it = object.find("local_time"); it != object.end() && it->is_string())
			ParseTime(it->get<std::string>(), s.localTimeHours);
		if (const auto it = object.find("ev100_lock"); it != object.end())
			s.ev100Lock = it->is_null() ? std::nullopt : std::optional<float>(it->get<float>());
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

	bool LoadScenarios()
	{
		g.scenarios.clear();
		// Output/sedx_probe/scenarios.json first (per-run sets), then the copy installed with the plugin
		for (const fs::path &path : { g.outputFolder / "scenarios.json", g.pluginFolder / "scenarios.json" })
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
				Log("Loaded " + std::to_string(g.scenarios.size()) + " scenarios from " + path.string());
				return !g.scenarios.empty();
			}
			catch (const std::exception &e)
			{
				Log("Could not read " + path.string() + ": " + e.what());
				return false;
			}
		}
		Log("No scenarios.json in " + g.outputFolder.string() + " or " + g.pluginFolder.string());
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

	void ApplyTime(const Scenario &s)
	{
		SetInt("sim/time/use_system_time", 0);
		double zulu = std::fmod(s.localTimeHours - s.utcOffset, 24.0);
		if (zulu < 0.0)
			zulu += 24.0;
		SetFloat("sim/time/zulu_time_sec", static_cast<float>(zulu * 3600.0));
		SetInt("sim/time/local_date_days", DayOfYearZeroBased(s.year, s.month, s.day));
	}

	void ApplyExposureLock(const Scenario &s)
	{
		if (!s.ev100Lock)
			return;
		// The control X-Plane's screenshot tool drives, plus a clamp so auto exposure can't move away from it
		SetFloat("sim/private/controls/photometric/ev100", *s.ev100Lock);
		SetFloat("sim/private/controls/photometric/ev100_min", *s.ev100Lock);
		SetFloat("sim/private/controls/photometric/ev100_max", *s.ev100Lock);
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
		if (!SetFloat("sim/weather/region/snow_cover", s.snowCover))
			Log("sim/weather/region/snow_cover is not writable here: the snow scenario keeps X-Plane's own cover");
		SetFloat("sim/weather/region/sealevel_temperature_c", s.temperatureC);
		SetInt("sim/weather/region/update_immediately", 1);
	}

	/* @brief Places the camera and moves the user aircraft out of sight (behind and above the camera). */
	void PlaceCameraAndUser(const Scenario &s)
	{
		const std::optional<double> ground = GroundMsl(s.lat, s.lon);
		if (!ground)
			Log(s.id + ": no terrain under the camera yet; using 0 m MSL as the ground");
		g.cameraLat = s.lat;
		g.cameraLon = s.lon;
		g.cameraMsl = ground.value_or(0.0) + s.altMAgl;
		g.cameraHeading = s.heading;
		g.cameraPitch = s.pitch;

		double userLat = 0.0, userLon = 0.0;
		Offset(s.lat, s.lon, s.heading + 180.0, 6000.0, userLat, userLon);
		XPLMPlaceUserAtLocation(userLat, userLon, static_cast<float>(g.cameraMsl + 1200.0), s.heading, 70.0f);

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
		dump["applied"] = applied;

		Json sim;
		for (const char *name : { "sim/graphics/scenery/sun_pitch_degrees", "sim/graphics/scenery/sun_heading_degrees", "sim/time/local_time_sec",
								  "sim/time/zulu_time_sec", "sim/time/local_date_days", "sim/graphics/view/field_of_view_deg",
								  "sim/graphics/view/window_width", "sim/graphics/view/window_height", "sim/graphics/view/visibility_effective_m",
								  "sim/weather/region/visibility_reported_sm", "sim/weather/region/runway_friction", "sim/weather/region/rain_percent",
								  "sim/weather/region/snow_cover", "sim/weather/region/cloud_coverage_percent", "sim/weather/region/cloud_base_msl_m",
								  "sim/weather/view/rain_ratio", "sim/weather/view/snow_ratio" })
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

	void Restore()
	{
		if (g.cameraActive)
		{
			g.cameraActive = false;
			XPLMDontControlCamera();
		}
		if (g.savedEv100)
			SetFloat("sim/private/controls/photometric/ev100", *g.savedEv100);
		if (g.savedEv100Min)
			SetFloat("sim/private/controls/photometric/ev100_min", *g.savedEv100Min);
		if (g.savedEv100Max)
			SetFloat("sim/private/controls/photometric/ev100_max", *g.savedEv100Max);
		if (g.savedFov)
			SetFloat("sim/graphics/view/field_of_view_deg", *g.savedFov);
		if (g.savedUseSystemTime)
			SetInt("sim/time/use_system_time", *g.savedUseSystemTime);
	}

	void StartRun(const bool all)
	{
		if (g.stage != Stage::Idle && g.stage != Stage::Finished)
		{
			Log("A run is already in progress");
			return;
		}
		if (!LoadScenarios())
			return;
		LoadArtControls();
		std::error_code ec;
		fs::create_directories(g.outputFolder, ec);
		if (!fs::exists(g.xplaneFolder / "Custom Scenery" / "SEDX_Calibration"))
			Log("Custom Scenery/SEDX_Calibration is not installed: the calibration panels won't be in the references");

		g.savedEv100 = GetFloat("sim/private/controls/photometric/ev100");
		g.savedEv100Min = GetFloat("sim/private/controls/photometric/ev100_min");
		g.savedEv100Max = GetFloat("sim/private/controls/photometric/ev100_max");
		g.savedFov = GetFloat("sim/graphics/view/field_of_view_deg");
		if (const std::optional<float> useSystem = GetFloat("sim/time/use_system_time"))
			g.savedUseSystemTime = static_cast<int>(*useSystem);

		g.index = 0;
		g.runAll = all;
		g.loadedAirport.clear();
		g.stage = Stage::MoveToAirport;
		g.framesLeft = -1;	// Not placed yet
		Log(std::string("Run started: ") + (all ? "all scenarios" : "the first scenario"));
	}

	void FinishRun()
	{
		Restore();
		g.stage = Stage::Finished;
		Log("Run finished: " + std::to_string(g.index) + " scenarios in " + g.outputFolder.string());
		if (g.quitWhenDone)
		{
			if (XPLMCommandRef quit = XPLMFindCommand("sim/operation/quit"))
				XPLMCommandOnce(quit);
		}
	}

	void NextScenario()
	{
		// Restore the exposure clamp between scenarios so an unlocked one meters freely
		if (g.savedEv100Min)
			SetFloat("sim/private/controls/photometric/ev100_min", *g.savedEv100Min);
		if (g.savedEv100Max)
			SetFloat("sim/private/controls/photometric/ev100_max", *g.savedEv100Max);
		++g.index;
		if (!g.runAll || g.index >= g.scenarios.size())
		{
			FinishRun();
			return;
		}
		g.stage = Stage::MoveToAirport;
		g.framesLeft = -1;
	}

	float FlightLoop(float, float, int, void *)
	{
		if (g.stage == Stage::Idle || g.stage == Stage::Finished)
			return -1.0f;
		const Scenario &s = g.scenarios[g.index];

		switch (g.stage)
		{
			case Stage::MoveToAirport:
			{
				if (g.framesLeft < 0)
				{
					if (s.icao != g.loadedAirport)
					{
						Log(s.id + ": moving to " + s.icao);
						XPLMPlaceUserAtAirport(s.icao.c_str());
						g.loadedAirport = s.icao;
						g.framesLeft = SCENERY_LOAD_FRAMES;
					}
					else
						g.framesLeft = 0;
					return -1.0f;
				}
				if (g.framesLeft-- > 0)
					return -1.0f;
				ApplyTime(s);
				ApplyWeather(s);
				ApplyExposureLock(s);
				PlaceCameraAndUser(s);
				g.framesLeft = s.settleFrames;
				g.stage = Stage::Settle;
				Log(s.id + ": settling for " + std::to_string(s.settleFrames) + " frames");
				return -1.0f;
			}

			case Stage::Settle:
			{
				// Hold the sun and the exposure still while everything else settles
				ApplyTime(s);
				ApplyExposureLock(s);
				if (g.framesLeft % 120 == 0)
				{
					// Terrain under the camera may have loaded in the meantime
					if (const std::optional<double> ground = GroundMsl(s.lat, s.lon))
						g.cameraMsl = *ground + s.altMAgl;
				}
				if (g.framesLeft-- > 0)
					return -1.0f;

				std::error_code ec;
				const fs::path before = NewestScreenshot();
				g.screenshotsBefore = before.empty() ? fs::file_time_type{} : fs::last_write_time(before, ec);
				g.screenshotFile.clear();
				g.screenshotSize = 0;
				g.screenshotStable = 0;
				g.framesLeft = SCREENSHOT_TIMEOUT_FRAMES;
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
					}
					if (g.screenshotStable >= SCREENSHOT_STABLE_FRAMES)
					{
						const fs::path target = g.outputFolder / (s.id + ".png");
						fs::copy_file(newest, target, fs::copy_options::overwrite_existing, ec);
						if (ec)
							Log(s.id + ": could not copy " + newest.string() + ": " + ec.message());
						else
							Log(s.id + ": " + target.string());
						WriteDump(s, newest);
						NextScenario();
						return -1.0f;
					}
				}
				if (g.framesLeft-- <= 0)
				{
					Log(s.id + ": no screenshot appeared in Output/screenshots; skipped");
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
		if (command == 1)
			StartRun(true);
		else if (command == 2)
			StartRun(false);
		else if (command == 3 && g.stage != Stage::Idle && g.stage != Stage::Finished)
		{
			Log("Run stopped");
			Restore();
			g.stage = Stage::Finished;
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
	XPLMAppendMenuItem(g.menu, "Run all scenarios", reinterpret_cast<void *>(intptr_t(1)), 0);
	XPLMAppendMenuItem(g.menu, "Run the first scenario", reinterpret_cast<void *>(intptr_t(2)), 0);
	XPLMAppendMenuItem(g.menu, "Stop", reinterpret_cast<void *>(intptr_t(3)), 0);

	// Unattended runs: Output/sedx_probe/autorun ("quit" inside: exit X-Plane when done)
	std::ifstream autorun(g.outputFolder / "autorun");
	if (autorun)
	{
		std::string text((std::istreambuf_iterator<char>(autorun)), std::istreambuf_iterator<char>());
		g.quitWhenDone = text.find("quit") != std::string::npos;
		Log(std::string("autorun found") + (g.quitWhenDone ? " (quit when done)" : ""));
		StartRun(true);
	}
	return 1;
}

PLUGIN_API void XPluginDisable(void)
{
	Restore();
	if (g.menu)
		XPLMDestroyMenu(g.menu);
	g.menu = nullptr;
	g.stage = Stage::Idle;
}

PLUGIN_API void XPluginReceiveMessage(XPLMPluginID, int, void *)
{
}

// -------------------------------------------------------
