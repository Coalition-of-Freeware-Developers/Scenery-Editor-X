/**
* -------------------------------------------------------
* Scenery Editor X
* -------------------------------------------------------
* Copyright (c) 2025 Thomas Ray
* Copyright (c) 2025 Coalition of Freeware Developers
* -------------------------------------------------------
* mat3.h
* -------------------------------------------------------
* Created: 12/8/2025
* -------------------------------------------------------
*/
#pragma once
#include <SceneryEditorX/utils/math/vector.h>
#include <SceneryEditorX/utils/math/epsilon.h>
#include <cmath>
#include <ostream>

/// -----------------------------------------------------

namespace SceneryEditorX
{
	/**
	 * @brief Fast 3x3 single-precision matrix (row-major) for 3D linear transforms (rotation/scale/skew).
	 *
	 * Layout (row-major):
	 * [ m00 m01 m02 ]
	 * [ m10 m11 m12 ]
	 * [ m20 m21 m22 ]
	 */
	class Mat3
	{
	public:
		float m00, m01, m02,
			  m10, m11, m12,
			  m20, m21, m22;

		using Vec3f = Utils::TVector3<float>;

		// -------------------------------------------------
		// Constructors
		constexpr Mat3() noexcept
			: m00(1), m01(0), m02(0),
			  m10(0), m11(1), m12(0),
			  m20(0), m21(0), m22(1) {}

		constexpr explicit Mat3(float s) noexcept
			: m00(s), m01(0), m02(0),
			  m10(0), m11(s), m12(0),
			  m20(0), m21(0), m22(s) {}

		constexpr Mat3(float _m00, float _m01, float _m02, float _m10, float _m11, float _m12, float _m20, float _m21, float _m22) noexcept
			: m00(_m00), m01(_m01), m02(_m02),
			  m10(_m10), m11(_m11), m12(_m12),
			  m20(_m20), m21(_m21), m22(_m22) {}

		// From rows / columns
		constexpr static Mat3 FromRows(const Vec3f& r0, const Vec3f& r1, const Vec3f& r2) noexcept
		{ return Mat3(r0.x, r0.y, r0.z, r1.x, r1.y, r1.z, r2.x, r2.y, r2.z); }

		constexpr static Mat3 FromColumns(const Vec3f& c0, const Vec3f& c1, const Vec3f& c2) noexcept
		{ return Mat3(c0.x, c1.x, c2.x, c0.y, c1.y, c2.y, c0.z, c1.z, c2.z); }

		// Identity / Zero
		constexpr static Mat3 Identity() noexcept { return Mat3(); }
		constexpr static Mat3 Zero() noexcept { return Mat3(0,0,0, 0,0,0, 0,0,0); }

		// Scale
		constexpr static Mat3 Scale(float sx, float sy, float sz) noexcept
		{ return Mat3(sx,0,0, 0,sy,0, 0,0,sz); }
		constexpr static Mat3 Scale(const Vec3f& s) noexcept
		{ return Scale(s.x, s.y, s.z); }

		// Rotation about principal axes (radians)
		static Mat3 RotationX(float r) noexcept
		{
			const float c = std::cos(r), s = std::sin(r);
			return Mat3(1,0,0, 0,c,-s, 0,s,c);
		}
		static Mat3 RotationY(float r) noexcept
		{
			const float c = std::cos(r), s = std::sin(r);
			return Mat3(c,0,s, 0,1,0, -s,0,c);
		}
		static Mat3 RotationZ(float r) noexcept
		{
			const float c = std::cos(r), s = std::sin(r);
			return Mat3(c,-s,0, s,c,0, 0,0,1);
		}
		static Mat3 RotationAxisAngle(const Vec3f& axis, float angle) noexcept
		{
			// Rodrigues' rotation formula
			const float c = std::cos(angle), s = std::sin(angle), t = 1.0f - c;
			const float x = axis.x, y = axis.y, z = axis.z;
			return Mat3(
				t*x*x + c,     t*x*y - s*z,   t*x*z + s*y,
				t*x*y + s*z,   t*y*y + c,     t*y*z - s*x,
				t*x*z - s*y,   t*y*z + s*x,   t*z*z + c
			);
		}

		// Accessors
		constexpr Vec3f Row(int r) const noexcept
		{ return r == 0 ? Vec3f{m00,m01,m02} : (r == 1 ? Vec3f{m10,m11,m12} : Vec3f{m20,m21,m22}); }
		constexpr Vec3f Col(int c) const noexcept
		{ return c == 0 ? Vec3f{m00,m10,m20} : (c == 1 ? Vec3f{m01,m11,m21} : Vec3f{m02,m12,m22}); }

		constexpr float& operator()(int r, int c) noexcept
		{
			switch (r*3 + c)
			{
				case 0: return m00; case 1: return m01; case 2: return m02;
				case 3: return m10; case 4: return m11; case 5: return m12;
				case 6: return m20; case 7: return m21; default: return m22;
			}
		}
		constexpr const float& operator()(int r, int c) const noexcept
		{
			switch (r*3 + c)
			{
				case 0: return m00; case 1: return m01; case 2: return m02;
				case 3: return m10; case 4: return m11; case 5: return m12;
				case 6: return m20; case 7: return m21; default: return m22;
			}
		}

		// -------------------------------------------------
		// Arithmetic
		constexpr Mat3 operator+(const Mat3& r) const noexcept
		{ return Mat3(m00+r.m00, m01+r.m01, m02+r.m02, m10+r.m10, m11+r.m11, m12+r.m12, m20+r.m20, m21+r.m21, m22+r.m22); }
		constexpr Mat3 operator-(const Mat3& r) const noexcept
		{ return Mat3(m00-r.m00, m01-r.m01, m02-r.m02, m10-r.m10, m11-r.m11, m12-r.m12, m20-r.m20, m21-r.m21, m22-r.m22); }
		constexpr Mat3 operator*(float s) const noexcept
		{ return Mat3(m00*s, m01*s, m02*s, m10*s, m11*s, m12*s, m20*s, m21*s, m22*s); }
		friend constexpr Mat3 operator*(float s, const Mat3& m) noexcept { return m * s; }

		constexpr Mat3& operator+=(const Mat3& r) noexcept
		{ m00+=r.m00; m01+=r.m01; m02+=r.m02; m10+=r.m10; m11+=r.m11; m12+=r.m12; m20+=r.m20; m21+=r.m21; m22+=r.m22; return *this; }
		constexpr Mat3& operator-=(const Mat3& r) noexcept
		{ m00-=r.m00; m01-=r.m01; m02-=r.m02; m10-=r.m10; m11-=r.m11; m12-=r.m12; m20-=r.m20; m21-=r.m21; m22-=r.m22; return *this; }
		constexpr Mat3& operator*=(float s) noexcept
		{ m00*=s; m01*=s; m02*=s; m10*=s; m11*=s; m12*=s; m20*=s; m21*=s; m22*=s; return *this; }

		// Matrix multiply
		constexpr Mat3 operator*(const Mat3& r) const noexcept
		{
			return Mat3(
				m00*r.m00 + m01*r.m10 + m02*r.m20,  m00*r.m01 + m01*r.m11 + m02*r.m21,  m00*r.m02 + m01*r.m12 + m02*r.m22,
				m10*r.m00 + m11*r.m10 + m12*r.m20,  m10*r.m01 + m11*r.m11 + m12*r.m21,  m10*r.m02 + m11*r.m12 + m12*r.m22,
				m20*r.m00 + m21*r.m10 + m22*r.m20,  m20*r.m01 + m21*r.m11 + m22*r.m21,  m20*r.m02 + m21*r.m12 + m22*r.m22
			);
		}
		constexpr Mat3& operator*=(const Mat3& r) noexcept { *this = (*this) * r; return *this; }

		// Vector multiply (column-vector convention)
		constexpr Vec3f operator*(const Vec3f& v) const noexcept
		{ return Vec3f{ m00*v.x + m01*v.y + m02*v.z, m10*v.x + m11*v.y + m12*v.z, m20*v.x + m21*v.y + m22*v.z }; }

		// -------------------------------------------------
		// Properties and helpers
		constexpr float Trace() const noexcept { return m00 + m11 + m22; }
		constexpr float Determinant() const noexcept
		{
			return m00*(m11*m22 - m12*m21) - m01*(m10*m22 - m12*m20) + m02*(m10*m21 - m11*m20);
		}
		constexpr Mat3 Transposed() const noexcept
		{ return Mat3(m00,m10,m20, m01,m11,m21, m02,m12,m22); }

		Mat3 Inversed(float eps = 1e-6f) const noexcept
		{
			const float det = Determinant();
			if (std::fabs(det) <= eps) return Mat3::Zero();
			const float invDet = 1.0f / det;
			// Adjugate (transpose of cofactor matrix)
			const float c00 =  (m11*m22 - m12*m21);
			const float c01 = -(m10*m22 - m12*m20);
			const float c02 =  (m10*m21 - m11*m20);
			const float c10 = -(m01*m22 - m02*m21);
			const float c11 =  (m00*m22 - m02*m20);
			const float c12 = -(m00*m21 - m01*m20);
			const float c20 =  (m01*m12 - m02*m11);
			const float c21 = -(m00*m12 - m02*m10);
			const float c22 =  (m00*m11 - m01*m10);
			return Mat3(
				c00*invDet, c10*invDet, c20*invDet,
				c01*invDet, c11*invDet, c21*invDet,
				c02*invDet, c12*invDet, c22*invDet
			);
		}
		bool TryInverse(Mat3& out, float eps = 1e-6f) const noexcept
		{
			const float det = Determinant();
			if (std::fabs(det) <= eps) { out = Mat3::Zero(); return false; }
			const float invDet = 1.0f / det;
			const float c00 =  (m11*m22 - m12*m21);
			const float c01 = -(m10*m22 - m12*m20);
			const float c02 =  (m10*m21 - m11*m20);
			const float c10 = -(m01*m22 - m02*m21);
			const float c11 =  (m00*m22 - m02*m20);
			const float c12 = -(m00*m21 - m01*m20);
			const float c20 =  (m01*m12 - m02*m11);
			const float c21 = -(m00*m12 - m02*m10);
			const float c22 =  (m00*m11 - m01*m10);
			out = Mat3(
				c00*invDet, c10*invDet, c20*invDet,
				c01*invDet, c11*invDet, c21*invDet,
				c02*invDet, c12*invDet, c22*invDet
			);
			return true;
		}

		// Orthonormalize (Gram-Schmidt) to build a clean rotation matrix
		static Mat3 Orthonormalize(const Mat3& m) noexcept
		{
			auto normalize = [](Vec3f v) {
				const float len2 = v.x*v.x + v.y*v.y + v.z*v.z;
				if (len2 <= 0.0f) return Vec3f{1,0,0};
				const float invLen = 1.0f / std::sqrt(len2);
				return Vec3f{v.x*invLen, v.y*invLen, v.z*invLen};
			};
			auto dot = [](const Vec3f& a, const Vec3f& b){ return a.x*b.x + a.y*b.y + a.z*b.z; };
			auto sub = [](const Vec3f& a, const Vec3f& b){ return Vec3f{a.x-b.x, a.y-b.y, a.z-b.z}; };

			Vec3f x = m.Col(0);
			Vec3f y = m.Col(1);
			Vec3f z = m.Col(2);

			x = normalize(x);
			y = sub(y, Vec3f{dot(y,x)*x.x, dot(y,x)*x.y, dot(y,x)*x.z});
			y = normalize(y);
			// z as cross(x,y)
			z = Vec3f{ x.y*y.z - x.z*y.y, x.z*y.x - x.x*y.z, x.x*y.y - x.y*y.x };
			z = normalize(z);

			return FromColumns(x,y,z);
		}

		// Comparisons
		constexpr bool operator==(const Mat3& r) const noexcept
		{
			return m00==r.m00 && m01==r.m01 && m02==r.m02 &&
				   m10==r.m10 && m11==r.m11 && m12==r.m12 &&
				   m20==r.m20 && m21==r.m21 && m22==r.m22;
		}
		constexpr bool operator!=(const Mat3& r) const noexcept { return !(*this == r); }
		static bool ApproxEqual(const Mat3& a, const Mat3& b, float eps = 1e-6f) noexcept
		{
			return epsilonEqual(a.m00,b.m00,eps) && epsilonEqual(a.m01,b.m01,eps) && epsilonEqual(a.m02,b.m02,eps) &&
				   epsilonEqual(a.m10,b.m10,eps) && epsilonEqual(a.m11,b.m11,eps) && epsilonEqual(a.m12,b.m12,eps) &&
				   epsilonEqual(a.m20,b.m20,eps) && epsilonEqual(a.m21,b.m21,eps) && epsilonEqual(a.m22,b.m22,eps);
		}
	};

	// Stream operator for debugging
	inline std::ostream& operator<<(std::ostream& os, const Mat3& m)
	{
		os << "[" << m.m00 << ", " << m.m01 << ", " << m.m02 << "; "
		   << m.m10 << ", " << m.m11 << ", " << m.m12 << "; "
		   << m.m20 << ", " << m.m21 << ", " << m.m22 << "]";
		return os;
	}
}

/// -----------------------------------------------------
