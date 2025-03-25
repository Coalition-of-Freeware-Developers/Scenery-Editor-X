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
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>
#include <SceneryEditorX/core/base.hpp>
#include <SceneryEditorX/scene/serializer.h>

// -------------------------------------------------------

enum CameraType
{
    LookAt,
    FirstPerson
};

namespace Camera
{
    class Camera
    {
    public:
        void update(float deltaTime);
        Camera(const std::string &name = "");
        // Update camera passing separate axis data (gamepad)
        // Returns true if view or position has been changed
        bool update_gamepad(glm::vec2 axis_left, glm::vec2 axis_right, float delta_time);

		virtual glm::mat4 get_projection() = 0; 

        CameraType type = CameraType::LookAt;

        glm::vec3 rotation = glm::vec3();
        glm::vec3 position = glm::vec3();

        float rotation_speed = 1.0f;
        float translation_speed = 1.0f;

        bool updated = false;

        struct
        {
            glm::mat4 perspective;
            glm::mat4 view;
        } matrices;

        struct
        {
            bool left = false;
            bool right = false;
            bool up = false;
            bool down = false;
        } keys;

        bool moving();

        float get_near_clip();

        float get_far_clip();

        void set_perspective(float fov, float aspect, float znear, float zfar);

        void update_aspect_ratio(float aspect);

        void set_position(const glm::vec3 &position);

        void set_rotation(const glm::vec3 &rotation);

        void rotate(const glm::vec3 &delta);

        void set_translation(const glm::vec3 &translation);

        void translate(const glm::vec3 &delta);

    private:
        float fov;
        float znear, zfar;
        std::string name;
        void update_view_matrix();
    };

} // namespace Camera

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
