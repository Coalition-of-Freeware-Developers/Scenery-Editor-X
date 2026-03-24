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
 * ui_widget.cpp
 * -------------------------------------------------------
 * Created: 5/9/2025
 * -------------------------------------------------------
 */
#include "ui_widget.h"
#include "Editor/core/editor_layer.h"
#include <Editor/ui/source/imgui/imgui_internal.h>
#include <SceneryEditorX/core/window/monitor_data.h>
#include <SceneryEditorX/renderer/vulkan/viewport.h>

// -------------------------------------------------------

using namespace SceneryEditorX;

// -------------------------------------------------------

Widget::Widget(EditorLayer *editor)
{
	m_Editor = editor;
	m_Window = nullptr;
}

void Widget::Tick()
{
	OnTick();

	if (!m_IsWindow || m_Visible)
		return;

	{
		// Size initial
		m_InitialSize = (m_InitialSize.x == DEFAULT_PANEL_PROPERTY && m_InitialSize.y == DEFAULT_PANEL_PROPERTY) ? Vec2(MonitorData::GetWidth() * 0.5f,MonitorData::GetHeight() * 0.5f) : m_InitialSize;
		ImGui::SetNextWindowSize(ImVec2(m_InitialSize.x, m_InitialSize.y), ImGuiCond_FirstUseEver);

		// Size min max
		if (m_MinSize.x != DEFAULT_PANEL_PROPERTY || m_MinSize.y != DEFAULT_PANEL_PROPERTY || m_MaxSize.x != FLT_MAX || m_MaxSize.y != FLT_MAX)
		{
			ImGui::SetNextWindowSizeConstraints(ImVec2(m_MinSize.x, m_MinSize.y), ImVec2(m_MaxSize.x, m_MaxSize.y));
		}

		// Padding
		if (m_Padding.x != DEFAULT_PANEL_PROPERTY || m_Padding.y != DEFAULT_PANEL_PROPERTY)
		{
			ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(m_Padding.x, m_Padding.y));
			varPushCount++;
		}

		// Alpha
		if (m_Alpha != DEFAULT_PANEL_PROPERTY)
		{
			ImGui::PushStyleVar(ImGuiStyleVar_Alpha, m_Alpha);
			varPushCount++;
		}

		// Callback
		OnPreBegin();

		// Begin
		if (ImGui::Begin(m_Title, &m_Visible, m_Flags))
		{
			m_Window = ImGui::GetCurrentWindow();
			m_Height = ImGui::GetWindowHeight();
		}

		// Callbacks
		if (m_Window && m_Window->Appearing)
		{
			OnVisible();
		}
		else if (!m_Visible)
		{
			OnInvisible();
		}
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

void Widget::OnPreBegin()
{
	// Set the position to the viewport's center
	if (Viewport* viewport = m_Editor->GetWidget<Viewport>())
	{
		if (ImGuiWindow* window = GetWindow())
		{
			ImVec2 pos    = window->Pos;
			ImVec2 sze    = window->Size;
			ImVec2 center = ImVec2(pos.x + sze.x * 0.5f, pos.y + sze.y * 0.5f);
			ImVec2 pivot  = ImVec2(0.5f, 0.5f);

			ImGui::SetNextWindowPos(center, ImGuiCond_FirstUseEver, pivot);
		}
	}	
}

Vec2 Widget::GetCenter() const
{
	ImVec2 pos    = m_Window->Pos;
	ImVec2 sze    = m_Window->Size;
	ImVec2 center = ImVec2(pos.x + sze.x * 0.5f, pos.y + sze.y * 0.5f);

	// Convert ImVec2 to Vec2 explicitly
	return {center.x, center.y};
}

// -------------------------------------------------------
