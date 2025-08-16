/**
* -------------------------------------------------------
* Scenery Editor X
* -------------------------------------------------------
* Copyright (c) 2025 Thomas Ray
* Copyright (c) 2025 Coalition of Freeware Developers
* -------------------------------------------------------
* math.h
* -------------------------------------------------------
* Created: 12/8/2025
* -------------------------------------------------------
*/
#pragma once

/// -------------------------------------------------------
// Top-level math umbrella header

#include <SceneryEditorX/utils/math/constants.h>
#include <SceneryEditorX/utils/math/dot.h>
#include <SceneryEditorX/utils/math/epsilon.h>
#include <SceneryEditorX/utils/math/math_utils.h>
#include <SceneryEditorX/utils/math/matrix.h>
#include <SceneryEditorX/utils/math/quat.h>
#include <SceneryEditorX/utils/math/transforms.h>
#include <SceneryEditorX/utils/math/vector.h>
#include <SceneryEditorX/utils/math/translate.h>
#include <SceneryEditorX/utils/math/rotation.h>
#include <SceneryEditorX/utils/math/scale.h>
#include <SceneryEditorX/utils/math/projection.h>


/// -------------------------------------------------------

// -----------------------------------------------------------------------------
// Compatibility forwarding layer
// Some legacy/test code referenced free math utility functions via the nested
// namespace SceneryEditorX::Utils::<Func>. The canonical definitions now live
// directly in namespace SceneryEditorX (see math_utils.h, epsilon.h, etc.).
// To preserve those call sites without rewriting the tests, we import the
// symbols into the Utils namespace here. This keeps the public surface stable
// while allowing the core math implementation to remain flat.
// -----------------------------------------------------------------------------
namespace SceneryEditorX { namespace Utils {
	using ::SceneryEditorX::Normalize;
	using ::SceneryEditorX::Dot;
	using ::SceneryEditorX::Cross;
	using ::SceneryEditorX::ToRadians;
	using ::SceneryEditorX::ToDegrees;
	using ::SceneryEditorX::epsilon;       // template
	using ::SceneryEditorX::epsilonEqual;  // template
}} // namespace SceneryEditorX::Utils

