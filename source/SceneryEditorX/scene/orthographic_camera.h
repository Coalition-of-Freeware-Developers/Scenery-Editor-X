/**
* -------------------------------------------------------
* Scenery Editor X
* -------------------------------------------------------
* Copyright (c) 2025 Thomas Ray 
* Copyright (c) 2025 Coalition of Freeware Developers
* -------------------------------------------------------
* orthographic_camera.h
* -------------------------------------------------------
* Created: 24/3/2025
* -------------------------------------------------------
*/

#pragma once

#include <SceneryEditorX/scene/camera.h>

// -------------------------------------------------------

namespace Camera
{
	class OrthographicCamera : public Camera
	{
	public:
	    OrthographicCamera(const std::string &name);
	
	    OrthographicCamera(const std::string &name,
	                       float left,
	                       float right,
	                       float bottom,
	                       float top,
	                       float near_plane,
	                       float far_plane);
	
	    virtual ~OrthographicCamera() = default;
	
	    void set_left(float new_left);
	    float get_left() const;
	
	    void set_right(float new_right);
	    float get_right() const;
	
	    void set_bottom(float new_bottom);
	    float get_bottom() const;
	
	    void set_top(float new_top);
	    float get_top() const;
	
	    void set_near_plane(float new_near_plane);
	    float get_near_plane() const;
	
	    void set_far_plane(float new_far_plane);
	    float get_far_plane() const;
	
	    virtual glm::mat4 get_projection() override;
	
	private:
	    float left{-1.0f};
	    float right{1.0f};
	    float bottom{-1.0f};
	    float top{1.0f};
	    float near_plane{0.0f};
	    float far_plane{1.0f};
	};

} // namespace Camera
