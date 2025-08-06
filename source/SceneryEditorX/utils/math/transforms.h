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
#include <SceneryEditorX/core/base.hpp>

/// Forward declarations
namespace SceneryEditorX
{
    //class Quat;
    class Matrix4x4;
}

/// -------------------------------------------------------------

namespace SceneryEditorX::Utils
{
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
     * This function extracts the individual transformation components from a given 4x4
     * transformation matrix. It's particularly useful for editing transformations or
     * for interpolating between different transformation states. The decomposition
     * assumes the matrix represents a standard TRS (Translation-Rotation-Scale) transformation
     * without perspective distortion or skewing.
     *
     * The function uses robust mathematical techniques to separate the components:
     * - Translation is extracted from the 4th column
     * - Scale is calculated from the lengths of the first three columns
     * - Rotation is derived from the normalized 3x3 upper-left matrix
     *
     * @param transform The 4x4 transformation matrix to decompose. Should be a valid
     *                  TRS matrix without perspective or skew components.
     * @param translation Output parameter that receives the translation component (position).
     * @param rotation Output parameter that receives the rotation component as a quaternion.
     * @param scale Output parameter that receives the scale component for each axis.
     *
     * @return `true` if the decomposition was successful, `false` if the matrix is
     *         singular, invalid, or contains unsupported transformations.
     *
     * @note This function uses GLM's robust decomposition algorithm.
     * @warning The function may fail if the matrix contains perspective transformations,
     *          non-uniform skewing, or is nearly singular.
     *
     * @example
     * @code
     * Mat4 transformMatrix = GetObjectTransform();
     * Vec3 position, scale;
     * glm::quat rotation;
     *
     * if (DecomposeTransform(transformMatrix, position, rotation, scale))
     * {
     *     // Successfully decomposed - can now edit individual components
     *     position.y += 5.0f; // Move object up by 5 units
     *     Mat4 newTransform = ComposeTransform(position, rotation, scale);
     * }
     * @endcode
     */
    bool DecomposeTransform(const Mat4 &transform, Vec3 &translation, glm::quat &rotation, Vec3 &scale);

	/**
	 * @brief Composes a transformation matrix from translation, rotation, and scale components.
	 *
	 * This function creates a 4x4 transformation matrix by combining individual TRS
	 * (Translation-Rotation-Scale) components. The resulting matrix can be used to
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
	 * @note This function uses GLM's optimized matrix operations for performance.
	 * @note Non-uniform scaling (different scale values for X, Y, Z) is fully supported.
	 *
	 * @example
	 * @code
	 * Vec3 position(10.0f, 5.0f, 0.0f);           // 10 units right, 5 units up
	 * glm::quat rotation = glm::quat::EulerDegrees(0, 45, 0); // 45 degree rotation around Y-axis
	 * Vec3 scale(2.0f, 1.0f, 2.0f);               // Twice as wide and deep, same height
	 *
	 * Mat4 objectTransform = ComposeTransform(position, rotation, scale);
	 * // Use objectTransform to position and orient an object in the scene
	 * @endcode
	 */
    Mat4 ComposeTransform(const Vec3 &translation, const glm::quat &rotation, const Vec3 &scale);

}

/// -------------------------------------------------------------
