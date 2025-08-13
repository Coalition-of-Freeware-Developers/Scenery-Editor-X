/**
* -------------------------------------------------------
* Scenery Editor X
* -------------------------------------------------------
* Copyright (c) 2025 Thomas Ray
* Copyright (c) 2025 Coalition of Freeware Developers
* -------------------------------------------------------
* matrix.h
* -------------------------------------------------------
* Created: 15/7/2025
* -------------------------------------------------------
*/
#pragma once
#include <initializer_list>
#include <string>
#include <cstdint>
#include <cstring>
#include <fmt/core.h>
#include <fmt/format.h>
#include <SceneryEditorX/core/base.hpp>
#include <SceneryEditorX/utils/math/vector.h>

/// -------------------------------------------------------

namespace SceneryEditorX
{
    /**
     * @class Matrix4x4
     * @brief A 4x4 matrix class for 3D transformations and mathematical operations.
     *
     * This class represents a 4x4 matrix commonly used in 3D graphics for transformations
     * such as translation, rotation, scaling, and projection. The matrix is stored in
     * row-major order and provides comprehensive mathematical operations and utility functions.
     *
     * The matrix layout is:
     * ```
     * [m00  m01  m02  m03]
     * [m10  m11  m12  m13]
     * [m20  m21  m22  m23]
     * [m30  m31  m32  m33]
     * ```
     *
     * For transformation matrices, the typical layout is:
     * - Upper-left 3x3: Rotation and scaling
     * - Fourth column (m03, m13, m23): Translation
     * - Fourth row (m30, m31, m32): Usually (0, 0, 0)
     * - Bottom-right (m33): Usually 1 for affine transformations
     */
    class Matrix4x4
    {
    public:

        /**
         * @brief Default constructor - creates a zero matrix.
         *
         * Initializes all matrix elements to zero. For an identity matrix,
         * use Matrix4x4::Identity() instead.
         */
        Matrix4x4() { memset(rows, 0, sizeof(rows)); }

        /**
         * @brief Constructs a matrix from an array of Vec4 rows.
         *
         * @param rows Array of 4 Vec4 objects representing the matrix rows.
         */
        Matrix4x4(Vec4 rows[4])
        {
            for (int i = 0; i < 4; i++)
                this->rows[i] = rows[i];
        }

        /**
         * @brief Constructs a matrix from an initializer list of Vec4 rows.
         *
         * @param rows Initializer list containing exactly 4 Vec4 objects.
         *
         * @example
         * @code
         * Matrix4x4 mat = {
         *     Vec4(1, 0, 0, 0),
         *     Vec4(0, 1, 0, 0),
         *     Vec4(0, 0, 1, 0),
         *     Vec4(0, 0, 0, 1)
         * };
         * @endcode
         */
        Matrix4x4(const std::initializer_list<Vec4> rows)
        {
            for (int i = 0; i < 4; i++)
                this->rows[i] = *(rows.begin() + i);
        }

		/**
		 * @brief Constructs a matrix from an initializer list of float values.
		 *
		 * Values are filled in row-major order. The list should contain 16 values.
		 *
		 * @param cells Initializer list of float values (should contain 16 elements).
		 *
		 * @note If fewer than 16 values are provided, remaining elements are initialized to zero.
		 *
		 * @example
		 * @code
		 * Matrix4x4 mat = {
		 *     1.0f, 0.0f, 0.0f, 0.0f,
		 *     0.0f, 1.0f, 0.0f, 0.0f,
		 *     0.0f, 0.0f, 1.0f, 0.0f,
		 *     0.0f, 0.0f, 0.0f, 1.0f
		 * };
		 * @endcode
		 */
		Matrix4x4(const std::initializer_list<float> cells)
		{
			memset(rows, 0, sizeof(rows));

            int i = 0;
            for (auto it = cells.begin(); it != cells.end() && i < 16; ++it, ++i)
			{
                const int row = i / 4;
                const int col = i % 4;
                rows[row][col] = *it;
			}
		}

        /**
         * @brief Copy constructor.
         *
         * @param copy The matrix to copy from.
         */
        Matrix4x4(const Matrix4x4& copy)
        {
            memcpy(rows, copy.rows, sizeof(rows));
        }

		/**
		 * @brief Creates a perspective projection matrix for 3D rendering.
		 *
		 * This function generates a perspective projection matrix that transforms 3D coordinates
		 * from view space (camera space) to clip space. The matrix implements a standard
		 * perspective projection with a symmetric frustum defined by field of view and aspect ratio.
		 *
		 * @param aspect The aspect ratio of the viewport (width / height). Must be positive.
		 * @param fieldOfView The vertical field of view angle in degrees. Typical values range from 45-90 degrees.
		 * @param nearPlane The distance to the near clipping plane. Must be positive and less than far plane.
		 * @param farPlane The distance to the far clipping plane. Must be positive and greater than near plane.
		 *
		 * @return Matrix4x4 A 4x4 perspective projection matrix suitable for use in graphics pipelines.
		 *
		 * @note The Y-axis is inverted (negative scale) to match common graphics coordinate systems.
		 * @note Objects closer than the near plane or farther than the far plane will be clipped.
		 * @warning Ensure near and far planes are positive and near < far to avoid degenerate matrices.
		 *
		 * @example
		 * @code
		 * float aspectRatio = screenWidth / screenHeight;
		 * Matrix4x4 projection = Matrix4x4::PerspectiveProjection(aspectRatio, 60.0f, 0.1f, 1000.0f);
		 * @endcode
		 */
		static Matrix4x4 PerspectiveProjection(float aspect, float fieldOfView, float nearPlane, float farPlane);

		/**
		 * @brief Creates an orthographic projection matrix for 2D rendering or technical drawing.
		 *
		 * This function generates an orthographic projection matrix that transforms 3D coordinates
		 * from view space to clip space without perspective distortion. Objects maintain their
		 * relative sizes regardless of distance from the camera, making it ideal for 2D rendering,
		 * UI elements, and technical/engineering drawings.
		 *
		 * @param left The left edge of the projection volume (minimum X coordinate).
		 * @param right The right edge of the projection volume (maximum X coordinate). Must be > left.
		 * @param top The top edge of the projection volume (maximum Y coordinate).
		 * @param bottom The bottom edge of the projection volume (minimum Y coordinate). Must be < top.
		 * @param nearPlane The distance to the near clipping plane. Can be negative.
		 * @param farPlane The distance to the far clipping plane. Must be != nearPlane.
		 *
		 * @return Matrix4x4 A 4x4 orthographic projection matrix suitable for 2D and isometric rendering.
		 *
		 * @note The coordinate system is right-handed with Y pointing up.
		 * @note Unlike perspective projection, parallel lines remain parallel after transformation.
		 * @warning Ensure right > left, top > bottom, and farPlane != nearPlane to avoid degenerate matrices.
		 *
		 * @example
		 * @code
		 * // Create orthographic projection for 2D rendering
		 * Matrix4x4 ortho = Matrix4x4::OrthographicProjection(0.0f, 800.0f, 600.0f, 0.0f, -1.0f, 1.0f);
		 * @endcode
		 */
		static Matrix4x4 OrthographicProjection(float left, float right, float top, float bottom, float nearPlane, float farPlane);

        /**
         * @brief Creates an orthographic projection matrix with symmetric frustum.
         *
         * This is a convenience overload that creates an orthographic projection matrix
         * with a symmetric frustum based on aspect ratio. The projection volume extends
         * from -aspect to +aspect horizontally and -1 to +1 vertically.
         *
         * @param aspect The aspect ratio (width/height) of the projection volume.
         * @param nearPlane The distance to the near clipping plane.
         * @param farPlane The distance to the far clipping plane.
         *
         * @return Matrix4x4 A symmetric orthographic projection matrix.
         *
         * @example
         * @code
         * Matrix4x4 ortho = Matrix4x4::OrthographicProjection(16.0f/9.0f, -1.0f, 1.0f);
         * @endcode
         */
        static Matrix4x4 OrthographicProjection(const float aspect, const float nearPlane, const float farPlane)
        {
            return OrthographicProjection(-aspect, aspect, -1, 1, nearPlane, farPlane);
        }

        /**
         * @brief Creates a translation (position) matrix.
         *
         * This function creates a 4x4 transformation matrix that represents a translation
         * (position offset) in 3D space. The resulting matrix can be used to move objects
         * from their original position to a new position.
         *
         * @param translation The translation vector specifying the offset in X, Y, and Z axes.
         *
         * @return Matrix4x4 A translation matrix that moves objects by the specified offset.
         *
         * @note The returned matrix is in the form:
         * ```
         * [1  0  0  tx]
         * [0  1  0  ty]
         * [0  0  1  tz]
         * [0  0  0  1 ]
         * ```
         * where (tx, ty, tz) is the translation vector.
         *
         * @example
         * @code
         * Vec3 position(10.0f, 5.0f, -3.0f);
         * Matrix4x4 translateMatrix = Matrix4x4::Translation(position);
         * @endcode
         */
        static Matrix4x4 Translation(const Vec3& translation)
        {
            return Matrix4x4({
                { 1, 0, 0, translation.x },
                { 0, 1, 0, translation.y },
                { 0, 0, 1, translation.z },
                { 0, 0, 0, 1 }
            });
        }

        /// @brief The matrix data stored as 4 rows of 4-component vectors.
        Vec4 rows[4];

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
         * @note This function internally creates a quaternion and converts it to a matrix.
         * @note For multiple rotations, consider using glm::quat directly for better performance.
         *
         * @example
         * @code
         * Matrix4x4 rotation = Matrix4x4::Angle(45.0f); // 45-degree  rotation
         * @endcode
         */
        static Matrix4x4 Angle(float degrees);

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
         * @note Rotation order matters. This function uses YXZ order to avoid gimbal lock in common cases.
         * @note For smooth interpolation between rotations, use quaternions directly.
         *
         * @example
         * @code
         * Vec3 rotation(15.0f, 45.0f, 0.0f); // 15° pitch, 45° yaw, 0° roll
         * Matrix4x4 rotMatrix = Matrix4x4::RotationDegrees(rotation);
         * @endcode
         */
        static Matrix4x4 RotationDegrees(const Vec3& eulerDegrees);

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
         * @note Rotation order is YXZ (yaw-pitch-roll).
         * @note Using radians directly can be more efficient when working with mathematical calculations.
         *
         * @example
         * @code
         * Vec3 rotation(0.262f, 0.785f, 0.0f); // ~15° pitch, ~45° yaw, 0° roll
         * Matrix4x4 rotMatrix = Matrix4x4::RotationRadians(rotation);
         * @endcode
         */
        static Matrix4x4 RotationRadians(const Vec3& eulerRadians);

        /**
         * @brief Creates a 2D scaling matrix.
         *
         * This function creates a 4x4 transformation matrix that scales objects in 2D space.
         * The Z-axis scaling is set to 1.0 (no scaling), making this suitable for 2D applications.
         *
         * @param scale A Vec2 containing the scaling factors for X and Y axes.
         *
         * @return Matrix4x4 A scaling matrix that scales objects by the specified factors.
         *
         * @note The resulting matrix has the form:
         * ```
         * [sx  0   0  0]
         * [0   sy  0  0]
         * [0   0   1  0]
         * [0   0   0  1]
         * ```
         *
         * @example
         * @code
         * Vec2 scale(2.0f, 0.5f); // Double width, halve height
         * Matrix4x4 scaleMatrix = Matrix4x4::Scale(scale);
         * @endcode
         */
        static Matrix4x4 Scale(const Vec2& scale)
        {
            return Matrix4x4({
                { scale.x,  0,       0,     0 },
                { 0,        scale.y, 0,     0 },
                { 0,        0,     1,     0 },
                { 0,        0,     0,     1 }
                });
        }

        /**
         * @brief Creates a 3D scaling matrix.
         *
         * This function creates a 4x4 transformation matrix that scales objects uniformly
         * or non-uniformly in 3D space along the X, Y, and Z axes.
         *
         * @param scale A Vec3 containing the scaling factors for each axis.
         *
         * @return Matrix4x4 A scaling matrix that scales objects by the specified factors.
         *
         * @note The resulting matrix has the form:
         * ```
         * [sx  0   0   0]
         * [0   sy  0   0]
         * [0   0   sz  0]
         * [0   0   0   1]
         * ```
         *
         * @example
         * @code
         * Vec3 scale(2.0f, 1.0f, 0.5f); // Double X, keep Y, halve Z
         * Matrix4x4 scaleMatrix = Matrix4x4::Scale(scale);
         * @endcode
         */
        static Matrix4x4 Scale(const Vec3& scale)
        {
            return Matrix4x4({
                { scale.x,  0,       0,       0 },
                { 0,        scale.y, 0,       0 },
                { 0,        0,       scale.z, 0 },
                { 0,        0,       0,       1 }
            });
        }

        /**
         * @brief Creates a zero matrix.
         *
         * This function creates a 4x4 matrix with all elements set to zero.
         * This is useful for initializing matrices that will be built up incrementally
         * or for mathematical operations that require a zero matrix.
         *
         * @return Matrix4x4 A matrix with all elements set to 0.
         *
         * @note A zero matrix represents no transformation and will nullify any
         *       point or vector it's multiplied with.
         *
         * @example
         * @code
         * Matrix4x4 zero = Matrix4x4::Zero();
         * @endcode
         */
        static Matrix4x4 Zero()
        {
            static Vec4 rows[4] = {
                Vec4{ 0, 0, 0, 0 },
                Vec4{ 0, 0, 0, 0 },
                Vec4{ 0, 0, 0, 0 },
                Vec4{ 0, 0, 0, 0 }
            };

            return {rows};
        }

        /**
         * @brief Creates an identity matrix.
         *
         * This function creates a 4x4 identity matrix, which represents no transformation.
         * When multiplied with any point or vector, the identity matrix returns the
         * original unchanged point or vector.
         *
         * @return Matrix4x4 A 4x4 identity matrix.
         *
         * @note The identity matrix has the form:
         * ```
         * [1  0  0  0]
         * [0  1  0  0]
         * [0  0  1  0]
         * [0  0  0  1]
         * ```
         *
         * @example
         * @code
         * Matrix4x4 identity = Matrix4x4::Identity();
         * @endcode
         */
        static Matrix4x4 Identity()
        {
            static Vec4 rows[4] = {
                Vec4{ 1, 0, 0, 0 },
                Vec4{ 0, 1, 0, 0 },
                Vec4{ 0, 0, 1, 0 },
                Vec4{ 0, 0, 0, 1 }
            };

            return {rows};
        }

        /**
         * @brief Subscript operator for non-const matrix access.
         *
         * Provides access to individual rows of the matrix for reading and writing.
         * Each row is a Vec4 representing the four columns of that row.
         *
         * @param index The row index (0-3).
         * @return Vec4& Reference to the specified row.
         *
         * @warning No bounds checking is performed. Index must be in range [0, 3].
         *
         * @example
         * @code
         * Matrix4x4 mat;
         * mat[0] = Vec4(1, 0, 0, 0); // Set first row
         * mat[1][2] = 5.0f;          // Set element at row 1, column 2
         * @endcode
         */
        Vec4& operator[](const int index) { return rows[index]; }

        /**
         * @brief Subscript operator for const matrix access.
         *
         * Provides read-only access to individual rows of the matrix.
         * Each row is a Vec4 representing the four columns of that row.
         *
         * @param index The row index (0-3).
         * @return const Vec4& Const reference to the specified row.
         *
         * @warning No bounds checking is performed. Index must be in range [0, 3].
         *
         * @example
         * @code
         * const Matrix4x4 mat = Matrix4x4::Identity();
         * Vec4 firstRow = mat[0];     // Get first row
         * float element = mat[1][2];  // Get element at row 1, column 2
         * @endcode
         */
        const Vec4& operator[](const int index) const { return rows[index]; }

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
         * @note This operation is commutative: A + B = B + A.
         * @note The original matrices are not modified.
         *
         * @example
         * @code
         * Matrix4x4 result = matrixA + matrixB;
         * @endcode
         */
        Matrix4x4 operator+(const Matrix4x4& rhs) const;

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
         * @note This operation is not commutative: A - B ≠ B - A.
         * @note The original matrices are not modified.
         *
         * @example
         * @code
         * Matrix4x4 result = matrixA - matrixB;
         * @endcode
         */
        Matrix4x4 operator-(const Matrix4x4& rhs) const;

        /**
         * @brief Matrix addition assignment operator.
         *
         * Adds the right-hand side matrix to this matrix and stores the result in this matrix.
         * This is equivalent to `*this = *this + rhs`.
         *
         * @param rhs The matrix to add to this matrix.
         * @return Matrix4x4& Reference to this matrix after the addition.
         *
         * @example
         * @code
         * matrixA += matrixB; // matrixA is modified
         * @endcode
         */
        Matrix4x4 operator+=(const Matrix4x4& rhs)
        {
            *this = *this + rhs;
            return *this;
        }

        /**
         * @brief Matrix subtraction assignment operator.
         *
         * Subtracts the right-hand side matrix from this matrix and stores the result in this matrix.
         * This is equivalent to `*this = *this - rhs`.
         *
         * @param rhs The matrix to subtract from this matrix.
         * @return Matrix4x4& Reference to this matrix after the subtraction.
         *
         * @example
         * @code
         * matrixA -= matrixB; // matrixA is modified
         * @endcode
         */
        Matrix4x4 operator-=(const Matrix4x4& rhs)
        {
            *this = *this - rhs;
            return *this;
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
         * @note This operation preserves the matrix structure while scaling the transformation.
         * @note The original matrix is not modified.
         *
         * @example
         * @code
         * Matrix4x4 scaled = originalMatrix * 2.0f; // Double all transformation components
         * @endcode
         */
        Matrix4x4 operator*(float rhs) const;

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
         * @note This operation is equivalent to multiplying by 1/rhs.
         * @note The original matrix is not modified.
         * @warning Division by zero will result in undefined behavior (infinity/NaN values).
         *
         * @example
         * @code
         * Matrix4x4 scaled = originalMatrix / 2.0f; // Halve all transformation components
         * @endcode
         */
        Matrix4x4 operator/(float rhs) const;

        /**
         * @brief Scalar multiplication assignment operator.
         *
         * Multiplies every element in this matrix by the given scalar value and stores
         * the result in this matrix. This is equivalent to `*this = *this * rhs`.
         *
         * @param rhs The scalar value to multiply each matrix element by.
         * @return Matrix4x4& Reference to this matrix after the multiplication.
         *
         * @example
         * @code
         * matrix *= 2.0f; // matrix is modified
         * @endcode
         */
        Matrix4x4 operator*=(const float rhs)
        {
            *this = *this * rhs;
            return *this;
        }

        /**
         * @brief Scalar division assignment operator.
         *
         * Divides every element in this matrix by the given scalar value and stores
         * the result in this matrix. This is equivalent to `*this = *this / rhs`.
         *
         * @param rhs The scalar value to divide each matrix element by. Must be non-zero.
         * @return Matrix4x4& Reference to this matrix after the division.
         *
         * @warning Division by zero will result in undefined behavior.
         *
         * @example
         * @code
         * matrix /= 2.0f; // matrix is modified
         * @endcode
         */
        Matrix4x4 operator/=(const float rhs)
        {
            *this = *this / rhs;
            return *this;
        }

        /**
         * @brief Matrix multiplication operator.
         *
         * Performs standard mathematical matrix multiplication between this matrix and
         * the right-hand side matrix. This operation combines two transformations.
         *
         * @param rhs The right-hand side matrix to multiply with.
         * @return Matrix4x4 The resulting matrix from the multiplication.
         *
         * @note Matrix multiplication is NOT commutative: A × B ≠ B × A in general.
         * @note The transformation order is: rhs applied first, then this matrix.
         *
         * @example
         * @code
         * Matrix4x4 result = transformA * transformB;
         * @endcode
         */
        Matrix4x4 operator*(const Matrix4x4& rhs) const { return Multiply(*this, rhs); }

        /**
         * @brief Matrix-vector multiplication operator (4D vector).
         *
         * Transforms a 4D vector by multiplying it with this matrix. This operation
         * applies the transformation represented by the matrix to the vector.
         *
         * @param rhs The 4D vector to transform.
         * @return Vec4 The transformed vector.
         *
         * @note For 3D points, use w=1.0f to include translation effects.
         * @note For 3D directions/normals, use w=0.0f to exclude translation effects.
         *
         * @example
         * @code
         * Vec4 transformedPoint = transformMatrix * Vec4(position, 1.0f);
         * @endcode
         */
        Vec4 operator*(const Vec4& rhs) const { return Multiply(*this, rhs); }

		/**
		 * @brief Matrix-vector multiplication operator (3D vector).
		 *
		 * Transforms a 3D vector by treating it as a 4D vector with w=1.0f, then
		 * multiplying it with this matrix. This includes translation effects.
		 *
		 * @param rhs The 3D vector to transform (treated as a point).
		 * @return Vec4 The transformed vector as a 4D vector.
		 *
		 * @note The input 3D vector is automatically converted to Vec4(x, y, z, 1.0f).
		 * @note Use this for transforming 3D points that should be affected by translation.
		 *
		 * @example
		 * @code
		 * Vec4 transformedPoint = transformMatrix * Vec3(1.0f, 2.0f, 3.0f);
		 * @endcode
		 */
		Vec4 operator*(const Vec3& rhs) const { return Multiply(*this, Vec4(rhs, 1.0f)); }

        /**
         * @brief Matrix multiplication assignment operator.
         *
         * Multiplies this matrix with the right-hand side matrix and stores the result
         * in this matrix. This is equivalent to `*this = *this * rhs`.
         *
         * @param rhs The matrix to multiply with this matrix.
         * @return Matrix4x4& Reference to this matrix after the multiplication.
         *
         * @example
         * @code
         * transformMatrix *= rotationMatrix; // transformMatrix is modified
         * @endcode
         */
        Matrix4x4 operator*=(const Matrix4x4& rhs)
        {
            *this = *this * rhs;
            return *this;
        }

        /**
         * @brief Matrix equality operator.
         *
         * Compares two matrices for equality by comparing all corresponding elements.
         * Two matrices are considered equal if all their elements are exactly equal.
         *
         * @param rhs The matrix to compare with this matrix.
         * @return bool True if matrices are equal, false otherwise.
         *
         * @note This performs exact floating-point comparison, which may not be suitable
         *       for matrices that have undergone floating-point calculations.
         * @note Consider using an epsilon-based comparison for more robust equality testing.
         *
         * @example
         * @code
         * if (matrixA == matrixB) {
         *     // Matrices are identical
         * }
         * @endcode
         */
        bool operator==(const Matrix4x4& rhs) const { return rows[0] == rhs.rows[0] && rows[1] == rhs.rows[1] && rows[2] == rhs.rows[2] && rows[3] == rhs.rows[3]; }

        /**
         * @brief Matrix inequality operator.
         *
         * Compares two matrices for inequality. Returns true if the matrices are not equal.
         * This is the logical negation of the equality operator.
         *
         * @param rhs The matrix to compare with this matrix.
         * @return bool True if matrices are not equal, false if they are equal.
         *
         * @example
         * @code
         * if (matrixA != matrixB) {
         *     // Matrices are different
         * }
         * @endcode
         */
        bool operator!=(const Matrix4x4& rhs) const { return !(*this == rhs); }

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
         * @note Matrix multiplication is NOT commutative: A × B ≠ B × A in general.
         * @note This function is static and can be called without a matrix instance.
         * @note Time complexity: O(64) - constant time for 4x4 matrices.
         *
         * @example
         * @code
         * Matrix4x4 transform = Matrix4x4::Multiply(projection, view);
         * // Equivalent to: transform = projection * view (if operator* were defined)
         * @endcode
         */
        static Matrix4x4 Multiply(const Matrix4x4& lhs, const Matrix4x4& rhs);

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
         * @note For 3D points, use w=1.0f to include translation effects.
         * @note For 3D directions/normals, use w=0.0f to exclude translation effects.
         * @note This function is static and can be called without a matrix instance.
         *
         * @example
         * @code
         * Vec4 worldPos = Matrix4x4::Multiply(worldTransform, localPos);
         * Vec4 clipPos = Matrix4x4::Multiply(projectionMatrix, viewPos);
         * @endcode
         */
        static Vec4 Multiply(const Matrix4x4& lhs, const Vec4& rhs);

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
         * @note For orthogonal matrices (pure rotations), the transpose equals the inverse.
         * @note This operation does not modify the input matrix.
         * @note Transposing twice returns the original matrix: transpose(transpose(A)) = A.
         *
         * @example
         * @code
         * Matrix4x4 rotation = Matrix4x4::RotationDegrees(45.0f, Vec3(0, 1, 0));
         * Matrix4x4 inverse = Matrix4x4::GetTranspose(rotation); // For pure rotations
         * @endcode
         */
        static Matrix4x4 GetTranspose(const Matrix4x4& mat);

        /**
         * @brief Transposes this matrix in-place.
         *
         * Modifies this matrix by swapping its rows and columns. This is equivalent
         * to `*this = GetTranspose(*this)` but may be more efficient.
         *
         * @note This operation modifies the current matrix.
         *
         * @example
         * @code
         * matrix.Transpose(); // matrix is modified
         * @endcode
         */
        void Transpose() { *this = GetTranspose(*this); }

        /**
         * @brief Gets the transpose of this matrix without modifying it.
         *
         * Returns a new matrix that is the transpose of this matrix, leaving
         * the original matrix unchanged.
         *
         * @return Matrix4x4 The transpose of this matrix.
         *
         * @example
         * @code
         * Matrix4x4 transposed = matrix.GetTranspose(); // matrix is not modified
         * @endcode
         */
        [[nodiscard]] Matrix4x4 GetTranspose() const { return GetTranspose(*this); }

        /**
         * @brief Gets the inverse of this matrix without modifying it.
         *
         * Computes and returns the mathematical inverse of this matrix. The inverse
         * of a matrix A is a matrix A⁻¹ such that A × A⁻¹ = I (identity matrix).
         *
         * @return Matrix4x4 The inverse of this matrix.
         *
         * @note If the matrix is singular (determinant = 0), the result is undefined.
         * @note The inverse of a transformation matrix can be used to "undo" transformations.
         * @note This operation does not modify the current matrix.
         *
         * @example
         * @code
         * Matrix4x4 inverse = transform.GetInverse();
         * @endcode
         */
        [[nodiscard]] Matrix4x4 GetInverse() const { return GetInverse(*this); }

        /**
         * @brief Inverts this matrix in-place.
         *
         * Modifies this matrix by replacing it with its mathematical inverse.
         * This is equivalent to `*this = GetInverse(*this)`.
         *
         * @note This operation modifies the current matrix.
         * @warning If the matrix is singular, the result is undefined.
         *
         * @example
         * @code
         * transform.Invert(); // transform is modified
         * @endcode
         */
        void Invert() { *this = GetInverse(*this); }

        /**
         * @brief Converts the matrix to a formatted string representation.
         *
         * Creates a human-readable string representation of the matrix with all
         * elements displayed in a 4x4 grid format. Useful for debugging and logging.
         *
         * @return std::string A formatted string showing all matrix elements.
         *
         * @example
         * @code
         * Matrix4x4 mat = Matrix4x4::Identity();
         * std::cout << mat.ToString() << std::endl;
         * // Output:
         * // [1 0 0 0]
         * // [0 1 0 0]
         * // [0 0 1 0]
         * // [0 0 0 1]
         * @endcode
         */
        [[nodiscard]] std::string ToString() const
        {
            return fmt::format("[{} {} {} {}]\n[{} {} {} {}]\n[{} {} {} {}]\n[{} {} {} {}]",
                rows[0][0], rows[0][1], rows[0][2], rows[0][3],
                rows[1][0], rows[1][1], rows[1][2], rows[1][3],
                rows[2][0], rows[2][1], rows[2][2], rows[2][3],
                rows[3][0], rows[3][1], rows[3][2], rows[3][3]);
        }

    private:
        /**
         * @brief Helper function to get the cofactor matrix.
         *
         * Extracts the cofactor matrix by removing the specified row and column
         * from the input matrix. This is used in determinant and inverse calculations.
         *
         * @param mat The source matrix to extract cofactor from.
         * @param cofactor Output matrix to store the cofactor.
         * @param p The row to exclude (0-based index).
         * @param q The column to exclude (0-based index).
         * @param n The size of the matrix (should be 4 for Matrix4x4).
         */
        static void GetCofactor(const Matrix4x4 &mat, Matrix4x4 &cofactor, int32_t p, int32_t q, int32_t n);

        /**
         * @brief Calculates the determinant of a matrix.
         *
         * Computes the determinant using cofactor expansion. The determinant
         * indicates whether the matrix is invertible (non-zero determinant)
         * and is used in inverse matrix calculations.
         *
         * @param mat The matrix to calculate the determinant for.
         * @param n The size of the matrix (should be 4 for Matrix4x4).
         *
         * @return int The determinant of the matrix.
         *
         * @note A determinant of 0 indicates the matrix is singular (non-invertible).
         */
    	static float GetDeterminant(const Matrix4x4 &mat, int32_t n);

        /**
         * @brief Calculates the adjoint (adjudicate) matrix.
         *
         * The adjoint matrix is the transpose of the cofactor matrix. It's used
         * in the calculation of the matrix inverse using the formula:
         * A⁻¹ = adj(A) / det(A)
         *
         * @param mat The matrix to calculate the adjoint for.
         *
         * @return Matrix4x4 The adjoint matrix.
         */
        static Matrix4x4 GetAdjoint(const Matrix4x4 &mat);

        /**
         * @brief Calculates the inverse of a matrix using analytical methods.
         *
         * Computes the matrix inverse using an optimized analytical approach
         * rather than the cofactor method. This implementation is more efficient
         * for 4x4 matrices and handles the calculation directly.
         *
         * @param mat The matrix to invert.
         *
         * @return Matrix4x4 The inverse matrix.
         *
         * @note If the input matrix is singular (determinant = 0), the result is undefined.
         * @note This is the actual implementation used by GetInverse().
         */
        static Matrix4x4 GetInverse(const Matrix4x4 &mat);
    };

}

/// fmt user-defined Formatter for SceneryEditorX::Matrix4x4
template <>
struct fmt::formatter<SceneryEditorX::Matrix4x4>
{
    /// Parses format specifications of the form ['f' | 'e'].
    static constexpr auto parse(const format_parse_context &ctx) -> decltype(ctx.begin())
    {
        /// Return an iterator past the end of the parsed range:
        return ctx.end();
    }

    /// Formats the point p using the parsed format specification (presentation)
    /// stored in this formatter.
    template <typename FormatContext>
    auto format(const SceneryEditorX::Matrix4x4 &mat, FormatContext &ctx) const -> decltype(ctx.out())
    {
        /// ctx.out() is an output iterator to write to.
        return fmt::format_to(ctx.out(), "{}", mat.ToString());
    }
};

/// -------------------------------------------------------
