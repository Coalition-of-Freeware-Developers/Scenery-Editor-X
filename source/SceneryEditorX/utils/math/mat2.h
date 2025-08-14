/**
* -------------------------------------------------------
* Scenery Editor X
* -------------------------------------------------------
* Copyright (c) 2025 Thomas Ray
* Copyright (c) 2025 Coalition of Freeware Developers
* -------------------------------------------------------
* mat2.h
* -------------------------------------------------------
* Created: 12/8/2025
* -------------------------------------------------------
*/
#pragma once
#include <cmath>
#include <ostream>
#include <SceneryEditorX/utils/math/epsilon.h>
#include <SceneryEditorX/utils/math/vector.h>

/// -----------------------------------------------------

namespace SceneryEditorX
{
	/**
	 * @brief Fast 2x2 single-precision matrix (row-major) for 2D transforms and linear algebra.
	 *
	 * Storage layout (row-major):
	 * [ m00 m01 ]
	 * [ m10 m11 ]
	 */
	class Mat2
	{
	public:
        float m00, m01;
        float m10, m11;

		/// -------------------------------------------------

	    // Constructors
		constexpr Mat2() noexcept : m00(1), m01(0), m10(0), m11(1) {}
		constexpr explicit Mat2(float s) noexcept : m00(s), m01(0), m10(0), m11(s) {}
		constexpr Mat2(float _m00, float _m01, float _m10, float _m11) noexcept : m00(_m00), m01(_m01), m10(_m10), m11(_m11) {}

		// From rows / columns
		constexpr static Mat2 FromRows(const Utils::TVector2<float>& r0, const Utils::TVector2<float>& r1) noexcept
		{
		    return {r0.x, r0.y, r1.x, r1.y};
		}

		constexpr static Mat2 FromColumns(const Utils::TVector2<float>& c0, const Utils::TVector2<float>& c1) noexcept
	    {
		    return {c0.x, c1.x, c0.y, c1.y};
	    }

		// Identity / Zero
		constexpr static Mat2 Identity() noexcept { return {}; }
		constexpr static Mat2 Zero() noexcept { return {0, 0, 0, 0}; }

		// Rotation (radians)
		static Mat2 Rotation(float radians) noexcept
		{
			const float c = std::cos(radians);
			const float s = std::sin(radians);
			return {c, -s, s, c};
		}

		// Scaling
		constexpr static Mat2 Scale(float sx, float sy) noexcept { return {sx, 0, 0, sy}; }
		constexpr static Mat2 Scale(const Utils::TVector2<float>& s) noexcept { return Scale(s.x, s.y); }

		// Accessors
		constexpr Utils::TVector2<float> Row(int r) const noexcept { return r == 0 ? Utils::TVector2<float>{m00, m01} : Utils::TVector2<float>{m10, m11}; }
		constexpr Utils::TVector2<float> Col(int c) const noexcept { return c == 0 ? Utils::TVector2<float>{m00, m10} : Utils::TVector2<float>{m01, m11}; }

		// Indexing helpers
		constexpr float& operator()(int r, int c) noexcept { return r == 0 ? (c == 0 ? m00 : m01) : (c == 0 ? m10 : m11); }
		constexpr const float& operator()(int r, int c) const noexcept { return r == 0 ? (c == 0 ? m00 : m01) : (c == 0 ? m10 : m11); }

		/// -------------------------------------------------

	    // Arithmetic
		constexpr Mat2 operator+(const Mat2& rhs) const noexcept { return {m00 + rhs.m00, m01 + rhs.m01, m10 + rhs.m10, m11 + rhs.m11}; }
		constexpr Mat2 operator-(const Mat2& rhs) const noexcept { return {m00 - rhs.m00, m01 - rhs.m01, m10 - rhs.m10, m11 - rhs.m11}; }
		constexpr Mat2 operator*(float s) const noexcept { return {m00 * s, m01 * s, m10 * s, m11 * s}; }
		friend constexpr Mat2 operator*(float s, const Mat2& m) noexcept { return m * s; }

		constexpr Mat2& operator+=(const Mat2& r) noexcept { m00 += r.m00; m01 += r.m01; m10 += r.m10; m11 += r.m11; return *this; }
		constexpr Mat2& operator-=(const Mat2& r) noexcept { m00 -= r.m00; m01 -= r.m01; m10 -= r.m10; m11 -= r.m11; return *this; }
		constexpr Mat2& operator*=(float s) noexcept { m00 *= s; m01 *= s; m10 *= s; m11 *= s; return *this; }

		// Matrix multiply
		constexpr Mat2 operator*(const Mat2& r) const noexcept
		{
			return {
			    m00 * r.m00 + m01 * r.m10,
			    m00 * r.m01 + m01 * r.m11,
			    m10 * r.m00 + m11 * r.m10,
			    m10 * r.m01 + m11 * r.m11
			};
		}
		constexpr Mat2& operator*=(const Mat2& r) noexcept { *this = (*this) * r; return *this; }

		// Vector multiply (column-vector convention)
		constexpr Utils::TVector2<float> operator*(const Utils::TVector2<float>& v) const noexcept { return Utils::TVector2<float>{ m00 * v.x + m01 * v.y, m10 * v.x + m11 * v.y }; }

		/// -------------------------------------------------

		// Properties and helpers
		constexpr float Trace() const noexcept { return m00 + m11; }
		constexpr float Determinant() const noexcept { return m00 * m11 - m01 * m10; }
		constexpr Mat2 Transposed() const noexcept { return {m00, m10, m01, m11}; }

		Mat2 Inversed(float eps = 1e-6f) const noexcept
		{
			const float det = Determinant();
			if (std::fabs(det) <= eps) return Mat2::Zero();
			const float invDet = 1.0f / det;
			/// adjudicate: [ m11 -m01; -m10 m00 ]
			return {
			    m11 * invDet, -m01 * invDet,
			    -m10 * invDet,  m00 * invDet
			};
		}

		bool TryInverse(Mat2& out, float eps = 1e-6f) const noexcept
		{
			const float det = Determinant();
			if (std::fabs(det) <= eps) { out = Mat2::Zero(); return false; }
			const float invDet = 1.0f / det;
			out = Mat2( m11 * invDet, -m01 * invDet,
					   -m10 * invDet,  m00 * invDet);
			return true;
		}

		// Comparisons
		constexpr bool operator==(const Mat2& r) const noexcept { return m00 == r.m00 && m01 == r.m01 && m10 == r.m10 && m11 == r.m11; }
		constexpr bool operator!=(const Mat2& r) const noexcept { return !(*this == r); }
		static bool ApproxEqual(const Mat2& a, const Mat2& b, float eps = 1e-6f) noexcept
		{
	     return epsilonEqual(a.m00, b.m00, eps) && epsilonEqual(a.m01, b.m01, eps) &&
			    epsilonEqual(a.m10, b.m10, eps) && epsilonEqual(a.m11, b.m11, eps);
		}
	};

	// Stream operator for debugging
	inline std::ostream& operator<<(std::ostream& os, const Mat2& m)
	{
		os << "[" << m.m00 << ", " << m.m01 << "; " << m.m10 << ", " << m.m11 << "]";
		return os;
	}
}

/// -----------------------------------------------------
