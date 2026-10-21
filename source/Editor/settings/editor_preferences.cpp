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
 * editor_preferences.cpp
 * -------------------------------------------------------
 * Created: 21/10/2026
 * -------------------------------------------------------
 */
#include "editor_preferences.h"
#include <algorithm>
#include <nlohmann/json.hpp>
#include <SceneryEditorX/filesystem/vfs.h>

// -------------------------------------------------------

namespace SceneryEditorX
{
	namespace
	{
		constexpr const char *PREFERENCES_FILE = "user:/editor_preferences.json";
		constexpr const char *LOG_TAG = "Preferences";

		constexpr const char *KEYMAP_IDS[] = { "blender", "3dsmax", "maya" };

		struct Store
		{
			bool loaded = false;
			EditorPreferences preferences;
		};

		Store &S()
		{
			static Store store;
			return store;
		}

		void Load(EditorPreferences &out)
		{
			out = EditorPreferences();
			std::string text;
			if (!VFS::Get().Read(PREFERENCES_FILE, text))
				return;	// First launch: defaults, keymapChosen stays false

			try
			{
				const nlohmann::json root = nlohmann::json::parse(text);
				if (const auto navigation = root.find("navigation"); navigation != root.end() && navigation->is_object())
				{
					const std::string keymap = navigation->value("keymap", std::string(KEYMAP_IDS[0]));
					for (size_t i = 0; i < std::size(KEYMAP_IDS); ++i)
					{
						if (keymap == KEYMAP_IDS[i])
							out.keymap = static_cast<NavigationKeymap>(i);
					}
					out.keymapChosen = navigation->value("chosen", false);
					out.invertOrbitX = navigation->value("invert_orbit_x", false);
					out.invertOrbitY = navigation->value("invert_orbit_y", false);
					out.invertLookY = navigation->value("invert_look_y", false);
					out.smoothViews = navigation->value("smooth_views", true);
					out.orbitSpeed = std::clamp(navigation->value("orbit_speed", 1.0f), 0.1f, 5.0f);
					out.zoomSpeed = std::clamp(navigation->value("zoom_speed", 1.0f), 0.1f, 5.0f);
				}
				if (const auto viewport = root.find("viewport2"); viewport != root.end() && viewport->is_object())
				{
					EditorPreferences::ViewportCameraState &camera = out.viewport2;
					camera.saved = true;
					if (const auto pivot = viewport->find("pivot"); pivot != viewport->end() && pivot->is_array() && pivot->size() == 3)
					{
						for (size_t i = 0; i < 3; ++i)
							camera.pivot[i] = (*pivot)[i].get<double>();
					}
					camera.distance = std::clamp(viewport->value("distance", camera.distance), 0.1, 1.0e7);
					camera.yaw = viewport->value("yaw", camera.yaw);
					camera.pitch = std::clamp(viewport->value("pitch", camera.pitch), -1.5707963267948966, 1.5707963267948966);
					camera.fovHorizontal = std::clamp(viewport->value("fov", camera.fovHorizontal), 0.05, 3.0);
					camera.orthographic = viewport->value("orthographic", camera.orthographic);
					camera.preset = std::clamp(viewport->value("preset", camera.preset), 0, 6);
				}
			}
			catch (const std::exception &e)
			{
				EDITOR_WARN_TAG(LOG_TAG, "{} could not be read ({}): using the defaults", PREFERENCES_FILE, e.what());
			}
		}
	}

	const char *GetNavigationKeymapName(const NavigationKeymap keymap)
	{
		switch (keymap)
		{
			case NavigationKeymap::Blender:	return "Blender";
			case NavigationKeymap::Max:		return "Autodesk 3ds Max";
			case NavigationKeymap::Maya:	return "Autodesk Maya";
			default:						return "Unknown";
		}
	}

	EditorPreferences &EditorPreferences::Get()
	{
		Store &store = S();
		if (!store.loaded)
		{
			Load(store.preferences);
			store.loaded = true;
		}
		return store.preferences;
	}

	void EditorPreferences::Reload()
	{
		Store &store = S();
		Load(store.preferences);
		store.loaded = true;
	}

	bool EditorPreferences::Save()
	{
		const EditorPreferences &p = Get();
		nlohmann::json navigation;
		navigation["keymap"] = KEYMAP_IDS[std::min(static_cast<size_t>(p.keymap), std::size(KEYMAP_IDS) - 1)];
		navigation["chosen"] = p.keymapChosen;
		navigation["invert_orbit_x"] = p.invertOrbitX;
		navigation["invert_orbit_y"] = p.invertOrbitY;
		navigation["invert_look_y"] = p.invertLookY;
		navigation["smooth_views"] = p.smoothViews;
		navigation["orbit_speed"] = p.orbitSpeed;
		navigation["zoom_speed"] = p.zoomSpeed;

		nlohmann::json root;
		root["navigation"] = navigation;
		if (p.viewport2.saved)
		{
			const EditorPreferences::ViewportCameraState &camera = p.viewport2;
			root["viewport2"] = { { "pivot", { camera.pivot[0], camera.pivot[1], camera.pivot[2] } }, { "distance", camera.distance }, { "yaw", camera.yaw },
				{ "pitch", camera.pitch }, { "fov", camera.fovHorizontal }, { "orthographic", camera.orthographic }, { "preset", camera.preset } };
		}

		std::string error;
		if (!VFS::Get().Write(PREFERENCES_FILE, root.dump(4) + "\n", &error))
		{
			EDITOR_WARN_TAG(LOG_TAG, "Could not save {}: {}", PREFERENCES_FILE, error);
			return false;
		}
		return true;
	}
}

// -------------------------------------------------------
