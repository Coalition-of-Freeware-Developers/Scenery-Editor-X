/**
* -------------------------------------------------------
* Scenery Editor X
* -------------------------------------------------------
* Copyright (c) 2025 Thomas Ray 
* Copyright (c) 2025 Coalition of Freeware Developers
* -------------------------------------------------------
* identifier.h
* -------------------------------------------------------
* Created: 13/7/2025
* -------------------------------------------------------
*/
#pragma once
//#include "hash.h"

/// ---------------------------------------------

namespace SceneryEditorX
{
    class Identifier
	{
	public:
		constexpr Identifier() = default;

        //explicit constexpr Identifier(const std::string_view name) noexcept : hash(Hash::GenerateFNVHash(name.data())), dbgName(name) {}
        explicit constexpr Identifier(const uint32_t hash) noexcept : hash(hash) {}

		constexpr bool operator==(const Identifier& other) const noexcept { return hash == other.hash; }
		constexpr bool operator!=(const Identifier& other) const noexcept { return hash != other.hash; }

        explicit constexpr operator uint32_t() const noexcept { return hash; }
        [[nodiscard]] constexpr std::string_view GetDBGName() const { return dbgName; }

	private:
		friend struct std::hash<Identifier>;
		uint32_t hash = 0;
		std::string_view dbgName;
	};

}

/// ---------------------------------------------
