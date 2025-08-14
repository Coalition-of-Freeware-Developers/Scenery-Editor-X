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
#include <cmath>
#include <SceneryEditorX/logging/asserts.h>
#include <SceneryEditorX/utils/math/math_utils.h>
#include <SceneryEditorX/utils/math/quat.h>
#include <SceneryEditorX/utils/math/scale.h>
#include <SceneryEditorX/utils/math/transforms.h>

/// -------------------------------------------------------

namespace SceneryEditorX
{

	/**
	 * @brief Decomposes a transformation matrix into translation, rotation, and scale components.
	 *
	 * Extracts translation (fourth column), orthonormalizes the upper-left 3x3 to recover
	 * scale (length of each basis vector) and rotation (quaternion from orthonormalized axes).
	 * Assumes no perspective or shear. Matrix must be affine and its bottom row (0,0,0,1).
	 *
	 * @param transform   Input affine transform matrix.
	 * @param translation Output translation.
	 * @param rotation    Output rotation quaternion.
	 * @param scale       Output non-uniform scale.
	 * @return true on success (non‑singular), false otherwise.
	 *
	 * @warning Perspective/shear not supported. Caller must ensure an affine matrix.
	 */
    bool Transforms::Decompose(const Mat4 &transform, Vec3 &translation, Quat &rotation, Vec3 &scale)
    {
        using T = float;
        Mat4 LocalMatrix(transform);

		if (::SceneryEditorX::epsilonEqual(LocalMatrix[3][3], static_cast<T>(0), ::SceneryEditorX::epsilon<T>()))
            return false;

        /// Assume matrix is already normalized
		SEDX_CORE_ASSERT(::SceneryEditorX::epsilonEqual(LocalMatrix[3][3], static_cast<T>(1), static_cast<T>(0.00001)));

	    /// Ignore perspective
        SEDX_CORE_ASSERT(::SceneryEditorX::epsilonEqual(LocalMatrix[0][3], static_cast<T>(0),
			::SceneryEditorX::epsilon<T>()) && ::SceneryEditorX::epsilonEqual(LocalMatrix[1][3], static_cast<T>(0),
			::SceneryEditorX::epsilon<T>()) && ::SceneryEditorX::epsilonEqual(LocalMatrix[2][3], static_cast<T>(0),
			::SceneryEditorX::epsilon<T>()));

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

    	#if SEDX_DEBUG
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

			float q[4]{}; /// [x,y,z,w]
			q[i] = static_cast<T>(0.5) * root;
			root = static_cast<T>(0.5) / root;
			q[j] = root * (Row[i][j] + Row[j][i]);
			q[k] = root * (Row[i][k] + Row[k][i]);
			qw = root * (Row[j][k] - Row[k][j]);
			qx = q[0]; qy = q[1]; qz = q[2];
		} /// End if <= 0

		rotation = Quat(qw, qx, qy, qz);
		return true;
    }

    /// -------------------------------------------------------

	/**
	 * @brief Composes a transformation matrix from translation, rotation, and scale components.
	 *
	 * This function creates a 4x4 transformation matrix that combines translation,
	 * rotation, and scale into a single matrix. The transformation order follows
	 * the standard TRS (Translate-Rotation-Scale) matrix multiplication order:
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
	 * @note - The transformation order is: Scale → Rotation → Translation.
	 * @note - This function uses GLM's matrix transformation functions for accuracy and performance.
	 */
    Mat4 Transforms::Compose(const Vec3 &translation, const Quat &rotation, const Vec3 &scale)
	{
		/// Compose using our Mat4 and Quat
		const Mat4 T = Mat4::Translate(translation);
		const Mat4 R = rotation.ToMatrix();
		const Mat4 S = Mat4::Scale(scale);
		return T * R * S;
	}

}

/// -------------------------------------------------------
