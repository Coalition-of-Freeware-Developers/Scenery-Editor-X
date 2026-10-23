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
 * editor_panel.h
 * -------------------------------------------------------
 * Created: 10/04/2026
 * -------------------------------------------------------
 */
#pragma once
#include "source/imgui/imgui.h"
#include "source/imgui/imgui_internal.h"


#include <SceneryEditorX/core/events/event_system.h>
#include <SceneryEditorX/project/project.h>
#include <SceneryEditorX/scene/scene.h>

// -------------------------------------------------------

struct ImGuiWindow;
const float DEFAULT_PANEL_PROPERTY = -1.0f;

// -------------------------------------------------------

namespace SceneryEditorX::UI
{
	/**
	 * @class EditorPanel
	 * @brief Abstract base class for all editor panels.
	 */
	class EditorPanel : public RefCounted
	{
	public:
		/**
		 * @brief Destructor for the EditorPanel class.
		 */
		virtual ~EditorPanel() = default;

		/* @brief Called every frame, regardless of the panel's visibility. */
		virtual void OnTick() {}

		/* @brief Called only when the panel is visible. */
		virtual void OnTickVisible() {}

		/* @brief Called when the panel becomes visible. */
		virtual void OnVisible() {}

		/* @brief Called when the panel becomes invisible. */
		virtual void OnInvisible() {}

		/* @brief Called just before ImGui::Begin() is called for the panel's window. */
		virtual void OnPreBegin() {}

		/**
		 * @brief Renders the UI for the panel.
		 * @param isOpen A reference to a boolean indicating whether the panel is open.
		 */
		virtual void OnUIRender(bool& isOpen) = 0;

		/**
		 * @brief Handles an event.
		 * @param e Event to handle.
		 */
		virtual void OnEvent(Event& e) {}

		/**
		 * @brief Called when the active project changes.
		 * @param project The new active project.
		 */
		virtual void OnProjectChanged(const Ref<Project>& project) {}

		/**
		 * @brief Sets the scene context for the panel.
		 * @param context scene context to set for the panel
		 * @note This is called when the active scene changes, 
		 * allowing panels to update their context accordingly.
		 */
		virtual void SetSceneContext(const Ref<Scene>& context) {}

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
		
		/**
		 * @brief Retrieves the center position of the panel.
		 * @return The center position of the panel as a Vec2.
		 */
		[[nodiscard]] xMath::Vec2 GetCenter() const
		{
			SEDX_CORE_ASSERT(!m_Visible, "UI Panel is not visible");

			// If we have a valid ImGui window pointer, compute center from it.
			if (m_Window)
			{
				ImVec2 pos  = m_Window->Pos;
				ImVec2 size = m_Window->Size;
				ImVec2 center = ImVec2(pos.x + size.x * 0.5f, pos.y + size.y * 0.5f);
				return { center.x, center.y };
			}

			return { 0.0f, 0.0f };
		}

		/**
		 * @brief Retrieves the height of the panel.
		 * @return The height of the panel as a float.
		 */
		[[nodiscard]] float GetHeight() const { return m_Height; }

		/**
		 * @brief Retrieves the width of the panel.
		 * @return The width of the panel as a float.
		 */
		[[nodiscard]] float GetWidth() const { return m_Width; }

		/**
		 * @brief Retrieves the ImGui window associated with the panel.
		 * @return A pointer to the ImGui window.
		 */
		[[nodiscard]] ImGuiWindow *GetWindow() const { return m_Window; }

		/**
		 * @brief Retrieves the title of the panel.
		 * @return The title of the panel as a C-string.
		 */
		[[nodiscard]] const char *GetTitle() const { return m_Title; }

		/**
		 * @brief Retrieves the visibility status of the panel.
		 * @return A reference to a boolean indicating whether the panel is visible.
		 */
		bool &IsVisible() { return m_Visible; }

		/**
		 * @brief Sets the visibility status of the panel.
		 * @param isVisible A boolean indicating whether the panel should be visible.
		 */
		virtual void SetVisible(const bool isVisible) { m_Visible = isVisible; }

	protected:
		bool m_IsWindow = true;
		bool m_Visible = true;
		int m_Flags = ImGuiWindowFlags_NoCollapse;
		float m_Height = 0;
		float m_Width = 0;
		float m_Alpha = -1.0f;

		// Vec2 initializations requires explicit constructor calls
		Vec2 m_InitialSize = Vec2(DEFAULT_PANEL_PROPERTY, DEFAULT_PANEL_PROPERTY);
		Vec2 m_Padding = Vec2(DEFAULT_PANEL_PROPERTY, DEFAULT_PANEL_PROPERTY);
		Vec2 m_MinSize = Vec2(DEFAULT_PANEL_PROPERTY, DEFAULT_PANEL_PROPERTY);
		Vec2 m_MaxSize = Vec2(FLT_MAX, FLT_MAX);

		void* m_Editor = nullptr;
		ImGuiWindow *m_Window = nullptr;
		const char *m_Title = "Panel";

	private:
		friend class RenderContext;
		uint8_t varPushCount = 0;
	};

}

// -------------------------------------------------------
