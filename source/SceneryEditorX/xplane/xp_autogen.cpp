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
 * xp_autogen.cpp
 * -------------------------------------------------------
 * Created: 22/10/2026
 * -------------------------------------------------------
 */
#include "xp_autogen.h"
#include "xp_assets.h"
#include "xp_text.h"
#include <algorithm>
#include <cmath>
#include <map>
#include <limits>
#include <numeric>
#include <unordered_map>

// Laying out a mile of autogen places tens of thousands of tiles: keep it optimized in MSVC debug builds too
#if defined(_MSC_VER) && !defined(__clang__) && !defined(NDEBUG)
#pragma optimize("gt", on)
#endif

// -------------------------------------------------------

namespace SceneryEditorX::XPlane
{
	namespace
	{
		constexpr double kPi = 3.14159265358979323846;

		bool Fail(std::string *error, const std::string &message)
		{
			if (error)
				*error = message;
			return false;
		}

		std::string PathFrom(const std::string_view line, const size_t token)
		{
			std::string path(RestOfLine(line, token));
			std::ranges::replace(path, '\\', '/');
			return path;
		}

		float Number(const std::vector<std::string_view> &tokens, const size_t index, const float fallback = 0.0f)
		{
			return index < tokens.size() ? ParseFloat(tokens[index], fallback) : fallback;
		}

		int Integer(const std::vector<std::string_view> &tokens, const size_t index, const int fallback = 0)
		{
			return index < tokens.size() ? ParseInt(tokens[index], fallback) : fallback;
		}

		std::vector<int> Integers(const std::vector<std::string_view> &tokens, const size_t first)
		{
			std::vector<int> values;
			for (size_t i = first; i < tokens.size(); ++i)
			{
				float value = 0.0f;
				if (!TryParseFloat(tokens[i], value))
					break;	// A trailing comment
				values.push_back(static_cast<int>(value));
			}
			return values;
		}

		// -------------------------------------------------------
		// Geometry

		/* @brief Left of travel seen from above (x east, z south). */
		Point2 Left(const Point2 &d) { return { d.z, -d.x }; }
		Point2 Right(const Point2 &d) { return { -d.z, d.x }; }

		Point2 Normalized(const Point2 &v)
		{
			const double length = std::hypot(v.x, v.z);
			return length > 1e-12 ? Point2{ v.x / length, v.z / length } : Point2{ 1.0, 0.0 };
		}

		double Dot(const Point2 &a, const Point2 &b) { return a.x * b.x + a.z * b.z; }

		/* @brief Where a tile stands: world = origin + u d + v n (n = left of d). */
		struct Frame
		{
			Point2 origin;
			Point2 d{ 1.0, 0.0 };
			Point2 n{ 0.0, -1.0 };

			[[nodiscard]] Point2 At(const double u, const double v) const { return { origin.x + d.x * u + n.x * v, origin.z + d.z * u + n.z * v }; }
			[[nodiscard]] Point2 Vector(const double u, const double v) const { return { d.x * u + n.x * v, d.z * u + n.z * v }; }
		};

		Frame MakeFrame(const Point2 &origin, const Point2 &direction)
		{
			Frame frame;
			frame.origin = origin;
			frame.d = Normalized(direction);
			frame.n = Left(frame.d);
			return frame;
		}

		/* @brief A tile texel relative to its anchor in the placement frame: meters, turned by the tile's rotation. */
		std::pair<double, double> TileLocal(const AutogenTile &tile, const double s, const double t)
		{
			const double x = (s - tile.GetAnchorS()) * tile.metersPerPixelS;
			const double y = (t - tile.GetAnchorT()) * tile.metersPerPixelT;
			switch (tile.rotation & 3)
			{
				case 1: return { -y, x };
				case 2: return { -x, -y };
				case 3: return { y, -x };
				default: return { x, y };
			}
		}

		/* @brief The texture's up direction in the placement frame. */
		std::pair<double, double> TileUp(const AutogenTile &tile)
		{
			switch (tile.rotation & 3)
			{
				case 1: return { -1.0, 0.0 };
				case 2: return { 0.0, -1.0 };
				case 3: return { 1.0, 0.0 };
				default: return { 0.0, 1.0 };
			}
		}

		/* @brief The tile's rectangle (or crop polygon) in s, t pairs. */
		std::vector<float> TileOutline(const AutogenTile &tile)
		{
			if (tile.crop.size() >= 6)
				return tile.crop;
			return { tile.left, tile.bottom, tile.right, tile.bottom, tile.right, tile.top, tile.left, tile.top };
		}

		struct Extent
		{
			double uMin = 0.0, uMax = 0.0, vMin = 0.0, vMax = 0.0;
			[[nodiscard]] double Width() const { return uMax - uMin; }
		};

		Extent TileExtent(const AutogenTile &tile)
		{
			Extent extent{ 1e30, -1e30, 1e30, -1e30 };
			const float corners[4][2] = { { tile.left, tile.bottom }, { tile.right, tile.bottom }, { tile.right, tile.top }, { tile.left, tile.top } };
			for (const auto &corner : corners)
			{
				const auto [u, v] = TileLocal(tile, corner[0], corner[1]);
				extent.uMin = std::min(extent.uMin, u);
				extent.uMax = std::max(extent.uMax, u);
				extent.vMin = std::min(extent.vMin, v);
				extent.vMax = std::max(extent.vMax, v);
			}
			return extent;
		}

		bool InRing(const std::vector<Point2> &ring, const Point2 &p)
		{
			bool inside = false;
			for (size_t i = 0, j = ring.size() - 1; i < ring.size(); j = i++)
			{
				if ((ring[i].z > p.z) != (ring[j].z > p.z) && p.x < (ring[j].x - ring[i].x) * (p.z - ring[i].z) / (ring[j].z - ring[i].z) + ring[i].x)
					inside = !inside;
			}
			return inside;
		}

		/* @brief Even-odd over the segments of polylines that together close the area (an AG string's contours). */
		bool InPolylines(const std::vector<std::vector<Point2>> &contours, const Point2 &p)
		{
			bool inside = false;
			for (const std::vector<Point2> &line : contours)
			{
				for (size_t i = 1; i < line.size(); ++i)
				{
					const Point2 &a = line[i - 1], &b = line[i];
					if ((a.z > p.z) != (b.z > p.z) && p.x < (b.x - a.x) * (p.z - a.z) / (b.z - a.z) + a.x)
						inside = !inside;
				}
			}
			return inside;
		}

		double Heading(const Point2 &direction)
		{
			return DirectionToHeading(direction);
		}

		/* @brief Deterministic pseudo-random numbers (the overlay builder's generator). */
		struct Random
		{
			uint32_t state = 1;
			explicit Random(const uint32_t seed) : state(seed * 2654435761u + 12345u) {}
			float Next()
			{
				state = state * 1664525u + 1013904223u;
				return static_cast<float>((state >> 8) & 0xFFFFFF) / 16777216.0f;
			}
			size_t Pick(const size_t count) { return count ? std::min(count - 1, static_cast<size_t>(Next() * static_cast<float>(count))) : 0; }
		};

		bool Appears(const int lo, const int hi, const int level, Random &random)
		{
			if (level <= 0 || level < lo)
				return false;
			if (level >= hi)
				return true;
			const double chance = (level - lo + 1.0) / std::max(1.0, hi - lo + 1.0);
			return random.Next() < chance;
		}

		/* @brief Where a scraper stands above its ground: past min in whole steps, at most max - min. */
		float ScraperRaise(const AutogenAnnotations::Object &object, const float height)
		{
			if (object.scraperStep <= 0.0f || height <= object.scraperMin)
				return 0.0f;
			const float steps = std::floor((height - object.scraperMin) / object.scraperStep);
			return std::clamp(steps * object.scraperStep, 0.0f, std::max(0.0f, object.scraperMax - object.scraperMin));
		}

		/* @brief Turns texture-space annotations into layout entries through a mapping of pixels to the ground. */
		template<typename ToGround>
		void EmitAnnotations(const AutogenAnnotations &annotations, const ToGround &toGround, const double upHeading, const Point2 &tileGround, const float height,
							 const AutogenLayoutOptions &options, Random &random, AutogenLayout &out)
		{
			for (const AutogenAnnotations::Object &object : annotations.objects)
			{
				if (!Appears(object.lo, object.hi, options.objectLevel, random))
					continue;
				AutogenLayout::Object placed;
				placed.object = object.object;
				placed.at = toGround(object.s, object.t);
				placed.heading = std::fmod(upHeading + object.heading + 720.0, 360.0);
				placed.graded = object.mode != AutogenAnnotations::Object::Mode::Draped;
				placed.ground = tileGround;
				placed.raise = object.mode == AutogenAnnotations::Object::Mode::Scraper ? ScraperRaise(object, height) : object.delta;
				out.objects.push_back(placed);
			}
			for (const AutogenAnnotations::Facade &facade : annotations.facades)
			{
				if (facade.points.size() < 4)
					continue;
				AutogenLayout::Facade placed;
				placed.facade = facade.facade;
				placed.height = facade.height;
				for (size_t i = 0; i + 1 < facade.points.size(); i += 2)
					placed.points.push_back(toGround(facade.points[i], facade.points[i + 1]));
				placed.walls = facade.walls;
				out.facades.push_back(std::move(placed));
			}
			for (const AutogenAnnotations::Tree &tree : annotations.trees)
				out.trees.push_back({ toGround(tree.s, tree.t), std::fmod(upHeading + tree.heading + 720.0, 360.0), tree.width, tree.layer });
			for (const AutogenAnnotations::TreeLine &line : annotations.treeLines)
				out.treeLines.push_back({ toGround(line.s1, line.t1), toGround(line.s2, line.t2), line.layer });
		}

		/* @brief A ground polygon in s, t pairs mapped to the ground, counter-clockwise, with its UVs. */
		template<typename ToGround>
		void EmitGround(const AutogenAsset &asset, const std::vector<float> &outline, const ToGround &toGround, AutogenLayout &out)
		{
			if (asset.hideTiles || asset.texture.empty() || outline.size() < 6)
				return;
			AutogenLayout::Ground ground;
			for (size_t i = 0; i + 1 < outline.size(); i += 2)
			{
				ground.ring.push_back(toGround(outline[i], outline[i + 1]));
				ground.uvs.emplace_back(outline[i] / std::max(1.0f, asset.textureWidth), outline[i + 1] / std::max(1.0f, asset.textureHeight));
			}
			if (SignedArea(ground.ring) < 0.0)
			{
				std::ranges::reverse(ground.ring);
				std::ranges::reverse(ground.uvs);
			}
			out.ground.push_back(std::move(ground));
		}

		/* @brief A tile placed in a frame: its ground, annotations and footprint (for collisions). */
		struct PlacedTile
		{
			std::vector<Point2> footprint;
			std::vector<Point2> stops;
		};

		PlacedTile Footprint(const AutogenTile &tile, const Frame &frame)
		{
			PlacedTile placed;
			const std::vector<float> outline = TileOutline(tile);
			for (size_t i = 0; i + 1 < outline.size(); i += 2)
			{
				const auto [u, v] = TileLocal(tile, outline[i], outline[i + 1]);
				placed.footprint.push_back(frame.At(u, v));
			}
			for (size_t i = 0; i + 1 < tile.stops.size(); i += 2)
			{
				const auto [u, v] = TileLocal(tile, tile.stops[i], tile.stops[i + 1]);
				placed.stops.push_back(frame.At(u, v));
			}
			return placed;
		}

		void EmitTile(const AutogenAsset &asset, const AutogenTile &tile, const Frame &frame, const float height, const Point2 *sharedGround,
					  const AutogenLayoutOptions &options, Random &random, AutogenLayout &out)
		{
			const auto toGround = [&](const float s, const float t)
			{
				const auto [u, v] = TileLocal(tile, s, t);
				return frame.At(u, v);
			};
			EmitGround(asset, TileOutline(tile), toGround, out);
			const auto [upU, upV] = TileUp(tile);
			const double upHeading = Heading(frame.Vector(upU, upV));
			const Point2 ground = sharedGround ? *sharedGround
								: tile.hasGround ? toGround(tile.ground[0], tile.ground[1]) : toGround(0.5f * (tile.left + tile.right), 0.5f * (tile.bottom + tile.top));
			EmitAnnotations(tile.annotations, toGround, upHeading, ground, height, options, random, out);
			++out.tiles;
		}

		/* @brief The ground point a tile would grade to in a frame (for SHARE_Y). */
		Point2 TileGround(const AutogenTile &tile, const Frame &frame)
		{
			const float s = tile.hasGround ? tile.ground[0] : 0.5f * (tile.left + tile.right);
			const float t = tile.hasGround ? tile.ground[1] : 0.5f * (tile.bottom + tile.top);
			const auto [u, v] = TileLocal(tile, s, t);
			return frame.At(u, v);
		}

		bool InBox(const Bounds2 &box, const Point2 &p)
		{
			return box.valid && p.x >= box.minX && p.x <= box.maxX && p.z >= box.minZ && p.z <= box.maxZ;
		}

		/* @brief Collision bookkeeping of an AG string: what's been placed so far (footprints with their boxes). */
		struct Occupancy
		{
			const std::vector<std::vector<Point2>> *contours = nullptr;
			std::vector<std::vector<Point2>> footprints;
			std::vector<Bounds2> boxes;
			std::vector<Point2> stops;

			/* @brief X-Plane's stop point rule (without the sliding): inside the polygon, clear of what's placed. */
			[[nodiscard]] bool Allows(const PlacedTile &tile) const
			{
				for (const Point2 &stop : tile.stops)
				{
					if (!InPolylines(*contours, stop))
						return false;
					for (size_t k = 0; k < footprints.size(); ++k)
					{
						if (InBox(boxes[k], stop) && footprints[k].size() >= 3 && InRing(footprints[k], stop))
							return false;
					}
				}
				if (tile.footprint.size() >= 3)
				{
					const Bounds2 box = GetBounds(tile.footprint);
					for (const Point2 &stop : stops)
					{
						if (InBox(box, stop) && InRing(tile.footprint, stop))
							return false;
					}
				}
				return true;
			}

			void Add(PlacedTile &&tile)
			{
				boxes.push_back(GetBounds(tile.footprint));
				footprints.push_back(std::move(tile.footprint));
				stops.insert(stops.end(), tile.stops.begin(), tile.stops.end());
			}

			/* @brief A point to roll back to when an atomic group doesn't fit. */
			struct Mark
			{
				size_t footprints = 0;
				size_t stops = 0;
			};

			[[nodiscard]] Mark Save() const { return { footprints.size(), stops.size() }; }

			void Restore(const Mark &mark)
			{
				footprints.resize(mark.footprints);
				boxes.resize(mark.footprints);
				stops.resize(mark.stops);
			}
		};

		// -------------------------------------------------------
		// AG string sides

		/* @brief A polyline with arc length lookups. */
		struct Path
		{
			std::vector<Point2> points;
			std::vector<double> lengths;	// Cumulative

			explicit Path(std::vector<Point2> p) : points(std::move(p))
			{
				lengths.assign(points.size(), 0.0);
				for (size_t i = 1; i < points.size(); ++i)
					lengths[i] = lengths[i - 1] + std::hypot(points[i].x - points[i - 1].x, points[i].z - points[i - 1].z);
			}

			[[nodiscard]] double Length() const { return lengths.empty() ? 0.0 : lengths.back(); }

			[[nodiscard]] Point2 At(const double distance) const
			{
				if (points.size() < 2)
					return points.empty() ? Point2{} : points.front();
				const double clamped = std::clamp(distance, 0.0, Length());
				const auto it = std::upper_bound(lengths.begin(), lengths.end(), clamped);
				const size_t i = std::clamp<size_t>(static_cast<size_t>(it - lengths.begin()), 1, points.size() - 1);
				const double span = lengths[i] - lengths[i - 1];
				const double f = span > 1e-12 ? (clamped - lengths[i - 1]) / span : 0.0;
				return { points[i - 1].x + (points[i].x - points[i - 1].x) * f, points[i - 1].z + (points[i].z - points[i - 1].z) * f };
			}
		};

		/* @brief A tile of width w along a side from arc position a: its front on the chord, its left edge at a. */
		Frame SideFrame(const Path &path, const double a, const double width, const Extent &extent)
		{
			const Point2 start = path.At(a);
			Point2 end = path.At(a + width);
			if (std::hypot(end.x - start.x, end.z - start.z) < 1e-6)
			{
				// Past the end: keep the last segment's direction
				const Point2 &p = path.points[path.points.size() - 2], &q = path.points.back();
				end = { start.x + (q.x - p.x), start.z + (q.z - p.z) };
			}
			Frame frame = MakeFrame(start, { end.x - start.x, end.z - start.z });
			frame.origin = { start.x - frame.d.x * extent.uMin, start.z - frame.d.z * extent.uMin };
			return frame;
		}

		struct Corner
		{
			size_t line = 0;						// Index into the contours' cleaned point lists
			size_t index = 0;						// Its vertex there
			double turn = 0.0;
			double lengthIn = 0.0, lengthOut = 0.0;	// The single segments either side
			double trimIn = 0.0, trimOut = 0.0;		// What its tiles cover of the sides either side
			int placedTile = -1;					// Middle tile placed (for group rules)
		};

		struct Side
		{
			std::vector<Point2> points;
			int startCorner = -1;					// Corners at its ends (-1: an open end)
			int endCorner = -1;
		};

		/* @brief Weighted pick among sets passing their filters; the first set when none does. */
		const AutogenSpellingSet *PickSet(const AutogenAsset &asset, const int param, const float height, const float width, const float depth, Random &random)
		{
			if (asset.spellingSets.empty())
				return nullptr;
			std::vector<const AutogenSpellingSet *> candidates;
			float total = 0.0f;
			for (const AutogenSpellingSet &set : asset.spellingSets)
			{
				if (set.param >= 0 && param >= 0 && set.param != param)
					continue;
				if (height < set.heightMin || height > set.heightMax || width < set.widthMin || width > set.widthMax || depth < set.depthMin || depth > set.depthMax)
					continue;
				candidates.push_back(&set);
				total += std::max(0.0f, set.frequency);
			}
			if (candidates.empty())
				return &asset.spellingSets.front();
			float roll = random.Next() * total;
			for (const AutogenSpellingSet *set : candidates)
			{
				roll -= std::max(0.0f, set->frequency);
				if (roll <= 0.0f)
					return set;
			}
			return candidates.back();
		}

		// -------------------------------------------------------
		// AG block grids

		/* @brief Spelling choice along one axis: positions and widths of the cells, by spelling index. */
		struct Assembly
		{
			std::vector<int> cells;			// Column / row indices in spelling order
			std::vector<double> start;		// Meters from the grid origin
			std::vector<double> width;		// Meters shown (a slop cell can be compressed)
			double inset = 0.0;				// Meters from the assembled start to EDGE_BEST
		};

		Assembly Assemble(const std::vector<std::vector<int>> &spellings, const std::vector<float> &cuts, const std::vector<float> &slop, const float mpp,
						  const float bestLow, const float bestHigh, const bool hasEdges, const double target, Random &random)
		{
			Assembly best;
			double bestError = 1e30;
			std::vector<size_t> ties;
			const size_t cellCount = cuts.size() >= 2 ? cuts.size() - 1 : 0;
			const auto cellWidth = [&](const int c) { return (cuts[static_cast<size_t>(c) + 1] - cuts[static_cast<size_t>(c)]) * static_cast<double>(mpp); };

			// The error of each spelling, then a random one among the best
			std::vector<double> errors(spellings.size(), 1e30);
			for (size_t k = 0; k < spellings.size(); ++k)
			{
				const std::vector<int> &cells = spellings[k];
				if (cells.empty() || std::ranges::any_of(cells, [cellCount](const int c) { return c < 0 || static_cast<size_t>(c) >= cellCount; }))
					continue;
				double fixed = 0.0, flexible = 0.0;
				for (const int c : cells)
				{
					const double overlap = slop[static_cast<size_t>(c)];
					if (overlap >= 0.0)
						flexible += std::max(0.0, cellWidth(c) - overlap * mpp);
					else
						fixed += cellWidth(c);
				}
				const double insets = hasEdges ? (bestLow - cuts[static_cast<size_t>(cells.front())] + cuts[static_cast<size_t>(cells.back()) + 1] - bestHigh) * mpp : 0.0;
				const double wanted = target + insets;
				errors[k] = wanted < fixed ? fixed - wanted : (wanted > fixed + flexible ? wanted - fixed - flexible : 0.0);
				bestError = std::min(bestError, errors[k]);
			}
			for (size_t k = 0; k < spellings.size(); ++k)
			{
				if (errors[k] <= bestError + 0.5)
					ties.push_back(k);
			}
			if (ties.empty())
				return best;

			const std::vector<int> &cells = spellings[ties[random.Pick(ties.size())]];
			double fixed = 0.0, flexible = 0.0;
			for (const int c : cells)
			{
				const double overlap = slop[static_cast<size_t>(c)];
				if (overlap >= 0.0)
					flexible += std::max(0.0, cellWidth(c) - overlap * mpp);
				else
					fixed += cellWidth(c);
			}
			best.inset = hasEdges ? (bestLow - cuts[static_cast<size_t>(cells.front())]) * mpp : 0.0;
			const double insets = hasEdges ? best.inset + (cuts[static_cast<size_t>(cells.back()) + 1] - bestHigh) * mpp : 0.0;
			const double share = flexible > 1e-9 ? std::clamp((target + insets - fixed) / flexible, 0.0, 1.0) : 0.0;
			double at = -best.inset;
			for (const int c : cells)
			{
				const double overlap = slop[static_cast<size_t>(c)];
				const double shown = overlap >= 0.0 ? std::max(0.0, cellWidth(c) - overlap * mpp) * share : cellWidth(c);
				best.cells.push_back(c);
				best.start.push_back(at);
				best.width.push_back(shown);
				at += shown;
			}
			return best;
		}

		/* @brief The cell of a grid coordinate (pixels), -1 outside. */
		int CellOf(const std::vector<float> &cuts, const float value)
		{
			for (size_t c = 0; c + 1 < cuts.size(); ++c)
			{
				if (value >= cuts[c] && (value < cuts[c + 1] || (c + 2 == cuts.size() && value <= cuts[c + 1])))
					return static_cast<int>(c);
			}
			return -1;
		}

		/* @brief Sutherland-Hodgman of a polygon against a convex counter-clockwise one, with UVs carried along. */
		void ClipToConvex(std::vector<Point2> &ring, std::vector<std::pair<float, float>> &uvs, const std::vector<Point2> &clip)
		{
			for (size_t e = 0; e < clip.size() && ring.size() >= 3; ++e)
			{
				const Point2 &a = clip[e], &b = clip[(e + 1) % clip.size()];
				// Inside is left of a -> b (counter-clockwise from above)
				const auto side = [&](const Point2 &p) { return Dot({ p.x - a.x, p.z - a.z }, Left(Normalized({ b.x - a.x, b.z - a.z }))); };
				std::vector<Point2> outRing;
				std::vector<std::pair<float, float>> outUvs;
				for (size_t i = 0; i < ring.size(); ++i)
				{
					const size_t j = (i + 1) % ring.size();
					const double si = side(ring[i]), sj = side(ring[j]);
					if (si >= 0.0)
					{
						outRing.push_back(ring[i]);
						outUvs.push_back(uvs[i]);
					}
					if ((si >= 0.0) != (sj >= 0.0))
					{
						const double f = si / (si - sj);
						outRing.push_back({ ring[i].x + (ring[j].x - ring[i].x) * f, ring[i].z + (ring[j].z - ring[i].z) * f });
						outUvs.emplace_back(static_cast<float>(uvs[i].first + (uvs[j].first - uvs[i].first) * f), static_cast<float>(uvs[i].second + (uvs[j].second - uvs[i].second) * f));
					}
				}
				ring.swap(outRing);
				uvs.swap(outUvs);
			}
		}
	}

	// -------------------------------------------------------

	const AutogenTile *AutogenAsset::FindTile(const int id) const
	{
		for (const AutogenTile &tile : tiles)
		{
			if (tile.id == id)
				return &tile;
		}
		return nullptr;
	}

	bool AutogenAsset::IsInGroup(const int group, const int tileId) const
	{
		for (const auto &[id, members] : groups)
		{
			if (id == group)
				return std::ranges::find(members, tileId) != members.end();
		}
		return false;
	}

	bool ParseAutogenAsset(const std::string_view text, AutogenAsset &out, std::string *error)
	{
		out = AutogenAsset();
		AssetHeader header;
		const size_t start = ParseAssetHeader(text, header);
		if (start == std::string_view::npos)
			return Fail(error, "Not an X-Plane autogen file (bad header)");
		if (header.type == "AG_POINT")
			out.type = AutogenType::Point;
		else if (header.type == "AG_STRING")
			out.type = AutogenType::String;
		else if (header.type == "AG_BLOCK")
			out.type = AutogenType::Block;
		else
			return Fail(error, "Expected AG_POINT, AG_STRING or AG_BLOCK, found " + header.type);

		// Pixels to meters: TEXTURE_SCALE pixels span TEXTURE_WIDTH (and TEXTURE_HEIGHT, else square pixels) meters
		float scaleW = 1.0f, scaleH = 1.0f, widthM = 1.0f, heightM = 1.0f;
		bool explicitHeight = false;
		const auto mppS = [&]() { return widthM / std::max(1e-6f, scaleW); };
		const auto mppT = [&]() { return explicitHeight ? heightM / std::max(1e-6f, scaleH) : mppS(); };

		AutogenTile *tile = nullptr;
		AutogenGrid *grid = nullptr;
		bool inCuts = false;
		int pendingId = -1, nextId = 0;
		AutogenSpellingSet *set = nullptr;
		int cornerPriority = 0, spellingPriority = 0;
		AutogenAnnotations::Object *lastObject = nullptr;
		std::map<std::string, std::vector<int>> namedS, namedT;
		float overlapH = 0.0f, overlapV = 0.0f;

		const auto currentSet = [&]() -> AutogenSpellingSet &
		{
			if (!set)
			{
				out.spellingSets.emplace_back();
				set = &out.spellingSets.back();
			}
			return *set;
		};
		const auto annotations = [&]() -> AutogenAnnotations *
		{
			if (out.type == AutogenType::Block)
				return grid && !inCuts ? &grid->annotations : nullptr;
			return tile ? &tile->annotations : nullptr;
		};
		const auto refreshScale = [&]()
		{
			if (grid && inCuts)
			{
				grid->metersPerPixelS = mppS();
				grid->metersPerPixelT = mppT();
			}
		};
		const auto finishCuts = [&]()
		{
			if (!grid)
				return;
			const size_t columns = grid->cutsH.size() >= 2 ? grid->cutsH.size() - 1 : 0;
			const size_t rows = grid->cutsV.size() >= 2 ? grid->cutsV.size() - 1 : 0;
			grid->slopH.resize(columns, -1.0f);
			grid->slopV.resize(rows, -1.0f);
			grid->antislopH.resize(columns, 0.0f);
			grid->antislopV.resize(rows, 0.0f);
			inCuts = false;
		};
		const auto newGrid = [&]()
		{
			finishCuts();
			out.grids.emplace_back();
			grid = &out.grids.back();
			inCuts = true;
			grid->metersPerPixelS = mppS();
			grid->metersPerPixelT = mppT();
		};
		const auto parseCorner = [&](const std::vector<std::string_view> &tokens, const bool lengths, const bool atomic, const bool pair)
		{
			AutogenCorner corner;
			corner.atomic = atomic;
			corner.pair = pair;
			corner.priority = cornerPriority;
			size_t next = 1;
			if (pair)
			{
				// CORNER_PAIR angle1-min angle1-max length-min length-max angle2-min angle2-max group ids
				corner.angleMin = Number(tokens, 1);
				corner.angleMax = Number(tokens, 2);
				corner.pairLenMin = Number(tokens, 3);
				corner.pairLenMax = Number(tokens, 4);
				corner.angle2Min = Number(tokens, 5);
				corner.angle2Max = Number(tokens, 6);
				corner.group = Integer(tokens, 7);
				next = 8;
			}
			else if (lengths)
			{
				// CORNER_LEN len1-min len1-max angle-min angle-max len2-min len2-max group ids
				corner.len1Min = Number(tokens, 1);
				corner.len1Max = Number(tokens, 2);
				corner.angleMin = Number(tokens, 3);
				corner.angleMax = Number(tokens, 4);
				corner.len2Min = Number(tokens, 5);
				corner.len2Max = Number(tokens, 6);
				corner.group = Integer(tokens, 7);
				next = 8;
			}
			else
			{
				corner.angleMin = Number(tokens, 1);
				corner.angleMax = Number(tokens, 2);
				next = 3;
			}
			if (corner.angleMin > corner.angleMax)
				std::swap(corner.angleMin, corner.angleMax);
			corner.tiles = Integers(tokens, next);
			if (corner.tiles.size() == 1 || corner.tiles.size() == 3)
				currentSet().corners.push_back(std::move(corner));
		};

		LineReader reader(text.substr(std::min(start, text.size())));
		std::string_view line;
		std::vector<std::string_view> tokens;
		while (reader.Next(line))
		{
			Tokenize(line, tokens);
			if (tokens.empty() || tokens[0].front() == '#')
				continue;
			const std::string_view key = tokens[0];

			// Shared properties
			if (key == "TEXTURE" && tokens.size() >= 2)
				out.texture = PathFrom(line, 1);
			else if (key == "TEXTURE_SCALE" && tokens.size() >= 3)
			{
				scaleW = Number(tokens, 1, 1.0f);
				scaleH = Number(tokens, 2, 1.0f);
				out.textureWidth = scaleW;
				out.textureHeight = scaleH;
				refreshScale();
			}
			else if (key == "TEXTURE_WIDTH" && tokens.size() >= 2)
			{
				widthM = Number(tokens, 1, 1.0f);
				refreshScale();
			}
			else if (key == "TEXTURE_HEIGHT" && tokens.size() >= 2)
			{
				heightM = Number(tokens, 1, 1.0f);
				explicitHeight = true;
				refreshScale();
			}
			else if (key == "HIDE_TILES" || key == "HIDE_TILE")
				out.hideTiles = true;
			else if (key == "LAYER_GROUP" && tokens.size() >= 3)
			{
				// The spec writes the offset first; the shipped art writes the name first
				float offset = 0.0f;
				if (TryParseFloat(tokens[1], offset))
				{
					out.layerOffset = static_cast<int>(offset);
					out.layerGroup = std::string(tokens[2]);
				}
				else
				{
					out.layerGroup = std::string(tokens[1]);
					out.layerOffset = Integer(tokens, 2);
				}
			}
			else if (key == "VEGETATION" && tokens.size() >= 2)
				out.vegetation = PathFrom(line, 1);
			else if (key == "FILL_LAYER" && tokens.size() >= 2)
				out.fillLayer = Integer(tokens, 1, -1);
			else if ((key == "CREASE_ANGLE" || key == "CREASING_ANGLE") && tokens.size() >= 3)
			{
				out.creaseLeft = -std::abs(Number(tokens, 1, 30.0f));
				out.creaseRight = std::abs(Number(tokens, 2, 30.0f));
			}
			else if (key == "SHARE_Y")
				out.shareY = true;
			else if (key == "CROP" && tokens.size() >= 2)
				out.crop = Integer(tokens, 1) != 0;
			else if (key == "FLIP_180")
				out.flip180 = true;
			else if (key == "OBJECT" && tokens.size() >= 2)
				out.objects.push_back(PathFrom(line, 1));
			else if (key == "FACADE" && tokens.size() >= 2)
				out.facades.push_back(PathFrom(line, 1));

			// Tiles (points, strings)
			else if (key == "TILE_ID" && tokens.size() >= 2)
				pendingId = Integer(tokens, 1);
			else if (key == "TILE" && tokens.size() >= 5 && out.type != AutogenType::Block)
			{
				AutogenTile &added = out.tiles.emplace_back();
				added.id = pendingId >= 0 ? pendingId : nextId;
				nextId = std::max(nextId, added.id) + 1;
				pendingId = -1;
				added.left = Number(tokens, 1);
				added.bottom = Number(tokens, 2);
				added.right = Number(tokens, 3);
				added.top = Number(tokens, 4);
				added.metersPerPixelS = mppS();
				added.metersPerPixelT = mppT();
				tile = &added;
				lastObject = nullptr;
			}
			else if (key == "ROTATION" && tile && tokens.size() >= 2)
				tile->rotation = ((Integer(tokens, 1) % 4) + 4) % 4;
			else if (key == "ANCHOR_PT" && tile && tokens.size() >= 3)
			{
				tile->hasAnchor = true;
				tile->anchor[0] = Number(tokens, 1);
				tile->anchor[1] = Number(tokens, 2);
			}
			else if (key == "GROUND_PT" && tile && tokens.size() >= 3)
			{
				tile->hasGround = true;
				tile->ground[0] = Number(tokens, 1);
				tile->ground[1] = Number(tokens, 2);
			}
			else if (key == "CROP_POLY" && tile && tokens.size() >= 7)
			{
				tile->crop.clear();
				for (size_t i = 1; i + 1 < tokens.size(); i += 2)
				{
					float s = 0.0f, t = 0.0f;
					if (!TryParseFloat(tokens[i], s) || !TryParseFloat(tokens[i + 1], t))
						break;
					tile->crop.push_back(s);
					tile->crop.push_back(t);
				}
			}
			else if (key == "STOP_PT" && tile && tokens.size() >= 3)
			{
				tile->stops.push_back(Number(tokens, 1));
				tile->stops.push_back(Number(tokens, 2));
			}
			else if (key == "PIVOT_LEAD_PT" && tile && tokens.size() >= 3)
			{
				tile->hasLeadPivot = true;
				tile->leadPivot[0] = Number(tokens, 1);
				tile->leadPivot[1] = Number(tokens, 2);
			}
			else if (key == "PIVOT_TRAIL_PT" && tile && tokens.size() >= 3)
			{
				tile->hasTrailPivot = true;
				tile->trailPivot[0] = Number(tokens, 1);
				tile->trailPivot[1] = Number(tokens, 2);
			}

			// Annotations
			else if ((key == "OBJ_DRAPED" || key == "OBJ_GRADED" || key == "OBJ_DELTA" || key == "OBJ_SCRAPER") && tokens.size() >= 5)
			{
				AutogenAnnotations *target = annotations();
				if (!target)
					continue;
				AutogenAnnotations::Object object;
				object.s = Number(tokens, 1);
				object.t = Number(tokens, 2);
				object.heading = Number(tokens, 3);
				size_t next = 4;
				if (key == "OBJ_DELTA")
				{
					object.mode = AutogenAnnotations::Object::Mode::Graded;
					object.delta = Number(tokens, 4);
					next = 5;
				}
				else if (key == "OBJ_GRADED")
					object.mode = AutogenAnnotations::Object::Mode::Graded;
				object.object = Integer(tokens, next, -1);
				++next;
				if (key == "OBJ_SCRAPER")
				{
					object.mode = AutogenAnnotations::Object::Mode::Scraper;
					object.scraperMin = Number(tokens, next);
					object.scraperMax = Number(tokens, next + 1);
					object.scraperStep = Number(tokens, next + 2);
					next += 3;
				}
				if (next + 1 < tokens.size())
				{
					object.lo = Integer(tokens, next, 1);
					object.hi = Integer(tokens, next + 1, 6);
				}
				if (object.object >= 0)
				{
					target->objects.push_back(object);
					lastObject = &target->objects.back();
				}
			}
			else if ((key == "FAC" || key == "FAC_WALLS") && tokens.size() >= 5)
			{
				AutogenAnnotations *target = annotations();
				if (!target)
					continue;
				AutogenAnnotations::Facade facade;
				facade.facade = Integer(tokens, 1, -1);
				facade.height = Number(tokens, 2);
				const size_t stride = key == "FAC_WALLS" ? 3 : 2;
				for (size_t i = 3; i + stride - 1 < tokens.size(); i += stride)
				{
					float s = 0.0f, t = 0.0f;
					if (!TryParseFloat(tokens[i], s) || !TryParseFloat(tokens[i + 1], t))
						break;
					facade.points.push_back(s);
					facade.points.push_back(t);
					if (stride == 3)
						facade.walls.push_back(Integer(tokens, i + 2));
				}
				if (facade.facade >= 0 && facade.points.size() >= 4)
					target->facades.push_back(std::move(facade));
			}
			else if (key == "TREE" && tokens.size() >= 6)
			{
				if (AutogenAnnotations *target = annotations())
					target->trees.push_back({ Number(tokens, 1), Number(tokens, 2), Number(tokens, 3), Number(tokens, 4), Integer(tokens, 5) });
			}
			else if (key == "TREE_LINE" && tokens.size() >= 6)
			{
				if (AutogenAnnotations *target = annotations())
					target->treeLines.push_back({ Number(tokens, 1), Number(tokens, 2), Number(tokens, 3), Number(tokens, 4), Integer(tokens, 5) });
			}

			// String selection rules
			else if (key == "GROUP" && tokens.size() >= 3)
				out.groups.emplace_back(Integer(tokens, 1), Integers(tokens, 2));
			else if (key == "CORNER" && tokens.size() >= 4)
				parseCorner(tokens, false, false, false);
			else if (key == "CORNER_LEN" && tokens.size() >= 9)
				parseCorner(tokens, true, false, false);
			else if (key == "ATOMIC_SPELLING_CORNER" && tokens.size() >= 9)
				parseCorner(tokens, true, true, false);
			else if (key == "CORNER_PAIR" && tokens.size() >= 9)
				parseCorner(tokens, false, false, true);
			else if (key == "CORNER_ORDER" && tokens.size() >= 4 && set && !set->corners.empty())
			{
				for (int i = 0; i < 3; ++i)
					set->corners.back().order[i] = std::clamp(Integer(tokens, static_cast<size_t>(i) + 1, i), 0, 2);
			}
			else if (key == "PRIORITY_MARKER_CORNER")
				++cornerPriority;
			else if (key == "PRIORITY_MARKER")
				++spellingPriority;
			else if (key == "SPELLING" && tokens.size() >= 2)
			{
				AutogenSpelling spelling;
				spelling.tiles = Integers(tokens, 1);
				spelling.priority = spellingPriority;
				if (!spelling.tiles.empty())
					currentSet().spellings.push_back(std::move(spelling));
			}
			else if (key == "ATOMIC_SPELLING" && tokens.size() >= 5)
			{
				AutogenSpelling spelling;
				spelling.atomic = true;
				spelling.lenMin = Number(tokens, 1);
				spelling.lenMax = Number(tokens, 2, 1e9f);
				spelling.group = Integer(tokens, 3);
				spelling.tiles = Integers(tokens, 4);
				spelling.priority = spellingPriority;
				if (!spelling.tiles.empty())
					currentSet().spellings.push_back(std::move(spelling));
			}
			else if (key == "CAPS" && tokens.size() >= 2)
			{
				const std::vector<int> caps = Integers(tokens, 1);
				currentSet().caps.insert(currentSet().caps.end(), caps.begin(), caps.end());
			}

			// Spelling sets (blocks, and strings that have them)
			else if (key == "SPELLING_SET")
			{
				AutogenSpellingSet &added = out.spellingSets.emplace_back();
				added.param = tokens.size() >= 2 ? Integer(tokens, 1, -1) : -1;
				set = &added;
				cornerPriority = 0;
				spellingPriority = 0;
			}
			else if (key == "HEIGHT_RANGE" && tokens.size() >= 3)
			{
				currentSet().heightMin = Number(tokens, 1);
				currentSet().heightMax = Number(tokens, 2, 1e9f);
			}
			else if (key == "WIDTH_RANGE" && tokens.size() >= 3)
			{
				currentSet().widthMin = Number(tokens, 1);
				currentSet().widthMax = Number(tokens, 2, 1e9f);
			}
			else if (key == "DEPTH_RANGE" && tokens.size() >= 3)
			{
				currentSet().depthMin = Number(tokens, 1);
				currentSet().depthMax = Number(tokens, 2, 1e9f);
			}
			else if (key == "FREQUENCY" && tokens.size() >= 2)
				currentSet().frequency = Number(tokens, 1, 1.0f);
			else if (key == "USE_GRID" && tokens.size() >= 2)
				currentSet().grid = Integer(tokens, 1);
			else if (key == "SPELLING_S" && tokens.size() >= 2)
				currentSet().spellingsS.push_back(Integers(tokens, 1));
			else if (key == "SPELLING_T" && tokens.size() >= 2)
				currentSet().spellingsT.push_back(Integers(tokens, 1));
			else if ((key == "DEFINE_SPELLING_S" || key == "DEFINE_SPELLING_T") && tokens.size() >= 3)
				(key == "DEFINE_SPELLING_S" ? namedS : namedT)[std::string(tokens[1])] = Integers(tokens, 2);
			else if ((key == "USE_SPELLING_S" || key == "USE_SPELLING_T" || key == "USE_SPELLING_t") && tokens.size() >= 2)
			{
				const bool columns = key == "USE_SPELLING_S";
				const auto &named = columns ? namedS : namedT;
				if (const auto it = named.find(std::string(tokens[1])); it != named.end())
					(columns ? currentSet().spellingsS : currentSet().spellingsT).push_back(it->second);
			}

			// Block grids
			else if (key == "SUB_GRID")
				newGrid();
			else if ((key == "CUT_H" || key == "CUT_V") && tokens.size() >= 2)
			{
				if (!grid || !inCuts)
					newGrid();
				(key == "CUT_H" ? grid->cutsH : grid->cutsV).push_back(Number(tokens, 1));
			}
			else if (key == "OVERLAP_H" && tokens.size() >= 2)
				overlapH = Number(tokens, 1);
			else if (key == "OVERLAP_V" && tokens.size() >= 2)
				overlapV = Number(tokens, 1);
			else if ((key == "SLOP_H" || key == "SLOP_V") && grid && inCuts)
			{
				const bool columns = key == "SLOP_H";
				std::vector<float> &cuts = columns ? grid->cutsH : grid->cutsV;
				std::vector<float> &slop = columns ? grid->slopH : grid->slopV;
				if (!cuts.empty())
				{
					const size_t cell = cuts.size() - 1;	// The tile whose first line was just cut
					if (slop.size() <= cell)
						slop.resize(cell + 1, -1.0f);
					slop[cell] = tokens.size() >= 2 ? Number(tokens, 1) : (columns ? overlapH : overlapV);
				}
			}
			else if ((key == "ANTISLOP_H" || key == "ANTISLOP_V") && grid && tokens.size() >= 2)
			{
				const bool columns = key == "ANTISLOP_H";
				std::vector<float> &cuts = columns ? grid->cutsH : grid->cutsV;
				std::vector<float> &antislop = columns ? grid->antislopH : grid->antislopV;
				if (!cuts.empty())
				{
					if (antislop.size() < cuts.size())
						antislop.resize(cuts.size(), 0.0f);
					antislop[cuts.size() - 1] = Number(tokens, 1);
				}
			}
			else if (key == "END_CUTS")
				finishCuts();
			else if ((key == "EDGE_MIN" || key == "EDGE_BEST" || key == "EDGE_MAX") && grid && tokens.size() >= 5)
			{
				float *edge = key == "EDGE_MIN" ? grid->edgeMin : key == "EDGE_BEST" ? grid->edgeBest : grid->edgeMax;
				for (int i = 0; i < 4; ++i)
					edge[i] = Number(tokens, static_cast<size_t>(i) + 1);
				grid->hasEdges = true;
			}
			else if (key == "GROUND_PT_FORMULA" && tokens.size() >= 5 && lastObject)
			{
				lastObject->hasGroundFormula = true;
				for (int i = 0; i < 4; ++i)
					lastObject->groundFormula[i] = Number(tokens, static_cast<size_t>(i) + 1);
			}
		}
		finishCuts();

		// Blocks without EDGE_* lines use the whole grid
		for (AutogenGrid &g : out.grids)
		{
			if (!g.hasEdges && g.cutsH.size() >= 2 && g.cutsV.size() >= 2)
			{
				g.edgeBest[0] = g.edgeMin[0] = g.edgeMax[0] = g.cutsH.front();
				g.edgeBest[1] = g.edgeMin[1] = g.edgeMax[1] = g.cutsV.front();
				g.edgeBest[2] = g.edgeMin[2] = g.edgeMax[2] = g.cutsH.back();
				g.edgeBest[3] = g.edgeMin[3] = g.edgeMax[3] = g.cutsV.back();
			}
		}

		switch (out.type)
		{
			case AutogenType::Point:
				if (out.tiles.empty())
					return Fail(error, "The autogen point has no TILE");
				break;
			case AutogenType::String:
				if (out.tiles.empty() && out.fillLayer < 0)
					return Fail(error, "The autogen string has neither tiles nor a FILL_LAYER");
				break;
			case AutogenType::Block:
				if (out.grids.empty() || out.grids.front().cutsH.size() < 2 || out.grids.front().cutsV.size() < 2)
					return Fail(error, "The autogen block has no grid (two CUT_H and two CUT_V at least)");
				if (out.spellingSets.empty())
					return Fail(error, "The autogen block has no SPELLING_SET");
				break;
		}
		return true;
	}

	bool LoadAutogenAsset(const std::filesystem::path &path, AutogenAsset &out, std::string *error)
	{
		std::string text;
		if (!ReadTextFile(path, text))
			return Fail(error, "Could not read " + path.string());
		std::string parseError;
		if (!ParseAutogenAsset(text, out, &parseError))
			return Fail(error, path.filename().string() + ": " + parseError);
		return true;
	}

	double GetAutogenTurn(const Point2 &incoming, const Point2 &outgoing)
	{
		// Counter-clockwise from above is a left turn; X-Plane counts left turns negative
		const Point2 a = Normalized(incoming), b = Normalized(outgoing);
		const double cross = a.x * b.z - a.z * b.x;		// In x / z (z south): positive for a right turn seen from above
		return std::atan2(cross, Dot(a, b)) * 180.0 / kPi;
	}

	// -------------------------------------------------------

	void LayoutAutogenPoint(const AutogenAsset &asset, const Point2 at, const double headingDegrees, const AutogenLayoutOptions &options, AutogenLayout &out)
	{
		if (asset.tiles.empty())
			return;
		Random random(options.seed);
		const AutogenTile &tile = asset.tiles[random.Pick(asset.tiles.size())];
		// The texture's up (after ROTATION) faces the heading, s to its right: unrotated at heading 0 the tile lies as
		// drawn, north up, the way WorldEditor previews it and X-Plane 12 draws X-Coder's KDEN concourses
		Frame frame;
		frame.origin = at;
		frame.n = HeadingToDirection(headingDegrees);
		frame.d = Right(frame.n);
		EmitTile(asset, tile, frame, 0.0f, nullptr, options, random, out);
	}

	double GetAutogenReach(const AutogenAsset &asset)
	{
		double reach = 0.0;
		for (const AutogenTile &tile : asset.tiles)
		{
			const Extent extent = TileExtent(tile);
			reach = std::max(reach, std::hypot(std::max(-extent.uMin, extent.uMax), std::max(-extent.vMin, extent.vMax)));
		}
		return reach;
	}

	// -------------------------------------------------------

	void LayoutAutogenString(const AutogenAsset &asset, const std::vector<std::vector<Point2>> &contours, const int spawning, const float height,
							 const AutogenLayoutOptions &options, AutogenLayout &out)
	{
		Random random(options.seed);
		Occupancy occupancy;
		occupancy.contours = &contours;
		const AutogenSpellingSet *set = asset.tiles.empty() ? nullptr : PickSet(asset, -1, height, 0.0f, 0.0f, random);

		// Each tile-spawning contour, cleaned: its corners (turns past the crease angles) and the sides between them
		std::vector<std::vector<Point2>> lines;
		std::vector<Corner> corners;
		std::vector<Side> sides;
		for (size_t c = 0; set && c < contours.size() && static_cast<int>(c) < spawning; ++c)
		{
			std::vector<Point2> points;
			for (const Point2 &p : contours[c])
			{
				if (points.empty() || std::hypot(p.x - points.back().x, p.z - points.back().z) > 0.05)
					points.push_back(p);
			}
			const bool closed = points.size() >= 4 && std::hypot(points.front().x - points.back().x, points.front().z - points.back().z) <= 0.05;
			if (closed)
				points.pop_back();
			if (points.size() < 2)
				continue;
			const size_t count = points.size();
			const size_t line = lines.size();
			const auto segment = [&points, count](const size_t i)
			{
				const Point2 &a = points[i % count], &b = points[(i + 1) % count];
				return Point2{ b.x - a.x, b.z - a.z };
			};

			std::vector<size_t> vertices;	// Corner vertices along the contour
			const size_t cornerBase = corners.size();
			for (size_t i = closed ? 0 : 1; i < (closed ? count : count - 1); ++i)
			{
				const Point2 in = segment((i + count - 1) % count), outgoing = segment(i);
				const double turn = GetAutogenTurn(in, outgoing);
				if (turn > asset.creaseLeft && turn < asset.creaseRight)
					continue;
				Corner corner;
				corner.line = line;
				corner.index = i;
				corner.turn = turn;
				corner.lengthIn = std::hypot(in.x, in.z);
				corner.lengthOut = std::hypot(outgoing.x, outgoing.z);
				corners.push_back(corner);
				vertices.push_back(i);
			}

			const auto addSide = [&](const size_t from, const size_t to, const int startCorner, const int endCorner)
			{
				Side side;
				for (size_t i = from; i <= to; ++i)
					side.points.push_back(points[i % count]);
				side.startCorner = startCorner;
				side.endCorner = endCorner;
				if (side.points.size() >= 2)
					sides.push_back(std::move(side));
			};
			if (vertices.empty())
				addSide(0, closed ? count : count - 1, -1, -1);	// A closed contour without corners: one side round it
			else
			{
				if (!closed && vertices.front() > 0)
					addSide(0, vertices.front(), -1, static_cast<int>(cornerBase));
				const size_t between = closed ? vertices.size() : vertices.size() - 1;
				for (size_t k = 0; k < between; ++k)
				{
					const size_t next = (k + 1) % vertices.size();
					const size_t to = next > k ? vertices[next] : vertices[next] + count;
					addSide(vertices[k], to, static_cast<int>(cornerBase + k), static_cast<int>(cornerBase + next));
				}
				if (!closed && vertices.back() < count - 1)
					addSide(vertices.back(), count - 1, static_cast<int>(cornerBase + vertices.size() - 1), -1);
			}
			lines.push_back(std::move(points));
		}

		// Places a group of tiles (a corner, a spelling): all of them or none when atomic, else those that fit
		const auto placeGroup = [&](const std::vector<std::pair<const AutogenTile *, Frame>> &plan, const std::vector<size_t> &order, const bool atomic, const bool share,
									std::vector<std::vector<Point2>> *footprints) -> bool
		{
			const Occupancy::Mark mark = occupancy.Save();
			std::vector<bool> fits(plan.size(), false);
			bool all = true;
			for (const size_t k : order)
			{
				PlacedTile footprint = Footprint(*plan[k].first, plan[k].second);
				fits[k] = occupancy.Allows(footprint);
				all = all && fits[k];
				if (fits[k])
				{
					if (footprints)
						footprints->push_back(footprint.footprint);
					occupancy.Add(std::move(footprint));
				}
			}
			if ((atomic && !all) || std::ranges::none_of(fits, [](const bool f) { return f; }))
			{
				occupancy.Restore(mark);
				if (footprints)
					footprints->clear();
				out.skipped += static_cast<uint32_t>(plan.size());
				return false;
			}
			const Point2 *shared = nullptr;
			Point2 sharedPoint;
			for (const size_t k : order)
			{
				if (!fits[k])
				{
					++out.skipped;
					continue;
				}
				if (share && asset.shareY && !shared)
				{
					sharedPoint = TileGround(*plan[k].first, plan[k].second);
					shared = &sharedPoint;
				}
				EmitTile(asset, *plan[k].first, plan[k].second, height, shared, options, random, out);
			}
			return true;
		};

		// 1. Corners, in contour order
		int previousCornerTile = -1;
		for (Corner &corner : corners)
		{
			const std::vector<Point2> &points = lines[corner.line];
			const size_t count = points.size();
			const Point2 &vertex = points[corner.index];
			const Point2 &before = points[(corner.index + count - 1) % count], &after = points[(corner.index + 1) % count];
			const Point2 dIn = Normalized({ vertex.x - before.x, vertex.z - before.z });
			const Point2 dOut = Normalized({ after.x - vertex.x, after.z - vertex.z });
			const Point2 bisector = Normalized({ dIn.x + dOut.x, dIn.z + dOut.z });

			// Matching rules by priority group (file order), shuffled within each
			std::vector<const AutogenCorner *> candidates;
			for (const AutogenCorner &rule : set->corners)
			{
				if (rule.pair)
					continue;	// ASSUMPTION: CORNER_PAIR isn't placed (no shipped string uses it)
				if (corner.turn < rule.angleMin || corner.turn > rule.angleMax)
					continue;
				if (corner.lengthIn < rule.len1Min || corner.lengthIn > rule.len1Max || corner.lengthOut < rule.len2Min || corner.lengthOut > rule.len2Max)
					continue;
				if (rule.group != 0 && previousCornerTile >= 0 && !asset.IsInGroup(rule.group, previousCornerTile))
					continue;
				candidates.push_back(&rule);
			}
			std::stable_sort(candidates.begin(), candidates.end(), [](const AutogenCorner *a, const AutogenCorner *b) { return a->priority < b->priority; });
			for (size_t a = 0; a < candidates.size();)
			{
				size_t b = a;
				while (b < candidates.size() && candidates[b]->priority == candidates[a]->priority)
					++b;
				for (size_t k = b - 1; k > a; --k)
					std::swap(candidates[k], candidates[a + random.Pick(k - a + 1)]);
				a = b;
			}

			for (const AutogenCorner *rule : candidates)
			{
				std::vector<std::pair<const AutogenTile *, Frame>> plan;
				for (const int id : rule->tiles)
					plan.emplace_back(asset.FindTile(id), Frame{});
				if (std::ranges::any_of(plan, [](const auto &entry) { return entry.first == nullptr; }))
					continue;

				if (plan.size() == 3 && plan[0].first->hasLeadPivot && plan[1].first->hasTrailPivot && plan[1].first->hasLeadPivot && plan[2].first->hasTrailPivot)
				{
					// Joined at the pivots: O1 = V + a1 dIn and O3 = V + a3 dOut on the sides, the middle turned half the turn;
					// lead(1) = trail(2) and lead(2) = trail(3) give a1 dIn - a3 dOut = B - A + T2 - L2
					const Frame f1 = MakeFrame({}, dIn), f2 = MakeFrame({}, bisector), f3 = MakeFrame({}, dOut);
					const AutogenTile &t1 = *plan[0].first, &t2 = *plan[1].first, &t3 = *plan[2].first;
					const auto [l1u, l1v] = TileLocal(t1, t1.leadPivot[0], t1.leadPivot[1]);
					const auto [t2u, t2v] = TileLocal(t2, t2.trailPivot[0], t2.trailPivot[1]);
					const auto [l2u, l2v] = TileLocal(t2, t2.leadPivot[0], t2.leadPivot[1]);
					const auto [t3u, t3v] = TileLocal(t3, t3.trailPivot[0], t3.trailPivot[1]);
					const Point2 A = f1.Vector(l1u, l1v), T2 = f2.Vector(t2u, t2v), L2 = f2.Vector(l2u, l2v), B = f3.Vector(t3u, t3v);
					const Point2 rhs{ B.x - A.x + T2.x - L2.x, B.z - A.z + T2.z - L2.z };
					const double det = dIn.x * -dOut.z + dOut.x * dIn.z;
					if (std::abs(det) < 1e-6)
						continue;
					const double a1 = (rhs.x * -dOut.z + dOut.x * rhs.z) / det;
					const double a3 = (dIn.x * rhs.z - rhs.x * dIn.z) / det;
					plan[0].second = MakeFrame({ vertex.x + dIn.x * a1, vertex.z + dIn.z * a1 }, dIn);
					plan[2].second = MakeFrame({ vertex.x + dOut.x * a3, vertex.z + dOut.z * a3 }, dOut);
					const Point2 pivot = plan[0].second.At(l1u, l1v);
					plan[1].second = MakeFrame({ pivot.x - T2.x, pivot.z - T2.z }, bisector);
				}
				else
				{
					// The middle tile on the corner along the bisector; the others along the sides either side of it
					const size_t middle = plan.size() == 3 ? 1 : 0;
					plan[middle].second = MakeFrame(vertex, bisector);
					if (plan.size() == 3)
					{
						const Extent e0 = TileExtent(*plan[0].first), e1 = TileExtent(*plan[1].first), e2 = TileExtent(*plan[2].first);
						const double back = std::abs(e1.uMin) + e0.uMax, ahead = e1.uMax - e2.uMin;
						plan[0].second = MakeFrame({ vertex.x - dIn.x * back, vertex.z - dIn.z * back }, dIn);
						plan[2].second = MakeFrame({ vertex.x + dOut.x * ahead, vertex.z + dOut.z * ahead }, dOut);
					}
				}

				std::vector<size_t> order;
				if (plan.size() == 3)
					order = { static_cast<size_t>(rule->order[0]), static_cast<size_t>(rule->order[1]), static_cast<size_t>(rule->order[2]) };
				else
					order = { 0 };
				std::vector<std::vector<Point2>> footprints;
				if (!placeGroup(plan, order, rule->atomic, true, &footprints))
					continue;

				// What the corner covers of the sides either side
				for (const std::vector<Point2> &footprint : footprints)
				{
					for (const Point2 &p : footprint)
					{
						const Point2 offset{ p.x - vertex.x, p.z - vertex.z };
						corner.trimIn = std::max(corner.trimIn, -Dot(offset, dIn));
						corner.trimOut = std::max(corner.trimOut, Dot(offset, dOut));
					}
				}
				corner.placedTile = rule->tiles[plan.size() == 3 ? 1 : 0];
				previousCornerTile = corner.placedTile;
				break;
			}
		}

		// 2. Sides: spellings from the free start to the free end; 3. a cap at the end. Strings carry up to a thousand
		// spellings: their widths and tiles are looked up once
		std::unordered_map<int, std::pair<const AutogenTile *, Extent>> tilesById;
		for (const AutogenTile &tile : asset.tiles)
			tilesById.try_emplace(tile.id, &tile, TileExtent(tile));
		const auto findTile = [&tilesById](const int id) -> const std::pair<const AutogenTile *, Extent> *
		{
			const auto it = tilesById.find(id);
			return it != tilesById.end() ? &it->second : nullptr;
		};
		std::vector<double> spellingWidths;
		if (set)
		{
			for (const AutogenSpelling &spelling : set->spellings)
			{
				double width = 0.0;
				for (const int id : spelling.tiles)
				{
					if (const auto *entry = findTile(id))
						width += entry->second.Width();
				}
				spellingWidths.push_back(width);
			}
		}
		for (const Side &side : sides)
		{
			const Path path(side.points);
			const double length = path.Length();
			const double from = side.startCorner >= 0 ? corners[static_cast<size_t>(side.startCorner)].trimOut : 0.0;
			const double to = length - (side.endCorner >= 0 ? corners[static_cast<size_t>(side.endCorner)].trimIn : 0.0);
			if (to - from < 1.0)
				continue;

			double at = from;
			int previousTile = -1;
			bool placedAny = false;
			while (to - at > 0.5)
			{
				// Spellings that fit what's left, from the first priority group that has one
				const double room = to - at;
				const AutogenSpelling *chosen = nullptr;
				std::vector<const AutogenSpelling *> group;
				int priority = std::numeric_limits<int>::max();
				for (size_t s = 0; s < set->spellings.size(); ++s)
				{
					const AutogenSpelling &spelling = set->spellings[s];
					if (spelling.priority > priority)
						continue;
					const double width = spellingWidths[s];
					if (width <= 0.0 || width > room + 1e-3)
						continue;
					if (spelling.atomic && (length < spelling.lenMin || length > spelling.lenMax))
						continue;
					if (spelling.atomic && spelling.group != 0 && previousTile >= 0 && !asset.IsInGroup(spelling.group, previousTile))
						continue;
					if (spelling.priority < priority)
					{
						priority = spelling.priority;
						group.clear();
					}
					group.push_back(&spelling);
				}
				if (group.empty())
					break;
				chosen = group[random.Pick(group.size())];

				// Its tiles one after another along the side
				std::vector<std::pair<const AutogenTile *, Frame>> plan;
				double u = at;
				for (const int id : chosen->tiles)
				{
					const auto *entry = findTile(id);
					if (!entry)
						continue;
					const Extent &extent = entry->second;
					plan.emplace_back(entry->first, SideFrame(path, u, extent.Width(), extent));
					u += extent.Width();
				}
				std::vector<size_t> order(plan.size());
				std::iota(order.begin(), order.end(), size_t{ 0 });
				placedAny |= placeGroup(plan, order, chosen->atomic, chosen->atomic, nullptr);
				previousTile = chosen->tiles.back();
				at = u;	// A group that didn't fit leaves its gap (X-Plane slides tiles instead)
			}

			// A cap flush with the side's end, over the last tile
			if (placedAny && !set->caps.empty())
			{
				if (const AutogenTile *cap = asset.FindTile(set->caps[random.Pick(set->caps.size())]))
				{
					const Extent extent = TileExtent(*cap);
					const Frame frame = SideFrame(path, std::max(from, to - extent.Width()), extent.Width(), extent);
					PlacedTile footprint = Footprint(*cap, frame);
					// It may cover the tile it ends, not the polygon's outside
					if (std::ranges::all_of(footprint.stops, [&contours](const Point2 &stop) { return InPolylines(contours, stop); }))
					{
						EmitTile(asset, *cap, frame, height, nullptr, options, random, out);
						occupancy.Add(std::move(footprint));
					}
				}
			}
		}

		// 4. Trees over what the tiles left
		if (asset.fillLayer >= 0 && !asset.vegetation.empty())
		{
			AutogenLayout::Fill fill;
			fill.contours = contours;
			fill.footprints = occupancy.footprints;
			fill.layer = asset.fillLayer;
			out.fills.push_back(std::move(fill));
		}
	}

	// -------------------------------------------------------

	void LayoutAutogenBlock(const AutogenAsset &asset, const std::vector<Point2> &polygon, const int blockCode, const float height,
							const AutogenLayoutOptions &options, AutogenLayout &out)
	{
		std::vector<Point2> quad = polygon;
		if (quad.size() == 5 && std::hypot(quad.front().x - quad.back().x, quad.front().z - quad.back().z) < 0.05)
			quad.pop_back();
		if (quad.size() != 4 || asset.grids.empty())
			return;
		Random random(options.seed);

		// The block's frame: the first side along the grid's bottom, the grid reaching into the polygon
		const Frame frame = MakeFrame(quad[0], { quad[1].x - quad[0].x, quad[1].z - quad[0].z });
		double u[4], v[4];
		for (int i = 0; i < 4; ++i)
		{
			const Point2 offset{ quad[i].x - quad[0].x, quad[i].z - quad[0].z };
			u[i] = Dot(offset, frame.d);
			v[i] = Dot(offset, frame.n);
		}
		// Narrowest width along the first side and depth across it
		const double width = std::min(u[1] - u[0], u[2] - u[3]);
		const double depth = std::min(v[2], v[3]);
		if (width <= 1.0 || depth <= 1.0)
			return;

		const AutogenSpellingSet *set = PickSet(asset, blockCode, height, static_cast<float>(width), static_cast<float>(depth), random);
		if (!set || set->spellingsS.empty() || set->spellingsT.empty())
			return;
		const AutogenGrid &grid = asset.grids[static_cast<size_t>(std::clamp(set->grid, 0, static_cast<int>(asset.grids.size()) - 1))];
		const Assembly columns = Assemble(set->spellingsS, grid.cutsH, grid.slopH, grid.metersPerPixelS, grid.edgeBest[0], grid.edgeBest[2], grid.hasEdges, width, random);
		const Assembly rows = Assemble(set->spellingsT, grid.cutsV, grid.slopV, grid.metersPerPixelT, grid.edgeBest[1], grid.edgeBest[3], grid.hasEdges, depth, random);
		if (columns.cells.empty() || rows.cells.empty())
			return;

		// The best-fit edge on the polygon's first corner
		const double originU = u[0], originV = 0.0;
		const double upHeading = Heading(frame.n);
		const auto inside = [&](const Point2 &p) { return InRing(quad, p); };

		// Ground: one quad per cell, cut at the polygon when CROP is on
		for (size_t i = 0; i < columns.cells.size(); ++i)
		{
			for (size_t j = 0; j < rows.cells.size(); ++j)
			{
				const double w = columns.width[i], h = rows.width[j];
				if (w <= 1e-3 || h <= 1e-3 || asset.hideTiles || asset.texture.empty())
					continue;
				const float s0 = grid.cutsH[static_cast<size_t>(columns.cells[i])], t0 = grid.cutsV[static_cast<size_t>(rows.cells[j])];
				const float s1 = s0 + static_cast<float>(w / grid.metersPerPixelS), t1 = t0 + static_cast<float>(h / grid.metersPerPixelT);
				const double x0 = originU + columns.start[i], y0 = originV + rows.start[j];
				AutogenLayout::Ground cell;
				cell.ring = { frame.At(x0, y0), frame.At(x0 + w, y0), frame.At(x0 + w, y0 + h), frame.At(x0, y0 + h) };
				const float tw = std::max(1.0f, asset.textureWidth), th = std::max(1.0f, asset.textureHeight);
				cell.uvs = { { s0 / tw, t0 / th }, { s1 / tw, t0 / th }, { s1 / tw, t1 / th }, { s0 / tw, t1 / th } };
				if (asset.crop)
				{
					std::vector<Point2> clip = quad;
					if (SignedArea(clip) < 0.0)
						std::ranges::reverse(clip);
					ClipToConvex(cell.ring, cell.uvs, clip);
				}
				if (cell.ring.size() >= 3)
					out.ground.push_back(std::move(cell));
			}
		}
		out.tiles += static_cast<uint32_t>(columns.cells.size() * rows.cells.size());

		// Annotations: one copy per occurrence of their cell in the spellings
		const auto place = [&](const float s, const float t, const size_t i, const size_t j) -> Point2
		{
			const int c = columns.cells[i], r = rows.cells[j];
			const double ds = (s - grid.cutsH[static_cast<size_t>(c)]) * grid.metersPerPixelS;
			const double dt = (t - grid.cutsV[static_cast<size_t>(r)]) * grid.metersPerPixelT;
			return frame.At(originU + columns.start[i] + ds, originV + rows.start[j] + dt);
		};
		const auto shown = [&](const float s, const float t, const size_t i, const size_t j)
		{
			const int c = columns.cells[i], r = rows.cells[j];
			return (s - grid.cutsH[static_cast<size_t>(c)]) * grid.metersPerPixelS <= columns.width[i] + 1e-3 &&
				   (t - grid.cutsV[static_cast<size_t>(r)]) * grid.metersPerPixelT <= rows.width[j] + 1e-3;
		};
		const auto groundOf = [&](const AutogenAnnotations::Object &object, const Point2 &fallback) -> Point2
		{
			if (!object.hasGroundFormula)
				return fallback;
			const double xr = object.groundFormula[0], yr = object.groundFormula[2];
			const double w0 = (1.0 - xr) * (1.0 - yr), w1 = xr * (1.0 - yr), w2 = xr * yr, w3 = (1.0 - xr) * yr;
			const Point2 blend{ quad[0].x * w0 + quad[1].x * w1 + quad[2].x * w2 + quad[3].x * w3, quad[0].z * w0 + quad[1].z * w1 + quad[2].z * w2 + quad[3].z * w3 };
			const Point2 delta = frame.Vector(object.groundFormula[1], object.groundFormula[3]);
			return { blend.x + delta.x, blend.z + delta.z };
		};

		const AutogenAnnotations &annotations = grid.annotations;
		for (size_t i = 0; i < columns.cells.size(); ++i)
		{
			for (size_t j = 0; j < rows.cells.size(); ++j)
			{
				const int c = columns.cells[i], r = rows.cells[j];
				const auto inCell = [&](const float s, const float t) { return CellOf(grid.cutsH, s) == c && CellOf(grid.cutsV, t) == r; };
				const Point2 cellCenter = frame.At(originU + columns.start[i] + 0.5 * columns.width[i], originV + rows.start[j] + 0.5 * rows.width[j]);

				for (const AutogenAnnotations::Object &object : annotations.objects)
				{
					if (!inCell(object.s, object.t) || !shown(object.s, object.t, i, j))
						continue;
					const Point2 at = place(object.s, object.t, i, j);
					if (asset.crop && !inside(at))
						continue;
					if (!Appears(object.lo, object.hi, options.objectLevel, random))
						continue;
					AutogenLayout::Object placed;
					placed.object = object.object;
					placed.at = at;
					placed.heading = std::fmod(upHeading + object.heading + 720.0, 360.0);
					placed.graded = object.mode != AutogenAnnotations::Object::Mode::Draped;
					placed.ground = groundOf(object, cellCenter);
					placed.raise = object.mode == AutogenAnnotations::Object::Mode::Scraper ? ScraperRaise(object, height) : object.delta;
					out.objects.push_back(placed);
				}
				for (const AutogenAnnotations::Facade &facade : annotations.facades)
				{
					if (!inCell(facade.points[0], facade.points[1]) || !shown(facade.points[0], facade.points[1], i, j))
						continue;
					AutogenLayout::Facade placed;
					placed.facade = facade.facade;
					placed.height = facade.height;
					// Every point moves with the cell its first point is in
					for (size_t k = 0; k + 1 < facade.points.size(); k += 2)
						placed.points.push_back(place(facade.points[k], facade.points[k + 1], i, j));
					placed.walls = facade.walls;
					if (asset.crop && !inside(placed.points.front()))
						continue;
					out.facades.push_back(std::move(placed));
				}
				for (const AutogenAnnotations::Tree &tree : annotations.trees)
				{
					if (!inCell(tree.s, tree.t) || !shown(tree.s, tree.t, i, j))
						continue;
					const Point2 at = place(tree.s, tree.t, i, j);
					if (asset.crop && !inside(at))
						continue;
					out.trees.push_back({ at, std::fmod(upHeading + tree.heading + 720.0, 360.0), tree.width, tree.layer });
				}
				for (const AutogenAnnotations::TreeLine &line : annotations.treeLines)
				{
					if (!inCell(line.s1, line.t1) || !shown(line.s1, line.t1, i, j))
						continue;
					out.treeLines.push_back({ place(line.s1, line.t1, i, j), place(line.s2, line.t2, i, j), line.layer });
				}
			}
		}
	}
}

#if defined(_MSC_VER) && !defined(__clang__) && !defined(NDEBUG)
#pragma optimize("", on)
#endif

// -------------------------------------------------------
