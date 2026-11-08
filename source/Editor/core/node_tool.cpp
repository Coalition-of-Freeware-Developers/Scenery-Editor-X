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
 * node_tool.cpp
 * -------------------------------------------------------
 * Created: 19/10/2026
 * -------------------------------------------------------
 */
#include "node_tool.h"
#include "airport_edit.h"
#include "edit_changes.h"
#include "editor_airports.h"
#include "editor_project.h"
#include "editor_selection.h"
#include "editor_ui.h"
#include "transform_tool.h"
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <functional>
#include <map>
#include <set>
#include <SceneryEditorX/xplane/xp_terrain_scene.h>

// -------------------------------------------------------

namespace SceneryEditorX::NodeTool
{
	using EditorSelection::Target;
	using EditorSelection::TargetType;
	using XPlane::AirportElementKind;
	using namespace Splines;

	namespace
	{
		constexpr float NODE_HALF = 4.5f;			// Node squares, pixels
		constexpr float NODE_HIT = 8.0f;
		constexpr float HANDLE_RADIUS = 4.0f;
		constexpr float HANDLE_HIT = 7.0f;
		constexpr float GRIP_OFFSET = 26.0f;		// How far from a node the grips that pull its handles out sit
		constexpr float GRIP_HALF = 3.0f;
		constexpr float EDGE_HIT = 6.0f;
		constexpr float DRAG_THRESHOLD = 3.0f;
		constexpr float SNAP_PIXELS = 10.0f;		// Dragged nodes snap onto nodes this close to the cursor
		constexpr float DASH_PIXELS = 5.0f;
		constexpr size_t MAX_CHAIN_MODES = 256;		// Curve modes remembered per chain
		constexpr uint32_t GIZMO_OWNER = 2;

		constexpr ImU32 COLOR_OUTLINE = IM_COL32(255, 154, 31, 235);
		constexpr ImU32 COLOR_OUTLINE_HOVER = IM_COL32(255, 214, 140, 255);
		constexpr ImU32 COLOR_NODE = IM_COL32(245, 245, 245, 255);
		constexpr ImU32 COLOR_NODE_SELECTED = IM_COL32(255, 154, 31, 255);
		constexpr ImU32 COLOR_NODE_BORDER = IM_COL32(20, 20, 22, 230);
		constexpr ImU32 COLOR_HANDLE = IM_COL32(120, 200, 255, 255);
		constexpr ImU32 COLOR_HANDLE_LINE = IM_COL32(170, 210, 240, 200);
		constexpr ImU32 COLOR_GRIP = IM_COL32(120, 200, 255, 150);
		constexpr ImU32 COLOR_CONTROL = IM_COL32(205, 160, 255, 255);	// B-spline control points and their polygon
		constexpr ImU32 COLOR_CAGE = IM_COL32(215, 185, 255, 235);
		constexpr ImU32 COLOR_CAGE_SHADOW = IM_COL32(25, 15, 40, 150);
		constexpr ImU32 COLOR_CURVE_NODE = IM_COL32(255, 214, 140, 220);
		constexpr ImU32 COLOR_SNAP = IM_COL32(90, 255, 140, 255);

		enum class Source : uint8_t
		{
			AirportShape,	// Pavement, linear feature, boundary
			Runway,			// Land runway: its two ends
			WaterRunway,
			ProjectItem
		};

		struct ScreenPoint
		{
			ImVec2 position;
			bool visible = false;
		};

		struct Session
		{
			Target target;
			Source source = Source::AirportShape;
			Shape shape;
			XPlane::GeoProjection projection;
			bool straight = false;		// No handles (runway ends, exclusion zones)
			bool pointsOnly = false;	// Nodes without edges (a forest of single trees)
			bool closable = false;		// Lines and strings: can open and close
			bool road = false;			// A road: its ends are junctions (road nodes) shared with other roads
			bool junction = false;		// A road node: one point the roads meeting at it follow
			bool reversed = false;		// Reversed since the last commit: a road's start and end swap too

			// Per frame
			std::vector<std::vector<Sample>> samples;
			bool resample = true;
			std::vector<std::vector<ScreenPoint>> sampleScreens;
			std::vector<std::vector<ScreenPoint>> pointScreens;	// What is edited: the nodes, or a B-spline's control points
			std::vector<std::vector<ScreenPoint>> curveScreens;	// A B-spline's nodes (on its curve; they follow the control points)
		};

		struct NodeKey
		{
			uint32_t session = 0;
			NodeRef ref;
			auto operator<=>(const NodeKey &) const = default;
		};

		enum class Drag : uint8_t
		{
			None,
			PendingNodes,	// Pressed on a node, not moved far enough yet
			Nodes,
			PendingEdge,	// Pressed on a curve: a click selects its two nodes, a drag bends it
			Bend,
			PendingHandle,	// Ctrl+click added a node: a drag pulls its handles out
			Handle,
			Gizmo,
			Property		// A value dragged in the PROPERTIES panel
		};

		struct Hit
		{
			enum class Type : uint8_t { None, Node, Handle, Grip, Edge } type = Type::None;
			uint32_t session = 0;
			NodeRef node;				// Node / Handle / Grip: the node; Edge: the segment's chain
			bool outgoing = false;		// Handle / Grip
			uint32_t segment = 0;		// Edge
			double t = 0.0;
		};

		/* @brief A chain's curve mode as last chosen (chains not listed are B-splines when their nodes form one). */
		struct ChainMode
		{
			Target target;
			uint32_t chain = 0;
			bool bspline = false;
		};

		struct State
		{
			std::vector<Session> sessions;
			uint64_t key = ~0ull;
			std::set<NodeKey> selected;
			std::vector<ChainMode> modes;

			Drag drag = Drag::None;
			Hit press;
			ImVec2 pressMouse;
			Point3 pressGround;
			bool pressGroundValid = false;
			bool pressAdd = false;
			bool pressToggle = false;
			bool breakSymmetry = false;
			std::vector<Shape> startShapes;		// Before the gesture: what a commit compares with, what Esc puts back
			std::vector<Shape> dragShapes;		// What the drag applies to (the start, plus a node Ctrl+click added)
			std::string gestureLabel;
			std::optional<Point2> snapTarget;	// Where a dragged node snapped onto another node (drawn)

			Hit hovered;
			std::string status;					// About the drag in progress
			std::string notice;					// About the last action, shown for a few seconds
			double noticeUntil = 0.0;
			std::optional<std::pair<ImVec2, ImVec2>> extendPreview;	// Ctrl held over the ground with a line end selected
		};

		State &S()
		{
			static State state;
			return state;
		}

		void SetNotice(State &s, std::string text)
		{
			s.notice = std::move(text);
			s.noticeUntil = ImGui::GetTime() + 4.0;
		}

		double Distance2(const ImVec2 &a, const ImVec2 &b)
		{
			const double dx = a.x - b.x, dy = a.y - b.y;
			return dx * dx + dy * dy;
		}

		double Distance(const Point2 &a, const Point2 &b)
		{
			return std::hypot(a.x - b.x, a.z - b.z);
		}

		/* @brief Squared distance from a point to a segment and where along it (0..1) the nearest point is. */
		double SegmentDistance2(const ImVec2 &p, const ImVec2 &a, const ImVec2 &b, double &along)
		{
			const double dx = b.x - a.x, dy = b.y - a.y;
			const double length2 = dx * dx + dy * dy;
			along = length2 > 1e-9 ? std::clamp(((p.x - a.x) * dx + (p.y - a.y) * dy) / length2, 0.0, 1.0) : 0.0;
			const ImVec2 nearest(static_cast<float>(a.x + dx * along), static_cast<float>(a.y + dy * along));
			return Distance2(p, nearest);
		}

		/* @brief True heading (degrees clockwise from north) of a ground vector (+X east, -Z north). */
		double HeadingOf(const Point2 &vector)
		{
			const double heading = std::atan2(vector.x, -vector.z) * 57.29577951308232;
			return heading < 0.0 ? heading + 360.0 : heading;
		}

		Point2 FromHeading(const double heading, const double length)
		{
			const double radians = heading / 57.29577951308232;
			return { std::sin(radians) * length, -std::cos(radians) * length };
		}

		uint64_t CurrentKey()
		{
			uint64_t key = EditorSelection::GetRevision() * 0x9E3779B97F4A7C15ull;
			if (const XPlane::AirportDocument *airport = EditorAirports::GetDocument())
				key ^= airport->GetRevision() * 0xC2B2AE3D27D4EB4Full;
			if (ProjectFile::ProjectDocument *project = EditorProjects::GetDocument())
				key ^= project->GetItemsRevision() * 0x165667B19E3779F9ull;
			return key;
		}

		/* @brief Whether a project item's rings close (polygons) or run open (lines, strings unless closed, tree lines). */
		bool IsClosed(const ProjectFile::SceneryItem &item)
		{
			using ProjectFile::ItemKind;
			switch (item.kind)
			{
				case ItemKind::Line:
				case ItemKind::String:			return item.closed;
				case ItemKind::Forest:			return item.fill == ProjectFile::ForestFill::Area;
				case ItemKind::ExclusionZone:
				case ItemKind::Road:			return false;
				case ItemKind::Facade:
				{
					// RING 0 facades (fences, hangars built along a line, doors) are open runs of walls
					const XPlane::FacadeAsset *asset = EditorProjects::GetFacade(item.resource);
					return !asset || asset->ring;
				}
				default:						return true;
			}
		}

		Chain RunwayChain(const XPlane::GeoProjection &projection, const double lat0, const double lon0, const double lat1, const double lon1)
		{
			Chain chain;
			chain.closed = false;
			for (const auto &[latitude, longitude] : { std::pair{ lat0, lon0 }, std::pair{ lat1, lon1 } })
			{
				Node &node = chain.nodes.emplace_back();
				node.position = projection.ToLocal(latitude, longitude);
				node.sourcePosition.local = node.position;
				node.sourcePosition.latitude = latitude;
				node.sourcePosition.longitude = longitude;
				node.sourcePosition.valid = true;
			}
			return chain;
		}

		std::optional<Session> Open(const Target &target)
		{
			Session session;
			session.target = target;
			if (target.type == TargetType::AirportElement)
			{
				const XPlane::AirportDocument *document = EditorAirports::GetDocument();
				if (!document)
					return std::nullopt;
				session.projection = XPlane::SceneAirports::GetProjection();
				const XPlane::AptAirport &airport = document->airport;
				if (AirportEdit::IsShapeKind(target.kind))
				{
					const std::vector<XPlane::AptNodeChain> *chains = AirportEdit::GetChains(airport, target.kind, target.index);
					if (!chains || chains->empty())
						return std::nullopt;
					session.source = Source::AirportShape;
					session.shape = FromAptChains(*chains, session.projection);
					session.closable = target.kind == AirportElementKind::LinearFeature && session.shape.chains.size() == 1;
					return session;
				}
				if (target.kind == AirportElementKind::Runway && target.index < airport.runways.size())
				{
					const XPlane::AptRunway &runway = airport.runways[target.index];
					session.source = Source::Runway;
					session.straight = true;
					session.shape.chains.push_back(RunwayChain(session.projection, runway.ends[0].latitude, runway.ends[0].longitude, runway.ends[1].latitude, runway.ends[1].longitude));
					return session;
				}
				if (target.kind == AirportElementKind::WaterRunway && target.index < airport.waterRunways.size())
				{
					const XPlane::AptWaterRunway &runway = airport.waterRunways[target.index];
					session.source = Source::WaterRunway;
					session.straight = true;
					session.shape.chains.push_back(RunwayChain(session.projection, runway.ends[0].latitude, runway.ends[0].longitude, runway.ends[1].latitude, runway.ends[1].longitude));
					return session;
				}
				return std::nullopt;
			}

			if (target.type == TargetType::ProjectItem)
			{
				ProjectFile::ProjectDocument *document = EditorProjects::GetDocument();
				const ProjectFile::SceneryItem *item = document ? document->FindItem(target.itemId) : nullptr;
				using ProjectFile::ItemKind;
				if (item && !item->locked && item->kind == ItemKind::RoadNode)
				{
					// A junction: one point; the roads meeting there follow it when it is written
					session.source = Source::ProjectItem;
					session.projection = EditorProjects::GetProjection();
					session.straight = true;
					session.pointsOnly = true;
					session.junction = true;
					Chain chain;
					chain.closed = false;
					Node &node = chain.nodes.emplace_back();
					node.position = session.projection.ToLocal(item->latitude, item->longitude);
					node.sourcePosition = { node.position, item->latitude, item->longitude, true };
					session.shape.chains.push_back(std::move(chain));
					return session;
				}
				if (!item || item->locked || !ProjectFile::HasNodes(item->kind) || item->rings.empty())
					return std::nullopt;
				session.source = Source::ProjectItem;
				session.projection = EditorProjects::GetProjection();
				session.road = item->kind == ItemKind::Road;
				session.shape = FromGeoRings(item->rings, IsClosed(*item), session.projection);
				session.straight = item->kind == ItemKind::ExclusionZone;
				session.pointsOnly = item->kind == ItemKind::Forest && item->fill == ProjectFile::ForestFill::Points;
				session.closable = (item->kind == ItemKind::Line || item->kind == ItemKind::String) && item->rings.size() == 1;
				return session;
			}
			return std::nullopt;
		}

		const ChainMode *FindMode(const State &s, const Target &target, const uint32_t chain)
		{
			for (const ChainMode &mode : s.modes)
				if (mode.chain == chain && mode.target.SameAs(target))
					return &mode;
			return nullptr;
		}

		void SetMode(State &s, const Target &target, const uint32_t chain, const bool bspline)
		{
			std::erase_if(s.modes, [&](const ChainMode &mode) { return mode.chain == chain && mode.target.SameAs(target); });
			s.modes.push_back({ target, chain, bspline });
			if (s.modes.size() > MAX_CHAIN_MODES)
				s.modes.erase(s.modes.begin());
		}

		void ForgetMode(State &s, const Target &target, const uint32_t chain)
		{
			std::erase_if(s.modes, [&](const ChainMode &mode) { return mode.chain == chain && mode.target.SameAs(target); });
		}

		/* @brief Follows the selection and the documents (undo, other panels), keeping the node selection where it still fits. */
		void Reload(State &s)
		{
			const uint64_t key = CurrentKey();
			if (key == s.key || s.drag != Drag::None)
				return;
			s.key = key;

			std::vector<Session> sessions;
			for (const Target &target : EditorSelection::Get())
			{
				if (std::optional<Session> session = Open(target))
					sessions.push_back(std::move(*session));
			}

			for (Session &session : sessions)
			{
				const Session *old = nullptr;
				for (const Session &candidate : s.sessions)
				{
					if (candidate.target.SameAs(session.target))
					{
						old = &candidate;
						break;
					}
				}
				for (uint32_t c = 0; c < session.shape.chains.size(); ++c)
				{
					Chain &chain = session.shape.chains[c];

					// The node types picked here outlive the reload (the formats store handles, not types)
					if (old && c < old->shape.chains.size() && old->shape.chains[c].nodes.size() == chain.nodes.size())
						for (size_t n = 0; n < chain.nodes.size(); ++n)
							chain.nodes[n].preferred = old->shape.chains[c].nodes[n].preferred;

					// B-splines are recognized from their nodes (unless the chain was put back to bezier editing)
					if (session.straight || session.pointsOnly)
						continue;
					const ChainMode *mode = FindMode(s, session.target, c);
					if (mode && !mode->bspline)
						continue;
					if (!BeginBSpline(chain, BSplineFit::NodesAsControls, true) && mode)
						ForgetMode(s, session.target, c);	// No longer one (undone, or edited elsewhere)
				}
			}

			std::set<NodeKey> kept;
			for (const NodeKey &selected : s.selected)
			{
				if (selected.session >= s.sessions.size())
					continue;
				const Target &old = s.sessions[selected.session].target;
				for (uint32_t i = 0; i < sessions.size(); ++i)
				{
					if (sessions[i].target.SameAs(old) && sessions[i].shape.IsValid(selected.ref.chain, selected.ref.node))
						kept.insert({ i, selected.ref });
				}
			}
			s.sessions = std::move(sessions);
			s.selected = std::move(kept);
		}

		std::vector<NodeRef> SelectedIn(const State &s, const uint32_t session)
		{
			std::vector<NodeRef> refs;
			for (const NodeKey &key : s.selected)
				if (key.session == session)
					refs.push_back(key.ref);
			return refs;
		}

		const Node *GetNode(const State &s, const uint32_t session, const NodeRef &ref)
		{
			if (session >= s.sessions.size() || !s.sessions[session].shape.IsValid(ref.chain, ref.node))
				return nullptr;
			return &s.sessions[session].shape.chains[ref.chain].nodes[ref.node];
		}

		/* @brief The selected node's chain, when it is edited with handles (not a B-spline, runway or tree points). */
		const Chain *GetBezierChain(const State &s, const NodeKey &key)
		{
			if (!GetNode(s, key.session, key.ref))
				return nullptr;
			const Session &session = s.sessions[key.session];
			const Chain &chain = session.shape.chains[key.ref.chain];
			return session.straight || session.pointsOnly || chain.IsBSpline() ? nullptr : &chain;
		}

		ScreenPoint Project(const ViewportView &view, const Point2 &point)
		{
			ScreenPoint screen;
			screen.visible = view.WorldToScreen(Point3{ point.x, XPlane::SceneTerrain::GetHeight(point.x, point.z), point.z }, screen.position);
			return screen;
		}

		void ProjectSessions(State &s, const ViewportView &view)
		{
			for (Session &session : s.sessions)
			{
				const size_t chains = session.shape.chains.size();
				if (session.resample || session.samples.size() != chains)
				{
					session.samples.resize(chains);
					for (size_t c = 0; c < chains; ++c)
						SampleChain(session.shape.chains[c], session.samples[c], 1.0);
					session.resample = false;
				}
				session.sampleScreens.resize(chains);
				session.pointScreens.resize(chains);
				session.curveScreens.resize(chains);
				for (size_t c = 0; c < chains; ++c)
				{
					const std::vector<Sample> &samples = session.samples[c];
					std::vector<ScreenPoint> &sampleScreens = session.sampleScreens[c];
					sampleScreens.resize(samples.size());
					for (size_t i = 0; i < samples.size(); ++i)
						sampleScreens[i] = Project(view, samples[i].position);

					const Chain &chain = session.shape.chains[c];
					std::vector<ScreenPoint> &pointScreens = session.pointScreens[c];
					pointScreens.resize(chain.nodes.size());
					for (uint32_t i = 0; i < chain.nodes.size(); ++i)
						pointScreens[i] = Project(view, chain.GetEditPoint(i));

					std::vector<ScreenPoint> &curveScreens = session.curveScreens[c];
					curveScreens.clear();
					if (chain.IsBSpline())
						for (const Node &node : chain.nodes)
							curveScreens.push_back(Project(view, node.position));
				}
			}
		}

		/* @brief Where the grip for pulling a missing handle out of a selected node sits (toward the neighbor on that side). */
		bool GripScreen(const State &s, const ViewportView &view, const NodeKey &key, const bool outgoing, ImVec2 &out)
		{
			const Chain *chain = GetBezierChain(s, key);
			if (!chain)
				return false;
			const Node &node = chain->nodes[key.ref.node];
			if (outgoing ? node.hasOut : node.hasIn)
				return false;
			const uint32_t count = static_cast<uint32_t>(chain->nodes.size());
			const uint32_t index = key.ref.node;
			const bool hasNeighbor = outgoing ? (index + 1 < count || (chain->closed && count > 2)) : (index > 0 || (chain->closed && count > 2));
			if (!hasNeighbor)
				return false;
			const uint32_t neighbor = outgoing ? (index + 1) % count : (index > 0 ? index - 1 : count - 1);
			const ScreenPoint a = Project(view, node.position);
			const ScreenPoint b = Project(view, chain->nodes[neighbor].position);
			if (!a.visible || !b.visible)
				return false;
			const float dx = b.position.x - a.position.x, dy = b.position.y - a.position.y;
			const float length = std::sqrt(dx * dx + dy * dy);
			if (length < 2.0f * NODE_HIT)
				return false;
			const float reach = std::min(GRIP_OFFSET, length * 0.4f);
			out = ImVec2(a.position.x + dx / length * reach, a.position.y + dy / length * reach);
			return true;
		}

		Hit HitTest(const State &s, const ViewportView &view, const ImVec2 &mouse)
		{
			Hit hit;

			// Handles and grips of selected nodes win: they sit on top
			double best = HANDLE_HIT * HANDLE_HIT;
			for (const NodeKey &key : s.selected)
			{
				if (!GetBezierChain(s, key))
					continue;
				const Node &node = *GetNode(s, key.session, key.ref);
				for (const bool outgoing : { false, true })
				{
					ImVec2 at;
					Hit::Type type = Hit::Type::Handle;
					if (outgoing ? node.hasOut : node.hasIn)
					{
						const ScreenPoint screen = Project(view, outgoing ? node.handleOut : node.handleIn);
						if (!screen.visible)
							continue;
						at = screen.position;
					}
					else if (GripScreen(s, view, key, outgoing, at))
						type = Hit::Type::Grip;
					else
						continue;
					const double d = Distance2(mouse, at);
					if (d < best)
					{
						best = d;
						hit.type = type;
						hit.session = key.session;
						hit.node = key.ref;
						hit.outgoing = outgoing;
					}
				}
			}
			if (hit.type != Hit::Type::None)
				return hit;

			best = NODE_HIT * NODE_HIT;
			for (uint32_t i = 0; i < s.sessions.size(); ++i)
			{
				const Session &session = s.sessions[i];
				for (uint32_t c = 0; c < session.pointScreens.size(); ++c)
				{
					for (uint32_t n = 0; n < session.pointScreens[c].size(); ++n)
					{
						const ScreenPoint &screen = session.pointScreens[c][n];
						const double d = screen.visible ? Distance2(mouse, screen.position) : 1e30;
						if (d < best)
						{
							best = d;
							hit.type = Hit::Type::Node;
							hit.session = i;
							hit.node = { c, n };
						}
					}
				}
			}
			if (hit.type != Hit::Type::None)
				return hit;

			best = EDGE_HIT * EDGE_HIT;
			for (uint32_t i = 0; i < s.sessions.size(); ++i)
			{
				const Session &session = s.sessions[i];
				if (session.pointsOnly)
					continue;
				for (uint32_t c = 0; c < session.sampleScreens.size(); ++c)
				{
					const std::vector<ScreenPoint> &screens = session.sampleScreens[c];
					const std::vector<Sample> &samples = session.samples[c];
					for (size_t k = 0; k + 1 < screens.size(); ++k)
					{
						if (!screens[k].visible || !screens[k + 1].visible)
							continue;
						double along = 0.0;
						const double d = SegmentDistance2(mouse, screens[k].position, screens[k + 1].position, along);
						if (d >= best)
							continue;
						best = d;
						hit.type = Hit::Type::Edge;
						hit.session = i;
						hit.node = { c, samples[k].segment };
						hit.segment = samples[k].segment;
						const double t1 = samples[k + 1].segment == samples[k].segment ? samples[k + 1].t : 1.0;
						hit.t = samples[k].t + (t1 - samples[k].t) * along;
					}
				}
			}
			return hit;
		}

		bool Snapping()
		{
			return TransformTool::GetSettings().snap != ImGui::GetIO().KeyCtrl;
		}

		void StartGesture(State &s)
		{
			s.startShapes.clear();
			for (const Session &session : s.sessions)
				s.startShapes.push_back(session.shape);
			s.dragShapes = s.startShapes;
			s.gestureLabel.clear();
		}

		void RestoreGesture(State &s)
		{
			for (size_t i = 0; i < s.sessions.size() && i < s.startShapes.size(); ++i)
			{
				s.sessions[i].shape = s.startShapes[i];
				s.sessions[i].resample = true;
			}
		}

		/**
		 * @brief Roads share their ends at road nodes: a junction that moved (a road node dragged, or a road's end)
		 * takes the node and every road meeting there along. The items it changes join the change set (one undo step).
		 */
		void FollowJunctions(ProjectFile::ProjectDocument &document, EditChanges::ChangeSet &changes)
		{
			using ProjectFile::ItemKind;
			using ProjectFile::GeoNode;
			std::map<std::string, std::pair<double, double>> moved;
			for (const EditChanges::ItemChange &change : changes.items)
			{
				const ProjectFile::SceneryItem &before = change.before;
				const ProjectFile::SceneryItem &after = change.after;
				if (after.kind == ItemKind::RoadNode)
				{
					if (after.latitude != before.latitude || after.longitude != before.longitude)
						moved[after.id] = { after.latitude, after.longitude };
					continue;
				}
				if (after.kind != ItemKind::Road || after.rings.empty() || before.rings.empty() || after.rings[0].empty() || before.rings[0].empty())
					continue;
				for (const bool start : { true, false })
				{
					const GeoNode &now = start ? after.rings[0].front() : after.rings[0].back();
					const GeoNode &was = start ? before.rings[0].front() : before.rings[0].back();
					const std::string &node = start ? after.startNode : after.endNode;
					if (!node.empty() && (now.latitude != was.latitude || now.longitude != was.longitude))
						moved[node] = { now.latitude, now.longitude };
				}
			}

			const auto record = [&changes](const ProjectFile::SceneryItem &item, const ProjectFile::SceneryItem &before)
			{
				for (EditChanges::ItemChange &change : changes.items)
				{
					if (change.after.id == item.id)
					{
						change.after = item;
						return;
					}
				}
				changes.items.push_back({ before, item });
			};
			for (const auto &[id, position] : moved)
			{
				if (ProjectFile::SceneryItem *node = document.FindItem(id); node && (node->latitude != position.first || node->longitude != position.second))
				{
					const ProjectFile::SceneryItem before = *node;
					node->latitude = position.first;
					node->longitude = position.second;
					record(*node, before);
				}
				for (const auto &[road, start] : ProjectFile::FindRoadsAt(document.GetProject(), id))
				{
					if (road->rings.empty() || road->rings[0].empty())
						continue;
					GeoNode &end = start ? road->rings[0].front() : road->rings[0].back();
					if (end.latitude == position.first && end.longitude == position.second)
						continue;
					const ProjectFile::SceneryItem before = *road;
					end.latitude = position.first;	// Its handle is an offset: it travels along
					end.longitude = position.second;
					record(*road, before);
				}
			}
		}

		/* @brief Writes the sessions that changed back into their rows / items as one undo step. */
		void Commit(State &s, const std::string &label, const std::vector<uint32_t> &sessions)
		{
			XPlane::AirportDocument *airport = EditorAirports::GetDocument();
			ProjectFile::ProjectDocument *project = EditorProjects::GetDocument();
			EditChanges::ChangeSet changes = EditChanges::Begin();

			for (const uint32_t index : sessions)
			{
				if (index >= s.sessions.size())
					continue;
				const Session &session = s.sessions[index];
				const Target &target = session.target;
				switch (session.source)
				{
					case Source::AirportShape:
					{
						std::vector<XPlane::AptNodeChain> *chains = airport ? AirportEdit::GetChains(airport->airport, target.kind, target.index) : nullptr;
						if (!chains)
							break;
						AirportEdit::Snapshot before = AirportEdit::Capture(airport->airport, target.kind, target.index);
						*chains = ToAptChains(session.shape, session.projection);
						changes.rows.push_back({ target.kind, target.index, std::move(before), AirportEdit::Capture(airport->airport, target.kind, target.index) });
						break;
					}
					case Source::Runway:
					case Source::WaterRunway:
					{
						if (!airport || session.shape.chains.empty() || session.shape.chains[0].nodes.size() != 2)
							break;
						AirportEdit::Snapshot before = AirportEdit::Capture(airport->airport, target.kind, target.index);
						for (size_t e = 0; e < 2; ++e)
						{
							const Node &node = session.shape.chains[0].nodes[e];
							double latitude = node.sourcePosition.latitude, longitude = node.sourcePosition.longitude;
							if (!node.sourcePosition.valid || node.position.x != node.sourcePosition.local.x || node.position.z != node.sourcePosition.local.z)
								session.projection.ToGeo(node.position, latitude, longitude);
							if (session.source == Source::Runway && target.index < airport->airport.runways.size())
							{
								airport->airport.runways[target.index].ends[e].latitude = latitude;
								airport->airport.runways[target.index].ends[e].longitude = longitude;
							}
							else if (session.source == Source::WaterRunway && target.index < airport->airport.waterRunways.size())
							{
								airport->airport.waterRunways[target.index].ends[e].latitude = latitude;
								airport->airport.waterRunways[target.index].ends[e].longitude = longitude;
							}
						}
						changes.rows.push_back({ target.kind, target.index, std::move(before), AirportEdit::Capture(airport->airport, target.kind, target.index) });
						break;
					}
					case Source::ProjectItem:
					{
						ProjectFile::SceneryItem *item = project ? project->FindItem(target.itemId) : nullptr;
						if (!item)
							break;
						ProjectFile::SceneryItem before = *item;
						if (session.junction)
						{
							if (session.shape.chains.empty() || session.shape.chains[0].nodes.empty())
								break;
							const Node &node = session.shape.chains[0].nodes[0];
							if (!node.sourcePosition.valid || node.position.x != node.sourcePosition.local.x || node.position.z != node.sourcePosition.local.z)
								session.projection.ToGeo(node.position, item->latitude, item->longitude);
						}
						else
							item->rings = ToGeoRings(session.shape, session.projection);
						if (session.road && session.reversed)
						{
							std::swap(item->startNode, item->endNode);
							std::swap(item->startLayer, item->endLayer);
						}
						if (session.closable && !session.shape.chains.empty())
							item->closed = session.shape.chains[0].closed;
						changes.items.push_back({ std::move(before), *item });
						break;
					}
				}
			}

			if (project)
				FollowJunctions(*project, changes);
			if (changes.IsEmpty())
				return;
			EditChanges::Publish(changes);
			EditChanges::Record(label, std::move(changes));
		}

		std::vector<uint32_t> SessionsWithSelection(const State &s)
		{
			std::vector<uint32_t> sessions;
			for (const NodeKey &key : s.selected)
				if (sessions.empty() || sessions.back() != key.session)
					sessions.push_back(key.session);
			return sessions;
		}

		std::vector<uint32_t> ChangedSessions(const State &s)
		{
			std::vector<uint32_t> changed;
			for (uint32_t i = 0; i < s.sessions.size() && i < s.startShapes.size(); ++i)
			{
				const Shape &a = s.sessions[i].shape;
				const Shape &b = s.startShapes[i];
				bool same = a.chains.size() == b.chains.size();
				for (size_t c = 0; same && c < a.chains.size(); ++c)
				{
					same = a.chains[c].nodes.size() == b.chains[c].nodes.size() && a.chains[c].closed == b.chains[c].closed;
					for (size_t n = 0; same && n < a.chains[c].nodes.size(); ++n)
					{
						const Node &x = a.chains[c].nodes[n];
						const Node &y = b.chains[c].nodes[n];
						same = x.position.x == y.position.x && x.position.z == y.position.z && x.hasIn == y.hasIn && x.hasOut == y.hasOut && x.split == y.split &&
							(!x.hasIn || (x.handleIn.x == y.handleIn.x && x.handleIn.z == y.handleIn.z)) && (!x.hasOut || (x.handleOut.x == y.handleOut.x && x.handleOut.z == y.handleOut.z));
					}
				}
				if (!same)
					changed.push_back(i);
			}
			return changed;
		}

		/* @brief Edit points of every session other than the dragged ones, nearest to the cursor within snapping distance. */
		std::optional<Point2> FindSnapNode(const State &s, const ImVec2 &mouse)
		{
			double best = SNAP_PIXELS * SNAP_PIXELS;
			std::optional<Point2> found;
			for (uint32_t i = 0; i < s.sessions.size(); ++i)
			{
				const Session &session = s.sessions[i];
				for (uint32_t c = 0; c < session.pointScreens.size(); ++c)
				{
					for (uint32_t n = 0; n < session.pointScreens[c].size(); ++n)
					{
						if (s.selected.contains({ i, { c, n } }))
							continue;
						const ScreenPoint &screen = session.pointScreens[c][n];
						const double d = screen.visible ? Distance2(mouse, screen.position) : 1e30;
						if (d < best && i < s.dragShapes.size() && s.dragShapes[i].IsValid(c, n))
						{
							best = d;
							found = s.dragShapes[i].chains[c].GetEditPoint(n);
						}
					}
				}
			}
			return found;
		}

		void DragNodes(State &s, const ViewportView &view, const ImVec2 &mouse)
		{
			Point3 ground;
			if (!s.pressGroundValid || !view.RayToGround(mouse, ground) || s.press.session >= s.dragShapes.size())
				return;

			// The node under the press leads; the others keep their offsets to it
			Point2 lead;
			const Shape &start = s.dragShapes[s.press.session];
			if (s.press.type == Hit::Type::Edge && start.IsValid(s.press.node.chain, s.press.segment))
				lead = start.chains[s.press.node.chain].GetEditPoint(s.press.segment);
			else if (start.IsValid(s.press.node.chain, s.press.node.node))
				lead = start.chains[s.press.node.chain].GetEditPoint(s.press.node.node);
			Point2 target{ lead.x + ground.x - s.pressGround.x, lead.z + ground.z - s.pressGround.z };

			s.snapTarget.reset();
			if (s.press.type == Hit::Type::Node)
			{
				if (const std::optional<Point2> snap = FindSnapNode(s, mouse))
				{
					target = *snap;
					s.snapTarget = snap;
				}
			}
			if (!s.snapTarget && Snapping())
			{
				const double step = std::max(0.01, static_cast<double>(TransformTool::GetSettings().snapMove));
				target = { std::round(target.x / step) * step, std::round(target.z / step) * step };
			}

			const Affine2 move = Affine2::Translation(target.x - lead.x, target.z - lead.z);
			for (uint32_t i = 0; i < s.sessions.size(); ++i)
			{
				s.sessions[i].shape = s.dragShapes[i];
				TransformNodes(s.sessions[i].shape, SelectedIn(s, i), move);
				s.sessions[i].resample = true;
			}
			char text[128];
			snprintf(text, sizeof(text), "Move %zu %s: %.2f m", s.selected.size(), s.selected.size() == 1 ? "point" : "points", std::hypot(target.x - lead.x, target.z - lead.z));
			s.status = text;
		}

		void DragHandle(State &s, const ViewportView &view, const ImVec2 &mouse)
		{
			Point3 ground;
			if (!view.RayToGround(mouse, ground) || s.press.session >= s.sessions.size())
				return;
			Session &session = s.sessions[s.press.session];
			session.shape = s.dragShapes[s.press.session];
			SetHandle(session.shape, s.press.node, s.press.outgoing, { ground.x, ground.z }, s.breakSymmetry);
			session.resample = true;
			if (const Node *node = GetNode(s, s.press.session, s.press.node))
			{
				const Point2 &handle = s.press.outgoing ? node->handleOut : node->handleIn;
				char text[160];
				snprintf(text, sizeof(text), "%s handle: %.2f m, heading %.1f deg (%s)%s", s.press.outgoing ? "Outgoing" : "Incoming", Distance(handle, node->position),
					HeadingOf({ handle.x - node->position.x, handle.z - node->position.z }), GetNodeTypeName(GetNodeType(*node)), s.breakSymmetry ? "" : "  |  Shift: this side only");
				s.status = text;
			}
		}

		void DragBend(State &s, const ViewportView &view, const ImVec2 &mouse)
		{
			Point3 ground;
			if (!s.pressGroundValid || !view.RayToGround(mouse, ground) || s.press.session >= s.sessions.size())
				return;
			Session &session = s.sessions[s.press.session];
			session.shape = s.dragShapes[s.press.session];
			const uint32_t chain = s.press.node.chain;
			if (chain >= session.shape.chains.size())
				return;

			// The grabbed point of the curve follows the mouse from where it was grabbed (no jump on the first move)
			const Point2 grabbed = GetSegment(session.shape.chains[chain], s.press.segment).Evaluate(std::clamp(s.press.t, 0.05, 0.95));
			const Point2 target{ grabbed.x + ground.x - s.pressGround.x, grabbed.z + ground.z - s.pressGround.z };
			BendSegment(session.shape, chain, s.press.segment, s.press.t, target);
			session.resample = true;
			const bool spline = session.shape.chains[chain].IsBSpline();
			char text[160];
			snprintf(text, sizeof(text), "Bend the curve: %.2f m%s", std::hypot(target.x - grabbed.x, target.z - grabbed.z),
				spline ? " (control points follow)" : "  |  smooth nodes stay smooth, corners bend this side only");
			s.status = text;
		}

		/* @brief The single selected node, when it is an end of an open line that can grow. */
		struct LineEnd
		{
			uint32_t session = 0;
			uint32_t chain = 0;
			bool atStart = false;
			uint32_t other = 0;		// The chain's other end
		};

		/* @brief Whether a road's start (or end) junction is shared with other roads. */
		bool JoinsOtherRoads(const Session &session, const bool start)
		{
			ProjectFile::ProjectDocument *document = EditorProjects::GetDocument();
			const ProjectFile::SceneryItem *item = document ? document->FindItem(session.target.itemId) : nullptr;
			if (!item)
				return false;
			const std::string &node = start ? item->startNode : item->endNode;
			for (const auto &[road, atStart] : ProjectFile::FindRoadsAt(document->GetProject(), node))
				if (road->id != item->id)
					return true;
			return false;
		}

		std::optional<LineEnd> SelectedEnd(const State &s)
		{
			if (s.selected.size() != 1)
				return std::nullopt;
			const NodeKey &key = *s.selected.begin();
			if (!GetNode(s, key.session, key.ref))
				return std::nullopt;
			const Session &session = s.sessions[key.session];
			const Chain &chain = session.shape.chains[key.ref.chain];
			const uint32_t count = static_cast<uint32_t>(chain.nodes.size());
			if (session.straight || chain.closed || count < 2 || (key.ref.node != 0 && key.ref.node + 1 != count))
				return std::nullopt;
			if (session.source == Source::AirportShape && session.target.kind != AirportElementKind::LinearFeature)
				return std::nullopt;
			if (session.road && JoinsOtherRoads(session, key.ref.node == 0))
				return std::nullopt;	// Extending would pull the other roads' ends along
			LineEnd end;
			end.session = key.session;
			end.chain = key.ref.chain;
			end.atStart = key.ref.node == 0;
			end.other = end.atStart ? count - 1 : 0;
			return end;
		}

		/* @brief Ctrl+click with a line's end selected: a new node where clicked (or, on the line's other end, closing it). */
		bool TryExtend(State &s, const ViewportView &view, const ImVec2 &mouse)
		{
			const std::optional<LineEnd> end = SelectedEnd(s);
			if (!end)
				return false;
			Session &session = s.sessions[end->session];

			if (s.hovered.type == Hit::Type::Node && s.hovered.session == end->session && s.hovered.node == NodeRef{ end->chain, end->other })
			{
				if (!session.closable)
					return false;
				StartGesture(s);
				if (!CloseChain(session.shape, end->chain))
					return false;
				session.resample = true;
				s.selected.clear();
				Commit(s, "Close line", { end->session });
				return true;
			}
			if (s.hovered.type != Hit::Type::None)
				return false;	// Over something else: the usual Ctrl+click

			Point3 ground;
			if (!view.RayToGround(mouse, ground))
				return false;
			StartGesture(s);
			NodeRef added;
			if (!AppendNode(session.shape, end->chain, end->atStart, { ground.x, ground.z }, &added))
				return false;
			session.resample = true;
			s.dragShapes[end->session] = session.shape;
			s.gestureLabel = "Extend line";
			s.selected = { NodeKey{ end->session, added } };
			s.press = {};
			s.press.session = end->session;
			s.press.node = added;
			s.press.outgoing = !end->atStart;	// Dragging pulls the handle that leads on, in the direction the line grows
			s.pressMouse = mouse;
			s.pressGround = ground;
			s.pressGroundValid = true;
			s.breakSymmetry = ImGui::GetIO().KeyShift;
			const bool spline = session.shape.chains[end->chain].IsBSpline();
			s.press.type = spline ? Hit::Type::Node : Hit::Type::Handle;
			s.drag = spline ? Drag::PendingNodes : Drag::PendingHandle;
			return true;
		}

		void AddDashedLine(ImDrawList *drawList, const ImVec2 &a, const ImVec2 &b, const ImU32 color)
		{
			const float dx = b.x - a.x, dy = b.y - a.y;
			const float length = std::sqrt(dx * dx + dy * dy);
			if (length < 1.0f)
				return;
			if (length > 4000.0f)
			{
				drawList->AddLine(a, b, color, 1.0f);
				return;
			}
			const float ux = dx / length, uy = dy / length;
			for (float d = 0.0f; d < length; d += DASH_PIXELS * 2.0f)
			{
				const float e = std::min(length, d + DASH_PIXELS);
				drawList->AddLine(ImVec2(a.x + ux * d, a.y + uy * d), ImVec2(a.x + ux * e, a.y + uy * e), color, 1.2f);
			}
		}

		void AddDiamond(ImDrawList *drawList, const ImVec2 &c, const float half, const ImU32 fill, const ImU32 border)
		{
			const ImVec2 top(c.x, c.y - half), right(c.x + half, c.y), bottom(c.x, c.y + half), left(c.x - half, c.y);
			drawList->AddQuadFilled(top, right, bottom, left, fill);
			drawList->AddQuad(top, right, bottom, left, border, 1.2f);
		}

		void Draw(const State &s, const ViewportView &view)
		{
			ImDrawList *drawList = ImGui::GetWindowDrawList();
			const ImVec2 imageMax(view.imageMin.x + view.imageSize.x, view.imageMin.y + view.imageSize.y);
			drawList->PushClipRect(view.imageMin, imageMax, true);

			// Outlines (the edited shape, as it will be)
			std::vector<ImVec2> line;
			for (uint32_t i = 0; i < s.sessions.size(); ++i)
			{
				const Session &session = s.sessions[i];
				if (session.pointsOnly)
					continue;
				for (uint32_t c = 0; c < session.sampleScreens.size(); ++c)
				{
					const bool hovered = s.hovered.type == Hit::Type::Edge && s.hovered.session == i && s.hovered.node.chain == c;
					const ImU32 color = hovered ? COLOR_OUTLINE_HOVER : COLOR_OUTLINE;
					line.clear();
					for (const ScreenPoint &point : session.sampleScreens[c])
					{
						if (point.visible)
						{
							line.push_back(point.position);
							continue;
						}
						if (line.size() > 1)
							drawList->AddPolyline(line.data(), static_cast<int>(line.size()), color, ImDrawFlags_None, 2.0f);
						line.clear();
					}
					if (line.size() > 1)
						drawList->AddPolyline(line.data(), static_cast<int>(line.size()), color, ImDrawFlags_None, 2.0f);
				}
			}

			// B-splines: the control polygon (dashed) and the nodes on the curve
			for (const Session &session : s.sessions)
			{
				for (uint32_t c = 0; c < session.pointScreens.size() && c < session.shape.chains.size(); ++c)
				{
					const Chain &chain = session.shape.chains[c];
					if (!chain.IsBSpline())
						continue;
					const std::vector<ScreenPoint> &points = session.pointScreens[c];
					const size_t edges = chain.closed ? points.size() : points.size() - 1;
					for (size_t e = 0; e < edges && points.size() > 1; ++e)
					{
						const ScreenPoint &a = points[e];
						const ScreenPoint &b = points[(e + 1) % points.size()];
						if (a.visible && b.visible)
						{
							// A dark line under the dashes keeps them readable on light ground
							drawList->AddLine(a.position, b.position, COLOR_CAGE_SHADOW, 3.0f);
							AddDashedLine(drawList, a.position, b.position, COLOR_CAGE);
						}
					}
					for (const ScreenPoint &point : session.curveScreens[c])
					{
						if (!point.visible)
							continue;
						drawList->AddCircleFilled(point.position, 3.0f, COLOR_NODE_BORDER);
						drawList->AddCircleFilled(point.position, 2.0f, COLOR_CURVE_NODE);
					}
				}
			}

			// Handles of selected nodes, and grips where a handle could be pulled out
			for (const NodeKey &key : s.selected)
			{
				if (!GetBezierChain(s, key))
					continue;
				const Node &node = *GetNode(s, key.session, key.ref);
				const ScreenPoint center = Project(view, node.position);
				for (const bool outgoing : { false, true })
				{
					const bool hovered = (s.hovered.type == Hit::Type::Handle || s.hovered.type == Hit::Type::Grip) && s.hovered.session == key.session &&
						s.hovered.node == key.ref && s.hovered.outgoing == outgoing;
					if (!(outgoing ? node.hasOut : node.hasIn))
					{
						ImVec2 grip;
						if (center.visible && GripScreen(s, view, key, outgoing, grip))
						{
							drawList->AddLine(center.position, grip, COLOR_GRIP, 1.0f);
							AddDiamond(drawList, grip, GRIP_HALF + (hovered ? 1.5f : 0.0f), COLOR_GRIP, COLOR_NODE_BORDER);
						}
						continue;
					}
					const ScreenPoint handle = Project(view, outgoing ? node.handleOut : node.handleIn);
					if (!center.visible || !handle.visible)
						continue;
					drawList->AddLine(center.position, handle.position, COLOR_HANDLE_LINE, 1.5f);
					drawList->AddCircleFilled(handle.position, HANDLE_RADIUS + (hovered ? 1.5f : 0.0f), COLOR_HANDLE);
					drawList->AddCircle(handle.position, HANDLE_RADIUS + (hovered ? 1.5f : 0.0f), COLOR_NODE_BORDER, 0, 1.0f);
				}
			}

			// Edit points: nodes shaped by type (square corner, diamond free, circle smooth, ringed circle symmetric),
			// B-spline control points as hollow squares
			for (uint32_t i = 0; i < s.sessions.size(); ++i)
			{
				const Session &session = s.sessions[i];
				for (uint32_t c = 0; c < session.pointScreens.size(); ++c)
				{
					const Chain &chain = session.shape.chains[c];
					for (uint32_t n = 0; n < session.pointScreens[c].size(); ++n)
					{
						const ScreenPoint &screen = session.pointScreens[c][n];
						if (!screen.visible)
							continue;
						const bool selected = s.selected.contains({ i, { c, n } });
						const bool hovered = s.hovered.type == Hit::Type::Node && s.hovered.session == i && s.hovered.node == NodeRef{ c, n };
						const float half = NODE_HALF + (hovered ? 1.5f : 0.0f);
						const ImVec2 &p = screen.position;
						const ImVec2 a(p.x - half, p.y - half), b(p.x + half, p.y + half);
						if (chain.IsBSpline())
						{
							drawList->AddRectFilled(a, b, selected ? COLOR_NODE_SELECTED : IM_COL32(30, 22, 40, 200));
							drawList->AddRect(a, b, COLOR_CONTROL, 0.0f, 0, 1.6f);
							continue;
						}
						const ImU32 fill = selected ? COLOR_NODE_SELECTED : COLOR_NODE;
						switch (session.straight ? NodeType::Corner : GetNodeType(chain.nodes[n]))
						{
							case NodeType::Corner:
								drawList->AddRectFilled(a, b, fill);
								drawList->AddRect(a, b, COLOR_NODE_BORDER, 0.0f, 0, 1.2f);
								break;
							case NodeType::Free:
								AddDiamond(drawList, p, half + 1.0f, fill, COLOR_NODE_BORDER);
								break;
							case NodeType::Smooth:
								drawList->AddCircleFilled(p, half, fill);
								drawList->AddCircle(p, half, COLOR_NODE_BORDER, 0, 1.2f);
								break;
							case NodeType::Symmetric:
								drawList->AddCircleFilled(p, half, fill);
								drawList->AddCircle(p, half, COLOR_NODE_BORDER, 0, 1.2f);
								drawList->AddCircleFilled(p, 1.6f, COLOR_NODE_BORDER);
								break;
						}
					}
				}
			}

			if (s.snapTarget)
			{
				const ScreenPoint snap = Project(view, *s.snapTarget);
				if (snap.visible)
					drawList->AddCircle(snap.position, NODE_HALF + 5.0f, COLOR_SNAP, 0, 2.0f);
			}

			// Where Ctrl+click would extend the line to
			if (s.extendPreview)
			{
				const auto &[from, to] = *s.extendPreview;
				AddDashedLine(drawList, from, to, COLOR_OUTLINE_HOVER);
				drawList->AddRect(ImVec2(to.x - NODE_HALF, to.y - NODE_HALF), ImVec2(to.x + NODE_HALF, to.y + NODE_HALF), COLOR_OUTLINE_HOVER, 0.0f, 0, 1.2f);
			}
			drawList->PopClipRect();
		}

		/* @brief Center of the selected nodes on the ground (false when none of them is there any more). */
		bool SelectedCenter(const State &s, Point2 &center)
		{
			double sumX = 0.0, sumZ = 0.0;
			size_t count = 0;
			for (const NodeKey &key : s.selected)
			{
				if (!GetNode(s, key.session, key.ref))
					continue;
				const Point2 &point = s.sessions[key.session].shape.chains[key.ref.chain].GetEditPoint(key.ref.node);
				sumX += point.x;
				sumZ += point.z;
				++count;
			}
			if (count == 0)
				return false;
			center = { sumX / static_cast<double>(count), sumZ / static_cast<double>(count) };
			return true;
		}

		/**
		 * @brief The move / rotate / scale gizmo on the selected nodes (W / E / R with nodes selected): they move, turn
		 * (a single node turns its handles) and scale together on the ground about their center.
		 */
		bool NodeGizmo(State &s, const ViewportView &view, const ViewportGizmo::Mode mode, const bool allowInput)
		{
			if (s.selected.empty() || (s.drag != Drag::None && s.drag != Drag::Gizmo))
				return false;
			Point2 center;
			if (s.drag != Drag::Gizmo && !SelectedCenter(s, center))
				return false;

			using ViewportGizmo::Handle;
			const TransformTool::Settings &settings = TransformTool::GetSettings();
			ViewportGizmo::Options options;
			options.mode = mode;
			options.handles = mode == ViewportGizmo::Mode::Rotate ? Handle::AxisY : Handle::Ground;
			options.uniformGround = true;
			options.snap = Snapping();
			options.snapMove = settings.snapMove;
			options.snapAngle = settings.snapAngle;
			options.snapScale = settings.snapScale;
			ViewportGizmo::Frame frame;
			frame.pivot = { center.x, XPlane::SceneTerrain::GetHeight(center.x, center.z), center.z };
			const ViewportGizmo::Result result = ViewportGizmo::Manipulate(GIZMO_OWNER, view, frame, options, allowInput);

			if (result.started && s.drag != Drag::Gizmo)
			{
				s.drag = Drag::Gizmo;
				StartGesture(s);
			}
			if (s.drag != Drag::Gizmo)
				return result.hovered;

			if (result.active || result.finished)
			{
				// The ground-plane part of the drag (row vectors: x' = a x + b z + tx, z' = c x + d z + tz)
				const ViewportGizmo::Matrix &m = result.delta;
				Affine2 affine;
				affine.a = m.m[0][0];
				affine.b = m.m[2][0];
				affine.c = m.m[0][2];
				affine.d = m.m[2][2];
				affine.tx = m.m[3][0];
				affine.tz = m.m[3][2];
				for (uint32_t i = 0; i < s.sessions.size() && i < s.dragShapes.size(); ++i)
				{
					s.sessions[i].shape = s.dragShapes[i];
					TransformNodes(s.sessions[i].shape, SelectedIn(s, i), affine);
					s.sessions[i].resample = true;
				}

				const size_t count = s.selected.size();
				const char *noun = count == 1 ? "node" : "nodes";
				char text[128];
				switch (mode)
				{
					case ViewportGizmo::Mode::Translate:
						snprintf(text, sizeof(text), "Move %zu %s: %.2f m", count, noun, std::hypot(affine.tx, affine.tz));
						break;
					case ViewportGizmo::Mode::Rotate:
						snprintf(text, sizeof(text), "Rotate %zu %s: %.1f deg", count, noun, std::atan2(affine.b, affine.a) * 57.29577951308232);
						break;
					case ViewportGizmo::Mode::Scale:
						snprintf(text, sizeof(text), "Scale %zu %s: x%.3f / x%.3f", count, noun, std::hypot(affine.a, affine.c), std::hypot(affine.b, affine.d));
						break;
				}
				s.status = text;
			}

			if (result.finished)
			{
				static constexpr const char *VERBS[] = { "Move", "Rotate", "Scale" };
				const size_t count = s.selected.size();
				Commit(s, std::string(VERBS[static_cast<int>(mode)]) + " " + std::to_string(count) + (count == 1 ? " node" : " nodes"), ChangedSessions(s));
				s.drag = Drag::None;
				s.status.clear();
			}
			else if (!result.active)
			{
				// Dropped without a release (cancelled, or the gizmo wasn't shown for a frame)
				RestoreGesture(s);
				s.drag = Drag::None;
				s.status.clear();
			}
			return result.hovered || result.active;
		}

		/* @brief The chains an action on "the selection" means: those holding selected nodes, else every chain of the selected shapes. */
		std::vector<std::pair<uint32_t, uint32_t>> TargetChains(const State &s)
		{
			std::set<std::pair<uint32_t, uint32_t>> chains;
			for (const NodeKey &key : s.selected)
				chains.insert({ key.session, key.ref.chain });
			if (chains.empty())
				for (uint32_t i = 0; i < s.sessions.size(); ++i)
					for (uint32_t c = 0; c < s.sessions[i].shape.chains.size(); ++c)
						chains.insert({ i, c });

			std::vector<std::pair<uint32_t, uint32_t>> usable;
			for (const auto &[session, chain] : chains)
			{
				if (session >= s.sessions.size() || chain >= s.sessions[session].shape.chains.size())
					continue;
				const Session &owner = s.sessions[session];
				if (!owner.straight && !owner.pointsOnly && owner.shape.chains[chain].nodes.size() >= owner.shape.chains[chain].GetMinimumNodes())
					usable.emplace_back(session, chain);
			}
			return usable;
		}

		/* @brief The session (and chain 0) a close / open applies to: the selected node's, else the only selected shape. */
		Session *ClosableSession(State &s)
		{
			uint32_t index = ~0u;
			if (!s.selected.empty())
				index = s.selected.begin()->session;
			else if (s.sessions.size() == 1)
				index = 0;
			if (index >= s.sessions.size() || !s.sessions[index].closable || s.sessions[index].shape.chains.empty())
				return nullptr;
			return &s.sessions[index];
		}

		/* @brief Changes a project item's fields as one undo step. */
		void EditItem(const std::string &itemId, const std::string &label, const std::function<void(ProjectFile::SceneryItem &)> &edit)
		{
			ProjectFile::ProjectDocument *project = EditorProjects::GetDocument();
			ProjectFile::SceneryItem *item = project ? project->FindItem(itemId) : nullptr;
			if (!item)
				return;
			EditChanges::ChangeSet changes = EditChanges::Begin();
			ProjectFile::SceneryItem before = *item;
			edit(*item);
			changes.items.push_back({ std::move(before), *item });
			EditChanges::Publish(changes);
			EditChanges::Record(label, std::move(changes));
		}

		/* @brief A road's network, road type (named from its .net) and end levels; a road node's roads. Inside the property grid. */
		void DrawRoadProperties(const std::string &itemId, const bool junction)
		{
			ProjectFile::ProjectDocument *project = EditorProjects::GetDocument();
			const ProjectFile::SceneryItem *found = project ? project->FindItem(itemId) : nullptr;
			if (!found)
				return;
			const ProjectFile::SceneryItem item = *found;	// A copy: an edit below replaces the document's
			if (junction)
			{
				EditorUI::PropertyLabel("Roads here");
				ImGui::Text("%zu", ProjectFile::FindRoadsAt(project->GetProject(), item.id).size());
				EditorUI::ItemTooltip("Roads that start or end at this node: they follow it when it moves");
				return;
			}

			EditorUI::PropertyLabel("Network");
			ImGui::TextUnformatted(item.resource.c_str());
			const XPlane::RoadNetworkAsset *network = EditorProjects::GetRoadNetwork(item.resource);

			EditorUI::PropertyLabel("Road type");
			ImGui::SetNextItemWidth(-FLT_MIN);
			const std::string current = network ? network->GetSubtypeName(item.roadSubtype) + " (" + std::to_string(item.roadSubtype) + ")" : std::to_string(item.roadSubtype);
			if (network && !network->virtualTypes.empty())
			{
				if (ImGui::BeginCombo("##RoadType", current.c_str(), ImGuiComboFlags_HeightLarge))
				{
					for (const XPlane::RoadNetworkAsset::VirtualType &type : network->virtualTypes)
					{
						if (type.id < 0 || type.id > 255)
							continue;	// DSF road types are one byte
						const std::string label = (type.description.empty() ? "Type" : type.description) + " (" + std::to_string(type.id) + ")";
						if (ImGui::Selectable(label.c_str(), type.id == item.roadSubtype) && type.id != item.roadSubtype)
							EditItem(item.id, "Change road type", [&type](ProjectFile::SceneryItem &road) { road.roadSubtype = type.id; });
					}
					ImGui::EndCombo();
				}
			}
			else
			{
				int subtype = item.roadSubtype;
				if (ImGui::InputInt("##RoadType", &subtype, 1, 10, ImGuiInputTextFlags_EnterReturnsTrue) && subtype >= 0 && subtype <= 255 && subtype != item.roadSubtype)
					EditItem(item.id, "Change road type", [subtype](ProjectFile::SceneryItem &road) { road.roadSubtype = subtype; });
			}
			EditorUI::ItemTooltip(network ? "The network's road types (its virtual types): the DSF subtype X-Plane draws the road with"
										  : "DSF road subtype 0..255 (the network couldn't be read for names)");

			EditorUI::PropertyLabel("Levels");
			int levels[2] = { item.startLayer, item.endLayer };
			ImGui::SetNextItemWidth(-FLT_MIN);
			if (ImGui::InputInt2("##Levels", levels, ImGuiInputTextFlags_EnterReturnsTrue))
			{
				levels[0] = std::clamp(levels[0], 0, 2);
				levels[1] = std::clamp(levels[1], 0, 2);
				if (levels[0] != item.startLayer || levels[1] != item.endLayer)
					EditItem(item.id, "Change road levels", [levels](ProjectFile::SceneryItem &road) { road.startLayer = levels[0]; road.endLayer = levels[1]; });
			}
			EditorUI::ItemTooltip("Height level at the start and end: 0 on the ground, 1 and 2 raised (bridges and overpasses)");
		}

		/* @brief Starts / ends a gesture for a value dragged or typed in the PROPERTIES panel (call after its widget). */
		void TrackProperty(State &s, const char *label)
		{
			if (ImGui::IsItemActivated() && s.drag == Drag::None)
			{
				StartGesture(s);
				s.drag = Drag::Property;
				s.gestureLabel = label;
			}
			if (ImGui::IsItemDeactivated() && s.drag == Drag::Property)
			{
				Commit(s, s.gestureLabel, ChangedSessions(s));
				s.drag = Drag::None;
				s.gestureLabel.clear();
			}
		}

		/**
		 * @brief The node editing overlay and its mouse handling, shared by the node tool (gizmo null) and the move /
		 * rotate / scale tools working on selected nodes (the operation's gizmo on them, over the overlay).
		 */
		bool Gui(State &s, const ViewportView &view, const bool allowInput, const ViewportGizmo::Mode *gizmo)
		{
			Reload(s);
			if (!view.valid || s.sessions.empty())
			{
				s.hovered = {};
				if (s.drag != Drag::None && s.drag != Drag::Property)
					Cancel();
				return false;
			}

			ProjectSessions(s, view);
			if (s.drag == Drag::Property)
			{
				// A value is being dragged in the PROPERTIES panel: show it, take no input
				s.hovered = {};
				Draw(s, view);
				return false;
			}

			const ImGuiIO &io = ImGui::GetIO();
			const ImVec2 mouse = io.MousePos;
			const bool inImage = view.Contains(mouse);

			bool wantsMouse = gizmo && NodeGizmo(s, view, *gizmo, allowInput && s.drag == Drag::None);
			if (wantsMouse || s.drag == Drag::Gizmo)
			{
				s.hovered = {};
				Draw(s, view);
				ViewportGizmo::Draw(view);
				return true;
			}

			if (s.drag == Drag::None)
				s.hovered = allowInput && inImage ? HitTest(s, view, mouse) : Hit{};

			s.extendPreview.reset();
			if (s.drag == Drag::None && allowInput && inImage && io.KeyCtrl && !io.KeyAlt && s.hovered.type == Hit::Type::None)
			{
				if (const std::optional<LineEnd> end = SelectedEnd(s))
				{
					const std::vector<ScreenPoint> &points = s.sessions[end->session].pointScreens[end->chain];
					const ScreenPoint from = points[end->atStart ? 0 : points.size() - 1];
					if (from.visible)
						s.extendPreview = std::pair{ from.position, mouse };
				}
			}

			// Press: Ctrl+click extends a selected line end; otherwise on a handle, grip, node or curve (empty space is left
			// to object picking and the marquee)
			if (s.drag == Drag::None && allowInput && inImage && ImGui::IsMouseClicked(ImGuiMouseButton_Left) && !io.KeyAlt)
			{
				const bool add = io.KeyShift;
				const bool toggle = io.KeyCtrl;
				if (toggle && TryExtend(s, view, mouse))
					wantsMouse = true;
				else if (s.hovered.type != Hit::Type::None)
				{
					s.press = s.hovered;
					s.pressMouse = mouse;
					s.pressGroundValid = view.RayToGround(mouse, s.pressGround);
					s.pressAdd = add;
					s.pressToggle = toggle;

					if (s.press.type == Hit::Type::Handle || s.press.type == Hit::Type::Grip)
					{
						s.drag = Drag::Handle;
						s.breakSymmetry = io.KeyShift;
						StartGesture(s);
					}
					else if (s.press.type == Hit::Type::Node)
					{
						const NodeKey key{ s.press.session, s.press.node };
						if (toggle && s.selected.contains(key))
							s.selected.erase(key);
						else
						{
							if (!s.selected.contains(key) && !add && !toggle)
								s.selected.clear();
							s.selected.insert(key);
							s.drag = Drag::PendingNodes;
							StartGesture(s);
						}
					}
					else if (s.press.type == Hit::Type::Edge)
					{
						const Session &session = s.sessions[s.press.session];
						if (ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left) && !session.straight)
						{
							// Insert a node (a control point on a B-spline) where the curve was clicked
							StartGesture(s);
							NodeRef inserted;
							if (InsertNode(s.sessions[s.press.session].shape, s.press.node.chain, s.press.segment, std::clamp(s.press.t, 0.02, 0.98), &inserted))
							{
								s.sessions[s.press.session].resample = true;
								s.selected = { NodeKey{ s.press.session, inserted } };
								Commit(s, session.shape.chains[s.press.node.chain].IsBSpline() ? "Add control point" : "Insert node", { s.press.session });
							}
						}
						else if (session.straight)
						{
							// Runway / exclusion edges: grab both ends
							const uint32_t count = static_cast<uint32_t>(session.shape.chains[s.press.node.chain].nodes.size());
							if (!add && !toggle)
								s.selected.clear();
							s.selected.insert({ s.press.session, { s.press.node.chain, s.press.segment } });
							s.selected.insert({ s.press.session, { s.press.node.chain, (s.press.segment + 1) % count } });
							s.drag = Drag::PendingNodes;
							StartGesture(s);
						}
						else
						{
							s.drag = Drag::PendingEdge;
							StartGesture(s);
						}
					}
					wantsMouse = true;
				}
			}

			// Past the drag threshold, a pending press becomes its drag
			const bool moved = ImGui::IsMouseDown(ImGuiMouseButton_Left) && Distance2(mouse, s.pressMouse) > DRAG_THRESHOLD * DRAG_THRESHOLD;
			if (moved && s.drag == Drag::PendingNodes)
				s.drag = Drag::Nodes;
			else if (moved && s.drag == Drag::PendingEdge)
				s.drag = Drag::Bend;
			else if (moved && s.drag == Drag::PendingHandle)
				s.drag = Drag::Handle;

			if (s.drag == Drag::Nodes)
				DragNodes(s, view, mouse);
			else if (s.drag == Drag::Handle)
				DragHandle(s, view, mouse);
			else if (s.drag == Drag::Bend)
				DragBend(s, view, mouse);

			// Release: a click on a curve selects its two nodes; anything that changed is written
			if (s.drag != Drag::None && !ImGui::IsMouseDown(ImGuiMouseButton_Left))
			{
				if (s.drag == Drag::PendingEdge && s.press.session < s.sessions.size())
				{
					const Chain &chain = s.sessions[s.press.session].shape.chains[s.press.node.chain];
					const uint32_t count = static_cast<uint32_t>(chain.nodes.size());
					if (!s.pressAdd && !s.pressToggle)
						s.selected.clear();
					for (const uint32_t node : { s.press.segment, (s.press.segment + 1) % count })
					{
						const NodeKey key{ s.press.session, { s.press.node.chain, node } };
						if (s.pressToggle && s.selected.contains(key))
							s.selected.erase(key);
						else
							s.selected.insert(key);
					}
				}
				const std::vector<uint32_t> changed = ChangedSessions(s);
				if (!changed.empty())
				{
					std::string label = s.gestureLabel;
					if (label.empty())
					{
						if (s.drag == Drag::Handle || s.drag == Drag::Bend)
							label = "Bend curve";
						else
							label = "Move " + std::to_string(s.selected.size()) + (s.selected.size() == 1 ? " node" : " nodes");
					}
					Commit(s, label, changed);
				}
				s.drag = Drag::None;
				s.gestureLabel.clear();
				s.snapTarget.reset();
				s.status.clear();
			}

			Draw(s, view);
			if (gizmo)
				ViewportGizmo::Draw(view);
			if (s.drag == Drag::None)
				s.status.clear();
			return wantsMouse || s.drag != Drag::None || (inImage && s.hovered.type != Hit::Type::None);
		}
	}

	// -------------------------------------------------------

	bool OnViewportGui(EditorContext *, const ViewportView &view, const bool allowInput)
	{
		return Gui(S(), view, allowInput, nullptr);
	}

	bool OnTransformGui(EditorContext *, const ViewportView &view, const ViewportGizmo::Mode mode, const bool allowInput)
	{
		return Gui(S(), view, allowInput, &mode);
	}

	bool HasNodeSelection()
	{
		State &s = S();
		Reload(s);
		return !s.sessions.empty() && !s.selected.empty();
	}

	void DrawProperties(EditorContext *)
	{
		State &s = S();
		Reload(s);
		if (s.sessions.empty() || !EditorUI::SectionHeader("NODES"))
			return;

		// The chain shown: the first selected node's, or the first shape's outline
		NodeKey focus;
		if (!s.selected.empty())
			focus = *s.selected.begin();
		if (focus.session >= s.sessions.size() || focus.ref.chain >= s.sessions[focus.session].shape.chains.size())
			return;

		// Everything shown is read first: an edit made below rebuilds the sessions
		const Session &session = s.sessions[focus.session];
		const Target target = session.target;
		const XPlane::GeoProjection projection = session.projection;
		const Chain chain = session.shape.chains[focus.ref.chain];
		const size_t chains = session.shape.chains.size();
		const bool handles = !chain.IsBSpline() && !session.straight && !session.pointsOnly;
		const size_t selected = s.selected.size();
		bool bezierSelected = false;
		for (const NodeKey &key : s.selected)
			bezierSelected |= GetBezierChain(s, key) != nullptr;
		NodeType type = NodeType::Corner;
		const bool sameType = GetSelectedNodeType(type);
		const bool splineUsable = !TargetChains(s).empty();
		const bool splineActive = IsBSplineActive();
		bool closed = false;
		const bool closable = CanToggleClosed(&closed);
		const bool road = session.road;
		const bool junction = session.junction;

		if (!EditorUI::BeginPropertyGrid("##Nodes"))
			return;
		const float dpi = ImGui::GetStyle().FontScaleDpi;

		EditorUI::PropertyLabel("Selected");
		if (selected == 0)
			ImGui::TextDisabled("Click a node (Ctrl+A: all)");
		else
			ImGui::Text("%zu %s", selected, chain.IsBSpline() ? (selected == 1 ? "control point" : "control points") : (selected == 1 ? "node" : "nodes"));

		if (selected == 1 && focus.ref.node < chain.nodes.size())
		{
			// Where it is: typed, applied on Enter
			const Point2 point = chain.GetEditPoint(focus.ref.node);
			double latitude = 0.0, longitude = 0.0;
			projection.ToGeo(point, latitude, longitude);
			bool moved = false;
			EditorUI::PropertyLabel("Latitude");
			ImGui::SetNextItemWidth(-FLT_MIN);
			moved |= ImGui::InputDouble("##Latitude", &latitude, 0.0, 0.0, "%.8f", ImGuiInputTextFlags_EnterReturnsTrue);
			EditorUI::PropertyLabel("Longitude");
			ImGui::SetNextItemWidth(-FLT_MIN);
			moved |= ImGui::InputDouble("##Longitude", &longitude, 0.0, 0.0, "%.8f", ImGuiInputTextFlags_EnterReturnsTrue);
			if (moved && s.drag == Drag::None)
			{
				const Point2 to = projection.ToLocal(latitude, longitude);
				StartGesture(s);
				TransformNodes(s.sessions[focus.session].shape, { focus.ref }, Affine2::Translation(to.x - point.x, to.z - point.z));
				s.sessions[focus.session].resample = true;
				Commit(s, chain.IsBSpline() ? "Move control point" : "Move node", ChangedSessions(s));
			}

			if (handles)
			{
				const Node node = chain.nodes[focus.ref.node];
				for (const bool outgoing : { false, true })
				{
					EditorUI::PropertyLabel(outgoing ? "Out handle" : "In handle");
					ImGui::PushID(outgoing ? "Out" : "In");
					if (!(outgoing ? node.hasOut : node.hasIn))
					{
						ImGui::TextDisabled("None (drag its grip in the viewport)");
						ImGui::PopID();
						continue;
					}
					// Length and heading, dragged live (one undo step per drag)
					const Point2 handle = outgoing ? node.handleOut : node.handleIn;
					const Point2 offset{ handle.x - node.position.x, handle.z - node.position.z };
					double length = std::hypot(offset.x, offset.z);
					double heading = HeadingOf(offset);
					const double minimum = 0.01, maximum = 100000.0;
					const float width = (ImGui::GetContentRegionAvail().x - ImGui::GetStyle().ItemSpacing.x) * 0.5f;
					ImGui::SetNextItemWidth(width);
					bool changed = ImGui::DragScalar("##Length", ImGuiDataType_Double, &length, 0.05f, &minimum, &maximum, "%.2f m");
					EditorUI::ItemTooltip("Handle length: how far the curve reaches out of the node on this side");
					TrackProperty(s, "Edit handle");
					ImGui::SameLine();
					ImGui::SetNextItemWidth(width);
					changed |= ImGui::DragScalar("##Heading", ImGuiDataType_Double, &heading, 0.5f, nullptr, nullptr, "%.1f deg");
					EditorUI::ItemTooltip("Handle heading (true, clockwise from north): the direction the curve leaves the node in");
					TrackProperty(s, "Edit handle");
					if (changed && s.drag == Drag::Property && focus.session < s.dragShapes.size() && focus.session < s.sessions.size())
					{
						Session &edited = s.sessions[focus.session];
						edited.shape = s.dragShapes[focus.session];
						const Point2 offsetNow = FromHeading(heading, length);
						SetHandle(edited.shape, focus.ref, outgoing, { node.position.x + offsetNow.x, node.position.z + offsetNow.z }, false);
						edited.resample = true;
					}
					ImGui::PopID();
				}
			}
		}

		if (bezierSelected)
		{
			EditorUI::PropertyLabel("Type");
			ImGui::SetNextItemWidth(-FLT_MIN);
			if (ImGui::BeginCombo("##Type", sameType ? GetNodeTypeName(type) : "Mixed"))
			{
				for (const NodeType option : { NodeType::Corner, NodeType::Smooth, NodeType::Symmetric, NodeType::Free })
					if (ImGui::Selectable(GetNodeTypeName(option), sameType && option == type))
						SetSelectedNodeType(option);
				ImGui::EndCombo();
			}
			EditorUI::ItemTooltip("Corner: no handles (Shift+C)\nSmooth: handles opposite each other, each its own length (Shift+S)\n"
								  "Symmetric: mirror-image handles (Shift+Y)\nFree: handles move on their own (Shift+F)");
		}

		// The chain
		EditorUI::PropertyLabel("Shape");
		std::string what = EditorSelection::Describe(target);
		if (chains > 1)
			what += focus.ref.chain == 0 ? "  (outline)" : "  (hole " + std::to_string(focus.ref.chain) + ")";
		ImGui::TextUnformatted(what.c_str());
		EditorUI::PropertyLabel("Nodes");
		ImGui::Text("%zu, %s", chain.nodes.size(), chain.closed ? "closed" : "open");
		// Measured again only when the chain changes (the bend samples every half meter of the curve)
		struct Measure
		{
			uint64_t fingerprint = 0;
			double length = 0.0;
			double radius = 0.0;
		};
		static Measure s_Measure;
		uint64_t fingerprint = 1469598103934665603ull;
		const auto mix = [&fingerprint](const double value)
		{
			uint64_t bits = 0;
			std::memcpy(&bits, &value, sizeof(bits));
			fingerprint = (fingerprint ^ bits) * 1099511628211ull;
		};
		mix(chain.closed ? 1.0 : 0.0);
		for (const Node &node : chain.nodes)
		{
			for (const Point2 &point : { node.position, node.handleIn, node.handleOut })
			{
				mix(point.x);
				mix(point.z);
			}
			mix((node.hasIn ? 1.0 : 0.0) + (node.hasOut ? 2.0 : 0.0));
		}
		if (fingerprint != s_Measure.fingerprint)
		{
			s_Measure.fingerprint = fingerprint;
			s_Measure.length = GetChainLength(chain);
			s_Measure.radius = GetMinimumRadius(chain);
		}
		EditorUI::PropertyLabel("Length");
		ImGui::Text("%.2f m", s_Measure.length);
		EditorUI::PropertyLabel("Tightest bend");
		const double radius = s_Measure.radius;
		if (std::isinf(radius))
			ImGui::TextDisabled("None (straight segments)");
		else
			ImGui::Text("Radius %.1f m", radius);
		EditorUI::ItemTooltip("Smallest radius along the curves, measured over half a meter (sharp corners at nodes and segments under a meter don't count):\n"
							  "taxiway lines and roads need gentle bends");

		if (splineUsable)
		{
			EditorUI::PropertyLabel("Curve");
			if (ImGui::RadioButton("Bezier", !splineActive) && splineActive)
				SetBSpline(false);
			EditorUI::ItemTooltip("Edit nodes and their handles");
			ImGui::SameLine(0.0f, 10.0f * dpi);
			if (ImGui::RadioButton("B-spline", splineActive) && !splineActive)
				SetBSpline(true);
			EditorUI::ItemTooltip("Edit a control polygon instead (Shift+B): the curve stays smooth (C2) and only changes near what moves.\n"
								  "The nodes become the control points; a chain that already is a B-spline keeps its curve.");
			if (!splineActive)
			{
				EditorUI::PropertyLabel("");
				if (ImGui::Button("B-spline through the nodes"))
					SetBSpline(true, BSplineFit::ThroughNodes);
				EditorUI::ItemTooltip("A B-spline that still passes through every node (best with evenly spaced nodes)");
			}
		}

		if (road || junction)
			DrawRoadProperties(target.itemId, junction);

		if (closable)
		{
			EditorUI::PropertyLabel("Closed");
			bool value = closed;
			if (ImGui::Checkbox("##Closed", &value))
				ToggleClosed();
			EditorUI::ItemTooltip(closed ? "Open the line at its selected node (or its first one)" : "Close the line with a segment back to its first node");
		}
		EditorUI::EndPropertyGrid();
	}

	bool HandleShortcuts()
	{
		const ImGuiIO &io = ImGui::GetIO();
		if (!io.KeyShift || io.KeyCtrl || io.KeyAlt || io.KeySuper)
			return false;
		if (ImGui::IsKeyPressed(ImGuiKey_C, false))
			SetSelectedNodeType(NodeType::Corner);
		else if (ImGui::IsKeyPressed(ImGuiKey_S, false))
			SetSelectedNodeType(NodeType::Smooth);
		else if (ImGui::IsKeyPressed(ImGuiKey_Y, false))
			SetSelectedNodeType(NodeType::Symmetric);
		else if (ImGui::IsKeyPressed(ImGuiKey_F, false))
			SetSelectedNodeType(NodeType::Free);
		else if (ImGui::IsKeyPressed(ImGuiKey_L, false))
			StraightenSelectedSegments();
		else if (ImGui::IsKeyPressed(ImGuiKey_B, false))
			SetBSpline(!IsBSplineActive());
		else
			return false;
		return true;
	}

	bool IsDragging()
	{
		return S().drag != Drag::None;
	}

	void Cancel()
	{
		State &s = S();
		if (s.drag == Drag::None)
			return;
		if (s.drag == Drag::Gizmo)
			ViewportGizmo::Cancel();
		RestoreGesture(s);
		s.drag = Drag::None;
		s.gestureLabel.clear();
		s.snapTarget.reset();
		SetNotice(s, "Cancelled");
	}

	bool HasShapes()
	{
		State &s = S();
		Reload(s);
		return !s.sessions.empty();
	}

	size_t GetSelectedNodeCount()
	{
		return S().selected.size();
	}

	void ClearNodeSelection()
	{
		S().selected.clear();
	}

	void SelectAllNodes()
	{
		State &s = S();
		Reload(s);
		for (uint32_t i = 0; i < s.sessions.size(); ++i)
			for (const NodeRef &ref : AllNodes(s.sessions[i].shape))
				s.selected.insert({ i, ref });
	}

	bool SelectNode(const uint32_t shape, const uint32_t chain, const uint32_t node, const bool add)
	{
		State &s = S();
		Reload(s);
		if (shape >= s.sessions.size() || !s.sessions[shape].shape.IsValid(chain, node))
			return false;
		if (!add)
			s.selected.clear();
		s.selected.insert({ shape, { chain, node } });
		return true;
	}

	bool SelectNodesInRect(const ImVec2 &min, const ImVec2 &max, const bool add)
	{
		State &s = S();
		if (s.sessions.empty())
			return false;
		if (!add)
			s.selected.clear();
		for (uint32_t i = 0; i < s.sessions.size(); ++i)
		{
			const Session &session = s.sessions[i];
			for (uint32_t c = 0; c < session.pointScreens.size(); ++c)
			{
				for (uint32_t n = 0; n < session.pointScreens[c].size(); ++n)
				{
					const ScreenPoint &screen = session.pointScreens[c][n];
					if (screen.visible && screen.position.x >= min.x && screen.position.x <= max.x && screen.position.y >= min.y && screen.position.y <= max.y)
						s.selected.insert({ i, { c, n } });
				}
			}
		}
		return true;
	}

	bool TranslateSelectedNodes(const double east, const double north)
	{
		State &s = S();
		Reload(s);
		if (s.selected.empty() || s.drag != Drag::None)
			return false;
		StartGesture(s);
		for (const uint32_t i : SessionsWithSelection(s))
		{
			TransformNodes(s.sessions[i].shape, SelectedIn(s, i), Affine2::Translation(east, -north));
			s.sessions[i].resample = true;
		}
		const std::vector<uint32_t> changed = ChangedSessions(s);
		Commit(s, "Move " + std::to_string(s.selected.size()) + " nodes", changed);
		return !changed.empty();
	}

	bool DeleteSelectedNodes()
	{
		State &s = S();
		if (s.selected.empty() || s.drag != Drag::None)
			return false;
		StartGesture(s);
		const std::vector<uint32_t> sessions = SessionsWithSelection(s);
		size_t removed = 0;
		bool keptRoadEnds = false;
		for (const uint32_t i : sessions)
		{
			Session &session = s.sessions[i];
			if (session.junction)
				continue;	// A road node goes with the item (Select tool, Del)
			std::vector<NodeRef> refs = SelectedIn(s, i);
			if (session.road)
			{
				// A road's ends are its junctions: they stay (another road may end there too)
				const size_t before = refs.size();
				std::erase_if(refs, [&session](const NodeRef &ref) { return ref.node == 0 || ref.node + 1 == session.shape.chains[ref.chain].nodes.size(); });
				keptRoadEnds |= refs.size() != before;
			}
			removed += DeleteNodes(session.shape, refs);
			session.resample = true;
		}
		if (removed == 0)
		{
			SetNotice(s, keptRoadEnds ? "A road's end nodes are its junctions: move them, or delete the road" : "Shapes keep at least their minimum nodes (3 for a ring, 2 for a line)");
			return false;
		}
		s.selected.clear();
		Commit(s, "Delete " + std::to_string(removed) + " node" + (removed == 1 ? "" : "s"), ChangedSessions(s));
		return true;
	}

	bool SetSelectedNodeType(const NodeType type)
	{
		State &s = S();
		Reload(s);
		if (s.drag != Drag::None)
			return false;
		bool any = false;
		StartGesture(s);
		for (const uint32_t i : SessionsWithSelection(s))
		{
			if (s.sessions[i].straight || s.sessions[i].pointsOnly)
				continue;
			const std::vector<NodeRef> refs = SelectedIn(s, i);
			for (const NodeRef &ref : refs)
				any |= !s.sessions[i].shape.chains[ref.chain].IsBSpline();
			SetNodeType(s.sessions[i].shape, refs, type);
			s.sessions[i].resample = true;
		}
		// A type that changes no handle (free, or smooth on mirror images) only changes how they drag next
		const std::vector<uint32_t> changed = ChangedSessions(s);
		if (!changed.empty())
			Commit(s, std::string("Make nodes ") + GetNodeTypeName(type), changed);
		return any;
	}

	bool GetSelectedNodeType(NodeType &type)
	{
		const State &s = S();
		bool found = false;
		for (const NodeKey &key : s.selected)
		{
			if (!GetBezierChain(s, key))
				continue;
			const NodeType nodeType = GetNodeType(*GetNode(s, key.session, key.ref));
			if (found && nodeType != type)
				return false;
			type = nodeType;
			found = true;
		}
		return found;
	}

	bool StraightenSelectedSegments()
	{
		State &s = S();
		Reload(s);
		if (s.selected.size() < 2 || s.drag != Drag::None)
			return false;
		StartGesture(s);
		size_t straightened = 0;
		for (const uint32_t i : SessionsWithSelection(s))
		{
			straightened += StraightenSegments(s.sessions[i].shape, SelectedIn(s, i));
			s.sessions[i].resample = true;
		}
		if (straightened == 0)
		{
			SetNotice(s, "Select both nodes of the curved segments to straighten");
			return false;
		}
		Commit(s, "Straighten " + std::to_string(straightened) + " segment" + (straightened == 1 ? "" : "s"), ChangedSessions(s));
		return true;
	}

	bool ReverseSelectedShapes()
	{
		State &s = S();
		if (s.sessions.empty() || s.drag != Drag::None)
			return false;
		StartGesture(s);
		std::vector<uint32_t> sessions = SessionsWithSelection(s);
		if (sessions.empty())
			for (uint32_t i = 0; i < s.sessions.size(); ++i)
				sessions.push_back(i);
		for (const uint32_t i : sessions)
		{
			std::set<uint32_t> chains;
			for (const NodeRef &ref : SelectedIn(s, i))
				chains.insert(ref.chain);
			if (chains.empty())
				for (uint32_t c = 0; c < s.sessions[i].shape.chains.size(); ++c)
					chains.insert(c);
			for (const uint32_t c : chains)
				ReverseChain(s.sessions[i].shape.chains[c]);
			s.sessions[i].resample = true;
			s.sessions[i].reversed = !s.sessions[i].reversed;
		}
		s.selected.clear();
		Commit(s, "Reverse direction", sessions);
		for (Session &session : s.sessions)
			session.reversed = false;
		return true;
	}

	bool CanUseBSpline()
	{
		State &s = S();
		Reload(s);
		return !TargetChains(s).empty();
	}

	bool IsBSplineActive()
	{
		const State &s = S();
		const std::vector<std::pair<uint32_t, uint32_t>> chains = TargetChains(s);
		return !chains.empty() && std::ranges::all_of(chains, [&s](const auto &chain) { return s.sessions[chain.first].shape.chains[chain.second].IsBSpline(); });
	}

	bool SetBSpline(const bool enable, const BSplineFit fit)
	{
		State &s = S();
		Reload(s);
		if (s.drag != Drag::None)
			return false;
		const std::vector<std::pair<uint32_t, uint32_t>> chains = TargetChains(s);
		if (chains.empty())
			return false;
		StartGesture(s);
		size_t converted = 0;
		for (const auto &[index, c] : chains)
		{
			Session &session = s.sessions[index];
			Chain &chain = session.shape.chains[c];
			if (enable)
			{
				if (!BeginBSpline(chain, fit))
					continue;
			}
			else
				EndBSpline(chain);
			SetMode(s, session.target, c, enable);
			session.resample = true;
			++converted;
		}
		// Chains that were B-splines already change no node: only the ones refit are an edit
		const std::vector<uint32_t> changed = ChangedSessions(s);
		if (!changed.empty())
			Commit(s, enable ? (fit == BSplineFit::ThroughNodes ? "B-spline through nodes" : "Make B-spline") : "Edit as bezier", changed);
		SetNotice(s, enable ? std::to_string(converted) + (converted == 1 ? " chain is" : " chains are") + " edited as a B-spline: drag the square control points" : "Back to nodes and handles");
		return converted > 0;
	}

	bool CanToggleClosed(bool *closed)
	{
		State &s = S();
		Reload(s);
		const Session *session = ClosableSession(s);
		if (!session)
			return false;
		const Chain &chain = session->shape.chains[0];
		if (closed)
			*closed = chain.closed;
		return chain.nodes.size() >= 3 || chain.closed;
	}

	bool ToggleClosed()
	{
		State &s = S();
		Reload(s);
		if (s.drag != Drag::None)
			return false;
		Session *session = ClosableSession(s);
		if (!session)
			return false;
		const uint32_t index = static_cast<uint32_t>(session - s.sessions.data());
		const bool wasClosed = session->shape.chains[0].closed;
		uint32_t at = 0;
		for (const NodeKey &key : s.selected)
		{
			if (key.session == index && key.ref.chain == 0)
			{
				at = key.ref.node;
				break;
			}
		}
		StartGesture(s);
		if (!(wasClosed ? OpenChain(session->shape, 0, at) : CloseChain(session->shape, 0)))
		{
			SetNotice(s, "A line needs three nodes to close");
			return false;
		}
		session->resample = true;
		s.selected.clear();
		Commit(s, wasClosed ? "Open line" : "Close line", { index });
		return true;
	}

	const std::string &GetStatus()
	{
		return S().status;
	}

	std::string GetHint()
	{
		State &s = S();
		if (!s.status.empty())
			return s.status;
		if (!s.notice.empty() && ImGui::GetTime() < s.noticeUntil)
			return s.notice;
		if (s.sessions.empty())
			return "Select pavement, lines, boundaries, runways or project shapes to edit their nodes";

		bool spline = false;
		for (const NodeKey &key : s.selected)
			if (GetNode(s, key.session, key.ref))
				spline |= s.sessions[key.session].shape.chains[key.ref.chain].IsBSpline();
		if (s.selected.empty())
		{
			bool anySpline = false;
			for (const Session &session : s.sessions)
				for (const Chain &chain : session.shape.chains)
					anySpline |= chain.IsBSpline();
			return std::string("Click a node (box: drag)  |  drag a curve to bend it  |  double-click a curve to insert") +
				(anySpline ? "  |  B-splines: drag the square control points" : "  |  Shift+B: edit as a B-spline");
		}

		std::string hint = std::to_string(s.selected.size()) + " selected  |  W / E / R: gizmo";
		if (spline)
			hint += "  |  drag control points or the curve  |  double-click the curve: add a point  |  Shift+B: bezier";
		else
			hint += "  |  Shift+C/S/Y/F: corner, smooth, symmetric, free  |  Shift+L: straighten  |  drag a grip to pull handles";
		if (SelectedEnd(s))
			hint += "  |  Ctrl+click: extend the line (on its other end: close it)";
		return hint + "  |  Del";
	}
}

// -------------------------------------------------------
