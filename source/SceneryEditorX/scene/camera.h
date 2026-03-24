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
#include "SceneryEditorX/renderer/vulkan/viewport.h"
#include <SceneryEditorX/core/identifiers/flag.h>
#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include "components/renderable.h"
#include <glm/glm.hpp>
#include <xMath/includes/frustum.h>

// -------------------------------------------------------

namespace SceneryEditorX
{
	struct TransformComponent;
	struct CameraComponent;
	class Entity;

	/**
	 * @enum CameraMode
	 * @brief Represents the possible camera modes.
	 */
	enum CameraMode : uint8_t
	{
		NONE  = BIT(0),
		ORBIT = BIT(1),
		FLY   = BIT(2),
		FREE  = BIT(3),
		FIXED = BIT(4)
	};

	/**
	 * @enum CameraMovement
	 * @brief Represents the possible camera movements.
	 */
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

	/**
	 * @enum CameraType
	 * @brief Represents the possible camera types.
	 */
	enum class CameraType : uint8_t
	{
		Perspective  = BIT(0),
		Orthographic = BIT(1)
	};

	// -------------------------------------------------------

	/**
	 * @struct CameraShaderData
	 * @brief Represents the data layout for the camera shader.
	 * @note Align to 16 bytes for standard Vulkan UBO memory requirements (std140)
	 */
	struct alignas(16) CameraShaderData 
	{
		Mat4 view;                  // View matrix (reverse-Z)
		Mat4 projection;            // Projection matrix (reverse-Z)
		Mat4 viewProjection;		// Pre-calculated on CPU to save GPU cycles, as it's commonly used in shaders
		Mat4 inverseViewProjection; // Pre-inverted to save GPU cycles, as it's commonly used for mirror reconstruction in shaders
		Vec3 positionWorld;			// Camera position in world space
		float padding;				// Aligned with 'float padding' in Slang
	};

	/**
	 * @class Camera
	 * @brief Represents a camera in the scene.
	 *
	 * The Camera class is responsible for managing the view and projection matrices, handling camera movement and input, 
	 * and providing utility functions for coordinate transformations and frustum culling. 
	 * It inherits from Node, allowing it to be part of the scene graph and have a transform hierarchy.
	 */
	class Camera : public RefCounted
	{
	public:
		Camera();
		CameraShaderData GetShaderData() const;
		//virtual void Serialize(Serializer &ser) override;

		void Init();
		void Tick();

		Camera(const Camera &) = default;
		Camera &operator=(const Camera &) = default;

		// Syncs the camera logic with the data stored in the ECS registry
		void Update(const TransformComponent& transform, const CameraComponent& cameraData, const Viewport& viewport);

		// Accessors for the Vulkan renderer
		const xMath::Mat4& GetViewMatrix() const { return m_ViewMatrix; }
		const xMath::Mat4& GetProjectionMatrix() const { return m_ProjectionMatrix; }
		
		// Input processing for the free-look editor camera
		void ProcessInput(TransformComponent& transform, float deltaTime);

		virtual Entity *GetEntity() const { return nullptr; }
		virtual xMath::Matrix GetViewProjectionMatrix() const { return xMath::Matrix{}; }

		/**
		 * @brief Converts a world position to screen coordinates. 
		 * 
		 * The resulting screen coordinates are in pixels, where (0,0) is the top-left corner of the 
		 * viewport and (viewportWidth, viewportHeight) is the bottom-right corner.
		 * @param worldPos The position in world space to be converted to screen coordinates.
		 * @param screenPos The output parameter that will hold the resulting screen coordinates in pixels.
		 */
		void WorldToScreenCoordinates(const xMath::Vec3 &worldPos, xMath::Vec2 &screenPos) const;

		/**
		 * @brief Converts a world bounding box to a screen rectangle.
		 * @param boundingBox The bounding box in world space to be converted to screen coordinates.
		 * @return The resulting screen rectangle in pixels.
		 */
		xMath::Rectangle WorldToScreenCoordinates(const xMath::BoundingBox &boundingBox) const;

		/**
		 * @brief Converts screen coordinates to a world position.
		 * @param screenPos The screen coordinates in pixels.
		 * @param z The depth value, ranging from 0.0f (near plane) to 1.0f (far plane).
		 * @return The corresponding world position.
		 */
		xMath::Vec3 ScreenToWorldCoordinates(const xMath::Vec2 &screenPos, const float z) const;

		/**
		 * @brief Gets the horizontal field of view in degrees.
		 * @return The horizontal field of view in degrees.
		 */
		float GetFovHorizontalDeg() const;

		/**
		 * @brief Gets the vertical field of view in radians.
		 * @return The vertical field of view in radians.
		 */
		float GetFovVerticalRad() const;

		/**
		 * @brief Sets the horizontal field of view in degrees.
		 * @param fov The horizontal field of view in degrees.
		 */
		void SetFovHorizontalDeg(float fov);

		/**
		 * @brief Gets the aspect ratio of the camera's viewport (width divided by height).
		 * @return The aspect ratio of the camera's viewport.
		 */
		static float GetAspectRatio();

		/**
		 * @brief 
		 * @return 
		 */
		float GetShutterSpeed() const { return m_ShutterSpeed; }

		/**
		 * @brief 
		 * @param shutterSpeed 
		 */
		void SetShutterSpeed(const float shutterSpeed)  { m_ShutterSpeed = shutterSpeed; }

		/**
		 * @brief 
		 * @return 
		 */
		float GetIso() const { return m_ISO; }

		/**
		 * @brief 
		 * @param iso 
		 */
		void SetIso(const float iso) { m_ISO = iso; }

		/**
		 * @brief 
		 * @return 
		 */
		float GetAperture() const { return m_Aperture; }

		/**
		 * @brief 
		 * @param aperture 
		 */
		void SetAperture(const float aperture) { m_Aperture = aperture; }

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

		/**
		 * @brief Checks if a bounding box is within the camera's view frustum.
		 * @param boundingBox The bounding box to test against the camera's view frustum.
		 * @return True if the bounding box is within the view frustum, false otherwise.
		 */
		bool IsInViewFrustum(const xMath::BoundingBox &boundingBox) const;

		/**
		 * @brief 
		 * @return 
		 */
		CameraType GetProjectionType() const { return cameraType; }

		/**
		 * @brief Checks if a renderable is within the camera's view frustum.
		 * @param renderable The renderable to test against the camera's view frustum.
		 * @return True if the renderable is within the view frustum, false otherwise.
		 */
		bool IsInViewFrustum(Ref<Renderable> renderable) const;

		/**
		 * @brief Gets the current camera flag, which indicates whether the view or projection matrices need to be recomputed.
		 * @return The current camera flag.
		 */
		inline Flag GetFlag() { return m_CameraFlag; }

		/**
		 * @brief Sets the current camera flag, which indicates whether the view or projection matrices need to be recomputed.
		 * @return The previous camera flag.
		 */
		Flag SetFlag();

		// -------------------------------------------------------

		CameraType cameraType = CameraType::Perspective;
		CameraMode mode = ORBIT;

		Vec3 eye		= Vec3(0.0f, 0.0f, 0.0f);
		Vec3 center		= Vec3(0.0f, 0.0f, 0.0f);
		Vec2 extent		= Vec2(1.0f, 1.0f);
		Vec3 rotation	= Vec3(0.0f, 0.0f, 0.0f); // Euler angles: x=pitch, y=yaw (radians)

		// -------------------------------------------------------

		float farDistance  = 1000.0f;
		float nearDistance = 0.01f;
		float horizontalFov = 60.0f;

		float orthoFarDistance  = 10.0f;
		float orthoNearDistance = -100.0f;

		/* @brief Computes a fresh view matrix from current eye/center/rotation. */
		//Mat4 GetViewMatrix() const;

		/* @brief Computes a fresh projection matrix (reverse-Z). */
		//Mat4 GetProjectionMatrix() const;

		/**
		 * @brief Sets the projection type of the camera.
		 * @param projection The projection type to set.
		 */
		void SetProjection(CameraType projection);

		/**
		 * @brief Updates the view matrix based on the current camera parameters.
		 * @return The updated view matrix.
		 */
		Mat4 UpdateViewMatrix() const;

		/**
		 * @brief Computes a projection matrix based on the given near and far plane distances.
		 * @param nearPlane The near plane distance.
		 * @param farPlane The far plane distance.
		 * @return The computed projection matrix.
		 */
		Mat4 ComputeProjection(float nearPlane, float farPlane) const;

		/**
		 * @brief Sets the orthographic projection matrix based on the given left, right, top, bottom, near, and far plane distances.
		 * @param left The left plane distance.
		 * @param right The right plane distance.
		 * @param top The top plane distance.
		 * @param bottom The bottom plane distance.
		 * @param nearPlane The near plane distance.
		 * @param farPlane The far plane distance.
		 */
		void SetOrthographicProjection(float left, float right, float top, float bottom, float nearPlane, float farPlane);

		/**
		 * @brief Sets the perspective projection matrix based on the given field of view, aspect ratio, and near/far plane distances.
		 * @param fov_rad The field of view in radians.
		 * @param aspect The aspect ratio (width/height).
		 * @param near_z The near plane distance.
		 * @param far_z The far plane distance.
		 */
		void SetPerspectiveProjection(float fov_rad, float aspect, float near_z, float far_z);

		/**
		 * @brief Sets the view matrix based on the given position, direction, and up vector.
		 * @param position The position of the camera.
		 * @param direction The direction the camera is looking at.
		 * @param up The up vector of the camera.
		 */
		void SetViewDirection(Vec3 position, Vec3 direction, Vec3 up = Vec3{0.f, -1.f, 0.f});

		/**
		 * @brief Sets the view matrix based on the given position, target, and up vector.
		 * @param position The position of the camera.
		 * @param target The target point the camera is looking at.
		 * @param up The up vector of the camera.
		 */
		void SetViewTarget(Vec3 position, Vec3 target, Vec3 up = Vec3{0.f, -1.f, 0.f});

		/**
		 * @brief Sets the view matrix based on the given position and rotation (YXZ Euler angles).
		 * @param position The position of the camera.
		 * @param rotation The rotation of the camera (YXZ Euler angles).
		 */
		void SetViewYXZ(Vec3 position, Vec3 rotation);

		/**
		 * @brief Sets the selected entity for the camera.
		 * @param entity The entity to select.
		 */
		static void SetSelectedEntity(Entity* entity);

		/**
		 * @brief Gets the currently selected entity for the camera.
		 * @return The currently selected entity.
		 */
		Entity* GetSelectedEntity();

		/**
		 * @brief Gets the currently selected entities for the camera.
		 * @return A reference to the vector of currently selected entities.
		 */
		std::vector<Entity*> &GetSelectedEntities();

		/**
		 * @brief Checks if the given entity is currently selected by the camera.
		 * @param entity The entity to check.
		 * @return True if the entity is selected, false otherwise.
		 */
		bool IsSelected(Entity* entity) const;

		/**
		 * @brief Checks if the camera is currently controlled.
		 * @return True if the camera is controlled, false otherwise.
		 */
		bool IsControlled();

		/* @brief Clears the currently selected entity for the camera. */
		void ClearSelection();

		float GetExposure() const
		{
			// computed ev (using squared aperture for photometric accuracy)
			// note: this calculates the exposure scale factor (1/l_avg)
			float ev100 = std::log2((m_Aperture * m_Aperture) / m_ShutterSpeed * 100.0f / m_ISO);
		
			// standard output sensitivity (sos) calculation
			// 1.2 is a common calibration constant (matches ue5/frostbite)
			// this maps the average scene luminance to middle grey (0.18)
			const float calibrationConstant = 1.2f;
			float baseExposure = 1.0f / (calibrationConstant * std::pow(2.0f, ev100));
		
			return baseExposure;
		}

	private:

		/**
		 * @brief Computes the view and projection matrices based on the current camera parameters.
		 * @param transform The transform component of the camera.
		 * @param cameraData The camera component data.
		 * @param viewport The viewport information.
		 */
		void ComputeMatrices(const TransformComponent& transform, const CameraComponent& cameraData, const Viewport& viewport);

		/* @brief Reads keyboard and mouse state and updates eye/center/rotation. */
		void ProcessCameraInput();

		Flag m_CameraFlag; // Dirty flag — set when view or projection must be recomputed.
		Vec2 m_Jitter = Vec2(0);

		uint32_t m_JitterIndex = 0;
		Vec3 m_MoveSpeed = Vec3(0.0f);

		bool m_IsControlled = false;
		bool m_UseJitter = true;
		float m_Zoom				= 10.0f; // Orbit-mode distance from center.
		float m_Aperture		= 5.6f;          // aperture value in f-stop. Controls the amount of light, depth of field and chromatic aberration
		float m_ShutterSpeed	= 1.0f / 125.0f; // length of time for which the camera shutter is open (sec). Also controls the amount of motion blur
		float m_ISO				= 200.0f;        // sensitivity to light

		Viewport m_LastViewport;
		float m_HorizontalFov_Rad     = 90.0f * xMath::DEG_TO_RAD;
		float m_NearPlane             = 0.1f;
		float m_FarPlane              = 10'000.0f; // Max for 32-bit reverse-Z depth buffer.
		CameraType m_ProjectionType   = CameraType::Perspective;

		std::vector<Entity *> m_SelectedEntities;

		xMath::Mat4 m_ProjectionMatrix{1.f};
		xMath::Mat4 m_ViewMatrix{1.f};

		xMath::Mat4 m_View							= xMath::Mat4(1.0f);
		xMath::Mat4 m_Projection					= xMath::Mat4(1.0f);
		xMath::Mat4 m_ProjectionNonReverseZ			= xMath::Mat4(1.0f);
		xMath::Mat4 m_ViewProjection				= xMath::Mat4(1.0f);
		xMath::Mat4 m_ViewProjectionNonReverseZ		= xMath::Mat4(1.0f);
		xMath::Mat4 m_MatrixPrevious				= xMath::Mat4(1.0f);
		xMath::Vec2 m_LastMousePosition				= xMath::Vec2::Zero;
		xMath::Vec3 m_MovementSpeed					= xMath::Vec3(0.0f);
		xMath::Frustum m_Frustum;
		
		friend class AssetManager;
	};

}

// -------------------------------------------------------
