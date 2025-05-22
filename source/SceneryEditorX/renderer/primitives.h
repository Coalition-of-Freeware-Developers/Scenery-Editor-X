/**
* -------------------------------------------------------
* Scenery Editor X
* -------------------------------------------------------
* Copyright (c) 2025 Thomas Ray 
* Copyright (c) 2025 Coalition of Freeware Developers
* -------------------------------------------------------
* primitives.h
* -------------------------------------------------------
* Created: 20/5/2025
* -------------------------------------------------------
*/
#pragma once
#include <SceneryEditorX/scene/asset.h>

// -------------------------------------------------------

namespace SceneryEditorX
{
    /**
     * @enum PrimitiveType
     * @brief Types of primitive shapes that can be created
     */
    enum class PrimitiveType : uint8_t
    {
        Cube,       ///< 3D cube
        Plane,      ///< 2D quad
        Sphere,     ///< 3D sphere
        Cylinder,   ///< 3D cylinder
        Pyramid     ///< 3D pyramid
    };

    class Primitives
    {
    public:
        GLOBAL ObjectType CreateBox(const glm::vec3 &size);
        GLOBAL ObjectType CreateSphere(float radius);
        GLOBAL ObjectType CreateCylinder(float radius, float height);
        GLOBAL ObjectType CreatePlane(const glm::vec2 &size);
        GLOBAL ObjectType CreatePyramid(const glm::vec3 &baseSize, float height);
    };

}

// -------------------------------------------------------
