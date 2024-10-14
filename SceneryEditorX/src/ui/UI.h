#pragma once
#ifndef UI_H
#define UI_H

#include <GLFW/glfw3.h>

namespace SceneryEditorX::UI {

	inline bool showExitModal = false;
	inline bool showAboutModal = false;

	void MainMenuBar();

    void LayerStack();

    //void Properties();

	//void ExitConfirmationModal();
	//void AboutModal();
}

#endif 
