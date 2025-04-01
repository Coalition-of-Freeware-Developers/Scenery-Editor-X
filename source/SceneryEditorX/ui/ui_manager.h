/**
* -------------------------------------------------------
* Scenery Editor X
* -------------------------------------------------------
* Copyright (c) 2025 Thomas Ray 
* Copyright (c) 2025 Coalition of Freeware Developers
* -------------------------------------------------------
* ui_manager.h
* -------------------------------------------------------
* Created: 29/3/2025
* -------------------------------------------------------
*/

#pragma once
#include <SceneryEditorX/ui/ui.h>
#include <GLFW/glfw3.h>

// -------------------------------------------------------

namespace UI
{
    extern bool showCreateProjectModal;
	extern bool showExitModal;
    extern bool showAboutModal;
    extern bool showSettingsPanel;

    void SetDarkThemeColors();

	void MainMenuBar();

    void LayerStack();
    void AssetBrowser();
    void SettingsPanel();
    //void Properties();

    void CreateProjectModal(GLFWwindow *window);
	void ExitConfirmationModal(GLFWwindow *window);
	void AboutModal();

} // namespace UI

