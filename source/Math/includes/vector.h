/**
* -------------------------------------------------------
* Scenery Editor X
* -------------------------------------------------------
* Copyright (c) 2025 Thomas Ray
* Copyright (c) 2025 Coalition of Freeware Developers
* -------------------------------------------------------
* vector.h
* -------------------------------------------------------
* Created: 15/7/2025
* -------------------------------------------------------
*/
#pragma once
#include <Math/math_config.h>
#include <Math/includes/vec2.h>
#include <Math/includes/vec3.h>
#include <Math/includes/vec4.h>

/// -----------------------------------------------------

namespace SceneryEditorX
{
	using Vec2 = Utils::TVector2<float>;
	using Vec3 = Utils::TVector3<float>;
	using Vec4 = Utils::TVector4<float>;

	#ifndef SEDX_MATH_HAS_VECTOR_ALIASES
	#define SEDX_MATH_HAS_VECTOR_ALIASES 1
	#endif
}

/// -----------------------------------------------------
