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
 * season_model.h
 * -------------------------------------------------------
 * Created: 20/10/2026
 * -------------------------------------------------------
 */
#pragma once
#include <array>
#include <cstdint>
#include <string_view>

// -------------------------------------------------------

namespace SceneryEditorX::Environment
{
	/* @brief X-Plane's seasons, in their cycle order (each SEASONAL texture is the next one's albedo). */
	enum class Season : uint8_t
	{
		Spring,
		Summer,
		Fall,
		Winter
	};

	/* @brief The library season bit (xp_library.h SeasonBits: spring 1, summer 2, fall 4, winter 8). */
	constexpr uint8_t GetSeasonBit(const Season season) { return static_cast<uint8_t>(1u << static_cast<uint8_t>(season)); }

	/* @brief "spr", "sum", "fal", "win" (library.txt codes). */
	const char *GetSeasonCode(Season season);
	/* @brief "Spring" ... "Winter". */
	const char *GetSeasonName(Season season);
	/* @brief Parses a library.txt code; false for anything else. */
	bool ParseSeasonCode(std::string_view code, Season &out);

	/* @brief The season after this one (spring -> summer -> fall -> winter -> spring). */
	constexpr Season NextSeason(const Season season) { return static_cast<Season>((static_cast<uint8_t>(season) + 1) % 4); }

	/**
	 * @brief A DSF tile's season rasters reduced to one cell: for each season the day-of-year range it covers
	 * (spr1/spr2 ... win1/win2). Ranges may wrap the year end (winter 335 .. 59). A season the tile doesn't have
	 * (valid false) never applies.
	 */
	struct DsfSeasonRanges
	{
		std::array<int, 4> start = { 0, 0, 0, 0 };	// Day of year, 1-366, indexed by Season
		std::array<int, 4> end = { 0, 0, 0, 0 };
		std::array<bool, 4> valid = { false, false, false, false };
	};

	/* @brief A season and how far it has moved toward the next one. */
	struct SeasonResult
	{
		Season season = Season::Summer;
		uint8_t bits = GetSeasonBit(Season::Summer);	// The library season bit
		float ratio = 0.0f;								// 0 deep in the season .. 1 at the change to the next
	};

	/**
	 * @brief The season for a day of the year and a latitude (plan TASK-012).
	 *
	 * With DSF ranges: the season whose range holds the day; the ratio rises across the gap to the next season's range
	 * (days between ranges belong to the earlier season) and, inside a range, over its last 15 days. Without ranges
	 * (ASSUMPTION until calibrated against X-Plane): northern spring is days 60-151, summer 152-243, fall 244-334,
	 * winter 335-59; the southern hemisphere is shifted by 182 days; within 23.5 degrees of the equator it is always
	 * summer with ratio 0. The ratio rises over the last 15 days of each season.
	 */
	SeasonResult ComputeSeason(int dayOfYear, double latitudeDeg, const DsfSeasonRanges *ranges = nullptr);

	/* @brief Fallback season boundaries (northern hemisphere): first day of each season. */
	inline constexpr std::array<int, 4> NORTHERN_SEASON_START = { 60, 152, 244, 335 };
	inline constexpr int SEASON_TRANSITION_DAYS = 15;
	inline constexpr double TROPICS_LATITUDE = 23.5;
}

// -------------------------------------------------------
