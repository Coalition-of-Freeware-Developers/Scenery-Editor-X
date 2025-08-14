/**
* -------------------------------------------------------
* Scenery Editor X
* -------------------------------------------------------
* Copyright (c) 2025 Thomas Ray
* Copyright (c) 2025 Coalition of Freeware Developers
* -------------------------------------------------------
* editor_camera.h
* -------------------------------------------------------
* Created: 1/8/2025
* -------------------------------------------------------
*/
#pragma once
#include <SceneryEditorX/core/events/event_system.h>
#include <SceneryEditorX/core/events/mouse_events.h>
#include <SceneryEditorX/core/time/time.h>
#include <SceneryEditorX/renderer/camera.h>
#include <SceneryEditorX/scene/camera.h>

/// -------------------------------------------------------

namespace SceneryEditorX
{

    class EditorCamera : public Camera
    {
    public:
        EditorCamera(const float degFov, const float width, const float height, const float nearP, const float farP);
        void Init();

		void Focus(const Vec3 &focusPoint);
        void OnUpdate(DeltaTime dt);
        void OnEvent(Event &event);

		[[nodiscard]] bool IsActive() const {return m_IsActive; }
        void SetActive(bool active) { m_IsActive = active; }

		[[nodiscard]] CameraMode GetCurrentMode() const { return m_CameraMode; }
		[[nodiscard]] const Vec3 &GetFocalPoint() const { return m_FocalPoint; }

        float GetDistance() const { return m_Distance; }
        void SetDistance(float distance) { m_Distance = distance; }

        void SetViewportBounds(uint32_t left, uint32_t top, uint32_t right, uint32_t bottom)
		{
			if (m_ViewportLeft == left && m_ViewportTop == top && m_ViewportRight == right && m_ViewportBottom == bottom)
				return;

			if ((right - left) != (m_ViewportRight - m_ViewportLeft) || (bottom - top) != (m_ViewportBottom - m_ViewportTop))
			{
				float width = static_cast<float>(right - left);
				float height = static_cast<float>(bottom - top);
				if (width > 0.0f && height > 0.0f)
                    SetPerspectiveProjectionMatrix(m_VerticalFOV, width, height, m_NearClip, m_FarClip);
            }

			m_ViewportLeft = left;
			m_ViewportTop = top;
			m_ViewportRight = right;
			m_ViewportBottom = bottom;
		}

        [[nodiscard]] const Mat4& GetViewMatrix() const { return m_ViewMatrix; }
        [[nodiscard]] Mat4 GetViewProjection() const { return GetProjectionMatrix() * m_ViewMatrix; }
		[[nodiscard]] Mat4 GetUnReversedViewProjection() const { return GetUnReversedProjectionMatrix() * m_ViewMatrix; }

		[[nodiscard]] Vec3 GetUpDirection() const;
		[[nodiscard]] Vec3 GetRightDirection() const;
		[[nodiscard]] Vec3 GetForwardDirection() const;

		[[nodiscard]] const Vec3& GetPosition() const { return m_Position; }
        [[nodiscard]] Quat GetOrientation() const;

		[[nodiscard]] float GetVerticalFOV() const { return m_VerticalFOV; }
		[[nodiscard]] float GetAspectRatio() const { return m_AspectRatio; }
		[[nodiscard]] float GetNearClip() const { return m_NearClip; }
		[[nodiscard]] float GetFarClip() const { return m_FarClip; }
		[[nodiscard]] float GetPitch() const { return m_Pitch; }
		[[nodiscard]] float GetYaw() const { return m_Yaw; }
		[[nodiscard]] float GetCameraSpeed() const;

    private:
        void UpdateCameraView();
        bool OnMouseScroll(MouseScrolledEvent &e);
        void MousePan(const Vec2 &delta);
        void MouseRotate(const Vec2 &delta);
        void MouseZoom(float delta);

        [[nodiscard]] Vec3 CalculatePosition() const;
        [[nodiscard]] std::pair<float, float> PanSpeed() const;
        [[nodiscard]] float RotationSpeed() const;
        [[nodiscard]] float ZoomSpeed() const;

		Mat4 m_ViewMatrix;
        Vec3 m_Position;
        Vec3 m_Direction;
        Vec3 m_FocalPoint;

        /// Perspective projection params
        float m_VerticalFOV;
        float m_AspectRatio;
        float m_NearClip;
        float m_FarClip;

        bool m_IsActive = false;
        bool m_Panning;
        bool m_Rotating;
        Vec2 m_InitialMousePosition{};
        Vec3 m_InitialFocalPoint;
        Vec3 m_InitialRotation;

        float m_Distance;
        float m_NormalSpeed{0.002f};

        float m_Pitch;
        float m_Yaw;
        float m_PitchDelta{};
        float m_YawDelta{};
        float m_MinFocusDistance{100.0f};
        Vec3 m_PositionDelta{};
        Vec3 m_RightDirection{};

        CameraMode m_CameraMode{ORBIT};

        uint32_t m_ViewportLeft = 0;
        uint32_t m_ViewportTop = 0;
        uint32_t m_ViewportRight = 1280;
        uint32_t m_ViewportBottom = 720;

        constexpr static float MIN_SPEED{0.0005f};
        constexpr static float MAX_SPEED{2.0f};

        friend class Editor;
        friend class Viewport;
    };

}

/// -------------------------------------------------------
