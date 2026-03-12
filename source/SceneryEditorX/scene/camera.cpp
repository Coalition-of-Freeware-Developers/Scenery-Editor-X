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
 * camera.cpp
 * -------------------------------------------------------
 * Created: 07/03/2026
 * -------------------------------------------------------
 */
#include "camera.h"
#include "SceneryEditorX/core/input/input.h"
#include "SceneryEditorX/core/window/window.h"
#include "SceneryEditorX/renderer/renderer.h"
#include <algorithm>
#include <cmath>
#include <limits>
#include <string>

// -------------------------------------------------------

namespace SceneryEditorX
{
	Camera::Camera() { m_CameraFlag.SetDirty(); }

	CameraShaderData Camera::GetShaderData() const
	{
		// One-shot diagnostic: projection[2][3] must equal -1 (clip_w = -view_z).
		// A non-negative value means clip_w < 0 for in-front objects -> black screen.
		static bool isProjChecked = false;
		if (!isProjChecked)
		{
			const float p23 = m_Projection[2][3];
			if (p23 >= 0.0f)
			{
				SEDX_CORE_WARN_TAG("Camera", "projection[2][3] = {:.6f} (expected -1.0) -- wrong perspective divide; clip_w < 0 for in-front geometry -> black screen", p23);
			}
			else
			{
				SEDX_CORE_INFO_TAG("Camera", "projection[2][3] = {:.6f}  (OK)", p23);
			}
			isProjChecked = true;
		}
		CameraShaderData data;
		data.view = m_View;
		data.projection = m_Projection;
		data.viewProjection = m_ViewProjection;
		data.inverseViewProjection = m_ViewProjection.GetInverse();
		data.positionWorld = GetEyePosition();
		data.padding = 0.0f; // Ensure padding is zeroed for consistent shader data
		return data;
	}

	void Camera::Init()
	{
		m_CameraFlag.SetDirty();
		ComputeMatrices();
	}

	void Camera::Tick()
	{
		if (const auto &currentViewport = Renderer::GetViewport(); m_LastViewport != currentViewport)
		{
			m_LastViewport = currentViewport;
			m_CameraFlag.SetDirty();
		}
	
		ProcessCameraInput();
		ComputeMatrices();
	}

	void Camera::WorldToScreenCoordinates(const xMath::Vec3 &worldPos, xMath::Vec2 &screenPos) const
	{
		const auto &vp = Renderer::GetViewport();
		const Vec4 clipPos = m_ViewProjection * worldPos;
	
		if (std::abs(clipPos.w) < 1e-7f)
		{
			screenPos = xMath::Vec2(0.0f, 0.0f);
			return;
		}
	
		const float invW = 1.0f / clipPos.w;
		const float ndcX = clipPos.x * invW;
		const float ndcY = clipPos.y * invW;
	
		screenPos.x = (ndcX + 1.0f) * 0.5f * vp.width;
		screenPos.y = (1.0f - ndcY) * 0.5f * vp.height;
	}

	xMath::Rectangle Camera::WorldToScreenCoordinates(const xMath::BoundingBox &boundingBox) const
	{
		const auto &vp = Renderer::GetViewport();
	
		std::array<Vec3, 8> corners;
		boundingBox.GetCorners(&corners);
	
		float minX = std::numeric_limits<float>::max();
		float minY = std::numeric_limits<float>::max();
		float maxX = std::numeric_limits<float>::lowest();
		float maxY = std::numeric_limits<float>::lowest();
	
		for (const Vec3 &corner : corners)
		{
			const Vec4 clipPos = m_ViewProjection * corner;
			if (std::abs(clipPos.w) < 1e-7f)
				continue;
	
			const float invW = 1.0f / clipPos.w;
			const float sx = (clipPos.x * invW + 1.0f) * 0.5f * vp.width;
			const float sy = (1.0f - clipPos.y * invW) * 0.5f * vp.height;
	
			minX = Min(sx, minX);
			minY = Min(sy, minY);
			maxX = Max(sx, maxX);
			maxY = Max(sy, maxY);
		}
	
		if (minX > maxX)
			return Rectangle::Zero;
	
		return {minX, minY, maxX - minX, maxY - minY};
	}

	Vec3 Camera::ScreenToWorldCoordinates(const Vec2 &screenPos, const float z) const
	{
		const auto &vp = Renderer::GetViewport();
		const float ndcX = (screenPos.x / vp.width) * 2.0f - 1.0f;
		const float ndcY = -(screenPos.y / vp.height) * 2.0f + 1.0f;
	
		const Mat4 invVP = m_ViewProjection.GetInverse();
		const Vec4 worldPos = invVP * Vec4(ndcX, ndcY, z, 1.0f);
	
		if (std::abs(worldPos.w) < 1e-7f)
		{
			return Vec3(0.0f);
		}
	
		const float invW = 1.0f / worldPos.w;
		return {worldPos.x * invW, worldPos.y * invW, worldPos.z * invW};
	}

	float Camera::GetFovHorizontalDeg() const
	{
		return m_HorizontalFov_Rad * xMath::RAD_TO_DEG;
	}

	float Camera::GetFovVerticalRad() const
	{
		return 2.0f * atan(tan(m_HorizontalFov_Rad / 2.0f) * (Renderer::GetViewport().height / Renderer::GetViewport().width));
	}

	void Camera::SetFovHorizontalDeg(const float fov)
	{
		m_HorizontalFov_Rad = fov * xMath::DEG_TO_RAD;
		SetFlag();
	}

	float Camera::GetAspectRatio()
	{
		return Renderer::GetViewport().GetAspectRatio();
	}

	Vec3 Camera::GetEyePosition() const
	{
		return eye;
	}

	bool Camera::IsInViewFrustum(const BoundingBox &boundingBox) const
	{
		const Vec3 center = boundingBox.GetCenter();
		const Vec3 extents = boundingBox.GetExtents();
		return m_Frustum.IsVisible(center, extents, false);
	}

	bool Camera::IsInViewFrustum(Ref<Renderable> renderable) const
	{
		// TODO: This will need to be implemented properly once Renderable has a way to get its bounding box
		return true;
	}

	Flag Camera::SetFlag()
	{
		m_CameraFlag.SetDirty();
		return m_CameraFlag;
	}

	Mat4 Camera::GetViewMatrix() const
	{
		constexpr Vec3 worldUp = Vec3(0.0f, 1.0f, 0.0f);
		Vec3 forward;
	
		if (mode == ORBIT)
		{
			forward = xMath::Normalize(center - eye);
		}
		else
		{
			// Derive forward from pitch (rotation.x) and yaw (rotation.y)
			const float cosPitch = std::cos(rotation.x);
			forward = xMath::Normalize(
				Vec3(cosPitch * std::sin(rotation.y), std::sin(rotation.x), cosPitch * std::cos(rotation.y)));
		}

		const Vec3 target = eye + forward;
		return Mat4::LookAt(eye, target, worldUp);
	}

	Mat4 Camera::GetProjectionMatrix() const
	{
		return ComputeProjection(m_NearPlane, m_FarPlane);
	}

	void Camera::SetProjection(const CameraType projection)
	{
		m_ProjectionType = projection;
		SetFlag();
	}

	Mat4 Camera::UpdateViewMatrix() const
	{
		return GetViewMatrix();
	}

	Mat4 Camera::ComputeProjection(const float nearPlane, const float farPlane) const
	{
		if (m_ProjectionType == CameraType::Perspective)
		{
		   return Mat4::PerspectiveProjection(
				GetAspectRatio(),
				GetFovVerticalRad() * xMath::RAD_TO_DEG,
				nearPlane,
				farPlane);
		}
		if (m_ProjectionType == CameraType::Orthographic)
		{
			const auto &vp = Renderer::GetViewport();
			return Mat4::OrthographicProjection(-vp.width / 2.0f,
												vp.width / 2.0f,
												vp.height / 2.0f,
												-vp.height / 2.0f,
												nearPlane, farPlane);
		}

		return Mat4::Identity();
	}

	void Camera::ComputeMatrices()
	{
		if (!m_CameraFlag.IsDirty())
			return;
	
		// Orbit mode: derive eye position from spherical coordinates
		if (mode == ORBIT)
		{
			const float cosPitch = std::cos(rotation.x);
			const float sinPitch = std::sin(rotation.x);
			const float cosYaw = std::cos(rotation.y);
			const float sinYaw = std::sin(rotation.y);
			eye = center + Vec3(zoom * cosPitch * sinYaw, zoom * sinPitch, zoom * cosPitch * cosYaw);
		}
	
		m_View = GetViewMatrix();
	  m_Projection = ComputeProjection(m_NearPlane, m_FarPlane);
		m_ProjectionNonReverseZ = m_Projection;
		m_ViewProjection = m_Projection * m_View;
	 m_ViewProjectionNonReverseZ = m_ViewProjection;
		m_Frustum = xMath::Frustum(xMath::Matrix(m_View.Data()), xMath::Matrix(m_Projection.Data()));
		m_CameraFlag.Check();
	}

	void Camera::ProcessCameraInput()
	{
		constexpr float mouseSensitivity = 0.005f;
		constexpr float moveSpeed = 5.0f;
		constexpr float panSensitivity = 0.001f;
		constexpr float pitchLimit = PI * 0.49f;
	
		const auto [mouseX, mouseY] = Input::GetMousePosition();
		const Vec2 currentMouse(mouseX, mouseY);
		const Vec2 delta = currentMouse - m_LastMousePosition;
		m_LastMousePosition = currentMouse;

		const bool leftMouseDown  = Input::IsMouseButtonDown(MouseButton::Left);
		const bool rightMouseDown = Input::IsMouseButtonDown(MouseButton::Right);
		const bool wDown          = Input::IsKeyDown(KeyCode::W);
		const bool sDown          = Input::IsKeyDown(KeyCode::S);
		const bool aDown          = Input::IsKeyDown(KeyCode::A);
		const bool dDown          = Input::IsKeyDown(KeyCode::D);
		const bool qDown          = Input::IsKeyDown(KeyCode::Q);
		const bool eDown          = Input::IsKeyDown(KeyCode::E);
		const bool upDown         = Input::IsKeyDown(KeyCode::Up);
		const bool downDown       = Input::IsKeyDown(KeyCode::Down);
		const bool leftDown       = Input::IsKeyDown(KeyCode::Left);
		const bool rightDown      = Input::IsKeyDown(KeyCode::Right);
		const bool spaceDown      = Input::IsKeyDown(KeyCode::Space);
		const float scrollDelta   = Window::GetDeltaScroll();

		bool cameraChanged = false;
		std::string action;
	
		if (mode == ORBIT)
		{
			// Left-mouse drag: orbit (yaw / pitch around center)
			if (leftMouseDown)
			{
				rotation.y += delta.x * mouseSensitivity;
				rotation.x += delta.y * mouseSensitivity;
				rotation.x = std::clamp(rotation.x, -pitchLimit, pitchLimit);
				m_CameraFlag.SetDirty();
			   cameraChanged = true;
				action = "orbit_drag";
			}
	
			// Right-mouse drag: pan (translate center in screen plane)
		   if (rightMouseDown)
			{
				const float cosPitch = std::cos(rotation.x);
				const Vec3 forward = xMath::Normalize(
					Vec3(cosPitch * std::sin(rotation.y), std::sin(rotation.x), cosPitch * std::cos(rotation.y)));
				const Vec3 right = xMath::Normalize(xMath::Cross(forward, Vec3(0.0f, 1.0f, 0.0f)));
				const Vec3 up = xMath::Cross(right, forward);
	
				const float panScale = zoom * panSensitivity;
				center -= right * (delta.x * panScale);
				center += up * (delta.y * panScale);
				m_CameraFlag.SetDirty();
			   cameraChanged = true;
				action = "pan_drag";
			}
	
			// Keyboard zoom (W/S or Up/Down) while in orbit
		  if (wDown || upDown)
			{
				zoom = std::max(0.1f, zoom - moveSpeed * 0.016f);
				m_CameraFlag.SetDirty();
			   cameraChanged = true;
				action = "zoom_in";
			}
			if (sDown || downDown)
			{
				zoom += moveSpeed * 0.016f;
				m_CameraFlag.SetDirty();
			   cameraChanged = true;
				action = "zoom_out";
			}

			if (scrollDelta != 0.0f)
			{
				SEDX_CORE_TRACE_TAG("CameraInput", "MouseScroll detected (delta={:.3f}) in ORBIT mode (no scroll-zoom binding in Camera::ProcessCameraInput yet)", scrollDelta);
			}
		}
		else if (mode == FLY || mode == FREE)
		{
			// Right-mouse drag: look around (yaw / pitch)
		   if (rightMouseDown)
			{
				rotation.y += delta.x * mouseSensitivity;
				rotation.x += delta.y * mouseSensitivity;
				if (mode == FLY)
				{
					rotation.x = std::clamp(rotation.x, -pitchLimit, pitchLimit);
				}
				m_CameraFlag.SetDirty();
			   cameraChanged = true;
				action = "look_drag";
			}
	
			// WASD movement using forward/right vectors derived from rotation
			const float cosPitch = std::cos(rotation.x);
			const Vec3 forward = xMath::Normalize(
				Vec3(cosPitch * std::sin(rotation.y), std::sin(rotation.x), cosPitch * std::cos(rotation.y)));
			const Vec3 right = xMath::Normalize(xMath::Cross(forward, Vec3(0.0f, 1.0f, 0.0f)));
			constexpr float speed = moveSpeed * 0.016f; // ~1 frame at 60 fps
	
		  if (wDown || upDown)
			{
				eye += forward * speed;
				m_CameraFlag.SetDirty();
			   cameraChanged = true;
				action = "move_forward";
			}
			if (sDown || downDown)
			{
				eye -= forward * speed;
				m_CameraFlag.SetDirty();
			   cameraChanged = true;
				action = "move_backward";
			}
			if (aDown || leftDown)
			{
				eye -= right * speed;
				m_CameraFlag.SetDirty();
			   cameraChanged = true;
				action = "strafe_left";
			}
		   if (dDown || rightDown)
			{
				eye += right * speed;
				m_CameraFlag.SetDirty();
			   cameraChanged = true;
				action = "strafe_right";
			}
		   if (eDown || spaceDown)
			{
				eye.y += speed;
				m_CameraFlag.SetDirty();
			   cameraChanged = true;
				action = "move_up";
			}
		   if (qDown)
			{
				eye.y -= speed;
				m_CameraFlag.SetDirty();
			   cameraChanged = true;
				action = "move_down";
			}
		}

		if (cameraChanged || scrollDelta != 0.0f)
		{
			SEDX_CORE_TRACE_TAG(
				"CameraInput",
				"action={} mode={} mouse=({:.2f},{:.2f}) delta=({:.3f},{:.3f}) scroll={:.3f} eye=({:.3f},{:.3f},{:.3f}) center=({:.3f},{:.3f},{:.3f}) rot(pitch,yaw)=({:.3f},{:.3f}) zoom={:.3f} keys[W={} S={} A={} D={} Q={} E={} Up={} Down={} Left={} Right={} Space={}]",
				action,
				static_cast<uint32_t>(mode),
				currentMouse.x,
				currentMouse.y,
				delta.x,
				delta.y,
				scrollDelta,
				eye.x,
				eye.y,
				eye.z,
				center.x,
				center.y,
				center.z,
				rotation.x,
				rotation.y,
				zoom,
				wDown,
				sDown,
				aDown,
				dDown,
				qDown,
				eDown,
				upDown,
				downDown,
				leftDown,
				rightDown,
				spaceDown);
		}
	}
	void Camera::SetOrthographicProjection(float left, float right, float top, float bottom, float nearPlane, float farPlane) 
	{
		m_ProjectionMatrix = glm::mat4{1.0f};
		m_ProjectionMatrix[0][0] = 2.f / (right - left);
		m_ProjectionMatrix[1][1] = 2.f / (bottom - top);
		m_ProjectionMatrix[2][2] = 1.f / (farPlane - nearPlane);
		m_ProjectionMatrix[3][0] = -(right + left) / (right - left);
		m_ProjectionMatrix[3][1] = -(bottom + top) / (bottom - top);
		m_ProjectionMatrix[3][2] = -nearPlane / (farPlane - nearPlane);
	}

	void Camera::SetPerspectiveProjection(float fovy, float aspect, float nearPlane, float farPlane) 
	{
		SEDX_CORE_ASSERT(glm::abs(aspect - std::numeric_limits<float>::epsilon()) > 0.0f);
		const float tanHalfFovy = tan(fovy / 2.f);
		m_ProjectionMatrix = glm::mat4{0.0f};
		m_ProjectionMatrix[0][0] = 1.f / (aspect * tanHalfFovy);
		m_ProjectionMatrix[1][1] = 1.f / (tanHalfFovy);
		m_ProjectionMatrix[2][2] = farPlane / (farPlane - nearPlane);
		m_ProjectionMatrix[2][3] = 1.f;
		m_ProjectionMatrix[3][2] = -(farPlane * nearPlane) / (farPlane - nearPlane);
	}

	void Camera::SetViewDirection(glm::vec3 position, glm::vec3 direction, glm::vec3 up) 
	{
		const glm::vec3 w{glm::normalize(direction)};
		const glm::vec3 u{glm::normalize(glm::cross(w, up))};
		const glm::vec3 v{glm::cross(w, u)};
		
		m_ViewMatrix = glm::mat4{1.f};
		m_ViewMatrix[0][0] = u.x;
		m_ViewMatrix[1][0] = u.y;
		m_ViewMatrix[2][0] = u.z;
		m_ViewMatrix[0][1] = v.x;
		m_ViewMatrix[1][1] = v.y;
		m_ViewMatrix[2][1] = v.z;
		m_ViewMatrix[0][2] = w.x;
		m_ViewMatrix[1][2] = w.y;
		m_ViewMatrix[2][2] = w.z;
		m_ViewMatrix[3][0] = -glm::dot(u, position);
		m_ViewMatrix[3][1] = -glm::dot(v, position);
		m_ViewMatrix[3][2] = -glm::dot(w, position);
	}

	void Camera::SetViewTarget(glm::vec3 position, glm::vec3 target, glm::vec3 up) 
	{
	    SetViewDirection(position, target - position, up);
	}

	void Camera::SetViewYXZ(glm::vec3 position, glm::vec3 rotation) 
	{
		const float c3 = glm::cos(rotation.z);
		const float s3 = glm::sin(rotation.z);
		const float c2 = glm::cos(rotation.x);
		const float s2 = glm::sin(rotation.x);
		const float c1 = glm::cos(rotation.y);
		const float s1 = glm::sin(rotation.y);
		const glm::vec3 u{(c1 * c3 + s1 * s2 * s3), (c2 * s3), (c1 * s2 * s3 - c3 * s1)};
		const glm::vec3 v{(c3 * s1 * s2 - c1 * s3), (c2 * c3), (c1 * c3 * s2 + s1 * s3)};
		const glm::vec3 w{(c2 * s1), (-s2), (c1 * c2)};
		m_ViewMatrix = glm::mat4{1.f};
		m_ViewMatrix[0][0] = u.x;
		m_ViewMatrix[1][0] = u.y;
		m_ViewMatrix[2][0] = u.z;
		m_ViewMatrix[0][1] = v.x;
		m_ViewMatrix[1][1] = v.y;
		m_ViewMatrix[2][1] = v.z;
		m_ViewMatrix[0][2] = w.x;
		m_ViewMatrix[1][2] = w.y;
		m_ViewMatrix[2][2] = w.z;
		m_ViewMatrix[3][0] = -glm::dot(u, position);
		m_ViewMatrix[3][1] = -glm::dot(v, position);
		m_ViewMatrix[3][2] = -glm::dot(w, position);
	}

} // namespace SceneryEditorX

// -------------------------------------------------------
