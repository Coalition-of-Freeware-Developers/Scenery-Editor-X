/**
* -------------------------------------------------------
* Scenery Editor X
* -------------------------------------------------------
* Copyright (c) 2025 Thomas Ray 
* Copyright (c) 2025 Coalition of Freeware Developers
* -------------------------------------------------------
* ui_context.h
* -------------------------------------------------------
* Created: 28/3/2025
* -------------------------------------------------------
*/
#pragma once
#include <imgui/imgui.h>
#include "SceneryEditorX/core/modules/module.h"
#include "SceneryEditorX/utils/static_states.h"

/// -------------------------------------------------------

namespace SceneryEditorX::UI
{
    class GUI;

	struct UIResource
	{
	    ImGuiContext *imguiContext = nullptr;
	    ImGuiIO *imguiIO = nullptr;
	    ImGuiStyle *imguiStyle = nullptr;
	    ImGuiPlatformIO *imguiPlatformIO = nullptr;
	    ImGuiViewport *imguiViewport = nullptr;
        ImGuiID imguiDockspaceID = 0;
	};

	class UIContext : public Module 
	{
    public:
        virtual void Begin() = 0;
        virtual void End() = 0;

        static Ref<UIContext> CreateRef();

	    virtual void SetGUI(GUI *guiInstance) = 0;

        static void SetDarkThemeColors();
        static void SetDarkThemeV2Colors();
        static void AllowInputEvents(bool allowEvents);
        static UIContext *Create();

	private:
        UIResource uiResource;
	
	};

}

/// -------------------------------------------------------
