/**
* -------------------------------------------------------
* Scenery Editor X
* -------------------------------------------------------
* Copyright (c) 2025 Thomas Ray 
* Copyright (c) 2025 Coalition of Freeware Developers
* -------------------------------------------------------
* perspective_camera.cpp
* -------------------------------------------------------
* Created: 24/3/2025
* -------------------------------------------------------
*/

#include <SceneryEditorX/scene/camera.h>
#include <SceneryEditorX/scene/perspective_camera.h>

// -------------------------------------------------------

namespace Scene::Camera
{
/*
	PerspectiveCamera::PerspectiveCamera(const std::string &name) : Camera{name} { }
	
	void PerspectiveCamera::set_field_of_view(float new_fov) { fov = new_fov; }
	void PerspectiveCamera::set_far_plane(float zfar) { far_plane = zfar; }
	void PerspectiveCamera::set_near_plane(float znear) { near_plane = znear; }
	void PerspectiveCamera::set_aspect_ratio(float new_aspect_ratio) { aspect_ratio = new_aspect_ratio; }

	float PerspectiveCamera::get_far_plane() const { return far_plane; }
	float PerspectiveCamera::get_near_plane() const { return near_plane; }
	float PerspectiveCamera::get_field_of_view() { return fov; }
	float PerspectiveCamera::get_aspect_ratio()  { return aspect_ratio; }
	
	glm::mat4 PerspectiveCamera::get_projection()
	{
	    // Note: Using reversed depth-buffer for increased precision, so Znear and Zfar are flipped
	    return glm::perspective(fov, aspect_ratio, far_plane, near_plane);
	}
*/
} // namespace Camera

// -------------------------------------------------------
