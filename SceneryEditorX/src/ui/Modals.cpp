#include "imgui.h"
#include <imgui_impl_glfw.cpp>
#include <GLFW/glfw3.h>

#include "UI.h"


namespace SceneryEditorX::UI {
    /*
    void ExitConfirmationModal()
    {
        if (showExitModal)
        {
            ImGui::OpenPopup("ExitConfirmation");

            ImVec2 modalSize(460, 210);
            ImGui::SetNextWindowSize(modalSize);

            if (ImGui::BeginPopupModal("ExitConfirmation", &showExitModal, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoTitleBar))
            {
                ImGui::Text("Are you sure you want to exit?");
                ImGui::Checkbox("Don't show this message again", &showExitModal);
                ImGui::Separator();

                // Buttons
                if (ImGui::Button("Yes", ImVec2(100.0f, 0.0f)))
                {
                    glfwSetWindowShouldClose(window, true);
                    showExitModal = false; // Close the modal
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
                ImGui::Text("Jared Bruni");
            }
        }
    }
    */
}