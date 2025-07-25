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
* -------------------------------------------------------
*/
#include <SceneryEditorX/utils/math/math_utils.h>
#include <SceneryEditorX/utils/math/matrix.h>
#include <SceneryEditorX/utils/math/quat.h>

#include <tracy/Tracy.hpp>

/// -------------------------------------------------------

namespace SceneryEditorX
{
    Matrix4x4 Matrix4x4::PerspectiveProjection(const float aspect, const float fieldOfView, const float n, const float f)
    {
		const float tanHalfFOV = tan(Utils::Math::Math::ToRadians(fieldOfView / 2.0f));

        return Matrix4x4({
			{ 1 / (aspect * tanHalfFOV), 0, 0, 0 },
			{ 0, -1 / tanHalfFOV, 0, 0 },
			{ 0, 0, f / (f - n), -f * n / (f - n) },
			{ 0, 0, 1, 0 }
		});
    }

	Matrix4x4 Matrix4x4::OrthographicProjection(float l, float r, float t, float b, float nearPlane, float farPlane)
	{
		return Matrix4x4({
			{ 2 / (r - l), 0, 0, -(r + l) / (r - l) },
			{ 0, 2 / (b - t), 0, -(b + t) / (b - t) },
			{ 0, 0, 1 / (farPlane - nearPlane), -nearPlane / (farPlane - nearPlane) },
			{ 0, 0, 0, 1 }
		});
	}

	Matrix4x4 Matrix4x4::Angle(const float degrees) { return Quat::EulerDegrees(0, 0, degrees).ToMatrix(); }

	Matrix4x4 Matrix4x4::RotationDegrees(const Vec3& eulerDegrees) { return Quat::EulerDegrees(eulerDegrees).ToMatrix(); }

	Matrix4x4 Matrix4x4::RotationRadians(const Vec3& eulerRadians) { return Quat::EulerRadians(eulerRadians).ToMatrix(); }

	Matrix4x4 Matrix4x4::operator+(const Matrix4x4& rhs) const
	{
		Matrix4x4 result{};
		
		for (int i = 0; i < 4; i++)
		{
			result.rows[i] = rows[i] + rhs.rows[i];
		}

		return result;
	}

	Matrix4x4 Matrix4x4::operator-(const Matrix4x4& rhs) const
	{
		Matrix4x4 result{};

		for (int i = 0; i < 4; i++)
		{
			result.rows[i] = rows[i] - rhs.rows[i];
		}

		return result;
	}

	Matrix4x4 Matrix4x4::operator*(float rhs) const
	{
		ZoneScoped;

		Matrix4x4 result{};

		for (auto &row : result.rows)
        {
            row *= rhs;
		}

		return result;
	}

	Matrix4x4 Matrix4x4::operator/(const float rhs) const
	{
		ZoneScoped;

		Matrix4x4 result{};

		for (auto &row : result.rows)
        {
            row /= rhs;
		}

		return result;
	}

	Matrix4x4 Matrix4x4::Multiply(const Matrix4x4& lhs, const Matrix4x4& rhs)
	{
		ZoneScoped;

		Matrix4x4 result{};

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

    Vec4 Matrix4x4::Multiply(const Matrix4x4& lhs, const Vec4& rhs)
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

	Matrix4x4 Matrix4x4::GetTranspose(const Matrix4x4& mat)
	{
		ZoneScoped;

		Matrix4x4 result{};

		for (int i = 0; i < 4; i++)
		{
			for (int j = 0; j < 4; j++)
			{
				result.rows[i][j] = mat.rows[j][i];
			}
		}

		return result;
	}

	void Matrix4x4::GetCofactor(const Matrix4x4 &mat, Matrix4x4 &cofactor, const int32_t p, const int32_t q, const int32_t n)
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

					// Row is filled, so increase row index and
					// reset col index
					if (j == n - 1) 
					{
						j = 0;
						i++;
					}
				}
			}
		}
	}

	int Matrix4x4::GetDeterminant(const Matrix4x4& mat, const int32_t n)
	{
		ZoneScoped;

		int determinant = 0;

		if (n == 1)
			return mat[0][0];

		Matrix4x4 temp{};

		int sign = 1;

		// Iterate for each element of first row
		for (int32_t f = 0; f < n; f++) 
		{
			// Getting Cofactor of A[0][f]
			GetCofactor(mat, temp, 0, f, n);
			determinant += sign * mat[0][f] * GetDeterminant(temp, n - 1);

			// terms are to be added with alternate sign
			sign = -sign;
		}

		return determinant;
	}

	Matrix4x4 Matrix4x4::GetAdjoint(const Matrix4x4& mat)
	{
		ZoneScoped;

		Matrix4x4 adj{};

		// temp is used to store cofactors of mat[][]
		Matrix4x4 temp{};

		int sign = 1;

		for (int i = 0; i < 4; i++) 
		{
			for (int j = 0; j < 4; j++) 
			{
				// Get cofactor of A[i][j]
				GetCofactor(mat, temp, i, j, 4);

				// sign of adj[j][i] positive if sum of row
				// and column indexes is even.
				sign = ((i + j) % 2 == 0) ? 1 : -1;

				// Interchanging rows and columns to get the
				// transpose of the cofactor matrix
				adj[j][i] = sign * (GetDeterminant(temp, 4 - 1));
			}
		}

		return adj;
	}

	Matrix4x4 Matrix4x4::GetInverse(const Matrix4x4& matx)
	{
		ZoneScoped;

		const float n11 = matx[0][0], n12 = matx[1][0], n13 = matx[2][0], n14 = matx[3][0];
		const float n21 = matx[0][1], n22 = matx[1][1], n23 = matx[2][1], n24 = matx[3][1];
		const float n31 = matx[0][2], n32 = matx[1][2], n33 = matx[2][2], n34 = matx[3][2];
		const float n41 = matx[0][3], n42 = matx[1][3], n43 = matx[2][3], n44 = matx[3][3];

		const float t11 = n23 * n34 * n42 - n24 * n33 * n42 + n24 * n32 * n43 - n22 * n34 * n43 - n23 * n32 * n44 + n22 * n33 * n44;
		const float t12 = n14 * n33 * n42 - n13 * n34 * n42 - n14 * n32 * n43 + n12 * n34 * n43 + n13 * n32 * n44 - n12 * n33 * n44;
		const float t13 = n13 * n24 * n42 - n14 * n23 * n42 + n14 * n22 * n43 - n12 * n24 * n43 - n13 * n22 * n44 + n12 * n23 * n44;
		const float t14 = n14 * n23 * n32 - n13 * n24 * n32 - n14 * n22 * n33 + n12 * n24 * n33 + n13 * n22 * n34 - n12 * n23 * n34;

		const float det = n11 * t11 + n21 * t12 + n31 * t13 + n41 * t14;
		const float idet = 1.0f / det;

		Matrix4x4 ret;

		ret[0][0] = t11 * idet;
		ret[0][1] = (n24 * n33 * n41 - n23 * n34 * n41 - n24 * n31 * n43 + n21 * n34 * n43 + n23 * n31 * n44 - n21 * n33 * n44) * idet;
		ret[0][2] = (n22 * n34 * n41 - n24 * n32 * n41 + n24 * n31 * n42 - n21 * n34 * n42 - n22 * n31 * n44 + n21 * n32 * n44) * idet;
		ret[0][3] = (n23 * n32 * n41 - n22 * n33 * n41 - n23 * n31 * n42 + n21 * n33 * n42 + n22 * n31 * n43 - n21 * n32 * n43) * idet;

		ret[1][0] = t12 * idet;
		ret[1][1] = (n13 * n34 * n41 - n14 * n33 * n41 + n14 * n31 * n43 - n11 * n34 * n43 - n13 * n31 * n44 + n11 * n33 * n44) * idet;
		ret[1][2] = (n14 * n32 * n41 - n12 * n34 * n41 - n14 * n31 * n42 + n11 * n34 * n42 + n12 * n31 * n44 - n11 * n32 * n44) * idet;
		ret[1][3] = (n12 * n33 * n41 - n13 * n32 * n41 + n13 * n31 * n42 - n11 * n33 * n42 - n12 * n31 * n43 + n11 * n32 * n43) * idet;

		ret[2][0] = t13 * idet;
		ret[2][1] = (n14 * n23 * n41 - n13 * n24 * n41 - n14 * n21 * n43 + n11 * n24 * n43 + n13 * n21 * n44 - n11 * n23 * n44) * idet;
		ret[2][2] = (n12 * n24 * n41 - n14 * n22 * n41 + n14 * n21 * n42 - n11 * n24 * n42 - n12 * n21 * n44 + n11 * n22 * n44) * idet;
		ret[2][3] = (n13 * n22 * n41 - n12 * n23 * n41 - n13 * n21 * n42 + n11 * n23 * n42 + n12 * n21 * n43 - n11 * n22 * n43) * idet;

		ret[3][0] = t14 * idet;
		ret[3][1] = (n13 * n24 * n31 - n14 * n23 * n31 + n14 * n21 * n33 - n11 * n24 * n33 - n13 * n21 * n34 + n11 * n23 * n34) * idet;
		ret[3][2] = (n14 * n22 * n31 - n12 * n24 * n31 - n14 * n21 * n32 + n11 * n24 * n32 + n12 * n21 * n34 - n11 * n22 * n34) * idet;
		ret[3][3] = (n12 * n23 * n31 - n13 * n22 * n31 + n13 * n21 * n32 - n11 * n23 * n32 - n12 * n21 * n33 + n11 * n22 * n33) * idet;

		return ret;
	}


}

/// -------------------------------------------------------
