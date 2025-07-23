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
#include <SceneryEditorX/core/identifiers/uuid.h>
#include <vector>
#include <functional>

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

            [[nodiscard]] size_t GetHash() const
			{
				size_t hash = value.GetHash();
                hash ^= std::hash<float>{}(position) + 0x9e3779b9 + (hash << 6) + (hash >> 2);
				return hash;
			}
        };

        Gradient();
		Gradient(std::initializer_list<Key> list, float degrees = 0);
        explicit Gradient(const std::vector<Key>& list, float degrees = 0);

		[[nodiscard]] float GetDegrees() const { return degrees; }
		void SetDegrees(const float degrees) { this->degrees = degrees; }

		[[nodiscard]] uint32_t GetNumKeys() const { return static_cast<uint32_t>(keys.size()); }
		[[nodiscard]] const std::vector<Key>& GetKeys() const { return keys; }
        [[nodiscard]] const Key& GetKeyAt(const uint32_t index) const { return keys[index]; }
		Key& GetKeyAt(const uint32_t index) { return keys[index]; }

        [[nodiscard]] Color Evaluate(float position) const;

		void Clear();
		void AddKey(const Key& key);
		void AddKey(float position, const Color& color);
		void RemoveKeyAt(uint32_t index);

        [[nodiscard]] size_t GetHash(float degrees) const;
        [[nodiscard]] size_t GetHash() const;

		bool operator==(const Gradient& rhs) const { return GetHash() == rhs.GetHash(); }
		bool operator!=(const Gradient& rhs) const { return !operator==(rhs); }

    private:
        std::vector<Key> keys{};
		float degrees = 0.0f;
    };

}

/// -------------------------------------------------------
