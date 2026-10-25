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
#include <Editor/core/viewport_camera.h>
#include <SceneryEditorX/scene/camera.h>

// -------------------------------------------------------

namespace SceneryEditorX
{
	struct EditorContext;
}

namespace SceneryEditorX::EditorCamera
{
	/**
	 * Editor-side camera actions shared by the viewport toolbar, the tool strip and the menus, and the main viewport's
	 * navigation (ViewportCamera::Controller with the user's keymap). They act on the scene camera (Scene::GetCamera()
	 * and the primary camera entity the renderer draws through) and are safe to call while building the UI: the render
	 * thread, which ticks the camera, is idle while the main thread runs.
	 */

	/* @brief The camera the viewport renders through, or nullptr before the scene exists. */
	Camera *Get();

	/* @brief Remembers the current view so Reset() can return to it. Called once the scene is up. */
	void CaptureHome();

	/**
	 * @brief Startup view flags: --camera-view top|bottom|front|back|left|right and --camera-ortho, applied once the
	 * startup airport or project has opened (and framed the camera). For captures and repeatable checks.
	 */
	void ReadCommandLine();

	/* @brief Restores the view captured by CaptureHome(). */
	void Reset();

	/* @brief Switches between ORBIT and FLY navigation, keeping the current view in place. */
	void SetMode(CameraMode mode);

	/* @brief True when the camera navigates in the given mode. */
	bool IsMode(CameraMode mode);

	/* @brief Switches the projection between perspective and orthographic, keeping the framing at the orbit pivot. */
	void SetProjection(CameraType projection);

	/* @brief True when the camera uses the given projection. */
	bool IsProjection(CameraType projection);

	/* @brief Perspective <-> orthographic (numpad 5). */
	void ToggleProjection();

	/* @brief Short display name for the active navigation mode ("Orbit", "Fly"). */
	const char *GetModeName();

	/* @brief The main camera as a rig (false before the scene has a camera). */
	bool GetRig(ViewportCamera::Rig &rig);

	/* @brief Moves the main camera to a rig (the entity the renderer reads and the Camera's orbit state). */
	void SetRig(const ViewportCamera::Rig &rig);

	/* @brief Looks along a preset axis (top, front...) around the pivot, gliding there. */
	void SetViewPreset(ViewportCamera::ViewPreset preset);

	/* @brief The preset the camera looks along, or Free once it was turned away from it. */
	ViewportCamera::ViewPreset GetViewPreset();

	/* @brief Fits a sphere in view without turning. */
	void Frame(const Point3 &center, double radius);

	/* @brief Fits everything: the open airport, else the project's scenery, else whatever the scene draws. */
	bool FrameAll();

	/* @brief Orbit steps (numpad 4 / 6 / 8 / 2) and the opposite side (numpad 9). */
	void Orbit(double yawDegrees, double pitchDegrees);
	void ViewFromOtherSide();

	/**
	 * @brief Navigation for this frame (the viewport panel calls it once per frame): mouse drags and the wheel with
	 * the keymap, fly keys, running view transitions, and the orthographic height for the viewport's shape.
	 */
	void Update(const ViewportCamera::Input &input);

	/* @brief True while a camera drag is in progress (tools stand back). */
	bool IsNavigating();

	/* @brief The keymap's view keys over the viewport: numpad views, Home, and T F L P U Z / F A per keymap. */
	void HandleShortcuts(EditorContext *context);
}

// -------------------------------------------------------
