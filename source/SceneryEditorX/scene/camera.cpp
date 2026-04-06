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
#include "entity.h"
#include "scene.h"
#include "components/component_sets.h"
#include <algorithm>
#include <cmath>
#include <limits>
#include <string>
#include <SceneryEditorX/core/input/input.h>
#include <SceneryEditorX/core/window/window.h>
#include <SceneryEditorX/renderer/renderer.h>

// -------------------------------------------------------

namespace SceneryEditorX
{
	Camera::Camera() { m_CameraFlag.SetDirty(); }

	CameraShaderData Camera::GetShaderData() const
	{
		CameraShaderData data;
		
		// xMath matrices are Row-Major; Transpose them for the Column-Major Slang shader
		data.view = m_View.GetTranspose();
		data.projection = m_Projection.GetTranspose();
		
		// Pre-calculate View-Projection on CPU
		xMath::Mat4 vp = m_Projection * m_View; 
		data.viewProjection = vp.GetTranspose();
		
		data.inverseViewProjection = vp.GetInverse().GetTranspose();
		data.positionWorld = eye; // Your Vec3 camera position
		data.padding = 0.0f;
		
		return data;
	}

	void Camera::Init()
	{
		m_LastViewport = Renderer::GetViewport();
		m_CameraFlag.SetDirty();

		TransformComponent transform{};
		transform.translation = eye;

		if (mode == ORBIT)
		{
			const Vec3 toTarget = center - eye;
			if (Length2(toTarget) > 1e-6f)
			{
				const Vec3 forward = xMath::Normalize(toTarget);
				rotation.x = std::asin(forward.y);
				rotation.y = std::atan2(forward.x, forward.z);
				m_Zoom = Length(toTarget);
			}
		}

		transform.SetRotationEuler(rotation);

		CameraComponent cameraData{};
		cameraData.horizontalFov_Rad = m_HorizontalFov_Rad;
		cameraData.nearPlane = m_NearPlane;
		cameraData.farPlane = m_FarPlane;
		cameraData.projectionType =
			(m_ProjectionType == CameraType::Orthographic)
			? CameraComponent::ProjectionType::Orthographic
			: CameraComponent::ProjectionType::Perspective;

		ComputeMatrices(transform, cameraData, m_LastViewport);
	}

	void Camera::Tick()
	{
		if (const auto &currentViewport = Renderer::GetViewport(); m_LastViewport != currentViewport)
		{
			m_LastViewport = currentViewport;
			m_CameraFlag.SetDirty();
		}

		// Runtime/editor fallback path: if no ECS system pushes camera data every frame,
		// keep legacy input-driven camera behavior alive.
		ProcessCameraInput();

		if (mode == ORBIT)
		{
			const float cosPitch = std::cos(rotation.x);
			const float sinPitch = std::sin(rotation.x);
			const float cosYaw = std::cos(rotation.y);
			const float sinYaw = std::sin(rotation.y);
			eye = center + Vec3(m_Zoom * cosPitch * sinYaw, m_Zoom * sinPitch, m_Zoom * cosPitch * cosYaw);
		}

		TransformComponent transform{};
		transform.translation = eye;
		transform.SetRotationEuler(rotation);

		CameraComponent cameraData{};
		cameraData.horizontalFov_Rad = m_HorizontalFov_Rad;
		cameraData.nearPlane = m_NearPlane;
		cameraData.farPlane = m_FarPlane;
		cameraData.projectionType =
			(m_ProjectionType == CameraType::Orthographic)
			? CameraComponent::ProjectionType::Orthographic
			: CameraComponent::ProjectionType::Perspective;

		ComputeMatrices(transform, cameraData, m_LastViewport);
	}

	void Camera::Update(const TransformComponent& transform, const CameraComponent& cameraData, const Viewport& viewport)
	{
		m_LastViewport = viewport;
		m_CameraFlag.SetDirty();
		ComputeMatrices(transform, cameraData, viewport);
	}

	void Camera::ProcessInput(TransformComponent& transform, float deltaTime)
	{
		constexpr float mouseSensitivity = 0.005f;
		constexpr float moveSpeed = 5.0f;
		constexpr float pitchLimit = PI * 0.49f;

		if (deltaTime <= 0.0f)
		{
			return;
		}

		const auto [mouseX, mouseY] = Input::GetMousePosition();
		const Vec2 currentMouse(mouseX, mouseY);
		const Vec2 delta = currentMouse - m_LastMousePosition;
		m_LastMousePosition = currentMouse;

		Vec3 euler = transform.GetRotationEuler();
		Vec3 translation = transform.translation;

		const bool rightMouseDown = Input::IsMouseButtonDown(MouseButton::Right);
		if (rightMouseDown)
		{
			euler.y += delta.x * mouseSensitivity;
			euler.x += delta.y * mouseSensitivity;
			euler.x = std::clamp(euler.x, -pitchLimit, pitchLimit);
		}

		const float cosPitch = std::cos(euler.x);
		const Vec3 forward = xMath::Normalize(
			Vec3(cosPitch * std::sin(euler.y), std::sin(euler.x), cosPitch * std::cos(euler.y)));
		const Vec3 right = xMath::Normalize(xMath::Cross(forward, Vec3(0.0f, 1.0f, 0.0f)));

		const float speed = moveSpeed * deltaTime;
		if (Input::IsKeyDown(KeyCode::W) || Input::IsKeyDown(KeyCode::Up))
			translation += forward * speed;
		if (Input::IsKeyDown(KeyCode::S) || Input::IsKeyDown(KeyCode::Down))
			translation -= forward * speed;
		if (Input::IsKeyDown(KeyCode::A) || Input::IsKeyDown(KeyCode::Left))
			translation -= right * speed;
		if (Input::IsKeyDown(KeyCode::D) || Input::IsKeyDown(KeyCode::Right))
			translation += right * speed;
		if (Input::IsKeyDown(KeyCode::E) || Input::IsKeyDown(KeyCode::Space))
			translation.y += speed;
		if (Input::IsKeyDown(KeyCode::Q))
			translation.y -= speed;

		transform.translation = translation;
		transform.SetRotationEuler(euler);

		eye = transform.translation;
		rotation = euler;
		m_CameraFlag.SetDirty();
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
		{
			return Rectangle::ZERO;
		}
	
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

	/*
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
	*/

	/*
	Mat4 Camera::GetProjectionMatrix() const
	{
		return ComputeProjection(m_NearPlane, m_FarPlane);
	}
	*/

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


	Entity *Camera::GetSelectedEntity()
	{
		return nullptr;
	}

	std::vector<Entity *> &Camera::GetSelectedEntities()
	{
		return m_SelectedEntities;
	}

	bool Camera::IsSelected(Entity *entity) const
	{
		return false;
	}

	bool Camera::IsControlled()
	{
		return false;
	}

	void Camera::ClearSelection()
	{
	}

	void Camera::ComputeMatrices(const TransformComponent& transform, const CameraComponent& cameraData, const Viewport& viewport)
	{
		if (!m_CameraFlag.IsDirty())
			return;

		if (viewport.width <= 0.0f || viewport.height <= 0.0f)
			return;

		// 1. Calculate Projection Matrix
		float aspect = (float)viewport.width / (float)viewport.height;
		if (cameraData.projectionType == CameraComponent::ProjectionType::Perspective)
		{
		   m_Projection = xMath::Perspective(cameraData.horizontalFov_Rad, aspect, cameraData.nearPlane, cameraData.farPlane);
		}
		else
		{
			m_Projection = Mat4::OrthographicProjection(-viewport.width / 2.0f,
				viewport.width / 2.0f,
				viewport.height / 2.0f,
				-viewport.height / 2.0f,
				cameraData.nearPlane,
				cameraData.farPlane);
		}
		m_ProjectionMatrix = m_Projection;
		m_ProjectionNonReverseZ = m_Projection;

		// 2. Calculate View Matrix (Replaces SetViewYXZ logic)
		// We use the TransformComponent's translation and rotation directly
		xMath::Mat4 translation = xMath::Mat4::Translate(transform.translation);
		xMath::Mat4 worldRotation = transform.GetRotation().ToMatrix();

		// View Matrix is the inverse of the camera's World Transform
		// For a camera, View = (Rotation * Translation)^-1
		m_View = (translation * worldRotation).GetInverse();
		m_ViewMatrix = m_View;
		m_ViewProjection = m_Projection * m_View;
		m_ViewProjectionNonReverseZ = m_ViewProjection;

		eye = transform.translation;
		rotation = transform.GetRotationEuler();
		m_HorizontalFov_Rad = cameraData.horizontalFov_Rad;
		m_NearPlane = cameraData.nearPlane;
		m_FarPlane = cameraData.farPlane;
		m_ProjectionType = cameraData.projectionType == CameraComponent::ProjectionType::Orthographic
			? CameraType::Orthographic
			: CameraType::Perspective;

		m_Frustum = xMath::Frustum(xMath::Matrix(m_View.Data()), xMath::Matrix(m_Projection.Data()));
		m_CameraFlag.Check();
	
		// -------------------------------------------------------

		/* Uncomment when you want to restore the original ORBIT mode behavior, but it will override the TransformComponent's position/rotation
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
		*/

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
	
				const float panScale = m_Zoom * panSensitivity;
				center -= right * (delta.x * panScale);
				center += up * (delta.y * panScale);
				m_CameraFlag.SetDirty();
			   cameraChanged = true;
				action = "pan_drag";
			}
	
			// Keyboard zoom (W/S or Up/Down) while in orbit
			if (wDown || upDown)
			{
				m_Zoom = std::max(0.1f, m_Zoom - moveSpeed * 0.016f);
				m_CameraFlag.SetDirty();
			   cameraChanged = true;
				action = "zoom_in";
			}
			if (sDown || downDown)
			{
				m_Zoom += moveSpeed * 0.016f;
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
				m_Zoom,
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
		m_Projection = xMath::Mat4{1.0f};
		m_Projection[0][0] = 2.f / (right - left);
		m_Projection[1][1] = 2.f / (bottom - top);
		m_Projection[2][2] = 1.f / (farPlane - nearPlane);
		m_Projection[3][0] = -(right + left) / (right - left);
		m_Projection[3][1] = -(bottom + top) / (bottom - top);
		m_Projection[3][2] = -nearPlane / (farPlane - nearPlane);
		m_ProjectionMatrix = m_Projection;
		m_ViewProjection = m_Projection * m_View;
	}

	void Camera::SetPerspectiveProjection(float fov_rad, float aspect, float near_z, float far_z) 
	{
		float tan_half_fov = tanf(fov_rad / 2.0f);
		m_Projection = xMath::Mat4(0.0f);
		
		// X scale
		m_Projection[0][0] = 1.0f / (aspect * tan_half_fov);
		
		// Y scale: Negated for Vulkan's Y-down clip space
		m_Projection[1][1] = -1.0f / tan_half_fov; 
		
		// Z scale: Mapped to [0, 1] for Vulkan
		m_Projection[2][2] = far_z / (far_z - near_z);
		m_Projection[2][3] = 1.0f;
		
		// Z translation
		m_Projection[3][2] = -(far_z * near_z) / (far_z - near_z);
		m_ProjectionMatrix = m_Projection;
		m_ViewProjection = m_Projection * m_View;
	}

	void Camera::SetViewDirection(Vec3 position, Vec3 direction, Vec3 up) 
	{
		const Vec3 w{ xMath::Normalize(direction) };
		const Vec3 u{ xMath::Normalize(xMath::Cross(w, up)) };
		const Vec3 v{ xMath::Cross(w, u) };
		
		m_ViewMatrix = xMath::Mat4{1.f};
		m_ViewMatrix[0][0] = u.x;
		m_ViewMatrix[1][0] = u.y;
		m_ViewMatrix[2][0] = u.z;
		m_ViewMatrix[0][1] = v.x;
		m_ViewMatrix[1][1] = v.y;
		m_ViewMatrix[2][1] = v.z;
		m_ViewMatrix[0][2] = w.x;
		m_ViewMatrix[1][2] = w.y;
		m_ViewMatrix[2][2] = w.z;
		m_ViewMatrix[3][0] = -xMath::Dot(u, position);
		m_ViewMatrix[3][1] = -xMath::Dot(v, position);
		m_ViewMatrix[3][2] = -xMath::Dot(w, position);
		m_View = m_ViewMatrix;
		m_ViewProjection = m_Projection * m_View;
		eye = position;
	}

	void Camera::SetViewTarget(Vec3 position, Vec3 target, Vec3 up) 
	{
		center = target;
		m_Zoom = Length(target - position);
		const Vec3 direction = xMath::Normalize(target - position);
		rotation.x = std::asin(direction.y);
		rotation.y = std::atan2(direction.x, direction.z);
		SetViewDirection(position, target - position, up);
	}

	void Camera::SetViewYXZ(Vec3 position, Vec3 rotation) 
	{
		const float c3 = glm::cos(rotation.z);
		const float s3 = glm::sin(rotation.z);
		const float c2 = glm::cos(rotation.x);
		const float s2 = glm::sin(rotation.x);
		const float c1 = glm::cos(rotation.y);
		const float s1 = glm::sin(rotation.y);
		const Vec3 u{(c1 * c3 + s1 * s2 * s3), (c2 * s3), (c1 * s2 * s3 - c3 * s1)};
		const Vec3 v{(c3 * s1 * s2 - c1 * s3), (c2 * c3), (c1 * c3 * s2 + s1 * s3)};
		const Vec3 w{(c2 * s1), (-s2), (c1 * c2)};
		m_ViewMatrix = xMath::Mat4{1.f};
		m_ViewMatrix[0][0] = u.x;
		m_ViewMatrix[1][0] = u.y;
		m_ViewMatrix[2][0] = u.z;
		m_ViewMatrix[0][1] = v.x;
		m_ViewMatrix[1][1] = v.y;
		m_ViewMatrix[2][1] = v.z;
		m_ViewMatrix[0][2] = w.x;
		m_ViewMatrix[1][2] = w.y;
		m_ViewMatrix[2][2] = w.z;
		m_ViewMatrix[3][0] = -xMath::Dot(u, position);
		m_ViewMatrix[3][1] = -xMath::Dot(v, position);
		m_ViewMatrix[3][2] = -xMath::Dot(w, position);
		m_View = m_ViewMatrix;
		m_ViewProjection = m_Projection * m_View;
		eye = position;
		this->rotation = rotation;
	}

	void Camera::SetSelectedEntity(Entity *entity)
	{
		Scene::GetEntity(entity->GetUUID());
	}

} // namespace SceneryEditorX

// -------------------------------------------------------
