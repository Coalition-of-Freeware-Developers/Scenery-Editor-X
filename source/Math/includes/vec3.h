/**
* -------------------------------------------------------
* Scenery Editor X
* -------------------------------------------------------
* Copyright (c) 2025 Thomas Ray
* Copyright (c) 2025 Coalition of Freeware Developers
* -------------------------------------------------------
* vec3.h - TVector3 template
* -------------------------------------------------------
* Created: 12/8/2025
* -------------------------------------------------------
*/
#pragma once
#include <Math/math_config.h>

/// -----------------------------------------------------

namespace SceneryEditorX::Utils
{
    // Forward declaration to allow declaring converting constructor without including vec4.h here.
	template<typename T> struct TVector4;

	template<typename T>
	struct TVector3
	{
		// Provide multiple semantic aliases for components (position/color/texcoord)
		union
		{
			struct { T x, y, z; }; // Cartesian
			struct { T r, g, b; }; // Color
			struct { T s, t, p; }; // Texture (p = third coordinate)
		};

		constexpr TVector3() noexcept : x(0), y(0), z(0) {}
		constexpr explicit TVector3(T s) noexcept : x(s), y(s), z(s) {}
		constexpr TVector3(T _x, T _y, T _z) noexcept : x(_x), y(_y), z(_z) {}

		// Converting constructor from a 4D vector (drops w component).
		// Marked explicit to avoid competing with TVector4<T>::operator TVector3<T>() and
		// thereby preventing ambiguous user-defined conversions (MSVC E0348) when
		// assigning a TVector4<T> to a TVector3<T> destination.
	explicit constexpr TVector3(const TVector4<T>& v) noexcept : x(v.x), y(v.y), z(v.z) {}


		// Basic arithmetic
		[[nodiscard]] constexpr TVector3 operator+(const TVector3& r) const noexcept { return {x + r.x, y + r.y, z + r.z}; }
		[[nodiscard]] constexpr TVector3 operator-(const TVector3& r) const noexcept { return {x - r.x, y - r.y, z - r.z}; }
		[[nodiscard]] constexpr TVector3 operator*(T s) const noexcept { return {x * s, y * s, z * s}; }
		[[nodiscard]] constexpr TVector3 operator/(T s) const noexcept { return {x / s, y / s, z / s}; }

		// Equality
		[[nodiscard]] constexpr bool operator==(const TVector3& r) const noexcept { return x == r.x && y == r.y && z == r.z; }
		[[nodiscard]] constexpr bool operator!=(const TVector3& r) const noexcept { return !(*this == r); }

		// Unary minus
		[[nodiscard]] constexpr TVector3 operator-() const noexcept { return TVector3{-x, -y, -z}; }

		constexpr TVector3& operator+=(const TVector3& r) noexcept { x += r.x; y += r.y; z += r.z; return *this; }
		constexpr TVector3& operator-=(const TVector3& r) noexcept { x -= r.x; y -= r.y; z -= r.z; return *this; }
		constexpr TVector3& operator*=(T s) noexcept { x *= s; y *= s; z *= s; return *this; }
		constexpr TVector3& operator/=(T s) noexcept { x /= s; y /= s; z /= s; return *this; }
	    //TVector3& operator*(float scalar) const { return TVector3(x * scalar, y * scalar, z * scalar); }

		// Indexing helpers
		[[nodiscard]] constexpr T& operator[](int i) noexcept { return (&x)[i]; }
		[[nodiscard]] constexpr const T& operator[](int i) const noexcept { return (&x)[i]; }
	};

	// Free-function scalar multiplication to support expressions like: float * Vec3
	template<typename T>
	[[nodiscard]] constexpr TVector3<T> operator*(T s, const TVector3<T>& v) noexcept
	{
		return TVector3<T>(v.x * s, v.y * s, v.z * s);
	}

}

/// -----------------------------------------------------
