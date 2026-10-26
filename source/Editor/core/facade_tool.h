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
 * facade_tool.h
 * -------------------------------------------------------
 * Created: 19/10/2026
 * -------------------------------------------------------
 */
#pragma once
#include "viewport_view.h"
#include <cstdint>
#include <optional>
#include <string>
#include <vector>
#include <SceneryEditorX/project/facade_edit.h>

// -------------------------------------------------------

namespace SceneryEditorX
{
	struct EditorContext;
}

namespace SceneryEditorX::FacadeTool
{
	/**
	 * Facades in the project: drawing new ones, picking and typing their walls, and the fixes and rebuilds the
	 * context menus, the PROPERTIES panel and the FACADES inspector offer. Every edit is one undo step.
	 *
	 * The tool (B) draws with the armed facade (picked from the toolbar, the X-PLANE LIBRARY panel, the hotbox's
	 * facade library or a selected facade):
	 *   Polygon    click the corners; click the first one, double-click or Enter to close (Line: to finish)
	 *   Rectangle  click the front wall's two ends, then how deep it goes (Shift: square)
	 *   Circle     click the center, then the radius (walls about "Circle wall" meters long)
	 *   Line       an open run: fences, walls, hangars built along their length, hangar doors
	 * Shift turns a new wall by 45 degree steps from the previous one, Ctrl inverts grid snapping, "Fit lengths"
	 * snaps a wall to the lengths the facade's segments fill without stretching (doors, hangars), Backspace removes
	 * the last corner, Esc cancels and a quick right-click finishes.
	 *
	 * With facades selected and nothing being drawn, the tool shows each wall's number, type and length; click a wall
	 * to select it (Shift adds), right-click for its menu, and the walls' types change from the toolbar, the menus or
	 * PROPERTIES.
	 */

	enum class DrawMode : uint8_t
	{
		Polygon,
		Rectangle,
		Circle,
		Line,
		Count
	};

	const char *GetDrawModeName(DrawMode mode);

	struct Settings
	{
		std::string resource;			// The armed facade (library path or pack-relative)
		DrawMode mode = DrawMode::Rectangle;
		float height = 0.0f;			// 0: the facade's own (its first height that is at least 3 m)
		int wallType = 0;				// Wall type new facades get on every wall
		bool fitLengths = false;
		float circleWall = 4.0f;		// Meters per wall of a circle
		float subdivideLength = 0.65f;	// Meters per piece when curved walls are subdivided (the sidewalk article's 0.5 - 0.8)
		float sidewalkOffset = 0.0f;	// Meters out from a building a sidewalk around it starts
		bool showLabels = true;			// Wall numbers, types and lengths on the selected facades
	};

	Settings &GetSettings();

	/* @brief Arms a facade for drawing (and remembers it as recent); switches the drawing mode to suit it. */
	void SetResource(const std::string &resource);

	/* @brief Facades armed lately, newest first. */
	const std::vector<std::string> &GetRecent();

	/* @brief The facade an item, a selection or the tool uses (null when it can't be read). */
	const XPlane::FacadeAsset *GetAsset(const std::string &resource);

	/**
	 * @brief Draws, picks walls and shows labels. Call inside the viewport window before its input item, while the
	 * facade tool is active.
	 * @return True when the tool takes the mouse (drawing, or a wall is under the cursor).
	 */
	bool OnViewportGui(EditorContext *context, const ViewportView &view, bool allowInput);

	/* @brief Labels of the selected facades' walls, drawn for the other tools too when enabled. */
	void DrawLabels(const ViewportView &view);

	bool IsDrawing();
	void Cancel();

	/* @brief Enter / Backspace while drawing, the mode keys (1 - 4). True when a key was used. */
	bool HandleShortcuts(EditorContext *context);

	/* @brief A quick right-click while drawing finishes the shape (true); otherwise the context menu opens. */
	bool OnRightClick(EditorContext *context);

	std::string GetHint();

	/* @brief The tool's options in the viewport toolbar: facade, mode, height, wall type, fit. */
	void DrawToolbar(EditorContext *context, float buttonSize);

	// -------------------------------------------------------
	// Facade items

	/* @brief The selected project items that are facades (the active one first). */
	std::vector<std::string> GetSelectedFacades();

	bool IsFacade(const std::string &itemId);

	/* @brief The walls of a facade item (local meters), empty when it isn't one. */
	std::vector<FacadeEdit::WallInfo> GetWalls(const std::string &itemId);

	/* @brief The wall under a screen position (the selected facades' walls), as item and wall index. */
	std::optional<std::pair<std::string, uint32_t>> FindWallAt(const ViewportView &view, const ImVec2 &screen);

	/* @brief The walls picked with the tool, on the active facade. */
	const std::vector<uint32_t> &GetSelectedWalls();
	void SelectWalls(const std::string &itemId, const std::vector<uint32_t> &walls);

	// Edits on facade items (one undo step each; false when nothing changed). Wall lists empty = every wall.
	bool SetWallTypes(const std::vector<std::string> &items, const std::vector<uint32_t> &walls, int type);
	bool SetHeight(const std::vector<std::string> &items, float height);
	bool SetPickWalls(const std::vector<std::string> &items, bool pick);
	bool SubdivideCurves(const std::vector<std::string> &items, float pieceLength);
	bool SquareCorners(const std::vector<std::string> &items);
	bool SetFrontWall(const std::string &itemId, uint32_t wall);
	bool Reverse(const std::vector<std::string> &items);
	bool Replace(const std::vector<std::string> &items, const std::string &resource);

	/* @brief A new facade (a sidewalk, a curb, a fence...) around each item's outline, offset out by `offset` meters. */
	std::vector<std::string> AddAround(EditorContext *context, const std::vector<std::string> &items, const std::string &resource, float offset);

	/* @brief Copies of project items a few meters south-east of the originals (any kind of item). */
	std::vector<std::string> Duplicate(EditorContext *context, const std::vector<std::string> &items);

	/* @brief A new facade item from a footprint in local meters. */
	std::string Create(EditorContext *context, const std::string &resource, const Splines::Chain &footprint, float height, const std::string &name = {});

	/* @brief Drops a library facade on the ground: a default-sized footprint there, selected and ready to edit. */
	std::string Drop(EditorContext *context, const std::string &resource, const float position[3]);

	// -------------------------------------------------------
	// Menus and panels

	/* @brief Menu items: each wall type of the facades (applied to the picked walls, else to every wall). */
	void DrawWallTypeMenu(const std::vector<std::string> &items);

	/* @brief Menu items: the heights the facade can be built at. */
	void DrawHeightMenu(const std::vector<std::string> &items);

	/* @brief Menu items: facades from the same library folder as the item's (other colors, sizes, materials). */
	void DrawReplaceMenu(const std::vector<std::string> &items);

	/**
	 * @brief Menu items: every facade in the X-Plane library, by folder (airport/ground/sidewalk_15cm...). Clicking
	 * one runs `pick` with its path (null: arms it for drawing and switches to the facade tool).
	 */
	void DrawLibraryMenu(void (*pick)(const std::string &resource) = nullptr);

	/* @brief The FACADE section of the PROPERTIES panel for the selected facades. */
	void DrawProperties(EditorContext *context);

	/* @brief Shows a facade in the FACADES inspector (the editor layer brings the panel up). */
	void Inspect(const std::string &resource);

	/* @brief The facade the inspector shows (empty: it follows the selection and the armed facade). */
	const std::string &GetInspected();

	/* @brief True once after Inspect(): the inspector shows the facade sent (it stops following the selection). */
	bool ConsumeInspectRequest();

	/* @brief True once after Inspect(): the editor layer opens the inspector panel and gives it focus. */
	bool ConsumeOpenRequest();

	/* @brief Registers the facade commands (EditorCommands). */
	void RegisterCommands();
}

// -------------------------------------------------------
