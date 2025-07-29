/**
* -------------------------------------------------------
* Scenery Editor X
* -------------------------------------------------------
* Copyright (c) 2025 Thomas Ray
* Copyright (c) 2025 Coalition of Freeware Developers
* -------------------------------------------------------
* gradients.cpp
* -------------------------------------------------------
* Created: 14/7/2025
* -------------------------------------------------------
*/
#include <algorithm>
#include <initializer_list>
#include <SceneryEditorX/utils/math/gradients.h>
#include <utility>

/// -----------------------------------------------------

namespace SceneryEditorX::Utils
{
	Gradient::Gradient() = default;
	Gradient::Gradient(const std::initializer_list<Key> list, const float degrees) : keys(list), degrees(degrees) {}
	Gradient::Gradient(const std::vector<Key>& list, const float degrees) : keys(list), degrees(degrees) {}

	Color Gradient::Evaluate(const float position) const
	{
		// Handle empty gradient
		if (keys.empty())
			return {};

		// Handle single key gradient
		if (keys.size() == 1)
			return keys[0].value;

		// Find the appropriate key pair for interpolation
        for (auto i = 0; std::cmp_less(i, keys.size()); i++)
		{
			if (i > 0 && keys[i - 1].position <= position && position < keys[i].position)
			{
				// Interpolate between keys[i-1] and keys[i]
				const float left = keys[i - 1].position;
				const float right = keys[i].position;
				float t = (position - left) / (right - left);

				// Clamp interpolation parameter to [0, 1] range for safety
				t = std::max(0.0f, std::min(1.0f, t));
				return Color::Lerp(keys[i - 1].value, keys[i].value, t);
			}
		}

		// Position is outside the range of defined keys
		// Return the appropriate edge color
		return position < 0 ? keys[0].value : keys.back().value;
	}

	void Gradient::Clear() { keys.clear(); }

	void Gradient::AddKey(const Key& key) { keys.push_back(key); }
	void Gradient::AddKey(const float position, const Color& color) { keys.push_back({.value = color, .position = position }); }

	void Gradient::RemoveKeyAt(const uint32_t index)
	{
		if (index < keys.size())
            keys.erase(keys.begin() + index);
    }

	size_t Gradient::GetHash() const
	{
		if (keys.empty())
			return 0;

		size_t hash = std::hash<float>{}(degrees);

		/*
		for (const auto& key : keys)
		{
			hash ^= key.GetHash() + 0x9e3779b9 + (hash << 6) + (hash >> 2);
		}
		*/

		return hash;
	}

	size_t Gradient::GetHash(const float degrees) const
	{
		if (keys.empty())
			return std::hash<float>{}(degrees);

		size_t hash = std::hash<float>{}(degrees);

		/*
		for (const auto& key : keys)
		{
			hash ^= key.GetHash() + 0x9e3779b9 + (hash << 6) + (hash >> 2);
		}
		*/

		return hash;
	}

}

/// -----------------------------------------------------
