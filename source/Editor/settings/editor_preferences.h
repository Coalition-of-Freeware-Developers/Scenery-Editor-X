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
 * editor_preferences.h
 * -------------------------------------------------------
 * Created: 21/10/2026
 * -------------------------------------------------------
 */
#pragma once
#include <cstdint>

// -------------------------------------------------------

namespace SceneryEditorX
{
	/* @brief The viewport navigation keymaps, laid out like the 3D packages people come from. */
	enum class NavigationKeymap : uint8_t
	{
		Blender,	// MMB / Alt+LMB orbit, Shift+MMB / Shift+Alt+LMB pan, Ctrl+MMB / wheel zoom, numpad views
		Max,		// Alt+MMB orbit, MMB pan, Ctrl+Alt+MMB / wheel zoom, T F L P U views, Z frames
		Maya,		// Alt+LMB tumble, Alt+MMB track, Alt+RMB / wheel dolly, F frames the selection, A everything
		Count
	};

	const char *GetNavigationKeymapName(NavigationKeymap keymap);

	/**
	 * @struct EditorPreferences
	 * @brief The user's editor preferences, kept in user:/editor_preferences.json (the roaming settings folder).
	 * Edit them through Get() and call Save() after a change.
	 */
	struct EditorPreferences
	{
		// Viewport navigation
		NavigationKeymap keymap = NavigationKeymap::Blender;
		bool keymapChosen = false;		// The first-launch setup ran (or the user picked a keymap in the preferences)
		bool invertOrbitX = false;		// Orbit against the mouse horizontally (the editor's behaviour before the keymaps)
		bool invertOrbitY = false;
		bool invertLookY = false;		// Right-button look: mouse up looks down
		bool smoothViews = true;		// View changes (presets, framing) glide instead of jumping
		float orbitSpeed = 1.0f;		// Multiplies the mouse sensitivity of orbiting and looking
		float zoomSpeed = 1.0f;			// Multiplies the wheel / drag zoom steps

		// The second viewport's camera (ViewportCamera::Rig), remembered between sessions
		struct ViewportCameraState
		{
			bool saved = false;
			double pivot[3] = { 0.0, 0.0, 0.0 };
			double distance = 200.0;
			double yaw = 0.0;
			double pitch = -1.5707963267948966;
			double fovHorizontal = 1.0471975511965976;
			bool orthographic = true;
			int preset = 1;				// ViewportCamera::ViewPreset (Top)
		};
		ViewportCameraState viewport2;

		/* @brief The preferences, loaded on first use. */
		static EditorPreferences &Get();

		/* @brief Writes the preferences (atomically). False and a log line when that fails. */
		static bool Save();

		/* @brief Loads them again from disk (defaults for anything missing). */
		static void Reload();
	};
}

// -------------------------------------------------------
