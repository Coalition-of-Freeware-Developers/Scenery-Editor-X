/**
* -------------------------------------------------------
* Scenery Editor X
* -------------------------------------------------------
* Copyright (c) 2025 Thomas Ray 
* Copyright (c) 2025 Coalition of Freeware Developers
* -------------------------------------------------------
* camera.cpp
* -------------------------------------------------------
* Created: 2/8/2025
* -------------------------------------------------------
*/
#include <SceneryEditorX/renderer/camera.h>

/// -------------------------------------------------------

namespace SceneryEditorX
{
    Camera::Camera(const glm::mat4& projection, const glm::mat4& unReversedProjection) : m_ProjectionMatrix(projection), m_UnReversedProjectionMatrix(unReversedProjection)
	{
	}

	Camera::Camera(const float degFov, const float width, const float height, const float nearP, const float farP)
		: m_ProjectionMatrix(glm::perspectiveFov(glm::radians(degFov), width, height, farP, nearP)), m_UnReversedProjectionMatrix(glm::perspectiveFov(glm::radians(degFov), width, height, nearP, farP))
	{
	}
}

/// -------------------------------------------------------
