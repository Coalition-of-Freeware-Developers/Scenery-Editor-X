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
//#include <SceneryEditorX/asset/asset.h>

/// -------------------------------------------------------

namespace SceneryEditorX
{

    /**
     * @enum PrimitiveType
     * @brief Types of primitive shapes that can be created
     */
    enum class PrimitiveType : uint8_t
    {
        Cube,       /// 3D cube
        Plane,      /// 2D quad
        Sphere,     /// 3D sphere
        Cylinder,   /// 3D cylinder
    };


    /**
     * @class Primitives
     * @brief Utility class for creating primitive 3D and 2D objects
     *
     * This class provides static methods to create various primitive shapes
     * that can be used as the foundation for more complex objects in the scene.
     * All methods return an ObjectType that can be further manipulated.
     */

    /// TODO: Add Primitive creation functionality 
    class Primitives
    {
    public:
        /**
         * @brief Creates a 3D box primitive
         * @param size The dimensions of the box (width, height, depth)
         * @return ObjectType representing the created box
         */
        //GLOBAL ObjectType CreateBox(const Vec3 &size);

        /**
         * @brief Creates a 3D sphere primitive
         * @param radius The radius of the sphere
         * @return ObjectType representing the created sphere
         */
        //GLOBAL ObjectType CreateSphere(float radius);

        /**
         * @brief Creates a 3D cylinder primitive
         * @param radius The radius of the cylinder's circular base
         * @param height The height of the cylinder
         * @return ObjectType representing the created cylinder
         */
        //GLOBAL ObjectType CreateCylinder(float radius, float height);

        /**
         * @brief Creates a 2D plane primitive
         * @param size The dimensions of the plane (width, height)
         * @return ObjectType representing the created plane
         */
        //GLOBAL ObjectType CreatePlane(const Vec2 &size);

    }; 

}

/// -------------------------------------------------------
