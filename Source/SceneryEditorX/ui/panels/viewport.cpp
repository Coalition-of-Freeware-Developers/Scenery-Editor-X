/**
* -------------------------------------------------------
* Scenery Editor X
* -------------------------------------------------------
* Copyright (c) 2025 Thomas Ray 
* Copyright (c) 2025 Coalition of Freeware Developers
* -------------------------------------------------------
* viewport.cpp
* -------------------------------------------------------
* Created: 4/4/2025
* -------------------------------------------------------
*/

#include <SceneryEditorX/UI/ui.h>
#include <imgui/imgui.h>

// -------------------------------------------------------

namespace UI
{
	void EditorViewport()
	{
		ImGui::Begin("Viewport");
        ImVec2 viewportSize = ImGui::GetContentRegionAvail();

		ImGui::End();
	}
} // namespace UI
