/**
* -------------------------------------------------------
* Scenery Editor X
* -------------------------------------------------------
* Copyright (c) 2025 Thomas Ray 
* Copyright (c) 2025 Coalition of Freeware Developers
* -------------------------------------------------------
* camera.cpp
* -------------------------------------------------------
* Created: 30/3/2025
* -------------------------------------------------------
*/
#include <SceneryEditorX/scene/camera.h>
#include <cassert>
#include <limits>
// ---------------------------------------------------------

namespace SceneryEditorX
{

	void Camera::setOrthographicProjection(float left, float right, float top, float bottom, float nearPlane, float farPlane)
	{
	    projectionMatrix = glm::mat4{1.0f};
	    projectionMatrix[0][0] = 2.f / (right - left);
	    projectionMatrix[1][1] = 2.f / (bottom - top);
	    projectionMatrix[2][2] = 1.f / (farPlane - nearPlane);
	    projectionMatrix[3][0] = -(right + left) / (right - left);
	    projectionMatrix[3][1] = -(bottom + top) / (bottom - top);
	    projectionMatrix[3][2] = -nearPlane / (farPlane - nearPlane);
	}
	
	void Camera::setPerspectiveProjection(float fovy, float aspect, float nearPlane, float farPlane)
	{
	    assert(glm::abs(aspect - std::numeric_limits<float>::epsilon()) > 0.0f);
	    const float tanHalfFovy = tan(fovy / 2.f);
	    projectionMatrix = glm::mat4{0.0f};
	    projectionMatrix[0][0] = 1.f / (aspect * tanHalfFovy);
	    projectionMatrix[1][1] = 1.f / (tanHalfFovy);
	    projectionMatrix[2][2] = farPlane / (farPlane - nearPlane);
	    projectionMatrix[2][3] = 1.f;
	    projectionMatrix[3][2] = -(farPlane * nearPlane) / (farPlane - nearPlane);
	}
	
	void Camera::setViewDirection(glm::vec3 position, glm::vec3 direction, glm::vec3 up)
	{
	    const glm::vec3 w{glm::normalize(direction)};
	    const glm::vec3 u{glm::normalize(glm::cross(w, up))};
	    const glm::vec3 v{glm::cross(w, u)};
	
	    viewMatrix = glm::mat4{1.f};
	    viewMatrix[0][0] = u.x;
	    viewMatrix[1][0] = u.y;
	    viewMatrix[2][0] = u.z;
	    viewMatrix[0][1] = v.x;
	    viewMatrix[1][1] = v.y;
	    viewMatrix[2][1] = v.z;
	    viewMatrix[0][2] = w.x;
	    viewMatrix[1][2] = w.y;
	    viewMatrix[2][2] = w.z;
	    viewMatrix[3][0] = -glm::dot(u, position);
	    viewMatrix[3][1] = -glm::dot(v, position);
	    viewMatrix[3][2] = -glm::dot(w, position);
	
	    inverseViewMatrix = glm::mat4{1.f};
	    inverseViewMatrix[0][0] = u.x;
	    inverseViewMatrix[0][1] = u.y;
	    inverseViewMatrix[0][2] = u.z;
	    inverseViewMatrix[1][0] = v.x;
	    inverseViewMatrix[1][1] = v.y;
	    inverseViewMatrix[1][2] = v.z;
	    inverseViewMatrix[2][0] = w.x;
	    inverseViewMatrix[2][1] = w.y;
	    inverseViewMatrix[2][2] = w.z;
	    inverseViewMatrix[3][0] = position.x;
	    inverseViewMatrix[3][1] = position.y;
	    inverseViewMatrix[3][2] = position.z;
	}
	
	void Camera::setViewTarget(glm::vec3 position, glm::vec3 target, glm::vec3 up)
	{
	    setViewDirection(position, target - position, up);
	}
	
	void Camera::setViewYXZ(glm::vec3 position, glm::vec3 rotation)
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
	    viewMatrix = glm::mat4{1.f};
	    viewMatrix[0][0] = u.x;
	    viewMatrix[1][0] = u.y;
	    viewMatrix[2][0] = u.z;
	    viewMatrix[0][1] = v.x;
	    viewMatrix[1][1] = v.y;
	    viewMatrix[2][1] = v.z;
	    viewMatrix[0][2] = w.x;
	    viewMatrix[1][2] = w.y;
	    viewMatrix[2][2] = w.z;
	    viewMatrix[3][0] = -glm::dot(u, position);
	    viewMatrix[3][1] = -glm::dot(v, position);
	    viewMatrix[3][2] = -glm::dot(w, position);
	
	    inverseViewMatrix = glm::mat4{1.f};
	    inverseViewMatrix[0][0] = u.x;
	    inverseViewMatrix[0][1] = u.y;
	    inverseViewMatrix[0][2] = u.z;
	    inverseViewMatrix[1][0] = v.x;
	    inverseViewMatrix[1][1] = v.y;
	    inverseViewMatrix[1][2] = v.z;
	    inverseViewMatrix[2][0] = w.x;
	    inverseViewMatrix[2][1] = w.y;
	    inverseViewMatrix[2][2] = w.z;
	    inverseViewMatrix[3][0] = position.x;
	    inverseViewMatrix[3][1] = position.y;
	    inverseViewMatrix[3][2] = position.z;
	}

	/*
	Camera::Camera(const glm::vec3 &position, const glm::quat &orientation,
	               float verticalFOV, float aspectRatio, float zNear, float zFar)
	    : m_Position(position), m_Orientation(orientation), m_VerticalFOV(verticalFOV), m_AspectRatio(aspectRatio),
	      m_ZNear(zNear), m_ZFar(zFar)
	{
	
	    m_Right = m_Orientation * LH_BASIS_RIGHT;
	    m_Up = m_Orientation * LH_BASIS_UP;
	    m_Forward = m_Orientation * LH_BASIS_FORWARD;
	
	    update();
	}

	void Camera::update() noexcept
    {
        if (m_UpdateViewMatrix)
        {
            update_view_matrix();
        }

        if (m_UpdateProjMatrix)
        {
            update_proj_matrix();
        }

        if (m_UpdateViewMatrix || m_UpdateProjMatrix)
        {
            m_InvViewProjMatrix = glm::inverse(m_ProjMatrix * m_ViewMatrix);
        }

        m_UpdateViewMatrix = false;
        m_UpdateProjMatrix = false;
    }

	void Camera::set_position(const glm::vec3 &position) noexcept
    {
        if (position == m_Position)
        {
            return;
        }

        m_UpdateViewMatrix = true;
        m_Position = position;
    }

    void Camera::set_orientation(const glm::quat &orientation) noexcept
    {
        if (orientation == m_Orientation)
        {
            return;
        }

        m_UpdateViewMatrix = true;
        m_Orientation = orientation;

        m_Right = glm::normalize(m_Orientation * LH_BASIS_RIGHT);
        m_Up = glm::normalize(m_Orientation * LH_BASIS_UP);
        m_Forward = glm::normalize(m_Orientation * LH_BASIS_FORWARD);
    }

    void Camera::set_vertical_fov(float fov) noexcept
    {
        if (fov == m_VerticalFOV)
        {
            return;
        }

        m_UpdateProjMatrix = true;
        m_VerticalFOV = fov;
    }

    void Camera::set_aspect_ratio(float aspectRatio) noexcept
    {
        if (aspectRatio == m_AspectRatio)
        {
            return;
        }

        m_UpdateProjMatrix = true;
        m_AspectRatio = aspectRatio;
    }

    void Camera::update_view_matrix()
    {
        m_ViewMatrix = glm::lookAt(m_Position, m_Position + m_Forward, m_Up);
    }

    void Camera::update_proj_matrix()
    {
        m_ProjMatrix = glm::perspective(glm::radians(m_VerticalFOV), m_AspectRatio, m_ZNear, m_ZFar);
    }
	*/

} // namespace SceneryEditorX

// ---------------------------------------------------------
