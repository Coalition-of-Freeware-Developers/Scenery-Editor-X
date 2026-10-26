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
 * environment.cpp
 * -------------------------------------------------------
 * Created: 20/10/2026
 * -------------------------------------------------------
 */
#include "environment.h"
#include "parity_scenario.h"
#include <algorithm>
#include <cmath>

// -------------------------------------------------------

namespace SceneryEditorX::Environment
{
	World::World()
	{
		m_Integrator.Reset(DecodeRunwayFriction(m_State.runwayFriction));
		Rederive();
	}

	void World::SetState(const EnvironmentState &state)
	{
		if (state == m_State)
			return;
		const bool surfaceModeChanged = state.integrateSurface != m_State.integrateSurface || state.runwayFriction != m_State.runwayFriction;
		m_State = state;
		if (surfaceModeChanged)
			m_Integrator.Reset(DecodeRunwayFriction(m_State.runwayFriction));
		Rederive();
	}

	void World::SetAirportLocation(const double latitudeDeg, const double longitudeDeg, const float elevationM)
	{
		if (!m_State.followAirport)
			return;
		Edit([&](EnvironmentState &s)
		{
			s.latitudeDeg = latitudeDeg;
			s.longitudeDeg = longitudeDeg;
			s.elevationM = elevationM;
		});
	}

	void World::SetObserverAltitude(const float altitudeMslM)
	{
		// The altitude only matters to the sky classes above 6 km: re-derive on real changes only
		if (std::abs(altitudeMslM - m_ObserverAltitudeM) < 50.0f)
			return;
		const bool mattered = std::max(altitudeMslM, m_ObserverAltitudeM) > 5000.0f;
		m_ObserverAltitudeM = altitudeMslM;
		if (mattered)
			Rederive();
	}

	void World::Tick(const double deltaSeconds)
	{
		if (!m_State.integrateSurface)
			return;
		const SurfaceWeather before = m_Integrator.Get();
		m_Integrator.Step(static_cast<float>(deltaSeconds), m_State.rainPercent, m_State.temperatureC);
		const SurfaceWeather &after = m_Integrator.Get();
		if (std::abs(after.wetness - before.wetness) > 1e-3f || std::abs(after.puddles - before.puddles) > 1e-3f)
			Rederive();
	}

	void World::ApplyScenario(const ParityScenario &scenario)
	{
		SetSceneryDate(scenario.sceneryDate);
		Edit([&](EnvironmentState &s)
		{
			s.year = scenario.year;
			s.month = scenario.month;
			s.day = scenario.day;
			s.localTimeHours = scenario.localTimeHours;
			s.utcOffsetHours = scenario.utcOffsetHours;
			// The camera's position stands in for the place until the airport opens (the airport then takes over)
			s.latitudeDeg = scenario.latitude;
			s.longitudeDeg = scenario.longitude;
			s.visibilityM = scenario.visibilityM;
			for (size_t i = 0; i < s.clouds.size(); ++i)
			{
				s.clouds[i].baseMslM = scenario.clouds[i].baseMslM;
				s.clouds[i].topMslM = scenario.clouds[i].topMslM;
				s.clouds[i].coverage = scenario.clouds[i].coverage;
				s.clouds[i].type = static_cast<float>(scenario.clouds[i].type);
			}
			s.rainPercent = scenario.rainPercent;
			s.runwayFriction = scenario.runwayFriction;
			s.snowCover = scenario.snowCover;
			s.temperatureC = scenario.temperatureC;
			if (scenario.waveAmplitudeM)
				s.waveAmplitudeM = std::max(*scenario.waveAmplitudeM, 0.0f);
			if (scenario.waveDirectionDeg)
				s.waveDirectionDegT = *scenario.waveDirectionDeg;
			s.integrateSurface = false;
			s.season = SeasonOverride::Auto;
			s.seasonRatioOverride = -1.0f;
		});
	}

	void World::SetSceneryDate(const std::optional<std::array<int, 3>> &date)
	{
		const std::optional<int> day = date ? std::optional<int>(DayOfYear((*date)[0], (*date)[1], (*date)[2])) : std::nullopt;
		if (day == m_SceneryDayOfYear)
			return;
		m_SceneryDayOfYear = day;
		Rederive();
	}

	bool World::LoadSkyConditions(const std::filesystem::path &path, std::string *error)
	{
		if (!m_SkyTable.Load(path, error))
			return false;
		Rederive();
		return true;
	}

	World::Snapshot World::GetSnapshot() const
	{
		std::scoped_lock lock(m_SnapshotMutex);
		return m_Snapshot;
	}

	void World::Rederive()
	{
		const SurfaceWeather integrated = m_Integrator.Get();
		m_Derived = Derive(m_State, m_ObserverAltitudeM, &m_SkyTable, &integrated);
		// Scenery loaded on another date keeps that date's seasonal art (a season the user forces applies to both)
		if (m_SceneryDayOfYear && m_State.season == SeasonOverride::Auto)
			m_Derived.scenerySeason = ComputeSeason(*m_SceneryDayOfYear, m_State.latitudeDeg, nullptr);
		++m_Version;
		std::scoped_lock lock(m_SnapshotMutex);
		m_Snapshot.state = m_State;
		m_Snapshot.derived = m_Derived;
		m_Snapshot.version = m_Version;
	}

	World &Get()
	{
		static World world;
		return world;
	}
}

// -------------------------------------------------------
