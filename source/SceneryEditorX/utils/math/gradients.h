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
#include <SceneryEditorX/utils/math/colors.h>
#include <SceneryEditorX/utils/math/math_utils.h>

/// -----------------------------------------------------

namespace SceneryEditorX::Utils
{

    class Gradient
    {
    public:
        struct Key
        {
			Color value{};
            float position{};

			size_t GetHash() const
			{
				size_t hash = value.GetHash();
				CombineHash(hash, position);
				return hash;
			}
        };

        Gradient();
		Gradient(std::initializer_list<Key> list, float degrees = 0);
		Gradient(const Array<Key>& list, float degrees = 0);

		float GetDegrees() const { return degrees; }

		void SetDegrees(float degrees) { this->degrees = degrees; }

		uint32_t GetNumKeys() const { return keys.GetSize(); }
		const Array<Key>& GetKeys() const { return keys; }
		const Key& GetKeyAt(uint32_t index) const { return keys[index]; }

		Key& GetKeyAt(uint32_t index) { return keys[index]; }

		Color Evaluate(float position) const;

		void Clear();
		void AddKey(const Key& key);
		void AddKey(float position, const Color& color);
		void RemoveKeyAt(uint32_t index);

		size_t GetHash() const;

		bool operator==(const Gradient& rhs) const { return GetHash() == rhs.GetHash(); }
		bool operator!=(const Gradient& rhs) const { return !operator==(rhs); }

    private:

        Array<Key> keys{};
		float degrees = 0.0f;

    };

}

/// -------------------------------------------------------
