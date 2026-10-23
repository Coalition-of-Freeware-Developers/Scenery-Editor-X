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
 * modals.cpp
 * -------------------------------------------------------
 * Created: 29/3/2025
 * -------------------------------------------------------
 */
//#include <imgui/imgui.h>
//#include <SceneryEditorX/ui/ui.h>

// -------------------------------------------------------

// Define showExitModal and showAboutModal variables
//bool UI::showExitModal = false;
//bool UI::showAboutModal = false;
//bool UI::showCreateProjectModal = false;

/*
namespace UI
{

	// Define projectName and projectLocation variables
    static char projectName[128];
	static char projectLocation[2048];

	// ---------------------------------------------------------

    void UIManager::ExitConfirmationModal(GLFWwindow *window)
    {
        ImGui::SetNextWindowSize(ImVec2(400, 150), ImGuiCond_FirstUseEver);
        ImGui::SetNextWindowPos(ImVec2(ImGui::GetIO().DisplaySize.x * 0.5f, ImGui::GetIO().DisplaySize.y * 0.5f),
                                ImGuiCond_FirstUseEver, ImVec2(0.5f, 0.5f));
        
        if (ImGui::BeginPopupModal("Exit Confirmation", &showExitModal))
        {
            ImGui::Text("Are you sure you want to exit?");
            ImGui::Text("Any unsaved changes will be lost.");
            
            ImGui::Separator();
            
            ImGui::SetCursorPosX(ImGui::GetWindowWidth() - 180);
            if (ImGui::Button("Exit", ImVec2(85, 0)))
            {
                // Exit application
                glfwSetWindowShouldClose(window, GLFW_TRUE);
                showExitModal = false;
            }
            
            ImGui::SameLine();
            
            if (ImGui::Button("Cancel", ImVec2(85, 0)))
            {
                showExitModal = false;
            }
            
            ImGui::EndPopup();
        }
        else
        {
            ImGui::OpenPopup("Exit Confirmation");
        }
    }

    void UIManager::CreateProjectModal(GLFWwindow *window)
    {
        if (showCreateProjectModal)
        {
            ImGui::OpenPopup("NewProject");
            constexpr ImVec2 modalSize(460, 210);
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

    //TODO: Implement the ability to close the modal popup by clicking outside the Modal
    void UIManager::AboutModal()
    {
        ImGui::SetNextWindowSize(ImVec2(400, 200), ImGuiCond_FirstUseEver);
        ImGui::SetNextWindowPos(ImVec2(ImGui::GetIO().DisplaySize.x * 0.5f, ImGui::GetIO().DisplaySize.y * 0.5f),
                                ImGuiCond_FirstUseEver, ImVec2(0.5f, 0.5f));
        
        if (ImGui::BeginPopupModal("About Scenery Editor X", &showAboutModal))
        {
            ImGui::Text("Scenery Editor X");
            ImGui::Text("Version 1.0.0");
            ImGui::Separator();
            ImGui::Text(R"(© 2025 Thomas Ray)");
            ImGui::Text(R"(© 2025 Coalition of Freeware Developers)");
            ImGui::Separator();
            ImGui::Text("Powered by Vulkan and Dear ImGui");
            
            ImGui::Separator();
            
            ImGui::SetCursorPosX((ImGui::GetWindowWidth() - 85) * 0.5f);
            if (ImGui::Button("Close", ImVec2(85, 0)))
            {
                showAboutModal = false;
            }
            
            ImGui::EndPopup();
        }
        else
        {
            ImGui::OpenPopup("About Scenery Editor X");
        }
    }

} // namespace UI
*/

/// -------------------------------------------------------
