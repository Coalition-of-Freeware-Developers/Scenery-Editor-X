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

#include <imgui/imgui.h>
#include <SceneryEditorX/ui/ui.h>

// -------------------------------------------------------

//bool UI::showSettingsPanel = false;

// -------------------------------------------------------

// Settings state variables
namespace UI
{
	// General settings
	static int languageIndex = 0;
	static bool autosave = true;
	static int autosaveInterval = 5; // minutes
	
	// Graphics settings
	static int msaaLevel = 2; // 0: Off, 1: 2x, 2: 4x, 3: 8x
	static bool vSync = true;
	static float fov = 60.0f;
	static int shadowQuality = 2; // 0: Low, 1: Medium, 2: High
	
	// Input settings
	static float mouseSensitivity = 1.0f;
	static bool invertY = false;
	static bool cameraMomentum = true;
	
	// Paths settings
    static char xplanePath[1024] = "C:/X-Plane 12"; // Temporary path
	static char defaultProjectPath[1024] = "C:/Users/Documents/Scenery Editor X/Projects"; // Temporary path
	
	// Editor settings
	static bool showGrid = true;
	static float gridSize = 1.0f;
	static bool autoSnap = true;

} // namespace UI

// -------------------------------------------------------

namespace UI
{
	void UIManager::SettingsPanel()
	{
	    if (!showSettingsPanel)
	        return;
	
	    ImGui::SetNextWindowSizeConstraints(ImVec2(600, 400), ImVec2(800, 600));
	    ImGui::SetNextWindowPos(ImVec2(ImGui::GetIO().DisplaySize.x * 0.5f, ImGui::GetIO().DisplaySize.y * 0.5f),
	                            ImGuiCond_FirstUseEver,
	                            ImVec2(0.5f, 0.5f));
	
	    ImGui::Begin("Settings", &showSettingsPanel, ImGuiWindowFlags_NoCollapse);
	
	    // Left side: Categories
	    static int selectedTab = 0;
	    const char *tabs[] = {"General", "Graphics", "Input", "Paths", "Editor"};
	
	    ImGui::BeginChild("SettingsTabs", ImVec2(150, 0), true);
	    for (int i = 0; i < IM_ARRAYSIZE(tabs); i++)
	    {
	        bool isSelected = (selectedTab == i);
	        if (ImGui::Selectable(tabs[i], isSelected))
	            selectedTab = i;
	
	        if (isSelected)
	            ImGui::SetItemDefaultFocus();
	    }
	    ImGui::EndChild();
	
	    ImGui::SameLine();
	
	    // Right side: Settings content
	    ImGui::BeginChild("SettingsContent", ImVec2(0, -35), true);
	
	    // Define all arrays outside the switch statement
	    const char *languages[] = {"English", "French", "German", "Spanish", "Chinese"};
	    const char *msaaOptions[] = {"Off", "2x MSAA", "4x MSAA", "8x MSAA"};
	    const char *shadowOptions[] = {"Low", "Medium", "High"};
	
	    switch (selectedTab)
	    {
	    case 0: // General
	        {
	            ImGui::Text("Application Settings");
	            ImGui::Separator();
	
	            ImGui::Combo("Language", &languageIndex, languages, IM_ARRAYSIZE(languages));
	
	            ImGui::Checkbox("Autosave", &autosave);
	            if (autosave)
	            {
	                ImGui::SameLine();
	                ImGui::SetNextItemWidth(100);
	                ImGui::SliderInt("Interval (minutes)", &autosaveInterval, 1, 30);
	            }
	
	            ImGui::Separator();
	            ImGui::TextWrapped("Note: Some settings require application restart to take effect.");
	        }
	        break;
	
	    case 1: // Graphics
	        {
	            ImGui::Text("Graphics Settings");
	            ImGui::Separator();
	
	            ImGui::Combo("Anti-aliasing", &msaaLevel, msaaOptions, IM_ARRAYSIZE(msaaOptions));
	
	            ImGui::Checkbox("Vertical Sync", &vSync);
	
	            ImGui::SetNextItemWidth(200);
	            ImGui::SliderFloat("Field of View", &fov, 45.0f, 90.0f, "%.1f°");
	
	            ImGui::Combo("Shadow Quality", &shadowQuality, shadowOptions, IM_ARRAYSIZE(shadowOptions));
	        }
	        break;
	
	    case 2: // Input
	        {
	            ImGui::Text("Input Settings");
	            ImGui::Separator();
	
	            ImGui::SetNextItemWidth(200);
	            ImGui::SliderFloat("Mouse Sensitivity", &mouseSensitivity, 0.1f, 3.0f, "%.2f");
	
	            ImGui::Checkbox("Invert Y Axis", &invertY);
	            ImGui::Checkbox("Camera Momentum", &cameraMomentum);
	
	            if (ImGui::Button("Reset to Defaults"))
	            {
	                mouseSensitivity = 1.0f;
	                invertY = false;
	                cameraMomentum = true;
	            }
	        }
	        break;
	
	    case 3: // Paths
	        {
	            ImGui::Text("Path Settings");
	            ImGui::Separator();
	
	            ImGui::InputText("X-Plane Path", xplanePath, IM_ARRAYSIZE(xplanePath));
	            ImGui::SameLine();
	            if (ImGui::Button("Browse##1"))
	            {
	                // File dialog would be implemented here
	            }
	
	            ImGui::InputText("Default Project Path", defaultProjectPath, IM_ARRAYSIZE(defaultProjectPath));
	            ImGui::SameLine();
	            if (ImGui::Button("Browse##2"))
	            {
	                // File dialog would be implemented here
	            }
	        }
	        break;
	
	    case 4: // Editor
	        {
	            ImGui::Text("Editor Settings");
	            ImGui::Separator();
	
	            ImGui::Checkbox("Show Grid", &showGrid);
	            if (showGrid)
	            {
	                ImGui::SameLine();
	                ImGui::SetNextItemWidth(100);
	                ImGui::InputFloat("Grid Size", &gridSize, 0.1f, 1.0f, "%.1f");
	            }
	
	            ImGui::Checkbox("Auto Snap", &autoSnap);
	
	            ImGui::Separator();
	            ImGui::Text("Color Scheme");
	            // Here you would add color pickers for UI elements
	        }
	        break;
	    }
	
	    ImGui::EndChild();
	
	    // Bottom buttons
	    ImGui::Separator();
	    ImGui::SetCursorPosX(ImGui::GetWindowWidth() - 220);
	    if (ImGui::Button("Apply", ImVec2(100.0f, 0.0f)))
	    {
	        // Save settings
	        // This would actually save to a config file
	    }
	    ImGui::SameLine();
	    if (ImGui::Button("Close", ImVec2(100.0f, 0.0f)))
	    {
	        showSettingsPanel = false;
	    }
	
	    ImGui::End();
	}


} // namespace UI

// -------------------------------------------------------
