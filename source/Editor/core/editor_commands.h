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
 * editor_commands.h
 * -------------------------------------------------------
 * Created: 19/10/2026
 * -------------------------------------------------------
 */
#pragma once
#include <functional>
#include <string>
#include <string_view>
#include <vector>

// -------------------------------------------------------

namespace SceneryEditorX
{
	struct EditorContext;
}

namespace SceneryEditorX::EditorCommands
{
	/**
	 * The editor's named actions, drawn by id wherever they are offered (the right-click quad menu, the Space hotbox,
	 * the menu bar), so an action reads, looks and behaves the same in each, and the hotbox's "Recent Commands" can
	 * repeat any of them. Main thread only.
	 */
	struct Command
	{
		std::string id;							// "facade.subdivide", "tool.select"...
		std::string label;
		const char *icon = nullptr;
		std::string shortcut;					// Shown beside the label (the key itself is handled where it always was)
		std::string tooltip;
		std::function<bool(EditorContext *)> enabled;	// Null: always
		std::function<bool(EditorContext *)> checked;	// Null: not a toggle
		std::function<void(EditorContext *)> run;
	};

	/* @brief Adds a command, or replaces the one with the same id. */
	void Register(Command command);

	const Command *Find(std::string_view id);

	bool IsEnabled(std::string_view id, EditorContext *context);

	/* @brief Runs a command when it is enabled and remembers it as the most recent. */
	bool Run(std::string_view id, EditorContext *context);

	/**
	 * @brief The command as a menu item (icon, label, shortcut, check mark, greyed out when disabled, tooltip); runs it
	 * when clicked. Nothing is drawn for an unknown id.
	 * @param label Shown instead of the command's own label when not null.
	 * @return True when it ran.
	 */
	bool MenuItem(std::string_view id, EditorContext *context, const char *label = nullptr);

	/* @brief Ids of the commands run last, newest first (at most 10). */
	const std::vector<std::string> &GetRecent();
}

// -------------------------------------------------------
