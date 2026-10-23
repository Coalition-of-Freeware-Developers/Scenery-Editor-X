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
 * editor_xplane.h
 * -------------------------------------------------------
 * Created: 17/10/2026
 * -------------------------------------------------------
 */
#pragma once
#include <cstdint>
#include <filesystem>
#include <string>
#include <vector>
#include <SceneryEditorX/xplane/xp_obj8.h>

// -------------------------------------------------------

namespace SceneryEditorX
{
	struct EditorContext;
}

namespace SceneryEditorX::EditorXPlane
{
	/**
	 * Editor actions for the X-Plane integration, shared by the menu bar, the X-PLANE LIBRARY panel and the
	 * properties panel. All of them run on the main thread.
	 */

	/* @brief Drag-and-drop payload type for library assets (payload: the virtual path, NUL-terminated). */
	inline constexpr const char *ASSET_PAYLOAD = "SEDX_XP_LIBRARY_ASSET";

	/* @brief Asks for the X-Plane 12 folder with a folder picker and applies it. */
	void BrowseForInstall();

	/* @brief Asks for a library folder to track (one with a library.txt) and adds it. */
	void BrowseForTrackedFolder();

	/**
	 * @brief Asks for an OBJ8 file and places it in the scene at the origin.
	 * @param context Receives the new selection.
	 */
	void ImportObjFile(EditorContext *context);

	/**
	 * @brief Places a library object (resolved through the library index) in the scene.
	 * @param virtualPath "lib/..." path.
	 * @param context Receives the new selection (optional).
	 * @param position World position (x, y, z); nullptr places at the origin and frames the camera on the object.
	 * @return The root entity UUID (filled in once the object has loaded in the background), or 0 when the asset can't
	 * be placed.
	 */
	uint64_t PlaceLibraryAsset(const std::string &virtualPath, EditorContext *context, const float *position = nullptr);

	/**
	 * @brief Places an OBJ8 file in the scene.
	 * @param file The .obj file.
	 * @param virtualPath Library path to remember with the placement (may be empty).
	 * @param context Receives the new selection (optional).
	 * @param position World position (x, y, z); nullptr places at the origin and frames the camera on the object.
	 * @return The root entity UUID, or 0 on failure (the reason is logged). Returns at once: the object loads in the
	 * background and its meshes appear under the root when it is ready (a file that fails to load is logged and removed).
	 */
	uint64_t PlaceObjFile(const std::filesystem::path &file, const std::string &virtualPath, EditorContext *context, const float *position = nullptr);

	/**
	 * @brief Table of dataref controls (driver, value, range).
	 * @param id ImGui id for the table.
	 * @param only Restrict to these datarefs (an object's), or nullptr for every registered dataref.
	 */
	void DrawDatarefTable(const char *id, const std::vector<XPlane::Obj8DatarefUsage> *only);

	/* @brief Sim clock and environment controls (play / pause, speed, time of day, wind, lights). */
	void DrawSimulationControls();

	/**
	 * @brief Progress bar for objects loading in the background ("Loading 2 / 5: name.obj"), the queue in its tooltip.
	 * Draws nothing while no object is loading.
	 * @param width Bar width; 0 for the default item width, negative to fill the line minus that much.
	 * @return True when the bar was drawn.
	 */
	bool DrawObjectLoadProgress(float width = 0.0f);

}

// -------------------------------------------------------
