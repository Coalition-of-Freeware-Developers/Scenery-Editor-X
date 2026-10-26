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
 * editor_history.h
 * -------------------------------------------------------
 * Created: 19/10/2026
 * -------------------------------------------------------
 */
#pragma once
#include <cstddef>
#include <cstdint>
#include <functional>
#include <string>

// -------------------------------------------------------

namespace SceneryEditorX::EditorHistory
{
	/**
	 * Undo / redo for the viewport tools (moves, rotations, node edits, deletions). Each step holds closures that
	 * put the edited data back the way it was and forward again; they address what they edit by identity (entity
	 * UUID, airport element, project item id), and return false when that is gone, in which case the step is
	 * dropped with a note in the log. Main thread only.
	 */

	struct Command
	{
		std::string label;					// "Move 3 objects", "Edit nodes of Pavement 4"
		std::function<bool()> undo;
		std::function<bool()> redo;
		size_t bytes = 0;					// Rough size of what the closures hold (the history trims itself by it)
	};

	/* @brief Records an edit that has been applied. Clears what could be redone. */
	void Push(Command command);

	bool CanUndo();
	bool CanRedo();

	/* @brief Label of the step Undo / Redo would apply (empty when none). */
	const std::string &GetUndoLabel();
	const std::string &GetRedoLabel();

	bool Undo();
	bool Redo();

	/* @brief Forgets every step (the documents they refer to were closed or replaced). */
	void Clear();

	/* @brief Changes whenever a step is pushed, undone or redone. */
	uint64_t GetRevision();
}

// -------------------------------------------------------
