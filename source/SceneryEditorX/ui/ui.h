/**
* -------------------------------------------------------
* Scenery Editor X
* -------------------------------------------------------
* Copyright (c) 2025 Thomas Ray 
* Copyright (c) 2025 Coalition of Freeware Developers
* -------------------------------------------------------
* ui.h
* -------------------------------------------------------
* Created: 25/3/2025
* -------------------------------------------------------
*/
#pragma once
#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_vulkan.h>

// -------------------------------------------------------

namespace UI
{


	class GUI
    {
    public:
        void init();
        void shutdown();
        void newFrame();
        void render();
        void resize(int width, int height);

        void setStyle();

	private:
        void setDarkThemeColors();
        void setLightThemeColors();

    };

    } // namespace UI

// -------------------------------------------------------
