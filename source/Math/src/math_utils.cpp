/**
* -------------------------------------------------------
* Scenery Editor X
* -------------------------------------------------------
* Copyright (c) 2025 Thomas Ray
* Copyright (c) 2025 Coalition of Freeware Developers
* -------------------------------------------------------
* math_utils.cpp - Mathematical utility functions
* -------------------------------------------------------
* Created: 12/8/2025
* -------------------------------------------------------
*/
#include <Math/includes/math_utils.h>
#include <Math/includes/quat.h>
#include <Math/includes/transforms.h>
#include <Math/includes/vector.h>

/// -----------------------------------------------------

namespace SceneryEditorX
{
	/**
	 * @brief Checks if two floating-point numbers are approximately equal within a given epsilon.
	 *
	 * This function compares the absolute difference between two numbers.
	 *
	 * @param a
	 * @param b
	 * @param epsilon
	 * @return true if the absolute difference between a and b is less than or equal to epsilon, false otherwise.
	 */
	bool IsEqual(float a, float b, float epsilon)
	{
	    return fabsf(a - b) <= epsilon;
	}

    /**
     * @brief Checks if a floating-point number is approximately zero within a given epsilon.
     *
     * This function determines if the absolute value of the number
     * is less than or equal to epsilon.
     *
     * @param value
     * @param epsilon
     * @return true if the absolute value of the number is less than or equal to epsilon, false otherwise.
     */
    bool IsZero(float value, float epsilon)
	{
	    return fabsf(value) <= epsilon;
	}

    /**
     * @brief Calculates the distance between two 3D points.
     *
     * This function computes the Euclidean distance between two points in 3D space.
     *
     * @param a
     * @param b
     * @return The Euclidean distance between points a and b.
     */
    float Distance(const Vec3& a, const Vec3& b)
	{
	    const float dx = b.x - a.x;
	    const float dy = b.y - a.y;
	    const float dz = b.z - a.z;
	    return sqrtf(dx*dx + dy*dy + dz*dz);
	}

    /**
	 * @brief Calculates the length of a 3D vector.
	 *
	 * This function computes the Euclidean length (magnitude) of the vector
	 *
	 * @param v
	 * @return The Euclidean length of the vector v.
	 */
	float Length(const Vec3& v)
	{
		return sqrtf(v.x * v.x + v.y * v.y + v.z * v.z);
	}

    /**
     * @brief Calculates the squared length of a 3D vector.
     *
     * This function is useful for performance when you only need to compare lengths
     *
     * @param v
     * @return The squared length of the vector v.
     */
    float Length2(const Vec3& v)
	{
		return v.x * v.x + v.y * v.y + v.z * v.z;
	}

    /**
     * @brief Normalizes a 3D vector.
     *
     * This function returns a new vector that has the same direction
     * as the input vector but with a length of 1.
     *
     * @param v
     * @return A new Vec3 that is the normalized version of v.
     */
    Vec3 Normalize(const Vec3& v)
	{
	    const float len2 = v.x*v.x + v.y*v.y + v.z*v.z;
	    if (len2 <= 0.0f)
            return Vec3(0.0f, 0.0f, 0.0f);

        const float invLen = 1.0f / sqrtf(len2);
	    return Vec3(v.x*invLen, v.y*invLen, v.z*invLen);
	}

    /**
     * @brief Calculates the dot product of two 3D vectors.
     *
     * This function computes the dot product, which is a measure of how
     * aligned two vectors are. It is defined as the sum of the products
     * of their corresponding components.
     *
     * @param a First vector a
     * @param b Second vector b
     * @return The dot product of vectors a and b.
     */
    float Dot(const Vec3& a, const Vec3& b)
	{
	    return a.x*b.x + a.y*b.y + a.z*b.z;
	}

    /**
     * @brief Calculates the cross product of two 3D vectors.
     *
     * This function computes the cross product, which results in a vector
     * that is perpendicular to both input vectors. The direction of the
     * resulting vector follows the right-hand rule.
     *
     * @param a First vector a
     * @param b Second vector b
     * @return A new Vec3 that is the cross product of a and b.
     */
    Vec3 Cross(const Vec3& a, const Vec3& b)
	{
	    return {
	        a.y * b.z - a.z * b.y,	/// X
	        a.z * b.x - a.x * b.z,	/// Y
	        a.x * b.y - a.y * b.x		/// Z
	    };
	}

	/// ---------------------------------------------------------------------
	/// Compatibility wrappers (glm::quat <-> native Quat)
	/// ---------------------------------------------------------------------

    /*
    bool Math::DecomposeTransform(const Mat4 &mat, Vec3 &translation, Quat &rotation, Vec3 &scale)
	{
		Quat nativeQ;
		const bool ok = DecomposeTransform(mat, translation, nativeQ, scale);
		if (ok)
            rotation = Quat(nativeQ.w, nativeQ.x, nativeQ.y, nativeQ.z);
		return ok;
	}
	*/

	/*
    Mat4 Math::ComposeTransform(const Vec3 &translation, const Quat &rotation, const Vec3 &scale)
	{
		const Quat q(rotation.w, rotation.x, rotation.y, rotation.z);
		return ComposeTransform(translation, q, scale);
	}
	*/

	/// ---------------------------------------------------------------------
	/// Native Quat overloads (no GLM types)
	/// ---------------------------------------------------------------------

    /**
     * @brief Decomposes a transformation matrix into translation, rotation, and scale components.
     *
     * This function extracts the translation, rotation (as a quaternion), and scale
     * from a 4x4 transformation matrix. It is useful for breaking down complex
     * transformations into their individual components for further manipulation or analysis.
     *
     * @param mat The transformation matrix to decompose
     * @param translation Output parameter for the translation vector
     * @param rotation Output parameter for the rotation quaternion
     * @param scale Output parameter for the scale vector
     * @return true if the decomposition was successful, false otherwise.
     */
	bool Math::DecomposeTransform(const Mat4 &mat, Vec3 &translation, Quat &rotation, Vec3 &scale)
	{
        return Transforms::Decompose(mat, translation, rotation, scale);
	}

    /**
     * @brief Composes a transformation Mat4 matrix from translation, rotation, and scale components.
     *
     * This function creates a 4x4 transformation matrix that combines
     * the specified translation, rotation (as a quaternion), and scale.
     *
     * @param translation
     * @param rotation
     * @param scale
     * @return A 4x4 transformation matrix representing the combined transformation.
     */
    Mat4 Math::ComposeTransform(const Vec3 &translation, const Quat &rotation, const Vec3 &scale)
    {
        return Transforms::Compose(translation, rotation, scale);
	}

}

/// -------------------------------------------------------
