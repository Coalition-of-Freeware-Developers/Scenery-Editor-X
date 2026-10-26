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
 * environment_panel.cpp
 * -------------------------------------------------------
 * Created: 20/10/2026
 * -------------------------------------------------------
 */
#include "environment_panel.h"
#include <Editor/core/editor_theme.h>
#include <Editor/core/editor_ui.h>
#include <algorithm>
#include <cmath>
#include <cstring>
#include <SceneryEditorX/environment/environment.h>
#include <SceneryEditorX/renderer/font/font_awesome.h>

// -------------------------------------------------------

namespace SceneryEditorX
{
	namespace
	{
		using namespace Environment;

		constexpr float LABEL_WIDTH = 120.0f;

		const char *CompassPoint(const double degrees)
		{
			static constexpr const char *points[16] = { "N", "NNE", "NE", "ENE", "E", "ESE", "SE", "SSE", "S", "SSW", "SW", "WSW", "W", "WNW", "NW", "NNW" };
			const int index = static_cast<int>(std::lround(std::fmod(degrees + 360.0, 360.0) / 22.5)) % 16;
			return points[index];
		}

		bool IsLeap(const int year) { return (year % 4 == 0 && year % 100 != 0) || year % 400 == 0; }

		int DaysIn(const int year, const int month)
		{
			static constexpr int days[12] = { 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 };
			return days[std::clamp(month, 1, 12) - 1] + (month == 2 && IsLeap(year) ? 1 : 0);
		}
	}

	EnvironmentPanel::EnvironmentPanel(EditorContext *context) : m_Context(context)
	{
		m_Title = "ENVIRONMENT";
	}

	void EnvironmentPanel::OnUIRender(bool &isOpen)
	{
		if (!EditorUI::BeginPanel("ENVIRONMENT###Environment", &isOpen))
		{
			EditorUI::EndPanel();
			return;
		}

		World &world = Get();
		EnvironmentState state = world.GetState();
		const EnvironmentDerived &derived = world.GetDerived();
		bool changed = false;

		if (EditorUI::SectionHeader("PRESETS"))
			changed |= DrawPresets(state);
		if (EditorUI::SectionHeader("DATE & TIME"))
			changed |= DrawDateTime(state);
		if (EditorUI::SectionHeader("SUN & MOON"))
			DrawSunMoon(derived);
		if (EditorUI::SectionHeader("WEATHER"))
			changed |= DrawWeather(state);
		if (EditorUI::SectionHeader("SURFACE"))
			changed |= DrawSurface(state, derived);
		if (EditorUI::SectionHeader("SEASON"))
			changed |= DrawSeason(state, derived);
		if (EditorUI::SectionHeader("SKY CONDITION", false))
			DrawSky(derived);

		if (changed)
			world.SetState(state);
		EditorUI::EndPanel();
	}

	bool EnvironmentPanel::DrawPresets(EnvironmentState &state)
	{
		bool changed = false;
		static constexpr const char *presets[] = { "Clear", "Hazy", "Foggy", "Overcast", "Rain", "Snow" };
		const float dpi = ImGui::GetStyle().FontScaleDpi;
		for (size_t i = 0; i < std::size(presets); ++i)
		{
			if (i > 0 && i % 3 != 0)
				ImGui::SameLine();
			if (ImGui::Button(presets[i], ImVec2(80.0f * dpi, 0.0f)))
			{
				ApplyPreset(state, presets[i]);
				changed = true;
			}
		}
		EditorUI::ItemTooltip("Weather of the X-Plane 12 parity scenarios (tools/xp12_reference_probe)");
		return changed;
	}

	void EnvironmentPanel::ApplyPreset(EnvironmentState &state, const char *preset)
	{
		// Keeps date, time and place; replaces weather and surface
		const auto reset = [&state]
		{
			state.clouds = {};
			state.rainPercent = 0.0f;
			state.runwayFriction = 0;
			state.snowCover = 0.0f;
			state.temperatureC = 15.0f;
		};
		reset();
		if (std::strcmp(preset, "Clear") == 0)
			state.visibilityM = 80000.0f;
		else if (std::strcmp(preset, "Hazy") == 0)
			state.visibilityM = 8000.0f;
		else if (std::strcmp(preset, "Foggy") == 0)
			state.visibilityM = 800.0f;
		else if (std::strcmp(preset, "Overcast") == 0)
		{
			state.visibilityM = 30000.0f;
			state.clouds[0] = { state.elevationM + 600.0f, state.elevationM + 1270.0f, 1.0f, 1.0f };	// 8/8 stratus, base 600 m above the ground
		}
		else if (std::strcmp(preset, "Rain") == 0)
		{
			state.visibilityM = 20000.0f;
			state.rainPercent = 0.6f;
			state.runwayFriction = 5;
			state.temperatureC = 12.0f;
			state.clouds[0] = { state.elevationM + 900.0f, state.elevationM + 2500.0f, 0.9f, 1.0f };
		}
		else if (std::strcmp(preset, "Snow") == 0)
		{
			state.visibilityM = 30000.0f;
			state.snowCover = 1.0f;
			state.runwayFriction = 9;
			state.temperatureC = -5.0f;
		}
	}

	bool EnvironmentPanel::DrawDateTime(EnvironmentState &state)
	{
		bool changed = false;
		if (!EditorUI::BeginPropertyGrid("##EnvDateTime", LABEL_WIDTH))
			return false;

		EditorUI::PropertyLabel("Date");
		int date[3] = { state.year, state.month, state.day };
		ImGui::SetNextItemWidth(-FLT_MIN);
		if (ImGui::DragInt3("##Date", date, 0.1f, 1, 9999, "%d"))
		{
			state.year = std::clamp(date[0], 1900, 2100);
			state.month = std::clamp(date[1], 1, 12);
			state.day = std::clamp(date[2], 1, DaysIn(state.year, state.month));
			changed = true;
		}
		EditorUI::ItemTooltip("Year, month, day (local date)");

		EditorUI::PropertyLabel("Local time");
		float hours = static_cast<float>(state.localTimeHours);
		const int totalMinutes = static_cast<int>(std::lround(hours * 60.0f));
		char format[32];
		snprintf(format, sizeof(format), "%02d:%02d", totalMinutes / 60, totalMinutes % 60);
		ImGui::SetNextItemWidth(-FLT_MIN);
		if (ImGui::SliderFloat("##LocalTime", &hours, 0.0f, 24.0f, format))
		{
			state.localTimeHours = std::clamp(static_cast<double>(hours), 0.0, 24.0);
			changed = true;
		}
		EditorUI::ItemTooltip("Drives the sun and moon, sim/time/* and when lights come on");

		EditorUI::PropertyLabel("UTC offset");
		ImGui::SetNextItemWidth(-FLT_MIN);
		changed |= ImGui::SliderFloat("##UtcOffset", &state.utcOffsetHours, -12.0f, 14.0f, "%+.1f h");

		EditorUI::PropertyLabel("Place");
		changed |= ImGui::Checkbox("Use the airport's location", &state.followAirport);
		EditorUI::ItemTooltip("Take latitude, longitude and elevation from the open airport's datum");

		ImGui::BeginDisabled(state.followAirport);
		EditorUI::PropertyLabel("Latitude");
		ImGui::SetNextItemWidth(-FLT_MIN);
		changed |= ImGui::InputDouble("##Latitude", &state.latitudeDeg, 0.0, 0.0, "%.6f");
		EditorUI::PropertyLabel("Longitude");
		ImGui::SetNextItemWidth(-FLT_MIN);
		changed |= ImGui::InputDouble("##Longitude", &state.longitudeDeg, 0.0, 0.0, "%.6f");
		EditorUI::PropertyLabel("Elevation");
		ImGui::SetNextItemWidth(-FLT_MIN);
		changed |= ImGui::DragFloat("##Elevation", &state.elevationM, 1.0f, -500.0f, 9000.0f, "%.0f m");
		ImGui::EndDisabled();
		state.latitudeDeg = std::clamp(state.latitudeDeg, -90.0, 90.0);
		state.longitudeDeg = std::clamp(state.longitudeDeg, -180.0, 180.0);

		EditorUI::EndPropertyGrid();
		return changed;
	}

	void EnvironmentPanel::DrawSunMoon(const EnvironmentDerived &derived)
	{
		if (!EditorUI::BeginPropertyGrid("##EnvSunMoon", LABEL_WIDTH))
			return;
		EditorUI::PropertyLabel("Sun");
		ImGui::Text("%.1f deg up, %.0f deg %s", derived.sun.apparentElevationDeg, derived.sun.azimuthDeg, CompassPoint(derived.sun.azimuthDeg));
		EditorUI::PropertyLabel("Moon");
		ImGui::Text("%.1f deg up, %.0f deg %s", derived.moon.apparentElevationDeg, derived.moon.azimuthDeg, CompassPoint(derived.moon.azimuthDeg));
		EditorUI::PropertyLabel("Moon phase");
		ImGui::Text("%.0f%% lit, %s", derived.moon.illuminatedFraction * 100.0, derived.moon.waxing ? "waxing" : "waning");
		EditorUI::PropertyLabel("UTC");
		const int utcMinutes = static_cast<int>(std::lround(derived.utcHours * 60.0)) % 1440;
		ImGui::Text("%02d:%02d, day %d of the year", utcMinutes / 60, utcMinutes % 60, derived.dayOfYear);
		EditorUI::PropertyLabel("Daylight");
		ImGui::Text("%.0f%% (lights %.0f%%)", derived.daylight * 100.0f, derived.nightFactor * 100.0f);
		EditorUI::EndPropertyGrid();
	}

	bool EnvironmentPanel::DrawWeather(EnvironmentState &state)
	{
		bool changed = false;
		if (!EditorUI::BeginPropertyGrid("##EnvWeather", LABEL_WIDTH))
			return false;

		EditorUI::PropertyLabel("Visibility");
		float km = state.visibilityM / 1000.0f;
		ImGui::SetNextItemWidth(-FLT_MIN);
		if (ImGui::SliderFloat("##Visibility", &km, 0.1f, 100.0f, km < 10.0f ? "%.2f km" : "%.0f km", ImGuiSliderFlags_Logarithmic))
		{
			state.visibilityM = std::clamp(km, 0.1f, 100.0f) * 1000.0f;
			changed = true;
		}

		static constexpr const char *cloudTypes[] = { "Cirrus", "Stratus", "Cumulus", "Cumulonimbus" };
		for (size_t i = 0; i < state.clouds.size(); ++i)
		{
			CloudLayer &layer = state.clouds[i];
			ImGui::PushID(static_cast<int>(i));
			char label[32];
			snprintf(label, sizeof(label), "Cloud layer %d", static_cast<int>(i) + 1);
			EditorUI::PropertyLabel(label);
			float coverage = layer.coverage * 8.0f;
			ImGui::SetNextItemWidth(-FLT_MIN);
			if (ImGui::SliderFloat("##Coverage", &coverage, 0.0f, 8.0f, coverage <= 0.0f ? "none" : "%.0f/8"))
			{
				layer.coverage = std::clamp(coverage / 8.0f, 0.0f, 1.0f);
				if (layer.coverage > 0.0f && layer.topMslM <= layer.baseMslM)
				{
					layer.baseMslM = state.elevationM + 900.0f * static_cast<float>(i + 1);
					layer.topMslM = layer.baseMslM + 600.0f;
				}
				changed = true;
			}
			if (layer.coverage > 0.0f)
			{
				EditorUI::PropertyLabel("  Base / tops");
				float range[2] = { layer.baseMslM, layer.topMslM };
				ImGui::SetNextItemWidth(-FLT_MIN);
				if (ImGui::DragFloat2("##BaseTops", range, 10.0f, 0.0f, 20000.0f, "%.0f m MSL"))
				{
					layer.baseMslM = range[0];
					layer.topMslM = std::max(range[1], range[0] + 10.0f);
					changed = true;
				}
				EditorUI::PropertyLabel("  Type");
				int type = std::clamp(static_cast<int>(std::lround(layer.type)), 0, 3);
				ImGui::SetNextItemWidth(-FLT_MIN);
				if (ImGui::Combo("##Type", &type, cloudTypes, 4))
				{
					layer.type = static_cast<float>(type);
					changed = true;
				}
			}
			ImGui::PopID();
		}

		EditorUI::PropertyLabel("Rain");
		float rain = state.rainPercent * 100.0f;
		ImGui::SetNextItemWidth(-FLT_MIN);
		if (ImGui::SliderFloat("##Rain", &rain, 0.0f, 100.0f, "%.0f%%"))
		{
			state.rainPercent = std::clamp(rain / 100.0f, 0.0f, 1.0f);
			changed = true;
		}
		EditorUI::ItemTooltip("Falls as snow below 0 C");

		EditorUI::PropertyLabel("Temperature");
		ImGui::SetNextItemWidth(-FLT_MIN);
		changed |= ImGui::SliderFloat("##Temperature", &state.temperatureC, -40.0f, 45.0f, "%.0f C");

		EditorUI::PropertyLabel("Wind from");
		ImGui::SetNextItemWidth(-FLT_MIN);
		changed |= ImGui::SliderFloat("##WindDir", &state.windDirectionDegT, 0.0f, 360.0f, "%.0f deg");
		EditorUI::PropertyLabel("Wind speed");
		ImGui::SetNextItemWidth(-FLT_MIN);
		changed |= ImGui::SliderFloat("##WindSpeed", &state.windSpeedKt, 0.0f, 60.0f, "%.0f kt");

		EditorUI::PropertyLabel("Waves");
		ImGui::SetNextItemWidth(-FLT_MIN);
		changed |= ImGui::SliderFloat("##WaveAmplitude", &state.waveAmplitudeM, 0.0f, 10.0f, "%.1f m");
		EditorUI::PropertyLabel("Waves from");
		ImGui::SetNextItemWidth(-FLT_MIN);
		changed |= ImGui::SliderFloat("##WaveDir", &state.waveDirectionDegT, 0.0f, 360.0f, "%.0f deg");

		EditorUI::EndPropertyGrid();
		return changed;
	}

	bool EnvironmentPanel::DrawSurface(EnvironmentState &state, const EnvironmentDerived &derived)
	{
		bool changed = false;
		if (!EditorUI::BeginPropertyGrid("##EnvSurface", LABEL_WIDTH))
			return false;

		EditorUI::PropertyLabel("Runway state");
		ImGui::SetNextItemWidth(-FLT_MIN);
		if (ImGui::BeginCombo("##Friction", GetRunwayFrictionName(state.runwayFriction)))
		{
			for (int f = 0; f <= 15; ++f)
			{
				char label[48];
				snprintf(label, sizeof(label), "%d  %s", f, GetRunwayFrictionName(f));
				if (ImGui::Selectable(label, f == state.runwayFriction))
				{
					state.runwayFriction = f;
					changed = true;
				}
			}
			ImGui::EndCombo();
		}
		EditorUI::ItemTooltip("X-Plane's sim/weather/region/runway_friction: how wet, snowy or icy the ground is");

		EditorUI::PropertyLabel("Snow cover");
		float snow = state.snowCover * 100.0f;
		ImGui::SetNextItemWidth(-FLT_MIN);
		if (ImGui::SliderFloat("##SnowCover", &snow, 0.0f, 100.0f, "%.0f%%"))
		{
			state.snowCover = std::clamp(snow / 100.0f, 0.0f, 1.0f);
			changed = true;
		}

		EditorUI::PropertyLabel("Over time");
		changed |= ImGui::Checkbox("Rain soaks and the sun dries", &state.integrateSurface);
		EditorUI::ItemTooltip("Wetness builds while it rains and dries afterwards, instead of following the runway state");

		EditorUI::PropertyLabel("Result");
		ImGui::TextDisabled("wet %.0f%%  puddles %.0f%%  snow %.0f%%  ice %.0f%%", derived.surface.wetness * 100.0f, derived.surface.puddles * 100.0f,
			derived.surface.snow * 100.0f, derived.surface.ice * 100.0f);

		EditorUI::EndPropertyGrid();
		return changed;
	}

	bool EnvironmentPanel::DrawSeason(EnvironmentState &state, const EnvironmentDerived &derived)
	{
		bool changed = false;
		if (!EditorUI::BeginPropertyGrid("##EnvSeason", LABEL_WIDTH))
			return false;

		EditorUI::PropertyLabel("Season");
		static constexpr const char *seasons[] = { "Auto (date and latitude)", "Spring", "Summer", "Fall", "Winter" };
		int season = static_cast<int>(state.season);
		ImGui::SetNextItemWidth(-FLT_MIN);
		if (ImGui::Combo("##Season", &season, seasons, 5))
		{
			state.season = static_cast<SeasonOverride>(season);
			changed = true;
		}

		EditorUI::PropertyLabel("Toward next");
		bool autoRatio = state.seasonRatioOverride < 0.0f;
		if (ImGui::Checkbox("Auto##Ratio", &autoRatio))
		{
			state.seasonRatioOverride = autoRatio ? -1.0f : derived.season.ratio;
			changed = true;
		}
		if (!autoRatio)
		{
			ImGui::SameLine();
			ImGui::SetNextItemWidth(-FLT_MIN);
			changed |= ImGui::SliderFloat("##Ratio", &state.seasonRatioOverride, 0.0f, 1.0f, "%.2f");
		}

		EditorUI::PropertyLabel("Result");
		ImGui::TextDisabled("%s, %.0f%% toward %s", GetSeasonName(derived.season.season), derived.season.ratio * 100.0f, GetSeasonName(NextSeason(derived.season.season)));

		// A scenario's scenery loaded on another date (World::SetSceneryDate): the terrain keeps that date's season
		if (derived.scenerySeason.bits != derived.season.bits)
		{
			EditorUI::PropertyLabel("Scenery art");
			ImGui::TextDisabled("%s (loaded on another date)", GetSeasonName(derived.scenerySeason.season));
			ImGui::SameLine();
			if (ImGui::SmallButton("Follow the date"))
				Get().SetSceneryDate(std::nullopt);
		}

		EditorUI::EndPropertyGrid();
		return changed;
	}

	void EnvironmentPanel::DrawSky(const EnvironmentDerived &derived)
	{
		if (!EditorUI::BeginPropertyGrid("##EnvSky", LABEL_WIDTH))
			return;
		for (size_t i = 0; i < SKY_CONDITION_COUNT; ++i)
		{
			if (derived.skyWeights[i] <= 0.005f)
				continue;
			EditorUI::PropertyLabel(GetSkyConditionCode(static_cast<SkyCondition>(i)));
			ImGui::ProgressBar(derived.skyWeights[i], ImVec2(-FLT_MIN, 0.0f));
		}
		EditorUI::PropertyLabel("Multipliers");
		ImGui::TextDisabled("ambient %.2f  direct %.2f  tone %.2f", derived.sky.ambient, derived.sky.direct, derived.sky.tone);
		EditorUI::EndPropertyGrid();
	}
}

// -------------------------------------------------------
