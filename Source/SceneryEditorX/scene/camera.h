/**
* -------------------------------------------------------
* Scenery Editor X
* -------------------------------------------------------
* Copyright (c) 2025 Thomas Ray 
* Copyright (c) 2025 Coalition of Freeware Developers
* -------------------------------------------------------
* camera.h
* -------------------------------------------------------
* Created: 30/3/2025
* -------------------------------------------------------
*/

#pragma once
#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <typeindex>

// ---------------------------------------------------------

enum class CameraMode
{
    Orbit,
    Normal
};

enum class CameraAspect
{
    Perspective,
    Orthographic
};

static const std::string cameraModes[] = {"Orbit", "Normal"};
static const std::string cameraAspect[] = {"Perspective", "Orthographic"};

// ---------------------------------------------------------
namespace SceneryEditorX
{
	class Camera
	{
	public:

		struct Matrices
	    {
	        glm::mat4 perspective;
	        glm::mat4 view;
	    };
	
		struct
	    {
	        bool left = false;
	        bool right = false;
	        bool up = false;
	        bool down = false;
	    } keys;

		void setOrthographicProjection(float left, float right, float top, float bottom, float nearPlane, float farPlane);
        void setPerspectiveProjection(float fovy, float aspect, float nearPlane, float farPlane);

        void setViewDirection(glm::vec3 position, glm::vec3 direction, glm::vec3 up = glm::vec3{0.f, -1.f, 0.f});
        void setViewTarget(glm::vec3 position, glm::vec3 target, glm::vec3 up = glm::vec3{0.f, -1.f, 0.f});
        void setViewYXZ(glm::vec3 position, glm::vec3 rotation);

        const glm::mat4 &getProjection() const
        {
            return projectionMatrix;
        }
        const glm::mat4 &getView() const
        {
            return viewMatrix;
        }
        const glm::mat4 &getInverseView() const
        {
            return inverseViewMatrix;
        }
        const glm::vec3 getPosition() const
        {
            return glm::vec3(inverseViewMatrix[3]);
        }

	
	private:

        glm::mat4 projectionMatrix{1.f};
        glm::mat4 viewMatrix{1.f};
        glm::mat4 inverseViewMatrix{1.f};
	};
} // namespace SceneryEditorX
// ---------------------------------------------------------
