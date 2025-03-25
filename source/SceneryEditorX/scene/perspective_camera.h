/**
* -------------------------------------------------------
* Scenery Editor X
* -------------------------------------------------------
* Copyright (c) 2025 Thomas Ray 
* Copyright (c) 2025 Coalition of Freeware Developers
* -------------------------------------------------------
* perspective_camera.h
* -------------------------------------------------------
* Created: 24/3/2025
* -------------------------------------------------------
*/

#pragma once

#include <SceneryEditorX/scene/camera.h>

// -------------------------------------------------------

namespace Camera
{
    class PerspectiveCamera : public Camera
    {
    public:
        PerspectiveCamera(const std::string &name);

        virtual ~PerspectiveCamera() = default;

        void set_aspect_ratio(float aspect_ratio);

        void set_field_of_view(float fov);

        float get_far_plane() const;

        void set_far_plane(float zfar);

        float get_near_plane() const;

        void set_near_plane(float znear);

        float get_aspect_ratio();

        float get_field_of_view();

        virtual glm::mat4 get_projection() override;

    private:
        /**
		 * @brief Screen size aspect ratio
		 */
        float aspect_ratio{1.0f};

        /**
		 * @brief Horizontal field of view in radians
		 */
        float fov{glm::radians(60.0f)};

        float far_plane{100.0};

        float near_plane{0.1f};
    };
} // namespace Camera

// -------------------------------------------------------
