/**
* -------------------------------------------------------
* Scenery Editor X
* -------------------------------------------------------
* Copyright (c) 2025 Thomas Ray
* Copyright (c) 2025 Coalition of Freeware Developers
* -------------------------------------------------------
* gradients.h
* -------------------------------------------------------
* Created: 14/7/2025
* -------------------------------------------------------
*/
#pragma once
#include <SceneryEditorX/utils/math/gradients.h>
#include <SceneryEditorX/utils/math/math_utils.h>
#include <initializer_list>

/// -----------------------------------------------------

namespace SceneryEditorX::Utils
{
	Gradient::Gradient() = default;

	Gradient::Gradient(std::initializer_list<Key> list, const float degrees) : keys(list), degrees(degrees) {}

	Gradient::Gradient(const Array<Key>& list, const float degrees) : keys(list), degrees(degrees) {}

	Color Gradient::Evaluate(float position) const
	{
		if (keys.GetSize() == 0)
			return Color();
		if (keys.GetSize() == 1)
			return keys[0].value;

		for (int i = 0; i < keys.GetSize(); i++)
		{
			if (i > 0 && keys[i - 1].position <= position && position < keys[i].position)
			{
				float left = keys[i - 1].position;
				float right = keys[i].position;
				return Color::Lerp(keys[i - 1].value, keys[i].value, Math::Clamp((position - left) / (right - left)));
			}
		}

		return position < 0 ? keys[0].value : keys.GetLast().value;
	}

	void Gradient::Clear() { keys.Clear(); }

	void Gradient::AddKey(const Key& key) { keys.Add(key); }

	void Gradient::AddKey(float position, const Color& color) { keys.Add({ color, position }); }

	void Gradient::RemoveKeyAt(uint32_t index) { keys.RemoveAt(index); }

	size_t Gradient::GetHash() const
	{
		if (keys.IsEmpty())
			return 0;

		size_t hash = GetHash(degrees);

		for (int i = 0; i < keys.GetSize(); ++i)
		{
			CombineHash(hash, keys[i]);
		}

		return hash;
	}

}

/// -----------------------------------------------------
