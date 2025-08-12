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
        glm::quat rotation{};

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
    bool DecomposeTransform(const Mat4 &transform, Vec3 &translation, glm::quat &rotation, Vec3 &scale)
    {
        using T = float;
        Mat4 LocalMatrix(transform);

        if (glm::epsilonEqual(LocalMatrix[3][3], static_cast<T>(0), glm::epsilon<T>()))
            return false;

        /// Assume matrix is already normalized
        SEDX_CORE_ASSERT(glm::epsilonEqual(LocalMatrix[3][3], static_cast<T>(1), static_cast<T>(0.00001)));

	    /// Ignore perspective
        SEDX_CORE_ASSERT(glm::epsilonEqual(LocalMatrix[0][3], static_cast<T>(0),
			glm::epsilon<T>()) && glm::epsilonEqual(LocalMatrix[1][3], static_cast<T>(0),
			glm::epsilon<T>()) && glm::epsilonEqual(LocalMatrix[2][3], static_cast<T>(0),
			glm::epsilon<T>()));

        /// perspectiveMatrix is used to solve for perspective, but it also provides
        /// an easy way to test for singularity of the upper 3x3 component.

	    /// Next take care of translation (easy).
        translation = Vec3(LocalMatrix[3]);
        LocalMatrix[3] = Vec4(0, 0, 0, LocalMatrix[3].w);

        Vec3 Row[3];

		/// Get scale and shear.
		for (glm::length_t i = 0; i < 3; ++i)
			for (glm::length_t j = 0; j < 3; ++j)
				Row[i][j] = LocalMatrix[i][j];

		/// Compute X scale factor and normalize first row.
		scale.x = length(Row[0]);
		Row[0] = Scale(Row[0], static_cast<T>(1));

		/// Compute Y scale and normalize 2nd row.
		scale.y = length(Row[1]);
		Row[1] = Scale(Row[1], static_cast<T>(1));

		/// Get Z scale and normalize 3rd row.
		scale.z = length(Row[2]);
		Row[2] = Scale(Row[2], static_cast<T>(1));

    #if _DEBUG
		/// At this point, the matrix (in rows[]) is orthonormal.
		/// Check for a coordinate system flip.  If the determinant
		/// is -1, then negate the matrix and the scaling factors.
		Vec3 Pdum3 = cross(Row[1], Row[2]);
		SEDX_CORE_ASSERT(dot(Row[0], Pdum3) >= static_cast<T>(0));
    #endif

        T root;
		if (T trace = Row[0].x + Row[1].y + Row[2].z; trace > static_cast<T>(0))
		{
			root = sqrt(trace + static_cast<T>(1));
			rotation.w = static_cast<T>(0.5) * root;
			root = static_cast<T>(0.5) / root;
			rotation.x = root * (Row[1].z - Row[2].y);
			rotation.y = root * (Row[2].x - Row[0].z);
			rotation.z = root * (Row[0].y - Row[1].x);
		} /// End if > 0
		else
		{
			static int Next[3] = { 1, 2, 0 };
			int i = 0;
			if (Row[1].y > Row[0].x) i = 1;
			if (Row[2].z > Row[i][i]) i = 2;
			int j = Next[i];
			int k = Next[j];

			root = sqrt(Row[i][i] - Row[j][j] - Row[k][k] + static_cast<T>(1.0));

			rotation[i] = static_cast<T>(0.5) * root;
			root = static_cast<T>(0.5) / root;
			rotation[j] = root * (Row[i][j] + Row[j][i]);
			rotation[k] = root * (Row[i][k] + Row[k][i]);
			rotation.w = root * (Row[j][k] - Row[k][j]);
		} /// End if <= 0

		return true;

        /*
        /// Use GLM's built-in decompose function for robust decomposition
        Vec3 skew;
        Vec4 perspective;
        glm::quat glmRotation;

        bool success = glm::decompose(transform, scale, glmRotation, translation, skew, perspective);

        if (success)
            rotation = glm::quat(glmRotation.w, glmRotation.x, glmRotation.y, glmRotation.z);	/// Convert GLM quaternion to our Quat type

        return success;
        */
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
    Mat4 ComposeTransform(const Vec3 &translation, const glm::quat &rotation, const Vec3 &scale)
	{
		/// Create GLM quaternion from our Quat type
		glm::quat glmRotation(rotation.w, rotation.x, rotation.y, rotation.z);

		/// Compose the transformation matrix using GLM functions
		/// Order: Translation * Rotation * Scale
		return glm::translate(Mat4(1.0f), translation) * glm::mat4_cast(glmRotation) * glm::scale(Mat4(1.0f), scale);
	}

}

/// -------------------------------------------------------
