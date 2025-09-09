/**
* -------------------------------------------------------
* Scenery Editor X
* -------------------------------------------------------
* Copyright (c) 2025 Thomas Ray
* Copyright (c) 2025 Coalition of Freeware Developers
* -------------------------------------------------------
* editor_camera.cpp
* -------------------------------------------------------
* Created: 1/8/2025
* -------------------------------------------------------
*/
//#include "editor_camera.h"
//#include <SceneryEditorX/core/input/input.h>
//#include <SceneryEditorX/ui/ui_manager.h>
//#include <algorithm>
//#include <cmath>
//#include <SceneryEditorX/core/input/input.h>
//#include <SceneryEditorX/ui/ui_manager.h>

/// -------------------------------------------------------

/*
namespace SceneryEditorX
{

    EditorCamera::EditorCamera(const float degFov, const float width, const float height, const float nearP, const float farP)
        : Camera(PerspectiveFov(ToRadians(degFov), width, height, farP, nearP),
                 PerspectiveFov(ToRadians(degFov), width, height, nearP, farP)),
          m_FocalPoint(0.0f), m_VerticalFOV(ToRadians(degFov)), m_NearClip(nearP), m_FarClip(farP)
	{
		Init();
	}

	void EditorCamera::Init()
	{
    Vec3 position = {-5.0f, 5.0f, 5.0f};
    m_Distance = Distance(position, m_FocalPoint);

    m_Yaw = 3.0f * PI / 4.0f;
    m_Pitch = PI / 4.0f;

        m_Position = CalculatePosition();
        const Quat orientation = GetOrientation();
        m_Direction = orientation.ToEulerDegrees();
        m_ViewMatrix = Mat4::Translate(m_Position) * orientation.ToMatrix();
        m_ViewMatrix = m_ViewMatrix.GetInverse();
	}

    static void DisableMouse()
    {
        Input::SetCursorMode(CursorMode::Locked);
        UI::SetInputEnabled(false);
    }

    static void EnableMouse()
    {
        Input::SetCursorMode(CursorMode::Normal);
        UI::SetInputEnabled(true);
    }

	void EditorCamera::Focus(const Vec3 &focusPoint)
	{
        m_FocalPoint = focusPoint;
        m_CameraMode = CameraMode::FLY;
        if (m_Distance > m_MinFocusDistance)
        {
            m_Distance -= m_Distance - m_MinFocusDistance;
            m_Position = m_FocalPoint - GetForwardDirection() * m_Distance;
        }
        m_Position = m_FocalPoint - GetForwardDirection() * m_Distance;
        UpdateCameraView();
	}

	void EditorCamera::OnUpdate(DeltaTime dt)
	{
        const Vec2 mouse{Input::GetMouseX(), Input::GetMouseY()};
        const Vec2 delta = (mouse - m_InitialMousePosition) * 0.002f;

        if (!m_IsActive)
        {
            if (!UI::IsInputEnabled())
                UI::SetInputEnabled(true);

            m_InitialMousePosition = mouse;
            return;
        }

        if (Input::IsMouseButtonDown(MouseButton::Right) && !Input::IsKeyDown(KeyCode::LeftAlt))
        {
            m_CameraMode = CameraMode::FLY;
            DisableMouse();

            const float yawSign = GetUpDirection().y < 0 ? -1.0f : 1.0f;
            const float speed = GetCameraSpeed();

            if (Input::IsKeyDown(KeyCode::Q))
                m_PositionDelta -= dt.GetMilliseconds() * speed * Vec3{0.f, yawSign, 0.f};
            if (Input::IsKeyDown(KeyCode::E))
                m_PositionDelta += dt.GetMilliseconds() * speed * Vec3{0.f, yawSign, 0.f};
            if (Input::IsKeyDown(KeyCode::S))
                m_PositionDelta -= dt.GetMilliseconds() * speed * m_Direction;
            if (Input::IsKeyDown(KeyCode::W))
                m_PositionDelta += dt.GetMilliseconds() * speed * m_Direction;
            if (Input::IsKeyDown(KeyCode::A))
                m_PositionDelta -= dt.GetMilliseconds() * speed * m_RightDirection;
            if (Input::IsKeyDown(KeyCode::D))
                m_PositionDelta += dt.GetMilliseconds() * speed * m_RightDirection;

            constexpr float maxRate{0.12f};
            m_YawDelta += Math::Clamp(yawSign * delta.x * RotationSpeed(), -maxRate, maxRate);
            m_PitchDelta += Math::Clamp(delta.y * RotationSpeed(), -maxRate, maxRate);

            m_RightDirection = Cross(m_Direction, Vec3{0.f, yawSign, 0.f});

            const Quat pitchQ = Quat::AngleAxisRadians(-m_PitchDelta, m_RightDirection);
            const Quat yawQ = Quat::AngleAxisRadians(-m_YawDelta, Vec3{0.f, yawSign, 0.f});
            m_Direction = Quat::Normalize(yawQ * pitchQ) * m_Direction;

            const float distance = Distance(m_FocalPoint, m_Position);
            m_FocalPoint = m_Position + GetForwardDirection() * distance;
            m_Distance = distance;
        }
        else if (Input::IsKeyDown(KeyCode::LeftAlt))
        {
            m_CameraMode = CameraMode::ORBIT;

            if (Input::IsMouseButtonDown(MouseButton::Middle))
            {
                DisableMouse();
                MousePan(delta);
            }
            else if (Input::IsMouseButtonDown(MouseButton::Left))
            {
                DisableMouse();
                MouseRotate(delta);
            }
            else if (Input::IsMouseButtonDown(MouseButton::Right))
            {
                DisableMouse();
                MouseZoom((delta.x + delta.y) * 0.1f);
            }
            else
                EnableMouse();
        }
        else
        {
            EnableMouse();
        }

        m_InitialMousePosition = mouse;
        m_Position += m_PositionDelta;
        m_Yaw += m_YawDelta;
        m_Pitch += m_PitchDelta;

        if (m_CameraMode == CameraMode::ORBIT)
            m_Position = CalculatePosition();

        UpdateCameraView();
	}

	void EditorCamera::OnEvent(Event &event)
    {
        EventDispatcher dispatcher(event);
        dispatcher.Dispatch<MouseScrolledEvent>([this](MouseScrolledEvent &e) { return OnMouseScroll(e); });
    }

	Vec3 EditorCamera::GetUpDirection() const
	{
        // Rotate world up by current orientation
        return GetOrientation() * Vec3(0.0f, 1.0f, 0.0f);
	}

	Vec3 EditorCamera::GetRightDirection() const
	{
        return GetOrientation() * Vec3(1.f, 0.f, 0.f);
	}

	Vec3 EditorCamera::GetForwardDirection() const
	{
        return GetOrientation() * Vec3(0.0f, 0.0f, -1.0f);
	}

    Quat EditorCamera::GetOrientation() const
	{
        return Quat::FromEulerDegrees(Vec3(-m_Pitch - m_PitchDelta, -m_Yaw - m_YawDelta, 0.0f));
	}

	float EditorCamera::GetCameraSpeed() const
	{
        float speed = m_NormalSpeed;
        if (Input::IsKeyDown(KeyCode::LeftControl))
            speed /= 2 - std::log(m_NormalSpeed);
        if (Input::IsKeyDown(KeyCode::LeftShift))
            speed *= 2 - std::log(m_NormalSpeed);

        return Math::Clamp(speed, MIN_SPEED, MAX_SPEED);
	}

	void EditorCamera::UpdateCameraView()
	{
        const float yawSign = GetUpDirection().y < 0 ? -1.0f : 1.0f;

        /// Extra step to handle the problem when the camera direction is the same as the up vector
        const float cosAngle = Dot(GetForwardDirection(), GetUpDirection());
        if (cosAngle * yawSign > 0.99f)
            m_PitchDelta = 0.f;

        const Vec3 lookAt = m_Position + GetForwardDirection();
        m_Direction = Normalize(lookAt - m_Position);
        m_Distance = Distance(m_Position, m_FocalPoint);
        m_ViewMatrix = Mat4::LookAt(m_Position, lookAt, Vec3{0.f, yawSign, 0.f});

        ///Damping for smooth camera
        m_YawDelta *= 0.6f;
        m_PitchDelta *= 0.6f;
        m_PositionDelta *= 0.8f;
	}

	bool EditorCamera::OnMouseScroll(MouseScrolledEvent &e)
	{
        if (Input::IsMouseButtonDown(MouseButton::Right))
        {
            m_NormalSpeed += e.GetYOffset() * 0.3f * m_NormalSpeed;
            m_NormalSpeed = std::clamp(m_NormalSpeed, MIN_SPEED, MAX_SPEED);
        }
        else
        {
            MouseZoom(e.GetYOffset() * 0.1f);
            UpdateCameraView();
        }

        return true;
	}

	void EditorCamera::MousePan(const Vec2 &delta)
	{
        auto [xSpeed, ySpeed] = PanSpeed();
        m_FocalPoint -= GetRightDirection() * delta.x * xSpeed * m_Distance;
        m_FocalPoint += GetUpDirection() * delta.y * ySpeed * m_Distance;
	}

	void EditorCamera::MouseRotate(const Vec2 &delta)
	{
        const float yawSign = GetUpDirection().y < 0.0f ? -1.0f : 1.0f;
        m_YawDelta += yawSign * delta.x * RotationSpeed();
        m_PitchDelta += delta.y * RotationSpeed();
	}

	void EditorCamera::MouseZoom(float delta)
	{
        m_Distance -= delta * ZoomSpeed();
        const Vec3 forwardDir = GetForwardDirection();
        m_Position = m_FocalPoint - forwardDir * m_Distance;
        if (m_Distance < 1.0f)
        {
            m_FocalPoint += forwardDir * m_Distance;
            m_Distance = 1.0f;
        }
        m_PositionDelta += delta * ZoomSpeed() * forwardDir;
	}

	Vec3 EditorCamera::CalculatePosition() const
	{
        // Orbit calculation: position = focalPoint - forward * distance
        return m_FocalPoint - GetForwardDirection() * m_Distance;
	}

	std::pair<float, float> EditorCamera::PanSpeed() const
	{
        const float x = Math::Min(float(m_ViewportRight - m_ViewportLeft) / 1000.0f, 2.4f); // max = 2.4f
        const float xFactor = 0.0366f * (x * x) - 0.1778f * x + 0.3021f;

        const float y = Math::Min(float(m_ViewportBottom - m_ViewportTop) / 1000.0f, 2.4f); // max = 2.4f
        const float yFactor = 0.0366f * (y * y) - 0.1778f * y + 0.3021f;

        return {xFactor, yFactor};
	}

	float EditorCamera::RotationSpeed() const
	{
        return 0.3f;
	}

	float EditorCamera::ZoomSpeed() const
	{
        float distance = m_Distance * 0.2f;
        distance = Math::Max(distance, 0.0f);
        float speed = distance * distance;
        speed = Math::Min(speed, 50.0f); // max speed = 50
        return speed;
	}
}
*/

/// -------------------------------------------------------
