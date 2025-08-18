/**
* -------------------------------------------------------
* Scenery Editor X
* -------------------------------------------------------
* Umbrella math header (renamed from math.h to xmath.hpp to avoid collision with <math.h>)
* -------------------------------------------------------
* Provides consolidated inclusion of all public math types and utilities.
* -------------------------------------------------------
*/
#pragma once
#include <Math/math_config.h>
// Provide engine alias types (Vec2/Vec3/etc.) expected by legacy tests.
#if __has_include(<SceneryEditorX/core/base.hpp>)
#include <SceneryEditorX/core/base.hpp>
#endif

// Public module includes
// Order matters: vector types must be available before dot/epsilon overloads.
#include <Math/includes/vector.h>
#include <Math/includes/constants.h>
#include <Math/includes/epsilon.h>
//#include <Math/includes/dot.h> // Dot was removed and placed into math_util.h
#include <Math/includes/math_utils.h>
#include <Math/includes/matrix.h>
#include <Math/includes/projection.h>
#include <Math/includes/quat.h>
#include <Math/includes/rotation.h>
#include <Math/includes/scale.h>
#include <Math/includes/transforms.h>
#include <Math/includes/translate.h>

// Legacy forwarding namespace (kept for backward compatibility with previous API usage)
namespace SceneryEditorX::Utils
{
	using ::SceneryEditorX::Normalize;
	//using ::SceneryEditorX::Dot; // Dot was removed and placed into math_util.h
	using ::SceneryEditorX::Cross;
	using ::SceneryEditorX::ToRadians;
	using ::SceneryEditorX::ToDegrees;
	using ::SceneryEditorX::epsilon;      // templates
	using ::SceneryEditorX::epsilonEqual; // templates
}
