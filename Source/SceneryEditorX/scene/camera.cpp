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

// ---------------------------------------------------------

namespace SceneryEditorX
{
	//Camera::Camera(const std::string &name)
	//{
	//    this->name = name;
	//    update_view_matrix();
	//}
	
	//glm::mat4 Camera::get_view()
//{
//
//}
	
	//const glm::mat4 Camera::get_pre_rotation()
//{
//    return pre_rotation;
//}
	
	//void Camera::pollMouseWheel(float yOffset)
//{
//}
	
	//bool Camera::moving()
//{
//    return keys.left || keys.right || keys.up || keys.down;
//}
	
	//void Camera::update_view_matrix()
//{
//    glm::mat4 rotation_matrix = glm::mat4(1.0f);
//    glm::mat4 transformation_matrix;
//
//	rotation_matrix = glm::rotate(rotation_matrix, glm::radians(rotation.x), glm::vec3(1.0f, 0.0f, 0.0f));
//    rotation_matrix = glm::rotate(rotation_matrix, glm::radians(rotation.y), glm::vec3(0.0f, 1.0f, 0.0f));
//    rotation_matrix = glm::rotate(rotation_matrix, glm::radians(rotation.z), glm::vec3(0.0f, 0.0f, 1.0f));
//
//    transformation_matrix = glm::translate(glm::mat4(1.0f), position);
//
//	matrices.view = rotation_matrix * transformation_matrix;
//    updated = true;
//}
	
	//float Camera::get_near_clip()
//{
//    return znear;
//}
	
	//float Camera::get_far_clip()
//{
//    return zfar;
//}
	
	//void Camera::set_perspective(float fov, float aspect, float znear, float zfar)
//{
//    this->fov = fov;
//    this->znear = znear;
//    this->zfar = zfar;
//    matrices.perspective = glm::perspective(glm::radians(fov), aspect, znear, zfar);
//    updated = true;
//}
	
	//void Camera::update_aspect_ratio(float aspect)
//{
//    matrices.perspective = glm::perspective(glm::radians(fov), aspect, znear, zfar);
//    updated = true;
//}
	
	//void Camera::set_position(const glm::vec3 &position)
//{
//    this->position = position;
//    update_view_matrix();
//}
	
	//void Camera::set_rotation(const glm::vec3 &rotation)
//{
//    this->rotation = rotation;
//    update_view_matrix();
//}
	
	//void Camera::rotate(const glm::vec3 &delta)
//{
//    this->rotation += delta;
//    update_view_matrix();
//}
	
	//void Camera::set_translation(const glm::vec3 &translation)
//{
//    this->position = translation;
//    update_view_matrix();
//}
	
	//void Camera::translate(const glm::vec3 &delta)
//{
//    this->position += delta;
//    update_view_matrix();
//}
	
	//void Camera::update(float deltaTime)
//{
//    updated = false;
//
//    if (moving())
//    {
//        glm::vec3 front;
//        front.x = -cos(glm::radians(rotation.x)) * sin(glm::radians(rotation.y));
//        front.y = sin(glm::radians(rotation.x));
//        front.z = cos(glm::radians(rotation.x)) * cos(glm::radians(rotation.y));
//        front = glm::normalize(front);
//
//        float move_speed = deltaTime * translation_speed;
//
//        if (keys.up)
//        {
//            position += front * move_speed;
//        }
//        if (keys.down)
//        {
//            position -= front * move_speed;
//        }
//        if (keys.left)
//        {
//            position -= glm::normalize(glm::cross(front, glm::vec3(0.0f, 1.0f, 0.0f))) * move_speed;
//        }
//        if (keys.right)
//        {
//            position += glm::normalize(glm::cross(front, glm::vec3(0.0f, 1.0f, 0.0f))) * move_speed;
//        }
//
//        update_view_matrix();
//    }
//
//}
	
	//bool Camera::update_gamepad(glm::vec2 axis_left, glm::vec2 axis_right, float delta_time)
//{
//    bool changed = false;
//
//    const float dead_zone = 0.0015f;
//    const float range = 1.0f - dead_zone;
//
//    glm::vec3 front;
//    front.x = -cos(glm::radians(rotation.x)) * sin(glm::radians(rotation.y));
//    front.y = sin(glm::radians(rotation.x));
//    front.z = cos(glm::radians(rotation.x)) * cos(glm::radians(rotation.y));
//    front = glm::normalize(front);
//
//    float move_speed = delta_time * translation_speed * 2.0f;
//    float new_rotation_speed = delta_time * rotation_speed * 50.0f;
//
//    // Move
//    if (fabsf(axis_left.y) > dead_zone)
//    {
//        float pos = (fabsf(axis_left.y) - dead_zone) / range;
//        position -= front * pos * ((axis_left.y < 0.0f) ? -1.0f : 1.0f) * move_speed;
//        changed = true;
//    }
//    if (fabsf(axis_left.x) > dead_zone)
//    {
//        float pos = (fabsf(axis_left.x) - dead_zone) / range;
//        position += glm::normalize(glm::cross(front, glm::vec3(0.0f, 1.0f, 0.0f))) * pos *
//                    ((axis_left.x < 0.0f) ? -1.0f : 1.0f) * move_speed;
//        changed = true;
//    }
//
//    // Rotate
//    if (fabsf(axis_right.x) > dead_zone)
//    {
//        float pos = (fabsf(axis_right.x) - dead_zone) / range;
//        rotation.y += pos * ((axis_right.x < 0.0f) ? -1.0f : 1.0f) * new_rotation_speed;
//        changed = true;
//    }
//    if (fabsf(axis_right.y) > dead_zone)
//    {
//        float pos = (fabsf(axis_right.y) - dead_zone) / range;
//        rotation.x -= pos * ((axis_right.y < 0.0f) ? -1.0f : 1.0f) * new_rotation_speed;
//        changed = true;
//    }
//
//    if (changed)
//    {
//        update_view_matrix();
//    }
//
//    return changed;
//}

	void Camera::setOrthographic(float left, float right, float top, float bottom, float, float)
	{
	}
	
	void Camera::setPerspective(float fovy, float aspect, float, float zFar)
	{
	}

} // namespace SceneryEditorX

// ---------------------------------------------------------
