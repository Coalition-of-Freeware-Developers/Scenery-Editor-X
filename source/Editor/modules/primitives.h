/**
 * -------------------------------------------------------
 * Scenery Editor X
 * -------------------------------------------------------
 * Copyright (c) 2026 Thomas Ray 
 * Copyright (c) 2026 Coalition of Freeware Developers
 * -------------------------------------------------------
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 * -------------------------------------------------------
 * primitives.h
 * -------------------------------------------------------
 * Created: 20/5/2025
 * -------------------------------------------------------
 */
#pragma once
//#include "SceneryEditorX/asset/asset.h"

// -------------------------------------------------------

///TODO: Add Primitive creation functionality

/*
namespace SceneryEditorX
{

    /**
     * @enum PrimitiveType
     * @brief Types of primitive shapes that can be created
     #1#
    enum class PrimitiveType : uint8_t
    {
		None = 0,   /// No primitive type
        Cube,       /// 3D cube
        Plane,      /// 2D quad
        Sphere,     /// 3D sphere
        Cylinder,   /// 3D cylinder
		Cone,       /// 3D cone
        MaxEnum   /// Maximum enum value for validation
    };

    /**
     * @class Primitives
     * @brief Utility class for creating primitive 3D and 2D objects
     *
     * This class provides static methods to create various primitive shapes
     * that can be used as the foundation for more complex objects in the scene.
     * All methods return an ObjectType that can be further manipulated.
     #1#
    class Primitives
    {
    public:

		Primitives() = delete;
        ~Primitives() = delete;

        /**
         * @brief Creates a 3D box primitive
         *
         * @param size The dimensions of the box (width, height, depth)
         * @return ObjectType representing the created box
         *
         * @note - The box is centered at the origin (0,0,0).
         * @note - All dimensions must be positive and non-zero.
         *
         * @code
         * Vec3 size(1.0f, 2.0f, 3.0f);
         * ObjectType box = Primitives::CreateBox(size);
         * @endcode
         #1#
        static ObjectType CreateBox(const Vec3 &size);

        /**
         * @brief Creates a 3D sphere primitive
         *
         * @param radius The radius of the sphere
         * @return ObjectType representing the created sphere
         *
         * @note - The sphere is centered at the origin (0,0,0).
         * @note - The radius must be positive and non-zero.
         *
         * @code
         * float radius = 1.0f;
         * ObjectType sphere = Primitives::CreateSphere(radius);
         * @endcode
         #1#
        static ObjectType CreateSphere(float radius);

        /**
         * @brief Creates a 3D cylinder primitive
         *
         * @param radius The radius of the cylinder's circular base
         * @param height The height of the cylinder
         * @return ObjectType representing the created cylinder
         *
         * @note - The cylinder is aligned along the Y-axis, with its base centered at the origin.
         * @note - The radius must be positive and non-zero.
         * @note - The height must be positive and non-zero.
         *
         * @code
         * float radius = 1.0f;
         * float height = 2.0f;
         * ObjectType cylinder = Primitives::CreateCylinder(radius, height);
         * @endcode
         #1#
        static ObjectType CreateCylinder(float radius, float height);

        /**
         * @brief Creates a 2D plane primitive
         * @param size The dimensions of the plane (width, height)
         * @return ObjectType representing the created plane
         #1#
        static ObjectType CreatePlane(const Vec2 &size);

        /**
         * @brief Shows text-based input interface for primitive creation
         *
         * This method provides a simpler text-only interface for primitive creation:
         * - Text input fields for dimensions
         * - Primitive type selection
         * - Validation of input values
         *
         * @param primitiveType Current selected primitive type
         * @param dimensions Current dimension values
         * @return true if user confirmed creation, false otherwise
         *
         * @note - This is a placeholder method to be implemented when the UI system is ready.
         * @note - Input validation ensures dimensions are positive and non-zero.
         * @note - Returns true only if the user confirms creation with valid inputs.
         *
         * @code
         * PrimitiveType type = PrimitiveType::Cube;
         * Vec3 size(1.0f, 1.0f, 1.0f);
         * if (Primitives::ShowTextInputInterface(type, size))
         * {
         *    ObjectType primitive = Primitives::CreateBox(size);
         * }
         * @endcode
         #1#
        static bool ShowTextInputInterface(PrimitiveType& primitiveType, Vec3& dimensions);

    private:
        /**
         * @brief Validates primitive dimensions based on type
         *
         * @param type The primitive type to validate for
         * @param size The dimensions to validate
         * @return true if dimensions are valid, false otherwise
         *
         * @note - Cube requires all dimensions > 0
         * @note - Sphere requires radius (x) > 0
         * @note - Cylinder requires radius (x) > 0 and height (y) > 0
         * @note - Plane requires width (x) > 0 and height (y) > 0
         * @note - None type is always invalid
         * @note - This method is used internally before creating primitives
         *
         * @code
         * Vec3 size(1.0f, 2.0f, 3.0f);
         * bool valid = Primitives::ValidatePrimitiveDimensions(PrimitiveType::Cube, size);
         * @endcode
         *
         *
         #1#
        static bool ValidatePrimitiveDimensions(PrimitiveType type, const Vec3& size);
    };

}
*/

// -------------------------------------------------------
