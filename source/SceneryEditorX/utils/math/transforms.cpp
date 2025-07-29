/**
* -------------------------------------------------------
* Scenery Editor X
* -------------------------------------------------------
* Copyright (c) 2025 Thomas Ray
* Copyright (c) 2025 Coalition of Freeware Developers
* -------------------------------------------------------
* transforms.cpp
* -------------------------------------------------------
* Created: 30/3/2025
* -------------------------------------------------------
*/
#include <SceneryEditorX/utils/math/quat.h>

/// -------------------------------------------------------

namespace SceneryEditorX::Utils
{

    /**
	 * @struct TransformComponent
	 * @brief Represents the spatial transformation properties of an object in the scene.
	 *
	 * This structure encapsulates the position, scale, and rotation of an object,
	 * storing the fundamental transformation values needed to place and orient objects
	 * in 3D space. It provides functionality to generate a transformation matrix
	 * that can be used in rendering and spatial calculations.
	 *
	 * The TransformComponent uses a quaternion for rotation to avoid gimbal lock
	 * and provide smooth interpolation between orientations. The scale component
	 * supports non-uniform scaling along each axis.
	 *
	 * @note This structure is primarily used internally for transformation calculations
	 * @note The quaternion rotation provides better mathematical properties than Euler angles
	 */
	struct TransformComponent
	{
	    /** @brief The position of the object in 3D space (translation vector) */
	    Vec3 translation{};

	    /** @brief The scale of the object along each axis, defaults to (1,1,1) meaning no scaling */
	    Vec3 scale{1.f, 1.f, 1.f};

	    /** @brief The rotation of the object as a quaternion, defaults to identity (no rotation) */
        Quat rotation{};

	    /**
	     * @brief Generates a 4x4 transformation matrix from the component's values.
	     *
	     * Computes a combined transformation matrix that represents the translation,
	     * rotation, and scaling defined in this component. The rotation is applied
	     * using quaternion-to-matrix conversion with optimized trigonometric calculations.
	     *
	     * The matrix is constructed manually using Euler angle calculations extracted
	     * from the quaternion components. The transformation order is Scale → Rotation → Translation.
	     *
	     * @return Mat4 The resulting 4x4 transformation matrix suitable for GPU rendering
	     *
	     * @note This implementation manually constructs the matrix for performance
	     * @note The rotation order is YXZ (Yaw-Pitch-Roll) based on quaternion components
	     * @warning There appears to be a potential bug in the matrix construction -
	     *          the quaternion is used as if it contains Euler angles rather than quaternion components
	     */
	    Mat4 mat4()
	    {
	        const float c3 = glm::cos(rotation.z);
	        const float s3 = glm::sin(rotation.z);
	        /// -------------------------------------------------------
	        const float c2 = glm::cos(rotation.x);
	        const float s2 = glm::sin(rotation.x);
	        /// -------------------------------------------------------
	        const float c1 = glm::cos(rotation.y);
	        const float s1 = glm::sin(rotation.y);
	        /// -------------------------------------------------------
	        return Mat4 {
	            {
	                scale.x * (c1 * c3 + s1 * s2 * s3),
	                scale.x * (c2 * s3),
	                scale.x * (c1 * s2 * s3 - c3 * s1),
	                0.0f,
	            },
	            {
	               scale.y * (c3 * s1 * s2 - c1 * s3),
	               scale.y * (c2 * c3),
	               scale.y * (c1 * c3 * s3 * s1 * s3),
	               0.0f,
	            },
	            {
	                scale.z * (c2 * s1),
	                scale.z * (-s2),
	                scale.z * (c1 * c2),
	                0.0f,
	            },
	            {translation.x, translation.y, translation.z, 1.0f}
	        };
	    }
	};

    /// -------------------------------------------------------

	/**
	 * @brief Scales a 3D vector to a desired length.
	 *
	 * This function takes a vector and scales it to the specified length while
	 * preserving its direction. If the input vector has zero length, it returns
	 * a zero vector to avoid division by zero.
	 *
	 * @param vector The input vector to be scaled.
	 * @param desiredLength The target length for the resulting vector.
	 *
	 * @return A new vector that has the same direction as the input vector but
	 *         with the specified length. Returns zero vector if input is zero vector.
	 *
	 * @note This function uses GLM's epsilon comparison to safely check for zero-length vectors.
	 */
	Vec3 Scale(const Vec3& vector, float desiredLength)
	{
		float mag = glm::length(vector);
		if (glm::epsilonEqual(mag, 0.0f, glm::epsilon<float>()))
			return Vec3(0.0f);

		return vector * desiredLength / mag;
	}

    /**
     * @brief Decomposes a transformation matrix into translation, rotation, and scale components.
     *
     * This function extracts the translation, rotation, and scale from a given transformation matrix.
     * It assumes the matrix is normalized and does not contain perspective transformations.
     * The decomposition is performed using matrix analysis techniques to separate the TRS components.
     *
     * @param transform The transformation matrix to decompose.
     * @param translation Output parameter that will receive the translation component.
     * @param rotation Output parameter that will receive the rotation component as a quaternion.
     * @param scale Output parameter that will receive the scale component.
     *
     * @return `true` if the decomposition was successful, `false` if the matrix is singular or invalid.
     *
     * @note This function uses GLM's decompose functionality for robust matrix decomposition.
     * @warning The input matrix should not contain perspective or skew transformations.
     */
    bool DecomposeTransform(const Mat4& transform, Vec3& translation, Quat& rotation, Vec3& scale)
    {
        // Use GLM's built-in decompose function for robust decomposition
        Vec3 skew;
        Vec4 perspective;
        glm::quat glmRotation;

        bool success = glm::decompose(transform, scale, glmRotation, translation, skew, perspective);

        if (success)
        {
            // Convert GLM quaternion to our Quat type
            rotation = Quat(glmRotation.w, glmRotation.x, glmRotation.y, glmRotation.z);
        }

        return success;
    }

    /// -------------------------------------------------------

	/**
	 * @brief Composes a transformation matrix from translation, rotation, and scale components.
	 *
	 * This function creates a 4x4 transformation matrix that combines translation,
	 * rotation, and scale into a single matrix. The transformation order follows
	 * the standard TRS (Translation-Rotation-Scale) matrix multiplication order:
	 * M = T * R * S, where transformations are applied in reverse order (scale first,
	 * then rotation, then translation).
	 *
	 * @param translation The translation vector (Vec3) representing the position offset.
	 * @param rotation The rotation quaternion (Quat) representing the orientation.
	 * @param scale The scale vector (Vec3) representing the scaling factors for each axis.
	 *
	 * @return Mat4 The resulting 4x4 transformation matrix that can be used to transform
	 *              points and vectors in 3D space.
	 *
	 * @note The transformation order is: Scale → Rotation → Translation.
	 * @note This function uses GLM's matrix transformation functions for accuracy and performance.
	 */
    Mat4 ComposeTransform(const Vec3& translation, const Quat& rotation, const Vec3& scale)
	{
		// Create GLM quaternion from our Quat type
		glm::quat glmRotation(rotation.w, rotation.x, rotation.y, rotation.z);

		// Compose the transformation matrix using GLM functions
		// Order: Translation * Rotation * Scale
		return glm::translate(Mat4(1.0f), translation) *
		       glm::mat4_cast(glmRotation) *
		       glm::scale(Mat4(1.0f), scale);
	}


}

/// -------------------------------------------------------
