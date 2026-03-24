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
#include <Editor/ui/ui_widget.h>
#include <SceneryEditorX/core/events/key_events.h>
#include <SceneryEditorX/core/events/mouse_events.h>
#include <SceneryEditorX/core/time/time.h>

// -------------------------------------------------------

class SceneViewport : public Widget
{
public:
	SceneViewport(const char* name, SceneryEditorX::EditorLayer *editor);

	void OnTickVisible() override;
	void OnVisible() override;
	void OnInvisible() override;
	void OnPreBegin() override;

	const std::string &GetName() const;
	bool IsViewportVisible() const;
	bool IsMainViewport() const;

	std::array<Vec2, 2> GetViewportBounds() const;

	void SetMainViewport(bool isMain);
	void SetVisible(bool visible);

	void Tick(SceneryEditorX::DeltaTime dt);
	void OnEvent(SceneryEditorX::Event &e);
	void ResetCamera();
	bool *GetIsVisibleMemory();
	Vec2 GetViewportSize();

private:
	void UI_DrawGizmos();
	void UI_GizmosToolbar();
	void UI_CentralToolbar();
	void UI_ViewportSettings();
	void UI_HandleAssetDrop();

	bool OnKeyPressedEvent(SceneryEditorX::KeyPressedEvent &e);
	bool OnMouseButtonPressed(SceneryEditorX::MouseButtonPressedEvent &e);

	std::pair<float, float> GetMouseViewportSpace(bool primaryViewport) const;
	std::pair<xMath::Vec3, xMath::Vec3> CastRay(float mx, float my) const;

	SceneryEditorX::EditorLayer *m_Editor = nullptr;
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

	enum class SelectionMode : uint8_t
	{
		Entity = 0,
		SubMesh = 1
	};
	SelectionMode m_SelectionMode = SelectionMode::Entity;

	enum class TransformationTarget : uint8_t
	{
		MedianPoint,
		IndividualOrigins
	};

	TransformationTarget m_MultiTransformTarget = TransformationTarget::MedianPoint;

}; 


// -------------------------------------------------------
