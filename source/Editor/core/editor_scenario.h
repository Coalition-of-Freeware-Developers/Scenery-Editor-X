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
 * editor_scenario.h
 * -------------------------------------------------------
 * Created: 20/10/2026
 * -------------------------------------------------------
 */
#pragma once
#include "editor_context.h"
#include <SceneryEditorX/environment/parity_scenario.h>

// -------------------------------------------------------

namespace SceneryEditorX::EditorScenario
{
	/**
	 * Parity scenarios from the command line (plan TASK-005): renders one entry of a scenario file the way the X-Plane 12
	 * reference probe renders it, and optionally writes the frame to a PNG and exits.
	 *
	 *   --env-scenario <file>            scenario file (tools/xp12_reference_probe/scenarios/xp12_parity.json)
	 *   --env-scenario-index <n | id>    entry to render (default 0)
	 *   --capture-png <path>             write the frame (and <name>_sky.png) once the scene has settled, then exit; without
	 *                                    --env-scenario: the startup view, after its --xp-airport (if any) has opened
	 *   --capture-after-frames <n>       frames to wait after the camera is placed (default: the scenario's settle_frames)
	 *   --reference-image <png>          load a reference into the viewport REFERENCE overlay (with or without a scenario)
	 *   --reference-view <view>          split (default), difference, flicker or off
	 *
	 * The scenario opens its airport, renders at its width x height with the grid and selection outlines hidden and the
	 * camera locked, and sets the date, time and weather the environment model supports.
	 */

	/* @brief Reads the command line. Call once, before the first Tick(). */
	void Init();

	/* @brief Drives the active scenario: opens its airport, places the camera, counts frames, captures, exits. */
	void Tick(EditorContext *context);

	/* @brief The scenario being rendered, or nullptr. */
	const Environment::ParityScenario *GetActive();

	/* @brief Starts rendering a scenario now (opens its airport when another one is open). */
	void Start(const Environment::ParityScenario &scenario);
}

// -------------------------------------------------------
