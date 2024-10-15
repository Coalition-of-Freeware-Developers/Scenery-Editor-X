#pragma once
#ifndef UI_H
#define UI_H

#include <GLFW/glfw3.h>

namespace SceneryEditorX::UI {

	extern bool showExitModal;
    extern bool showAboutModal;

	void MainMenuBar();

    void LayerStack();
    void AssetBrowser();
    //void Properties();

	void ExitConfirmationModal(GLFWwindow *window);
	void AboutModal();
}

#endif 
