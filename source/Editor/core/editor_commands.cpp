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
 * editor_commands.cpp
 * -------------------------------------------------------
 * Created: 19/10/2026
 * -------------------------------------------------------
 */
#include "editor_commands.h"
#include "editor_ui.h"
#include <algorithm>
#include <unordered_map>
#include <SceneryEditorX/renderer/ui/source/imgui/imgui.h>

// -------------------------------------------------------

namespace SceneryEditorX::EditorCommands
{
	namespace
	{
		constexpr size_t MAX_RECENT = 10;

		struct State
		{
			std::unordered_map<std::string, Command> commands;
			std::vector<std::string> recent;
		};

		State &S()
		{
			static State state;
			return state;
		}

		void Remember(const std::string &id)
		{
			std::vector<std::string> &recent = S().recent;
			std::erase(recent, id);
			recent.insert(recent.begin(), id);
			if (recent.size() > MAX_RECENT)
				recent.resize(MAX_RECENT);
		}
	}

	void Register(Command command)
	{
		std::string id = command.id;
		S().commands.insert_or_assign(std::move(id), std::move(command));
	}

	const Command *Find(const std::string_view id)
	{
		const auto &commands = S().commands;
		const auto it = commands.find(std::string(id));
		return it == commands.end() ? nullptr : &it->second;
	}

	bool IsEnabled(const std::string_view id, EditorContext *context)
	{
		const Command *command = Find(id);
		return command && command->run && (!command->enabled || command->enabled(context));
	}

	bool Run(const std::string_view id, EditorContext *context)
	{
		const Command *command = Find(id);
		if (!command || !IsEnabled(id, context))
			return false;
		// A copy: the command may register commands (and so move the one running)
		const std::function<void(EditorContext *)> run = command->run;
		Remember(command->id);
		run(context);
		return true;
	}

	bool MenuItem(const std::string_view id, EditorContext *context, const char *label)
	{
		const Command *command = Find(id);
		if (!command)
			return false;
		const std::string text = command->icon ? EditorUI::IconLabel(command->icon, label ? label : command->label.c_str()) : std::string(label ? label : command->label.c_str());
		const bool checked = command->checked && command->checked(context);
		const bool enabled = IsEnabled(id, context);
		const bool clicked = ImGui::MenuItem(text.c_str(), command->shortcut.empty() ? nullptr : command->shortcut.c_str(), checked, enabled);
		if (!command->tooltip.empty())
			EditorUI::ItemTooltip(command->tooltip.c_str());
		return clicked && Run(id, context);
	}

	const std::vector<std::string> &GetRecent()
	{
		return S().recent;
	}
}

// -------------------------------------------------------
