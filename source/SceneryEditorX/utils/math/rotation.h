/**
* -------------------------------------------------------
* Scenery Editor X
* -------------------------------------------------------
* Copyright (c) 2025 Thomas Ray
* Copyright (c) 2025 Coalition of Freeware Developers
* -------------------------------------------------------
* rotation.h
* -------------------------------------------------------
* Created: 13/8/2025
* -------------------------------------------------------
*/
#pragma once
#include <cmath>
#include <SceneryEditorX/utils/math/constants.h>
#include <SceneryEditorX/utils/math/matrix.h>
#include <SceneryEditorX/utils/math/vector.h>

/// -------------------------------------------------------

namespace SceneryEditorX
{

	/**
	 * @brief Rotate around the Z-axis by a given angle in radians.
	 *
	 * @param degrees Angle in degrees to rotate around the Z-axis.
	 * @return Mat4 Rotation matrix for the Z-axis rotation.
	 *
	 * @note - This function creates a rotation matrix that rotates points around the Z-axis by the specified angle in degrees.
	 * @note - The rotation is right-handed, meaning positive angles rotate counter-clockwise when looking from the positive Z-axis towards the origin.
	 *
	 * @code
	 * Mat4 rotationMatrix = RotateZDegrees(45.0f); // Rotate 45 degrees around the X-axis
	 * @endcode
	 */
	inline Mat4 RotateZDegrees(float degrees)
	{
		const float r = degrees * DEG_TO_RAD;
		const float c = std::cos(r);
		const float s = std::sin(r);
		return Mat4({
			{ c, -s, 0.0f, 0.0f },
			{ s,  c, 0.0f, 0.0f },
			{0.0f, 0.0f, 1.0f, 0.0f},
			{0.0f, 0.0f, 0.0f, 1.0f}
		});
	}

    /**
     * @brief Rotate around the Z-axis by a given angle in radians, applied to a matrix.
     *
     * @param m The matrix to which the rotation will be applied.
     * @param degrees Angle in degrees to rotate around the Z-axis.
     * @return Mat4 The resulting matrix after applying the Z-axis rotation.
     *
     * @note - This function multiplies the input matrix by a rotation matrix that rotates points around the Z-axis by the specified angle in degrees.
     * @note - The rotation is right-handed, meaning positive angles rotate counter-clockwise when looking from the positive Z-axis towards the origin.
     *
     * @code
     * Mat4 originalMatrix = ...; // Some existing matrix
     * Mat4 rotatedMatrix = RotateZDegrees(originalMatrix, 45.0f); // Rotate the matrix 45 degrees around the Z-axis
     * @endcode
     */
    inline Mat4 RotateZDegrees(const Mat4& m, float degrees)
	{
		return m * RotateZDegrees(degrees);
	}

    ///////////////////////////////////////////////////////////
    //			Euler rotations (YXZ order) in radians	     //
    ///////////////////////////////////////////////////////////

    /**
     * @brief Create a rotation matrix from Euler angles in radians.
     *
     * This function constructs a rotation matrix based on the provided Euler angles in radians.
     * The rotation is applied in the Y-X-Z order (YXZ), which is a common convention for 3D rotations.
     *
     * @param euler The Euler angles in radians, represented as a Vec3 where:
     *             - euler.x is the rotation around the X-axis,
     *             - euler.y is the rotation around the Y-axis,
     *             - euler.z is the rotation around the Z-axis.
     * @return Mat4 A 4x4 rotation matrix representing the combined rotation.
     *
     * @note - The resulting matrix can be used to transform points or vectors in 3D space by applying the rotation defined by the Euler angles.
     * @note - The angles are assumed to be in radians, so if you have angles in degrees, you should convert them to radians before calling this function.
     *
     * @code
     * Vec3 eulerRadians(0.0f, 1.57f, 0.0f); // Example Euler angles in radians
     * Mat4 rotationMatrix = RotateEulerRadians(eulerRadians); // Create rotation matrix
     * @endcode
     */
    inline Mat4 RotateEulerRadians(const Vec3& euler)
	{
		const float cx = std::cos(euler.x), sx = std::sin(euler.x);
		const float cy = std::cos(euler.y), sy = std::sin(euler.y);
		const float cz = std::cos(euler.z), sz = std::sin(euler.z);

		/// Y * X * Z order (YXZ)
		const Mat4 Ry({ { cy, 0.0f, sy, 0.0f }, { 0.0f, 1.0f, 0.0f, 0.0f }, { -sy, 0.0f, cy, 0.0f }, { 0.0f, 0.0f, 0.0f, 1.0f } });
		const Mat4 Rx({ { 1.0f, 0.0f, 0.0f, 0.0f }, { 0.0f, cx, -sx, 0.0f }, { 0.0f, sx, cx, 0.0f }, { 0.0f, 0.0f, 0.0f, 1.0f } });
		const Mat4 Rz({ { cz, -sz, 0.0f, 0.0f }, { sz, cz, 0.0f, 0.0f }, { 0.0f, 0.0f, 1.0f, 0.0f }, { 0.0f, 0.0f, 0.0f, 1.0f } });
		return Ry * Rx * Rz;
	}

    /**
     * @brief Create a rotation matrix from Euler angles in radians, applied to a matrix.
     *
     * This function constructs a rotation matrix based on the provided Euler angles in radians and applies it to an existing matrix.
     *
     * @param eulerDeg The Euler angles in radians, represented as a Vec3 where:
     *            - euler.x is the rotation around the X-axis,
     *            - euler.y is the rotation around the Y-axis,
     *            - euler.z is the rotation around the Z-axis.
     * @return Mat4 A 4x4 rotation matrix representing the combined rotation applied to the input matrix.
     *
     * @note - The resulting matrix can be used to transform points or vectors in 3D space by applying the rotation defined by the Euler angles.
     * @note - The angles are assumed to be in radians, so if you have angles in degrees, you should convert them to radians before calling this function.
     *
     * @code
     * Vec3 eulerRadians(0.0f, 1.57f, 0.0f); // Example Euler angles in radians
     * Mat4 originalMatrix = ...; // Some existing matrix
     * Mat4 rotatedMatrix = RotateEulerRadians(originalMatrix, eulerRadians); // Create rotation matrix and apply it to the original matrix
     * @endcode
     */
    inline Mat4 RotateEulerDegrees(const Vec3& eulerDeg)
	{
		return RotateEulerRadians({ eulerDeg.x * DEG_TO_RAD, eulerDeg.y * DEG_TO_RAD, eulerDeg.z * DEG_TO_RAD });
	}

    /**
     * @brief Create a rotation matrix from Euler angles in radians, applied to an existing matrix.
     *
     * This function constructs a rotation matrix based on the provided Euler angles in radians and applies it to an existing matrix.
     *
     * @param m The existing matrix to which the rotation will be applied.
     * @param euler The Euler angles in radians, represented as a Vec3 where:
     *            - euler.x is the rotation around the X-axis,
     *            - euler.y is the rotation around the Y-axis,
     *            - euler.z is the rotation around the Z-axis.
     * @return Mat4 A 4x4 rotation matrix representing the combined rotation applied to the input matrix.
     *
     * @note - The resulting matrix can be used to transform points or vectors in 3D space by applying the rotation defined by the Euler angles.
     * @note - The angles are assumed to be in radians, so if you have angles in degrees, you should convert them to radians before calling this function.
     *
     * @code
     * Vec3 eulerRadians(0.0f, 1.57f, 0.0f); // Example Euler angles in radians
     * Mat4 originalMatrix = ...; // Some existing matrix
     * Mat4 rotatedMatrix = RotateEulerRadians(originalMatrix, eulerRadians); // Create rotation matrix and apply it to the original matrix
     * @endcode
     */
    inline Mat4 RotateEulerRadians(const Mat4& m, const Vec3& euler)
	{
		return m * RotateEulerRadians(euler);
	}

    /**
     * @brief Create a rotation matrix from Euler angles in degrees, applied to an existing matrix.
     *
     * This function constructs a rotation matrix based on the provided Euler angles in degrees and applies it to an existing matrix.
     *
     * @param m The existing matrix to which the rotation will be applied.
     * @param eulerDeg The Euler angles in degrees, represented as a Vec3 where:
     *           - euler.x is the rotation around the X-axis,
     *           - euler.y is the rotation around the Y-axis,
     *           - euler.z is the rotation around the Z-axis.
     * @return Mat4 A 4x4 rotation matrix representing the combined rotation applied to the input matrix.
     *
     * @note - The resulting matrix can be used to transform points or vectors in 3D space by applying the rotation defined by the Euler angles.
     * @note - The angles are assumed to be in degrees, so if you have angles in radians, you should convert them to degrees before calling this function.
     *
     * @code
     * Vec3 eulerDegrees(0.0f, 90.0f, 0.0f); // Example Euler angles in degrees
     * Mat4 originalMatrix = ...; // Some existing matrix
     * Mat4 rotatedMatrix = RotateEulerDegrees(originalMatrix, eulerDegrees); // Create rotation matrix and apply it to the original matrix
     * @endcode
     */
    inline Mat4 RotateEulerDegrees(const Mat4& m, const Vec3& eulerDeg)
	{
		return m * RotateEulerDegrees(eulerDeg);
	}

    ///////////////////////////////////////////////////////////
	//		 Axis-angle rotation (right-handed), radians	 //
    ///////////////////////////////////////////////////////////

    /**
     * @brief Create a rotation matrix that rotates around a specified axis by a given angle in radians.
     *
     * This function constructs a rotation matrix that rotates points around a specified axis by the given angle in radians.
     *
     * @param axis The axis of rotation, represented as a Vec3. This vector should be normalized (unit length) for correct rotation.
     * @param angle The angle in radians by which to rotate around the specified axis.
     * @return Mat4 A 4x4 rotation matrix representing the rotation around the specified axis.
     *
     * @note - The axis of rotation should be a normalized vector (unit length) for the rotation to be correct.
     * @note - The rotation is right-handed, meaning positive angles rotate counter-clockwise when looking along the axis of rotation towards the origin.
     * @note - If the axis is not normalized, the resulting rotation may not behave as expected.
     *
     * @code
     * Vec3 rotationAxis(0.0f, 1.0f, 0.0f); // Example axis (Y-axis)
     * float rotationAngle = 1.57f; // Example angle in radians (90 degrees)
     * Mat4 rotationMatrix = RotateAxisAngleRadians(rotationAxis, rotationAngle); // Create rotation matrix
     * @endcode
     */
    inline Mat4 RotateAxisAngleRadians(const Vec3& axis, float angle)
	{
		/// Normalize axis
		const float len2 = axis.x * axis.x + axis.y * axis.y + axis.z * axis.z;
		float invLen = len2 > 0.0f ? 1.0f / std::sqrt(len2) : 0.0f;
		const float x = axis.x * invLen;
		const float y = axis.y * invLen;
		const float z = axis.z * invLen;

		const float c = std::cos(angle);
		const float s = std::sin(angle);
		const float omc = 1.0f - c;

		return Mat4({
			{ c + x * x * omc,     x * y * omc - z * s, x * z * omc + y * s, 0.0f },
			{ y * x * omc + z * s, c + y * y * omc,     y * z * omc - x * s, 0.0f },
			{ z * x * omc - y * s, z * y * omc + x * s, c + z * z * omc,     0.0f },
			{ 0.0f,                0.0f,                0.0f,                1.0f }
		});
	}

    /**
     * @brief Create a rotation matrix that rotates around a specified axis by a given angle in radians, applied to an existing matrix.
     *
     * This function constructs a rotation matrix that rotates points around a specified axis by the given angle in radians and applies it to an existing matrix.
     *
     * @param m The existing matrix to which the rotation will be applied.
     * @param axis The axis of rotation, represented as a Vec3. This vector should be normalized (unit length) for correct rotation.
     * @param angle The angle in radians by which to rotate around the specified axis.
     * @return Mat4 A 4x4 rotation matrix representing the combined rotation applied to the input matrix.
     *
     * @note - The axis of rotation should be a normalized vector (unit length) for the rotation to be correct.
     * @note - The rotation is right-handed, meaning positive angles rotate counter-clockwise when looking along the axis of rotation towards the origin.
     * @note - If the axis is not normalized, the resulting rotation may not behave as expected.
     *
     * @code
     * Vec3 rotationAxis(0.0f, 1.0f, 0.0f); // Example axis (Y-axis)
     * float rotationAngle = 1.57f; // Example angle in radians (90 degrees)
     * Mat4 originalMatrix = ...; // Some existing matrix
     * Mat4 rotatedMatrix = RotateAxisAngleRadians(originalMatrix, rotationAxis, rotationAngle); // Create rotation matrix and apply it to the original matrix
     * @endcode
     */
    inline Mat4 RotateAxisAngleRadians(const Mat4& m, const Vec3& axis, float angle)
	{
		return m * RotateAxisAngleRadians(axis, angle);
	}

    /**
     * @brief Create a rotation matrix that rotates around a specified axis by a given angle in degrees.
     *
     * This function constructs a rotation matrix that rotates points around a specified axis by the given angle in degrees.
     *
     * @param axis The axis of rotation, represented as a Vec3. This vector should be normalized (unit length) for correct rotation.
     * @param degrees The angle in degrees by which to rotate around the specified axis.
     * @return Mat4 A 4x4 rotation matrix representing the rotation around the specified axis.
     *
     * @note - The axis of rotation should be a normalized vector (unit length) for the rotation to be correct.
     * @note - The rotation is right-handed, meaning positive angles rotate counter-clockwise when looking along the axis of rotation towards the origin.
     *
     * @code
     * Vec3 rotationAxis(0.0f, 1.0f, 0.0f); // Example axis (Y-axis)
     * float rotationDegrees = 90.0f; // Example angle in degrees
     * Mat4 rotationMatrix = RotateAxisAngleDegrees(rotationAxis, rotationDegrees); // Create rotation matrix
     * @endcode
     */
    inline Mat4 RotateAxisAngleDegrees(const Vec3& axis, float degrees)
	{
		return RotateAxisAngleRadians(axis, degrees * DEG_TO_RAD);
	}

	static Mat4 AxisAngleRadians(const Vec3& axis, float angle)
	{
	    // Normalize axis
	    Vec3 nAxis = Normalize(axis);
	    float c = std::cos(angle);
	    float s = std::sin(angle);
	    float t = 1.0f - c;
	    float x = nAxis.x, y = nAxis.y, z = nAxis.z;
	
	    // Rodrigues' rotation formula for 3x3
        Mat4 result = Mat4::Identity();
	    result[0][0] = t * x * x + c;
	    result[0][1] = t * x * y - s * z;
	    result[0][2] = t * x * z + s * y;
	    result[1][0] = t * x * y + s * z;
	    result[1][1] = t * y * y + c;
	    result[1][2] = t * y * z - s * x;
	    result[2][0] = t * x * z - s * y;
	    result[2][1] = t * y * z + s * x;
	    result[2][2] = t * z * z + c;
	    // The last row/column remain as for affine transform
	    return result;
	}

    /**
     * @brief Create a rotation matrix that rotates around a specified axis by a given angle in degrees, applied to an existing matrix.
     *
     * This function constructs a rotation matrix that rotates points around a specified axis by the given angle in degrees and applies it to an existing matrix.
     *
     * @param m The existing matrix to which the rotation will be applied.
     * @param axis The axis of rotation, represented as a Vec3. This vector should be normalized (unit length) for correct rotation.
     * @param degrees The angle in degrees by which to rotate around the specified axis.
     * @return Mat4 A 4x4 rotation matrix representing the combined rotation applied to the input matrix.
     *
     * @note - The axis of rotation should be a normalized vector (unit length) for the rotation to be correct.
     * @note - The rotation is right-handed, meaning positive angles rotate counter-clockwise when looking along the axis of rotation towards the origin.
     *
     * @code
     * Vec3 rotationAxis(0.0f, 1.0f, 0.0f); // Example axis (Y-axis)
     * float rotationDegrees = 90.0f; // Example angle in degrees
     * Mat4 originalMatrix = ...; // Some existing matrix
     * Mat4 rotatedMatrix = RotateAxisAngleDegrees(originalMatrix, rotationAxis, rotationDegrees); // Create rotation matrix and apply it to the original matrix
     * @endcode
     */
    inline Mat4 RotateAxisAngleDegrees(const Mat4& m, const Vec3& axis, float degrees)
	{
		return m * RotateAxisAngleDegrees(axis, degrees);
	}


    ///////////////////////////////////////////////////////////
	//			  2D rotation (Mat3 homogeneous)			 //
    ///////////////////////////////////////////////////////////

    /**
     * @brief Create a 2D rotation matrix that rotates points around the origin by a given angle in radians.
     *
     * This function constructs a 2D rotation matrix that rotates points around the origin by the specified angle in radians.
     *
     * @param radians The angle in radians by which to rotate points around the origin.
     * @return Mat3 A 3x3 rotation matrix representing the rotation in 2D space.
     *
     * @note - The rotation is right-handed, meaning positive angles rotate counter-clockwise when looking from above the 2D plane.
     * @note - The resulting matrix can be used to transform 2D points or vectors in homogeneous coordinates (x, y, 1).
     *
     * @code
     * float rotationAngle = 1.57f; // Example angle in radians (90 degrees)
     * Mat3 rotationMatrix = Rotate2D(rotationAngle); // Create 2D rotation matrix
     * @endcode
     */
    inline Mat3 Rotate2D(float radians)
	{
		const float c = std::cos(radians);
		const float s = std::sin(radians);
		return {
		    c, -s, 0.0f,
		    s,  c, 0.0f,
		    0.0f, 0.0f, 1.0f
		};
	}

    /**
     * @brief Create a 2D rotation matrix that rotates points around the origin by a given angle in radians, applied to an existing matrix.
     *
     * This function constructs a 2D rotation matrix that rotates points around the origin by the specified angle in radians and applies it to an existing matrix.
     *
     * @param m The existing matrix to which the rotation will be applied.
     * @param radians The angle in radians by which to rotate points around the origin.
     * @return Mat3 A 3x3 rotation matrix representing the combined rotation applied to the input matrix.
     *
     * @note - The rotation is right-handed, meaning positive angles rotate counter-clockwise when looking from above the 2D plane.
     * @note - The resulting matrix can be used to transform 2D points or vectors in homogeneous coordinates (x, y, 1).
     * @note - If the input matrix is not a 2D transformation matrix, the result may not behave as expected.
     *
     * @code
     * Mat3 originalMatrix = ...; // Some existing 2D transformation matrix
     * float rotationAngle = 1.57f; // Example angle in radians (90 degrees)
     * Mat3 rotatedMatrix = Rotate2D(originalMatrix, rotationAngle); // Create 2D rotation matrix and apply it to the original matrix
     * @endcode
     */
    inline Mat3 Rotate2D(const Mat3& m, float radians)
	{
	    return m * Rotate2D(radians);
	}

}

/// -------------------------------------------------------

// -------------------------------------------------------
// GLM compatibility shims for rotate (axis-angle), using radians
// Signatures match glm::rotate(m, angle, axis) and glm::rotate(angle, axis)
// -------------------------------------------------------
namespace glm
{
	inline ::SceneryEditorX::Mat4 rotate(const ::SceneryEditorX::Mat4& m, float angleRadians, const ::SceneryEditorX::Vec3& axis)
	{
		return ::SceneryEditorX::RotateAxisAngleRadians(m, axis, angleRadians);
	}

	inline ::SceneryEditorX::Mat4 rotate(float angleRadians, const ::SceneryEditorX::Vec3& axis)
	{
		return ::SceneryEditorX::RotateAxisAngleRadians(axis, angleRadians);
	}
}

/// -------------------------------------------------------
