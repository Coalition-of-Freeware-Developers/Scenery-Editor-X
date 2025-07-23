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
#include <format>

/// -------------------------------------------------------

namespace SceneryEditorX
{
    class Matrix4x4
    {
    public:

        Matrix4x4() { memset(rows, 0, sizeof(rows)); }

        Matrix4x4(Vec4 rows[4])
        {
            for (int i = 0; i < 4; i++)
                this->rows[i] = rows[i];
        }

        Matrix4x4(const std::initializer_list<Vec4> rows)
        {
            for (int i = 0; i < 4; i++)
                this->rows[i] = *(rows.begin() + i);
        }

		Matrix4x4(const std::initializer_list<float> cells)
		{
			memset(rows, 0, sizeof(rows));

			for (int i = 0; i < cells.size(); i++)
			{
				const int row = cells.size() / 4;
				const int col = cells.size() % 4;

				rows[row][col] = *(cells.begin() + i);
			}
		}

        Matrix4x4(const Matrix4x4& copy) { memcpy(rows, copy.rows, sizeof(rows)); }

		static Matrix4x4 PerspectiveProjection(float aspect, float fieldOfView, float nearPlane, float farPlane);
		static Matrix4x4 OrthographicProjection(float left, float right, float top, float bottom, float nearPlane, float farPlane);
        static Matrix4x4 OrthographicProjection(const float aspect, const float nearPlane, const float farPlane) { return OrthographicProjection(-aspect, aspect, -1, 1, nearPlane, farPlane); }

        static Matrix4x4 Translation(const Vec3& translation)
        {
            return Matrix4x4({
                { 1, 0, 0, translation.x },
                { 0, 1, 0, translation.y },
                { 0, 0, 1, translation.z },
                { 0, 0, 0, 1          }
            });
        }

        Vec4 rows[4];

        //! @brief 2D angle in Z axis in degrees.
        static Matrix4x4 Angle(float degrees);

        //! @brief 3D euler angles in degrees 
        static Matrix4x4 RotationDegrees(const Vec3& eulerDegrees);

        //! @brief 3D euler angles in radians
        static Matrix4x4 RotationRadians(const Vec3& eulerRadians);

        static Matrix4x4 Scale(const Vec2& scale)
        {
            return Matrix4x4({
                { scale.x,  0,       0,     0 },
                { 0,        scale.y, 0,     0 },
                { 0,        0,     1,     0 },
                { 0,        0,     0,     1 }
                });
        }

        static Matrix4x4 Scale(const Vec3& scale)
        {
            return Matrix4x4({
                { scale.x,  0,       0,       0 },
                { 0,        scale.y, 0,       0 },
                { 0,        0,       scale.z, 0 },
                { 0,        0,       0,       1 }
            });
        }

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

        Vec4& operator[](const int index) { return rows[index]; }
        const Vec4& operator[](const int index) const { return rows[index]; }

        Matrix4x4 operator+(const Matrix4x4& rhs) const;
        Matrix4x4 operator-(const Matrix4x4& rhs) const;

        Matrix4x4 operator+=(const Matrix4x4& rhs)
        {
            *this = *this + rhs;
            return *this;
        }

        Matrix4x4 operator-=(const Matrix4x4& rhs)
        {
            *this = *this - rhs;
            return *this;
        }

        Matrix4x4 operator*(float rhs) const;
        Matrix4x4 operator/(float rhs) const;

        Matrix4x4 operator*=(const float rhs)
        {
            *this = *this * rhs;
            return *this;
        }

        Matrix4x4 operator/=(const float rhs)
        {
            *this = *this / rhs;
            return *this;
        }

        Matrix4x4 operator*(const Matrix4x4& rhs) const { return Multiply(*this, rhs); }
        
        Vec4 operator*(const Vec4& rhs) const { return Multiply(*this, rhs); }
		Vec4 operator*(const Vec3& rhs) const { return Multiply(*this, Vec4(rhs, 1.0f)); }

        Matrix4x4 operator*=(const Matrix4x4& rhs)
        {
            *this = *this * rhs;
            return *this;
        }

        bool operator==(const Matrix4x4& rhs) const { return rows[0] == rhs.rows[0] && rows[1] == rhs.rows[1] && rows[2] == rhs.rows[2] && rows[3] == rhs.rows[3]; }
        bool operator!=(const Matrix4x4& rhs) const { return !(*this == rhs); }

        static Matrix4x4 Multiply(const Matrix4x4& lhs, const Matrix4x4& rhs);
        static Vec4 Multiply(const Matrix4x4& lhs, const Vec4& rhs);
        static Matrix4x4 GetTranspose(const Matrix4x4& mat);
        void Transpose() { *this = GetTranspose(*this); }
        [[nodiscard]] Matrix4x4 GetTranspose() const { return GetTranspose(*this); }
        [[nodiscard]] Matrix4x4 GetInverse() const { return GetInverse(*this); }
        void Invert() { *this = GetInverse(*this); }

        [[nodiscard]] std::string ToString() const
        {
            return fmt::format("[{} {} {} {}]\n[{} {} {} {}]\n[{} {} {} {}]\n[{} {} {} {}]",
                rows[0][0], rows[0][1], rows[0][2], rows[0][3],
                rows[1][0], rows[1][1], rows[1][2], rows[1][3],
                rows[2][0], rows[2][1], rows[2][2], rows[2][3],
                rows[3][0], rows[3][1], rows[3][2], rows[3][3]);
        }

    private:
        /// Function to get determinant of mat[p][q]
        static void GetCofactor(const Matrix4x4 &mat, Matrix4x4 &cofactor, int32_t p, int32_t q, int32_t n);
        static int GetDeterminant(const Matrix4x4 &mat, int32_t n);
        static Matrix4x4 GetAdjoint(const Matrix4x4 &mat);
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
