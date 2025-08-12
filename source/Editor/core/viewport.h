/**
* -------------------------------------------------------
* Scenery Editor X
* -------------------------------------------------------
* Copyright (c) 2025 Thomas Ray
* Copyright (c) 2025 Coalition of Freeware Developers
* -------------------------------------------------------
* viewport.h
* -------------------------------------------------------
* Created: 1/8/2025
* -------------------------------------------------------
*/
#pragma once
#include <Editor/core/editor_camera.h>
#include <SceneryEditorX/core/events/event_system.h>
#include <SceneryEditorX/core/time/time.h>

/// -------------------------------------------------------

namespace SceneryEditorX
{

	/// Forward declarations
    class Scene;
    class Editor;
    class Renderer2D;
    class SceneRenderer;
    class KeyPressedEvent;
    class MouseButtonPressedEvent;

    /// -------------------------------------------------------

    class Viewport : public RefCounted
    {
    public:
		/// Default constructor to allow aggregate members to default-construct
		Viewport() : m_ViewportCamera(45.0f, 1280.0f, 720.0f, 0.1f, 1000.0f) {}
        Viewport(std::string viewportName, Editor *editor);

        const std::string &GetName() const;

        bool IsViewportVisible() const;
        bool IsMainViewport() const;

        Ref<SceneRenderer> GetRenderer() const;
        Ref<Renderer2D> GetRenderer2D() const;

		EditorCamera &GetViewportCamera();
        std::array<Vec2, 2> GetViewportBounds() const;

        void Init(const Ref<Scene> &scene);
        void SetIsMainViewport(bool isMain);
        void SetIsVisible(bool visible);

        void OnUpdate(DeltaTime dt);
        void OnRender2D();
        void OnUIRender();
        void OnEvent(Event &e);
        void ResetCamera();
        bool *GetIsVisibleMemory();
        //Viewport GetViewportSize(){ return m_ViewportBounds; }

    private:
        void UI_DrawGizmos();
        void UI_GizmosToolbar();
        void UI_CentralToolbar();
        void UI_ViewportSettings();
        void UI_HandleAssetDrop();

        bool OnKeyPressedEvent(KeyPressedEvent &e);
        bool OnMouseButtonPressed(MouseButtonPressedEvent &e);

        std::pair<float, float> GetMouseViewportSpace(bool primaryViewport) const;
        std::pair<Vec3, Vec3> CastRay(float mx, float my) const;

        Editor *m_Editor = nullptr;
        EditorCamera m_ViewportCamera;
        std::string m_ViewportName = "Viewport";
        Ref<SceneRenderer> m_ViewportRenderer;
        Ref<Renderer2D> m_ViewportRenderer2D;
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

}

/// -------------------------------------------------------
