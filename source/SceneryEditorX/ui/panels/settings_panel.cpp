/**
* -------------------------------------------------------
* Scenery Editor X
* -------------------------------------------------------
* Copyright (c) 2025 Thomas Ray 
* Copyright (c) 2025 Coalition of Freeware Developers
* -------------------------------------------------------
* settings_panel.cpp
* -------------------------------------------------------
* Created: 29/3/2025
* -------------------------------------------------------
*/

#include <SceneryEditorX/UI/ui.h>
#include <imgui/imgui.h>

// -------------------------------------------------------

namespace SceneryEditorX
{
	
	bool UI::showSettingsPanel = false;
	
	namespace UI
	{
	    void SettingsPanel()
	    {
	        ImGui::GetWindowHeight();
	        ImGui::SetNextWindowSizeConstraints(ImVec2(FLT_MIN, FLT_MIN), ImVec2(FLT_MAX, FLT_MAX));
	        ImGui::SetNextWindowPos(ImVec2(0, 0), ImGuiCond_Always, ImVec2(0.5f, 0.5f));
	
	        ImGui::Begin("Settings", nullptr, ImGuiWindowFlags_NoMove | ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoTitleBar);
	
	        if (ImGui::Button("Apply", ImVec2(100.0f, 0.0f)))
	        {
	
	        }
	        ImGui::SameLine();
	        if (ImGui::Button("Cancel", ImVec2(100.0f, 0.0f)))
	        {
	            showSettingsPanel = false;
	        }
	        ImGui::End();
	    }

	} // namespace UI
} // namespace SceneryEditorX

// -------------------------------------------------------
