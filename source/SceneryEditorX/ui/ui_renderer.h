/**
* -------------------------------------------------------
* Scenery Editor X
* -------------------------------------------------------
* Copyright (c) 2025 Thomas Ray 
* Copyright (c) 2025 Coalition of Freeware Developers
* -------------------------------------------------------
* ui_renderer.h
* -------------------------------------------------------
* Created: 31/7/2025
* -------------------------------------------------------
*/
#pragma once
#include "SceneryEditorX/core/modules/module.h"
#include "SceneryEditorX/renderer/vulkan/vk_cmd_buffers.h"

/// ---------------------------------------------------------

namespace SceneryEditorX
{
    class UIModule : public Module
	{
	public:
        UIModule() = default;
        explicit UIModule(const std::string &name);
        virtual ~UIModule() override;

        void Start();
        void End();

		virtual void OnAttach() override;
		virtual void OnDetach() override;
        virtual void OnUIRender() override;
	private:
		Ref<CommandBuffer> m_RenderCommandBuffer;
		float m_Time = 0.0f;
	};
}

/// ---------------------------------------------------------
