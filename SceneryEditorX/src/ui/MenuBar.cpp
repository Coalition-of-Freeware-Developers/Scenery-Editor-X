
#include "../src/IO/FileDialog.h"
#include "../xpeditorpch.h"
#include "UI.h"
#include <cstring>
#include <imgui.h>

// Function to open a file dialog and return the selected file path
//std::string FileDialogs()
//{
   // Implementation of file dialog (platform-specific)
   // For example, using a library like tinyfiledialogs or native API calls
   // This is a placeholder implementation
//}

namespace SceneryEditorX::UI {
    void MainMenuBar() {
        if (ImGui::BeginMainMenuBar()) {
            if (ImGui::BeginMenu("File"))
            {
                if (ImGui::MenuItem("New", "Ctrl+N"))
                {
                    showCreateProjectModal = true;
                }

                if (ImGui::MenuItem("Open", "Ctrl+O")) { /* Prompt the user to select a file from a file manager */ }
                if (ImGui::BeginMenu("Open Recent"))
                {
                    ImGui::MenuItem("KHVN | Tweed New Haven", nullptr);
                    ImGui::MenuItem("EGLL | London Heathrow", nullptr);
                    ImGui::MenuItem("EINN | Shannon Intl.", nullptr);
                    ImGui::MenuItem("EIDW | Dublin Intl.", nullptr);
                    ImGui::MenuItem("KMEM | Memphis Intl.", nullptr);
                    ImGui::EndMenu();
                }

                ImGui::Separator();

                if (ImGui::BeginMenu("Import", "Ctrl+I"))
                {
                    ImGui::MenuItem("Import Scenery Gateway", nullptr);
                    ImGui::MenuItem("Import WED Project", nullptr);
                    ImGui::Separator();
                    ImGui::MenuItem("Convert WED Project");
                    ImGui::EndMenu();
                }

                if (ImGui::BeginMenu("Export", "Ctrl+I"))
                {
                    ImGui::MenuItem("Export to Scenery Gateway", nullptr);
                    ImGui::MenuItem("Export to X-Plane", nullptr);
                    ImGui::Separator();

                    if (ImGui::BeginMenu("Export to File"))
                    {
                        ImGui::MenuItem("Export '.apt'", nullptr);
                        ImGui::MenuItem("Export '.dsf'", nullptr);
                        ImGui::EndMenu();
                    }
                    ImGui::MenuItem("Export Terrain", nullptr);
                    ImGui::EndMenu();
                }

                ImGui::Separator();

                if (ImGui::MenuItem("Save", "Ctrl+S")) { /* Do something here */ }
                if (ImGui::MenuItem("Save As..", "Ctrl+Shift+S")) { /* Do something here */ }

                ImGui::Separator();

                if (ImGui::MenuItem("Exit", "Alt+F4"))
                {
                    showExitModal = true;
                   //glfwSetWindowShouldClose(window, true);
                }

                ImGui::EndMenu();

            }

            if (ImGui::BeginMenu("Edit"))
            {
                if (ImGui::MenuItem("Undo", "Ctrl+Z")) { /* Do something here */ }
                if (ImGui::MenuItem("Redo", "Ctrl+Y")) { /* Do something here */ }
                ImGui::Separator();
                if (ImGui::MenuItem("Cut", "Ctrl+X")) { /* Do something here */ }
                if (ImGui::MenuItem("Copy", "Ctrl+C")) { /* Do something here */ }
                if (ImGui::MenuItem("Paste", "Ctrl+V")) { /* Do something here */ }
                if (ImGui::MenuItem("Duplicate", "Ctrl+D")) { /* Do something here */ }
                ImGui::EndMenu();
            }
            
            if (ImGui::BeginMenu("Window"))
            {
                if (ImGui::BeginMenu("Views"))
                {
                    static bool enabled = true;
                    ImGui::Text("Camera Types");
                    ImGui::MenuItem("Perspective View", nullptr, &enabled);
                    ImGui::MenuItem("Orthoscopic View", nullptr, &enabled);
                    ImGui::Separator();
                    ImGui::MenuItem("Center on Airport", nullptr);
                    ImGui::MenuItem("Tower View", nullptr, &enabled);
                    ImGui::MenuItem("Walk Around", nullptr, &enabled);
                    ImGui::MenuItem("Free Cam", nullptr, &enabled);
                    ImGui::EndMenu();
                }

                ImGui::Separator();
               
                if (ImGui::BeginMenu("Toolbars"))
                {
                    static bool enabled = true;
                    ImGui::MenuItem("Content Browser", nullptr, &enabled);
                    ImGui::MenuItem("Layer Stack", nullptr, &enabled);
                    ImGui::MenuItem("Errors", nullptr, &enabled);
                    ImGui::MenuItem("Properties", nullptr, &enabled);
                    ImGui::EndMenu();
                }

                ImGui::Separator();
                static bool enabled = true;
                if (ImGui::MenuItem("Show Grid", "G", &enabled)) { /* Do something here */ }
                if (ImGui::MenuItem("Show Axis", nullptr, &enabled)) { /* Do something here */ }
                ImGui::Separator();
                if (ImGui::MenuItem("Save UI", nullptr)) { /* Do something here */}
                if (ImGui::MenuItem("Reset UI", nullptr)) { /* Do something here */}
                ImGui::EndMenu();
            }
            
			if (ImGui::BeginMenu("Viewport"))
            {
                static bool enabled = true;
                ImGui::Text("Terrain");
                ImGui::MenuItem("Show Terrain", nullptr, &enabled);
                ImGui::MenuItem("Terrain Wireframe", "Ctrl+W", &enabled);
                ImGui::Separator();
                ImGui::Text("Models");
                ImGui::MenuItem("Scene Wireframe", nullptr, &enabled);
                ImGui::EndMenu();
            }

            if (ImGui::BeginMenu("Plugins"))
            {
                if (ImGui::MenuItem("Plugin Manager", nullptr))
                {
                    //showPluginManager = true;
                }
                ImGui::Separator();
                ImGui::MenuItem("Install Plugin", nullptr);
                if (ImGui::MenuItem("Plugin Config", nullptr))
                {
                    //showPluginConfig = true;
                }
                ImGui::EndMenu();
            }
            
			if (ImGui::BeginMenu("Options"))
            {
                static bool enabled = true;
                if (ImGui::MenuItem("Toggle Snaps", "S", &enabled)) { /* Do something here */ }
                if (ImGui::BeginMenu("Snaps"))
                {
                    ImGui::MenuItem("Toggle Grid Snap", nullptr, &enabled);
                    ImGui::MenuItem("Toggle Vertex Snap", nullptr, &enabled);
                    ImGui::MenuItem("Toggle Surface Snap", nullptr, &enabled);
                    ImGui::EndMenu();
                }
                ImGui::EndMenu();
			}
            
			if (ImGui::MenuItem("Settings"))
            {
                showSettingsPanel = true;
            }

            if (ImGui::BeginMenu("Help"))
            {
                static bool showHelp = true;
                ImGui::MenuItem("Show Help", nullptr, &showHelp);
                if (ImGui::MenuItem("Check for Updates", nullptr)) { /* Do something here */}
                (ImGui::MenuItem("Release Notes", nullptr));
                (ImGui::MenuItem("Bug Report", nullptr));
                if (ImGui::MenuItem("About", nullptr))
                {
                    showAboutModal = true;
                }
                ImGui::EndMenu();
			}
        ImGui::EndMainMenuBar();
        }
    }
}
