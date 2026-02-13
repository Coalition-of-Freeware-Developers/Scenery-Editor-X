/**
* -------------------------------------------------------
* Scenery Editor X
* -------------------------------------------------------
* Copyright (c) 2025 Thomas Ray
* Copyright (c) 2025 Coalition of Freeware Developers
* -------------------------------------------------------
* ui_layer.h
* -------------------------------------------------------
* Created: 24/12/2025
* -------------------------------------------------------
*/
#pragma once
//#include "SceneryEditorX/renderer/command_buffer.h"
#include <SceneryEditorX/core/layers/layer.h>

// -------------------------------------------------------

namespace SceneryEditorX::UI
{
	class UILayer : public Layer
	{
	public:
        UILayer();
        UILayer(const std::string &name);
        virtual ~UILayer();
        virtual void Begin();
        virtual void End();

	    virtual void OnAttach() override;
        virtual void OnDetach() override;
        virtual void OnUIRender() override;

        void SetDarkThemeColors();
        void SetDarkThemeV2Colors();

        void AllowInputEvents(bool allowEvents);

    private:
        //Ref<CommandBuffer> m_CommandBuffer;
        float m_Time = 0.0f;
	};

}

// -------------------------------------------------------
