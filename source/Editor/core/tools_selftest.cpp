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
#include "asset_thumbnails.h"
#include "editor_airports.h"
#include "editor_camera.h"
#include "editor_history.h"
#include "editor_project.h"
#include "editor_scene.h"
#include "editor_selection.h"
#include "editor_tools.h"
#include "facade_authoring.h"
#include "facade_tool.h"
#include "node_tool.h"
#include "transform_tool.h"
#include "viewport_menus.h"
#include <algorithm>
#include <chrono>
#include <cmath>
#include <map>
#include <optional>
#include <SceneryEditorX/logging/logging.hpp>
#include <SceneryEditorX/renderer/gbuffer.h>
#include <SceneryEditorX/renderer/gpu_scene.h>
#include <SceneryEditorX/renderer/scene_picking.h>
#include <SceneryEditorX/scene/components/renderable.h>
#include <SceneryEditorX/filesystem/vfs.h>
#include <SceneryEditorX/xplane/xp_facade_writer.h>
#include <SceneryEditorX/xplane/xp_library_manager.h>
#include <SceneryEditorX/xplane/xp_texture_loader.h>
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
			OrthoTop,
			OrthoPick,
			OrthoWaitPick,
			LibraryFilters,
			LargePolygonCreate,
			LargePolygonSelect,
			LargePolygonEdit,
			LargePolygonCheck,
			Thumbnails,
			ThumbnailsWait,
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
			FacadesCreate,
			FacadesDrawn,
			FacadesEdit,
			FacadeShowcase,
			FacadeShowcaseMenu,
			FacadeShowcaseHold,
			FacadeHotboxHold,
			AuthorBuilding,
			AuthorBuildingDrawn,
			AuthorBuildingLive,
			AuthorCurb,
			AuthorCurbDrawn,
			AuthorType2Copy,
			AuthorType2Drawn,
			FacadesUndo,
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
			std::optional<ViewportCamera::Rig> rigBefore;
			std::string largeItem;					// A ~3000 node polygon (editing big shapes stays smooth)
			int largeEdits = 0;
			float largeWorstFrame = 0.0f;			// Seconds, the slowest frame while editing it
			uint32_t largeVerticesBefore = 0;		// Global geometry buffer size before the edits
			std::vector<std::filesystem::path> thumbnailFiles;	// One library asset of each kind with a thumbnail
			int thumbnailFrames = 0;
			uint32_t largeVerticesAfterAdd = 0;	// The view before the orthographic steps (put back after)
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

			std::vector<std::string> facadeItems;	// The curved sidewalk, the skewed outline, the hangar door line, then what the edits added
			size_t itemsBefore = 0;					// Project items before the facade tests
			int facadeEdits = 0;					// Undo steps the facade tests pushed
			double doorLength = 0.0;				// The door facade's first natural length
			bool facadeShowcase = false;			// --facade-showcase
			std::vector<std::filesystem::path> authoredFiles;	// Files the authoring steps wrote (removed at the end)
			std::filesystem::path authoredFolder;	// The project's facades folder when the test created it
			float authoredScale = 0.0f;				// The building's SCALE after the live edit
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
		void CheckPicks(const ViewportView &view, const ScenePicking::Result &result, const float renderWidth, const float renderHeight, const bool remember = true)
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

			if (!remember)
				return;

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

	void EnableFacadeShowcase()
	{
		S().facadeShowcase = true;
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
		// Every frame while the large polygon is edited counts, waiting ones included (that is when its scenery rebuilds)
		if (s.step == Step::LargePolygonEdit || s.step == Step::LargePolygonCheck)
			s.largeWorstFrame = std::max(s.largeWorstFrame, ImGui::GetIO().DeltaTime);
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
				s.step = Step::OrthoTop;
				break;
			}

			case Step::OrthoTop:
			{
				// Numpad 7 then 5: straight down, orthographic. The renderer's projection must match the tools' (picks line up)
				ViewportCamera::Rig rig;
				if (!EditorCamera::GetRig(rig))
				{
					s.step = Step::ItemMove;
					break;
				}
				s.rigBefore = rig;
				double yaw = 0.0, pitch = 0.0;
				ViewportCamera::GetPresetDirection(ViewportCamera::ViewPreset::Top, yaw, pitch);
				rig.yaw = yaw;
				rig.pitch = pitch;
				rig.preset = ViewportCamera::ViewPreset::Top;
				rig.orthographic = true;
				EditorCamera::SetRig(rig);
				s.pickAttempts = 0;
				s.wait = 90;
				s.step = Step::OrthoPick;
				break;
			}

			case Step::OrthoPick:
			{
				Check(view.orthographic && EditorCamera::IsProjection(CameraType::Orthographic) && EditorCamera::GetViewPreset() == ViewportCamera::ViewPreset::Top,
					"orthographic top view (numpad 7 + 5)");
				Check(std::abs(view.forward.y + 1.0) < 1e-6 && std::abs(view.up.z + 1.0) < 1e-6, "the top view looks straight down with north up");

				// A ground point under the middle of the image comes back to the same pixel, and a meter covers the same
				// pixels wherever it is (parallel projection)
				const ImVec2 middle(view.imageMin.x + view.imageSize.x * 0.5f, view.imageMin.y + view.imageSize.y * 0.5f);
				Point3 ground;
				ImVec2 back;
				const bool hit = view.RayToPlane(middle, 0.0, ground) && view.WorldToScreen(ground, back);
				Check(hit && std::abs(back.x - middle.x) < 0.5f && std::abs(back.y - middle.y) < 0.5f, "orthographic rays and projection agree");
				Check(std::abs(view.PixelsPerMeter(ground) - view.PixelsPerMeter({ ground.x + 500.0, ground.y + 30.0, ground.z })) < 1e-9, "orthographic scale is the same everywhere");

				s.ticket = ScenePicking::Request(0, 0, static_cast<uint32_t>(renderWidth), static_cast<uint32_t>(renderHeight));
				s.step = Step::OrthoWaitPick;
				break;
			}

			case Step::OrthoWaitPick:
			{
				ScenePicking::Result result;
				if (!ScenePicking::TryGetResult(s.ticket, result))
				{
					if (s.frames > 12000)
					{
						Check(false, "orthographic pick readback arrived");
						s.step = Step::ItemMove;
					}
					break;
				}
				if (std::ranges::none_of(result.ids, [](const uint32_t id) { return id != 0; }) && ++s.pickAttempts < 4)
				{
					s.wait = 120;
					s.ticket = ScenePicking::Request(0, 0, static_cast<uint32_t>(renderWidth), static_cast<uint32_t>(renderHeight));
					break;
				}
				EDITOR_INFO_TAG(LOG_TAG, "Orthographic top view: {:.0f} x {:.0f} m", view.halfWidth * 2.0, view.halfHeight * 2.0);
				CheckPicks(view, result, renderWidth, renderHeight, false);
				if (s.rigBefore)
					EditorCamera::SetRig(*s.rigBefore);
				s.wait = 60;
				s.step = Step::LibraryFilters;
				break;
			}

			case Step::LibraryFilters:
			{
				// A library-browser filter saved in the project: an undoable edit that marks the project changed
				ProjectFile::ProjectDocument *document = EditorProjects::GetDocument();
				s.step = Step::ItemMove;
				if (!document)
					break;
				const std::vector<ProjectFile::LibraryFilter> before = document->GetProject().libraryFilters;
				const uint64_t revision = document->GetRevision();
				ProjectFile::LibraryFilter filter;
				filter.name = "SelfTest Sidewalks";
				filter.search = "sidewalk";
				filter.types = { "fac", "lin" };
				std::vector<ProjectFile::LibraryFilter> saved = before;
				saved.push_back(filter);
				Check(EditorProjects::SetLibraryFilters(saved, "Save library filter SelfTest Sidewalks"), "library filters: saved in the open project");
				Check(document->GetProject().libraryFilters == saved && document->GetRevision() != revision && document->IsDirty(), "library filters: the project holds it and has unsaved changes");
				Check(EditorHistory::Undo() && document->GetProject().libraryFilters == before, "library filters: undo takes it back out");
				Check(EditorHistory::Redo() && document->GetProject().libraryFilters == saved, "library filters: redo puts it back");
				EditorHistory::Undo();
				s.step = Step::LargePolygonCreate;
				break;
			}

			case Step::LargePolygonCreate:
			{
				// A big draped polygon: a 300 m circle of 3000 nodes 700 m north of the scene's center
				s.step = Step::ItemMove;
				if (!EditorProjects::GetDocument())
					break;
				ProjectFile::SceneryItem item;
				item.kind = ProjectFile::ItemKind::Polygon;
				item.name = "SelfTest Large Polygon";
				item.resource = "lib/airport/ground/pavement/asphalt_L/plain.pol";
				std::vector<ProjectFile::GeoNode> ring;
				constexpr int NODES = 3000;
				for (int i = 0; i < NODES; ++i)
				{
					const double angle = 6.283185307179586 * i / NODES;
					ProjectFile::GeoNode node;
					projectProjection.ToGeo(XPlane::Point2{ 300.0 * std::sin(angle), -700.0 - 300.0 * std::cos(angle) }, node.latitude, node.longitude);
					ring.push_back(node);
				}
				item.rings.push_back(std::move(ring));
				s.largeVerticesBefore = GeometryBuffer::GetVertexCount();
				s.largeItem = EditorProjects::AddItem(item, context, "Add SelfTest Large Polygon");
				Check(!s.largeItem.empty(), "large polygon: a 3000 node polygon added to the project");
				s.largeEdits = 0;
				s.largeWorstFrame = 0.0f;
				s.wait = 60;
				s.step = s.largeItem.empty() ? Step::ItemMove : Step::LargePolygonSelect;
				break;
			}

			case Step::LargePolygonSelect:
			{
				s.largeVerticesAfterAdd = GeometryBuffer::GetVertexCount();
				EditorSelection::Select(context, Target::ForItem(s.largeItem));
				EditorTools::SetTool(EditorTools::Tool::Nodes);
				s.largeWorstFrame = 0.0f;
				s.wait = 20;
				s.step = Step::LargePolygonEdit;
				break;
			}

			case Step::LargePolygonEdit:
			{
				// Eight node moves a few frames apart, as quick clicks and drags would make; the slowest frame is kept
				s.largeWorstFrame = std::max(s.largeWorstFrame, ImGui::GetIO().DeltaTime);
				if (s.wait == 0 && s.largeEdits < 8)
				{
					const auto start = std::chrono::steady_clock::now();
					NodeTool::SelectNode(0, 0, static_cast<uint32_t>(s.largeEdits * 300), false);
					NodeTool::TranslateSelectedNodes(2.0, 1.0);
					EDITOR_INFO_TAG(LOG_TAG, "Large polygon edit {}: {:.1f} ms in the node tool", s.largeEdits + 1,
						std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - start).count());
					++s.largeEdits;
					s.wait = 8;
					break;
				}
				if (s.largeEdits >= 8)
				{
					s.wait = 30;
					s.step = Step::LargePolygonCheck;
				}
				break;
			}

			case Step::LargePolygonCheck:
			{
				const uint32_t vertices = GeometryBuffer::GetVertexCount();
				const uint32_t added = s.largeVerticesAfterAdd > s.largeVerticesBefore ? s.largeVerticesAfterAdd - s.largeVerticesBefore : 0;
				const uint32_t grown = vertices > s.largeVerticesAfterAdd ? vertices - s.largeVerticesAfterAdd : 0;
				EDITOR_INFO_TAG(LOG_TAG, "Large polygon: slowest frame {:.1f} ms over 8 edits; geometry buffer {} -> {} (+{} for the polygon) -> {} vertices", s.largeWorstFrame * 1000.0f,
					s.largeVerticesBefore, s.largeVerticesAfterAdd, added, vertices);
				Check(s.largeWorstFrame < 0.25f, "large polygon: no frame over 250 ms while editing its nodes");
				Check(grown <= added + added / 2, "large polygon: edits reuse the scene's geometry space instead of piling it up");
				for (int i = 0; i < s.largeEdits; ++i)
					EditorHistory::Undo();
				EditorHistory::Undo();	// The add
				Check(FindItem(s.largeItem) == nullptr, "large polygon: undo takes the edits and the polygon back out");
				EditorTools::SetTool(EditorTools::Tool::Select);
				EditorSelection::Clear(context);
				s.wait = 30;
				s.step = Step::Thumbnails;
				break;
			}

			case Step::Thumbnails:
			{
				// One library asset of each kind that has a thumbnail
				s.step = Step::ItemMove;
				s.thumbnailFiles.clear();
				const std::shared_ptr<const XPlane::LibraryIndex> index = XPlane::LibraryManager::GetIndex();
				if (!index)
					break;
				for (const XPlane::AssetType type : { XPlane::AssetType::Object, XPlane::AssetType::Facade, XPlane::AssetType::Forest, XPlane::AssetType::Polygon,
					XPlane::AssetType::Line, XPlane::AssetType::String, XPlane::AssetType::AutogenPoint })
				{
					for (const XPlane::LibraryAsset &asset : index->GetAssets())
					{
						const XPlane::AssetProvider *resolved = asset.GetResolved();
						if (asset.type == type && resolved && resolved->fileExists && !asset.IsPrivate() && !asset.IsDeprecated())
						{
							s.thumbnailFiles.push_back(resolved->realPath);
							break;
						}
					}
				}
				if (s.thumbnailFiles.empty())
				{
					EDITOR_INFO_TAG(LOG_TAG, "No library assets: skipping the thumbnail tests");
					break;
				}
				s.thumbnailFrames = 0;
				s.step = Step::ThumbnailsWait;
				break;
			}

			case Step::ThumbnailsWait:
			{
				// Asked for every frame, as a panel showing them does (requests nobody draws are dropped)
				size_t ready = 0, failed = 0;
				std::string errors;
				for (const std::filesystem::path &file : s.thumbnailFiles)
				{
					AssetThumbnails::Thumbnail thumbnail;
					const AssetThumbnails::State state = AssetThumbnails::Get(file, thumbnail);
					ready += state == AssetThumbnails::State::Ready && thumbnail.texture != ImTextureID_Invalid ? 1 : 0;
					if (state == AssetThumbnails::State::Failed)
					{
						++failed;
						errors += "\n  " + VFS::ToUtf8(file.filename()) + ": " + AssetThumbnails::GetError(file);
					}
				}
				if (ready + failed < s.thumbnailFiles.size() && ++s.thumbnailFrames < 1200)
					break;

				const AssetThumbnails::Stats stats = AssetThumbnails::GetStats();
				size_t cached = 0;
				std::error_code ec;
				for (std::filesystem::directory_iterator it(VFS::GetAppLocation(VFS::AppLocation::Cache) / "thumbnails", ec), end; !ec && it != end; it.increment(ec))
					cached += it->path().extension() == ".png" ? 1 : 0;
				EDITOR_INFO_TAG(LOG_TAG, "Thumbnails: {} of {} drawn, {} failed, in {} frames ({} rendered in {:.0f} ms, {} from the disk cache, {} cached files){}", ready,
					s.thumbnailFiles.size(), failed, s.thumbnailFrames, stats.rendered, stats.renderMilliseconds, stats.fromDisk, cached, errors);
				Check(ready + failed == s.thumbnailFiles.size(), "thumbnails: every request finished");
				Check(ready + 1 >= s.thumbnailFiles.size(), "thumbnails: library objects, facades, forests, polygons, lines and strings are drawn");
				Check(cached >= ready, "thumbnails: kept in the disk cache");
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
				s.step = Step::FacadesCreate;
				break;
			}

			case Step::FacadesCreate:
			{
				// X-Plane's 3D sidewalk (lib/airport/ground/sidewalk_15cm) drawn with a curved front, a skewed outline to
				// square, and a hangar door (a RING 0 line) at its natural length, 100 m south of the tested pavement
				ProjectFile::ProjectDocument *document = EditorProjects::GetDocument();
				if (!document)
				{
					EDITOR_INFO_TAG(LOG_TAG, "No project: skipping the facade tests");
					s.step = Step::Delete;
					break;
				}
				s.facadeItems.clear();
				s.facadeEdits = 0;
				s.itemsBefore = document->GetProject().CountItems();
				XPlane::Point2 base{ 0.0, 0.0 };
				XPlane::AirportDocument *airport = EditorAirports::GetDocument();
				if (airport && s.pavement != ~0u)
				{
					double latitude = 0.0, longitude = 0.0;
					const XPlane::AptNodeChain &ring = airport->airport.pavements[s.pavement].rings[0];
					for (const XPlane::AptNode &node : ring.nodes)
					{
						latitude += node.latitude / static_cast<double>(ring.nodes.size());
						longitude += node.longitude / static_cast<double>(ring.nodes.size());
					}
					base = projectProjection.ToLocal(latitude, longitude);
				}
				const double x0 = base.x - 40.0, z0 = base.z + 100.0;

				const std::string sidewalk = "lib/airport/ground/sidewalk_15cm/sw_concrete.fac";
				const std::string door = "lib/airport/hangars/door/Uni/gray_1.fac";
				const XPlane::FacadeAsset *sidewalkAsset = FacadeTool::GetAsset(sidewalk);
				const XPlane::FacadeAsset *doorAsset = FacadeTool::GetAsset(door);
				Check(sidewalkAsset && XPlane::GetFacadeWallCount(*sidewalkAsset) == 7 && XPlane::GetFacadeFamily(*sidewalkAsset) == XPlane::FacadeFamily::Ground,
					"X-Plane's 3D sidewalk facade loads: 7 curb wall types, a ground facade");
				const std::vector<double> natural = doorAsset ? XPlane::GetFacadeNaturalLengths(*doorAsset, XPlane::ChooseFacadeFloor(*doorAsset, 4.0), 0) : std::vector<double>();
				s.doorLength = 0.0;
				for (const double length : natural)
				{
					if (length >= 8.0)
					{
						s.doorLength = length;
						break;
					}
				}
				Check(doorAsset && !doorAsset->ring && s.doorLength > 0.0, "a hangar door facade is a line with natural wall lengths (" + std::to_string(s.doorLength) + " m)");

				// The sidewalk's first wall bows 6 m south
				Splines::Chain curved = FacadeEdit::MakeChain({ { x0, z0 }, { x0 + 30.0, z0 }, { x0 + 30.0, z0 - 8.0 }, { x0, z0 - 8.0 } }, true);
				curved.nodes[0].hasOut = true;
				curved.nodes[0].handleOut = { x0 + 8.0, z0 + 6.0 };
				curved.nodes[0].split = true;
				curved.nodes[1].hasIn = true;
				curved.nodes[1].handleIn = { x0 + 22.0, z0 + 6.0 };
				curved.nodes[1].split = true;
				const Splines::Chain skewed = FacadeEdit::MakeChain({ { x0 + 40.0, z0 }, { x0 + 60.0, z0 + 0.6 }, { x0 + 59.4, z0 - 12.0 }, { x0 + 39.7, z0 - 12.3 } }, true);
				const Splines::Chain line = FacadeEdit::MakeChain({ { x0 + 70.0, z0 }, { x0 + 70.0 + std::max(1.0, s.doorLength), z0 } }, false);
				const uint64_t history = EditorHistory::GetRevision();
				s.facadeItems.push_back(FacadeTool::Create(context, sidewalk, curved, 0.15f, "SelfTest sidewalk"));
				s.facadeItems.push_back(FacadeTool::Create(context, sidewalk, skewed, 0.15f, "SelfTest outline"));
				s.facadeItems.push_back(FacadeTool::Create(context, door, line, 4.0f, "SelfTest door"));
				s.facadeEdits = 3;
				const bool created = std::ranges::all_of(s.facadeItems, [](const std::string &id) { return FindItem(id) != nullptr; });
				Check(created && EditorHistory::GetRevision() == history + 3, "draw a curved sidewalk, a skewed outline and a hangar door line (3 undo steps)");
				s.wait = 150;	// The project scenery rebuilds
				s.step = created ? Step::FacadesDrawn : Step::FacadesUndo;
				break;
			}

			case Step::FacadesDrawn:
			{
				Check(std::ranges::all_of(s.facadeItems, [](const std::string &id) { return EditorProjects::GetPlacedEntity(id) != 0; }), "the facades are drawn (an entity each)");
				const std::vector<FacadeEdit::WallInfo> doorWalls = FacadeTool::GetWalls(s.facadeItems[2]);
				Check(doorWalls.size() == 1 && std::abs(doorWalls[0].length - s.doorLength) < 0.01, "the door line is one wall of its natural length");
				s.step = Step::FacadesEdit;
				break;
			}

			case Step::FacadesEdit:
			{
				const std::string sidewalk = s.facadeItems[0], outline = s.facadeItems[1], door = s.facadeItems[2];
				const auto counted = [&s](const bool done)
				{
					s.facadeEdits += done ? 1 : 0;
					return done;
				};

				// A curb color on the curved wall: walls become picked
				const bool typed = counted(FacadeTool::SetWallTypes({ sidewalk }, { 0 }, 2));
				const ProjectFile::SceneryItem *item = FindItem(sidewalk);
				Check(typed && item && item->pickWalls && item->rings[0][0].wallType == 2, "set the curved wall's type (white_curb): the facade's walls become picked");

				// The 3D sidewalk article's recipe: the curve as ~0.65 m straight pieces, each keeping the curb
				const size_t before = item ? item->rings[0].size() : 0;
				const bool cut = counted(FacadeTool::SubdivideCurves({ sidewalk }, 0.65f));
				item = FindItem(sidewalk);
				const std::vector<FacadeEdit::WallInfo> walls = FacadeTool::GetWalls(sidewalk);
				const bool straight = std::ranges::none_of(walls, [](const FacadeEdit::WallInfo &wall) { return wall.curved; });
				const size_t white = item ? static_cast<size_t>(std::ranges::count_if(item->rings[0], [](const ProjectFile::GeoNode &node) { return node.wallType == 2; })) : 0;
				Check(cut && straight && item && item->rings[0].size() > before + 30 && white + 3 == item->rings[0].size(),
					"subdivide the curved wall into ~0.65 m straight pieces, each keeping its white curb (" + std::to_string(item ? item->rings[0].size() : 0) + " nodes)");

				// The skewed outline squared, its third wall made the front, then reversed
				const bool squared = counted(FacadeTool::SquareCorners({ outline }));
				std::vector<FacadeEdit::WallInfo> outlineWalls = FacadeTool::GetWalls(outline);
				bool right = outlineWalls.size() == 4;
				for (size_t i = 0; right && i < 4; ++i)
				{
					const FacadeEdit::WallInfo &a = outlineWalls[i], &b = outlineWalls[(i + 1) % 4];
					const double dot = ((a.end.x - a.start.x) * (b.end.x - b.start.x) + (a.end.z - a.start.z) * (b.end.z - b.start.z)) / (a.length * b.length);
					right = std::abs(dot) < 1e-6;
				}
				Check(squared && right, "square the skewed outline's corners");
				const double heading = outlineWalls.size() == 4 ? outlineWalls[2].heading : -1.0;
				const bool front = counted(FacadeTool::SetFrontWall(outline, 2));
				outlineWalls = FacadeTool::GetWalls(outline);
				Check(front && !outlineWalls.empty() && std::abs(outlineWalls[0].heading - heading) < 1e-6, "make the outline's third wall its front");
				const bool reversed = counted(FacadeTool::Reverse({ outline }));
				Check(reversed && FacadeTool::GetWalls(outline).size() == 4, "reverse the outline");

				// The asphalt sidewalk instead: the curbs keep their names
				const std::string asphalt = "lib/airport/ground/sidewalk_15cm/sw_asphalt.fac";
				const bool replaced = counted(FacadeTool::Replace({ sidewalk }, asphalt));
				item = FindItem(sidewalk);
				const XPlane::FacadeAsset *asphaltAsset = FacadeTool::GetAsset(asphalt);
				const std::vector<std::string> names = asphaltAsset ? XPlane::GetFacadeWallNames(*asphaltAsset) : std::vector<std::string>();
				const bool kept = item && !item->rings[0].empty() && static_cast<size_t>(item->rings[0][0].wallType) < names.size() && names[static_cast<size_t>(item->rings[0][0].wallType)] == "white_curb";
				Check(replaced && item && item->resource == asphalt && kept, "replace with the asphalt sidewalk: walls keep their curbs by name");

				// A sidewalk around the outline, and a copy of the door
				const std::vector<std::string> around = FacadeTool::AddAround(context, { outline }, "lib/airport/ground/sidewalk_15cm/sw_concrete.fac", 0.5f);
				s.facadeEdits += static_cast<int>(around.size());
				s.facadeItems.insert(s.facadeItems.end(), around.begin(), around.end());
				bool outward = false;
				if (around.size() == 1)
				{
					const ProjectFile::SceneryItem *ring = FindItem(around[0]);
					Splines::Shape shape;
					if (ring)
						shape = Splines::FromGeoRings({ ring->rings[0] }, true, projectProjection);
					outward = ring && !shape.chains.empty() && FacadeEdit::GetSignedArea(shape.chains[0]) > 200.0;
				}
				Check(around.size() == 1 && outward, "add a sidewalk around the outline (counter-clockwise, its curbs facing out)");
				const std::vector<std::string> copies = FacadeTool::Duplicate(context, { door });
				s.facadeEdits += static_cast<int>(copies.size());
				s.facadeItems.insert(s.facadeItems.end(), copies.begin(), copies.end());
				Check(copies.size() == 1 && FindItem(copies[0]) && FindItem(copies[0])->resource == FindItem(door)->resource, "duplicate the door");

				// Then both context menus are drawn on them (held for frame captures with --facade-showcase)
				s.wait = s.facadeShowcase ? 150 : 30;
				s.step = Step::FacadeShowcase;
				break;
			}

			case Step::FacadeShowcase:
			{
				// The facade tool on the subdivided sidewalk and the sidewalk around the outline: wall labels, picked walls
				FacadeTool::SetResource("lib/airport/ground/sidewalk_15cm/sw_concrete.fac");
				EditorTools::SetTool(EditorTools::Tool::Facade);
				std::vector<Target> targets = { Target::ForItem(s.facadeItems[0]) };
				if (s.facadeItems.size() > 3)
					targets.push_back(Target::ForItem(s.facadeItems[3]));
				EditorSelection::Select(context, targets);
				FacadeTool::SelectWalls(s.facadeItems[0], { 0, 1, 2, 3 });
				EditorTools::FrameSelection();
				s.wait = s.facadeShowcase ? 60 : 10;
				s.step = Step::FacadeShowcaseMenu;
				break;
			}

			case Step::FacadeShowcaseMenu:
			{
				// The quad menu, opened on the sidewalk's back wall as a right-click there would
				const std::vector<FacadeEdit::WallInfo> walls = FacadeTool::GetWalls(s.facadeItems[0]);
				ImVec2 at(view.imageMin.x + view.imageSize.x * 0.5f, view.imageMin.y + view.imageSize.y * 0.5f);
				if (walls.size() > 2)
				{
					const FacadeEdit::WallInfo &wall = walls[walls.size() - 2];
					view.WorldToScreen({ (wall.start.x + wall.end.x) * 0.5, 0.15, (wall.start.z + wall.end.z) * 0.5 }, at);
				}
				Point3 ground;
				const bool onGround = view.RayToPlane(at, 0.0, ground);
				ViewportMenus::OpenQuadMenu(context, at, Target::ForItem(s.facadeItems[0]), onGround ? std::optional<Point3>(ground) : std::nullopt);
				s.holdFrames = s.facadeShowcase ? 1200 : 10;
				if (s.facadeShowcase)
					EDITOR_INFO_TAG(LOG_TAG, "Facade showcase: wall labels and the quad menu on the sidewalk for {} frames (from frame {})", s.holdFrames, s.frames);
				s.step = Step::FacadeShowcaseHold;
				break;
			}

			case Step::FacadeShowcaseHold:
				if (--s.holdFrames > 0)
					break;
				Check(ViewportMenus::IsQuadMenuOpen(), "the quad menu opens on the sidewalk (facade panels) and stays up while drawn");
				ViewportMenus::CloseMenus();
				// The hotbox, as holding Space over the viewport shows it
				ViewportMenus::PinHotbox(context, ImVec2(view.imageMin.x + view.imageSize.x * 0.5f, view.imageMin.y + view.imageSize.y * 0.5f));
				s.holdFrames = s.facadeShowcase ? 1200 : 10;
				if (s.facadeShowcase)
					EDITOR_INFO_TAG(LOG_TAG, "Facade showcase: the hotbox for {} frames (from frame {})", s.holdFrames, s.frames);
				s.step = Step::FacadeHotboxHold;
				break;

			case Step::FacadeHotboxHold:
				if (--s.holdFrames > 0)
					break;
				Check(ViewportMenus::IsHotboxOpen(), "the hotbox shows its rows (menus, recent commands and facades, the selection's menus)");
				ViewportMenus::CloseMenus();
				FacadeTool::SelectWalls({}, {});
				EditorTools::SetTool(EditorTools::Tool::Select);
				s.wait = 4;
				s.step = Step::AuthorBuilding;
				break;

			case Step::AuthorBuilding:
			{
				// A new type 1 building from a texture (the hangar door's, from the X-Plane library), saved into the
				// project's facades folder, and a preview footprint built with it
				namespace fs = std::filesystem;
				const fs::path door = EditorProjects::ResolveResourceFile("lib/airport/hangars/door/Uni/gray_1.fac");
				XPlane::FacadeAsset doorAsset;
				fs::path texture;
				if (!door.empty() && XPlane::LoadFacadeAsset(door, doorAsset))
					texture = XPlane::TextureLoader::ResolveTexturePath(door.parent_path(), doorAsset.wallTexture);
				const fs::path folder = EditorProjects::GetProjectFolder() / "facades";
				if (!fs::exists(folder))
					s.authoredFolder = folder;
				const fs::path copied = texture.empty() ? fs::path() : folder / texture.filename();
				const bool copiedBefore = !copied.empty() && fs::exists(copied);
				std::string error;
				const bool made = !texture.empty() && FacadeAuthoring::NewBuilding(texture, &error);
				if (made && !copiedBefore)
					s.authoredFiles.push_back(copied);
				Check(made && FacadeAuthoring::GetKind() == FacadeAuthoring::Kind::Building && FacadeAuthoring::GetProblems().empty(),
					"author a type 1 building from a texture (" + VFS::ToUtf8(texture.filename()) + (error.empty() ? std::string() : ": " + error) + ")");
				if (!made)
				{
					s.step = Step::FacadesUndo;
					break;
				}
				FacadeAuthoring::SetName("SelfTest_building");
				XPlane::FacadeAsset &asset = FacadeAuthoring::GetAsset();
				// Narrower end panels: the left / center edge moves 32 pixels, both panels follow
				XPlane::FacadeAsset::Wall &wall = asset.walls[0];
				const float from = wall.leftPanels[0].second;
				const float to = XPlane::FacadeFromPixels(std::round(from * wall.textureWidth) - 32.0f, wall.textureWidth);
				const float moved = XPlane::MoveFacadeTextureCut(wall, false, from, to);
				FacadeAuthoring::MarkEdited();
				Check(moved == to && wall.centerPanels[0].first == to, "drag the left panel's edge: the center panel follows");

				const bool saved = FacadeAuthoring::Save(&error);
				s.authoredFiles.push_back(FacadeAuthoring::GetFilePath());
				XPlane::FacadeAsset reread;
				const bool read = saved && XPlane::LoadFacadeAsset(FacadeAuthoring::GetFilePath(), reread);
				Check(read && XPlane::WriteFacadeAsset(reread) == XPlane::WriteFacadeAsset(asset), "save it into the project's facades folder and read it back the same");
				FacadeAuthoring::RequestOpen();
				const uint64_t history = EditorHistory::GetRevision();
				const std::string preview = FacadeAuthoring::PlacePreview(context);
				if (!preview.empty())
				{
					s.facadeItems.push_back(preview);
					++s.facadeEdits;
				}
				Check(!preview.empty() && EditorHistory::GetRevision() == history + 1 && FacadeAuthoring::GetPreviewItem() == preview,
					"place a preview footprint built with it (one undo step)");
				if (s.facadeShowcase && !preview.empty())
				{
					// Held for frame captures: the FACADE EDITOR panel beside the framed preview
					EditorSelection::Select(context, Target::ForItem(preview));
					EditorTools::FrameSelection();
					EDITOR_INFO_TAG(LOG_TAG, "Facade showcase: the facade editor and its preview for 1200 frames (from frame {})", s.frames);
				}
				s.wait = s.facadeShowcase ? 1200 : 150;	// The project scenery rebuilds
				s.step = Step::AuthorBuildingDrawn;
				break;
			}

			case Step::AuthorBuildingDrawn:
			{
				const std::string &preview = FacadeAuthoring::GetPreviewItem();
				Check(!preview.empty() && EditorProjects::GetPlacedEntity(preview) != 0, "the authored building is drawn");
				// Live preview: a change is saved (and redrawn) shortly after it is made
				FacadeAuthoring::LiveSave() = true;
				XPlane::FacadeAsset::Wall &wall = FacadeAuthoring::GetAsset().walls[0];
				wall.scaleS *= 0.5f;
				wall.scaleT *= 0.5f;
				s.authoredScale = wall.scaleS;
				FacadeAuthoring::MarkEdited();
				s.wait = 200;	// The delayed save, then the scenery rebuild
				s.step = Step::AuthorBuildingLive;
				break;
			}

			case Step::AuthorBuildingLive:
			{
				XPlane::FacadeAsset reread;
				const bool read = XPlane::LoadFacadeAsset(FacadeAuthoring::GetFilePath(), reread);
				const std::string &building = FacadeAuthoring::GetPreviewItem();
				Check(read && !reread.walls.empty() && reread.walls[0].scaleS == s.authoredScale && !FacadeAuthoring::IsDirty() &&
					!building.empty() && EditorProjects::GetPlacedEntity(building) != 0, "live preview saves the change and the footprint is drawn again");
				s.step = Step::AuthorCurb;
				break;
			}

			case Step::AuthorCurb:
			{
				// A sidewalk curb from X-Plane's 15 cm cross-section, textured with the building's texture
				const std::string texture = FacadeAuthoring::GetAsset().wallTexture;
				std::string error;
				const bool made = FacadeAuthoring::NewCurb(XPlane::GetCurbProfilePresets().front(), &error);
				XPlane::CurbFacadeSpec &spec = FacadeAuthoring::GetCurbSpec();
				spec.wallTexture = texture;
				spec.textureLength = 2.0f;
				FacadeAuthoring::SetName("SelfTest_curb");
				FacadeAuthoring::MarkEdited();
				const bool saved = made && FacadeAuthoring::Save(&error);
				const std::filesystem::path file = FacadeAuthoring::GetFilePath();
				std::filesystem::path specFile = file;
				specFile.replace_extension(".fac.json");
				s.authoredFiles.push_back(file);
				s.authoredFiles.push_back(specFile);
				const std::vector<std::pair<float, float>> profile = spec.profile;
				// Opened again: the cross-section comes back from the spec saved beside it
				const bool reopened = saved && FacadeAuthoring::Edit(FacadeAuthoring::GetResource(), &error) && FacadeAuthoring::GetKind() == FacadeAuthoring::Kind::Curb &&
					FacadeAuthoring::GetCurbSpec().profile == profile;
				Check(saved && std::filesystem::exists(specFile) && reopened && FacadeAuthoring::GetProblems().empty(),
					"author a 15 cm sidewalk curb, save it and open it again from its spec" + (error.empty() ? std::string() : ": " + error));
				const uint64_t history = EditorHistory::GetRevision();
				const std::string preview = saved ? FacadeAuthoring::PlacePreview(context) : std::string();
				if (!preview.empty())
				{
					s.facadeItems.push_back(preview);
					++s.facadeEdits;
				}
				const XPlane::FacadeAsset *placed = FacadeTool::GetAsset(FacadeAuthoring::GetResource());
				Check(!preview.empty() && EditorHistory::GetRevision() == history + 1 && placed && XPlane::GetFacadeFamily(*placed) == XPlane::FacadeFamily::Ground &&
					placed->floors.size() == 1 && placed->floors[0].curvedSegments.size() == placed->floors[0].segments.size(),
					"place the curb: a ground facade with curved segment variants");
				if (s.facadeShowcase && !preview.empty())
				{
					// Held for frame captures: the curb's cross-section in the FACADE EDITOR beside the framed curb
					EditorSelection::Select(context, Target::ForItem(preview));
					EditorTools::FrameSelection();
					EDITOR_INFO_TAG(LOG_TAG, "Facade showcase: the curb editor and its preview for 1200 frames (from frame {})", s.frames);
				}
				s.wait = s.facadeShowcase ? 1200 : 150;
				s.step = Step::AuthorCurbDrawn;
				break;
			}

			case Step::AuthorCurbDrawn:
			{
				const std::string &preview = FacadeAuthoring::GetPreviewItem();
				Check(!preview.empty() && EditorProjects::GetPlacedEntity(preview) != 0, "the authored curb is drawn");
				s.wait = 4;
				s.step = Step::AuthorType2Copy;
				break;
			}

			case Step::AuthorType2Copy:
			{
				// "Edit a copy" of a library type 2 facade (a school: OBJs beside it and from the library, textures named
				// .png and shipped as .dds): its textures, OBJs and their textures are copied into the facades folder
				namespace fs = std::filesystem;
				const std::string resource = "lib/g10/autogen/civic/school_E_6m_1.fac";
				const fs::path folder = EditorProjects::GetProjectFolder() / "facades";
				const auto listFolder = [&folder]
				{
					std::vector<fs::path> files;
					std::error_code ec;
					for (const fs::directory_entry &entry : fs::directory_iterator(folder, ec))
						files.push_back(entry.path());
					return files;
				};
				const std::vector<fs::path> before = listFolder();
				std::string error;
				const bool opened = FacadeAuthoring::CanEdit(resource) && FacadeAuthoring::Edit(resource, &error);
				for (const fs::path &file : listFolder())
				{
					if (std::ranges::find(before, file) == before.end())
						s.authoredFiles.push_back(file);
				}
				const XPlane::FacadeAsset &asset = FacadeAuthoring::GetAsset();
				const bool type2 = opened && FacadeAuthoring::GetKind() == FacadeAuthoring::Kind::Type2 && asset.IsType2() && !asset.floors.empty();
				const bool textures = type2 && !FacadeAuthoring::ResolveTexture(asset.wallTexture).empty() && VFS::IsInside(FacadeAuthoring::ResolveTexture(asset.wallTexture), folder);
				const size_t local = type2 ? static_cast<size_t>(std::ranges::count_if(asset.objects, [&folder](const std::string &object) { return fs::exists(folder / VFS::FromUtf8(object)); })) : 0;
				const bool library = type2 && std::ranges::any_of(asset.objects, [](const std::string &object) { return object.starts_with("lib/"); });
				Check(type2 && textures && local >= 2 && library, "edit a copy of a library type 2 facade: its textures and " + std::to_string(local) +
					" OBJs copied into the facades folder, library OBJs kept (" + std::to_string(s.authoredFiles.size()) + " files" + (error.empty() ? std::string() : ", " + error) + ")");
				if (!type2)
				{
					s.step = Step::FacadesUndo;
					break;
				}

				// The panel's own undo: a roof scale and a wall name changed, undone and redone, the project's history untouched
				const uint64_t history = EditorHistory::GetRevision();
				const float roofScale = asset.roofScaleS;
				const std::string wallName = asset.floors[0].walls.empty() ? std::string() : asset.floors[0].walls[0].name;
				FacadeAuthoring::GetAsset().roofScaleS = roofScale * 2.0f;
				FacadeAuthoring::MarkEdited();
				if (!FacadeAuthoring::GetAsset().floors[0].walls.empty())
					FacadeAuthoring::GetAsset().floors[0].walls[0].name = "selftest_wall";
				FacadeAuthoring::MarkEdited();
				const size_t segments = FacadeAuthoring::GetAsset().floors[0].segments.size();
				const bool undoneName = FacadeAuthoring::Undo() && (FacadeAuthoring::GetAsset().floors[0].walls.empty() || FacadeAuthoring::GetAsset().floors[0].walls[0].name == wallName);
				const bool undoneScale = FacadeAuthoring::Undo() && FacadeAuthoring::GetAsset().roofScaleS == roofScale && !FacadeAuthoring::CanUndo();
				const bool redone = FacadeAuthoring::Redo() && FacadeAuthoring::GetAsset().roofScaleS == roofScale * 2.0f && FacadeAuthoring::CanRedo();
				Check(undoneName && undoneScale && redone && FacadeAuthoring::GetAsset().floors[0].segments.size() == segments && EditorHistory::GetRevision() == history,
					"the facade editor undoes and redoes its own edits (segments kept, the project's history untouched)");

				// Saved, it reads back with its segments and the edit
				FacadeAuthoring::SetName("SelfTest_type2");
				const bool saved = FacadeAuthoring::Save(&error);
				s.authoredFiles.push_back(FacadeAuthoring::GetFilePath());
				XPlane::FacadeAsset reread;
				const bool read = saved && XPlane::LoadFacadeAsset(FacadeAuthoring::GetFilePath(), reread);
				Check(read && XPlane::WriteFacadeAsset(reread) == XPlane::WriteFacadeAsset(FacadeAuthoring::GetAsset()) && reread.roofScaleS == roofScale * 2.0f &&
					reread.floors.size() == FacadeAuthoring::GetAsset().floors.size(), "save the type 2 copy and read it back the same");
				const uint64_t placeHistory = EditorHistory::GetRevision();
				const std::string preview = saved ? FacadeAuthoring::PlacePreview(context) : std::string();
				if (!preview.empty())
				{
					s.facadeItems.push_back(preview);
					++s.facadeEdits;
				}
				Check(!preview.empty() && EditorHistory::GetRevision() == placeHistory + 1, "place a footprint built with the type 2 copy");
				s.wait = 150;
				s.step = Step::AuthorType2Drawn;
				break;
			}

			case Step::AuthorType2Drawn:
			{
				const std::string &preview = FacadeAuthoring::GetPreviewItem();
				Check(!preview.empty() && EditorProjects::GetPlacedEntity(preview) != 0, "the type 2 copy is drawn (its copied OBJs resolve)");
				s.wait = 4;
				s.step = Step::FacadesUndo;
				break;
			}

			case Step::FacadesUndo:
			{
				for (int i = 0; i < s.facadeEdits; ++i)
					EditorHistory::Undo();
				const ProjectFile::ProjectDocument *document = EditorProjects::GetDocument();
				const bool gone = std::ranges::none_of(s.facadeItems, [](const std::string &id) { return !id.empty() && FindItem(id) != nullptr; });
				Check(document && gone && document->GetProject().CountItems() == s.itemsBefore,
					"undo every facade step (" + std::to_string(s.facadeEdits) + "): the project is as it was");
				s.facadeItems.clear();
				// The facades and textures the authoring steps wrote go again (and the facades folder when the test made it)
				std::error_code ec;
				for (const std::filesystem::path &file : s.authoredFiles)
					std::filesystem::remove(file, ec);
				if (!s.authoredFolder.empty() && std::filesystem::is_empty(s.authoredFolder, ec))
					std::filesystem::remove(s.authoredFolder, ec);
				if (!s.authoredFiles.empty())
					EditorProjects::ReloadArt();
				s.authoredFiles.clear();
				s.authoredFolder.clear();
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
