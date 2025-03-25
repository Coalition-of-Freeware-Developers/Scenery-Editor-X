/**
* -------------------------------------------------------
* Scenery Editor X
* -------------------------------------------------------
* Copyright (c) 2025 Thomas Ray 
* Copyright (c) 2025 Coalition of Freeware Developers
* -------------------------------------------------------
* camera.h
* -------------------------------------------------------
* Created: 24/3/2025
* -------------------------------------------------------
*/

#pragma once
#include <SceneryEditorX/scene/component.h>

// -------------------------------------------------------

namespace Scene::Camera
{
    // -------------------------------------------------------
    class Camera : public Component
    {
    public:
        void setOrthographicProjection(float left, float right, float top, float bottom, float near, float far);
        void setPerspectiveProjection(float fovy, float aspect, float near, float far);

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

} // namespace Scene::Camera

/*
struct CameraController
{
    float accelerationTime = 200.0f;
    float rotationSmooth = 0.2f;

    Vec3 moveSpeed = Vec3(.0f);
    Vec3 rotationSpeed = Vec3(.0f);

    void Reset();
    void Update(Ref<struct SceneAsset> &scene, Ref<struct CameraNode> &cam, bool viewportHovered, float deltaTime);
};

namespace AssetManager
{
	struct CameraNode : Node
	{
	    enum CameraMode
	    {
	        Orbit,
	        Fly
	    };
	    inline static const char *modeNames[] = {"Orbit", "Fly"};
	
	    enum class CameraType
	    {
	        Perspective,
	        Orthographic
	    };
	    inline static const char *typeNames[] = {"Perspective", "Orthographic"};
	
	    CameraType cameraType = CameraType::Perspective;
	    CameraMode mode = CameraMode::Orbit;
	
	    Vec3 eye = Vec3(0);
	    Vec3 center = Vec3(0);
	    Vec3 rotation = Vec3(0);
	
	    bool useJitter = true;
	    float zoom = 10.0f;
	
	    float farDistance = 1000.0f;
	    float nearDistance = 0.01f;
	    float horizontalFov = 60.0f;
	
	    float orthoFarDistance = 10.0f;
	    float orthoNearDistance = -100.0f;
	
	    Vec2 extent = Vec2(1.0f);
	
	    CameraNode();
	    virtual void Serialize(Serializer &s);
	
	    Mat4 GetView();
	    Mat4 GetProj();
	    Mat4 GetProjJittered();
	    Mat4 GetProj(float zNear, float zFar);
	
	    Vec2 GetJitter();
	    void NextJitter();
	
	private:
	    Vec2 jitter = Vec2(0);
	    uint32_t jitterIndex = 0;
	};
} // namespace AssetManager
*/

// -------------------------------------------------------
