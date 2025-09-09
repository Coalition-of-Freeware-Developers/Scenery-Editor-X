/**
* -------------------------------------------------------
* Scenery Editor X
* -------------------------------------------------------
* Copyright (c) 2025 Thomas Ray 
* Copyright (c) 2025 Coalition of Freeware Developers
* -------------------------------------------------------
* ui_panel.h
* -------------------------------------------------------
* Created: 5/9/2025
* -------------------------------------------------------
*/
#pragma once
#include <functional>
#include <string>
#include <vector.h>
#include "imgui/imgui.h"

/// -------------------------------------------------------

struct ImGuiWindow;
class Editor;

using namespace SceneryEditorX;

/// -------------------------------------------------------

constexpr float DEFAULT_PANEL_PROPERTY = -1.0f;

/// -------------------------------------------------------

class UI_Panel
{
public:
    UI_Panel(Editor *editor);
    virtual ~UI_Panel() = default;

    void Tick();

    // called always
    virtual void OnTick() {}

    // called only when the widget is visible
    virtual void OnTickVisible() {}

    // called when the window becomes visible
    virtual void OnVisible() {}

    // called when the window becomes invisible
    virtual void OnInvisible() {}

    // called just before ImGui::Begin()
    virtual void OnPreBegin();

    // use this to push style variables. They will be automatically popped.
    template <typename T>
    void PushStyleVar(ImGuiStyleVar idx, T val) { ImGui::PushStyleVar(idx, val); varPushCount++; }

    /// -------------------------------------------------------

    Vec2 GetCenter()			const;
    float GetHeight()			const	{ return m_height; }
    ImGuiWindow *GetWindow()	const	{ return m_window;}
    std::string GetTitle()		const	{ return m_title; }
    bool &GetVisible()					{ return m_visible;}
    void SetVisible(bool isVisible)		{ m_visible = isVisible; }

    /// -------------------------------------------------------

protected:
    bool m_isWindow		= true;
    bool m_visible		= true;
    int m_flags			= ImGuiWindowFlags_NoCollapse;
    float m_height		= 0;
    float m_alpha		= -1.0f;

    // Vec2 initializations requires explicit constructor calls
    Vec2 initialSize = DEFAULT_PANEL_PROPERTY;
    Vec2 padding     = DEFAULT_PANEL_PROPERTY;
    Vec2 minSize     = DEFAULT_PANEL_PROPERTY;
    Vec2 maxSize     = FLT_MAX;

    std::string m_title = "Panel";
    Editor *m_Editor = nullptr;
    ImGuiWindow *m_window = nullptr;

private:
    uint8_t varPushCount = 0;
};

/// -------------------------------------------------------
