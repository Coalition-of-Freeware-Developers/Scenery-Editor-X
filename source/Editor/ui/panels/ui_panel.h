/**
 * -------------------------------------------------------
 * Scenery Editor X
 * -------------------------------------------------------
 * Copyright (c) 2026 Thomas Ray 
 * Copyright (c) 2026 Coalition of Freeware Developers
 * -------------------------------------------------------
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
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

/*
class UI_Panel : public RefCounted
{
public:
    UI_Panel(Editor *editor);
    virtual ~UI_Panel() override = default;

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

    /*
    // Vec2 initializations requires explicit constructor calls
    Vec2 initialSize = DEFAULT_PANEL_PROPERTY;
    Vec2 padding     = DEFAULT_PANEL_PROPERTY;
    Vec2 minSize     = DEFAULT_PANEL_PROPERTY;
    Vec2 maxSize     = FLT_MAX;
    #1#

    std::string m_title = "Panel";
    Editor *m_Editor = nullptr;
    ImGuiWindow *m_window = nullptr;

private:
    uint8_t varPushCount = 0;
};
*/

/// -------------------------------------------------------
