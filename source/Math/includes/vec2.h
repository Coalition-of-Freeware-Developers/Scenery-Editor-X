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
#include <Math/math_config.h>

/// -----------------------------------------------------

namespace SceneryEditorX::Utils
{

	template<typename T>
	struct TVector2
	{
		// Provide multiple semantic aliases for components (position/color/texcoord)
		union
		{
			struct { T x, y; }; // Cartesian
			struct { T r, g; }; // Color
			struct { T s, t; }; // Texture
		};

		constexpr TVector2() noexcept : x(0), y(0) {}
		constexpr explicit TVector2(T s) noexcept : x(s), y(s) {}
		constexpr TVector2(T _x, T _y) noexcept : x(_x), y(_y) {}

		// Basic arithmetic (all pure, mark [[nodiscard]])
		[[nodiscard]] constexpr TVector2 operator+(const TVector2& r) const noexcept { return {x + r.x, y + r.y}; }
		[[nodiscard]] constexpr TVector2 operator-(const TVector2& r) const noexcept { return {x - r.x, y - r.y}; }
		[[nodiscard]] constexpr TVector2 operator*(T s) const noexcept { return {x * s, y * s}; }
		[[nodiscard]] constexpr TVector2 operator/(T s) const noexcept { return {x / s, y / s}; }

		// Equality
		[[nodiscard]] constexpr bool operator==(const TVector2& r) const noexcept { return x == r.x && y == r.y; }
		[[nodiscard]] constexpr bool operator!=(const TVector2& r) const noexcept { return !(*this == r); }

		constexpr TVector2& operator+=(const TVector2& r) noexcept { x += r.x; y += r.y; return *this; }
		constexpr TVector2& operator-=(const TVector2& r) noexcept { x -= r.x; y -= r.y; return *this; }
		constexpr TVector2& operator*=(T s) noexcept { x *= s; y *= s; return *this; }
		constexpr TVector2& operator/=(T s) noexcept { x /= s; y /= s; return *this; }

		// Utility
		[[nodiscard]] constexpr T& operator[](int i) noexcept { return i == 0 ? x : y; }
		[[nodiscard]] constexpr const T& operator[](int i) const noexcept { return i == 0 ? x : y; }

		// Length helpers (for floating-point specializations)
		[[nodiscard]] constexpr T Length2() const noexcept { return x * x + y * y; }
	};

}

/// -----------------------------------------------------
