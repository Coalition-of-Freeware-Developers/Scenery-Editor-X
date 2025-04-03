/**
* -------------------------------------------------------
* Scenery Editor X
* -------------------------------------------------------
* Copyright (c) 2025 Thomas Ray 
* Copyright (c) 2025 Coalition of Freeware Developers
* -------------------------------------------------------
* modals.cpp
* -------------------------------------------------------
* Created: 29/3/2025
* -------------------------------------------------------
*/

#include <imgui/imgui.h>
#include <SceneryEditorX/UI/ui.h>

// -------------------------------------------------------

namespace SceneryEditorX
{
	
	// Define showExitModal and showAboutModal variables
	bool UI::showExitModal = false;
	bool UI::showAboutModal = false;
	bool UI::showCreateProjectModal = false;
	
	namespace UI
	{
		
		// Define projectName and projectLocation variables
		char projectName[128];
		char projectLocation[2048];
		
		void ExitConfirmationModal(GLFWwindow *window)
	    {
	        if (showExitModal)
	        {
	            ImGui::OpenPopup("ExitConfirmation");
	
	            ImVec2 modalSize(226, 94);
	            ImGui::SetNextWindowSize(modalSize);
	
	            if (ImGui::BeginPopupModal("ExitConfirmation", &showExitModal, ImGuiWindowFlags_NoMove | ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoTitleBar))
	            {
	                ImGui::Text("Are you sure you want to exit?");
	                ImGui::Checkbox("Don't show this message again", &showExitModal);
	                ImGui::Separator();
	
	                // Buttons
	                if (ImGui::Button("Yes", ImVec2(100.0f, 0.0f)))
	                {
	                    showExitModal = false; // Close the modal
	                    glfwSetWindowShouldClose(window, true);
	                }
	
	                ImGui::SameLine();
	
	                if (ImGui::Button("No", ImVec2(100.0f, 0.0f)))
	                {
	                    showExitModal = false; // Close the modal without exiting
	                }
	
	                ImGui::EndPopup();
	            }
	        }
	    }
	
	    void CreateProjectModal(GLFWwindow *window)
	    {
	        if (showCreateProjectModal)
	        {
	            ImGui::OpenPopup("NewProject");
	            ImVec2 modalSize(460, 210);
	            ImGui::SetNextWindowSize(modalSize);
	
	            if (ImGui::BeginPopupModal("NewProject", &showCreateProjectModal, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoTitleBar))
	            {
	                ImGui::Text("Create New Project");
	                ImGui::Separator();
	                ImGui::Text("Project Name:");
	                ImGui::InputText("##ProjectName", projectName, IM_ARRAYSIZE(projectName));
	                ImGui::Text("Project Location:");
	                ImGui::InputText("##ProjectLocation", projectLocation, IM_ARRAYSIZE(projectLocation));
	                ImGui::Separator();
	
	                // Buttons
	                if (ImGui::Button("Create", ImVec2(100.0f, 0.0f)))
	                {
	                    showCreateProjectModal = false; // Close the modal
	                }
	                ImGui::SameLine();
	                if (ImGui::Button("Cancel", ImVec2(100.0f, 0.0f)))
	                {
	                    showCreateProjectModal = false; // Close the modal without creating a new project
	                }
	
	                ImGui::EndPopup();
	            }
	        }
	    }
	
	    //TODO: Impliment the ability to close the modal popup by clicking outside the Modal
	    void AboutModal()
	    {
	        if (showAboutModal)
	        {
	            ImGui::OpenPopup("About");
	            ImVec2 modalSize(460, 210);
	            ImGui::SetNextWindowSize(modalSize);
	
	            if (ImGui::BeginPopupModal("About", &showAboutModal, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoTitleBar))
	            {
	                ImGui::Text("Scenery Editor X");
	                ImGui::Text("Version 1.0.0");
	                ImGui::Text("Developed by: ");
	                ImGui::Text("Some Random Guy");
	                ImGui::EndPopup();
	            }
	        }
	    }
	
	} // namespace UI
} // namespace SceneryEditorX

// -------------------------------------------------------
