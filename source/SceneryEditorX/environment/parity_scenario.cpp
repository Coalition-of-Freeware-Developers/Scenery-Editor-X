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
 * parity_scenario.cpp
 * -------------------------------------------------------
 * Created: 20/10/2026
 * -------------------------------------------------------
 */
#include "parity_scenario.h"
#include <algorithm>
#include <charconv>
#include <fstream>
#include <sstream>
#include <nlohmann/json.hpp>

// -------------------------------------------------------

namespace SceneryEditorX::Environment
{
	namespace
	{
		using Json = nlohmann::json;

		bool IsLeapYear(const int year)
		{
			return (year % 4 == 0 && year % 100 != 0) || year % 400 == 0;
		}

		int DaysInMonth(const int year, const int month)
		{
			static constexpr int days[12] = { 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 };
			if (month < 1 || month > 12)
				return 0;
			return days[month - 1] + (month == 2 && IsLeapYear(year) ? 1 : 0);
		}

		bool ParseInt(const std::string_view text, int &value)
		{
			if (text.empty())
				return false;
			const auto [end, ec] = std::from_chars(text.data(), text.data() + text.size(), value);
			return ec == std::errc() && end == text.data() + text.size();
		}

		template <typename T>
		void Read(const Json &object, const char *key, T &value)
		{
			const auto it = object.find(key);
			if (it != object.end() && !it->is_null())
				value = it->get<T>();
		}

		/* @brief Applies the fields an object sets (a file's "defaults" or an entry) over a scenario. */
		bool ApplyFields(const Json &object, ParityScenario &scenario, std::string &error)
		{
			if (!object.is_object())
			{
				error = "not a JSON object";
				return false;
			}

			Read(object, "id", scenario.id);
			Read(object, "icao", scenario.icao);
			Read(object, "project", scenario.project);
			Read(object, "lat", scenario.latitude);
			Read(object, "lon", scenario.longitude);
			Read(object, "alt_m_agl", scenario.altitudeMAgl);
			Read(object, "heading", scenario.headingDeg);
			Read(object, "pitch", scenario.pitchDeg);
			Read(object, "fov", scenario.fovDeg);
			Read(object, "width", scenario.width);
			Read(object, "height", scenario.height);
			Read(object, "utc_offset", scenario.utcOffsetHours);
			Read(object, "visibility_m", scenario.visibilityM);
			Read(object, "rain_percent", scenario.rainPercent);
			Read(object, "runway_friction", scenario.runwayFriction);
			Read(object, "snow_cover", scenario.snowCover);
			Read(object, "temperature_c", scenario.temperatureC);
			Read(object, "settle_frames", scenario.settleFrames);
			Read(object, "notes", scenario.notes);

			if (const auto it = object.find("date"); it != object.end() && it->is_string())
			{
				if (!ParseIsoDate(it->get<std::string>(), scenario.year, scenario.month, scenario.day))
				{
					error = "\"date\" is not YYYY-MM-DD: " + it->get<std::string>();
					return false;
				}
			}
			if (const auto it = object.find("local_time"); it != object.end() && it->is_string())
			{
				if (!ParseClockTime(it->get<std::string>(), scenario.localTimeHours))
				{
					error = "\"local_time\" is not HH:MM[:SS]: " + it->get<std::string>();
					return false;
				}
			}
			if (const auto it = object.find("ev100_lock"); it != object.end())
			{
				if (it->is_null())
					scenario.ev100Lock.reset();
				else
					scenario.ev100Lock = it->get<float>();
			}
			if (const auto it = object.find("clouds"); it != object.end() && it->is_array())
			{
				scenario.clouds = {};
				for (size_t i = 0; i < std::min<size_t>(it->size(), scenario.clouds.size()); ++i)
				{
					const Json &layer = (*it)[i];
					if (!layer.is_object())
						continue;
					Read(layer, "base_msl_m", scenario.clouds[i].baseMslM);
					Read(layer, "top_msl_m", scenario.clouds[i].topMslM);
					Read(layer, "coverage", scenario.clouds[i].coverage);
					Read(layer, "type", scenario.clouds[i].type);
				}
			}
			if (const auto it = object.find("ui_mask"); it != object.end() && it->is_array())
			{
				scenario.uiMask.clear();
				for (const Json &rect : *it)
				{
					if (!rect.is_array() || rect.size() != 4)
					{
						error = "\"ui_mask\" rectangles are [x0, y0, x1, y1]";
						return false;
					}
					scenario.uiMask.push_back({ rect[0].get<float>(), rect[1].get<float>(), rect[2].get<float>(), rect[3].get<float>() });
				}
			}

			scenario.localTimeHours = std::clamp(scenario.localTimeHours, 0.0, 24.0);
			scenario.runwayFriction = std::clamp(scenario.runwayFriction, 0, 15);
			scenario.rainPercent = std::clamp(scenario.rainPercent, 0.0f, 1.0f);
			scenario.snowCover = std::clamp(scenario.snowCover, 0.0f, 1.0f);
			return true;
		}
	}

	bool ParseIsoDate(const std::string_view text, int &year, int &month, int &day)
	{
		if (text.size() != 10 || text[4] != '-' || text[7] != '-')
			return false;
		int y = 0, m = 0, d = 0;
		if (!ParseInt(text.substr(0, 4), y) || !ParseInt(text.substr(5, 2), m) || !ParseInt(text.substr(8, 2), d))
			return false;
		if (m < 1 || m > 12 || d < 1 || d > DaysInMonth(y, m))
			return false;
		year = y;
		month = m;
		day = d;
		return true;
	}

	bool ParseClockTime(const std::string_view text, double &hours)
	{
		// HH:MM or HH:MM:SS, one or two digits per field
		int parts[3] = { 0, 0, 0 };
		size_t count = 0;
		std::string_view rest = text;
		while (true)
		{
			const size_t colon = rest.find(':');
			if (count == 3 || !ParseInt(rest.substr(0, colon), parts[count]) || rest.substr(0, colon).size() > 2)
				return false;
			++count;
			if (colon == std::string_view::npos)
				break;
			rest.remove_prefix(colon + 1);
		}
		if (count < 2 || parts[0] < 0 || parts[0] > 24 || parts[1] < 0 || parts[1] > 59 || parts[2] < 0 || parts[2] > 59)
			return false;
		hours = parts[0] + parts[1] / 60.0 + parts[2] / 3600.0;
		return hours <= 24.0;
	}

	int DayOfYear(const int year, const int month, const int day)
	{
		int days = day;
		for (int m = 1; m < month; ++m)
			days += DaysInMonth(year, m);
		return days;
	}

	bool ParseParityScenarios(const std::string_view json, ParityScenarioFile &out, std::string *error)
	{
		std::string problem;
		const auto fail = [&](const std::string &message)
		{
			if (error)
				*error = message;
			return false;
		};

		Json root;
		try
		{
			root = Json::parse(json.begin(), json.end());
		}
		catch (const std::exception &e)
		{
			return fail(std::string("JSON syntax: ") + e.what());
		}
		if (!root.is_object())
			return fail("the file is not a JSON object");

		ParityScenarioFile file;
		try
		{
			Read(root, "xp_build", file.xpBuild);

			ParityScenario defaults;
			if (const auto it = root.find("defaults"); it != root.end() && !ApplyFields(*it, defaults, problem))
				return fail("defaults: " + problem);

			const auto list = root.find("scenarios");
			if (list == root.end() || !list->is_array())
				return fail("no \"scenarios\" array");

			for (size_t i = 0; i < list->size(); ++i)
			{
				ParityScenario scenario = defaults;
				scenario.id.clear();
				if (!ApplyFields((*list)[i], scenario, problem))
					return fail("scenario " + std::to_string(i) + ": " + problem);
				if (scenario.id.empty())
					return fail("scenario " + std::to_string(i) + ": no \"id\"");
				if (FindParityScenario(file, scenario.id))
					return fail("scenario " + std::to_string(i) + ": duplicate id \"" + scenario.id + "\"");
				if (scenario.width == 0 || scenario.height == 0)
					return fail("scenario \"" + scenario.id + "\": width and height must be positive");
				file.scenarios.push_back(std::move(scenario));
			}
		}
		catch (const std::exception &e)
		{
			// Wrong value types (a string where a number belongs)
			return fail(std::string("value type: ") + e.what());
		}

		out.xpBuild = std::move(file.xpBuild);
		out.scenarios.insert(out.scenarios.end(), std::make_move_iterator(file.scenarios.begin()), std::make_move_iterator(file.scenarios.end()));
		return true;
	}

	bool LoadParityScenarios(const std::filesystem::path &path, ParityScenarioFile &out, std::string *error)
	{
		std::ifstream stream(path, std::ios::binary);
		if (!stream)
		{
			if (error)
				*error = "can't open " + path.string();
			return false;
		}
		std::ostringstream text;
		text << stream.rdbuf();
		return ParseParityScenarios(text.str(), out, error);
	}

	const ParityScenario *FindParityScenario(const ParityScenarioFile &file, const std::string_view id)
	{
		const auto it = std::ranges::find_if(file.scenarios, [id](const ParityScenario &s) { return s.id == id; });
		return it == file.scenarios.end() ? nullptr : &*it;
	}
}

// -------------------------------------------------------
