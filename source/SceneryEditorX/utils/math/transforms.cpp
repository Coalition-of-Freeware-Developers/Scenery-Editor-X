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
#include <SceneryEditorX/core/base.hpp>
#include <SceneryEditorX/utils/math/quat.h>
#include <SceneryEditorX/utils/math/vector.h>
#include <SceneryEditorX/utils/math/math_utils.h>
#include <SceneryEditorX/utils/math/epsilon.h>
#include <SceneryEditorX/logging/asserts.h>
#include <cmath>

/// -------------------------------------------------------

namespace SceneryEditorX::Utils
{

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
		// Compute magnitude without relying on GLM
		const float mag = std::sqrt(vector.x * vector.x + vector.y * vector.y + vector.z * vector.z);
		if (::SceneryEditorX::Utils::epsilonEqual(mag, 0.0f, ::SceneryEditorX::Utils::epsilon<float>()))
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
	bool DecomposeTransform(const Mat4 &transform, Vec3 &translation, Quat &rotation, Vec3 &scale)
    {
        using T = float;
        Mat4 LocalMatrix(transform);

		if (::SceneryEditorX::Utils::epsilonEqual(LocalMatrix[3][3], static_cast<T>(0), ::SceneryEditorX::Utils::epsilon<T>()))
            return false;

        /// Assume matrix is already normalized
		SEDX_CORE_ASSERT(::SceneryEditorX::Utils::epsilonEqual(LocalMatrix[3][3], static_cast<T>(1), static_cast<T>(0.00001)));

	    /// Ignore perspective
        SEDX_CORE_ASSERT(::SceneryEditorX::Utils::epsilonEqual(LocalMatrix[0][3], static_cast<T>(0),
			::SceneryEditorX::Utils::epsilon<T>()) && ::SceneryEditorX::Utils::epsilonEqual(LocalMatrix[1][3], static_cast<T>(0),
			::SceneryEditorX::Utils::epsilon<T>()) && ::SceneryEditorX::Utils::epsilonEqual(LocalMatrix[2][3], static_cast<T>(0),
			::SceneryEditorX::Utils::epsilon<T>()));

        /// perspectiveMatrix is used to solve for perspective, but it also provides
        /// an easy way to test for singularity of the upper 3x3 component.

	    /// Next take care of translation (easy).
        translation = Vec3(LocalMatrix[3]);
        LocalMatrix[3] = Vec4(0, 0, 0, LocalMatrix[3].w);

		Vec3 Row[3];

		/// Get scale and shear.
		for (int i = 0; i < 3; ++i)
			for (int j = 0; j < 3; ++j)
				Row[i][j] = LocalMatrix[i][j];

		/// Compute X scale factor and normalize first row.
		scale.x = std::sqrt(Row[0].x*Row[0].x + Row[0].y*Row[0].y + Row[0].z*Row[0].z);
		Row[0] = Scale(Row[0], static_cast<T>(1));

		/// Compute Y scale and normalize 2nd row.
		scale.y = std::sqrt(Row[1].x*Row[1].x + Row[1].y*Row[1].y + Row[1].z*Row[1].z);
		Row[1] = Scale(Row[1], static_cast<T>(1));

		/// Get Z scale and normalize 3rd row.
		scale.z = std::sqrt(Row[2].x*Row[2].x + Row[2].y*Row[2].y + Row[2].z*Row[2].z);
		Row[2] = Scale(Row[2], static_cast<T>(1));

    	#if _DEBUG
		/// At this point, the matrix (in rows[]) is orthonormal.
		/// Check for a coordinate system flip.  If the determinant
		/// is -1, then negate the matrix and the scaling factors.
		Vec3 Pdum3 = Cross(Row[1], Row[2]);
		SEDX_CORE_ASSERT(Dot(Row[0], Pdum3) >= static_cast<T>(0));
    	#endif

        T root;
        float qx = 0.0f, qy = 0.0f, qz = 0.0f, qw = 1.0f;
		if (T trace = Row[0].x + Row[1].y + Row[2].z; trace > static_cast<T>(0))
		{
			root = sqrt(trace + static_cast<T>(1));
			qw = static_cast<T>(0.5) * root;
			root = static_cast<T>(0.5) / root;
			qx = root * (Row[1].z - Row[2].y);
			qy = root * (Row[2].x - Row[0].z);
			qz = root * (Row[0].y - Row[1].x);
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

			float q[4]{}; // [x,y,z,w]
			q[i] = static_cast<T>(0.5) * root;
			root = static_cast<T>(0.5) / root;
			q[j] = root * (Row[i][j] + Row[j][i]);
			q[k] = root * (Row[i][k] + Row[k][i]);
			qw = root * (Row[j][k] - Row[k][j]);
			qx = q[0]; qy = q[1]; qz = q[2];
		} /// End if <= 0

		rotation = Quat(qw, qx, qy, qz);
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
    Mat4 ComposeTransform(const Vec3 &translation, const Quat &rotation, const Vec3 &scale)
	{
		// Compose using our Matrix4x4 and Quat
		const Mat4 T = Matrix4x4::Translation(translation);
		const Mat4 R = rotation.ToMatrix();
		const Mat4 S = Matrix4x4::Scale(scale);
		return T * R * S;
	}

}

/// -------------------------------------------------------
