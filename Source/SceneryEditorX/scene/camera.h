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

class Camera
{
public:
    //Camera(const std::string &name);
    virtual ~Camera() = default;


    virtual glm::mat4 get_projection() = 0;
    glm::mat4 get_view();
    const glm::mat4 get_pre_rotation();
    void pollMouseWheel(float yOffset);

	glm::vec3 rotation = glm::vec3();
    glm::vec3 position = glm::vec3();
    float rotation_speed = 1.0f;
    float translation_speed = 1.0f;

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
    bool updated = false;

    float get_near_clip();
    float get_far_clip();

	void update(float deltaTime);
    bool update_gamepad(glm::vec2 axis_left, glm::vec2 axis_right, float delta_time);
    void set_perspective(float fov, float aspect, float znear, float zfar);
    void update_aspect_ratio(float aspect);
    void set_position(const glm::vec3 &position);
    void set_rotation(const glm::vec3 &rotation);
    void rotate(const glm::vec3 &delta);
    void set_translation(const glm::vec3 &translation);
    void translate(const glm::vec3 &delta);

private:
    glm::mat4 pre_rotation{1.0f};
    float fov;
    float znear, zfar;

    void update_view_matrix();
};

// ---------------------------------------------------------
