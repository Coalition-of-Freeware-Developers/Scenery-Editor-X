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

/// -----------------------------------------------------

namespace SceneryEditorX::Utils
{

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

		constexpr TVector3() : x(0), y(0), z(0) {}
		constexpr TVector3(T s) : x(s), y(s), z(s) {}
		constexpr TVector3(T _x, T _y, T _z) : x(_x), y(_y), z(_z) {}

		// Basic arithmetic
		constexpr TVector3 operator+(const TVector3& r) const { return {x + r.x, y + r.y, z + r.z}; }
		constexpr TVector3 operator-(const TVector3& r) const { return {x - r.x, y - r.y, z - r.z}; }
		constexpr TVector3 operator*(T s) const { return {x * s, y * s, z * s}; }
		constexpr TVector3 operator/(T s) const { return {x / s, y / s, z / s}; }

		// Equality
		constexpr bool operator==(const TVector3& r) const { return x == r.x && y == r.y && z == r.z; }
		constexpr bool operator!=(const TVector3& r) const { return !(*this == r); }

		// Unary minus
		constexpr TVector3 operator-() const { return TVector3{-x, -y, -z}; }

		TVector3& operator+=(const TVector3& r) { x += r.x; y += r.y; z += r.z; return *this; }
		TVector3& operator-=(const TVector3& r) { x -= r.x; y -= r.y; z -= r.z; return *this; }
		TVector3& operator*=(T s) { x *= s; y *= s; z *= s; return *this; }
		TVector3& operator/=(T s) { x /= s; y /= s; z /= s; return *this; }

		// Indexing helpers
		T& operator[](int i) { return (&x)[i]; }
		const T& operator[](int i) const { return (&x)[i]; }
	};

	// Free-function scalar multiplication to support expressions like: float * Vec3
	template<typename T>
	constexpr TVector3<T> operator*(T s, const TVector3<T>& v)
	{
		return TVector3<T>(v.x * s, v.y * s, v.z * s);
	}

}

/// -----------------------------------------------------
