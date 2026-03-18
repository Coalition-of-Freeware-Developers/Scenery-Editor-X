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
 * ui_widget.h
 * -------------------------------------------------------
 * Created: 5/9/2025
 * -------------------------------------------------------
 */
#pragma once
#include <cfloat>
#include <imgui.h>
#include <vector.h>

// -------------------------------------------------------

namespace SceneryEditorX
{
	class Editor;
}

struct ImGuiWindow;
const float DEFAULT_PANEL_PROPERTY = -1.0f;

// -------------------------------------------------------

/**
 * @class Widget
 * @brief Base class for all UI widgets in the editor.
 */
class Widget : public SceneryEditorX::RefCounted
{
public:
	Widget(SceneryEditorX::Editor *editor);
	virtual ~Widget() = default;

	void Tick();

	/* @brief Called every frame, regardless of the widget's visibility. */
	virtual void OnTick() {}

	/* @brief Called only when the widget is visible. */
	virtual void OnTickVisible() {}

	/* @brief Called when the window becomes visible. */
	virtual void OnVisible() {}

	/* @brief Called when the window becomes invisible. */
	virtual void OnInvisible() {}

	/* @brief Called just before ImGui::Begin() is called for the widget's window. */
	virtual void OnPreBegin();

	/**
	 * @brief Pushes a style variable onto the ImGui stack. The variable will be automatically popped when the widget is destroyed.
	 * @tparam T The type of the style variable.
	 * @param idx The index of the style variable.
	 * @param val The value of the style variable.
	 */
	template <typename T>
	void PushStyleVar(ImGuiStyleVar idx, T val)
	{
		ImGui::PushStyleVar(idx, val);
		varPushCount++;
	}

	// -------------------------------------------------------

	/**
	 * @brief Retrieves the center position of the widget.
	 * @return The center position of the widget as a Vec2.
	 */
	[[nodiscard]] Vec2 GetCenter() const;

	[[nodiscard]] float GetHeight() const { return m_Height; }

	[[nodiscard]] float GetWidth() const { return m_Width; }

	[[nodiscard]] ImGuiWindow *GetWindow() const { return m_Window; }

	[[nodiscard]] const char* GetTitle() const { return m_Title;}

	bool &GetVisible() { return m_Visible; }

	virtual void SetVisible(bool isVisible) { m_Visible = isVisible; }

protected:
	bool m_IsWindow		= true;
	bool m_Visible		= true;
	int m_Flags			= ImGuiWindowFlags_NoCollapse;
	float m_Height		= 0;
	float m_Width		= 0;
	float m_Alpha		= -1.0f;

	// Vec2 initializations requires explicit constructor calls
	Vec2 m_InitialSize	= Vec2(DEFAULT_PANEL_PROPERTY, DEFAULT_PANEL_PROPERTY);
	Vec2 m_Padding		= Vec2(DEFAULT_PANEL_PROPERTY, DEFAULT_PANEL_PROPERTY);
	Vec2 m_MinSize		= Vec2(DEFAULT_PANEL_PROPERTY, DEFAULT_PANEL_PROPERTY);
	Vec2 m_MaxSize		= Vec2(FLT_MAX, FLT_MAX);

	SceneryEditorX::Editor *m_Editor = nullptr;
	ImGuiWindow *m_Window	= nullptr;
	const char* m_Title		= "Panel";

private:
	friend class RenderContext;
	uint8_t varPushCount = 0;
};

// -------------------------------------------------------
