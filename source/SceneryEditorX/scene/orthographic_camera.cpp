/**
* -------------------------------------------------------
* Scenery Editor X
* -------------------------------------------------------
* Copyright (c) 2025 Thomas Ray 
* Copyright (c) 2025 Coalition of Freeware Developers
* -------------------------------------------------------
* orthographic_camera.cpp
* -------------------------------------------------------
* Created: 24/3/2025
* -------------------------------------------------------
*/

#include <SceneryEditorX/scene/camera.h>
#include <SceneryEditorX/scene/orthographic_camera.h>

// -------------------------------------------------------

namespace Camera
{
	OrthographicCamera::OrthographicCamera(const std::string &name) : Camera{name}
	{
	}
	
	OrthographicCamera::OrthographicCamera(const std::string &name,
	                                       float left,
	                                       float right,
	                                       float bottom,
	                                       float top,
	                                       float near_plane,
	                                       float far_plane)
	    : Camera{name}, left{left}, right{right}, top{top}, bottom{bottom}, near_plane{near_plane}, far_plane{far_plane}
	{
	}
	
	void OrthographicCamera::set_left(float new_left)
	{
	    left = new_left;
	}
	
	float OrthographicCamera::get_left() const
	{
	    return left;
	}
	
	void OrthographicCamera::set_right(float new_right)
	{
	    right = new_right;
	}
	
	float OrthographicCamera::get_right() const
	{
	    return right;
	}
	
	void OrthographicCamera::set_bottom(float new_bottom)
	{
	    bottom = new_bottom;
	}
	
	float OrthographicCamera::get_bottom() const
	{
	    return bottom;
	}
	
	void OrthographicCamera::set_top(float new_top)
	{
	    top = new_top;
	}
	
	float OrthographicCamera::get_top() const
	{
	    return top;
	}
	
	void OrthographicCamera::set_near_plane(float new_near_plane)
	{
	    near_plane = new_near_plane;
	}
	
	float OrthographicCamera::get_near_plane() const
	{
	    return near_plane;
	}
	
	void OrthographicCamera::set_far_plane(float new_far_plane)
	{
	    far_plane = new_far_plane;
	}
	
	float OrthographicCamera::get_far_plane() const
	{
	    return far_plane;
	}
	
	glm::mat4 OrthographicCamera::get_projection()
	{
	    // Note: Using reversed depth-buffer for increased precision, so Znear and Zfar are flipped
	    return glm::ortho(get_left(), get_right(), get_bottom(), get_top(), get_far_plane(), get_near_plane());
	}
	
} // namespace Camera
