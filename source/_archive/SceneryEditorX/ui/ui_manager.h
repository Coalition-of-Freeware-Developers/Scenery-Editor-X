#pragma once
#ifndef UI_H
#define UI_H

#include <GLFW/glfw3.h>

namespace SceneryEditorX::UI {

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
}

#endif 
