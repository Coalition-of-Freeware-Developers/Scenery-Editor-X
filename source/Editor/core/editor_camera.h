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
 * editor_camera.h
 * -------------------------------------------------------
 * Created: 17/10/2026
 * -------------------------------------------------------
 */
#pragma once
#include <SceneryEditorX/scene/camera.h>

// -------------------------------------------------------

namespace SceneryEditorX::EditorCamera
{
	/**
	 * Editor-side camera actions shared by the viewport toolbar, the tool strip and the menus.
	 * They act on the scene camera (Scene::GetCamera()) and are safe to call while building the UI:
	 * the render thread, which ticks the camera, is idle while the main thread runs.
	 */

	/* @brief The camera the viewport renders through, or nullptr before the scene exists. */
	Camera *Get();

	/* @brief Remembers the current view so Reset() can return to it. Called once the scene is up. */
	void CaptureHome();

	/* @brief Restores the view captured by CaptureHome(). */
	void Reset();

	/* @brief Switches between ORBIT and FLY navigation, keeping the current view in place. */
	void SetMode(CameraMode mode);

	/* @brief True when the camera navigates in the given mode. */
	bool IsMode(CameraMode mode);

	/* @brief Switches the projection between perspective and orthographic. */
	void SetProjection(CameraType projection);

	/* @brief True when the camera uses the given projection. */
	bool IsProjection(CameraType projection);

	/* @brief Short display name for the active navigation mode ("Orbit", "Fly"). */
	const char *GetModeName();

}

// -------------------------------------------------------
