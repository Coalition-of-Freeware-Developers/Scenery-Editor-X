/**
 * -------------------------------------------------------
 * Scenery Editor X
 * -------------------------------------------------------
 * Copyright (c) 2026 Thomas Ray
 * Copyright (c) 2026 Coalition of Freeware Developers
 * -------------------------------------------------------
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 * -------------------------------------------------------
 * sky_conditions.h
 * -------------------------------------------------------
 * Created: 20/10/2026
 * -------------------------------------------------------
 */
#pragma once
#include <array>
#include <cstdint>
#include <filesystem>
#include <string>
#include <string_view>

// -------------------------------------------------------

namespace SceneryEditorX::Environment
{
	struct EnvironmentState;

	/* @brief X-Plane 12's sky-condition classes (art controls skyc/<ratio>_<class>), in X-Plane's order. */
	enum class SkyCondition : uint8_t
	{
		Clean,		// Clear air, visibility 40 km and more
		Hazy,		// 5-40 km
		Foggy,		// Under about 1.6 km
		Socked,		// "sockd": socked in, under about 400 m
		Overcast,	// "ocast": full low cloud cover
		Stratus,	// "strat": full low stratus
		Snowy,		// Falling snow
		Mountain,	// "mount": reserved (weight 0)
		HighAltitude,	// "hialt": observer above about 9 km
		Orbit,		// Observer above about 30 km
		Final,		// Reserved (weight 0)
		Count
	};

	inline constexpr size_t SKY_CONDITION_COUNT = static_cast<size_t>(SkyCondition::Count);

	/* @brief The skyc suffix of a class ("clean", "hazy", "foggy", "sockd", "ocast", "strat", ...). */
	const char *GetSkyConditionCode(SkyCondition condition);

	/* @brief The multipliers each class carries (X-Plane: skyc/<ratio>_<class>). Offsets add, ratios multiply. */
	struct SkyMultipliers
	{
		float ambient = 1.0f;
		float direct = 1.0f;
		float tone = 1.0f;
		float rayleigh = 1.0f;
		float mie = 1.0f;
		float shadowLevel = 1.0f;
		float shadowOffset = 0.0f;

		bool operator==(const SkyMultipliers &) const = default;
	};

	using SkyWeights = std::array<float, SKY_CONDITION_COUNT>;

	/**
	 * @brief Weights (summing to 1) of each class for the weather and the observer's altitude (plan TASK-013).
	 * Visibility blends socked (250 m), foggy (1 km), hazy (10 km) and clean (40 km) in log space; a full low cloud
	 * deck (coverage 7/8 and more, base under 1000 m above the ground) turns it overcast or stratus; falling snow
	 * (rain below freezing) snowy; the observer's altitude high-altitude (from 6 to 9 km) and orbit (20 to 30 km).
	 * @param groundElevationM Ground under the observer, for the cloud base above the ground.
	 */
	SkyWeights ComputeSkyWeights(const EnvironmentState &state, float observerAltitudeMslM, float groundElevationM);

	/* @brief The per-class multipliers, loaded from resources/environment/xp12_sky_conditions.json (1.0 until calibrated). */
	class SkyConditionTable
	{
	public:
		SkyConditionTable();

		/* @brief Reads the table; missing classes or fields keep their defaults. False (and error) when it can't be read. */
		bool Load(const std::filesystem::path &path, std::string *error = nullptr);
		bool Parse(std::string_view json, std::string *error = nullptr);

		[[nodiscard]] const SkyMultipliers &Get(SkyCondition condition) const { return m_Classes[static_cast<size_t>(condition)]; }
		void Set(SkyCondition condition, const SkyMultipliers &multipliers) { m_Classes[static_cast<size_t>(condition)] = multipliers; }

		/* @brief The weighted blend of the classes. */
		[[nodiscard]] SkyMultipliers Blend(const SkyWeights &weights) const;

		[[nodiscard]] const std::string &GetXPlaneBuild() const { return m_XPlaneBuild; }

	private:
		std::array<SkyMultipliers, SKY_CONDITION_COUNT> m_Classes;
		std::string m_XPlaneBuild;
	};
}

// -------------------------------------------------------
