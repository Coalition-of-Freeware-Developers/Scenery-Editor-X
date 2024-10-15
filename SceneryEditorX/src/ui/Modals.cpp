// Modals.cpp

#include "imgui.h"
#include "UI.h"

// Define showExitModal and showAboutModal variables
bool SceneryEditorX::UI::showExitModal = false;
bool SceneryEditorX::UI::showAboutModal = false;

namespace SceneryEditorX::UI {
    void ExitConfirmationModal(GLFWwindow* window)
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
}
