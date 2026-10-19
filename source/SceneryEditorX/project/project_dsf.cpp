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
 * project_dsf.cpp
 * -------------------------------------------------------
 * Created: 19/10/2026
 * -------------------------------------------------------
 */
#include "project_dsf.h"
#include <algorithm>
#include <cctype>
#include <cmath>
#include <cstdio>
#include <map>
#include <string_view>

// -------------------------------------------------------

namespace SceneryEditorX::ProjectFile
{
	namespace
	{
		using XPlane::DsfFile;
		using XPlane::DsfObjectPlacement;
		using XPlane::DsfPoint;
		using XPlane::DsfPolygonPlacement;
		using XPlane::DsfRoadPoint;
		using XPlane::DsfRoadSegment;

		using Winding = std::vector<DsfPoint>;

		constexpr double MIRROR_EPSILON = 1e-10;	// Degrees (about 0.01 mm): handles this close to mirror images are one curve through the node

		/* @brief sim/exclude_* key for each ExclusionBits bit, in bit order. */
		constexpr const char *EXCLUSION_KEYS[] = { "sim/exclude_obj", "sim/exclude_fac", "sim/exclude_for", "sim/exclude_bch", "sim/exclude_net", "sim/exclude_lin", "sim/exclude_pol", "sim/exclude_str" };

		/* @brief What a DSF point carries after longitude and latitude. */
		enum class Layout : uint8_t
		{
			Plain,			// 2 planes
			Bezier,			// 4: control lon, control lat
			Wall,			// 3: wall type
			WallBezier,		// 5: wall type, control lon, control lat
			Uv,				// 4: s, t
			UvBezier		// 8: control lon, control lat, s, t, control s, control t
		};

		bool IsBezier(const Layout layout)
		{
			return layout == Layout::Bezier || layout == Layout::WallBezier || layout == Layout::UvBezier;
		}

		uint8_t PlaneCount(const Layout layout)
		{
			switch (layout)
			{
				case Layout::Plain:			return 2;
				case Layout::Bezier:		return 4;
				case Layout::Wall:			return 3;
				case Layout::WallBezier:	return 5;
				case Layout::Uv:			return 4;
				case Layout::UvBezier:		return 8;
			}
			return 2;
		}

		bool EndsWithNoCase(const std::string_view text, const std::string_view suffix)
		{
			if (text.size() < suffix.size())
				return false;
			return std::equal(suffix.begin(), suffix.end(), text.end() - static_cast<std::ptrdiff_t>(suffix.size()), [](const char a, const char b)
			{
				return std::tolower(static_cast<unsigned char>(a)) == std::tolower(static_cast<unsigned char>(b));
			});
		}

		/**
		 * @brief The DSF point of a node with a control point (an offset from the node in degrees) and a UV control
		 * point (an offset from its s, t). Zero offsets give a corner: the control point sits on the node.
		 */
		DsfPoint MakePoint(const GeoNode &node, const Layout layout, const double controlLatitude = 0.0, const double controlLongitude = 0.0, const float controlS = 0.0f, const float controlT = 0.0f)
		{
			DsfPoint point;
			point.planes = PlaneCount(layout);
			point.v[0] = node.longitude;
			point.v[1] = node.latitude;
			switch (layout)
			{
				case Layout::Plain:
					break;
				case Layout::Bezier:
					point.v[2] = node.longitude + controlLongitude;
					point.v[3] = node.latitude + controlLatitude;
					break;
				case Layout::Wall:
					point.v[2] = node.wallType;
					break;
				case Layout::WallBezier:
					point.v[2] = node.wallType;
					point.v[3] = node.longitude + controlLongitude;
					point.v[4] = node.latitude + controlLatitude;
					break;
				case Layout::Uv:
					point.v[2] = node.s;
					point.v[3] = node.t;
					break;
				case Layout::UvBezier:
					point.v[2] = node.longitude + controlLongitude;
					point.v[3] = node.latitude + controlLatitude;
					point.v[4] = node.s;
					point.v[5] = node.t;
					point.v[6] = static_cast<double>(node.s) + controlS;
					point.v[7] = static_cast<double>(node.t) + controlT;
					break;
			}
			return point;
		}

		/**
		 * @brief Appends the DSF points of a node. useLo / useHi say whether its incoming / outgoing handle shapes a
		 * curve (an open chain's first node has nothing coming in, its last nothing going out). Like WorldEditor's
		 * BezierPointSeqToTriple: mirror-image handles make one point; otherwise the incoming handle (as the mirror of
		 * a control point), the corner and the outgoing handle each make one, on top of each other.
		 */
		void AppendNode(Winding &out, const GeoNode &node, const Layout layout, const bool useLo, const bool useHi)
		{
			if (!IsBezier(layout))
			{
				out.push_back(MakePoint(node, layout));
				return;
			}

			const bool hasLo = useLo && node.HasLo();
			const bool hasHi = useHi && node.HasHi();
			const bool mirrored = std::abs(node.loLatitude + node.hiLatitude) <= MIRROR_EPSILON && std::abs(node.loLongitude + node.hiLongitude) <= MIRROR_EPSILON;
			if (hasLo && hasHi && mirrored)
			{
				out.push_back(MakePoint(node, layout, node.hiLatitude, node.hiLongitude, node.sHi, node.tHi));
				return;
			}
			if (hasLo)
				out.push_back(MakePoint(node, layout, -node.loLatitude, -node.loLongitude, -node.sLo, -node.tLo));
			out.push_back(MakePoint(node, layout));
			if (hasHi)
				out.push_back(MakePoint(node, layout, node.hiLatitude, node.hiLongitude, node.sHi, node.tHi));
		}

		/* @brief A ring's (closed) or chain's (open) DSF points. */
		Winding MakeWinding(const GeoRing &ring, const Layout layout, const bool closed)
		{
			Winding out;
			out.reserve(ring.size());
			for (size_t i = 0; i < ring.size(); ++i)
				AppendNode(out, ring[i], layout, closed || i > 0, closed || i + 1 < ring.size());
			return out;
		}

		bool HasBezier(const std::vector<GeoRing> &rings)
		{
			for (const GeoRing &ring : rings)
			{
				for (const GeoNode &node : ring)
				{
					if (node.HasBezier())
						return true;
				}
			}
			return false;
		}

		/* @brief Shoelace area in degrees (longitude east, latitude north): positive counter-clockwise. */
		double SignedArea(const GeoRing &ring)
		{
			if (ring.size() < 3)
				return 0.0;
			// Relative to the first node, so a few square meters don't drown in the rounding of -123 x 44
			const double lon0 = ring[0].longitude, lat0 = ring[0].latitude;
			double sum = 0.0;
			for (size_t i = 0, j = ring.size() - 1; i < ring.size(); j = i++)
				sum += (ring[j].longitude - lon0) * (ring[i].latitude - lat0) - (ring[i].longitude - lon0) * (ring[j].latitude - lat0);
			return 0.5 * sum;
		}

		/**
		 * @brief A closed ring the other way round. Handles swap ends, and what belongs to a segment (wall type, spawn
		 * flag: stored on the node the segment starts at) moves to the node the reversed segment starts at.
		 */
		GeoRing Reversed(const GeoRing &ring)
		{
			const size_t count = ring.size();
			GeoRing out(ring.rbegin(), ring.rend());
			for (size_t k = 0; k < count; ++k)
			{
				GeoNode &node = out[k];
				std::swap(node.loLatitude, node.hiLatitude);
				std::swap(node.loLongitude, node.hiLongitude);
				std::swap(node.sLo, node.sHi);
				std::swap(node.tLo, node.tHi);
				const GeoNode &segmentStart = ring[(2 * count - 2 - k) % count];
				node.wallType = segmentStart.wallType;
				node.spawnTiles = segmentStart.spawnTiles;
			}
			return out;
		}

		/* @brief Outline counter-clockwise, holes clockwise, as WorldEditor exports filled shapes. */
		std::vector<GeoRing> Oriented(std::vector<GeoRing> rings)
		{
			for (size_t i = 0; i < rings.size(); ++i)
			{
				const double area = SignedArea(rings[i]);
				if (area != 0.0 && (area > 0.0) != (i == 0))
					rings[i] = Reversed(rings[i]);
			}
			return rings;
		}

		/* @brief The item's rings when the outline has enough nodes (holes with too few are dropped), else none. */
		std::vector<GeoRing> UsableRings(const SceneryItem &item, const size_t minimum)
		{
			std::vector<GeoRing> rings;
			if (item.rings.empty() || item.rings[0].size() < minimum)
				return rings;
			for (const GeoRing &ring : item.rings)
			{
				if (ring.size() >= minimum)
					rings.push_back(ring);
			}
			return rings;
		}

		std::string FormatCoordinate(const double longitude, const double latitude)
		{
			char text[64];
			std::snprintf(text, sizeof(text), "%.6f/%.6f", longitude, latitude);
			return text;
		}

		// -------------------------------------------------------

		class Converter
		{
		public:
			explicit Converter(const DsfConversionOptions &options) : m_Options(options)
			{
				m_Result.dsf.properties.emplace_back("sim/overlay", "1");
			}

			void Walk(const std::vector<SceneryItem> &items, const bool parentHidden)
			{
				for (const SceneryItem &item : items)
				{
					const bool hidden = parentHidden || item.hidden;
					if (item.IsContainer())
					{
						Walk(item.children, hidden && !m_Options.includeHidden);
						continue;
					}
					if (item.kind == ItemKind::Raw)
						continue;
					if (hidden && !m_Options.includeHidden)
					{
						++m_Result.hidden;
						continue;
					}
					Convert(item);
				}
			}

			DsfConversion Finish()
			{
				DsfFile &dsf = m_Result.dsf;
				if (m_HasBounds)
				{
					dsf.west = std::floor(m_West);
					dsf.south = std::floor(m_South);
					dsf.east = std::max(dsf.west + 1.0, std::ceil(m_East));
					dsf.north = std::max(dsf.south + 1.0, std::ceil(m_North));
					// After sim/overlay, before the exclusions (WorldEditor's order)
					const std::pair<std::string, std::string> bounds[] = {
						{ "sim/west", std::to_string(static_cast<int>(dsf.west)) }, { "sim/east", std::to_string(static_cast<int>(dsf.east)) },
						{ "sim/north", std::to_string(static_cast<int>(dsf.north)) }, { "sim/south", std::to_string(static_cast<int>(dsf.south)) } };
					dsf.properties.insert(dsf.properties.begin() + 1, std::begin(bounds), std::end(bounds));
				}
				return std::move(m_Result);
			}

		private:
			void Warn(const SceneryItem &item, const std::string &problem)
			{
				const std::string name = item.name.empty() ? item.id : item.name;
				m_Result.warnings.push_back(name + " (" + GetItemKindName(item.kind) + "): " + problem);
			}

			void Include(const double latitude, const double longitude)
			{
				if (!m_HasBounds)
				{
					m_West = m_East = longitude;
					m_South = m_North = latitude;
					m_HasBounds = true;
					return;
				}
				m_West = std::min(m_West, longitude);
				m_East = std::max(m_East, longitude);
				m_South = std::min(m_South, latitude);
				m_North = std::max(m_North, latitude);
			}

			uint32_t Definition(std::vector<std::string> &definitions, std::map<std::string, uint32_t> &index, const std::string &resource)
			{
				const auto [it, added] = index.emplace(resource, static_cast<uint32_t>(definitions.size()));
				if (added)
					definitions.push_back(resource);
				return it->second;
			}

			void AddPolygon(const SceneryItem &item, const uint16_t parameter, const Layout layout, std::vector<Winding> windings)
			{
				std::erase_if(windings, [](const Winding &winding) { return winding.empty(); });
				if (windings.empty())
					return;
				for (const Winding &winding : windings)
				{
					for (const DsfPoint &point : winding)
						Include(point.Latitude(), point.Longitude());
				}
				DsfPolygonPlacement placement;
				placement.definition = Definition(m_Result.dsf.polygonDefinitions, m_PolygonIndex, item.resource);
				placement.parameter = parameter;
				placement.planes = PlaneCount(layout);
				placement.windings = std::move(windings);
				m_Result.dsf.polygons.push_back(std::move(placement));
				m_Result.polygonItems.push_back(item.id);
			}

			/* @brief Filled shapes: every ring a winding of one polygon. */
			void AddRings(const SceneryItem &item, const uint16_t parameter, const Layout layout, const std::vector<GeoRing> &rings)
			{
				std::vector<Winding> windings;
				for (const GeoRing &ring : rings)
					windings.push_back(MakeWinding(ring, layout, true));
				AddPolygon(item, parameter, layout, std::move(windings));
			}

			/* @brief Checks the resource suits the kind (case-insensitive extension). */
			bool CheckResource(const SceneryItem &item, std::initializer_list<std::string_view> extensions)
			{
				if (item.resource.empty())
				{
					Warn(item, "no resource");
					return false;
				}
				for (const std::string_view extension : extensions)
				{
					if (EndsWithNoCase(item.resource, extension))
						return true;
				}
				Warn(item, "'" + item.resource + "' is not a " + std::string(*extensions.begin()) + " asset");
				return false;
			}

			void Convert(const SceneryItem &item)
			{
				switch (item.kind)
				{
					case ItemKind::Object:				ConvertObject(item); break;
					case ItemKind::Polygon:				ConvertPolygon(item, false); break;
					case ItemKind::Orthophoto:			ConvertPolygon(item, true); break;
					case ItemKind::Line:				ConvertLine(item); break;
					case ItemKind::String:				ConvertString(item); break;
					case ItemKind::Forest:				ConvertForest(item); break;
					case ItemKind::Facade:				ConvertFacade(item); break;
					case ItemKind::Autogen:				ConvertAutogen(item); break;
					case ItemKind::ExclusionZone:
					case ItemKind::ExclusionPolygon:	ConvertExclusion(item); break;
					case ItemKind::Road:				ConvertRoad(item); break;
					default:							break;
				}
			}

			uint32_t Junction(const std::string &node)
			{
				const auto [it, added] = m_Junctions.emplace(node, static_cast<uint32_t>(m_Junctions.size() + 1));
				return it->second;
			}

			/**
			 * @brief A road segment from its start node to its end node. Every curved stretch becomes a cubic: the
			 * handle out of one node and into the next as two control points (a missing one is set a hair along the
			 * curve, as WorldEditor does: X-Plane wants a direction at both ends).
			 */
			void ConvertRoad(const SceneryItem &item)
			{
				if (!CheckResource(item, { ".net" }))
					return;
				if (item.rings.empty() || item.rings[0].size() < 2)
				{
					Warn(item, "needs at least 2 nodes");
					return;
				}
				if (item.roadSubtype < 0 || item.roadSubtype > 255)
				{
					Warn(item, "road type " + std::to_string(item.roadSubtype) + " is outside 0..255");
					return;
				}
				const GeoRing &chain = item.rings[0];
				DsfRoadSegment road;
				road.definition = Definition(m_Result.dsf.networkDefinitions, m_NetworkIndex, item.resource);
				road.subtype = static_cast<uint32_t>(item.roadSubtype);
				road.startJunction = Junction(item.startNode.empty() ? item.id + "/start" : item.startNode);
				road.endJunction = Junction(item.endNode.empty() ? item.id + "/end" : item.endNode);
				road.startLevel = item.startLayer;
				road.endLevel = item.endLayer;
				for (size_t i = 0; i < chain.size(); ++i)
				{
					const GeoNode &node = chain[i];
					road.points.push_back({ node.longitude, node.latitude, false });
					Include(node.latitude, node.longitude);
					if (i + 1 == chain.size())
						break;
					const GeoNode &next = chain[i + 1];
					const bool out = node.HasHi(), in = next.HasLo();
					if (!out && !in)
						continue;
					DsfRoadPoint c1{ node.longitude + node.hiLongitude, node.latitude + node.hiLatitude, true };
					DsfRoadPoint c2{ next.longitude + next.loLongitude, next.latitude + next.loLatitude, true };
					if (!out)
						c1 = { node.longitude + (c2.longitude - node.longitude) * 0.01, node.latitude + (c2.latitude - node.latitude) * 0.01, true };
					if (!in)
						c2 = { next.longitude + (c1.longitude - next.longitude) * 0.01, next.latitude + (c1.latitude - next.latitude) * 0.01, true };
					road.points.push_back(c1);
					road.points.push_back(c2);
				}
				m_Result.dsf.roads.push_back(std::move(road));
				m_Result.roadItems.push_back(item.id);
			}

			void ConvertObject(const SceneryItem &item)
			{
				const bool autogen = EndsWithNoCase(item.resource, ".agp");
				const bool object = EndsWithNoCase(item.resource, ".obj");
				if (object && !m_Options.includeObjects)
					return;
				if (!autogen && !object)
				{
					CheckResource(item, { ".obj", ".agp" });
					return;
				}

				DsfObjectPlacement placement;
				placement.definition = Definition(m_Result.dsf.objectDefinitions, m_ObjectIndex, item.resource);
				placement.longitude = item.longitude;
				placement.latitude = item.latitude;
				placement.heading = item.heading;
				placement.elevation = item.elevationMode == ElevationMode::Ground ? 0.0 : item.altitude;
				placement.mode = item.elevationMode == ElevationMode::Msl ? DsfObjectPlacement::Mode::Msl : item.elevationMode == ElevationMode::Agl ? DsfObjectPlacement::Mode::Agl : DsfObjectPlacement::Mode::Draped;
				Include(item.latitude, item.longitude);
				m_Result.dsf.objects.push_back(placement);
				m_Result.objectItems.push_back(item.id);
			}

			void ConvertPolygon(const SceneryItem &item, const bool orthophoto)
			{
				if (!CheckResource(item, { ".pol" }))
					return;
				const std::vector<GeoRing> rings = Oriented(UsableRings(item, 3));
				if (rings.empty())
				{
					Warn(item, "needs at least 3 nodes");
					return;
				}
				const bool bezier = HasBezier(rings);

				// Texture coordinates on every node make it UV mapped; without them it falls back to a projected texture
				const bool uv = orthophoto && std::ranges::all_of(rings, [](const GeoRing &ring) { return std::ranges::all_of(ring, [](const GeoNode &node) { return node.hasUv; }); });
				if (orthophoto && !uv)
					Warn(item, "nodes without texture coordinates: drawn with a projected texture");
				if (uv)
					AddRings(item, 65535, bezier ? Layout::UvBezier : Layout::Uv, rings);
				else
					AddRings(item, EncodePolygonHeading(item.textureHeading), bezier ? Layout::Bezier : Layout::Plain, rings);
			}

			void ConvertLine(const SceneryItem &item)
			{
				if (!CheckResource(item, { ".lin" }))
					return;
				if (item.rings.empty() || item.rings[0].size() < 2)
				{
					Warn(item, "needs at least 2 nodes");
					return;
				}
				const GeoRing &chain = item.rings[0];
				const Layout layout = HasBezier({ chain }) ? Layout::Bezier : Layout::Plain;
				AddPolygon(item, item.closed ? 1 : 0, layout, { MakeWinding(chain, layout, item.closed) });
			}

			void ConvertString(const SceneryItem &item)
			{
				if (!CheckResource(item, { ".str" }))
					return;
				if (item.rings.empty() || item.rings[0].size() < 2)
				{
					Warn(item, "needs at least 2 nodes");
					return;
				}
				// Strings are always open in the DSF: a closed one comes back to its first node
				GeoRing chain = item.rings[0];
				if (item.closed)
					chain.push_back(chain.front());
				const Layout layout = HasBezier({ chain }) ? Layout::Bezier : Layout::Plain;
				AddPolygon(item, EncodeStringSpacing(item.spacing), layout, { MakeWinding(chain, layout, false) });
			}

			void ConvertForest(const SceneryItem &item)
			{
				if (!CheckResource(item, { ".for" }))
					return;
				const uint16_t parameter = EncodeForestParameter(item.density, item.fill);
				switch (item.fill)
				{
					case ForestFill::Area:
					{
						const std::vector<GeoRing> rings = Oriented(UsableRings(item, 3));
						if (rings.empty())
						{
							Warn(item, "needs at least 3 nodes");
							return;
						}
						AddRings(item, parameter, Layout::Plain, rings);
						break;
					}
					case ForestFill::Line:
					{
						// One open chain per ring, each its own polygon
						const std::vector<GeoRing> rings = UsableRings(item, 2);
						if (rings.empty())
						{
							Warn(item, "needs at least 2 nodes");
							return;
						}
						for (const GeoRing &ring : rings)
							AddPolygon(item, parameter, Layout::Plain, { MakeWinding(ring, Layout::Plain, false) });
						break;
					}
					case ForestFill::Points:
					{
						// Every node of every ring, one tree each
						Winding points;
						for (const GeoRing &ring : item.rings)
						{
							for (const GeoNode &node : ring)
								points.push_back(MakePoint(node, Layout::Plain));
						}
						if (points.empty())
						{
							Warn(item, "has no nodes");
							return;
						}
						AddPolygon(item, parameter, Layout::Plain, { std::move(points) });
						break;
					}
				}
			}

			void ConvertFacade(const SceneryItem &item)
			{
				if (!CheckResource(item, { ".fac" }))
					return;
				// Node order says which way the walls face (and chains have no orientation), so it is kept
				const std::vector<GeoRing> rings = UsableRings(item, 2);
				if (rings.empty())
				{
					Warn(item, "needs at least 2 nodes");
					return;
				}
				const bool bezier = HasBezier(rings);
				const Layout layout = item.pickWalls ? (bezier ? Layout::WallBezier : Layout::Wall) : (bezier ? Layout::Bezier : Layout::Plain);
				const uint16_t height = static_cast<uint16_t>(std::clamp(static_cast<double>(item.height), 0.0, 65535.0));
				AddRings(item, height, layout, rings);
			}

			void ConvertAutogen(const SceneryItem &item)
			{
				if (!CheckResource(item, { ".ags", ".agb" }))
					return;
				const std::vector<GeoRing> rings = Oriented(UsableRings(item, 3));
				if (rings.empty())
				{
					Warn(item, "needs at least 3 nodes");
					return;
				}

				// The outline splits into chains where spawning switches on or off (WorldEditor's AUTOGEN STRING EXPORTER):
				// the spawning chains first, then the non-spawning ones, then the holes.
				const GeoRing &outline = rings[0];
				std::vector<Winding> spawning(1), quiet;
				bool spawn = true;
				int chains = 1;
				for (const GeoNode &node : outline)
				{
					const DsfPoint point = MakePoint(node, Layout::Plain);
					(spawn ? spawning.back() : quiet.back()).push_back(point);
					if (node.spawnTiles != spawn)
					{
						if (spawn)
							quiet.push_back({ point });
						else
						{
							spawning.push_back({ point });
							++chains;
						}
						spawn = !spawn;
					}
				}
				(spawn ? spawning.back() : quiet.back()).push_back(MakePoint(outline.front(), Layout::Plain));

				std::vector<Winding> windings = std::move(spawning);
				windings.insert(windings.end(), std::make_move_iterator(quiet.begin()), std::make_move_iterator(quiet.end()));
				for (size_t i = 1; i < rings.size(); ++i)
					windings.push_back(MakeWinding(rings[i], Layout::Plain, true));

				const bool block = EndsWithNoCase(item.resource, ".agb");
				AddPolygon(item, EncodeAutogenParameter(item.height, block ? item.spelling : chains), Layout::Plain, std::move(windings));
			}

			void ConvertExclusion(const SceneryItem &item)
			{
				if ((item.exclusions & ExcludeAll) == 0)
				{
					Warn(item, "excludes nothing");
					return;
				}

				std::string value;
				if (item.kind == ItemKind::ExclusionZone)
				{
					if (item.rings.empty() || item.rings[0].size() < 2)
					{
						Warn(item, "needs its 2 corner nodes");
						return;
					}
					const GeoNode &a = item.rings[0][0], &b = item.rings[0][1];
					value = FormatExclusionBounds(std::min(a.longitude, b.longitude), std::min(a.latitude, b.latitude), std::max(a.longitude, b.longitude), std::max(a.latitude, b.latitude));
					Include(a.latitude, a.longitude);
					Include(b.latitude, b.longitude);
				}
				else
				{
					const std::vector<GeoRing> rings = Oriented(UsableRings(item, 3));
					if (rings.empty())
					{
						Warn(item, "needs at least 3 nodes");
						return;
					}
					double west = rings[0][0].longitude, east = west, south = rings[0][0].latitude, north = south;
					std::string points;
					for (const GeoRing &ring : rings)
					{
						for (const GeoNode &node : ring)
						{
							west = std::min(west, node.longitude);
							east = std::max(east, node.longitude);
							south = std::min(south, node.latitude);
							north = std::max(north, node.latitude);
							if (!points.empty())
								points += ',';
							points += FormatCoordinate(node.longitude, node.latitude);
							Include(node.latitude, node.longitude);
						}
					}
					value = FormatExclusionBounds(west, south, east, north) + ";" + points;
				}

				for (size_t bit = 0; bit < std::size(EXCLUSION_KEYS); ++bit)
				{
					if (item.exclusions & (1u << bit))
						m_Result.dsf.properties.emplace_back(EXCLUSION_KEYS[bit], value);
				}
				m_Result.exclusionItems.push_back(item.id);
			}

			DsfConversionOptions m_Options;
			DsfConversion m_Result;
			std::map<std::string, uint32_t> m_PolygonIndex;
			std::map<std::string, uint32_t> m_ObjectIndex;
			std::map<std::string, uint32_t> m_NetworkIndex;
			std::map<std::string, uint32_t> m_Junctions;		// Road node id -> DSF junction id (1 and up)
			bool m_HasBounds = false;
			double m_West = 0.0, m_South = 0.0, m_East = 0.0, m_North = 0.0;
		};

		/* @brief FNV-1a over a sequence of values. */
		class Fingerprint
		{
		public:
			template<typename T>
			void Add(const T &value)
			{
				m_Hash = HashFnv1a64(std::string_view(reinterpret_cast<const char *>(&value), sizeof(T)), m_Hash);
			}

			void Add(const std::string &text)
			{
				Add(text.size());
				m_Hash = HashFnv1a64(text, m_Hash);
			}

			[[nodiscard]] uint64_t Get() const { return m_Hash; }

		private:
			uint64_t m_Hash = 0xcbf29ce484222325ull;
		};
	}

	// -------------------------------------------------------

	DsfConversion BuildProjectDsf(const SceneryProject &project, const DsfConversionOptions &options)
	{
		Converter converter(options);
		converter.Walk(project.items, false);
		return converter.Finish();
	}

	uint64_t GetDsfFingerprint(const XPlane::DsfFile &dsf)
	{
		Fingerprint hash;
		for (const auto *definitions : { &dsf.objectDefinitions, &dsf.polygonDefinitions })
		{
			hash.Add(definitions->size());
			for (const std::string &definition : *definitions)
				hash.Add(definition);
		}
		hash.Add(dsf.objects.size());
		for (const DsfObjectPlacement &object : dsf.objects)
		{
			hash.Add(object.definition);
			hash.Add(object.longitude);
			hash.Add(object.latitude);
			hash.Add(object.heading);
			hash.Add(object.elevation);
			hash.Add(object.mode);
		}
		hash.Add(dsf.polygons.size());
		for (const DsfPolygonPlacement &polygon : dsf.polygons)
		{
			hash.Add(polygon.definition);
			hash.Add(polygon.parameter);
			hash.Add(polygon.planes);
			hash.Add(polygon.windings.size());
			for (const Winding &winding : polygon.windings)
			{
				hash.Add(winding.size());
				for (const DsfPoint &point : winding)
				{
					for (uint8_t plane = 0; plane < point.planes && plane < point.v.size(); ++plane)
						hash.Add(point.v[plane]);
				}
			}
		}
		hash.Add(dsf.networkDefinitions.size());
		for (const std::string &definition : dsf.networkDefinitions)
			hash.Add(definition);
		hash.Add(dsf.roads.size());
		for (const XPlane::DsfRoadSegment &road : dsf.roads)
		{
			hash.Add(road.definition);
			hash.Add(road.subtype);
			hash.Add(road.startJunction);
			hash.Add(road.endJunction);
			hash.Add(road.startLevel);
			hash.Add(road.endLevel);
			hash.Add(road.points.size());
			for (const XPlane::DsfRoadPoint &point : road.points)
			{
				hash.Add(point.longitude);
				hash.Add(point.latitude);
				hash.Add(point.control);
			}
		}
		hash.Add(dsf.properties.size());
		for (const auto &[key, value] : dsf.properties)
		{
			hash.Add(key);
			hash.Add(value);
		}
		return hash.Get();
	}

	uint16_t EncodePolygonHeading(const double heading)
	{
		double wrapped = std::isfinite(heading) ? std::fmod(heading, 360.0) : 0.0;
		if (wrapped < 0.0)
			wrapped += 360.0;
		if (wrapped >= 360.0)
			wrapped = 0.0;
		const unsigned whole = static_cast<unsigned>(wrapped);
		const unsigned eighths = static_cast<unsigned>(128.0 * (wrapped - whole));
		return static_cast<uint16_t>(whole + 360u * std::min(eighths, 127u));
	}

	uint16_t EncodeStringSpacing(const double spacing)
	{
		if (!std::isfinite(spacing) || spacing <= 0.0)
			return 0;
		if (spacing < 433.8)
		{
			// Close to whole meters stays a plain integer (what X-Plane 11 reads too)
			const long nearest = std::lround(spacing);
			const double ratio = nearest == 0 ? 0.0 : spacing / static_cast<double>(nearest);
			if (ratio > 0.98 && ratio < 1.02)
				return static_cast<uint16_t>(nearest);
			return static_cast<uint16_t>(10000 + std::lround(128.0 * spacing));
		}
		return static_cast<uint16_t>(std::min(spacing, 65535.0));
	}

	uint16_t EncodeForestParameter(const float density, const ForestFill fill)
	{
		const int value = std::isfinite(density) ? std::clamp(static_cast<int>(static_cast<double>(density) * 255.0), 0, 255) : 0;
		return static_cast<uint16_t>(value + 256 * static_cast<int>(fill));
	}

	uint16_t EncodeAutogenParameter(const float height, const int low)
	{
		const long quarters = std::isfinite(height) ? std::clamp(std::lround(static_cast<double>(height) * 0.25), 0l, 255l) : 0l;
		return static_cast<uint16_t>((quarters << 8) + std::clamp(low, 0, 255));
	}

	std::string FormatExclusionBounds(const double west, const double south, const double east, const double north)
	{
		char text[128];
		std::snprintf(text, sizeof(text), "%.6f/%.6f/%.6f/%.6f", west, south, east, north);
		return text;
	}
}

// -------------------------------------------------------
