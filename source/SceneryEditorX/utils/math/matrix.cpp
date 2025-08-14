/**
* -------------------------------------------------------
* Scenery Editor X
* -------------------------------------------------------
* Copyright (c) 2025 Thomas Ray
* Copyright (c) 2025 Coalition of Freeware Developers
* -------------------------------------------------------
* matrix.cpp
* -------------------------------------------------------
* Created: 15/7/2025
* -------------------
*/
#include <SceneryEditorX/utils/math/math_utils.h>
#include <SceneryEditorX/utils/math/matrix.h>
#include <SceneryEditorX/utils/math/quat.h>
#if TRACY_ENABLE
#include <tracy/Tracy.hpp>
#endif

/// -------------------------------------------------------

namespace SceneryEditorX
{
    /**
     * @brief Creates a perspective projection matrix for 3D rendering.
     *
     * This function generates a perspective projection matrix that transforms 3D coordinates
     * from view space (camera space) to clip space. The matrix implements a standard
     * perspective projection with a symmetric frustum defined by field of view and aspect ratio.
     *
     * @param aspect The aspect ratio of the viewport (width / height). Must be positive.
     * @param fieldOfView The vertical field of view angle in degrees. Typical values range from 45-90 degrees.
     * @param n The distance to the near clipping plane. Must be positive and less than far plane.
     * @param f The distance to the far clipping plane. Must be positive and greater than near plane.
     *
     * @return Matrix4x4 A 4x4 perspective projection matrix suitable for use in graphics pipelines.
     *
     * @note - The Y-axis is inverted (negative scale) to match common graphics coordinate systems.
     * @note - Objects closer than the near plane or farther than the far plane will be clipped.
     * @warning Ensure near and far planes are positive and near < far to avoid degenerate matrices.
     *
     * @code
     * float aspectRatio = screenWidth / screenHeight;
     * Matrix4x4 projection = Matrix4x4::PerspectiveProjection(aspectRatio, 60.0f, 0.1f, 1000.0f);
     * @endcode
     */
    Mat4 Mat4::PerspectiveProjection(const float aspect, const float fieldOfView, const float n, const float f)
    {
		const float tanHalfFOV = tan(ToRadians(fieldOfView / 2.0f));

        return Mat4({
			{ 1 / (aspect * tanHalfFOV), 0, 0, 0 },
			{ 0, -1 / tanHalfFOV, 0, 0 },
			{ 0, 0, f / (f - n), -f * n / (f - n) },
			{ 0, 0, 1, 0 }
		});
    }

	/**
	 * @brief Creates an orthographic projection matrix for 2D rendering or technical drawing.
	 *
	 * This function generates an orthographic projection matrix that transforms 3D coordinates
	 * from view space to clip space without perspective distortion. Objects maintain their
	 * relative sizes regardless of distance from the camera, making it ideal for 2D rendering,
	 * UI elements, and technical/engineering drawings.
	 *
	 * @param l The left edge of the projection volume (minimum X coordinate).
	 * @param r The right edge of the projection volume (maximum X coordinate). Must be > left.
	 * @param t The top edge of the projection volume (maximum Y coordinate).
	 * @param b The bottom edge of the projection volume (minimum Y coordinate). Must be < top.
	 * @param nearPlane The distance to the near clipping plane. Can be negative.
	 * @param farPlane The distance to the far clipping plane. Must be != nearPlane.
	 *
	 * @return Matrix4x4 A 4x4 orthographic projection matrix suitable for 2D and isometric rendering.
	 *
	 * @note - The coordinate system is right-handed with Y pointing up.
	 * @note - Unlike perspective projection, parallel lines remain parallel after transformation.
	 * @warning Ensure right > left, top > bottom, and farPlane != nearPlane to avoid degenerate matrices.
	 *
	 * @code
	 * // Create orthographic projection for 2D rendering
	 * Matrix4x4 ortho = Matrix4x4::OrthographicProjection(0.0f, 800.0f, 600.0f, 0.0f, -1.0f, 1.0f);
	 * @endcode
	 */
	Mat4 Mat4::OrthographicProjection(float l, float r, float t, float b, float nearPlane, float farPlane)
	{
		return Mat4({
			{ 2 / (r - l), 0, 0, -(r + l) / (r - l) },
			{ 0, 2 / (b - t), 0, -(b + t) / (b - t) },
			{ 0, 0, 1 / (farPlane - nearPlane), -nearPlane / (farPlane - nearPlane) },
			{ 0, 0, 0, 1 }
		});
	}

    /**
     * @brief Creates a look-at matrix for camera transformations.
     *
     * This function generates a view matrix that transforms world coordinates
     * into camera space, effectively positioning and orienting the camera in 3D space.
     *
     * @param eye The position of the camera (eye point).
     * @param center The point in space the camera is looking at (target point).
     * @param up The up direction vector for the camera, defining its orientation.
     * @return Matrix4x4 A 4x4 look-at matrix that can be used to transform world coordinates to camera space.
     */
    Mat4 Mat4::LookAt(const Vec3& eye, const Vec3& center, const Vec3& up)
	{
		/// Right-handed look-at using row-major layout and column-vector math basis
		const Vec3 f = Normalize(center - eye);				/// forward
		const Vec3 s = Normalize(Cross(f, up));	/// right
		const Vec3 u = Cross(s, f);								/// true up

		/// Translate components
        const float tx = -Dot(s, eye); /// Negative dot product for translation
        const float ty = -Dot(u, eye); /// Negative dot product for translation
        const float tz = Dot(f, eye);  /// Positive dot product for translation

		return Mat4({
			{ s.x,  u.x, -f.x, tx },		/// Right vector
			{ s.y,  u.y, -f.y, ty },		/// Up vector
			{ s.z,  u.z, -f.z, tz },		/// Forward vector
			{ 0.0f, 0.0f, 0.0f, 1.0f }	/// Homogeneous coordinate (no translation)
		});
	}

	/**
	 * @brief Creates a 2D rotation matrix around the Z-axis.
	 *
	 * This convenience function creates a 4x4 transformation matrix that represents
	 * a rotation around the Z-axis (out of the screen in a right-handed coordinate system).
	 * This is commonly used for 2D rotations in UI elements, sprites, or 2D games.
	 *
	 * @param degrees The rotation angle in degrees. Positive values rotate counter-clockwise.
	 *
	 * @return Matrix4x4 A 4x4 transformation matrix representing rotation around the Z-axis.
	 *
	 * @note - This function internally creates a quaternion and converts it to a matrix.
	 * @note - For multiple rotations, consider using glm::quat directly for better performance.
	 *
	 * @code
	 * Matrix4x4 rotation = Matrix4x4::Angle(45.0f); // 45° degree rotation
	 * @endcode
	 */
	Mat4 Mat4::Angle(const float degrees)
    {
        return Quat::EulerDegrees(0, 0, degrees).ToMatrix();
    }

	/**
	 * @brief Creates a 3D rotation matrix from Euler angles in degrees.
	 *
	 * This function creates a 4x4 transformation matrix representing a 3D rotation
	 * defined by Euler angles. The rotation order is YXZ (yaw-pitch-roll), which
	 * is commonly used in games and 3D applications.
	 *
	 * @param eulerDegrees A Vec3 containing the rotation angles in degrees:
	 *                     - x: Pitch (rotation around X-axis)
	 *                     - y: Yaw (rotation around Y-axis)
	 *                     - z: Roll (rotation around Z-axis)
	 *
	 * @return Matrix4x4 A 4x4 transformation matrix representing the combined rotation.
	 *
	 * @note - Rotation order matters. This function uses YXZ order to avoid gimbal lock in common cases.
	 * @note - For smooth interpolation between rotations, use quaternions directly.
	 *
	 * @code
	 * Vec3 rotation(15.0f, 45.0f, 0.0f); // 15° pitch, 45° yaw, 0° roll
	 * Matrix4x4 rotMatrix = Matrix4x4::RotationDegrees(rotation);
	 * @endcode
	 */
	Mat4 Mat4::RotationDegrees(const Vec3& eulerDegrees)
    {
        return Quat::EulerDegrees(eulerDegrees).ToMatrix();
    }

	/**
	 * @brief Creates a 3D rotation matrix from Euler angles in radians.
	 *
	 * This function creates a 4x4 transformation matrix representing a 3D rotation
	 * defined by Euler angles in radians. This is the radian equivalent of RotationDegrees().
	 *
	 * @param eulerRadians A Vec3 containing the rotation angles in radians:
	 *                     - x: Pitch (rotation around X-axis)
	 *                     - y: Yaw (rotation around Y-axis)
	 *                     - z: Roll (rotation around Z-axis)
	 *
	 * @return Matrix4x4 A 4x4 transformation matrix representing the combined rotation.
	 *
	 * @note - Rotation order is YXZ (yaw-pitch-roll).
	 * @note - Using radians directly can be more efficient when working with mathematical calculations.
	 *
	 * @code
	 * Vec3 rotation(0.262f, 0.785f, 0.0f); // ~15° pitch, ~45° yaw, 0° roll
	 * Matrix4x4 rotMatrix = Matrix4x4::RotationRadians(rotation);
	 * @endcode
	 */
	Mat4 Mat4::RotationRadians(const Vec3 &eulerRadians)
    {
        return Quat::EulerRadians(eulerRadians).ToMatrix();
    }

	/**
	 * @brief Matrix addition operator.
	 *
	 * Performs element-wise addition of two 4x4 matrices. Each corresponding element
	 * in the matrices is added together to produce the result matrix.
	 *
	 * @param rhs The right-hand side matrix to add to this matrix.
	 *
	 * @return Matrix4x4 A new matrix containing the sum of both matrices.
	 *
	 * @note - This operation is commutative: A + B = B + A.
	 * @note - The original matrices are not modified.
	 *
	 * @example
	 * @code
	 * Matrix4x4 result = matrixA + matrixB;
	 * @endcode
	 */
	Mat4 Mat4::operator+(const Mat4& rhs) const
	{
		Mat4 result{};

		for (int i = 0; i < 4; i++)
            result.rows[i] = rows[i] + rhs.rows[i];

        return result;
	}

	/**
	 * @brief Matrix subtraction operator.
	 *
	 * Performs element-wise subtraction of two 4x4 matrices. Each element in the
	 * right-hand side matrix is subtracted from the corresponding element in this matrix.
	 *
	 * @param rhs The right-hand side matrix to subtract from this matrix.
	 *
	 * @return Matrix4x4 A new matrix containing the difference (this - rhs).
	 *
	 * @note - This operation is not commutative: A - B ≠ B - A.
	 * @note - The original matrices are not modified.
	 *
	 * @example
	 * @code
	 * Matrix4x4 result = matrixA - matrixB;
	 * @endcode
	 */
	Mat4 Mat4::operator-(const Mat4& rhs) const
	{
		Mat4 result{};

		for (int i = 0; i < 4; i++)
            result.rows[i] = rows[i] - rhs.rows[i];

        return result;
	}

	/**
	 * @brief Scalar multiplication operator.
	 *
	 * Multiplies every element in the matrix by the given scalar value. This operation
	 * scales the transformation represented by the matrix uniformly.
	 *
	 * @param rhs The scalar value to multiply each matrix element by.
	 *
	 * @return Matrix4x4 A new matrix with all elements scaled by the scalar value.
	 *
	 * @note - This operation preserves the matrix structure while scaling the transformation.
	 * @note - The original matrix is not modified.
	 *
	 * @code
	 * Matrix4x4 scaled = originalMatrix * 2.0f; // Double all transformation components
	 * @endcode
	 */
	Mat4 Mat4::operator*(float rhs) const
	{
		ZoneScoped;
		Mat4 result{};

		for (int i = 0; i < 4; ++i)
            result.rows[i] = rows[i] * rhs;

        return result;
	}

	/**
	 * @brief Scalar division operator.
	 *
	 * Divides every element in the matrix by the given scalar value. This operation
	 * scales the transformation represented by the matrix by the reciprocal of the divisor.
	 *
	 * @param rhs The scalar value to divide each matrix element by. Must be non-zero.
	 *
	 * @return Matrix4x4 A new matrix with all elements divided by the scalar value.
	 *
	 * @note - This operation is equivalent to multiplying by 1/rhs.
	 * @note - The original matrix is not modified.
	 * @warning Division by zero will result in undefined behavior (infinity/NaN values).
	 *
	 * @code
	 * Matrix4x4 scaled = originalMatrix / 2.0f; // Halve all transformation components
	 * @endcode
	 */
	Mat4 Mat4::operator/(const float rhs) const
	{
		ZoneScoped;
		Mat4 result{};

		for (int i = 0; i < 4; ++i)
            result.rows[i] = rows[i] / rhs;

        return result;
	}

	/**
	 * @brief Matrix multiplication (Matrix × Matrix).
	 *
	 * Performs standard mathematical matrix multiplication between two 4x4 matrices.
	 * This operation combines two transformations, where the result represents applying
	 * the right-hand side transformation first, then the left-hand side transformation.
	 *
	 * The multiplication follows the standard formula:
	 * result[i][j] = Σ(k=0 to 3) lhs[i][k] * rhs[k][j]
	 *
	 * @param lhs The left-hand side matrix (applied second in transformation order).
	 * @param rhs The right-hand side matrix (applied first in transformation order).
	 *
	 * @return Matrix4x4 The resulting matrix from the multiplication lhs × rhs.
	 *
	 * @note - Matrix multiplication is NOT commutative: A × B ≠ B × A in general.
	 * @note - This function is static and can be called without a matrix instance.
	 * @note - Time complexity: O(64) - constant time for 4x4 matrices.
	 *
	 * @code
	 * Matrix4x4 transform = Matrix4x4::Multiply(projection, view);
	 * // Equivalent to: transform = projection * view (if operator* were defined)
	 * @endcode
	 */
	Mat4 Mat4::Multiply(const Mat4& lhs, const Mat4& rhs)
	{
		ZoneScoped;
		Mat4 result{};

		for (int i = 0; i < 4; i++)
		{
			for (int j = 0; j < 4; j++)
			{
				result.rows[i][j] = 0;

				for (int k = 0; k < 4; k++)
				{
					result.rows[i][j] += lhs.rows[i][k] * rhs.rows[k][j];
				}
			}
		}

		return result;
	}

	/**
	 * @brief Matrix-vector multiplication (Matrix × Vector).
	 *
	 * Transforms a 4D vector by multiplying it with a 4x4 matrix. This operation
	 * applies the transformation represented by the matrix to the vector, which
	 * is fundamental for transforming vertices, points, and directions in 3D space.
	 *
	 * The multiplication follows the standard formula:
	 * result[i] = Σ(j=0 to 3) lhs[i][j] * rhs[j]
	 *
	 * @param lhs The 4x4 transformation matrix.
	 * @param rhs The 4D vector to transform (x, y, z, w components).
	 *
	 * @return Vec4 The transformed vector after applying the matrix transformation.
	 *
	 * @note - For 3D points, use w=1.0f to include translation effects.
	 * @note - For 3D directions/normals, use w=0.0f to exclude translation effects.
	 * @note - This function is static and can be called without a matrix instance.
	 *
	 * @code
	 * Vec4 worldPos = Matrix4x4::Multiply(worldTransform, localPos);
	 * Vec4 clipPos = Matrix4x4::Multiply(projectionMatrix, viewPos);
	 * @endcode
	 */
	Vec4 Mat4::Multiply(const Mat4& lhs, const Vec4& rhs)
    {
		ZoneScoped;
        Vec4 result{};

        for (int i = 0; i < 4; i++)
        {
            float value = 0;
            for (int j = 0; j < 4; j++)
            {
                value += lhs.rows[i][j] * rhs[j];
            }

            result[i] = value;
        }

        return result;
    }

	/**
	 * @brief Computes the transpose of a 4x4 matrix.
	 *
	 * The transpose of a matrix is obtained by swapping rows and columns, such that
	 * element at position [i][j] becomes element at position [j][i]. For transformation
	 * matrices, the transpose is useful for inverse rotation calculations and converting
	 * between row-major and column-major representations.
	 *
	 * Mathematical definition: transpose(A)[i][j] = A[j][i]
	 *
	 * @param mat The source matrix to transpose.
	 *
	 * @return Matrix4x4 A new matrix that is the transpose of the input matrix.
	 *
	 * @note - For orthogonal matrices (pure rotations), the transpose equals the inverse.
	 * @note - This operation does not modify the input matrix.
	 * @note - Transposing twice returns the original matrix: transpose(transpose(A)) = A.
	 *
	 * @code
	 * Matrix4x4 rotation = Matrix4x4::RotationDegrees(45.0f, Vec3(0, 1, 0));
	 * Matrix4x4 inverse = Matrix4x4::GetTranspose(rotation); // For pure rotations
	 * @endcode
	 */
	Mat4 Mat4::GetTranspose(const Mat4& mat)
	{
		ZoneScoped;
		Mat4 result{};

		for (int i = 0; i < 4; i++)
		{
			for (int j = 0; j < 4; j++)
			{
				result.rows[i][j] = mat.rows[j][i];
			}
		}

		return result;
	}

	/**
	 * @brief Helper function to extract cofactor matrix for determinant calculation.
	 *
	 * This function extracts a (n-1)×(n-1) submatrix from the input matrix by excluding
	 * the specified row and column. This cofactor matrix is used in determinant calculations
	 * and matrix inversion algorithms using cofactor expansion.
	 *
	 * @param mat The source matrix to extract the cofactor from.
	 * @param cofactor Output matrix to store the extracted cofactor (modified by this function).
	 * @param p The row index to exclude from the source matrix (0-based).
	 * @param q The column index to exclude from the source matrix (0-based).
	 * @param n The size of the matrix (should be 4 for Matrix4x4).
	 *
	 * @note - This is a helper function for determinant and inverse calculations.
	 * @note - The output cofactor matrix will be one dimension smaller than the input.
	 */
	void Mat4::GetCofactor(const Mat4 &mat, Mat4 &cofactor, const int32_t p, const int32_t q, const int32_t n)
	{
		ZoneScoped;
		int32_t i = 0, j = 0;

		for (int row = 0; row < n; row++)
		{
			for (int col = 0; col < n; col++)
			{
				if (row != p && col != q)
				{
					cofactor[i][j++] = mat[row][col];

					/// Row is filled, so increase row index and
					/// reset col index
					if (j == n - 1)
					{
						j = 0;
						i++;
					}
				}
			}
		}
	}

	/**
	 * @brief Recursively calculates the determinant of a matrix using cofactor expansion.
	 *
	 * This function computes the determinant using the recursive cofactor expansion method.
	 * The determinant is a scalar value that indicates if the matrix is invertible (non-zero
	 * determinant) and is fundamental for calculating matrix inverses.
	 *
	 * For a 4×4 matrix, the determinant is calculated as:
	 * det(A) = Σ(i=0 to 3) (-1)^i * A[0][i] * det(Minor[0][i])
	 *
	 * @param mat The matrix to calculate the determinant for.
	 * @param n The size of the matrix (4 for Matrix4x4, decreases in recursive calls).
	 *
	 * @return int The determinant of the matrix.
	 *
	 * @note - Base case: for 1×1 matrix, returns the single element.
	 * @note - A determinant of 0 indicates the matrix is singular (non-invertible).
	 * @note - This function uses recursive calls and may be slower for large matrices.
	 */
	float Mat4::GetDeterminant(const Mat4& mat, const int32_t n)
	{
		ZoneScoped;
		float determinant = 0.0f;

		if (n == 1)
			return mat[0][0];

		Mat4 temp{};
		int sign = 1;

		/// Iterate for each element of first row
		for (int32_t f = 0; f < n; f++)
		{
			/// Getting Cofactor of A[0][f]
			GetCofactor(mat, temp, 0, f, n);
			determinant += static_cast<float>(sign) * mat[0][f] * GetDeterminant(temp, n - 1);

			/// terms are to be added with alternate sign
			sign = -sign;
		}

		return determinant;
	}

	/**
	 * @brief Calculates the adjoint (adjudicate) matrix.
	 *
	 * The adjoint matrix is the transpose of the cofactor matrix. Each element of the
	 * adjoint matrix is calculated as: adj[j][i] = (-1)^(i+j) * Minor[i][j], where
	 * Minor[i][j] is the determinant of the matrix obtained by deleting row i and column j.
	 *
	 * The adjoint matrix is used in matrix inversion: A^(-1) = adj(A) / det(A)
	 *
	 * @param mat The matrix to calculate the adjoint for.
	 *
	 * @return Matrix4x4 The adjoint matrix.
	 *
	 * @note - The adjoint matrix is also known as the adjudicate matrix.
	 * @note - For a 4×4 matrix, this involves calculating 16 determinants of 3×3 submatrices.
	 */
	Mat4 Mat4::GetAdjoint(const Mat4& mat)
	{
		ZoneScoped;
		Mat4 adj{};

		/// temp is used to store cofactors of mat[][]
		Mat4 temp{};

        for (int i = 0; i < 4; i++)
		{
			for (int j = 0; j < 4; j++)
			{
				/// Get cofactor of A[i][j]
				GetCofactor(mat, temp, i, j, 4);

				/// sign of adj[j][i] positive if sum of row
				/// and column indexes is even.
				int sign = ((i + j) % 2 == 0) ? 1 : -1;

				/// Interchanging rows and columns to get the
				/// transpose of the cofactor matrix
				adj[j][i] = sign * (GetDeterminant(temp, 4 - 1));
			}
		}

		return adj;
	}

	/**
	 * @brief Calculates the inverse of a 4×4 matrix using an optimized analytical method.
	 *
	 * This function computes the matrix inverse using a direct analytical approach that
	 * is more efficient than the adjoint/determinant method for 4×4 matrices. The inverse
	 * satisfies the property: A × A^(-1) = I (identity matrix).
	 *
	 * The implementation uses an optimized algorithm that directly calculates all elements
	 * of the inverse matrix using determinant ratios and cofactor relationships.
	 *
	 * @param matrix The matrix to invert.
	 *
	 * @return Matrix4x4 The inverse of the input matrix.
	 *
	 * @warning If the input matrix is singular (determinant = 0), the result is undefined
	 *          and may contain infinity or NaN values.
	 * @note - This method is significantly faster than the cofactor expansion method.
	 * @note - The input matrix must be non-singular for a valid result.
	 *
	 * @code
	 * Matrix4x4 original = Matrix4x4::Translation(Vec3(1, 2, 3));
	 * Matrix4x4 inverse = Matrix4x4::GetInverse(original);
	 * Matrix4x4 identity = Matrix4x4::Multiply(original, inverse); // Should be identity
	 * @endcode
	 */
	Mat4 Mat4::GetInverse(const Mat4& matrix)
	{
        ZoneScoped; /// Enable Tracy profiling for this function

		/// Rows - Extract the elements of the matrix for easier access
		const float n11 = matrix[0][0], n12 = matrix[1][0], n13 = matrix[2][0], n14 = matrix[3][0]; /// First row
        const float n21 = matrix[0][1], n22 = matrix[1][1], n23 = matrix[2][1], n24 = matrix[3][1]; /// Second row
        const float n31 = matrix[0][2], n32 = matrix[1][2], n33 = matrix[2][2], n34 = matrix[3][2]; /// Third row
        const float n41 = matrix[0][3], n42 = matrix[1][3], n43 = matrix[2][3], n44 = matrix[3][3]; /// Fourth row

		/// Columns - Calculate the determinant using the first row and cofactors
		const float t11 = n23 * n34 * n42 - n24 * n33 * n42 + n24 * n32 * n43 - n22 * n34 * n43 - n23 * n32 * n44 + n22 * n33 * n44; /// First column
		const float t12 = n14 * n33 * n42 - n13 * n34 * n42 - n14 * n32 * n43 + n12 * n34 * n43 + n13 * n32 * n44 - n12 * n33 * n44; /// Second column
		const float t13 = n13 * n24 * n42 - n14 * n23 * n42 + n14 * n22 * n43 - n12 * n24 * n43 - n13 * n22 * n44 + n12 * n23 * n44; /// Third column
		const float t14 = n14 * n23 * n32 - n13 * n24 * n32 - n14 * n22 * n33 + n12 * n24 * n33 + n13 * n22 * n34 - n12 * n23 * n34; /// Fourth column

		/// Calculate the determinant using the first row and cofactors
		const float det = n11 * t11 + n21 * t12 + n31 * t13 + n41 * t14; /// Determinant calculation
        const float idet = 1.0f / det;                                   /// Inverse of the determinant

		Mat4 ret; /// Result matrix to store the inverse

		/// Calculate the inverse matrix using the cofactors and the determinant
		ret[0][0] = t11 * idet;																											/// First row, first column
		ret[0][1] = (n24 * n33 * n41 - n23 * n34 * n41 - n24 * n31 * n43 + n21 * n34 * n43 + n23 * n31 * n44 - n21 * n33 * n44) * idet; /// First row, second column
		ret[0][2] = (n22 * n34 * n41 - n24 * n32 * n41 + n24 * n31 * n42 - n21 * n34 * n42 - n22 * n31 * n44 + n21 * n32 * n44) * idet; /// First row, third column
		ret[0][3] = (n23 * n32 * n41 - n22 * n33 * n41 - n23 * n31 * n42 + n21 * n33 * n42 + n22 * n31 * n43 - n21 * n32 * n43) * idet; /// First row, fourth column

		/// Calculate the second row
        ret[1][0] = t12 * idet;																											/// Second row, first column
		ret[1][1] = (n13 * n34 * n41 - n14 * n33 * n41 + n14 * n31 * n43 - n11 * n34 * n43 - n13 * n31 * n44 + n11 * n33 * n44) * idet; /// Second row, second column
		ret[1][2] = (n14 * n32 * n41 - n12 * n34 * n41 - n14 * n31 * n42 + n11 * n34 * n42 + n12 * n31 * n44 - n11 * n32 * n44) * idet; /// Second row, third column
		ret[1][3] = (n12 * n33 * n41 - n13 * n32 * n41 + n13 * n31 * n42 - n11 * n33 * n42 - n12 * n31 * n43 + n11 * n32 * n43) * idet; /// Second row, fourth column

		/// Calculate the third row
        ret[2][0] = t13 * idet;																											/// Third row, first column
		ret[2][1] = (n14 * n23 * n41 - n13 * n24 * n41 - n14 * n21 * n43 + n11 * n24 * n43 + n13 * n21 * n44 - n11 * n23 * n44) * idet; /// Third row, second column
		ret[2][2] = (n12 * n24 * n41 - n14 * n22 * n41 + n14 * n21 * n42 - n11 * n24 * n42 - n12 * n21 * n44 + n11 * n22 * n44) * idet; /// Third row, third column
		ret[2][3] = (n13 * n22 * n41 - n12 * n23 * n41 - n13 * n21 * n42 + n11 * n23 * n42 + n12 * n21 * n43 - n11 * n22 * n43) * idet; /// Third row, fourth column

		/// Calculate the fourth row
        ret[3][0] = t14 * idet;																											/// Fourth row, first column
		ret[3][1] = (n13 * n24 * n31 - n14 * n23 * n31 + n14 * n21 * n33 - n11 * n24 * n33 - n13 * n21 * n34 + n11 * n23 * n34) * idet; /// Fourth row, second column
		ret[3][2] = (n14 * n22 * n31 - n12 * n24 * n31 - n14 * n21 * n32 + n11 * n24 * n32 + n12 * n21 * n34 - n11 * n22 * n34) * idet; /// Fourth row, third column
		ret[3][3] = (n12 * n23 * n31 - n13 * n22 * n31 + n13 * n21 * n32 - n11 * n23 * n32 - n12 * n21 * n33 + n11 * n22 * n33) * idet; /// Fourth row, fourth column

		return ret; /// Return the calculated inverse matrix
	}

}

/// -------------------------------------------------------
