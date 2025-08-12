/**
* -------------------------------------------------------
* Scenery Editor X
* -------------------------------------------------------
* Copyright (c) 2025 Thomas Ray 
* Copyright (c) 2025 Coalition of Freeware Developers
* -------------------------------------------------------
* camera.h
* -------------------------------------------------------
* Created: 2/8/2025
* -------------------------------------------------------
*/
#pragma once
#include <glm/glm.hpp>
#include <glm/ext/matrix_clip_space.hpp>

/// -------------------------------------------------------

namespace SceneryEditorX
{
	
	class Camera
	{
	public:
		Camera() = default;
		Camera(const Mat4& projection, const Mat4& unReversedProjection);
		Camera(const float degFov, const float width, const float height, const float nearP, const float farP);
		virtual ~Camera() = default;

        [[nodiscard]] const Mat4& GetProjectionMatrix() const { return m_ProjectionMatrix; }
        [[nodiscard]] const Mat4& GetUnReversedProjectionMatrix() const { return m_UnReversedProjectionMatrix; }

		void SetProjectionMatrix(const Mat4 &projection, const Mat4 &unReversedProjection)
		{
			m_ProjectionMatrix = projection;
			m_UnReversedProjectionMatrix = unReversedProjection;
		}

		void SetPerspectiveProjectionMatrix(const float radFov, const float width, const float height, const float nearP, const float farP)
		{
			m_ProjectionMatrix = glm::perspectiveFov(radFov, width, height, farP, nearP);
			m_UnReversedProjectionMatrix = glm::perspectiveFov(radFov, width, height, nearP, farP);
		}

		void SetOrthoProjectionMatrix(const float width, const float height, const float nearP, const float farP)
		{
			///TODO: Make sure this is correct.
			m_ProjectionMatrix = glm::ortho(-width * 0.5f, width * 0.5f, -height * 0.5f, height * 0.5f, farP, nearP);
			m_UnReversedProjectionMatrix = glm::ortho(-width * 0.5f, width * 0.5f, -height * 0.5f, height * 0.5f, nearP, farP);
		}

        float GetExposure() const { return m_Exposure; }
		float &GetExposure() { return m_Exposure; }
	protected:
		float m_Exposure = 0.8f;

	private:
		Mat4 m_ProjectionMatrix = Mat4(1.0f);
		Mat4 m_UnReversedProjectionMatrix = Mat4(1.0f); /// Currently only needed for shadow maps and ImGuizmo
	};

}

/// -------------------------------------------------------
