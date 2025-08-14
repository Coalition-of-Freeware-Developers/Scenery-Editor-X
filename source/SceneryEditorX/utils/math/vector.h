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

/// -----------------------------------------------------
/// Umbrella header for all vector types
/// -----------------------------------------------------

#include <SceneryEditorX/utils/math/vec2.h>	/// 2D Vector
#include <SceneryEditorX/utils/math/vec3.h>	/// 3D Vector
#include <SceneryEditorX/utils/math/vec4.h>	/// 4D Vector

/// -----------------------------------------------------

namespace SceneryEditorX
{
	using Vec2 = Utils::TVector2<float>;
	using Vec3 = Utils::TVector3<float>;
	using Vec4 = Utils::TVector4<float>;
}

/// -----------------------------------------------------
