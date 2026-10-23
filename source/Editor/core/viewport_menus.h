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
 * viewport_menus.h
 * -------------------------------------------------------
 * Created: 19/10/2026
 * -------------------------------------------------------
 */
#pragma once
#include "editor_selection.h"
#include "viewport_view.h"
#include <functional>
#include <optional>
#include <string>
#include <utility>
#include <vector>

// -------------------------------------------------------

namespace SceneryEditorX
{
	struct EditorContext;
}

namespace SceneryEditorX::ViewportMenus
{
	/**
	 * Context menus for the viewport, after 3ds Max's quad menu and Maya's hotbox:
	 *
	 *  - Quad menu: a quick right-click (no drag: dragging looks around) opens four panels around the cursor for what
	 *    was under it, which is selected first. The left panels hold that asset's own tools (a facade's wall types,
	 *    heights, replacements and rebuilds; a shape's node edits; creation on empty ground), top right is display,
	 *    bottom right the tools and transforms. A click outside or Esc closes it.
	 *  - Hotbox: hold Space over the viewport for every menu at the cursor: the menu bar's menus, recent commands, the
	 *    facade library and the selection's menus (the quad menu's panels). Letting go of Space closes it.
	 *
	 * Entries are EditorCommands, so they read and behave the same in both and repeat from "Recent Commands".
	 */

	using MenuDrawer = std::function<void()>;

	/* @brief The menu bar's menus (name, items), for the hotbox's top row. */
	void SetMainMenus(std::vector<std::pair<std::string, MenuDrawer>> menus);

	/**
	 * @brief Opens the quad menu at a screen position.
	 * @param target What was under the cursor (already selected), none for empty ground.
	 * @param ground The ground point under the cursor, for commands that place something there.
	 */
	void OpenQuadMenu(EditorContext *context, const ImVec2 &screen, const std::optional<EditorSelection::Target> &target, const std::optional<Point3> &ground);

	/* @brief Once per frame after the panels: the quad menu and the Space hotbox. */
	void Draw(EditorContext *context);

	/* @brief A quad menu or the hotbox is showing (the tools and their shortcuts keep still). */
	bool IsOpen();

	/* @brief The quad menu is up (or asked to open). */
	bool IsQuadMenuOpen();

	/* @brief The hotbox is showing. */
	bool IsHotboxOpen();

	/**
	 * @brief Shows the hotbox at a screen position without Space being held, until CloseMenus (self-tests and frame
	 * captures: nothing presses keys there).
	 */
	void PinHotbox(EditorContext *context, const ImVec2 &center);

	/* @brief Closes the quad menu and the hotbox. */
	void CloseMenus();

	/* @brief Registers the general commands both menus use (tools, edit, view). */
	void RegisterCommands();
}

// -------------------------------------------------------
