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
 * camera.h
 * -------------------------------------------------------
 * Created: 16/4/2025
 * -------------------------------------------------------
 */
#pragma once
#include "node.h"
#include "SceneryEditorX/renderer/vulkan/viewport.h"
#include <SceneryEditorX/core/identifiers/flag.h>
#include <xMath/includes/frustum.h>

// -------------------------------------------------------

namespace SceneryEditorX
{
    class Renderable;
    class Entity;

    enum CameraMode : uint8_t
	{
        NONE  = BIT(0),
	    ORBIT = BIT(1),
	    FLY   = BIT(2),
		FREE  = BIT(3),
        FIXED = BIT(4)
	};

	enum class CameraMovement : uint8_t 
    {
		FORWARD		= BIT(0),
		BACKWARD	= BIT(1),
		LEFT		= BIT(2),
		RIGHT		= BIT(3),
		UP			= BIT(4),
		DOWN		= BIT(5)
    };

    // -------------------------------------------------------
	
	enum class CameraType : uint8_t
	{
	    Perspective  = BIT(0),
	    Orthographic = BIT(1)
	};

    // -------------------------------------------------------

	// Align to 16 bytes for standard Vulkan UBO memory requirements (std140)
	struct alignas(16) CameraShaderData 
	{
	    Mat4 view;
	    Mat4 projection;
	    Mat4 viewProjection;
	    Mat4 inverseViewProjection;
	    Vec3 positionWorld;
	    float padding;
	};

    class Camera : public Node
    {
    public:
        Camera();
        virtual ~Camera() = default;
        CameraShaderData GetShaderData() const;
        //virtual void Serialize(Serializer &ser) override;

        void Init();
        void Tick();

        Camera(const Camera &) = default;
        Camera &operator=(const Camera &) = default;

        virtual Entity *GetEntity() const
        {
            return nullptr;
        }
        virtual xMath::Matrix GetViewProjectionMatrix() const
        {
            return xMath::Matrix{};
        }

        // Converts a world point to a screen point
        void WorldToScreenCoordinates(const xMath::Vec3 &worldPos, xMath::Vec2 &screenPos) const;

        // converts a world bounding box to a screen rectangle
        xMath::Rectangle WorldToScreenCoordinates(const xMath::BoundingBox &boundingBox) const;

        // converts a screen point to a world point. Z can be 0.0f to 1.0f and it will lerp between the near and far plane
        xMath::Vec3 ScreenToWorldCoordinates(const xMath::Vec2 &screenPos, const float z) const;

		float GetFovHorizontalDeg() const;
		float GetFovVerticalRad() const;
		void SetFovHorizontalDeg(float fov);
        static float GetAspectRatio();

		/* @brief Returns the current eye (camera world position). */
		Vec3 GetEyePosition() const;

		/* @brief Returns the cached near-plane distance. */
		float GetNearPlane() const { return m_NearPlane; }

		/* @brief Returns the cached far-plane distance. */
		float GetFarPlane() const { return m_FarPlane; }

		/* @brief Returns the cached view matrix (updated by Tick). */
        const Mat4 GetView() const { return m_View; }

		/* @brief Returns the cached projection matrix (updated by Tick). */
        const Mat4 GetProjection() const { return m_Projection; }

		// Frustum
		bool IsInViewFrustum(const xMath::BoundingBox &boundingBox) const;
		bool IsInViewFrustum(Ref<Renderable> renderable) const;

		inline Flag GetFlag() { return m_CameraFlag; }
		Flag SetFlag();

		// -------------------------------------------------------

		CameraType cameraType = CameraType::Perspective;
		CameraMode mode = ORBIT;

		Vec3 eye		= Vec3(0);
		Vec3 center		= Vec3(0);
		Vec2 extent		= Vec2(1.0f);
		Vec3 rotation	= Vec3(0); // Euler angles: x=pitch, y=yaw (radians)

		// -------------------------------------------------------

		float farDistance  = 1000.0f;
		float nearDistance = 0.01f;
		float horizontalFov = 60.0f;

		float orthoFarDistance  = 10.0f;
		float orthoNearDistance = -100.0f;

		/* @brief Computes a fresh view matrix from current eye/center/rotation. */
		Mat4 GetViewMatrix() const;

		/* @brief Computes a fresh projection matrix (reverse-Z). */
		Mat4 GetProjectionMatrix() const;

		void SetProjection(CameraType projection);
		Mat4 UpdateViewMatrix() const;
		Mat4 ComputeProjection(float nearPlane, float farPlane) const;

	private:
		void ComputeMatrices();

		/* @brief Reads keyboard and mouse state and updates eye/center/rotation. */
		void ProcessCameraInput();

		Flag m_CameraFlag; // Dirty flag — set when view or projection must be recomputed.
		Vec2 m_Jitter = Vec2(0);
		uint32_t m_JitterIndex = 0;
		Vec3 m_MoveSpeed = Vec3(0.0f);
		friend class AssetManager;
		bool useJitter = true;
		float zoom = 10.0f; // Orbit-mode distance from center.
		Viewport m_LastViewport;
		float m_HorizontalFov_Rad    = 90.0f * xMath::DEG_TO_RAD;
		float m_NearPlane             = 0.1f;
		float m_FarPlane              = 10'000.0f; // Max for 32-bit reverse-Z depth buffer.
		CameraType m_ProjectionType   = CameraType::Perspective;
        xMath::Mat4 m_View							= xMath::Mat4(1.0f);
		xMath::Mat4 m_Projection					= xMath::Mat4(1.0f);
		xMath::Mat4 m_ProjectionNonReverseZ			= xMath::Mat4(1.0f);
		xMath::Mat4 m_ViewProjection				= xMath::Mat4(1.0f);
		xMath::Mat4 m_ViewProjectionNonReverseZ		= xMath::Mat4(1.0f);
		xMath::Mat4 m_MatrixPrevious				= xMath::Mat4(1.0f);
		xMath::Vec2 m_LastMousePosition				= xMath::Vec2::Zero;
		xMath::Vec3 m_MovementSpeed					= xMath::Vec3(0.0f);
		xMath::Frustum m_Frustum;
    };

}

// -------------------------------------------------------
