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
 * selection_manager.cpp
 * -------------------------------------------------------
 * Created: 11/8/2025
 * -------------------------------------------------------
 */
//#include <Editor/core/selection_manager.h>
//#include <SceneryEditorX/core/application/application.h>
//#include <SceneryEditorX/core/events/scene_events.h>
//#include <algorithm>

/// -------------------------------------------------------

/*
namespace SceneryEditorX
{

	void SelectionManager::Select(UUID contextID, UUID selectionID)
	{
		auto& contextSelections = s_Contexts[contextID];
		if (std::ranges::find(contextSelections, selectionID) != contextSelections.end())
			return;

		/// TODO: Maybe verify if the selectionID is already selected in another context?

		contextSelections.push_back(selectionID);
		Application::Get().DispatchEvent<SelectionChangedEvent>(contextID, selectionID, true);
	}

	bool SelectionManager::IsSelected(UUID selectionID)
	{
		for (const auto &contextSelections : s_Contexts | std::views::values)
		{
			if (std::ranges::find(contextSelections, selectionID) != contextSelections.end())
                return true;
        }

		return false;
	}

	bool SelectionManager::IsSelected(UUID contextID, UUID selectionID)
	{
		const auto& contextSelections = s_Contexts[contextID];
		return std::ranges::find(contextSelections, selectionID) != contextSelections.end();
	}

	bool SelectionManager::IsEntityOrAncestorSelected(const Entity entity)
	{
		Entity e = entity;
		while (e)
		{
			if (IsSelected(e.GetUUID()))
                return true;

            e = e.GetParent();
		}
		return false;
	}

	bool SelectionManager::IsEntityOrAncestorSelected(UUID contextID, const Entity entity)
	{
		Entity e = entity;
		while (e)
		{
			if (IsSelected(contextID, e.GetUUID()))
                return true;

            e = e.GetParent();
		}
		return false;
	}

	void SelectionManager::Deselect(UUID selectionID)
	{
		for (auto& [contextID, contextSelections] : s_Contexts)
		{
			auto it = std::ranges::find(contextSelections, selectionID);
			if (it == contextSelections.end())
				continue;

			Application::Get().DispatchEvent<SelectionChangedEvent>(contextID, selectionID, false);
			contextSelections.erase(it);
			break;
		}
	}

	void SelectionManager::Deselect(UUID contextID, UUID selectionID)
	{
		auto& contextSelections = s_Contexts[contextID];
		auto it = std::ranges::find(contextSelections, selectionID);
		if (it == contextSelections.end())
			return;

		contextSelections.erase(it);
	}

	void SelectionManager::DeselectAll()
	{
		for (auto& [ctxID, contextSelections] : s_Contexts)
		{
			for (const auto& selectionID : contextSelections)
				Application::Get().DispatchEvent<SelectionChangedEvent>(ctxID, selectionID, false);
			contextSelections.clear();
		}
	}

	void SelectionManager::DeselectAll(UUID contextID)
	{
		auto& contextSelections = s_Contexts[contextID];

		for (const auto& selectionID : contextSelections)
			Application::Get().DispatchEvent<SelectionChangedEvent>(contextID, selectionID, false);

		contextSelections.clear();
	}

	UUID SelectionManager::GetSelection(UUID contextID, size_t index)
	{
		auto& contextSelections = s_Contexts[contextID];
		SEDX_CORE_VERIFY(index >= 0 && index < contextSelections.size());
		return contextSelections[index];
	}

	size_t SelectionManager::GetSelectionCount(UUID contextID)
	{
		return s_Contexts[contextID].size();
	}

}
*/

/// -------------------------------------------------------
