/**
* -------------------------------------------------------
* Scenery Editor X
* -------------------------------------------------------
* Copyright (c) 2025 Thomas Ray
* Copyright (c) 2025 Coalition of Freeware Developers
* -------------------------------------------------------
* vec2.h - TVector2 template
* -------------------------------------------------------
* Created: 12/8/2025
* -------------------------------------------------------
*/
#pragma once

/// -----------------------------------------------------

namespace SceneryEditorX::Utils
{

	template<typename T>
	struct TVector2
	{
		T x{0}, y{0};

		constexpr TVector2() = default;
		constexpr TVector2(T s) : x(s), y(s) {}
		constexpr TVector2(T _x, T _y) : x(_x), y(_y) {}

		// Basic arithmetic
		constexpr TVector2 operator+(const TVector2& r) const noexcept { return TVector2{x + r.x, y + r.y}; }
		constexpr TVector2 operator-(const TVector2& r) const noexcept { return TVector2{x - r.x, y - r.y}; }
		constexpr TVector2 operator*(T s) const noexcept { return TVector2{x * s, y * s}; }
		constexpr TVector2 operator/(T s) const noexcept { return TVector2{x / s, y / s}; }

		// Equality
		constexpr bool operator==(const TVector2& r) const noexcept { return x == r.x && y == r.y; }
		constexpr bool operator!=(const TVector2& r) const noexcept { return !(*this == r); }

		TVector2& operator+=(const TVector2& r) noexcept { x += r.x; y += r.y; return *this; }
		TVector2& operator-=(const TVector2& r) noexcept { x -= r.x; y -= r.y; return *this; }
		TVector2& operator*=(T s) noexcept { x *= s; y *= s; return *this; }
		TVector2& operator/=(T s) noexcept { x /= s; y /= s; return *this; }

		// Utility
		constexpr T& operator[](int i) { return i == 0 ? x : y; }
		constexpr const T& operator[](int i) const { return i == 0 ? x : y; }

		// Length helpers (for floating-point specializations)
		T Length2() const { return x * x + y * y; }
	};

}

/// -----------------------------------------------------
