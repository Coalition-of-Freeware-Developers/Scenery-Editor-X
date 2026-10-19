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
 * editor_history.cpp
 * -------------------------------------------------------
 * Created: 19/10/2026
 * -------------------------------------------------------
 */
#include "editor_history.h"
#include <algorithm>
#include <deque>
#include <vector>
#include <SceneryEditorX/logging/logging.hpp>

// -------------------------------------------------------

namespace SceneryEditorX::EditorHistory
{
	namespace
	{
		constexpr const char *LOG_TAG = "History";
		constexpr size_t MAX_STEPS = 256;
		constexpr size_t MAX_BYTES = 256ull * 1024 * 1024;	// Airport snapshots of big elements add up

		struct State
		{
			std::deque<Command> undo;
			std::vector<Command> redo;
			size_t bytes = 0;
			uint64_t revision = 0;
		};

		State &GetState()
		{
			static State state;
			return state;
		}

		const std::string &Empty()
		{
			static const std::string empty;
			return empty;
		}

		void Trim(State &state)
		{
			while (!state.undo.empty() && (state.undo.size() > MAX_STEPS || state.bytes > MAX_BYTES))
			{
				state.bytes -= std::min(state.bytes, state.undo.front().bytes);
				state.undo.pop_front();
			}
		}
	}

	void Push(Command command)
	{
		State &state = GetState();
		for (const Command &dropped : state.redo)
			state.bytes -= std::min(state.bytes, dropped.bytes);
		state.redo.clear();
		state.bytes += command.bytes;
		state.undo.push_back(std::move(command));
		Trim(state);
		++state.revision;
	}

	bool CanUndo()
	{
		return !GetState().undo.empty();
	}

	bool CanRedo()
	{
		return !GetState().redo.empty();
	}

	const std::string &GetUndoLabel()
	{
		const State &state = GetState();
		return state.undo.empty() ? Empty() : state.undo.back().label;
	}

	const std::string &GetRedoLabel()
	{
		const State &state = GetState();
		return state.redo.empty() ? Empty() : state.redo.back().label;
	}

	bool Undo()
	{
		State &state = GetState();
		if (state.undo.empty())
			return false;
		Command command = std::move(state.undo.back());
		state.undo.pop_back();
		++state.revision;
		if (!command.undo || !command.undo())
		{
			EDITOR_WARN_TAG(LOG_TAG, "Could not undo '{}': what it changed is no longer there", command.label);
			state.bytes -= std::min(state.bytes, command.bytes);
			return false;
		}
		EDITOR_INFO_TAG(LOG_TAG, "Undo: {}", command.label);
		state.redo.push_back(std::move(command));
		return true;
	}

	bool Redo()
	{
		State &state = GetState();
		if (state.redo.empty())
			return false;
		Command command = std::move(state.redo.back());
		state.redo.pop_back();
		++state.revision;
		if (!command.redo || !command.redo())
		{
			EDITOR_WARN_TAG(LOG_TAG, "Could not redo '{}': what it changed is no longer there", command.label);
			state.bytes -= std::min(state.bytes, command.bytes);
			return false;
		}
		EDITOR_INFO_TAG(LOG_TAG, "Redo: {}", command.label);
		state.undo.push_back(std::move(command));
		return true;
	}

	void Clear()
	{
		State &state = GetState();
		if (state.undo.empty() && state.redo.empty())
			return;
		state.undo.clear();
		state.redo.clear();
		state.bytes = 0;
		++state.revision;
	}

	uint64_t GetRevision()
	{
		return GetState().revision;
	}
}

// -------------------------------------------------------
