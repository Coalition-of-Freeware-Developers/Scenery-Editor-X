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
 * editor_scenario.cpp
 * -------------------------------------------------------
 * Created: 20/10/2026
 * -------------------------------------------------------
 */
#include "editor_scenario.h"
#include "editor_airports.h"
#include "editor_camera.h"
#include "editor_project.h"
#include <Editor/panels/scene_viewport.h>
#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <optional>
#include <SceneryEditorX/core/application/application.h>
#include <SceneryEditorX/logging/logging.hpp>
#include <SceneryEditorX/renderer/frame_capture.h>
#include <SceneryEditorX/renderer/renderer.h>
#include <SceneryEditorX/renderer/scene_picking.h>
#include <SceneryEditorX/scene/camera.h>
#include <SceneryEditorX/scene/scene.h>
#include <SceneryEditorX/xplane/xp_airport_scene.h>
#include <SceneryEditorX/xplane/xp_scene.h>
#include <SceneryEditorX/environment/environment.h>

// -------------------------------------------------------

namespace SceneryEditorX::EditorScenario
{
	namespace
	{
		constexpr const char *LOG_TAG = "Scenario";
		constexpr uint32_t MAX_LOADING_FRAMES = 7200;	// Settle anyway after this long waiting for the scenery (an asset that never loads)

		enum class Stage : uint8_t
		{
			Idle,
			WaitingForAirport,		// The airport (and the library index it waits for) is loading
			WaitingForProject,		// The scenario's project (drawn in the airport's frame) is opening
			Settling,				// Camera placed; counting frames while assets stream in and lighting settles
			FreeSettling,			// --capture-png without a scenario: the startup view, once its airport (if any) is up
			Capturing,				// FrameCapture request in flight
			Done
		};

		struct State
		{
			std::optional<Environment::ParityScenario> active;
			Stage stage = Stage::Idle;
			uint32_t framesLeft = 0;
			std::optional<uint32_t> captureAfterFrames;		// --capture-after-frames
			std::filesystem::path capturePng;				// --capture-png
			bool exitAfterCapture = false;
			bool waitForAirport = false;					// --xp-airport given: free captures wait for it
			bool projectEnvironmentReplaced = false;		// The scenario's environment applied again after its project opened
			bool editorKindsHidden = false;					// The airport's editor-only elements are hidden for the capture
			uint32_t loadingFrames = 0;						// Frames the settling waited for objects still loading
			uint64_t structure = 0;							// Scene structure version last frame (entities still being added)
		};

		State &GetState()
		{
			static State state;
			return state;
		}

		/* @brief Sets the primary camera entity's horizontal field of view (what the renderer projects with). */
		void SetFieldOfView(const float degrees)
		{
			if (Camera *camera = EditorCamera::Get())
				camera->SetFovHorizontalDeg(degrees);

			Scene *scene = Scene::GetActive();
			if (!scene)
				return;
			auto view = scene->GetRegistry().view<CameraComponent>();
			for (const entt::entity handle : view)
			{
				CameraComponent &component = view.get<CameraComponent>(handle);
				if (component.isPrimary)
					component.horizontalFov_Rad = degrees * xMath::DEG_TO_RAD;
			}
		}

		/**
		 * @brief The scenario's date, time and weather into the world environment (the airport's datum then sets the place),
		 * and its exposure: ev100_lock holds a fixed EV100 (plan TASK-102), none meters as X-Plane does.
		 */
		void ApplyEnvironment(const Environment::ParityScenario &scenario)
		{
			Environment::Get().ApplyScenario(scenario);
			ExposureSettings &exposure = Renderer::EditEnvironmentConstants().exposure;
			exposure.automatic = !scenario.ev100Lock.has_value();
			if (scenario.ev100Lock)
				exposure.manualEv100 = *scenario.ev100Lock;
			Renderer::ResetExposure();
		}

		/* @brief Waits for the capture to be written, logs it and exits when asked to. */
		void FinishCapture(State &state, const std::string &what)
		{
			FrameCapture::Result result;
			if (!FrameCapture::TakeResult(result))
				return;
			state.stage = Stage::Done;
			if (result.success)
			{
				EDITOR_INFO_TAG(LOG_TAG, "Scenario {} captured to {} ({})", what, result.png.string(), result.message);
				const Renderer::ExposureReadout exposure = Renderer::GetExposureReadout();
				if (exposure.valid)
					EDITOR_INFO_TAG(LOG_TAG, "Scenario {} exposure: EV100 {:.2f}, metered {:.2f} (raw {:.2f}), target {:.2f}, mean {:.4g} nits", what,
						exposure.ev100, exposure.meteredEv100, exposure.rawEv100, exposure.targetEv100, std::exp2(exposure.meanLog2Nits));
			}
			else
				EDITOR_ERROR_TAG(LOG_TAG, "Scenario {} capture failed: {}", what, result.message);
			if (state.exitAfterCapture)
			{
				EDITOR_INFO_TAG(LOG_TAG, "Exiting after the capture (--capture-png)");
				Application::Get().Stop();
			}
		}

		bool PlaceCamera(const Environment::ParityScenario &scenario)
		{
			SetFieldOfView(scenario.fovDeg);
			return XPlane::SceneAirports::LookFromGeo(scenario.latitude, scenario.longitude, scenario.altitudeMAgl, scenario.headingDeg, scenario.pitchDeg);
		}
	}

	// -------------------------------------------------------

	void Init()
	{
		const PlatformContext *platform = Application::Get().GetPlatformContext();
		if (!platform)
			return;

		State &state = GetState();
		const std::vector<std::string> &args = platform->GetCommandLineArgs();
		std::filesystem::path file;
		std::string selector = "0";
		std::filesystem::path referenceImage;			// --reference-image <png>: the viewport REFERENCE overlay
		SceneViewport::ReferenceView referenceView = SceneViewport::ReferenceView::Split;	// --reference-view split|difference|flicker|off
		for (size_t i = 0; i + 1 < args.size(); ++i)
		{
			if (args[i] == "--env-scenario")
				file = args[++i];
			else if (args[i] == "--env-scenario-index")
				selector = args[++i];
			else if (args[i] == "--capture-png")
			{
				state.capturePng = args[++i];
				state.exitAfterCapture = true;
			}
			else if (args[i] == "--capture-after-frames")
				state.captureAfterFrames = static_cast<uint32_t>(std::max(0, std::atoi(args[++i].c_str())));
			else if (args[i] == "--xp-airport")
				state.waitForAirport = true;
			else if (args[i] == "--reference-image")
				referenceImage = args[++i];
			else if (args[i] == "--dump-brdf-lut")
				FrameCapture::RequestDump(args[++i]);	// The GPU BRDF table, raw RG32F (BrdfLutTests reads it)
			else if (args[i] == "--reference-view")
			{
				const std::string &view = args[++i];
				referenceView = view == "difference" ? SceneViewport::ReferenceView::Difference
							  : view == "flicker"    ? SceneViewport::ReferenceView::Flicker
							  : view == "off"        ? SceneViewport::ReferenceView::Off
													 : SceneViewport::ReferenceView::Split;
			}
		}
		if (!referenceImage.empty())
			SceneViewport::RequestReference(referenceImage, referenceView);

		if (file.empty())
		{
			// No scenario: capture the startup view as it is (A/B comparisons of renderer changes)
			if (!state.capturePng.empty())
			{
				state.stage = Stage::FreeSettling;
				state.framesLeft = state.captureAfterFrames.value_or(600);
				EDITOR_INFO_TAG(LOG_TAG, "Capturing the startup view {} frames after it is ready", state.framesLeft);
			}
			return;
		}

		Environment::ParityScenarioFile scenarios;
		std::string error;
		if (!Environment::LoadParityScenarios(file, scenarios, &error))
		{
			EDITOR_ERROR_TAG(LOG_TAG, "--env-scenario {}: {}", file.string(), error);
			return;
		}

		// An index, else an id
		const Environment::ParityScenario *scenario = nullptr;
		char *end = nullptr;
		const long index = std::strtol(selector.c_str(), &end, 10);
		if (end && *end == '\0' && index >= 0 && static_cast<size_t>(index) < scenarios.scenarios.size())
			scenario = &scenarios.scenarios[static_cast<size_t>(index)];
		else
			scenario = Environment::FindParityScenario(scenarios, selector);
		if (!scenario)
		{
			EDITOR_ERROR_TAG(LOG_TAG, "--env-scenario-index {}: no such entry in {} ({} scenarios)", selector, file.string(), scenarios.scenarios.size());
			return;
		}

		EDITOR_INFO_TAG(LOG_TAG, "Scenario {} from {} (X-Plane {})", scenario->id, file.string(), scenarios.xpBuild);
		Start(*scenario);
	}

	void Start(const Environment::ParityScenario &scenario)
	{
		State &state = GetState();
		state.active = scenario;
		state.stage = Stage::WaitingForAirport;

		// Same image size and aspect as the reference; nothing editor-only in the image; the view stays put
		SceneViewport::SetFixedRenderSize(scenario.width, scenario.height);
		Renderer::SetGridVisible(false);
		ScenePicking::SetOutlineEnabled(false);
		Camera::SetInputLocked(true);
		Renderer::SetWaterFixedTimeStep(1.0f / 60.0f);	// The same sea every run: waves advance by frames, not the clock
		ApplyEnvironment(scenario);

		// The art controls the shot sets that SEDX emulates (the probe sets every one in X-Plane)
		for (const auto &[name, value] : scenario.artControls)
		{
			if (name == "ssao/enable")
			{
				RendererQualityPreset preset = Renderer::GetQualityPreset();
				preset.ao = value > 0.5f ? std::max(preset.ao, 1) : 0;
				Renderer::SetQualityPreset(preset);
			}
			else if (name == "debug/kill_ssr")
			{
				RendererFeatures features = Renderer::GetFeatures();
				features.ssr = value < 0.5f;	// X-Plane's switch for its screen-space reflections: the probes alone
				Renderer::SetFeatures(features);
			}
			else
			{
				EDITOR_WARN_TAG(LOG_TAG, "Scenario {}: art control {} isn't emulated, ignored", scenario.id, name);
			}
		}

		if (!scenario.icao.empty())
			EditorAirports::RequestOpenWhenReady(scenario.icao);
		else
			EDITOR_WARN_TAG(LOG_TAG, "Scenario {} names no airport: the camera can't be placed", scenario.id);
	}

	const Environment::ParityScenario *GetActive()
	{
		const State &state = GetState();
		return state.active ? &*state.active : nullptr;
	}

	void Tick(EditorContext *context)
	{
		(void)context;
		State &state = GetState();
		if (state.stage == Stage::FreeSettling)
		{
			if (state.waitForAirport && !XPlane::SceneAirports::IsLoaded())
				return;
			if (state.framesLeft > 0)
			{
				--state.framesLeft;
				return;
			}
			FrameCapture::Request(state.capturePng, true);
			state.stage = Stage::Capturing;
			return;
		}
		if (state.stage == Stage::Capturing && !state.active)
		{
			FinishCapture(state, "startup view");
			return;
		}
		if (!state.active)
			return;
		const Environment::ParityScenario &scenario = *state.active;

		switch (state.stage)
		{
			case Stage::WaitingForAirport:
			{
				if (!XPlane::SceneAirports::IsLoaded() || !EditorAirports::GetDocument())
					return;
				if (!scenario.project.empty())
				{
					// Opened once the airport is up, so its items are drawn in the airport's frame
					std::filesystem::path project = scenario.project;
					if (project.is_relative())
						project = std::filesystem::current_path() / project;
					EditorProjects::OpenWhenReady(project);
					state.stage = Stage::WaitingForProject;
					return;
				}
				[[fallthrough]];
			}

			case Stage::WaitingForProject:
			{
				if (!scenario.project.empty() && !EditorProjects::GetDocument())
					return;
				if (!scenario.project.empty() && state.stage == Stage::WaitingForProject && !state.projectEnvironmentReplaced)
				{
					// Opening the project put back the world it was saved in (its Environment block): the scenario's wins
					ApplyEnvironment(scenario);
					state.projectEnvironmentReplaced = true;
				}
				if (!state.editorKindsHidden)
				{
					// What X-Plane never draws: the ATC taxi network, the boundary, viewpoints, startup and truck locations
					// (the editor shows them as lit ribbons and markers over the pavement)
					using XPlane::AirportElementKind;
					for (const AirportElementKind kind : { AirportElementKind::TaxiRoutes, AirportElementKind::TaxiNodes, AirportElementKind::Boundary,
														   AirportElementKind::Viewpoint, AirportElementKind::Startup, AirportElementKind::LegacyStartup,
														   AirportElementKind::TruckParking, AirportElementKind::TruckDestination })
						XPlane::SceneAirports::SetKindVisible(kind, false);
					state.editorKindsHidden = true;
				}
				if (!PlaceCamera(scenario))
					return;
				Renderer::ResetExposure();	// Meter the placed view from scratch, not adapt to it from the startup view
				Renderer::ResetWaterTime();	// The capture sees the waves at the same time every run
				state.framesLeft = state.captureAfterFrames.value_or(scenario.settleFrames);
				state.stage = Stage::Settling;
				EDITOR_INFO_TAG(LOG_TAG, "Scenario {}: camera at {:.6f}, {:.6f}, {} m, heading {}, pitch {}; {} frames to settle", scenario.id,
					scenario.latitude, scenario.longitude, scenario.altitudeMAgl, scenario.headingDeg, scenario.pitchDeg, state.framesLeft);
				return;
			}

			case Stage::Settling:
			{
				// A rebuild (library index change) may recreate the scene's entities: keep the view on the scenario
				if ((state.framesLeft + state.loadingFrames) % 60 == 0)
					PlaceCamera(scenario);

				// The settle frames count once the scenery has loaded: objects still waiting for their assets, or entities
				// still being added, would leave the capture half drawn (payware airports stream in for many seconds)
				const uint64_t structure = Scene::GetStructureVersion();
				const bool loading = XPlane::SceneObjects::GetLoadingObjectCount() > 0 || structure != state.structure;
				state.structure = structure;
				if (loading && state.loadingFrames < MAX_LOADING_FRAMES)
				{
					if (++state.loadingFrames == MAX_LOADING_FRAMES)
						EDITOR_WARN_TAG(LOG_TAG, "Scenario {}: still loading after {} frames; settling anyway", scenario.id, MAX_LOADING_FRAMES);
					return;
				}
				if (state.framesLeft == state.captureAfterFrames.value_or(scenario.settleFrames) && state.loadingFrames > 1)
					EDITOR_INFO_TAG(LOG_TAG, "Scenario {}: scenery loaded after {} frames", scenario.id, state.loadingFrames);
				if (state.framesLeft > 0)
				{
					--state.framesLeft;
					return;
				}
				if (state.capturePng.empty())
				{
					state.stage = Stage::Done;
					EDITOR_INFO_TAG(LOG_TAG, "Scenario {} has settled", scenario.id);
					return;
				}
				FrameCapture::Request(state.capturePng, true);
				state.stage = Stage::Capturing;
				return;
			}

			case Stage::Capturing:
				FinishCapture(state, scenario.id);
				return;

			default:
				return;
		}
	}
}

// -------------------------------------------------------
