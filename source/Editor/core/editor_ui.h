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
 * editor_ui.h
 * -------------------------------------------------------
 * Created: 17/10/2026
 * -------------------------------------------------------
 */
#pragma once
#include <string>
#include <SceneryEditorX/renderer/ui/source/imgui/imgui.h>

// -------------------------------------------------------

namespace SceneryEditorX::EditorUI
{
	/**
	 * @brief Begins a dockable editor panel and, when docked, underlines its tab in the accent color.
	 * Always pair with EndPanel(), whatever this returns.
	 * @param name Window name; use an uppercase label with a stable "###Id" suffix (e.g. "PROPERTIES###Properties").
	 * @param open Optional close-button flag.
	 * @param flags Extra ImGui window flags.
	 * @return True when the panel's contents are visible.
	 */
	bool BeginPanel(const char *name, bool *open = nullptr, ImGuiWindowFlags flags = 0);

	/* @brief Ends a panel started with BeginPanel(). */
	void EndPanel();

	/**
	 * @brief Collapsible uppercase section header ("GENERAL PROPERTIES", "TRANSFORM", ...).
	 * @return True when the section is expanded.
	 */
	bool SectionHeader(const char *label, bool defaultOpen = true);

	/**
	 * @brief Starts a two-column label / value grid. Follow each PropertyLabel() with exactly one widget.
	 * @return True when the grid is visible; only call EndPropertyGrid() in that case.
	 */
	bool BeginPropertyGrid(const char *id, float labelWidth = 120.0f);

	/* @brief Starts a grid row: draws the dim label and moves to a full-width value cell. */
	void PropertyLabel(const char *label);

	/* @brief Ends a grid started with BeginPropertyGrid(). */
	void EndPropertyGrid();

	/**
	 * @enum StripStyle
	 * @brief How a strip button shows its active state.
	 */
	enum class StripStyle
	{
		Filled,	// Accent background (exclusive tools / modes)
		Tinted	// Accent icon only (toggles, e.g. panel visibility)
	};

	/**
	 * @brief Square icon button for the tool strips.
	 * @return True when clicked.
	 */
	bool StripButton(const char *icon, const char *tooltip, bool active, float size, StripStyle style = StripStyle::Filled);

	/* @brief Draws dim text in the top-left of the remaining panel area (e.g. "No properties found"). */
	void EmptyState(const char *text);

	/**
	 * @brief Search box for an ImGuiTextFilter with a "Search" placeholder; rebuilds the filter when edited.
	 * @param id ImGui id of the input.
	 * @param filter The filter to edit.
	 * @param width Item width (-FLT_MIN fills the line).
	 */
	void SearchFilter(const char *id, ImGuiTextFilter &filter, float width = -FLT_MIN);

	/* @brief Shows a delayed tooltip for the last item. */
	void ItemTooltip(const char *text);

	/* @brief Joins an icon glyph and a label ("<icon>  Label") for buttons and menu items. */
	std::string IconLabel(const char *icon, const char *label);

}

// -------------------------------------------------------
