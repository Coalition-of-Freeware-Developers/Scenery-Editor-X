/**
* -------------------------------------------------------
* Scenery Editor X
* -------------------------------------------------------
* Copyright (c) 2025 Thomas Ray
* Copyright (c) 2025 Coalition of Freeware Developers
* -------------------------------------------------------
* vec4.h - TVector4 template
* -------------------------------------------------------
* Created: 12/8/2025
* -------------------------------------------------------
*/
#pragma once

/// -----------------------------------------------------

namespace SceneryEditorX::Utils
{

	template<typename T> struct TVector3;

	template<typename T>
	struct TVector4
	{
		// Provide multiple semantic aliases for components (position/color/texcoord)
		union
		{
			struct { T x, y, z, w; }; // Cartesian
			struct { T r, g, b, a; }; // Color
			struct { T s, t, p, q; }; // Texture (p,q = 3rd/4th coords)
		};

		constexpr TVector4() : x(0), y(0), z(0), w(0) {}
		constexpr TVector4(T s) : x(s), y(s), z(s), w(s) {}
		constexpr TVector4(T _x, T _y, T _z, T _w) : x(_x), y(_y), z(_z), w(_w) {}
		constexpr TVector4(const TVector3<T>& v, T _w) : x(v.x), y(v.y), z(v.z), w(_w) {}

		/// Basic arithmetic
		constexpr TVector4 operator+(const TVector4& r) const { return {x + r.x, y + r.y, z + r.z, w + r.w}; }
		constexpr TVector4 operator-(const TVector4& r) const { return {x - r.x, y - r.y, z - r.z, w - r.w}; }
		constexpr TVector4 operator*(T s) const { return {x * s, y * s, z * s, w * s}; }
		constexpr TVector4 operator/(T s) const { return {x / s, y / s, z / s, w / s}; }

		/// Equality
		constexpr bool operator==(const TVector4& r) const { return x == r.x && y == r.y && z == r.z && w == r.w; }
		constexpr bool operator!=(const TVector4& r) const { return !(*this == r); }

		TVector4& operator+=(const TVector4& r) { x += r.x; y += r.y; z += r.z; w += r.w; return *this; }
		TVector4& operator-=(const TVector4& r) { x -= r.x; y -= r.y; z -= r.z; w -= r.w; return *this; }
		TVector4& operator*=(T s) { x *= s; y *= s; z *= s; w *= s; return *this; }
		TVector4& operator/=(T s) { x /= s; y /= s; z /= s; w /= s; return *this; }

		/// Conversions
		operator TVector3<T>() const { return TVector3<T>(x, y, z); }
	    // Allow explicit construction of a Vec3 from a Vec4 without relying on operator cast.
		friend struct TVector3<T>; // Grant access for converting ctor definition below.


		/// Indexing helpers
		T& operator[](int i) { return (&x)[i]; }
		const T& operator[](int i) const { return (&x)[i]; }
	};

}

/// -----------------------------------------------------

// Converting constructor definition (must be after TVector4 is visible via vec4.h include in umbrella header usage).
namespace SceneryEditorX::Utils
{
	template<typename T>
	constexpr TVector3<T>::TVector3(const TVector4<T>& v) : x(v.x), y(v.y), z(v.z) {}
}
