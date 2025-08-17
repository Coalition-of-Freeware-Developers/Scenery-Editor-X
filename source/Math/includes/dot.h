/**
* -------------------------------------------------------
* Scenery Editor X
* -------------------------------------------------------
* Copyright (c) 2025 Thomas Ray
* Copyright (c) 2025 Coalition of Freeware Developers
* -------------------------------------------------------
* dot.h
* -------------------------------------------------------
* Created: 12/8/2025
* -------------------------------------------------------
*/
#pragma once
#include <Math/math_config.h>
#include <Math/includes/vector.h>

/// -----------------------------------------------------

namespace SceneryEditorX
{
	// Local aliases (duplicated from vector.h for standalone inclusion safety)
	using Vec2 = Utils::TVector2<float>;
	using Vec3 = Utils::TVector3<float>;
	using Vec4 = Utils::TVector4<float>;

	inline float Dot(const Vec2& a, const Vec2& b) noexcept { return a.x * b.x + a.y * b.y; }
	inline float Dot(const Vec3& a, const Vec3& b) noexcept { return a.x * b.x + a.y * b.y + a.z * b.z; }
	inline float Dot(const Vec4& a, const Vec4& b) noexcept { return a.x * b.x + a.y * b.y + a.z * b.z + a.w * b.w; }

	inline Vec3 Cross(const Vec3& a, const Vec3& b) noexcept
	{
		return Vec3{ a.y * b.z - a.z * b.y,
		        a.z * b.x - a.x * b.z,
		        a.x * b.y - a.y * b.x };
	}
}

/// -----------------------------------------------------
