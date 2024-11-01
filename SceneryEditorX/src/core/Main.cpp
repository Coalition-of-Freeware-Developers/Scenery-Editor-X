#include "../src/xpeditorpch.h"
#include "Application.hpp"

int main(int, char **)
{
    SceneryEditorX::Application app;


/*
##########################################################
			IMGUI UI PANEL RENDERING FUNCTIONS
##########################################################
*/

void RenderMainMenu() 
{
	MainMenuBar();
}

void RenderPanels()
{
    LayerStack();
    AssetBrowser();
    SettingsPanel();
}
}

    return EXIT_SUCCESS;
}
