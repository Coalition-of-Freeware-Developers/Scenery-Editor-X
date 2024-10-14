#include "imgui.h"
#include "UI.h"

namespace SceneryEditorX::UI{
// Create and show main menu bar
	void MainMenuBar()
	{
		if (ImGui::BeginMainMenuBar()){
		if (ImGui::BeginMenu("File"))
		{
			if (ImGui::MenuItem("New", "Ctrl+N")) { /* Do something here */ }
			ImGui::Separator();
			if (ImGui::MenuItem("Open", "Ctrl+O"))
			{
	
			}
			if (ImGui::MenuItem("Save", "Ctrl+S")) { /* Do something here */ }
			ImGui::Separator();
			if (ImGui::MenuItem("Exit", "Alt+F4"))
			{
				showExitModal = true;
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
			ImGui::EndMenu();
		}
	
		if (ImGui::BeginMenu("View"))
		{
			static bool showHelp = true;
			ImGui::MenuItem("Show Help", nullptr, &showHelp);
			ImGui::Separator();
	
			// Add more menu items as needed
			ImGui::EndMenu();
		}
	
		if (ImGui::BeginMenu("Options"))
		{
			if (ImGui::MenuItem("Open", "Ctrl+O")) { /* Do something here */ }
			if (ImGui::MenuItem("Save", "Ctrl+S")) { /* Do something here */ }
			ImGui::Separator();
			ImGui::EndMenu();
		}
	
		if (ImGui::BeginMenu("Settings"))
		{
			if (ImGui::MenuItem("Open", "Ctrl+O")) { /* Do something here */ }
			if (ImGui::MenuItem("Save", "Ctrl+S")) { /* Do something here */ }
			ImGui::Separator();
			ImGui::EndMenu();
		}
	
		if (ImGui::BeginMenu("About"))
		{
			if (ImGui::MenuItem("Open", "Ctrl+O")) { /* Do something here */ }
			if (ImGui::MenuItem("Save", "Ctrl+S")) { /* Do something here */ }
			ImGui::Separator();
			ImGui::EndMenu();
		}
	
		ImGui::EndMainMenuBar();
		}
	}
}