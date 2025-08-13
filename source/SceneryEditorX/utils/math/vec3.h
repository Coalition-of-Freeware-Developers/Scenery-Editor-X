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
		T x{0}, y{0}, z{0};

		constexpr TVector3() = default;
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

		TVector3& operator+=(const TVector3& r) { x += r.x; y += r.y; z += r.z; return *this; }
		TVector3& operator-=(const TVector3& r) { x -= r.x; y -= r.y; z -= r.z; return *this; }
		TVector3& operator*=(T s) { x *= s; y *= s; z *= s; return *this; }
		TVector3& operator/=(T s) { x /= s; y /= s; z /= s; return *this; }

		// Indexing helpers
		T& operator[](int i) { return (&x)[i]; }
		const T& operator[](int i) const { return (&x)[i]; }
	};

}

/// -----------------------------------------------------
