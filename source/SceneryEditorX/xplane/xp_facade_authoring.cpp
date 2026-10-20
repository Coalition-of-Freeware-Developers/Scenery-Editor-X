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
 * xp_facade_authoring.cpp
 * -------------------------------------------------------
 * Created: 19/10/2026
 * -------------------------------------------------------
 */
#include "xp_facade_authoring.h"
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <fstream>

// -------------------------------------------------------

namespace SceneryEditorX::XPlane
{
	namespace
	{
		constexpr FacadePanelRow ACROSS[] = { FacadePanelRow::Left, FacadePanelRow::Center, FacadePanelRow::Right };
		constexpr FacadePanelRow UP[] = { FacadePanelRow::Bottom, FacadePanelRow::Middle, FacadePanelRow::Top };
		constexpr float EDGE_EPSILON = 1e-6f;

		/* @brief The rows of one axis. */
		const FacadePanelRow (&Rows(const bool floors))[3]
		{
			return floors ? UP : ACROSS;
		}

		/* @brief Quarters of a texture axis as the parser would read whole pixels: [0, 1/4], [1/4, 3/4], [3/4, 1]. */
		void QuarterCut(std::vector<std::pair<float, float>> &first, std::vector<std::pair<float, float>> &middle, std::vector<std::pair<float, float>> &last, const float size)
		{
			const float quarter = size > 0.0f ? FacadeFromPixels(std::round(size * 0.25f), size) : 0.25f;
			const float threeQuarters = size > 0.0f ? FacadeFromPixels(std::round(size * 0.75f), size) : 0.75f;
			const float one = size > 0.0f ? FacadeFromPixels(size, size) : 1.0f;
			first = { { 0.0f, quarter } };
			middle = { { quarter, threeQuarters } };
			last = { { threeQuarters, one } };
		}
	}

	const char *GetFacadePanelRowName(const FacadePanelRow row)
	{
		switch (row)
		{
			case FacadePanelRow::Left:		return "Left";
			case FacadePanelRow::Center:	return "Center";
			case FacadePanelRow::Right:		return "Right";
			case FacadePanelRow::Bottom:	return "Bottom";
			case FacadePanelRow::Middle:	return "Middle";
			case FacadePanelRow::Top:		return "Top";
			default:						return "?";
		}
	}

	std::vector<std::pair<float, float>> &GetFacadePanels(FacadeAsset::Wall &wall, const FacadePanelRow row)
	{
		switch (row)
		{
			case FacadePanelRow::Left:		return wall.leftPanels;
			case FacadePanelRow::Right:		return wall.rightPanels;
			case FacadePanelRow::Bottom:	return wall.bottomFloors;
			case FacadePanelRow::Middle:	return wall.middleFloors;
			case FacadePanelRow::Top:		return wall.topFloors;
			default:						return wall.centerPanels;
		}
	}

	const std::vector<std::pair<float, float>> &GetFacadePanels(const FacadeAsset::Wall &wall, const FacadePanelRow row)
	{
		return GetFacadePanels(const_cast<FacadeAsset::Wall &>(wall), row);
	}

	// -------------------------------------------------------
	// Type 1

	FacadeAsset::Wall MakeType1Wall(const FacadeAsset &asset)
	{
		FacadeAsset::Wall wall;
		if (!asset.walls.empty())
			wall = asset.walls.back();
		else
		{
			wall.scaleS = 10.0f;
			wall.scaleT = 10.0f;
			wall.textureWidth = asset.roofTextureWidth;
			wall.textureHeight = asset.roofTextureHeight;
			QuarterCut(wall.leftPanels, wall.centerPanels, wall.rightPanels, wall.textureWidth);
			QuarterCut(wall.bottomFloors, wall.middleFloors, wall.topFloors, wall.textureHeight);
		}
		wall.name = "Wall " + std::to_string(asset.walls.size() + 1);
		wall.minWidth = 0.0f;
		wall.maxWidth = 9999.0f;
		wall.filters = { FacadeAsset::WallFilter{ 0.0f, 9999.0f, 0.0f, 0.0f } };
		return wall;
	}

	FacadeAsset MakeType1Facade(const std::string &texture, const int textureWidth, const int textureHeight, const float metersAcross)
	{
		FacadeAsset asset;
		asset.version = 800;
		asset.ring = true;
		asset.wallTexture = texture;
		const float width = static_cast<float>(std::max(0, textureWidth)), height = static_cast<float>(std::max(0, textureHeight));
		asset.roofTextureWidth = width;
		asset.roofTextureHeight = height;

		FacadeAsset::Wall wall = MakeType1Wall(asset);
		wall.scaleS = metersAcross > 0.0f ? metersAcross : 10.0f;
		wall.scaleT = width > 0.0f && height > 0.0f ? wall.scaleS * height / width : wall.scaleS;
		asset.walls.push_back(std::move(wall));

		SetFacadeRoofRectangle(asset, 0.0f, 0.0f, FacadeFromPixels(width, width), FacadeFromPixels(height, height));
		if (width <= 0.0f || height <= 0.0f)
			SetFacadeRoofRectangle(asset, 0.0f, 0.0f, 1.0f, 1.0f);
		return asset;
	}

	std::vector<float> GetFacadeTextureCuts(const FacadeAsset::Wall &wall, const bool floors)
	{
		std::vector<float> cuts;
		for (const FacadePanelRow row : Rows(floors))
		{
			for (const auto &[from, to] : GetFacadePanels(wall, row))
			{
				cuts.push_back(from);
				cuts.push_back(to);
			}
		}
		std::ranges::sort(cuts);
		const auto end = std::unique(cuts.begin(), cuts.end(), [](const float a, const float b) { return std::abs(a - b) < EDGE_EPSILON; });
		cuts.erase(end, cuts.end());
		return cuts;
	}

	float MoveFacadeTextureCut(FacadeAsset::Wall &wall, const bool floors, const float from, const float to, const float minimum)
	{
		float lo = 0.0f, hi = 1.0f;
		bool found = false;
		for (const FacadePanelRow row : Rows(floors))
		{
			for (const auto &[a, b] : GetFacadePanels(wall, row))
			{
				if (std::abs(a - from) < EDGE_EPSILON)
				{
					found = true;
					hi = std::min(hi, b - minimum);	// A start stays before its end
				}
				if (std::abs(b - from) < EDGE_EPSILON)
				{
					found = true;
					lo = std::max(lo, a + minimum);
				}
			}
		}
		if (!found || lo > hi)
			return from;
		const float target = std::clamp(to, lo, hi);
		for (const FacadePanelRow row : Rows(floors))
		{
			for (auto &[a, b] : GetFacadePanels(wall, row))
			{
				if (std::abs(a - from) < EDGE_EPSILON)
					a = target;
				if (std::abs(b - from) < EDGE_EPSILON)
					b = target;
			}
		}
		return target;
	}

	bool SplitFacadePanel(FacadeAsset::Wall &wall, const FacadePanelRow row, const size_t index)
	{
		std::vector<std::pair<float, float>> &panels = GetFacadePanels(wall, row);
		if (index >= panels.size())
			return false;
		const auto [a, b] = panels[index];
		float middle = 0.5f * (a + b);
		const float size = IsFacadeFloorRow(row) ? wall.textureHeight : wall.textureWidth;
		if (size > 0.0f)
		{
			// On a whole pixel when that leaves both halves something
			const float pixel = FacadeFromPixels(std::round(middle * size), size);
			if (pixel > a && pixel < b)
				middle = pixel;
		}
		if (!(middle > a && middle < b))
			return false;
		panels[index].second = middle;
		panels.insert(panels.begin() + static_cast<std::ptrdiff_t>(index) + 1, { middle, b });
		return true;
	}

	void SnapFacadeWallToPixels(FacadeAsset::Wall &wall)
	{
		const auto snap = [](float &value, const float size)
		{
			if (size > 0.0f)
				value = FacadeFromPixels(std::round(value * size), size);
		};
		for (int r = 0; r < static_cast<int>(FacadePanelRow::Count); ++r)
		{
			const FacadePanelRow row = static_cast<FacadePanelRow>(r);
			const float size = IsFacadeFloorRow(row) ? wall.textureHeight : wall.textureWidth;
			for (auto &[a, b] : GetFacadePanels(wall, row))
			{
				snap(a, size);
				snap(b, size);
			}
		}
		snap(wall.basement, wall.textureHeight);
	}

	void SetFacadeTextureSize(FacadeAsset &asset, const int width, const int height)
	{
		const float w = static_cast<float>(std::max(0, width)), h = static_cast<float>(std::max(0, height));
		for (FacadeAsset::Wall &wall : asset.walls)
		{
			wall.textureWidth = w;
			wall.textureHeight = h;
		}
		asset.roofTextureWidth = w;
		asset.roofTextureHeight = h;
	}

	void SetFacadeRoofRectangle(FacadeAsset &asset, const float s1, const float t1, const float s2, const float t2)
	{
		asset.roofUv = { { s1, t1 }, { s2, t1 }, { s2, t2 }, { s1, t2 } };
		asset.roofScaled = false;
		asset.roofCenter = { 0.0f, 0.0f };
		asset.hasRoof = true;
	}

	void SetFacadeRoofScaled(FacadeAsset &asset, float s1, float t1, float s2, float t2, const float metersS, const float metersT)
	{
		if (s2 < s1)
			std::swap(s1, s2);
		if (t2 < t1)
			std::swap(t1, t2);
		SetFacadeRoofRectangle(asset, s1, t1, s2, t2);
		asset.roofScaled = s2 > s1 && t2 > t1;
		if (!asset.roofScaled)
			return;
		asset.roofCenter = { 0.5f * (s1 + s2), 0.5f * (t1 + t2) };
		asset.roofScaleS = std::max(0.01f, metersS) / (s2 - s1);
		asset.roofScaleT = std::max(0.01f, metersT) / (t2 - t1);
	}

	std::pair<float, float> GetFacadeRoofMeters(const FacadeAsset &asset)
	{
		if (asset.roofUv.size() != 4)
			return { asset.roofScaleS, asset.roofScaleT };
		return { asset.roofScaleS * (asset.roofUv[2].first - asset.roofUv[0].first), asset.roofScaleT * (asset.roofUv[2].second - asset.roofUv[0].second) };
	}

	// -------------------------------------------------------
	// Type 2

	const std::vector<CurbProfile> &GetCurbProfilePresets()
	{
		static const std::vector<CurbProfile> PRESETS =
		{
			// X-Plane 12's lib/airport/ground/sidewalk_15cm: a slightly sloped top, a beveled edge, buried 6 cm
			{ "Sidewalk curb 15 cm", { { 0.0f, 0.15f }, { 0.1f, 0.145f }, { 0.15f, 0.02f }, { 0.15f, -0.06f } } },
			{ "Low curb 10 cm", { { 0.0f, 0.1f }, { 0.08f, 0.096f }, { 0.12f, 0.01f }, { 0.12f, -0.06f } } },
			{ "Flush edge 5 cm", { { 0.0f, 0.05f }, { 0.05f, 0.0f }, { 0.05f, -0.06f } } },
			{ "Planter wall 45 cm", { { 0.0f, 0.45f }, { 0.2f, 0.45f }, { 0.22f, 0.43f }, { 0.22f, -0.1f } } },
			{ "Retaining wall 1 m", { { 0.0f, 1.0f }, { 0.3f, 1.0f }, { 0.3f, -0.1f } } },
		};
		return PRESETS;
	}

	namespace
	{
		/**
		 * @brief The profile swept `width` meters down a wall (along -z): one quad per profile edge per piece, pieces
		 * breaking where the texture strip restarts and at `cuts` even divisions (where a curved wall bends).
		 */
		FacadeAsset::Mesh SweepProfile(const CurbFacadeSpec &spec, const float width, const int cuts)
		{
			FacadeAsset::Mesh mesh;
			mesh.group = 0;
			mesh.lodFar = spec.lodFar;
			mesh.cuts = std::max(1, cuts);
			const std::vector<std::pair<float, float>> &profile = spec.profile;
			if (profile.size() < 2 || width <= 0.0f)
				return mesh;

			// t down the cross-section by its length: t2 at the top edge, t1 at the ground
			std::vector<double> along(profile.size(), 0.0);
			for (size_t i = 1; i < profile.size(); ++i)
				along[i] = along[i - 1] + std::hypot(profile[i].first - profile[i - 1].first, profile[i].second - profile[i - 1].second);
			const double profileLength = std::max(1e-6, along.back());

			const double tile = std::max(0.01, static_cast<double>(spec.textureLength));
			std::vector<double> breaks = { 0.0, static_cast<double>(width) };
			for (double d = tile; d < width - 1e-4; d += tile)
				breaks.push_back(d);
			for (int c = 1; c < mesh.cuts; ++c)
				breaks.push_back(static_cast<double>(width) * c / mesh.cuts);
			std::ranges::sort(breaks);
			breaks.erase(std::unique(breaks.begin(), breaks.end(), [](const double a, const double b) { return b - a < 1e-4; }), breaks.end());
			breaks.back() = width;	// Exactly: the parser measures the segment from it

			const auto vertex = [&](const std::pair<float, float> &point, const double d, const double s, const double t, const float nx, const float ny)
			{
				mesh.positions.insert(mesh.positions.end(), { point.first, point.second, d == 0.0 ? 0.0f : static_cast<float>(-d) });
				mesh.normals.insert(mesh.normals.end(), { nx, ny, 0.0f });
				mesh.uvs.insert(mesh.uvs.end(), { static_cast<float>(s), static_cast<float>(t) });
			};
			for (size_t b = 0; b + 1 < breaks.size(); ++b)
			{
				const double d0 = breaks[b], d1 = breaks[b + 1];
				const double strip = std::floor((d0 + 1e-5) / tile);
				const double sA = spec.textureS1 + (spec.textureS2 - spec.textureS1) * (d0 - strip * tile) / tile;
				const double sB = spec.textureS1 + (spec.textureS2 - spec.textureS1) * (d1 - strip * tile) / tile;
				for (size_t i = 0; i + 1 < profile.size(); ++i)
				{
					const float dx = profile[i + 1].first - profile[i].first, dy = profile[i + 1].second - profile[i].second;
					const float length = std::hypot(dx, dy);
					if (length < 1e-5f)
						continue;
					// Out of the solid: the edge turned a quarter (top faces point up, the curb face outward)
					const float nx = -dy / length, ny = dx / length;
					const double tA = spec.textureT2 - (spec.textureT2 - spec.textureT1) * along[i] / profileLength;
					const double tB = spec.textureT2 - (spec.textureT2 - spec.textureT1) * along[i + 1] / profileLength;
					const uint32_t base = static_cast<uint32_t>(mesh.positions.size() / 3);
					vertex(profile[i], d0, sA, tA, nx, ny);
					vertex(profile[i], d1, sB, tA, nx, ny);
					vertex(profile[i + 1], d1, sB, tB, nx, ny);
					vertex(profile[i + 1], d0, sA, tB, nx, ny);
					// Clockwise seen from the front, as X-Plane's own facades wind them
					mesh.indices.insert(mesh.indices.end(), { base, base + 1, base + 2, base, base + 2, base + 3 });
				}
			}
			return mesh;
		}
	}

	FacadeAsset BuildCurbFacade(const CurbFacadeSpec &input)
	{
		CurbFacadeSpec spec = input;
		if (spec.profile.size() < 2)
			spec.profile = GetCurbProfilePresets().front().points;

		FacadeAsset asset;
		asset.version = 1000;
		asset.ring = spec.ring;
		asset.wallTexture = spec.wallTexture;
		asset.wallTextureNormal = spec.wallTextureNormal;
		asset.roofTexture = spec.roofTexture;
		asset.roofTextureNormal = spec.roofTextureNormal;
		asset.roofScaleS = asset.roofScaleT = spec.roofScale > 0.01f ? spec.roofScale : 8.0f;
		asset.hasRoof = true;

		const float top = spec.profile.front().second;
		FacadeAsset::Floor floor;
		char name[48];
		snprintf(name, sizeof(name), "h_%gm", static_cast<double>(top));
		floor.name = name;
		floor.roofHeights = { top };
		floor.roofs = { FacadeAsset::Roof{ top, false, {} } };
		floor.hardRoof = spec.hardRoof;
		floor.height = top;

		FacadeAsset::WallRule wall;
		wall.name = spec.wallName.empty() ? "curb" : spec.wallName;
		wall.minWidth = 0.0f;
		wall.maxWidth = 9999.0f;
		wall.minHeading = 0.0f;
		wall.maxHeading = 0.0f;
		wall.filters = { FacadeAsset::WallFilter{ 0.0f, 9999.0f, 0.0f, 0.0f } };

		std::vector<float> lengths;
		for (const float length : spec.segmentLengths)
		{
			if (length >= 0.01f && std::ranges::none_of(lengths, [&](const float l) { return std::abs(l - length) < 1e-4f; }))
				lengths.push_back(length);
		}
		if (lengths.empty())
			lengths = { 1.0f };
		std::ranges::sort(lengths);

		const float piece = std::clamp(spec.curvedPiece, 0.05f, 100.0f);
		for (const float length : lengths)
		{
			FacadeAsset::Segment straight;
			straight.meshes.push_back(SweepProfile(spec, length, 1));
			straight.width = length;
			FacadeAsset::Segment curved;
			curved.meshes.push_back(SweepProfile(spec, length, std::max(1, static_cast<int>(std::lround(length / piece)))));
			curved.width = length;
			floor.segments.push_back(std::move(straight));
			floor.curvedSegments.push_back(std::move(curved));
		}

		// Spellings for every multiple of the shortest segment, longest segments first (WorldEditor's and X-Plane's
		// spelling choice then fits a wall to within the shortest segment)
		const float step = lengths.front();
		const int steps = std::max(1, static_cast<int>(std::floor(std::max(spec.spellingLength, lengths.back()) / step + 1e-4f)));
		for (int k = 1; k <= steps && k <= 1024; ++k)
		{
			double left = static_cast<double>(step) * k;
			std::vector<int> spelling;
			for (int s = static_cast<int>(lengths.size()) - 1; s >= 0 && left > 1e-4; --s)
			{
				while (left >= lengths[static_cast<size_t>(s)] - 1e-4)
				{
					spelling.push_back(s);
					left -= lengths[static_cast<size_t>(s)];
				}
			}
			if (std::abs(left) < 1e-4 && !spelling.empty())
				wall.spellings.push_back(std::move(spelling));
		}
		floor.walls.push_back(std::move(wall));
		asset.floors.push_back(std::move(floor));
		return asset;
	}

	// -------------------------------------------------------
	// Files

	std::vector<std::string> CheckFacadeAsset(const FacadeAsset &asset)
	{
		std::vector<std::string> problems;
		if (asset.wallTexture.empty())
			problems.emplace_back("No wall texture");

		if (!asset.IsType2())
		{
			if (asset.walls.empty())
				problems.emplace_back("No walls: X-Plane draws nothing");
			for (size_t w = 0; w < asset.walls.size(); ++w)
			{
				const FacadeAsset::Wall &wall = asset.walls[w];
				const std::string label = "Wall " + std::to_string(w + 1) + (wall.name.empty() ? std::string() : " (" + wall.name + ")");
				if (wall.scaleS <= 0.0f || wall.scaleT <= 0.0f)
					problems.push_back(label + ": SCALE must be more than 0 m");
				if (wall.leftPanels.empty() && wall.centerPanels.empty() && wall.rightPanels.empty())
					problems.push_back(label + ": needs at least one left, center or right panel");
				if (wall.bottomFloors.empty() && wall.middleFloors.empty() && wall.topFloors.empty())
					problems.push_back(label + ": needs at least one bottom, middle or top floor");
				bool empty = false, outside = false;
				for (int r = 0; r < static_cast<int>(FacadePanelRow::Count); ++r)
				{
					for (const auto &[a, b] : GetFacadePanels(wall, static_cast<FacadePanelRow>(r)))
					{
						empty |= !(b > a);
						outside |= a < -EDGE_EPSILON || b > 1.0f + EDGE_EPSILON;
					}
				}
				if (empty)
					problems.push_back(label + ": a panel is empty or reversed");
				if (outside)
					problems.push_back(label + ": a panel reaches outside the texture");
				for (const FacadeAsset::WallFilter &filter : wall.filters)
				{
					if (filter.minWidth > filter.maxWidth)
					{
						problems.push_back(label + ": its minimum width is more than its maximum");
						break;
					}
				}
			}
			if (asset.hasRoof && asset.roofUv.empty())
				problems.emplace_back("The roof has no texture corners");
			return problems;
		}

		if (asset.floors.empty())
			problems.emplace_back("No floors: X-Plane draws nothing");
		for (size_t f = 0; f < asset.floors.size(); ++f)
		{
			const FacadeAsset::Floor &floor = asset.floors[f];
			const std::string label = "Floor " + std::to_string(f) + (floor.name.empty() ? std::string() : " (" + floor.name + ")");
			if (floor.segments.empty())
				problems.push_back(label + ": no segments");
			if (!floor.curvedSegments.empty() && floor.curvedSegments.size() != floor.segments.size())
				problems.push_back(label + ": needs one SEGMENT_CURVED per SEGMENT (or none)");
			if (floor.walls.empty())
				problems.push_back(label + ": no walls");
			for (const FacadeAsset::WallRule &wall : floor.walls)
			{
				const std::string name = label + ", wall " + (wall.name.empty() ? std::string("(unnamed)") : wall.name);
				if (wall.spellings.empty())
					problems.push_back(name + ": no spellings");
				for (const std::vector<int> &spelling : wall.spellings)
				{
					if (std::ranges::any_of(spelling, [&](const int s) { return s < 0 || static_cast<size_t>(s) >= floor.segments.size(); }))
					{
						problems.push_back(name + ": a spelling names a segment the floor doesn't have");
						break;
					}
				}
			}
			bool badMesh = false, badObject = false;
			for (const std::vector<FacadeAsset::Segment> *list : { &floor.segments, &floor.curvedSegments })
			{
				for (const FacadeAsset::Segment &segment : *list)
				{
					for (const FacadeAsset::Mesh &mesh : segment.meshes)
					{
						const size_t vertices = mesh.positions.size() / 3;
						badMesh |= mesh.indices.size() % 3 != 0 || std::ranges::any_of(mesh.indices, [&](const uint32_t i) { return i >= vertices; });
					}
					for (const FacadeAsset::Attachment &attachment : segment.attachments)
						badObject |= attachment.object < 0 || static_cast<size_t>(attachment.object) >= asset.objects.size();
				}
			}
			if (badMesh)
				problems.push_back(label + ": a segment mesh has bad triangles");
			if (badObject)
				problems.push_back(label + ": a segment attaches an object the facade doesn't list");
		}
		return problems;
	}

	bool ReadTextureSize(const std::filesystem::path &path, int &width, int &height)
	{
		width = height = 0;
		std::ifstream file(path, std::ios::binary);
		unsigned char header[32] = {};
		if (!file || !file.read(reinterpret_cast<char *>(header), sizeof(header)))
			return false;
		const auto bigEndian = [&](const size_t at) { return static_cast<int>((static_cast<uint32_t>(header[at]) << 24) | (header[at + 1] << 16) | (header[at + 2] << 8) | header[at + 3]); };
		const auto littleEndian = [&](const size_t at) { return static_cast<int>(header[at] | (header[at + 1] << 8) | (header[at + 2] << 16) | (static_cast<uint32_t>(header[at + 3]) << 24)); };

		static constexpr unsigned char PNG[] = { 0x89, 'P', 'N', 'G', 0x0D, 0x0A, 0x1A, 0x0A };
		if (std::equal(std::begin(PNG), std::end(PNG), header) && header[12] == 'I' && header[13] == 'H' && header[14] == 'D' && header[15] == 'R')
		{
			width = bigEndian(16);
			height = bigEndian(20);
		}
		else if (header[0] == 'D' && header[1] == 'D' && header[2] == 'S' && header[3] == ' ' && littleEndian(4) == 124)
		{
			height = littleEndian(12);
			width = littleEndian(16);
		}
		return width > 0 && height > 0;
	}

	std::string MakeFacadeRelativePath(const std::filesystem::path &fromFile, const std::filesystem::path &target)
	{
		const std::filesystem::path relative = target.lexically_normal().lexically_relative(fromFile.lexically_normal().parent_path());
		return (relative.empty() ? target : relative).generic_string();
	}

	std::string MakeFacadeFileName(const std::string &name)
	{
		std::string out;
		for (const char c : name)
		{
			const bool keep = (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') || c == '-' || c == '_';
			out.push_back(keep ? c : '_');
		}
		return out.empty() ? "facade" : out;
	}
}

// -------------------------------------------------------
