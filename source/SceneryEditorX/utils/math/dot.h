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
#include "vector.h"

/// -----------------------------------------------------

namespace SceneryEditorX
{

	inline float Dot(const Vec2& a, const Vec2& b) noexcept
	{
		return a.x * b.x + a.y * b.y;
	}

	inline float Dot(const Vec4& a, const Vec4& b) noexcept
	{
		return a.x * b.x + a.y * b.y + a.z * b.z + a.w * b.w;
	}

}

/// -----------------------------------------------------
