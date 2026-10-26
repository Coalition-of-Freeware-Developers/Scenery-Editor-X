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
 * season_model.cpp
 * -------------------------------------------------------
 * Created: 20/10/2026
 * -------------------------------------------------------
 */
#include "season_model.h"
#include <algorithm>
#include <cmath>

// -------------------------------------------------------

namespace SceneryEditorX::Environment
{
	namespace
	{
		constexpr int YEAR_DAYS = 365;

		/* @brief Days from a to b going forward through the year (0 .. 364). */
		int DaysForward(const int from, const int to)
		{
			return ((to - from) % YEAR_DAYS + YEAR_DAYS) % YEAR_DAYS;
		}

		/* @brief The day lies in [start, end], which may wrap the year end. */
		bool InRange(const int day, const int start, const int end)
		{
			return DaysForward(start, day) <= DaysForward(start, end);
		}

		/* @brief Ratio over the last SEASON_TRANSITION_DAYS days before `next` (0 before them, 1 at next). */
		float TransitionRatio(const int day, const int next)
		{
			const int left = DaysForward(day, next);	// Days until the next season begins
			if (left == 0 || left > SEASON_TRANSITION_DAYS)
				return 0.0f;
			return static_cast<float>(SEASON_TRANSITION_DAYS - left + 1) / static_cast<float>(SEASON_TRANSITION_DAYS);
		}

		SeasonResult Make(const Season season, const float ratio)
		{
			SeasonResult result;
			result.season = season;
			result.bits = GetSeasonBit(season);
			result.ratio = std::clamp(ratio, 0.0f, 1.0f);
			return result;
		}

		SeasonResult FromRanges(const int day, const DsfSeasonRanges &ranges)
		{
			// The season holding the day, or the last one that started before it (gaps belong to the earlier season)
			int best = -1;
			int bestSinceStart = YEAR_DAYS;
			for (int s = 0; s < 4; ++s)
			{
				if (!ranges.valid[s])
					continue;
				const int since = DaysForward(ranges.start[s], day);
				if (since < bestSinceStart)
				{
					bestSinceStart = since;
					best = s;
				}
			}
			if (best < 0)
				return Make(Season::Summer, 0.0f);

			// The next valid season in the cycle
			int next = -1;
			for (int step = 1; step < 4 && next < 0; ++step)
			{
				const int candidate = (best + step) % 4;
				if (ranges.valid[candidate])
					next = candidate;
			}
			if (next < 0)
				return Make(static_cast<Season>(best), 0.0f);

			const int start = ranges.start[best];
			const int end = ranges.end[best];
			const int nextStart = ranges.start[next];
			if (InRange(day, start, end))
			{
				// Inside the range: rises over its last days only when the next range follows right after it
				if (DaysForward(end, nextStart) <= 1)
					return Make(static_cast<Season>(best), TransitionRatio(day, nextStart));
				return Make(static_cast<Season>(best), 0.0f);
			}
			// In the gap after the range: progress across the gap
			const int gap = DaysForward(end, nextStart);
			const int into = DaysForward(end, day);
			return Make(static_cast<Season>(best), gap > 0 ? static_cast<float>(into) / static_cast<float>(gap) : 1.0f);
		}
	}

	const char *GetSeasonCode(const Season season)
	{
		static constexpr const char *codes[4] = { "spr", "sum", "fal", "win" };
		return codes[static_cast<uint8_t>(season) & 3];
	}

	const char *GetSeasonName(const Season season)
	{
		static constexpr const char *names[4] = { "Spring", "Summer", "Fall", "Winter" };
		return names[static_cast<uint8_t>(season) & 3];
	}

	bool ParseSeasonCode(const std::string_view code, Season &out)
	{
		for (uint8_t s = 0; s < 4; ++s)
		{
			if (code == GetSeasonCode(static_cast<Season>(s)))
			{
				out = static_cast<Season>(s);
				return true;
			}
		}
		return false;
	}

	std::optional<DsfSeasonRanges> MakeDsfSeasonRanges(const std::array<float, 8> &days)
	{
		std::array<int, 8> day{};
		for (size_t i = 0; i < day.size(); ++i)
		{
			const int rounded = static_cast<int>(std::lround(std::clamp(days[i], 0.0f, 366.0f)));
			day[i] = ((rounded - 1) % YEAR_DAYS + YEAR_DAYS) % YEAR_DAYS + 1;	// 1-365: day 0 is Dec 31
		}

		// The days travelled going round the year through the values in turn: a year for one cycle, none without
		int travelled = 0;
		for (size_t i = 0; i < day.size(); ++i)
			travelled += DaysForward(day[i], day[(i + 1) % day.size()]);

		DsfSeasonRanges ranges;
		if (travelled == 0)
		{
			// No seasons: summer all year
			ranges.start[static_cast<size_t>(Season::Summer)] = 1;
			ranges.end[static_cast<size_t>(Season::Summer)] = YEAR_DAYS;
			ranges.valid[static_cast<size_t>(Season::Summer)] = true;
			return ranges;
		}
		if (travelled != YEAR_DAYS)
			return std::nullopt;	// The seasons overlap
		for (size_t s = 0; s < 4; ++s)
		{
			ranges.start[s] = day[2 * s];
			ranges.end[s] = day[2 * s + 1];
			ranges.valid[s] = true;
		}
		return ranges;
	}

	SeasonResult ComputeSeason(int dayOfYear, const double latitudeDeg, const DsfSeasonRanges *ranges)
	{
		dayOfYear = std::clamp(dayOfYear, 1, 366);
		if (dayOfYear == 366)
			dayOfYear = 365;	// Leap day 366 behaves like Dec 31

		if (ranges && std::ranges::any_of(ranges->valid, [](const bool v) { return v; }))
			return FromRanges(dayOfYear, *ranges);

		if (std::abs(latitudeDeg) < TROPICS_LATITUDE)
			return Make(Season::Summer, 0.0f);

		// The southern hemisphere's seasons run half a year apart
		const int day = latitudeDeg < 0.0 ? (dayOfYear - 1 + 182) % YEAR_DAYS + 1 : dayOfYear;
		for (int s = 3; s >= 0; --s)
		{
			const int start = NORTHERN_SEASON_START[s];
			const int nextStart = NORTHERN_SEASON_START[(s + 1) % 4];
			const int end = (nextStart - 2 + YEAR_DAYS) % YEAR_DAYS + 1;	// The day before the next season
			if (InRange(day, start, end))
				return Make(static_cast<Season>(s), TransitionRatio(day, nextStart));
		}
		return Make(Season::Summer, 0.0f);
	}
}

// -------------------------------------------------------
