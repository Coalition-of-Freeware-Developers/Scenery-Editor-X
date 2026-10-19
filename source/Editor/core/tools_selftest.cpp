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
 * tools_selftest.cpp
 * -------------------------------------------------------
 * Created: 19/10/2026
 * -------------------------------------------------------
 */
#include "tools_selftest.h"
#include "airport_edit.h"
#include "editor_airports.h"
#include "editor_history.h"
#include "editor_project.h"
#include "editor_scene.h"
#include "editor_selection.h"
#include "editor_tools.h"
#include "node_tool.h"
#include "transform_tool.h"
#include <algorithm>
#include <chrono>
#include <cmath>
#include <map>
#include <optional>
#include <SceneryEditorX/logging/logging.hpp>
#include <SceneryEditorX/renderer/gpu_scene.h>
#include <SceneryEditorX/renderer/scene_picking.h>
#include <SceneryEditorX/scene/components/renderable.h>
#include <SceneryEditorX/xplane/xp_scene.h>

// -------------------------------------------------------

namespace SceneryEditorX::ToolsSelfTest
{
	using EditorSelection::Target;
	using XPlane::AirportElementKind;

	namespace
	{
		constexpr const char *LOG_TAG = "SelfTest";

		enum class Step : uint8_t
		{
			WaitScene,
			Pick,
			WaitPick,
			ItemMove,
			ItemUndo,
			ItemRedo,
			ItemRotate,
			ItemRotateUndo,
			ObjectMove,
			ObjectUndo,
			RowMove,
			RowUndo,
			NodeSelect,
			NodeMove,
			NodeUndo,
			NodeTypes,
			NodeTypesCheck,
			BSpline,
			BSplineMove,
			BSplineCheck,
			BSplineUndoCheck,
			LineClose,
			LineCloseUndo,
			RoadsCreate,
			RoadsDrawn,
			RoadsJunction,
			RoadsJunctionUndo,
			RoadsEnd,
			RoadsEndUndo,
			Delete,
			DeleteUndo,
			Box,
			WaitBox,
			Finish,
			Showcase,
			ShowcaseSpline,
			ShowcaseNodes,
			ShowcaseHold,
			ShowcaseUndo,
			Cleanup,
			Reselect,
			Done
		};

		struct State
		{
			bool enabled = false;
			Step step = Step::WaitScene;
			int wait = 0;
			int frames = 0;
			int stableFrames = 0;
			uint64_t structure = 0;
			uint64_t ticket = 0;
			int pickAttempts = 0;
			int passed = 0;
			int failed = 0;

			std::optional<Target> picked;		// The object covering most of the picked image, selected at the end
			std::string itemId;					// A project polygon
			std::optional<ProjectFile::SceneryItem> itemBefore, itemAfter;
			std::string objectId;				// A project .obj item
			std::optional<ProjectFile::SceneryItem> objectBefore;
			uint32_t pavement = ~0u;			// An airport pavement row
			AirportEdit::Snapshot rowBefore;
			std::string lineId;					// A project line (.lin) or string
			std::optional<ProjectFile::SceneryItem> lineBefore;

			std::vector<std::string> roadItems;	// Road nodes West, Middle, East, then the roads West-Middle, Middle-East
			std::vector<ProjectFile::SceneryItem> roadsBefore;
			bool roadMoved = false;	// The junction drag made an undo step (undone next)
			bool hadRecovery = false;			// The project had unsaved work from an earlier session when the test started
			bool showcase = false;				// --tools-selftest-showcase
			std::optional<Target> showcaseOther;	// The bezier shape shown beside the B-spline pavement
			std::vector<std::pair<Target, AirportEdit::Snapshot>> showcaseRows;
			int showcaseEdits = 0;				// Undo steps the showcase pushed
			int holdFrames = 0;
		};

		State &S()
		{
			static State state;
			return state;
		}

		void Check(const bool ok, const std::string &what)
		{
			State &s = S();
			if (ok)
			{
				++s.passed;
				EDITOR_INFO_TAG(LOG_TAG, "PASS {}", what);
			}
			else
			{
				++s.failed;
				EDITOR_ERROR_TAG(LOG_TAG, "FAIL {}", what);
			}
		}

		ProjectFile::SceneryItem *FindItem(const std::string &id)
		{
			ProjectFile::ProjectDocument *document = EditorProjects::GetDocument();
			return document && !id.empty() ? document->FindItem(id) : nullptr;
		}

		bool SameItem(const ProjectFile::SceneryItem &a, const ProjectFile::SceneryItem &b)
		{
			return a.id == b.id && a.kind == b.kind && a.name == b.name && a.resource == b.resource && a.latitude == b.latitude && a.longitude == b.longitude &&
				a.altitude == b.altitude && a.heading == b.heading && a.rings == b.rings && a.textureHeading == b.textureHeading && a.closed == b.closed;
		}

		bool SameChains(const std::vector<XPlane::AptNodeChain> &a, const std::vector<XPlane::AptNodeChain> &b)
		{
			if (a.size() != b.size())
				return false;
			for (size_t c = 0; c < a.size(); ++c)
			{
				if (a[c].closed != b[c].closed || a[c].nodes.size() != b[c].nodes.size())
					return false;
				for (size_t n = 0; n < a[c].nodes.size(); ++n)
				{
					const XPlane::AptNode &x = a[c].nodes[n];
					const XPlane::AptNode &y = b[c].nodes[n];
					if (x.latitude != y.latitude || x.longitude != y.longitude || x.hasControl != y.hasControl || x.lineType != y.lineType || x.lightType != y.lightType ||
						(x.hasControl && (x.controlLatitude != y.controlLatitude || x.controlLongitude != y.controlLongitude)))
						return false;
				}
			}
			return true;
		}

		const std::vector<XPlane::AptNodeChain> *PavementRings(const AirportEdit::Snapshot &snapshot)
		{
			const XPlane::AptPavement *pavement = std::get_if<XPlane::AptPavement>(&snapshot);
			return pavement ? &pavement->rings : nullptr;
		}

		/* @brief The node chains a pavement or painted line row copy holds. */
		const std::vector<XPlane::AptNodeChain> *ShapeChains(const AirportEdit::Snapshot &snapshot)
		{
			if (const XPlane::AptLinearFeature *line = std::get_if<XPlane::AptLinearFeature>(&snapshot))
				return &line->chains;
			return PavementRings(snapshot);
		}

		/* @brief Screen rectangle an entity's drawn box covers (false when part of it is behind the camera). */
		bool ScreenBounds(const ViewportView &view, const entt::entity handle, ImVec2 &min, ImVec2 &max)
		{
			Entity *entity = EditorScene::FindEntity(handle);
			Renderable *renderable = entity ? entity->GetComponent<Renderable>() : nullptr;
			if (!renderable)
				return false;
			const xMath::BoundingBox &box = renderable->GetBoundingBoxMesh();
			const xMath::Matrix world = entity->GetMatrix();
			const Vec3 lo = box.GetMin(), hi = box.GetMax();
			min = ImVec2(1e9f, 1e9f);
			max = ImVec2(-1e9f, -1e9f);
			for (int corner = 0; corner < 8; ++corner)
			{
				const double x = (corner & 1) ? hi.x : lo.x, y = (corner & 2) ? hi.y : lo.y, z = (corner & 4) ? hi.z : lo.z;
				const Point3 p{ x * world.m00 + y * world.m10 + z * world.m20 + world.m30, x * world.m01 + y * world.m11 + z * world.m21 + world.m31, x * world.m02 + y * world.m12 + z * world.m22 + world.m32 };
				ImVec2 screen;
				if (!view.WorldToScreen(p, screen))
					return false;
				min = ImVec2(std::min(min.x, screen.x), std::min(min.y, screen.y));
				max = ImVec2(std::max(max.x, screen.x), std::max(max.y, screen.y));
			}
			return true;
		}

		/* @brief The pick ids line up with what is drawn: sampled pixels fall inside the projected boxes of the entities they name. */
		void CheckPicks(const ViewportView &view, const ScenePicking::Result &result, const float renderWidth, const float renderHeight)
		{
			State &s = S();
			entt::registry *registry = EditorScene::GetRegistry();
			if (!registry || result.ids.empty())
			{
				Check(false, "pick readback returned the region");
				return;
			}

			std::map<uint32_t, size_t> pixels;
			size_t covered = 0;
			for (const uint32_t id : result.ids)
			{
				if (id != 0)
				{
					++pixels[id];
					++covered;
				}
			}
			const GpuScene::Stats &gpu = GpuScene::GetStats();
			EDITOR_INFO_TAG(LOG_TAG, "Pick readback: {} x {} pixels, {} drawn ({} distinct entities); GPU scene {} instances, {} visible ({} late, {} occluded)",
				result.width, result.height, covered, pixels.size(), gpu.instances, gpu.visibleInstances, gpu.lateInstances, gpu.occludedInstances);
			Check(covered > 0, "pick ids cover the drawn scene");

			// Sample a grid; each sampled id's entity box (projected) must contain the pixel
			size_t checked = 0, inside = 0;
			for (int gy = 1; gy <= 9; ++gy)
			{
				for (int gx = 1; gx <= 13; ++gx)
				{
					const uint32_t px = result.width * gx / 14, py = result.height * gy / 10;
					const uint32_t id = result.ids[static_cast<size_t>(py) * result.width + px];
					if (id == 0)
						continue;
					const entt::entity handle = ScenePicking::Resolve(*registry, id);
					ImVec2 min, max;
					if (handle == entt::null || !ScreenBounds(view, handle, min, max))
						continue;
					const ImVec2 screen(view.imageMin.x + (static_cast<float>(result.x + px) + 0.5f) * view.imageSize.x / renderWidth,
						view.imageMin.y + (static_cast<float>(result.y + py) + 0.5f) * view.imageSize.y / renderHeight);
					++checked;
					constexpr float slack = 3.0f;
					if (screen.x >= min.x - slack && screen.x <= max.x + slack && screen.y >= min.y - slack && screen.y <= max.y + slack)
						++inside;
					else
						EDITOR_WARN_TAG(LOG_TAG, "Pixel ({}, {}) names '{}' whose box spans ({:.0f}, {:.0f})-({:.0f}, {:.0f}) on screen", result.x + px, result.y + py,
							EditorScene::GetName(handle), min.x, min.y, max.x, max.y);
				}
			}
			EDITOR_INFO_TAG(LOG_TAG, "Pick alignment: {} of {} sampled pixels inside their entity's projected box", inside, checked);
			Check(checked >= 3 && inside * 10 >= checked * 9, "pick ids line up with the drawn image (not flipped or offset)");

			// What clicking the most covered entity selects
			uint32_t largest = 0;
			size_t most = 0;
			for (const auto &[id, count] : pixels)
			{
				if (count > most && id != 0)
				{
					most = count;
					largest = id;
				}
			}
			const entt::entity handle = ScenePicking::Resolve(*registry, largest);
			if (handle != entt::null)
			{
				s.picked = EditorSelection::FromPick(EditorScene::GetID(handle));
				if (s.picked)
					EDITOR_INFO_TAG(LOG_TAG, "Clicking '{}' selects '{}'", EditorScene::GetName(handle), EditorSelection::Describe(*s.picked));
			}
			Check(s.picked.has_value(), "a picked entity resolves to a selectable target");
		}

		std::string FindProjectItem(const ProjectFile::ItemKind kind, const bool placedObject)
		{
			ProjectFile::ProjectDocument *document = EditorProjects::GetDocument();
			if (!document)
				return {};
			std::string found;
			document->GetProject().ForEachItem([&](const ProjectFile::SceneryItem &item, int)
			{
				if (item.kind != kind || item.hidden || item.locked)
					return true;
				if (kind != ProjectFile::ItemKind::Object && item.rings.empty())
					return true;
				const uint64_t entity = EditorProjects::GetPlacedEntity(item.id);
				if (entity == 0)
					return true;
				uint64_t root = 0;
				if (placedObject && !(XPlane::SceneObjects::FindObject(entity, &root) && root == entity))
					return true;
				found = item.id;
				return false;
			});
			return found;
		}
	}

	void Enable()
	{
		S().enabled = true;
	}

	void EnableShowcase()
	{
		S().showcase = true;
	}

	bool IsEnabled()
	{
		return S().enabled;
	}

	void Tick(EditorContext *context, const ViewportView &view, const float renderWidth, const float renderHeight)
	{
		State &s = S();
		if (!s.enabled || s.step == Step::Done || !view.valid)
			return;
		++s.frames;
		if (s.wait > 0)
		{
			--s.wait;
			return;
		}

		const XPlane::GeoProjection &projectProjection = EditorProjects::GetProjection();
		switch (s.step)
		{
			case Step::WaitScene:
			{
				// Settled: a project or airport open (they wait for the library scan), nothing loading and the scene's
				// structure unchanged for a few seconds
				const uint64_t structure = Scene::GetStructureVersion();
				const bool opened = EditorProjects::GetDocument() != nullptr || EditorAirports::GetDocument() != nullptr || s.frames > 6000;
				s.stableFrames = (opened && structure == s.structure && XPlane::SceneObjects::GetLoadingObjectCount() == 0) ? s.stableFrames + 1 : 0;
				s.structure = structure;
				if (s.frames > 300 && s.stableFrames > 180)
				{
					const ProjectFile::ProjectDocument *project = EditorProjects::GetDocument();
					s.hadRecovery = project && project->GetRecovery().has_value();
					EDITOR_INFO_TAG(LOG_TAG, "Scene settled after {} frames: starting", s.frames);
					s.step = Step::Pick;
				}
				break;
			}

			case Step::Pick:
				s.ticket = ScenePicking::Request(0, 0, static_cast<uint32_t>(renderWidth), static_cast<uint32_t>(renderHeight));
				s.step = Step::WaitPick;
				break;

			case Step::WaitPick:
			{
				ScenePicking::Result result;
				if (!ScenePicking::TryGetResult(s.ticket, result))
				{
					if (s.frames > 6000)
					{
						Check(false, "pick readback arrived");
						s.step = Step::ItemMove;
					}
					break;
				}
				// An empty readback may only mean nothing was drawn yet (streaming, culling warming up): retry a few times
				if (std::ranges::none_of(result.ids, [](const uint32_t id) { return id != 0; }) && ++s.pickAttempts < 4)
				{
					const GpuScene::Stats &gpu = GpuScene::GetStats();
					EDITOR_WARN_TAG(LOG_TAG, "Pick readback {} came back empty (GPU scene {} instances, {} visible): retrying", s.pickAttempts, gpu.instances, gpu.visibleInstances);
					s.wait = 120;
					s.step = Step::Pick;
					break;
				}
				CheckPicks(view, result, renderWidth, renderHeight);
				s.step = Step::ItemMove;
				break;
			}

			case Step::ItemMove:
			{
				s.itemId = FindProjectItem(ProjectFile::ItemKind::Polygon, false);
				if (s.itemId.empty())
					s.itemId = FindProjectItem(ProjectFile::ItemKind::Line, false);
				ProjectFile::SceneryItem *item = FindItem(s.itemId);
				if (!item)
				{
					EDITOR_INFO_TAG(LOG_TAG, "No project polygon or line: skipping the item tests");
					s.step = Step::ObjectMove;
					break;
				}
				s.itemBefore = *item;
				EditorSelection::Select(context, Target::ForItem(s.itemId));
				const bool moved = TransformTool::TransformSelection(10.0, 0.0, 0.0, 1.0);
				item = FindItem(s.itemId);
				Check(moved && item, "move a project shape 10 m east");
				if (item)
				{
					s.itemAfter = *item;
					const double expected = 10.0 / projectProjection.GetMetersPerDegreeLongitude();
					const ProjectFile::GeoNode &before = s.itemBefore->rings[0][0];
					const ProjectFile::GeoNode &after = item->rings[0][0];
					Check(std::abs(after.longitude - before.longitude - expected) < 1e-9 && std::abs(after.latitude - before.latitude) < 1e-9,
						"its nodes moved 10 m east and nothing north (lon +" + std::to_string(after.longitude - before.longitude) + ")");
				}
				s.wait = 5;
				s.step = Step::ItemUndo;
				break;
			}

			case Step::ItemUndo:
			{
				EditorHistory::Undo();
				const ProjectFile::SceneryItem *item = FindItem(s.itemId);
				Check(item && SameItem(*item, *s.itemBefore), "undo puts the project shape back bit for bit");
				s.wait = 5;
				s.step = Step::ItemRedo;
				break;
			}

			case Step::ItemRedo:
			{
				EditorHistory::Redo();
				const ProjectFile::SceneryItem *item = FindItem(s.itemId);
				Check(item && SameItem(*item, *s.itemAfter), "redo moves it again");
				EditorHistory::Undo();
				s.wait = 5;
				s.step = Step::ItemRotate;
				break;
			}

			case Step::ItemRotate:
			{
				const bool turned = TransformTool::TransformSelection(0.0, 0.0, 90.0, 1.0);
				const ProjectFile::SceneryItem *item = FindItem(s.itemId);
				Check(turned && item && item->rings != s.itemBefore->rings, "turn the project shape 90 degrees about its center");
				if (item && item->kind == ProjectFile::ItemKind::Polygon)
					Check(std::abs(std::remainder(static_cast<double>(item->textureHeading) - s.itemBefore->textureHeading + 90.0, 360.0)) < 1e-3, "its texture heading turned with it");
				s.wait = 5;
				s.step = Step::ItemRotateUndo;
				break;
			}

			case Step::ItemRotateUndo:
			{
				EditorHistory::Undo();
				const ProjectFile::SceneryItem *item = FindItem(s.itemId);
				Check(item && SameItem(*item, *s.itemBefore), "undo of the turn restores it bit for bit");
				s.wait = 5;
				s.step = Step::ObjectMove;
				break;
			}

			case Step::ObjectMove:
			{
				s.objectId = FindProjectItem(ProjectFile::ItemKind::Object, true);
				ProjectFile::SceneryItem *item = FindItem(s.objectId);
				if (!item)
				{
					EDITOR_INFO_TAG(LOG_TAG, "No placed project object: skipping the object tests");
					s.step = Step::RowMove;
					break;
				}
				s.objectBefore = *item;
				EditorSelection::Select(context, Target::ForItem(s.objectId));
				const bool moved = TransformTool::TransformSelection(5.0, 5.0, 30.0, 1.0);
				item = FindItem(s.objectId);
				Check(moved && item, "move and turn a project object");
				if (item)
				{
					const XPlane::Point2 a = projectProjection.ToLocal(s.objectBefore->latitude, s.objectBefore->longitude);
					const XPlane::Point2 b = projectProjection.ToLocal(item->latitude, item->longitude);
					Check(std::abs(b.x - a.x - 5.0) < 0.02 && std::abs(-(b.z - a.z) - 5.0) < 0.02, "the object moved 5 m east and 5 m north (" + std::to_string(b.x - a.x) + ", " + std::to_string(a.z - b.z) + ")");
					Check(std::abs(std::remainder(static_cast<double>(item->heading) - s.objectBefore->heading + 30.0, 360.0)) < 0.05, "its heading turned 30 degrees (clockwise heading -30)");
				}
				s.wait = 3;
				s.step = Step::ObjectUndo;
				break;
			}

			case Step::ObjectUndo:
			{
				EditorHistory::Undo();
				const ProjectFile::SceneryItem *item = FindItem(s.objectId);
				const bool back = item && std::abs(item->latitude - s.objectBefore->latitude) < 1e-7 && std::abs(item->longitude - s.objectBefore->longitude) < 1e-7 &&
					std::abs(std::remainder(static_cast<double>(item->heading) - s.objectBefore->heading, 360.0)) < 0.01;
				Check(back, "undo puts the object back (within the scene's float precision)");
				s.wait = 3;
				s.step = Step::RowMove;
				break;
			}

			case Step::RowMove:
			{
				XPlane::AirportDocument *airport = EditorAirports::GetDocument();
				s.pavement = ~0u;
				if (airport)
				{
					for (uint32_t i = 0; i < airport->airport.pavements.size(); ++i)
					{
						if (!airport->airport.pavements[i].rings.empty() && airport->airport.pavements[i].rings[0].nodes.size() >= 4)
						{
							s.pavement = i;
							break;
						}
					}
				}
				if (s.pavement == ~0u)
				{
					EDITOR_INFO_TAG(LOG_TAG, "No airport pavement: skipping the airport row tests");
					s.step = Step::Delete;
					break;
				}
				s.rowBefore = AirportEdit::Capture(airport->airport, AirportElementKind::Pavement, s.pavement);
				EditorSelection::Select(context, Target::ForElement(AirportElementKind::Pavement, s.pavement));
				const bool moved = TransformTool::TransformSelection(4.0, 0.0, 0.0, 1.0);
				const XPlane::AptNode &before = PavementRings(s.rowBefore)->front().nodes.front();
				const XPlane::AptNode &after = airport->airport.pavements[s.pavement].rings.front().nodes.front();
				const double expected = 4.0 / XPlane::SceneAirports::GetProjection().GetMetersPerDegreeLongitude();
				Check(moved && std::abs(after.longitude - before.longitude - expected) < 1e-9, "move an airport pavement 4 m east");
				s.wait = 4;	// The row is rebuilt on the next Tick (the log has its time)
				s.step = Step::RowUndo;
				break;
			}

			case Step::RowUndo:
			{
				std::vector<uint64_t> parts;
				XPlane::SceneAirports::FindEntities(AirportElementKind::Pavement, s.pavement, parts);
				Check(!parts.empty(), "the moved row was rebuilt (" + std::to_string(parts.size()) + " part entities)");
				EditorHistory::Undo();
				XPlane::AirportDocument *airport = EditorAirports::GetDocument();
				Check(airport && SameChains(airport->airport.pavements[s.pavement].rings, *PavementRings(s.rowBefore)), "undo puts the pavement back bit for bit");
				s.wait = 4;
				s.step = Step::NodeSelect;
				break;
			}

			case Step::NodeSelect:
				EditorTools::SetTool(EditorTools::Tool::Nodes);
				EditorSelection::Select(context, Target::ForElement(AirportElementKind::Pavement, s.pavement));
				s.wait = 2;	// The node tool opens its sessions on the next frames
				s.step = Step::NodeMove;
				break;

			case Step::NodeMove:
			{
				Check(NodeTool::HasShapes(), "the node tool opens the selected pavement");
				NodeTool::SelectAllNodes();
				const size_t nodes = NodeTool::GetSelectedNodeCount();
				const bool moved = NodeTool::TranslateSelectedNodes(0.0, 3.0);
				XPlane::AirportDocument *airport = EditorAirports::GetDocument();
				const XPlane::AptNode &before = PavementRings(s.rowBefore)->front().nodes.front();
				const XPlane::AptNode &after = airport->airport.pavements[s.pavement].rings.front().nodes.front();
				const double expected = 3.0 / XPlane::SceneAirports::GetProjection().GetMetersPerDegreeLatitude();
				Check(moved && nodes > 0 && std::abs(after.latitude - before.latitude - expected) < 1e-9, "move all " + std::to_string(nodes) + " nodes of the pavement 3 m north");
				s.wait = 4;
				s.step = Step::NodeUndo;
				break;
			}

			case Step::NodeUndo:
			{
				EditorHistory::Undo();
				XPlane::AirportDocument *airport = EditorAirports::GetDocument();
				Check(airport && SameChains(airport->airport.pavements[s.pavement].rings, *PavementRings(s.rowBefore)), "undo of the node edit restores the pavement bit for bit");
				s.wait = 4;
				s.step = Step::NodeTypes;
				break;
			}

			case Step::NodeTypes:
			{
				NodeTool::SelectAllNodes();
				const bool set = NodeTool::SetSelectedNodeType(Splines::NodeType::Smooth);
				XPlane::AirportDocument *airport = EditorAirports::GetDocument();
				Check(set && airport && !SameChains(airport->airport.pavements[s.pavement].rings, *PavementRings(s.rowBefore)), "make every pavement node smooth");
				s.wait = 4;
				s.step = Step::NodeTypesCheck;
				break;
			}

			case Step::NodeTypesCheck:
			{
				// Read back from the rewritten rows: smooth nodes are split pairs whose handles line up, or mirror images
				Splines::NodeType type = Splines::NodeType::Corner;
				const bool same = NodeTool::GetSelectedNodeType(type);
				Check(NodeTool::GetSelectedNodeCount() > 0 && same && type == Splines::NodeType::Smooth,
					std::string("the nodes read back as smooth after the rows were rewritten (") + (same ? Splines::GetNodeTypeName(type) : "mixed") + ")");
				EditorHistory::Undo();
				XPlane::AirportDocument *airport = EditorAirports::GetDocument();
				Check(airport && SameChains(airport->airport.pavements[s.pavement].rings, *PavementRings(s.rowBefore)), "undo of the node types restores the pavement bit for bit");
				s.wait = 4;
				s.step = Step::BSpline;
				break;
			}

			case Step::BSpline:
			{
				NodeTool::SelectAllNodes();
				const bool made = NodeTool::SetBSpline(true);
				Check(made && NodeTool::IsBSplineActive(), "edit the pavement as a B-spline");
				s.wait = 4;
				s.step = Step::BSplineMove;
				break;
			}

			case Step::BSplineMove:
			{
				// apt.dat stores bezier nodes only: the B-spline must be recognized again in the rewritten rows
				Check(NodeTool::IsBSplineActive(), "the rewritten pavement is recognized as a B-spline again");
				NodeTool::SelectAllNodes();
				const bool moved = NodeTool::TranslateSelectedNodes(0.0, 2.0);
				Check(moved, "move all of its control points 2 m north");
				s.wait = 4;
				s.step = Step::BSplineCheck;
				break;
			}

			case Step::BSplineCheck:
			{
				Check(NodeTool::IsBSplineActive(), "still a B-spline after its control points moved");
				EditorHistory::Undo();
				EditorHistory::Undo();
				s.wait = 4;
				s.step = Step::BSplineUndoCheck;
				break;
			}

			case Step::BSplineUndoCheck:
			{
				XPlane::AirportDocument *airport = EditorAirports::GetDocument();
				Check(airport && SameChains(airport->airport.pavements[s.pavement].rings, *PavementRings(s.rowBefore)), "undoing the B-spline restores the pavement bit for bit");
				NodeTool::SelectAllNodes();
				Check(!NodeTool::IsBSplineActive(), "the restored pavement is edited with nodes and handles again");
				s.wait = 2;
				s.step = Step::LineClose;
				break;
			}

			case Step::LineClose:
			{
				// A line or string that can close (three nodes or more) or open; a two-node one must refuse
				std::string shortLine;
				s.lineId.clear();
				if (ProjectFile::ProjectDocument *document = EditorProjects::GetDocument())
				{
					document->GetProject().ForEachItem([&](const ProjectFile::SceneryItem &item, int)
					{
						using ProjectFile::ItemKind;
						if ((item.kind != ItemKind::Line && item.kind != ItemKind::String) || item.locked || item.rings.size() != 1)
							return true;
						if (item.closed || item.rings[0].size() >= 3)
						{
							s.lineId = item.id;
							return false;
						}
						if (shortLine.empty())
							shortLine = item.id;
						return true;
					});
				}
				if (s.lineId.empty() && !shortLine.empty())
				{
					EditorSelection::Select(context, Target::ForItem(shortLine));
					NodeTool::ClearNodeSelection();
					const ProjectFile::SceneryItem before = *FindItem(shortLine);
					const bool refused = !NodeTool::CanToggleClosed() && !NodeTool::ToggleClosed();
					Check(refused && SameItem(*FindItem(shortLine), before), "a two-node line refuses to close and stays as it was");
				}
				ProjectFile::SceneryItem *item = FindItem(s.lineId);
				if (!item)
				{
					EDITOR_INFO_TAG(LOG_TAG, "No project line or string of three nodes: skipping the close / open test");
					EditorTools::SetTool(EditorTools::Tool::Select);
					s.step = Step::Delete;
					break;
				}
				s.lineBefore = *item;
				EditorSelection::Select(context, Target::ForItem(s.lineId));
				NodeTool::ClearNodeSelection();
				bool closed = false;
				const bool can = NodeTool::CanToggleClosed(&closed);
				const bool toggled = can && NodeTool::ToggleClosed();
				item = FindItem(s.lineId);
				const size_t nodesBefore = s.lineBefore->rings[0].size();
				const size_t nodesAfter = item && !item->rings.empty() ? item->rings[0].size() : 0;
				Check(toggled && item && item->closed != s.lineBefore->closed && nodesAfter == nodesBefore + (closed ? 1 : 0),
					std::string(closed ? "open" : "close") + " a project line (" + std::to_string(nodesBefore) + " -> " + std::to_string(nodesAfter) + " nodes)");
				s.wait = 4;
				s.step = Step::LineCloseUndo;
				break;
			}

			case Step::LineCloseUndo:
			{
				EditorHistory::Undo();
				const ProjectFile::SceneryItem *item = FindItem(s.lineId);
				Check(item && SameItem(*item, *s.lineBefore), "undo puts the line back as it was");
				EditorTools::SetTool(EditorTools::Tool::Select);
				s.wait = 4;
				s.step = Step::RoadsCreate;
				break;
			}

			case Step::RoadsCreate:
			{
				// Three road nodes and two roads meeting at the middle one (a straight with a shape point, then a curve),
				// beside the tested pavement, on X-Plane's own road network
				ProjectFile::ProjectDocument *document = EditorProjects::GetDocument();
				if (!document)
				{
					EDITOR_INFO_TAG(LOG_TAG, "No project: skipping the road tests");
					s.step = Step::Delete;
					break;
				}
				double latitude = 0.0, longitude = 0.0;
				XPlane::AirportDocument *airport = EditorAirports::GetDocument();
				if (airport && s.pavement != ~0u)
				{
					const XPlane::AptNodeChain &ring = airport->airport.pavements[s.pavement].rings[0];
					for (const XPlane::AptNode &node : ring.nodes)
					{
						latitude += node.latitude / static_cast<double>(ring.nodes.size());
						longitude += node.longitude / static_cast<double>(ring.nodes.size());
					}
				}
				else
					projectProjection.ToGeo({ 0.0, 0.0 }, latitude, longitude);
				const auto at = [&](const double east, const double north)
				{
					return std::pair{ latitude + north / projectProjection.GetMetersPerDegreeLatitude(), longitude + east / projectProjection.GetMetersPerDegreeLongitude() };
				};

				// A local road type of the network (the first one named "local...", else the first it can draw)
				const std::string resource = "lib/g10/roads.net";
				int subtype = 1;
				const XPlane::RoadNetworkAsset *network = EditorProjects::GetRoadNetwork(resource);
				if (network)
				{
					bool found = false;
					for (const XPlane::RoadNetworkAsset::VirtualType &type : network->virtualTypes)
					{
						if (type.id <= 0 || type.id > 255 || !network->FindRoadType(type.id))
							continue;
						if (!found || type.description.find("local") != std::string::npos)
						{
							subtype = type.id;
							found = true;
							if (type.description.find("local") != std::string::npos)
								break;
						}
					}
				}
				Check(network != nullptr, "X-Plane's road network " + resource + " loads (road type " + std::to_string(subtype) + ")");

				std::vector<ProjectFile::SceneryItem> items;
				const char *names[] = { "SelfTest West", "SelfTest Middle", "SelfTest East" };
				const std::pair<double, double> places[] = { at(-60.0, -60.0), at(0.0, -60.0), at(50.0, -20.0) };
				for (int i = 0; i < 3; ++i)
				{
					ProjectFile::SceneryItem node;
					node.kind = ProjectFile::ItemKind::RoadNode;
					node.id = ProjectFile::MakeItemId();
					node.name = names[i];
					node.latitude = places[i].first;
					node.longitude = places[i].second;
					items.push_back(node);
				}
				const auto geo = [](const std::pair<double, double> &place)
				{
					ProjectFile::GeoNode node;
					node.latitude = place.first;
					node.longitude = place.second;
					return node;
				};
				ProjectFile::SceneryItem straight;
				straight.kind = ProjectFile::ItemKind::Road;
				straight.id = ProjectFile::MakeItemId();
				straight.name = "SelfTest straight road";
				straight.resource = resource;
				straight.roadSubtype = subtype;
				straight.startNode = items[0].id;
				straight.endNode = items[1].id;
				straight.rings = { { geo(places[0]), geo(at(-30.0, -62.0)), geo(places[1]) } };
				ProjectFile::SceneryItem curve = straight;
				curve.id = ProjectFile::MakeItemId();
				curve.name = "SelfTest curved road";
				curve.startNode = items[1].id;
				curve.endNode = items[2].id;
				ProjectFile::GeoNode start = geo(places[1]), end = geo(places[2]);
				start.hiLongitude = 30.0 / projectProjection.GetMetersPerDegreeLongitude();	// Leaves east, arrives from the south
				end.loLatitude = -25.0 / projectProjection.GetMetersPerDegreeLatitude();
				curve.rings = { { start, end } };
				items.push_back(straight);
				items.push_back(curve);

				s.roadItems.clear();
				bool inserted = true;
				for (const ProjectFile::SceneryItem &item : items)
				{
					inserted &= EditorProjects::InsertItem(item, std::string(), document->GetProject().items.size());
					s.roadItems.push_back(item.id);
				}
				bool valid = true;
				for (const std::string &problem : ProjectFile::ValidateProject(document->GetProject()))
					valid &= problem.find("SelfTest") == std::string::npos;
				Check(inserted && valid, "add 3 road nodes and 2 roads to the project (they validate)");
				s.wait = 150;	// The project scenery rebuilds when what it draws changes
				s.step = Step::RoadsDrawn;
				break;
			}

			case Step::RoadsDrawn:
			{
				Check(EditorProjects::GetPlacedEntity(s.roadItems[3]) != 0 && EditorProjects::GetPlacedEntity(s.roadItems[4]) != 0, "both roads are drawn (an entity each)");
				EditorTools::SetTool(EditorTools::Tool::Nodes);
				EditorSelection::Select(context, Target::ForItem(s.roadItems[1]));
				s.wait = 2;
				s.step = Step::RoadsJunction;
				break;
			}

			case Step::RoadsJunction:
			{
				// The middle node moved 10 m north: both roads' ends there follow it, keeping their handles
				s.roadsBefore.clear();
				for (const std::string &id : s.roadItems)
					if (const ProjectFile::SceneryItem *item = FindItem(id))
						s.roadsBefore.push_back(*item);
				NodeTool::SelectAllNodes();
				const bool moved = NodeTool::TranslateSelectedNodes(0.0, 10.0);
				s.roadMoved = moved;
				const ProjectFile::SceneryItem *middle = FindItem(s.roadItems[1]);
				const ProjectFile::SceneryItem *straight = FindItem(s.roadItems[3]);
				const ProjectFile::SceneryItem *curve = FindItem(s.roadItems[4]);
				const double north = 10.0 / projectProjection.GetMetersPerDegreeLatitude();
				const bool follows = s.roadsBefore.size() == 5 && middle && straight && curve && std::abs(middle->latitude - s.roadsBefore[1].latitude - north) < 1e-9 &&
					straight->rings[0].back().latitude == middle->latitude && curve->rings[0].front().latitude == middle->latitude &&
					curve->rings[0].front().hiLongitude == s.roadsBefore[4].rings[0].front().hiLongitude;
				Check(moved && follows, "drag a junction 10 m north: both roads meeting there follow, handles kept");
				s.wait = 4;
				s.step = Step::RoadsJunctionUndo;
				break;
			}

			case Step::RoadsJunctionUndo:
			{
				if (s.roadMoved)
					EditorHistory::Undo();
				bool back = s.roadsBefore.size() == 5;
				for (size_t i = 0; i < s.roadItems.size() && i < s.roadsBefore.size(); ++i)
				{
					const ProjectFile::SceneryItem *item = FindItem(s.roadItems[i]);
					back &= item && SameItem(*item, s.roadsBefore[i]);
				}
				Check(back, "one undo puts the junction and both roads back");
				EditorSelection::Select(context, Target::ForItem(s.roadItems[3]));
				s.wait = 2;
				s.step = Step::RoadsEnd;
				break;
			}

			case Step::RoadsEnd:
			{
				// The straight road's end dragged 5 m east: its junction and the curve starting there follow; its ends can't be deleted
				const ProjectFile::SceneryItem *road = FindItem(s.roadItems[3]);
				const uint32_t last = road ? static_cast<uint32_t>(road->rings[0].size() - 1) : 0;
				const bool moved = road && NodeTool::SelectNode(0, 0, last, false) && NodeTool::TranslateSelectedNodes(5.0, 0.0);
				const ProjectFile::SceneryItem *middle = FindItem(s.roadItems[1]);
				const ProjectFile::SceneryItem *curve = FindItem(s.roadItems[4]);
				const double east = 5.0 / projectProjection.GetMetersPerDegreeLongitude();
				Check(moved && middle && curve && s.roadsBefore.size() == 5 && std::abs(middle->longitude - s.roadsBefore[1].longitude - east) < 1e-9 &&
					curve->rings[0].front().longitude == middle->longitude, "drag a road's end node: its junction and the other road there follow");

				NodeTool::SelectAllNodes();
				const size_t before = FindItem(s.roadItems[3])->rings[0].size();
				NodeTool::DeleteSelectedNodes();
				const ProjectFile::SceneryItem *after = FindItem(s.roadItems[3]);
				Check(after && after->rings[0].size() == 2 && before == 3, "deleting every node of a road keeps its two ends (its junctions)");
				s.wait = 4;
				s.step = Step::RoadsEndUndo;
				break;
			}

			case Step::RoadsEndUndo:
			{
				EditorHistory::Undo();	// The delete
				EditorHistory::Undo();	// The move
				bool back = s.roadsBefore.size() == 5;
				for (size_t i = 0; i < s.roadItems.size() && i < s.roadsBefore.size(); ++i)
				{
					const ProjectFile::SceneryItem *item = FindItem(s.roadItems[i]);
					back &= item && SameItem(*item, s.roadsBefore[i]);
				}
				Check(back, "undo puts the roads and their junction back bit for bit");
				EditorTools::SetTool(EditorTools::Tool::Select);
				s.wait = 4;
				s.step = Step::Delete;
				break;
			}

			case Step::Delete:
			{
				const ProjectFile::SceneryItem *item = FindItem(s.itemId);
				if (!item)
				{
					s.step = Step::Box;
					break;
				}
				s.itemBefore = *item;
				EditorSelection::Select(context, Target::ForItem(s.itemId));
				EditorTools::DeleteSelection(context);
				Check(FindItem(s.itemId) == nullptr, "delete a project item");
				s.wait = 5;
				s.step = Step::DeleteUndo;
				break;
			}

			case Step::DeleteUndo:
			{
				EditorHistory::Undo();
				const ProjectFile::SceneryItem *item = FindItem(s.itemId);
				Check(item && SameItem(*item, *s.itemBefore), "undo brings the deleted item back as it was");
				s.wait = 10;
				s.step = Step::Box;
				break;
			}

			case Step::Box:
				s.ticket = ScenePicking::Request(static_cast<int32_t>(renderWidth * 0.25f), static_cast<int32_t>(renderHeight * 0.25f), static_cast<uint32_t>(renderWidth * 0.5f), static_cast<uint32_t>(renderHeight * 0.5f));
				s.step = Step::WaitBox;
				break;

			case Step::WaitBox:
			{
				ScenePicking::Result result;
				if (!ScenePicking::TryGetResult(s.ticket, result))
					break;
				entt::registry *registry = EditorScene::GetRegistry();
				std::vector<uint32_t> ids = result.ids;
				std::sort(ids.begin(), ids.end());
				ids.erase(std::unique(ids.begin(), ids.end()), ids.end());
				std::vector<Target> targets;
				for (const uint32_t id : ids)
				{
					const entt::entity handle = registry ? ScenePicking::Resolve(*registry, id) : entt::null;
					if (handle == entt::null)
						continue;
					if (const std::optional<Target> target = EditorSelection::FromPick(EditorScene::GetID(handle)))
						if (std::ranges::none_of(targets, [&target](const Target &known) { return known.SameAs(*target); }))
							targets.push_back(*target);
				}
				EDITOR_INFO_TAG(LOG_TAG, "Box over the middle of the image: {} entities -> {} selectable targets", ids.size(), targets.size());
				Check(!targets.empty(), "box selection finds targets");
				s.step = Step::Finish;
				break;
			}

			case Step::Finish:
				if (s.picked)
					EditorSelection::Select(context, *s.picked);
				EDITOR_INFO_TAG(LOG_TAG, "Done: {} passed, {} failed. Selected '{}' for the outline", s.passed, s.failed, s.picked ? EditorSelection::Describe(*s.picked) : std::string("nothing"));
				s.step = s.showcase ? Step::Showcase : Step::Cleanup;
				break;

			case Step::Showcase:
			{
				// The pavement tested above as a B-spline, beside a painted line (or another pavement) edited with handles
				XPlane::AirportDocument *airport = EditorAirports::GetDocument();
				if (!airport || s.pavement == ~0u)
				{
					EDITOR_INFO_TAG(LOG_TAG, "Showcase: no airport pavement to show");
					s.step = Step::Done;
					break;
				}
				// The painted line or pavement nearest the tested pavement, so both fill the view
				const XPlane::AptAirport &apt = airport->airport;
				const auto center = [](const XPlane::AptNodeChain &chain)
				{
					double latitude = 0.0, longitude = 0.0;
					for (const XPlane::AptNode &node : chain.nodes)
					{
						latitude += node.latitude;
						longitude += node.longitude;
					}
					const double count = static_cast<double>(std::max<size_t>(1, chain.nodes.size()));
					return std::pair{ latitude / count, longitude / count };
				};
				const auto [baseLatitude, baseLongitude] = center(apt.pavements[s.pavement].rings[0]);
				double nearest = 1e30;
				const auto consider = [&](const AirportElementKind kind, const uint32_t index, const std::vector<XPlane::AptNodeChain> &chains)
				{
					if (chains.empty() || chains[0].nodes.size() < 3)
						return;
					const auto [latitude, longitude] = center(chains[0]);
					const double distance = std::hypot(latitude - baseLatitude, (longitude - baseLongitude) * std::cos(baseLatitude * 0.017453292519943295));
					if (distance < nearest)
					{
						nearest = distance;
						s.showcaseOther = Target::ForElement(kind, index);
					}
				};
				for (uint32_t i = 0; i < apt.linearFeatures.size(); ++i)
					consider(AirportElementKind::LinearFeature, i, apt.linearFeatures[i].chains);
				for (uint32_t i = 0; i < apt.pavements.size(); ++i)
					if (i != s.pavement)
						consider(AirportElementKind::Pavement, i, apt.pavements[i].rings);
				if (s.roadItems.size() == 5 && FindItem(s.roadItems[3]))
					s.showcaseOther = Target::ForItem(s.roadItems[3]);	// The road the road tests made, drawn from roads.net

				const Target pavement = Target::ForElement(AirportElementKind::Pavement, s.pavement);
				s.showcaseRows.clear();
				for (const std::optional<Target> &target : { std::optional<Target>(pavement), s.showcaseOther })
					if (target && target->type == EditorSelection::TargetType::AirportElement)
						s.showcaseRows.emplace_back(*target, AirportEdit::Capture(apt, target->kind, target->index));
				EditorTools::SetTool(EditorTools::Tool::Nodes);
				EditorSelection::Select(context, pavement);
				s.wait = 2;
				s.step = Step::ShowcaseSpline;
				break;
			}

			case Step::ShowcaseSpline:
			{
				NodeTool::SelectAllNodes();
				const uint64_t revision = EditorHistory::GetRevision();
				NodeTool::SetBSpline(true);
				s.showcaseEdits += EditorHistory::GetRevision() != revision ? 1 : 0;
				NodeTool::ClearNodeSelection();
				std::vector<Target> targets = { Target::ForElement(AirportElementKind::Pavement, s.pavement) };
				if (s.showcaseOther)
					targets.push_back(*s.showcaseOther);
				EditorSelection::Select(context, targets);
				s.wait = 3;
				s.step = Step::ShowcaseNodes;
				break;
			}

			case Step::ShowcaseNodes:
			{
				// A smooth node selected on the second shape: its handles and the node shapes show
				if (s.showcaseOther && NodeTool::SelectNode(1, 0, 1, false))
				{
					const uint64_t revision = EditorHistory::GetRevision();
					NodeTool::SetSelectedNodeType(Splines::NodeType::Smooth);
					s.showcaseEdits += EditorHistory::GetRevision() != revision ? 1 : 0;
				}
				EditorTools::FrameSelection();
				s.holdFrames = 2400;
				EDITOR_INFO_TAG(LOG_TAG, "Showcase: '{}' as a B-spline beside '{}' with a smooth node selected, for {} frames (from frame {})",
					EditorSelection::Describe(Target::ForElement(AirportElementKind::Pavement, s.pavement)), s.showcaseOther ? EditorSelection::Describe(*s.showcaseOther) : std::string("nothing"),
					s.holdFrames, s.frames);
				s.step = Step::ShowcaseHold;
				break;
			}

			case Step::ShowcaseHold:
				if (--s.holdFrames <= 0)
					s.step = Step::ShowcaseUndo;
				break;

			case Step::ShowcaseUndo:
			{
				for (int i = 0; i < s.showcaseEdits; ++i)
					EditorHistory::Undo();
				XPlane::AirportDocument *airport = EditorAirports::GetDocument();
				bool restored = airport != nullptr;
				for (const auto &[target, row] : s.showcaseRows)
				{
					const std::vector<XPlane::AptNodeChain> *now = airport ? AirportEdit::GetChains(airport->airport, target.kind, target.index) : nullptr;
					const std::vector<XPlane::AptNodeChain> *before = ShapeChains(row);
					restored &= now && before && SameChains(*now, *before);
				}
				Check(restored, "the showcase's edits are undone (" + std::to_string(s.showcaseEdits) + " steps)");
				NodeTool::ClearNodeSelection();
				EditorTools::SetTool(EditorTools::Tool::Select);
				if (s.picked)
					EditorSelection::Select(context, *s.picked);
				EDITOR_INFO_TAG(LOG_TAG, "Showcase finished");
				s.step = Step::Cleanup;
				break;
			}

			case Step::Cleanup:
			{
				// The road tests' items go (the project is reopened from its file just after anyway, when it can be)
				for (auto it = s.roadItems.rbegin(); it != s.roadItems.rend(); ++it)
					if (FindItem(*it))
						EditorProjects::RemoveItem(*it, context);
				s.roadItems.clear();
				// Every edit above was undone, but the project still counts as edited: open it again from its file so
				// the run leaves no recovery data behind (unless unsaved work from an earlier session is waiting)
				s.step = Step::Done;
				if (!EditorProjects::IsDirty())
					break;
				if (s.failed > 0 || s.hadRecovery)
				{
					EDITOR_WARN_TAG(LOG_TAG, "The project stays marked as edited ({})", s.failed > 0 ? "checks failed: kept for a look" : "it had unsaved work from an earlier session");
					break;
				}
				if (EditorProjects::Revert(context))
				{
					EDITOR_INFO_TAG(LOG_TAG, "Reopened the project from its file: the test leaves no unsaved changes");
					s.wait = 30;
					s.step = Step::Reselect;
				}
				break;
			}

			case Step::Reselect:
				if (s.picked)
					EditorSelection::Select(context, *s.picked);
				s.step = Step::Done;
				break;

			case Step::Done:
				break;
		}
	}
}

// -------------------------------------------------------
