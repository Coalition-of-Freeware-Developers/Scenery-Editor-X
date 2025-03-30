/**
* -------------------------------------------------------
* Scenery Editor X
* -------------------------------------------------------
* Copyright (c) 2025 Thomas Ray 
* Copyright (c) 2025 Coalition of Freeware Developers
* -------------------------------------------------------
* gui_context.h
* -------------------------------------------------------
* Created: 28/3/2025
* -------------------------------------------------------
*/
#pragma once
#include <SceneryEditorX/core/layer.h>

// -------------------------------------------------------

namespace UI
{
	class GuiContext : Layer
	{
    public:
        virtual void Begin() = 0;
        virtual void End() = 0;

        void SetDarkThemeColors();
        void SetDarkThemeV2Colors();

        void AllowInputEvents(bool allowEvents);

        static GuiContext *Create();
	
	};

} // namespace UI
