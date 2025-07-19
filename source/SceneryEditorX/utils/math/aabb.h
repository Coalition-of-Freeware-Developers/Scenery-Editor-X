/**
* -------------------------------------------------------
* Scenery Editor X
* -------------------------------------------------------
* Copyright (c) 2025 Thomas Ray
* Copyright (c) 2025 Coalition of Freeware Developers
* -------------------------------------------------------
* aabb.h
* -------------------------------------------------------
* Created: 14/7/2025
* -------------------------------------------------------
*/
#pragma once
#include <SceneryEditorX/core/base.hpp>

/// -----------------------------------------------------

namespace SceneryEditorX::Utils
{
	struct AABB
	{
		Vec3 Min, Max;
		AABB() : Min(0.0f), Max(0.0f) {}
		AABB(const Vec3& min, const Vec3& max) : Min(min), Max(max) {}

        [[nodiscard]] Vec3 Size() const { return Max - Min; }
        [[nodiscard]] Vec3 Center() const { return Min + Size() * 0.5f; }
	};
}

/// -----------------------------------------------------
