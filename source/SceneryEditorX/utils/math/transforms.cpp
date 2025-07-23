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
#include <SceneryEditorX/utils/math/transforms.h>

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
	 */
	struct TransformComponent
	{
	    /** @brief The position of the object in 3D space */
	    Vec3 translation{};

	    /** @brief The scale of the object along each axis, defaults to (1,1,1) */
	    Vec3 scale{1.f, 1.f, 1.f};

	    /** @brief The rotation of the object in radians around each axis */
        Quat rotation{};

	    /**
	     * @brief Generates a 4x4 transformation matrix from the component's values.
	     *
	     * Computes a combined transformation matrix that represents the translation,
	     * rotation, and scaling defined in this component. The rotation follows the
	     * Euler angle convention with the order: Y (rotation.y), X (rotation.x),
	     * Z (rotation.z).
	     *
	     * @return Mat4 The resulting 4x4 transformation matrix
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


	Vec3 Scale(const Vec3& v, float desiredLength)
	{
		float mag = glm::length(v);
		if (glm::epsilonEqual(mag, 0.0f, glm::epsilon<float>()))
			return Vec3(0.0f);

		return v * desiredLength / mag;
	}

	bool DecomposeTransform(const Mat4& transform, Vec3& translation, Quat& rotation, Vec3& scale)
	{
		using T = float;

	    Mat4 LocalMatrix(transform);

		if (glm::epsilonEqual(LocalMatrix[3][3], static_cast<T>(0), glm::epsilon<T>()))
			return false;

		// Assume matrix is already normalized
        SEDX_CORE_ASSERT(glm::epsilonEqual(LocalMatrix[3][3], static_cast<T>(1), static_cast<T>(0.00001)));
		//for (length_t i = 0; i < 4; ++i)
		//	for (length_t j = 0; j < 4; ++j)
		//		LocalMatrix[i][j] /= LocalMatrix[3][3];

		// Ignore perspective
        SEDX_CORE_ASSERT(glm::epsilonEqual(LocalMatrix[0][3], static_cast<T>(0),
			glm::epsilon<T>()) && glm::epsilonEqual(LocalMatrix[1][3],
			static_cast<T>(0), glm::epsilon<T>()) && glm::epsilonEqual(LocalMatrix[2][3],
			static_cast<T>(0), glm::epsilon<T>())
		);
		//// perspectiveMatrix is used to solve for perspective, but it also provides
		//// an easy way to test for singularity of the upper 3x3 component.
		//mat<4, 4, T, Q> PerspectiveMatrix(LocalMatrix);
		//
		//for (length_t i = 0; i < 3; i++)
		//	PerspectiveMatrix[i][3] = static_cast<T>(0);
		//PerspectiveMatrix[3][3] = static_cast<T>(1);
		//
		///// TODO: Fixme!
		//if (epsilonEqual(determinant(PerspectiveMatrix), static_cast<T>(0), epsilon<T>()))
		//	return false;
		//
		//// First, isolate perspective.  This is the messiest.
		//if (
		//	epsilonNotEqual(LocalMatrix[0][3], static_cast<T>(0), epsilon<T>()) ||
		//	epsilonNotEqual(LocalMatrix[1][3], static_cast<T>(0), epsilon<T>()) ||
		//	epsilonNotEqual(LocalMatrix[2][3], static_cast<T>(0), epsilon<T>()))
		//{
		//	// rightHandSide is the right hand side of the equation.
		//	vec<4, T, Q> RightHandSide;
		//	RightHandSide[0] = LocalMatrix[0][3];
		//	RightHandSide[1] = LocalMatrix[1][3];
		//	RightHandSide[2] = LocalMatrix[2][3];
		//	RightHandSide[3] = LocalMatrix[3][3];
		//
		//	// Solve the equation by inverting PerspectiveMatrix and multiplying
		//	// rightHandSide by the inverse.  (This is the easiest way, not
		//	// necessarily the best.)
		//	mat<4, 4, T, Q> InversePerspectiveMatrix = glm::inverse(PerspectiveMatrix);//   inverse(PerspectiveMatrix, inversePerspectiveMatrix);
		//	mat<4, 4, T, Q> TransposedInversePerspectiveMatrix = glm::transpose(InversePerspectiveMatrix);//   transposeMatrix4(inversePerspectiveMatrix, transposedInversePerspectiveMatrix);
		//
		//	Perspective = TransposedInversePerspectiveMatrix * RightHandSide;
		//	//  v4MulPointByMatrix(rightHandSide, transposedInversePerspectiveMatrix, perspectivePoint);
		//
		//	// Clear the perspective partition
		//	LocalMatrix[0][3] = LocalMatrix[1][3] = LocalMatrix[2][3] = static_cast<T>(0);
		//	LocalMatrix[3][3] = static_cast<T>(1);
		//}
		//else
		//{
		//	// No perspective.
		//	Perspective = vec<4, T, Q>(0, 0, 0, 1);
		//}

		// Next take care of translation (easy).
		translation = Vec3(LocalMatrix[3]);
		LocalMatrix[3] = Vec4(0, 0, 0, LocalMatrix[3].w);

		Vec3 Row[3];

		// Now get scale and shear.
		for (glm::length_t i = 0; i < 3; ++i)
			for (glm::length_t j = 0; j < 3; ++j)
				Row[i][j] = LocalMatrix[i][j];

		// Compute X scale factor and normalize first row.
		scale.x = length(Row[0]);
		Row[0] = Scale(Row[0], static_cast<T>(1));

		// Ignore shear
		//// Compute XY shear factor and make 2nd row orthogonal to 1st.
		//Skew.z = dot(Row[0], Row[1]);
		//Row[1] = detail::combine(Row[1], Row[0], static_cast<T>(1), -Skew.z);

		// Now, compute Y scale and normalize 2nd row.
		scale.y = length(Row[1]);
		Row[1] = Scale(Row[1], static_cast<T>(1));
		//Skew.z /= Scale.y;

		//// Compute XZ and YZ shears, orthogonalize 3rd row.
		//Skew.y = glm::dot(Row[0], Row[2]);
		//Row[2] = detail::combine(Row[2], Row[0], static_cast<T>(1), -Skew.y);
		//Skew.x = glm::dot(Row[1], Row[2]);
		//Row[2] = detail::combine(Row[2], Row[1], static_cast<T>(1), -Skew.x);

		// Next, get Z scale and normalize 3rd row.
		scale.z = length(Row[2]);
		Row[2] = Scale(Row[2], static_cast<T>(1));
		//Skew.y /= Scale.z;
		//Skew.x /= Scale.z;

#if _DEBUG
		// At this point, the matrix (in rows[]) is orthonormal.
		// Check for a coordinate system flip.  If the determinant
		// is -1, then negate the matrix and the scaling factors.
		Vec3 Pdum3 = cross(Row[1], Row[2]); // v3Cross(row[1], row[2], Pdum3);
		SEDX_CORE_ASSERT(dot(Row[0], Pdum3) >= static_cast<T>(0));
#endif
		//if (dot(Row[0], Pdum3) < 0)
		//{
		//	for (length_t i = 0; i < 3; i++)
		//	{
		//		scale[i] *= static_cast<T>(-1);
		//		Row[i] *= static_cast<T>(-1);
		//	}
		//}

		// Rotation as XYZ Euler angles
		//rotation.y = asin(-Row[0][2]);
		//if (cos(rotation.y) != 0.f)
		//{
		//	rotation.x = atan2(Row[1][2], Row[2][2]);
		//	rotation.z = atan2(Row[0][1], Row[0][0]);
		//}
		//else
		//{
		//	rotation.x = atan2(-Row[2][0], Row[1][1]);
		//	rotation.z = 0;
		//}

		// Rotation as quaternion
        T root;
		if (T trace = Row[0].x + Row[1].y + Row[2].z; trace > static_cast<T>(0))
		{
			root = sqrt(trace + static_cast<T>(1));
			rotation.w = static_cast<T>(0.5) * root;
			root = static_cast<T>(0.5) / root;
			rotation.x = root * (Row[1].z - Row[2].y);
			rotation.y = root * (Row[2].x - Row[0].z);
			rotation.z = root * (Row[0].y - Row[1].x);
		} // End if > 0
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
		} // End if <= 0

		return true;
	}


    static Mat4 ComposeTransform(const Vec3& translation, const Quat& rotation, const Vec3& scale)
	{
		return glm::translate(Mat4(1.0f), translation) * glm::mat4_cast(rotation) * glm::scale(Mat4(1.0f), scale);
	}


}

/// -------------------------------------------------------
