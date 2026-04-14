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
 * scene_viewport.h
 * -------------------------------------------------------
 * Created: 01/08/2025
 * -------------------------------------------------------
 */
#pragma once
#include <SceneryEditorX/core/events/key_events.h>
#include <SceneryEditorX/core/events/mouse_events.h>
#include <SceneryEditorX/core/time/time.h>
#include <SceneryEditorX/renderer/ui/editor_panel.h>

// -------------------------------------------------------

namespace SceneryEditorX
{
	class UILayer;
	class EditorLayer;

	/**
	 * @class SceneViewport
	 * @brief Represents a viewport in the scene editor.
	 */
	class SceneViewport : public UI::EditorPanel
	{
	public:
		/**
		 * @brief 
		 * @param name 
		 * @param editor 
		 */
		SceneViewport(const char* name, EditorLayer *editor);

		/**
		 * @brief 
		 */
		void OnTickVisible() override;

		/**
		 * @brief 
		 */
		void OnVisible() override;

		/**
		 * @brief 
		 */
		void OnInvisible() override;

		/**
		 * @brief 
		 */
		void OnPreBegin() override;

		/**
		 * @brief 
		 * @return 
		 */
		const std::string &GetName() const;

		/**
		 * @brief 
		 * @return 
		 */
		bool IsViewportVisible() const;

		/**
		 * @brief 
		 * @return 
		 */
		bool IsMainViewport() const;

		/**
		 * @brief 
		 * @return 
		 */
		std::array<Vec2, 2> GetViewportBounds() const;
	
		/**
		 * @brief 
		 * @param isMain 
		 */
		void SetMainViewport(bool isMain);

		/**
		 * @brief 
		 * @param visible 
		 */
		void SetVisible(bool visible);

		/**
		 * @brief 
		 * @param dt 
		 */
		void Tick(DeltaTime dt);

		/**
		 * @brief 
		 * @param e 
		 */
		void OnEvent(Event &e);

		/**
		 * @brief 
		 */
		void ResetCamera();

		/**
		 * @brief 
		 * @return 
		 */
		bool *GetIsVisibleMemory();

		/**
		 * @brief 
		 * @return 
		 */
		Vec2 GetViewportSize();
	
	private:
		/**
		 * @brief 
		 */
		void UI_DrawGizmos();

		/**
		 * @brief 
		 */
		void UI_GizmosToolbar();

		/**
		 * @brief 
		 */
		void UI_CentralToolbar();

		/**
		 * @brief 
		 */
		void UI_ViewportSettings();

		/**
		 * @brief 
		 */
		void UI_HandleAssetDrop();

		/**
		 * @brief 
		 * @param e 
		 * @return 
		 */
		bool OnKeyPressedEvent(KeyPressedEvent &e);

		/**
		 * @brief 
		 * @param e 
		 * @return 
		 */
		bool OnMouseButtonPressed(MouseButtonPressedEvent &e);

		/**
		 * @brief 
		 * @param primaryViewport 
		 * @return 
		 */
		std::pair<float, float> GetMouseViewportSpace(bool primaryViewport) const;

		/**
		 * @brief 
		 * @param mx 
		 * @param my 
		 * @return 
		 */
		std::pair<xMath::Vec3, xMath::Vec3> CastRay(float mx, float my) const;
	
		UILayer *m_Editor = nullptr;
		const char* m_ViewportName = "Viewport";
		std::array<Vec2, 2> m_ViewportBounds = {};
	
		/**
		 * Main means the last active viewport.
		 * If you click on any viewport, and it is focus this viewport will be main.
		 */
		bool m_IsMainViewport = false;
		bool m_IsVisible = false;
		bool m_IsMouseOver = false;
		bool m_IsFocused = false;
		bool m_ShowIcons = true;
		bool m_ShowGizmos = true;
		bool m_ShowBoundingBoxSelectedMeshOnly = true;
		bool m_ShowBoundingBoxSubmeshes = false;
		bool m_ShowGizmosInPlayMode = false;
		bool m_DrawOnTopBoundingBoxes = true;
		bool m_ShowBoundingBoxes = false;
		float m_LineWidth = 2.0f;

		/**
		 * @enum SelectionMode
		 * @brief Defines the selection mode for the viewport.
		 */
		enum class SelectionMode : uint8_t
		{
			Entity = 0,
			SubMesh = 1
		};

		// TODO: this should probably be moved to editor preferences, and not be a per viewport setting
		SelectionMode m_SelectionMode = SelectionMode::Entity;

		/**
		 * @enum TransformationTarget
		 * @brief Defines the transformation target for the viewport.
		 */
		enum class TransformationTarget : uint8_t
		{
			MedianPoint,
			IndividualOrigins
		};
	
		// TODO: this should probably be moved to editor preferences, and not be a per viewport setting
		TransformationTarget m_MultiTransformTarget = TransformationTarget::MedianPoint;
	
	}; 
	
}

// -------------------------------------------------------
