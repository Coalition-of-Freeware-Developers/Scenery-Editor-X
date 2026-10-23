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
 * xp_facade.cpp
 * -------------------------------------------------------
 * Created: 19/10/2026
 * -------------------------------------------------------
 */
#include "xp_facade.h"
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <set>
#include <utility>

// -------------------------------------------------------

namespace SceneryEditorX::XPlane
{
	namespace
	{
		/* @brief "12", "3.5", "0.15": meters without trailing zeros. */
		std::string Meters(const double meters)
		{
			char text[32];
			if (std::abs(meters - std::round(meters)) < 0.005)
				snprintf(text, sizeof(text), "%.0f", meters);
			else if (std::abs(meters * 10.0 - std::round(meters * 10.0)) < 0.05)
				snprintf(text, sizeof(text), "%.1f", meters);
			else
				snprintf(text, sizeof(text), "%.2f", meters);
			return text;
		}

		double WrapDegrees(double degrees)
		{
			degrees = std::fmod(degrees, 360.0);
			return degrees < 0.0 ? degrees + 360.0 : degrees;
		}

		/* @brief The filters of a wall type, or nullptr when it doesn't exist. */
		const std::vector<FacadeAsset::WallFilter> *GetFilters(const FacadeAsset &asset, const int floor, const int wall)
		{
			if (wall < 0)
				return nullptr;
			if (asset.IsType2())
			{
				if (floor < 0 || static_cast<size_t>(floor) >= asset.floors.size() || static_cast<size_t>(wall) >= asset.floors[static_cast<size_t>(floor)].walls.size())
					return nullptr;
				return &asset.floors[static_cast<size_t>(floor)].walls[static_cast<size_t>(wall)].filters;
			}
			return static_cast<size_t>(wall) < asset.walls.size() ? &asset.walls[static_cast<size_t>(wall)].filters : nullptr;
		}

		int CountWalls(const FacadeAsset &asset, const int floor)
		{
			if (!asset.IsType2())
				return static_cast<int>(asset.walls.size());
			if (floor < 0 || static_cast<size_t>(floor) >= asset.floors.size())
				return 0;
			return static_cast<int>(asset.floors[static_cast<size_t>(floor)].walls.size());
		}

		bool Accepts(const std::vector<FacadeAsset::WallFilter> &filters, const double length, const double relativeHeading)
		{
			if (filters.empty())
				return true;
			return std::ranges::any_of(filters, [&](const FacadeAsset::WallFilter &filter) { return filter.Accepts(length, relativeHeading); });
		}

		/* @brief A spelling with its natural width. */
		struct Spelling
		{
			const std::vector<int> *indices = nullptr;
			double width = 0.0;
		};

		std::vector<Spelling> SortedSpellings(const FacadeAsset::Floor &floor, const FacadeAsset::WallRule &wall)
		{
			std::vector<Spelling> spellings;
			for (const std::vector<int> &indices : wall.spellings)
			{
				double width = 0.0;
				for (const int index : indices)
				{
					if (index >= 0 && static_cast<size_t>(index) < floor.segments.size())
						width += floor.segments[static_cast<size_t>(index)].width;
				}
				if (width > 1e-6)
					spellings.push_back({ &indices, width });
			}
			// WorldEditor sorts by width (a stable sort keeps the file order of equal ones)
			std::ranges::stable_sort(spellings, {}, &Spelling::width);
			return spellings;
		}

		/* @brief WorldEditor's closer_to: b is nearer the target than a. */
		bool CloserTo(const double a, const double b, const double target)
		{
			return std::abs(target - a) > std::abs(target - b);
		}
	}

	// -------------------------------------------------------

	const char *GetFacadeFamilyName(const FacadeFamily family)
	{
		switch (family)
		{
			case FacadeFamily::Building:	return "Building";
			case FacadeFamily::Ground:		return "Ground (sidewalk, curb, planter)";
			case FacadeFamily::Enclosure:	return "Enclosure (walls without a roof)";
			case FacadeFamily::Line:		return "Line (fence, wall, hangar, door)";
			default:						return "Facade";
		}
	}

	FacadeFamily GetFacadeFamily(const FacadeAsset &asset)
	{
		if (!asset.ring)
			return FacadeFamily::Line;
		const bool roof = asset.hasRoof && !asset.noRoofMesh;
		const FacadeHeights heights = GetFacadeHeights(asset);
		if (roof && heights.maximum > 0.0f && heights.maximum < 1.0f)
			return FacadeFamily::Ground;
		return roof ? FacadeFamily::Building : FacadeFamily::Enclosure;
	}

	std::vector<std::string> GetFacadeWallNames(const FacadeAsset &asset)
	{
		std::vector<std::string> names;
		if (asset.IsType2())
		{
			if (!asset.floors.empty())
			{
				for (const FacadeAsset::WallRule &wall : asset.floors.front().walls)
					names.push_back(wall.name);
			}
		}
		else
		{
			for (const FacadeAsset::Wall &wall : asset.walls)
				names.push_back(wall.name);
		}
		for (size_t i = 0; i < names.size(); ++i)
		{
			if (names[i].empty())
				names[i] = "Wall " + std::to_string(i + 1);
		}
		return names;
	}

	int GetFacadeWallCount(const FacadeAsset &asset)
	{
		return CountWalls(asset, asset.IsType2() ? 0 : -1);
	}

	std::string DescribeFacadeWall(const FacadeAsset &asset, const int wall, const int floor)
	{
		const std::vector<FacadeAsset::WallFilter> *filters = GetFilters(asset, asset.IsType2() ? floor : -1, wall);
		if (!filters || filters->empty())
			return "any width";
		std::string text;
		for (const FacadeAsset::WallFilter &filter : *filters)
		{
			if (!text.empty())
				text += " or ";
			const bool anyMin = filter.minWidth <= 0.5f, anyMax = filter.maxWidth >= 900.0f;
			if (anyMin && anyMax)
				text += "any width";
			else if (anyMax)
				text += Meters(filter.minWidth) + " m and wider";
			else if (anyMin)
				text += "up to " + Meters(filter.maxWidth) + " m";
			else
				text += Meters(filter.minWidth) + " - " + Meters(filter.maxWidth) + " m";
			if (filter.minHeading != filter.maxHeading)
				text += ", facing " + Meters(filter.minHeading) + " - " + Meters(filter.maxHeading) + " deg from the front";
		}
		return text;
	}

	FacadeHeights GetFacadeHeights(const FacadeAsset &asset)
	{
		FacadeHeights out;
		if (asset.IsType2())
		{
			for (const FacadeAsset::Floor &floor : asset.floors)
			{
				if (floor.segments.empty() || floor.walls.empty())
					continue;
				out.choices.push_back(floor.height);
			}
			std::ranges::sort(out.choices);
			out.choices.erase(std::unique(out.choices.begin(), out.choices.end(), [](const float a, const float b) { return std::abs(a - b) < 0.005f; }), out.choices.end());
			if (!out.choices.empty())
			{
				out.minimum = out.choices.front();
				out.maximum = out.choices.back();
			}
			out.fixed = out.choices.size() <= 1;
			return out;
		}

		// Type 1: bottom and top rows once, middle rows repeated (each one a floor); FLOORS_MIN / MAX clamp the count
		if (asset.walls.empty())
		{
			out.fixed = true;
			return out;
		}
		const FacadeAsset::Wall &wall = asset.walls.front();
		const auto rowsHeight = [&wall](const std::vector<std::pair<float, float>> &rows)
		{
			double height = 0.0;
			for (const auto &[t1, t2] : rows)
				height += static_cast<double>(t2 - t1) * wall.scaleT;
			return height;
		};
		const double fixed = rowsHeight(wall.bottomFloors) + rowsHeight(wall.topFloors);
		const double middle = rowsHeight(wall.middleFloors);
		const int fixedRows = static_cast<int>(wall.bottomFloors.size() + wall.topFloors.size());
		out.floorHeight = wall.middleFloors.empty() ? 0.0f : static_cast<float>(middle / static_cast<double>(wall.middleFloors.size()));
		if (out.floorHeight <= 0.0f)
		{
			out.minimum = out.maximum = static_cast<float>(fixed);
			out.fixed = true;
			return out;
		}
		out.minimum = static_cast<float>(fixed + out.floorHeight * std::max(fixed > 0.0 ? 0 : 1, asset.floorsMin - fixedRows));
		out.maximum = asset.floorsMax >= 999 ? std::max(out.minimum, 300.0f) : static_cast<float>(fixed + out.floorHeight * std::max(0, asset.floorsMax - fixedRows));
		out.maximum = std::max(out.maximum, out.minimum);
		return out;
	}

	std::string DescribeFacadeHeights(const FacadeHeights &heights)
	{
		if (heights.choices.size() > 1)
			return Meters(heights.minimum) + " - " + Meters(heights.maximum) + " m, " + std::to_string(heights.choices.size()) + " heights";
		if (heights.fixed)
			return heights.maximum > 0.0f ? Meters(heights.maximum) + " m (fixed)" : std::string("set by its segments");
		const std::string text = Meters(heights.floorHeight) + " m floors, " + Meters(heights.minimum);
		return text + (heights.maximum >= 300.0f ? " m and up" : " - " + Meters(heights.maximum) + " m");
	}

	double SnapFacadeHeight(const FacadeAsset &asset, const double height)
	{
		if (asset.IsType2())
		{
			const int floor = ChooseFacadeFloor(asset, height);
			return floor < 0 ? height : asset.floors[static_cast<size_t>(floor)].height;
		}
		const FacadeHeights heights = GetFacadeHeights(asset);
		if (heights.fixed)
			return heights.maximum > 0.0f ? heights.maximum : height;
		return std::clamp(height, static_cast<double>(heights.minimum), static_cast<double>(heights.maximum));
	}

	int ChooseFacadeFloor(const FacadeAsset &asset, const double height)
	{
		int best = -1;
		double bestError = 1e30;
		for (size_t i = 0; i < asset.floors.size(); ++i)
		{
			const FacadeAsset::Floor &floor = asset.floors[i];
			if (floor.segments.empty() || floor.walls.empty())
				continue;
			const double error = std::abs(static_cast<double>(floor.height) - height);
			if (error < bestError - 1e-6)
			{
				bestError = error;
				best = static_cast<int>(i);
			}
		}
		return best;
	}

	double GetFacadeRoofHeight(const FacadeAsset::Floor &floor, const double height)
	{
		if (floor.roofHeights.empty())
			return floor.height;
		double best = floor.roofHeights.front();
		for (const float roof : floor.roofHeights)
		{
			if (std::abs(roof - height) < std::abs(best - height))
				best = roof;
		}
		return best;
	}

	int ChooseFacadeWall(const FacadeAsset &asset, const int floor, const double length, const double relativeHeading, const int requested, const bool pickWalls, bool *fits)
	{
		const int count = CountWalls(asset, floor);
		if (count <= 0)
		{
			if (fits)
				*fits = false;
			return 0;
		}
		if (pickWalls)
		{
			const int wall = std::clamp(requested, 0, count - 1);
			if (fits)
				*fits = Accepts(*GetFilters(asset, floor, wall), length, relativeHeading);
			return wall;
		}
		for (int wall = 0; wall < count; ++wall)
		{
			if (Accepts(*GetFilters(asset, floor, wall), length, relativeHeading))
			{
				if (fits)
					*fits = true;
				return wall;
			}
		}
		if (fits)
			*fits = false;
		return 0;
	}

	std::vector<int> PickFacadeSpelling(const FacadeAsset::Floor &floor, const FacadeAsset::WallRule &wall, const double length, double &total)
	{
		std::vector<int> out;
		total = 0.0;
		const std::vector<Spelling> spellings = SortedSpellings(floor, wall);
		if (spellings.empty())
			return out;

		const size_t count = spellings.size();
		const auto add = [&](const Spelling &spelling)
		{
			out.insert(out.end(), spelling.indices->begin(), spelling.indices->end());
			total += spelling.width;
		};
		for (int guard = 0; guard < 4096; ++guard)
		{
			if (total + spellings.back().width < length)
			{
				// Longer than everything left: a middling spelling (WorldEditor's int_seedrand is the middle of the range)
				const size_t low = std::min(count / 4, count - 1);
				const size_t high = std::min(count * 3 / 4, count - 1);
				add(spellings[(low + high) / 2]);
				continue;
			}
			int best = -1;
			for (int n = static_cast<int>(count) - 1; n >= 0; --n)
			{
				const double with = total + spellings[static_cast<size_t>(n)].width;
				if (!CloserTo(total, with, length))
					continue;
				if (best == -1 || CloserTo(total + spellings[static_cast<size_t>(best)].width, with, length))
				{
					best = n;
					if (with < length)
						break;
				}
			}
			if (best != -1)
				add(spellings[static_cast<size_t>(best)]);
			break;
		}
		if (out.empty())
			add(spellings.front());
		return out;
	}

	FacadeLayout LayoutFacade(const FacadeAsset &asset, const std::vector<FacadeSideInput> &sides, const bool pickWalls, const double height)
	{
		FacadeLayout layout;
		layout.roofHeight = height;
		if (asset.IsType2())
		{
			layout.floor = ChooseFacadeFloor(asset, height);
			if (layout.floor >= 0)
				layout.roofHeight = GetFacadeRoofHeight(asset.floors[static_cast<size_t>(layout.floor)], height);
		}

		const double front = sides.empty() ? 0.0 : sides.front().heading;
		layout.walls.reserve(sides.size());
		for (const FacadeSideInput &side : sides)
		{
			FacadeWallLayout wall;
			const double relative = WrapDegrees(side.heading - front);
			wall.wall = ChooseFacadeWall(asset, layout.floor, side.length, relative, side.wallType, pickWalls, &wall.fits);
			wall.chosen = !pickWalls;
			if (asset.IsType2() && layout.floor >= 0)
			{
				const FacadeAsset::Floor &floor = asset.floors[static_cast<size_t>(layout.floor)];
				if (static_cast<size_t>(wall.wall) < floor.walls.size())
				{
					wall.spelling = PickFacadeSpelling(floor, floor.walls[static_cast<size_t>(wall.wall)], side.length, wall.total);
					wall.stretch = wall.total > 1e-6 ? side.length / wall.total : 1.0;
				}
			}
			layout.walls.push_back(std::move(wall));
		}
		return layout;
	}

	std::vector<double> GetFacadeNaturalLengths(const FacadeAsset &asset, int floor, const int wall, const double maxLength, const size_t limit)
	{
		std::vector<double> out;
		if (!asset.IsType2() || asset.floors.empty())
			return out;
		if (floor < 0 || static_cast<size_t>(floor) >= asset.floors.size())
			floor = 0;
		const FacadeAsset::Floor &f = asset.floors[static_cast<size_t>(floor)];
		if (wall < 0 || static_cast<size_t>(wall) >= f.walls.size())
			return out;
		const FacadeAsset::WallRule &rule = f.walls[static_cast<size_t>(wall)];
		const std::vector<Spelling> spellings = SortedSpellings(f, rule);
		if (spellings.empty())
			return out;

		// Candidates: sums of spelling widths (in millimeters, so float noise doesn't split equal sums)
		std::set<long long> candidates;
		std::vector<long long> widths;
		for (const Spelling &spelling : spellings)
			widths.push_back(std::llround(spelling.width * 1000.0));
		widths.erase(std::unique(widths.begin(), widths.end()), widths.end());
		const long long max = std::llround(maxLength * 1000.0);
		std::vector<long long> frontier(widths.begin(), widths.end());
		for (const long long w : frontier)
			if (w <= max)
				candidates.insert(w);
		for (int depth = 0; depth < 6 && !frontier.empty() && candidates.size() < 4000; ++depth)
		{
			std::vector<long long> next;
			for (const long long sum : frontier)
			{
				for (const long long w : widths)
				{
					const long long total = sum + w;
					if (total <= max && candidates.insert(total).second)
						next.push_back(total);
				}
			}
			frontier.swap(next);
		}

		// Keep the lengths the spelling choice really fills without stretching
		for (const long long candidate : candidates)
		{
			const double length = static_cast<double>(candidate) / 1000.0;
			double total = 0.0;
			PickFacadeSpelling(f, rule, length, total);
			if (std::abs(total - length) < 0.005)
			{
				out.push_back(length);
				if (out.size() >= limit)
					break;
			}
		}
		return out;
	}

	double SnapFacadeLength(const std::vector<double> &natural, const double length)
	{
		if (natural.empty())
			return length;
		double best = natural.front();
		for (const double candidate : natural)
		{
			if (std::abs(candidate - length) < std::abs(best - length))
				best = candidate;
		}
		return best;
	}

	// -------------------------------------------------------

	FacadePath::FacadePath(std::vector<Point2> points)
	{
		for (const Point2 &point : points)
		{
			if (!m_Points.empty() && std::hypot(point.x - m_Points.back().x, point.z - m_Points.back().z) < 1e-6)
				continue;
			m_Points.push_back(point);
		}
		m_Distances.reserve(m_Points.size());
		double distance = 0.0;
		for (size_t i = 0; i < m_Points.size(); ++i)
		{
			if (i > 0)
				distance += std::hypot(m_Points[i].x - m_Points[i - 1].x, m_Points[i].z - m_Points[i - 1].z);
			m_Distances.push_back(distance);
		}
	}

	void FacadePath::Evaluate(const double distance, Point2 &position, Point2 &direction) const
	{
		const size_t count = m_Points.size();
		if (count < 2)
		{
			position = count == 1 ? m_Points.front() : Point2{};
			direction = { 1.0, 0.0 };
			return;
		}
		const auto pieceDirection = [this](const size_t i)
		{
			const double dx = m_Points[i + 1].x - m_Points[i].x, dz = m_Points[i + 1].z - m_Points[i].z;
			const double length = std::max(1e-12, std::hypot(dx, dz));
			return Point2{ dx / length, dz / length };
		};
		const double length = GetLength();
		if (distance <= 0.0 || distance >= length)
		{
			// Straight on past the ends
			const bool start = distance <= 0.0;
			direction = pieceDirection(start ? 0 : count - 2);
			const Point2 &end = start ? m_Points.front() : m_Points.back();
			const double past = start ? distance : distance - length;
			position = { end.x + direction.x * past, end.z + direction.z * past };
			return;
		}

		const size_t i = static_cast<size_t>(std::upper_bound(m_Distances.begin(), m_Distances.end(), distance) - m_Distances.begin()) - 1;
		const size_t piece = std::min(i, count - 2);
		const double span = std::max(1e-12, m_Distances[piece + 1] - m_Distances[piece]);
		const double t = std::clamp((distance - m_Distances[piece]) / span, 0.0, 1.0);
		position = { m_Points[piece].x + (m_Points[piece + 1].x - m_Points[piece].x) * t, m_Points[piece].z + (m_Points[piece + 1].z - m_Points[piece].z) * t };

		// Tangents averaged at the inner points and blended along the piece: offset geometry stays joined at the kinks
		const Point2 here = pieceDirection(piece);
		const auto tangentAt = [&](const size_t point)
		{
			if (point == 0 || point == count - 1)
				return here;
			const Point2 a = pieceDirection(point - 1), b = pieceDirection(point);
			const double x = a.x + b.x, z = a.z + b.z, l = std::hypot(x, z);
			return l > 1e-9 ? Point2{ x / l, z / l } : b;
		};
		const Point2 t0 = tangentAt(piece), t1 = tangentAt(piece + 1);
		const double x = t0.x + (t1.x - t0.x) * t, z = t0.z + (t1.z - t0.z) * t, l = std::hypot(x, z);
		direction = l > 1e-9 ? Point2{ x / l, z / l } : here;
	}
}

// -------------------------------------------------------
