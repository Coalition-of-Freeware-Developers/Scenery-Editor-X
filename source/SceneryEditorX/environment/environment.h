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
 * environment.h
 * -------------------------------------------------------
 * Created: 20/10/2026
 * -------------------------------------------------------
 */
#pragma once
#include "environment_state.h"
#include <cstdint>
#include <filesystem>
#include <mutex>

// -------------------------------------------------------

namespace SceneryEditorX::Environment
{
	struct ParityScenario;

	/**
	 * @class World
	 * @brief The one source of truth for the viewport's world environment (plan TASK-014): the state, what follows from
	 * it, and a version that changes whenever either does. The editor's datarefs (sim/time, sun, weather), the scene's
	 * time of day, the ENVIRONMENT panel, project files and (later) the renderer all read and write it here.
	 *
	 * Main thread: Get(), SetState(), Edit(), Tick(). Any thread: GetSnapshot().
	 */
	class World
	{
	public:
		struct Snapshot
		{
			EnvironmentState state;
			EnvironmentDerived derived;
			uint64_t version = 0;
		};

		World();
		World(const World &) = delete;
		World &operator=(const World &) = delete;

		[[nodiscard]] const EnvironmentState &GetState() const { return m_State; }
		[[nodiscard]] const EnvironmentDerived &GetDerived() const { return m_Derived; }
		[[nodiscard]] uint64_t GetVersion() const { return m_Version; }

		/* @brief Replaces the state; re-derives and bumps the version when it changed. */
		void SetState(const EnvironmentState &state);

		/* @brief Edits the state in place: Edit([](EnvironmentState &s) { s.visibilityM = 800.0f; }). */
		template <typename F>
		void Edit(F &&edit)
		{
			EnvironmentState state = m_State;
			edit(state);
			SetState(state);
		}

		/* @brief The place, from an airport's datum, when the state follows the airport. */
		void SetAirportLocation(double latitudeDeg, double longitudeDeg, float elevationM);

		/* @brief The camera's altitude above sea level (for the high-altitude and orbit sky classes). */
		void SetObserverAltitude(float altitudeMslM);

		/* @brief Advances what changes over time (the integrated surface); call once per frame. */
		void Tick(double deltaSeconds);

		/* @brief Sets the date, time, place and weather a parity scenario names. */
		void ApplyScenario(const ParityScenario &scenario);

		/* @brief Loads resources/environment/xp12_sky_conditions.json (sky-class multipliers). */
		bool LoadSkyConditions(const std::filesystem::path &path, std::string *error = nullptr);
		[[nodiscard]] const SkyConditionTable &GetSkyConditions() const { return m_SkyTable; }

		/* @brief A consistent copy for another thread (the renderer). */
		[[nodiscard]] Snapshot GetSnapshot() const;

	private:
		void Rederive();

		EnvironmentState m_State;
		EnvironmentDerived m_Derived;
		SkyConditionTable m_SkyTable;
		SurfaceWeatherIntegrator m_Integrator;
		float m_ObserverAltitudeM = 0.0f;
		uint64_t m_Version = 1;

		mutable std::mutex m_SnapshotMutex;
		Snapshot m_Snapshot;
	};

	/* @brief The editor's world. */
	World &Get();
}

// -------------------------------------------------------
