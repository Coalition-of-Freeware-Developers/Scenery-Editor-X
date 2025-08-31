/**
* -------------------------------------------------------
* Scenery Editor X
* -------------------------------------------------------
* Copyright (c) 2025 Thomas Ray 
* Copyright (c) 2025 Coalition of Freeware Developers
* -------------------------------------------------------
* SplashScreen.h
* -------------------------------------------------------
* Created: 26/8/2025
* -------------------------------------------------------
*/
#pragma once
#include "window.h"
#include <SceneryEditorX/renderer/texture.h>

/// -------------------------------------------------------

namespace SceneryEditorX
{

    class SplashScreen
    {
    public:
		SplashScreen();
		~SplashScreen() = default;
        void HideSplashScreen();
		void Show();
        void Close();

    private:
        RenderContext renderContext;
        GLFWwindow *splashWindow = nullptr;
        VkSurfaceKHR splashSurface = VK_NULL_HANDLE;
        Ref<SwapChain> splashSwap;
        Ref<Texture2D> splashTexture;

        /// -------------------------------------------------------

        bool InitializeSplashContext();
        bool CreateSplashTexture(unsigned char *imageData, int width, int height);
        void RenderSplashScreen();
        void CleanupSplashContext();
    };
}

/// -------------------------------------------------------
