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
 * parity_scenario.h
 * -------------------------------------------------------
 * Created: 20/10/2026
 * -------------------------------------------------------
 */
#pragma once
#include <array>
#include <cstdint>
#include <filesystem>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

// -------------------------------------------------------

namespace SceneryEditorX::Environment
{
	/* @brief A cloud layer as X-Plane's sim/weather/region datarefs describe it (coverage 0 = no layer). */
	struct ScenarioCloudLayer
	{
		float baseMslM = 0.0f;
		float topMslM = 0.0f;
		float coverage = 0.0f;			// 0-1 (X-Plane's cloud_coverage_percent / 100)
		int type = 0;					// sim/weather/region/cloud_type: 0 cirrus, 1 stratus, 2 cumulus, 3 cumulonimbus
	};

	/**
	 * @brief One entry of a parity scenario file (tools/xp12_reference_probe/scenarios/xp12_parity.json): the environment
	 * and camera that X-Plane 12 (through the reference probe) and SEDX (through --env-scenario) both render, so their
	 * images can be compared. Every field has a default, and a file's "defaults" object overrides them for its entries.
	 */
	struct ParityScenario
	{
		std::string id;									// File stem of the images: <id>.png, <id>.json
		std::string icao;								// Airport SEDX opens (and the probe's fallback placement)
		std::string project;							// SEDX project drawn with it (relative to the repository; the probe
														// expects the same scenery installed as a Custom Scenery pack)

		// Camera (true heading clockwise from north, pitch above the horizon, horizontal field of view)
		double latitude = 0.0;
		double longitude = 0.0;
		float altitudeMAgl = 2.0f;
		float headingDeg = 0.0f;
		float pitchDeg = 0.0f;
		float fovDeg = 60.0f;
		uint32_t width = 1280;							// Image size both renderers produce (SEDX renders at it; the
		uint32_t height = 720;							// probe records X-Plane's window size next to it)

		// Date and local time (utcOffsetHours: local = UTC + offset)
		int year = 2026;
		int month = 7;
		int day = 15;
		double localTimeHours = 12.0;
		float utcOffsetHours = 0.0f;

		// Weather and surface
		float visibilityM = 80000.0f;
		std::array<ScenarioCloudLayer, 3> clouds = {};
		float rainPercent = 0.0f;						// 0-1
		int runwayFriction = 0;							// 0-15, sim/weather/region/runway_friction
		float snowCover = 0.0f;							// 0-1
		float temperatureC = 15.0f;

		// Capture
		uint32_t settleFrames = 600;					// Frames before the screenshot: exposure and weather settle
		std::optional<float> ev100Lock;					// Locked exposure (TASK-102); none = auto exposure
		std::vector<std::array<float, 4>> uiMask;		// Rectangles (x0, y0, x1, y1 in 0-1 of the image) the comparison ignores
		std::string notes;
	};

	/* @brief A parity scenario file: what X-Plane build its references belong to and its entries. */
	struct ParityScenarioFile
	{
		std::string xpBuild;							// "12.4.4-b4"
		std::vector<ParityScenario> scenarios;
	};

	/**
	 * @brief Reads a scenario file. Entries take the file's "defaults" first, then their own fields; "date" is
	 * "YYYY-MM-DD" and "local_time" "HH:MM[:SS]".
	 * @param error Receives the first problem found (a missing id, a malformed date, JSON syntax), with the entry index.
	 * @return False when the file can't be read or parsed; entries are only appended on success.
	 */
	bool ParseParityScenarios(std::string_view json, ParityScenarioFile &out, std::string *error = nullptr);
	bool LoadParityScenarios(const std::filesystem::path &path, ParityScenarioFile &out, std::string *error = nullptr);

	/* @brief The entry with an id, or nullptr. */
	const ParityScenario *FindParityScenario(const ParityScenarioFile &file, std::string_view id);

	/* @brief "YYYY-MM-DD" -> year, month, day (validated against the month's length). */
	bool ParseIsoDate(std::string_view text, int &year, int &month, int &day);

	/* @brief "HH:MM" or "HH:MM:SS" -> hours (0-24). */
	bool ParseClockTime(std::string_view text, double &hours);

	/* @brief Day of the year, 1 = January 1st. */
	int DayOfYear(int year, int month, int day);
}

// -------------------------------------------------------
