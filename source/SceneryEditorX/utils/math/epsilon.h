/**
* -------------------------------------------------------
* Scenery Editor X
* -------------------------------------------------------
* Copyright (c) 2025 Thomas Ray
* Copyright (c) 2025 Coalition of Freeware Developers
* -------------------------------------------------------
* epsilon.h
* -------------------------------------------------------
* Created: 12/8/2025
* -------------------------------------------------------
*/
#pragma once
#include <cmath>
#include <limits>

/// -----------------------------------------------------

namespace SceneryEditorX
{
	/// GLM-compatible epsilon<T>() replacement
	template <typename T>
	constexpr T epsilon() noexcept
	{
	    return std::numeric_limits<T>::epsilon();
	}

	/// Scalar epsilonEqual
	template <typename T>
	inline bool epsilonEqual(T a, T b, T eps = epsilon<T>()) noexcept
	{
		return std::fabs(static_cast<double>(a - b)) <= static_cast<double>(eps);
	}
}

/// -----------------------------------------------------

