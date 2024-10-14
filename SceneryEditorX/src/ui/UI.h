#pragma once
#ifndef UI_H
#define UI_H

#include <GLFW/glfw3.h>

namespace SceneryEditorX::UI {

	extern bool showExitModal;
    extern bool showAboutModal;

	void MainMenuBar();

    void LayerStack();

    //void Properties();

	void ExitConfirmationModal();
	void AboutModal();
}

#endif 
