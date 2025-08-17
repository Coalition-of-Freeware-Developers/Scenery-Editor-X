/**
* -------------------------------------------------------
* Scenery Editor X
* -------------------------------------------------------
* Copyright (c) 2025 Thomas Ray
* Copyright (c) 2025 Coalition of Freeware Developers
* -------------------------------------------------------
* transforms.h
* -------------------------------------------------------
* Created: 13/7/2025
* -------------------------------------------------------
*/
#pragma once
#include <Math/math_config.h>
//#include <SceneryEditorX/core/base.hpp>
#include <Math/includes/mat4.h>
#include <Math/includes/vector.h>
#include <Math/includes/quat.h>

/// -------------------------------------------------------------

namespace SceneryEditorX
{

	class XMATH_API Transforms
    {
    public:

        Transforms() = delete;
        ~Transforms() = delete;

		/**
		 * @enum CoordSpace
		 * @brief Defines the coordinate space for transformation operations.
		 *
		 * This enumeration represents different coordinate spaces that can be used
		 * when performing transformations on objects in the scene. Each coordinate
		 * space provides a different frame of reference for positioning and orienting
		 * objects in 3D space.
		 *
		 * @value WorldSpace Global coordinate space of the scene. Transformations in this
		 *                   space are absolute positions in the world.
		 * @value LocalSpace Coordinate space relative to the parent object. Transformations
		 *                   are relative to the parent's coordinate system.
		 * @value ObjectSpace Coordinate space relative to the object itself. Also known as
		 *                    model space, this is the object's original coordinate system.
		 * @value CameraSpace Coordinate space relative to the camera's perspective. Also
		 *                    known as view space, transformations are relative to the camera.
		 */
		enum class CoordSpace : uint8_t
		{
			WorldSpace,   ///< Global world coordinate space
		    LocalSpace,   ///< Parent-relative coordinate space
		    ObjectSpace,  ///< Object-local coordinate space (model space)
			CameraSpace   ///< Camera-relative coordinate space (view space)
		};

		/// -------------------------------------------------------------

		/**
		 * @brief Decomposes a transformation matrix into translation, rotation, and scale components.
		 *
		 * Extracts TRS from a 4x4 affine matrix (no perspective/skew). Rotation is returned
		 * as the engine's native quaternion type (Quat) and the matrix composition is assumed
		 * to be M = T * R * S.
		 *
		 * @param transform The 4x4 transformation matrix to decompose.
		 * @param translation Out: translation vector (position).
		 * @param rotation Out: rotation as a native quaternion (Quat).
		 * @param scale Out: non-uniform scale per axis.
		 * @return true if decomposition succeeded; false for invalid/singular inputs.
		 */
        static bool Decompose(const Mat4 &transform, Vec3 &translation, Quat &rotation, Vec3 &scale);

		/**
		 * @brief Composes a transformation matrix from translation, rotation, and scale components.
		 *
		 * This function creates a 4x4 transformation matrix by combining individual TRS
		 * (Translate-Rotation-Scale) components. The resulting matrix can be used to
		 * transform vertices, vectors, and other geometric data in 3D space. The transformation
		 * order follows standard computer graphics conventions.
		 *
		 * The composition follows the mathematical order: M = T * R * S
		 * Where transformations are applied to vertices in the reverse order:
		 * 1. Scale is applied first (to the original object space)
		 * 2. Rotation is applied second (to the scaled object)
		 * 3. Translation is applied last (to position the rotated and scaled object)
		 *
		 * @param translation The translation vector representing the position offset in world space.
		 *                    Each component corresponds to movement along X, Y, and Z axes.
		 * @param rotation The rotation quaternion representing the orientation. Quaternions provide
		 *                 smooth interpolation and avoid gimbal lock issues compared to Euler angles.
		 * @param scale The scale vector representing scaling factors for each axis. A value of 1.0
		 *              means no scaling, values > 1.0 increase size, values < 1.0 decrease size.
		 *              Negative values can be used for mirroring/reflection.
		 *
		 * @return Mat4 The resulting 4x4 transformation matrix ready for use in rendering
		 *              pipelines, physics calculations, or further matrix operations.
		 *
		 * @note - This function uses GLM's optimized matrix operations for performance.
		 * @note - Non-uniform scaling (different scale values for X, Y, Z) is fully supported.
		 *
		 * @code
		 * Vec3 position(10.0f, 5.0f, 0.0f);           // 10 units right, 5 units up
		 * Quat rotation = Quat::EulerDegrees(0, 45, 0); // 45-degree  rotation around Y-axis
		 * Vec3 scale(2.0f, 1.0f, 2.0f);               // Twice as wide and deep, same height
		 *
		 * Mat4 objectTransform = ComposeTransform(position, rotation, scale);
		 * // Use objectTransform to position and orient an object in the scene
		 * @endcode
		 */
        static Mat4 Compose(const Vec3 &translation, const Quat &rotation, const Vec3 &scale);


    };


}

/// -------------------------------------------------------------
