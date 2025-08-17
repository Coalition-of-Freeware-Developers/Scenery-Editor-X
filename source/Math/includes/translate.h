/**
* -------------------------------------------------------
* Scenery Editor X
* -------------------------------------------------------
* Copyright (c) 2025 Thomas Ray
* Copyright (c) 2025 Coalition of Freeware Developers
* -------------------------------------------------------
* translate.h
* -------------------------------------------------------
* Created: 13/8/2025
* -------------------------------------------------------
*/
#pragma once
#include <cmath>
#include <Math/includes/matrix.h>   // brings in mat2/3/4
#include <Math/includes/vector.h>
#include <Math/math_config.h>

/// -------------------------------------------------------

namespace SceneryEditorX
{
    ///////////////////////////////////////////////////////////
	//					3D translation (Mat4)				 //
    ///////////////////////////////////////////////////////////

    /**
     * @brief Creates a translation matrix that translates by the given vector.
     *
     * This function generates a 4x4 translation matrix that can be used to translate
     * 3D points in space. The translation is applied in the order of the matrix multiplication,
     * meaning it will translate points by the vector t when multiplied with the matrix.
     *
     * @param t translation matrix vector
     * @return Mat4 A 4x4 translation matrix that translates points by the vector t.
     *
     * @code
     * Mat4 translationMatrix = Translate(Vec3(1.0f, 2.0f, 3.0f));
     * Mat4 combinedMatrix = Translate(translationMatrix, Vec3(4.0f, 5.0f, 6.0f));
     * @endcode
     */
    inline Mat4 Translate(const Vec3& t)
	{
		/// Row-major: translation in last column
		return Mat4({
			{1.0f, 0.0f, 0.0f, t.x}, /// Translate X
			{0.0f, 1.0f, 0.0f, t.y}, /// Translate Y
			{0.0f, 0.0f, 1.0f, t.z}, /// Translate Z
			{0.0f, 0.0f, 0.0f, 1.0f}  /// Homogeneous coordinate (no translation)
		});
	}

    /**
     * @brief Creates a translation matrix that translates by the given vector.
     *
     * This function generates a 4x4 translation matrix that can be used to translate
     * 3D points in space. The translation is applied in the order of the matrix multiplication,
     * meaning it will translate points by the vector t when multiplied with the matrix.
     *
     * @param m
     * @param t transform matrix vector
     * @return
     *
     * @code
     * Mat4 translationMatrix = Translate(Vec3(1.0f, 2.0f, 3.0f));
     * Mat4 combinedMatrix = Translate(translationMatrix, Vec3(4.0f, 5.0f, 6.0f));
     * @endcode
     */
    inline Mat4 Translate(const Mat4& m, const Vec3& t) { return m * Translate(t); }

    ///////////////////////////////////////////////////////////
	//			2D translation (homogeneous Mat3)			 //
    ///////////////////////////////////////////////////////////

    /**
     * @brief Creates a translation matrix that translates by the given 2D vector.
     *
     * This function generates a 3x3 translation matrix that can be used to translate
     * 2D points in space. The translation is applied in the order of the matrix multiplication,
     * meaning it will translate points by the vector t when multiplied with the matrix.
     *
     * @param t translation matrix vector
     * @return Mat3 A 3x3 translation matrix that translates points by the vector t.
     *
     * @code
     * Mat3 translationMatrix = Translate2D(Utils::TVector2<float>(1.0f, 2.0f));
     * Mat3 combinedMatrix = Translate2D(translationMatrix, Utils::TVector2<float>(3.0f, 4.0f));
     * @endcode
     */
    inline Mat3 Translate2D(const Utils::TVector2<float>& t)
	{
		return {
		    1.0f, 0.0f, t.x, /// Translate X
		    0.0f, 1.0f, t.y, /// Translate Y
		    0.0f, 0.0f, 1.0f /// Homogeneous coordinate (no translation)
		};
	}

    /**
     * @brief Creates a translation matrix that translates by the given 2D vector.
     *
     * This function generates a 3x3 translation matrix that can be used to translate
     * 2D points in space. The translation is applied in the order of the matrix multiplication,
     * meaning it will translate points by the vector t when multiplied with the matrix.
     *
     * @param m matrix to translate
     * @param t transform matrix vector
     * @return Mat3 A 3x3 translation matrix that translates points by the vector t.
     *
     * @code
     * Mat3 translationMatrix = Translate2D(Utils::TVector2<float>(1.0f, 2.0f));
     * Mat3 combinedMatrix = Translate2D(translationMatrix, Utils::TVector2<float>(3.0f, 4.0f));
     * @endcode
     */
    inline Mat3 Translate2D(const Mat3& m, const Utils::TVector2<float>& t)
    {
        return m * Translate2D(t);
    }

}

/// -------------------------------------------------------
