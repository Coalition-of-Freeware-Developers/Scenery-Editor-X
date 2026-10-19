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
#include "transform_tool.h"
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <set>
#include <SceneryEditorX/project/spline_edit.h>
#include <SceneryEditorX/renderer/ui/source/imguizmo/ImGuizmo.h>

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
		constexpr float EDGE_HIT = 6.0f;
		constexpr float DRAG_THRESHOLD = 3.0f;
		constexpr float SNAP_PIXELS = 10.0f;		// Dragged nodes snap onto nodes this close to the cursor

		constexpr ImU32 COLOR_OUTLINE = IM_COL32(255, 154, 31, 235);
		constexpr ImU32 COLOR_OUTLINE_HOVER = IM_COL32(255, 214, 140, 255);
		constexpr ImU32 COLOR_NODE = IM_COL32(245, 245, 245, 255);
		constexpr ImU32 COLOR_NODE_SELECTED = IM_COL32(255, 154, 31, 255);
		constexpr ImU32 COLOR_NODE_BORDER = IM_COL32(20, 20, 20, 230);
		constexpr ImU32 COLOR_HANDLE = IM_COL32(120, 200, 255, 255);
		constexpr ImU32 COLOR_HANDLE_LINE = IM_COL32(170, 210, 240, 200);
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

			// Per frame
			std::vector<std::vector<Sample>> samples;
			bool resample = true;
			std::vector<std::vector<ScreenPoint>> sampleScreens;
			std::vector<std::vector<ScreenPoint>> nodeScreens;
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
			PendingNodes,	// Pressed on a node / edge, not moved far enough yet
			Nodes,
			Handle,
			Gizmo
		};

		struct Hit
		{
			enum class Type : uint8_t { None, Node, Handle, Edge } type = Type::None;
			uint32_t session = 0;
			NodeRef node;				// Node / Handle: the node; Edge: the segment's chain
			bool outgoing = false;		// Handle
			uint32_t segment = 0;		// Edge
			double t = 0.0;
		};

		struct State
		{
			std::vector<Session> sessions;
			uint64_t key = ~0ull;
			std::set<NodeKey> selected;

			Drag drag = Drag::None;
			Hit press;
			ImVec2 pressMouse;
			Point3 pressGround;
			bool pressGroundValid = false;
			bool breakSymmetry = false;
			std::vector<Shape> startShapes;
			std::optional<Point2> snapTarget;	// Where a dragged node snapped onto another node (drawn)

			float gizmo[16] = {};
			double gizmoStart[16] = {};
			Hit hovered;
			std::string status;
		};

		State &S()
		{
			static State state;
			return state;
		}

		double Distance2(const ImVec2 &a, const ImVec2 &b)
		{
			const double dx = a.x - b.x, dy = a.y - b.y;
			return dx * dx + dy * dy;
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
				case ItemKind::ExclusionZone:	return false;
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
				if (!item || item->locked || !ProjectFile::HasNodes(item->kind) || item->rings.empty())
					return std::nullopt;
				session.source = Source::ProjectItem;
				session.projection = EditorProjects::GetProjection();
				session.shape = FromGeoRings(item->rings, IsClosed(*item), session.projection);
				session.straight = item->kind == ProjectFile::ItemKind::ExclusionZone;
				session.pointsOnly = item->kind == ProjectFile::ItemKind::Forest && item->fill == ProjectFile::ForestFill::Points;
				return session;
			}
			return std::nullopt;
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

		ScreenPoint Project(const ViewportView &view, const Point2 &point)
		{
			ScreenPoint screen;
			screen.visible = view.WorldToScreen(Point3{ point.x, 0.0, point.z }, screen.position);
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
				session.nodeScreens.resize(chains);
				for (size_t c = 0; c < chains; ++c)
				{
					const std::vector<Sample> &samples = session.samples[c];
					std::vector<ScreenPoint> &sampleScreens = session.sampleScreens[c];
					sampleScreens.resize(samples.size());
					for (size_t i = 0; i < samples.size(); ++i)
						sampleScreens[i] = Project(view, samples[i].position);

					const std::vector<Node> &nodes = session.shape.chains[c].nodes;
					std::vector<ScreenPoint> &nodeScreens = session.nodeScreens[c];
					nodeScreens.resize(nodes.size());
					for (size_t i = 0; i < nodes.size(); ++i)
						nodeScreens[i] = Project(view, nodes[i].position);
				}
			}
		}

		Hit HitTest(const State &s, const ViewportView &view, const ImVec2 &mouse)
		{
			Hit hit;
			double best = 0.0;

			// Handles of selected nodes win: they sit on top
			best = HANDLE_HIT * HANDLE_HIT;
			for (const NodeKey &key : s.selected)
			{
				const Node *node = GetNode(s, key.session, key.ref);
				if (!node || s.sessions[key.session].straight)
					continue;
				for (const bool outgoing : { false, true })
				{
					if (!(outgoing ? node->hasOut : node->hasIn))
						continue;
					const ScreenPoint screen = Project(view, outgoing ? node->handleOut : node->handleIn);
					const double d = screen.visible ? Distance2(mouse, screen.position) : 1e30;
					if (d < best)
					{
						best = d;
						hit.type = Hit::Type::Handle;
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
				for (uint32_t c = 0; c < session.nodeScreens.size(); ++c)
				{
					for (uint32_t n = 0; n < session.nodeScreens[c].size(); ++n)
					{
						const ScreenPoint &screen = session.nodeScreens[c][n];
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
		}

		void RestoreGesture(State &s)
		{
			for (size_t i = 0; i < s.sessions.size() && i < s.startShapes.size(); ++i)
			{
				s.sessions[i].shape = s.startShapes[i];
				s.sessions[i].resample = true;
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
						item->rings = ToGeoRings(session.shape, session.projection);
						changes.items.push_back({ std::move(before), *item });
						break;
					}
				}
			}

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
					same = a.chains[c].nodes.size() == b.chains[c].nodes.size();
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

		/* @brief Nodes of every session other than the dragged ones, nearest to the cursor within snapping distance. */
		std::optional<Point2> FindSnapNode(const State &s, const ImVec2 &mouse)
		{
			double best = SNAP_PIXELS * SNAP_PIXELS;
			std::optional<Point2> found;
			for (uint32_t i = 0; i < s.sessions.size(); ++i)
			{
				const Session &session = s.sessions[i];
				for (uint32_t c = 0; c < session.nodeScreens.size(); ++c)
				{
					for (uint32_t n = 0; n < session.nodeScreens[c].size(); ++n)
					{
						if (s.selected.contains({ i, { c, n } }))
							continue;
						const ScreenPoint &screen = session.nodeScreens[c][n];
						const double d = screen.visible ? Distance2(mouse, screen.position) : 1e30;
						if (d < best && i < s.startShapes.size() && s.startShapes[i].IsValid(c, n))
						{
							best = d;
							found = s.startShapes[i].chains[c].nodes[n].position;
						}
					}
				}
			}
			return found;
		}

		void DragNodes(State &s, const ViewportView &view, const ImVec2 &mouse)
		{
			Point3 ground;
			if (!s.pressGroundValid || !view.RayToPlane(mouse, 0.0, ground) || s.press.session >= s.startShapes.size())
				return;

			// The node under the press leads; the others keep their offsets to it
			Point2 lead;
			if (s.press.type == Hit::Type::Edge)
				lead = s.startShapes[s.press.session].chains[s.press.node.chain].nodes[s.press.segment].position;
			else if (s.startShapes[s.press.session].IsValid(s.press.node.chain, s.press.node.node))
				lead = s.startShapes[s.press.session].chains[s.press.node.chain].nodes[s.press.node.node].position;
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
				s.sessions[i].shape = s.startShapes[i];
				TransformNodes(s.sessions[i].shape, SelectedIn(s, i), move);
				s.sessions[i].resample = true;
			}
			char text[128];
			snprintf(text, sizeof(text), "Move %zu node%s: %.2f m", s.selected.size(), s.selected.size() == 1 ? "" : "s", std::hypot(target.x - lead.x, target.z - lead.z));
			s.status = text;
		}

		void DragHandle(State &s, const ViewportView &view, const ImVec2 &mouse)
		{
			Point3 ground;
			if (!view.RayToPlane(mouse, 0.0, ground) || s.press.session >= s.sessions.size())
				return;
			Session &session = s.sessions[s.press.session];
			session.shape = s.startShapes[s.press.session];
			SetHandle(session.shape, s.press.node, s.press.outgoing, { ground.x, ground.z }, s.breakSymmetry);
			session.resample = true;
			s.status = s.breakSymmetry ? "Bend one side (handles split)" : "Bend the curve (Shift: just this side)";
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

			// Handles of selected nodes
			for (const NodeKey &key : s.selected)
			{
				const Node *node = GetNode(s, key.session, key.ref);
				if (!node || s.sessions[key.session].straight)
					continue;
				const ScreenPoint center = Project(view, node->position);
				for (const bool outgoing : { false, true })
				{
					if (!(outgoing ? node->hasOut : node->hasIn))
						continue;
					const ScreenPoint handle = Project(view, outgoing ? node->handleOut : node->handleIn);
					if (!center.visible || !handle.visible)
						continue;
					const bool hovered = s.hovered.type == Hit::Type::Handle && s.hovered.session == key.session && s.hovered.node == key.ref && s.hovered.outgoing == outgoing;
					drawList->AddLine(center.position, handle.position, COLOR_HANDLE_LINE, 1.5f);
					drawList->AddCircleFilled(handle.position, HANDLE_RADIUS + (hovered ? 1.5f : 0.0f), COLOR_HANDLE);
					drawList->AddCircle(handle.position, HANDLE_RADIUS + (hovered ? 1.5f : 0.0f), COLOR_NODE_BORDER, 0, 1.0f);
				}
			}

			// Nodes: squares, curved ones as circles
			for (uint32_t i = 0; i < s.sessions.size(); ++i)
			{
				const Session &session = s.sessions[i];
				for (uint32_t c = 0; c < session.nodeScreens.size(); ++c)
				{
					for (uint32_t n = 0; n < session.nodeScreens[c].size(); ++n)
					{
						const ScreenPoint &screen = session.nodeScreens[c][n];
						if (!screen.visible)
							continue;
						const bool selected = s.selected.contains({ i, { c, n } });
						const bool hovered = s.hovered.type == Hit::Type::Node && s.hovered.session == i && s.hovered.node == NodeRef{ c, n };
						const float half = NODE_HALF + (hovered ? 1.5f : 0.0f);
						const ImU32 fill = selected ? COLOR_NODE_SELECTED : COLOR_NODE;
						if (session.shape.chains[c].nodes[n].HasCurve())
						{
							drawList->AddCircleFilled(screen.position, half, fill);
							drawList->AddCircle(screen.position, half, COLOR_NODE_BORDER, 0, 1.2f);
						}
						else
						{
							const ImVec2 a(screen.position.x - half, screen.position.y - half), b(screen.position.x + half, screen.position.y + half);
							drawList->AddRectFilled(a, b, fill);
							drawList->AddRect(a, b, COLOR_NODE_BORDER, 0.0f, 0, 1.2f);
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
			drawList->PopClipRect();
		}

		/* @brief The node gizmo (two or more selected nodes): move, turn and scale them together on the ground. */
		bool NodeGizmo(State &s, const ViewportView &view, const bool allowInput)
		{
			if (s.selected.size() < 2 || (s.drag != Drag::None && s.drag != Drag::Gizmo))
				return false;

			const auto toMatrix = [](const double *values, float *out) { for (int i = 0; i < 16; ++i) out[i] = static_cast<float>(values[i]); };
			if (s.drag != Drag::Gizmo)
			{
				double sumX = 0.0, sumZ = 0.0;
				size_t count = 0;
				for (const NodeKey &key : s.selected)
				{
					if (const Node *node = GetNode(s, key.session, key.ref))
					{
						sumX += node->position.x;
						sumZ += node->position.z;
						++count;
					}
				}
				if (count == 0)
					return false;
				double identity[16] = { 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, sumX / count, 0, sumZ / count, 1 };
				std::copy(std::begin(identity), std::end(identity), std::begin(s.gizmoStart));
				toMatrix(s.gizmoStart, s.gizmo);
			}

			ImGuizmo::SetOrthographic(false);
			ImGuizmo::SetDrawlist(ImGui::GetWindowDrawList());
			ImGuizmo::SetRect(view.imageMin.x, view.imageMin.y, view.imageSize.x, view.imageSize.y);
			ImGuizmo::Enable(allowInput || s.drag == Drag::Gizmo);
			const ImGuizmo::OPERATION mask = ImGuizmo::TRANSLATE_X | ImGuizmo::TRANSLATE_Z | ImGuizmo::ROTATE_Y | ImGuizmo::SCALE_XU | ImGuizmo::SCALE_ZU;
			const TransformTool::Settings &settings = TransformTool::GetSettings();
			const float snap[3] = { settings.snapMove, settings.snapMove, settings.snapMove };
			ImGuizmo::Manipulate(view.view, view.projection, mask, ImGuizmo::WORLD, s.gizmo, nullptr, Snapping() ? snap : nullptr);

			const bool usingNow = ImGuizmo::IsUsing();
			if (usingNow && s.drag != Drag::Gizmo)
			{
				s.drag = Drag::Gizmo;
				StartGesture(s);
			}
			if (s.drag == Drag::Gizmo)
			{
				// G = start * D: the ground-plane part of D (row-vector) as the node transform
				const double *a = s.gizmoStart;
				double g[16];
				for (int i = 0; i < 16; ++i)
					g[i] = s.gizmo[i];
				const double tx0 = a[12], tz0 = a[14];
				Affine2 affine;
				affine.a = g[0];
				affine.b = g[8];
				affine.c = g[2];
				affine.d = g[10];
				// Rotation / scale about the start pivot, then the gizmo's translation
				affine.tx = g[12] - (affine.a * tx0 + affine.b * tz0);
				affine.tz = g[14] - (affine.c * tx0 + affine.d * tz0);
				for (uint32_t i = 0; i < s.sessions.size(); ++i)
				{
					s.sessions[i].shape = s.startShapes[i];
					TransformNodes(s.sessions[i].shape, SelectedIn(s, i), affine);
					s.sessions[i].resample = true;
				}
				char text[128];
				snprintf(text, sizeof(text), "Transform %zu nodes: %.2f m, %.1f deg", s.selected.size(), std::hypot(g[12] - tx0, g[14] - tz0), std::atan2(g[8], g[0]) * 57.29577951308232);
				s.status = text;

				if (!usingNow)
				{
					const std::vector<uint32_t> changed = ChangedSessions(s);
					Commit(s, "Transform " + std::to_string(s.selected.size()) + " nodes", changed);
					s.drag = Drag::None;
					s.status.clear();
				}
			}
			return usingNow || ImGuizmo::IsOver();
		}
	}

	// -------------------------------------------------------

	bool OnViewportGui(EditorContext *, const ViewportView &view, const bool allowInput)
	{
		State &s = S();
		Reload(s);
		if (!view.valid || s.sessions.empty())
		{
			s.hovered = {};
			if (s.drag != Drag::None)
				Cancel();
			return false;
		}

		ProjectSessions(s, view);
		const ImGuiIO &io = ImGui::GetIO();
		const ImVec2 mouse = io.MousePos;
		const bool inImage = view.Contains(mouse);

		bool wantsMouse = NodeGizmo(s, view, allowInput && s.drag == Drag::None);
		if (wantsMouse || s.drag == Drag::Gizmo)
		{
			s.hovered = {};
			Draw(s, view);
			return true;
		}

		if (s.drag == Drag::None)
			s.hovered = allowInput && inImage ? HitTest(s, view, mouse) : Hit{};

		// Press: on a handle, a node or an edge (empty space is left to object picking and the marquee)
		if (s.drag == Drag::None && allowInput && inImage && ImGui::IsMouseClicked(ImGuiMouseButton_Left) && !io.KeyAlt && s.hovered.type != Hit::Type::None)
		{
			s.press = s.hovered;
			s.pressMouse = mouse;
			s.pressGroundValid = view.RayToPlane(mouse, 0.0, s.pressGround);
			const bool add = io.KeyShift;
			const bool toggle = io.KeyCtrl;

			if (s.press.type == Hit::Type::Handle)
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
				const Chain &chain = session.shape.chains[s.press.node.chain];
				if (ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left) && !session.straight)
				{
					// Insert a node where the edge was clicked; the curve keeps its shape
					StartGesture(s);
					NodeRef inserted;
					if (InsertNode(s.sessions[s.press.session].shape, s.press.node.chain, s.press.segment, std::clamp(s.press.t, 0.02, 0.98), &inserted))
					{
						s.sessions[s.press.session].resample = true;
						s.selected = { NodeKey{ s.press.session, inserted } };
						Commit(s, "Insert node", { s.press.session });
					}
				}
				else
				{
					// Select the edge's two nodes; dragging moves them
					if (!add && !toggle)
						s.selected.clear();
					const uint32_t count = static_cast<uint32_t>(chain.nodes.size());
					s.selected.insert({ s.press.session, { s.press.node.chain, s.press.segment } });
					s.selected.insert({ s.press.session, { s.press.node.chain, (s.press.segment + 1) % count } });
					s.drag = Drag::PendingNodes;
					StartGesture(s);
				}
			}
			wantsMouse = true;
		}

		if (s.drag == Drag::PendingNodes && ImGui::IsMouseDown(ImGuiMouseButton_Left) && Distance2(mouse, s.pressMouse) > DRAG_THRESHOLD * DRAG_THRESHOLD)
			s.drag = Drag::Nodes;

		if (s.drag == Drag::Nodes)
			DragNodes(s, view, mouse);
		else if (s.drag == Drag::Handle)
			DragHandle(s, view, mouse);

		// Release: write what changed
		if (s.drag != Drag::None && !ImGui::IsMouseDown(ImGuiMouseButton_Left))
		{
			if (s.drag == Drag::Nodes || s.drag == Drag::Handle)
			{
				const std::vector<uint32_t> changed = ChangedSessions(s);
				const std::string label = s.drag == Drag::Handle ? "Bend curve" : "Move " + std::to_string(s.selected.size()) + " nodes";
				Commit(s, label, changed);
			}
			s.drag = Drag::None;
			s.snapTarget.reset();
			s.status.clear();
		}

		Draw(s, view);
		if (s.drag == Drag::None && s.status.empty() && !s.selected.empty())
		{
			char text[160];
			snprintf(text, sizeof(text), "%zu node%s selected", s.selected.size(), s.selected.size() == 1 ? "" : "s");
			s.status = text;
		}
		else if (s.drag == Drag::None && s.selected.empty())
			s.status.clear();

		return wantsMouse || s.drag != Drag::None || (inImage && s.hovered.type != Hit::Type::None);
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
		RestoreGesture(s);
		s.drag = Drag::None;
		s.snapTarget.reset();
		s.status = "Cancelled";
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
			for (uint32_t c = 0; c < session.nodeScreens.size(); ++c)
			{
				for (uint32_t n = 0; n < session.nodeScreens[c].size(); ++n)
				{
					const ScreenPoint &screen = session.nodeScreens[c][n];
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
		for (const uint32_t i : sessions)
		{
			removed += DeleteNodes(s.sessions[i].shape, SelectedIn(s, i));
			s.sessions[i].resample = true;
		}
		if (removed == 0)
		{
			s.status = "Shapes keep at least their minimum nodes (3 for a ring, 2 for a line)";
			return false;
		}
		s.selected.clear();
		Commit(s, "Delete " + std::to_string(removed) + " node" + (removed == 1 ? "" : "s"), ChangedSessions(s));
		return true;
	}

	bool SmoothSelectedNodes()
	{
		State &s = S();
		if (s.selected.empty() || s.drag != Drag::None)
			return false;
		StartGesture(s);
		for (const uint32_t i : SessionsWithSelection(s))
		{
			if (s.sessions[i].straight)
				continue;
			SmoothNodes(s.sessions[i].shape, SelectedIn(s, i));
			s.sessions[i].resample = true;
		}
		const std::vector<uint32_t> changed = ChangedSessions(s);
		Commit(s, "Smooth nodes", changed);
		return !changed.empty();
	}

	bool CornerSelectedNodes()
	{
		State &s = S();
		if (s.selected.empty() || s.drag != Drag::None)
			return false;
		StartGesture(s);
		for (const uint32_t i : SessionsWithSelection(s))
		{
			CornerNodes(s.sessions[i].shape, SelectedIn(s, i));
			s.sessions[i].resample = true;
		}
		const std::vector<uint32_t> changed = ChangedSessions(s);
		Commit(s, "Make corners", changed);
		return !changed.empty();
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
		}
		s.selected.clear();
		Commit(s, "Reverse direction", sessions);
		return true;
	}

	const std::string &GetStatus()
	{
		return S().status;
	}
}

// -------------------------------------------------------
