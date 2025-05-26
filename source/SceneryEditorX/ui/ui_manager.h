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
#include <GLFW/glfw3.h>
#include <SceneryEditorX/ui/ui.h>

// -------------------------------------------------------

namespace UI
{
    extern bool showViewport;
    extern bool showCreateProjectModal;
    extern bool showExitModal;
    extern bool showAboutModal;
    extern bool showSettingsPanel;

    // -------------------------------------------------------

    void SetDarkThemeColors();

	class UIManager
    {
    public:
        GLOBAL void SetupDockspace(bool* p_open = nullptr);
        GLOBAL void MainMenuBar();
        GLOBAL void LayerStack();
        GLOBAL void AssetBrowser();
        GLOBAL void SettingsPanel();
        //void Properties();
        GLOBAL void CreateProjectModal(GLFWwindow *window);
        GLOBAL void ExitConfirmationModal(GLFWwindow *window);
        GLOBAL void AboutModal();
        GLOBAL void ViewportWindow(glm::ivec2& viewportSize, bool& viewportHovered, VkImageView imageView);
    };

} // namespace UI

// -------------------------------------------------------

