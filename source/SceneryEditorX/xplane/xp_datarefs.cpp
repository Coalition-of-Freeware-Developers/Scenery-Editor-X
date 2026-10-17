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
 * xp_datarefs.cpp
 * -------------------------------------------------------
 * Created: 17/10/2026
 * -------------------------------------------------------
 */
#include "xp_datarefs.h"
#include <algorithm>
#include <cmath>

// -------------------------------------------------------

namespace SceneryEditorX::XPlane
{
	namespace
	{
		constexpr float PI = 3.14159265358979323846f;

		std::map<std::string, DatarefControl> s_Controls;
		DatarefEnvironment s_Environment;
		double s_SimTime = 0.0;
		bool s_Playing = true;
		float s_TimeScale = 1.0f;

		float LocalTimeSeconds()
		{
			return std::clamp(s_Environment.localTimeHours, 0.0f, 24.0f) * 3600.0f;
		}

		/* @brief Rough sun elevation for a mid-latitude equinox: -90 at midnight, +60 at noon. */
		float SunPitchDegrees()
		{
			const float hours = std::clamp(s_Environment.localTimeHours, 0.0f, 24.0f);
			return std::sin((hours - 6.0f) / 12.0f * PI) * (hours >= 6.0f && hours <= 18.0f ? 60.0f : 90.0f);
		}

		/* @brief Values for the datarefs the editor derives from the environment (returns false for others). */
		bool EvaluateBuiltin(const std::string_view name, float &value)
		{
			if (name == "sim/time/total_running_time_sec" || name == "sim/time/total_flight_time_sec" || name == "sim/time/running_time_sec")
			{
				value = static_cast<float>(s_SimTime);
				return true;
			}

			if (name == "sim/time/local_time_sec" || name == "sim/time/zulu_time_sec")
			{
				value = LocalTimeSeconds();
				return true;
			}

			if (name == "sim/graphics/scenery/sun_pitch_degrees")
			{
				value = SunPitchDegrees();
				return true;
			}

			if (name == "sim/graphics/scenery/percent_lights_on")
			{
				value = Datarefs::GetNightFactor();
				return true;
			}

			if (name == "sim/weather/wind_direction_degt" || name == "sim/weather/aircraft/wind_direction_degt")
			{
				value = s_Environment.windDirectionDeg;
				return true;
			}

			if (name == "sim/weather/wind_speed_kt" || name == "sim/weather/aircraft/wind_speed_kts")
			{
				value = s_Environment.windSpeedKnots;
				return true;
			}

			// Windsocks: psi is the heading the sock points to (downwind), the is the droop (-90 limp .. 0 straight out).
			if (name == "sim/graphics/animation/windsock_psi")
			{
				value = std::fmod(s_Environment.windDirectionDeg + 180.0f, 360.0f);
				return true;
			}

			if (name == "sim/graphics/animation/windsock_the")
			{
				const float strength = std::clamp(s_Environment.windSpeedKnots / 15.0f, 0.0f, 1.0f);
				const float gust = std::sin(static_cast<float>(s_SimTime) * 1.7f) * 3.0f * strength;
				value = std::clamp(-90.0f + strength * 90.0f + gust, -90.0f, 0.0f);
				return true;
			}

			if (name == "sim/graphics/animation/airport_beacon_rotation")
			{
				value = std::fmod(static_cast<float>(s_SimTime) * 72.0f, 360.0f); // 12 rpm
				return true;
			}

			return false;
		}

		DatarefDriver DefaultDriver(const std::string_view name)
		{
			if (Datarefs::IsBuiltin(name))
				return DatarefDriver::Builtin;

			if (name.find("/time/") != std::string_view::npos || name.find("running_time") != std::string_view::npos)
				return DatarefDriver::Clock;

			// The simulator animates these continuously (buoys, flags, radar dishes, ...).
			if (name.starts_with("sim/graphics/animation/"))
				return DatarefDriver::Sweep;

			return DatarefDriver::Value;
		}

		float EvaluateControl(const DatarefControl &control, const std::string_view name)
		{
			switch (control.driver)
			{
				case DatarefDriver::Value:
					return control.value;

				case DatarefDriver::Sweep:
				{
					const float period = std::max(0.05f, control.period);
					const float phase = static_cast<float>(std::fmod(s_SimTime, static_cast<double>(period))) / period;
					const float t = 0.5f - 0.5f * std::cos(phase * 2.0f * PI); // smooth ping-pong 0 -> 1 -> 0
					return control.minValue + (control.maxValue - control.minValue) * t;
				}

				case DatarefDriver::Clock:
					return static_cast<float>(s_SimTime) * control.rate;

				case DatarefDriver::Builtin:
				{
					float value = 0.0f;
					return EvaluateBuiltin(name, value) ? value : control.value;
				}
			}
			return control.value;
		}
	}

	// -------------------------------------------------------

	const char *GetDatarefDriverName(const DatarefDriver driver)
	{
		switch (driver)
		{
			case DatarefDriver::Value:		return "Value";
			case DatarefDriver::Sweep:		return "Sweep";
			case DatarefDriver::Clock:		return "Sim Clock";
			case DatarefDriver::Builtin:	return "Auto";
		}
		return "";
	}

	void Datarefs::Tick(const double deltaSeconds)
	{
		if (s_Playing)
			s_SimTime += std::clamp(deltaSeconds, 0.0, 0.25) * static_cast<double>(s_TimeScale);

		// Keep the displayed values current for the UI.
		for (auto &[key, control] : s_Controls)
		{
			if (control.driver != DatarefDriver::Value)
			{
				const size_t bracket = key.find('[');
				control.value = EvaluateControl(control, bracket == std::string::npos ? std::string_view(key) : std::string_view(key).substr(0, bracket));
			}
		}
	}

	float Datarefs::Get(const DatarefRef &ref)
	{
		if (!ref.IsSet())
			return 0.0f;

		if (ref.index >= 0)
		{
			if (const auto it = s_Controls.find(ref.ToString()); it != s_Controls.end())
				return EvaluateControl(it->second, ref.name);
		}

		if (const auto it = s_Controls.find(ref.name); it != s_Controls.end())
			return EvaluateControl(it->second, ref.name);

		float value = 0.0f;
		return EvaluateBuiltin(ref.name, value) ? value : 0.0f;
	}

	DatarefReader Datarefs::GetReader()
	{
		return [](const DatarefRef &ref) { return Get(ref); };
	}

	DatarefControl &Datarefs::Register(const Obj8DatarefUsage &usage)
	{
		const std::string key = usage.dataref.ToString();
		auto [it, inserted] = s_Controls.try_emplace(key);
		DatarefControl &control = it->second;
		if (inserted)
		{
			control.key = key;
			control.driver = DefaultDriver(usage.dataref.name);
			control.minValue = usage.minValue;
			control.maxValue = usage.maxValue;
			control.value = usage.minValue;

			// Clocks for looping animations run at real time; sweeps take a few seconds per cycle.
			control.period = 4.0f;
			control.rate = 1.0f;
		}
		else
		{
			control.minValue = std::min(control.minValue, usage.minValue);
			control.maxValue = std::max(control.maxValue, usage.maxValue);
		}

		if (control.maxValue <= control.minValue)
			control.maxValue = control.minValue + 1.0f;

		++control.users;
		return control;
	}

	DatarefControl *Datarefs::Find(const std::string &key)
	{
		const auto it = s_Controls.find(key);
		return it != s_Controls.end() ? &it->second : nullptr;
	}

	std::map<std::string, DatarefControl> &Datarefs::GetControls()
	{
		return s_Controls;
	}

	void Datarefs::SetValue(const std::string &key, const float value)
	{
		if (DatarefControl *control = Find(key))
		{
			control->driver = DatarefDriver::Value;
			control->value = value;
		}
	}

	bool Datarefs::IsPlaying() { return s_Playing; }
	void Datarefs::SetPlaying(const bool playing) { s_Playing = playing; }
	float Datarefs::GetTimeScale() { return s_TimeScale; }
	void Datarefs::SetTimeScale(const float scale) { s_TimeScale = std::clamp(scale, 0.0f, 100.0f); }
	double Datarefs::GetSimTime() { return s_SimTime; }
	void Datarefs::ResetClock() { s_SimTime = 0.0; }
	DatarefEnvironment &Datarefs::GetEnvironment() { return s_Environment; }

	float Datarefs::GetNightFactor()
	{
		if (s_Environment.forceLightsOn)
			return 1.0f;

		// Lights come on as the sun drops below a few degrees above the horizon.
		return std::clamp((3.0f - SunPitchDegrees()) / 9.0f, 0.0f, 1.0f);
	}

	bool Datarefs::IsBuiltin(const std::string_view name)
	{
		float value = 0.0f;
		return EvaluateBuiltin(name, value);
	}

}

// -------------------------------------------------------
