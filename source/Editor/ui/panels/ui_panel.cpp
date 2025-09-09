/**
* -------------------------------------------------------
* Scenery Editor X
* -------------------------------------------------------
* Copyright (c) 2025 Thomas Ray 
* Copyright (c) 2025 Coalition of Freeware Developers
* -------------------------------------------------------
* ui_panel.cpp
* -------------------------------------------------------
* Created: 5/9/2025
* -------------------------------------------------------
*/
#include "ui_panel.h"
#include <imgui_internal.h>
#include <SceneryEditorX/core/window/monitor_data.h>

/// -------------------------------------------------------

UI_Panel::UI_Panel(Editor *editor)
{
    m_Editor = editor;
    m_window = nullptr;
}

void UI_Panel::Tick()
{
    OnTick();

	if (!m_isWindow || m_visible)
        return;

    {
        // Size initial
        initialSize = initialSize == DEFAULT_PANEL_PROPERTY ? Vec2(MonitorData::GetWidth() * 0.5f,MonitorData::GetHeight() * 0.5f) : initialSize;
        ImGui::SetNextWindowSize(ImVec2(initialSize.x, initialSize.y), ImGuiCond_FirstUseEver);

        // Size min max
        if (minSize != DEFAULT_PANEL_PROPERTY || maxSize != FLT_MAX)
            ImGui::SetNextWindowSizeConstraints(ImVec2(minSize.x, minSize.y), ImVec2(maxSize.x, maxSize.y));

        // Padding
        if (padding != DEFAULT_PANEL_PROPERTY)
        {
            ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(padding.x, padding.y));
            varPushCount++;
        }

        // Alpha
        if (m_alpha != DEFAULT_PANEL_PROPERTY)
        {
            ImGui::PushStyleVar(ImGuiStyleVar_Alpha, m_alpha);
            varPushCount++;
        }

        // Callback
        OnPreBegin();

        // Begin
        if (ImGui::Begin(m_title.c_str(), &m_visible, m_flags))
        {
            m_window = ImGui::GetCurrentWindow();
            m_height = ImGui::GetWindowHeight();
        }

        // Callbacks
        if (m_window && m_window->Appearing)
            OnVisible();
        else if (!m_visible)
            OnInvisible();
    }

    OnTickVisible();

    // End
    {
        // End
        ImGui::End();

        // Pop style variables
        ImGui::PopStyleVar(varPushCount);
        varPushCount = 0;

    }
}


/// -------------------------------------------------------
