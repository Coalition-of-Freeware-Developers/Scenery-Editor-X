/**
* -------------------------------------------------------
* Scenery Editor X
* -------------------------------------------------------
* Copyright (c) 2025 Thomas Ray
* Copyright (c) 2025 Coalition of Freeware Developers
* -------------------------------------------------------
* scale.h
* -------------------------------------------------------
* Created: 13/8/2025
* -------------------------------------------------------
*/
#pragma once
#include <SceneryEditorX/utils/math/matrix.h>
#include <SceneryEditorX/utils/math/vector.h>

/// -------------------------------------------------------

namespace SceneryEditorX
{

		///////////////////////////////////////////////////////////
		//					 3D scale (Mat4)					 //
		///////////////////////////////////////////////////////////

		/**
		 * @brief Create a 3D scaling matrix.
		 *
		 * This function constructs a 3D scaling matrix that scales points in the X, Y, and Z dimensions.
		 *
		 * @param s A Vec3 representing the scaling factors for the X, Y, and Z dimensions:
		 *        - s.x is the scaling factor for the X-axis,
		 *        - s.y is the scaling factor for the Y-axis,
		 *        - s.z is the scaling factor for the Z-axis.
		 *
		 * @return Mat4 A 4x4 matrix representing the scaling transformation.
		 *
		 * @note - The resulting matrix can be used to transform points or vectors in 3D space by applying the scaling defined by the Vec3 parameter.
		 * @note - The last row and column of the matrix are set to 0.0f and 1.0f, respectively, to maintain the homogeneous coordinate system.
		 *
		 * @code
		 * Vec3 scaleFactors(2.0f, 3.0f, 4.0f); // Example scaling factors
		 * Mat4 scalingMatrix = Scale(scaleFactors); // Create a scaling matrix
		 * @endcode
		 */
		inline Mat4 Scale(const Vec3& s)
		{
			return Mat4({
				{s.x, 0.0f, 0.0f, 0.0f},		/// Scale X
				{0.0f, s.y, 0.0f, 0.0f},		/// Scale Y
				{0.0f, 0.0f, s.z, 0.0f},		/// Scale Z
		        {0.0f, 0.0f, 0.0f, 1.0f}	/// Homogeneous coordinate (no translation)
			});
		}

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
		 * @note - This function uses GLM's epsilon comparison to safely check for zero-length vectors.
		 */
        inline Vec3 Scale(const Vec3 &vector, float desiredLength)
		{
		    /// Compute magnitude without relying on GLM
		    const float mag = std::sqrt(vector.x * vector.x + vector.y * vector.y + vector.z * vector.z);
		    if (::SceneryEditorX::epsilonEqual(mag, 0.0f, ::SceneryEditorX::epsilon<float>()))
		        return Vec3(0.0f);

		    return vector * desiredLength / mag;
		}

		/**
		 * @brief Create a 3D scaling matrix and apply it to an existing matrix.
		 *
		 * This function constructs a 3D scaling matrix and multiplies it with an existing matrix.
		 *
		 * @param s A Vec3 representing the scaling factors for the X, Y, and Z dimensions:
		 *       - s.x is the scaling factor for the X-axis,
		 *       - s.y is the scaling factor for the Y-axis,
		 *       - s.z is the scaling factor for the Z-axis.
		 * @return Mat4 A 4x4 matrix representing the scaling transformation applied to the existing matrix.
		 *
		 * @note - The resulting matrix can be used to transform points or vectors in 3D space by applying the scaling defined by the Vec3 parameter.
		 * @note - The last row and column of the matrix are set to 0.0f and 1.0f, respectively, to maintain the homogeneous coordinate system.
		 *
		 * @code
		 * Mat4 existingMatrix = ...; // Some existing matrix
		 * Mat4 scalingMatrix = Scale(existingMatrix, Vec3(2.0f, 3.0f, 4.0f)); // Create a scaling matrix and apply it
		 * @endcode
		 */
		inline Mat4 Scale(const Vec2& s)
		{
			return Mat4({
				{s.x, 0.0f, 0.0f, 0.0f},		/// Scale X
				{0.0f, s.y, 0.0f, 0.0f},		/// Scale Y
				{0.0f, 0.0f, 1.0f, 0.0f},	/// Scale Z (no scaling)
		        {0.0f, 0.0f, 0.0f, 1.0f}	/// Homogeneous coordinate (no translation)
			});
		}

		/**
		 * @brief Create a 3D scaling matrix and apply it to an existing matrix.
		 *
		 * This function constructs a 3D scaling matrix and multiplies it with an existing matrix.
		 *
		 * @param m An existing 4x4 matrix to which the scaling will be applied.
		 * @param s A Vec3 representing the scaling factors for the X, Y, and Z dimensions:
		 *       - s.x is the scaling factor for the X-axis,
		 *       - s.y is the scaling factor for the Y-axis,
		 *       - s.z is the scaling factor for the Z-axis.
		 * @return Mat4 A 4x4 matrix representing the scaling transformation applied to the existing matrix.
		 *
		 * @note - The resulting matrix can be used to transform points or vectors in 3D space by applying the scaling defined by the Vec3 parameter.
		 * @note - The last row and column of the matrix are set to 0.0f and 1.0f, respectively, to maintain the homogeneous coordinate system.
		 *
		 * @code
		 * Mat4 existingMatrix = ...; // Some existing matrix
		 * Mat4 scalingMatrix = Scale(existingMatrix, Vec3(2.0f, 3.0f, 4.0f)); // Create a scaling matrix and apply it
		 * @endcode
		 */
		inline Mat4 Scale(const Mat4& m, const Vec3& s)
		{
		    return m * Scale(s);
		}

		/**
		 * @brief Create a 2D scaling matrix and apply it to an existing matrix.
		 *
		 * This function constructs a 2D scaling matrix and multiplies it with an existing matrix.
		 *
		 * @param m An existing 3x3 matrix to which the scaling will be applied.
		 * @param s A Vec2 representing the scaling factors for the X and Y dimensions:
		 *      - s.x is the scaling factor for the X-axis,
		 *      - s.y is the scaling factor for the Y-axis.
		 * @return Mat3 A 3x3 matrix representing the scaling transformation applied to the existing matrix.
		 *
		 * @note - The resulting matrix can be used to transform points or vectors in 2D space by applying the scaling defined by the Vec2 parameter.
		 * @note - The last row and column of the matrix are set to 0.0f and 1.0f, respectively, to maintain the homogeneous coordinate system.
		 *
		 * @code
		 * Mat3 existingMatrix = ...; // Some existing matrix
		 * Mat3 scalingMatrix = Scale2D(existingMatrix, Vec2(2.0f, 3.0f)); // Create a scaling matrix and apply it
		 * @endcode
		 */
		inline Mat4 Scale(const Mat4& m, const Vec2& s)
		{
		    return m * Scale(s);
		}

		///////////////////////////////////////////////////////////
		//			   2D scale (homogeneous Mat3)				 //
		///////////////////////////////////////////////////////////

		/**
		 * @brief Create a 2D scaling matrix.
		 *
		 * This function constructs a 2D scaling matrix that scales points in the X and Y dimensions.
		 * The resulting matrix can be used to transform points or vectors in 2D space by applying the scaling defined by the Vec2 parameter.
		 *
		 * @param s A Vec2 representing the scaling factors for the X and Y dimensions:
		 *         - s.x is the scaling factor for the X-axis,
		 *        - s.y is the scaling factor for the Y-axis.
		 * @return Mat3 A 3x3 matrix representing the scaling transformation.
		 *
		 * @note - The resulting matrix is in homogeneous coordinates, which allows for translation and rotation to be combined with scaling in a single matrix.
		 * @note - The last row and column of the matrix are set to 0.0f and 1.0f, respectively, to maintain the homogeneous coordinate system.
		 *
		 * @code
		 * Vec2 scaleFactors(2.0f, 3.0f); // Example scaling factors
		 * Mat3 scalingMatrix = Scale2D(scaleFactors); // Create a scaling matrix
		 * @endcode
		 *
		 */
		inline Mat3 Scale2D(const Vec2& s)
		{
			return {
			    s.x, 0.0f, 0.0f, /// Scale X
			    0.0f, s.y, 0.0f, /// Scale Y
			    0.0f, 0.0f, 1.0f /// Homogeneous coordinate (no translation)
			};
		}

		/**
		 * @brief Create a 2D scaling matrix and apply it to an existing matrix.
		 *
		 * This function constructs a 2D scaling matrix and multiplies it with an existing matrix.
		 *
		 * @param m An existing 3x3 matrix to which the scaling will be applied.
		 * @param s A Vec2 representing the scaling factors for the X and Y dimensions:
		 *       - s.x is the scaling factor for the X-axis,
		 *       - s.y is the scaling factor for the Y-axis.
		 * @return Mat3 A 3x3 matrix representing the scaling transformation applied to the existing matrix.
		 *
		 * @note - The resulting matrix can be used to transform points or vectors in 2D space by applying the scaling defined by the Vec2 parameter.
		 * @note - The last row and column of the matrix are set to 0.0f and 1.0f, respectively, to maintain the homogeneous coordinate system.
		 *
		 * @code
		 * Mat3 existingMatrix = ...; // Some existing matrix
		 * Mat3 scalingMatrix = Scale2D(existingMatrix, Vec2(2.0f, 3.0f)); // Create a scaling matrix and apply it
		 * @endcode
		 */
		inline Mat3 Scale2D(const Mat3& m, const Vec2& s)
		{
		    return m * Scale2D(s);
		}

}

/// -------------------------------------------------------
