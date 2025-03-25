/**
* -------------------------------------------------------
* Scenery Editor X
* -------------------------------------------------------
* Copyright (c) 2025 Thomas Ray 
* Copyright (c) 2025 Coalition of Freeware Developers
* -------------------------------------------------------
* transform.h
* -------------------------------------------------------
* Created: 24/3/2025
* -------------------------------------------------------
*/

#pragma once

#include <glm/glm.hpp>

// -------------------------------------------------------

/*
struct TransformComponent
{
    glm::vec3 translation{};
    glm::vec3 rotation{};
    glm::vec3 scale{1.f, 1.f, 1.f};

    // Matrix corrsponds to Translate * Ry * Rx * Rz * Scale
    // Rotation correspond to Tait-bryan angles of Y(1), X(2), Z(3)
    // https://en.wikipedia.org/wiki/Euler_angles#Rotation_matrix

    glm::mat4 mat4()
    {
        const float c3 = glm::cos(rotation.z);
        const float s3 = glm::sin(rotation.z);
        const float c2 = glm::cos(rotation.x);
        const float s2 = glm::sin(rotation.x);
        const float c1 = glm::cos(rotation.y);
        const float s1 = glm::sin(rotation.y);
        return glm::mat4
		{
            scale.x * (c1 * c3 + s1 * s2 * s3),
            scale.x * (c2 * s3),
            scale.x * (c1 * s2 * s3 - c3 * s1),
            0.f,
        },
        {
            scale.y * (c3 * s1 * s2 - c1 * s3),
            scale.y * (c2 * c3),
            scale.y * (c1 * s3 + c3 * s1 * s2),
            0.f,
        },
        {
            scale.z * (c2 * s1),
            scale.z * (-s2),
            scale.z * (c1 * c2),
            0.f,
        },
        {translation.x, traslation.y, translation.z, 1.0f};
    };

};
*/
