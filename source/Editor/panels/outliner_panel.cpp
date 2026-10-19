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
 * outliner_panel.cpp
 * -------------------------------------------------------
 * Created: 17/10/2026
 * -------------------------------------------------------
 */
#include "outliner_panel.h"
#include <algorithm>
#include <Editor/core/editor_scene.h>
#include <Editor/core/editor_selection.h>
#include <Editor/core/editor_tools.h>
#include <Editor/core/editor_ui.h>
#include <unordered_set>
#include <SceneryEditorX/renderer/font/font_awesome.h>
#include <SceneryEditorX/renderer/ui/source/imgui/imgui_internal.h>
#include <SceneryEditorX/scene/components/renderable.h>
#include <SceneryEditorX/xplane/xp_airport_scene.h>
#include <SceneryEditorX/xplane/xp_scene.h>

// -------------------------------------------------------

namespace SceneryEditorX
{
	namespace
	{
		const char *EntityIcon(Entity &entity)
		{
			const entt::entity handle = static_cast<entt::entity>(entity);
			if (EditorScene::Has<XPlane::XPlaneObjectComponent>(handle))
				return SEDX_ICON_PLANE;
			if (const XPlane::AirportElementComponent *element = EditorScene::TryGet<XPlane::AirportElementComponent>(handle))
			{
				switch (element->kind)
				{
					case XPlane::AirportElementKind::Airport:	return SEDX_ICON_GLOBE;
					case XPlane::AirportElementKind::Group:		return SEDX_ICON_FOLDER_OPEN;
					case XPlane::AirportElementKind::Runway:
					case XPlane::AirportElementKind::WaterRunway:
					case XPlane::AirportElementKind::Helipad:	return SEDX_ICON_PLANE;
					case XPlane::AirportElementKind::LinearFeature:
					case XPlane::AirportElementKind::TaxiRoutes:	return SEDX_ICON_ROAD;
					case XPlane::AirportElementKind::Pavement:
					case XPlane::AirportElementKind::Boundary:	return SEDX_ICON_MAP_O;
					case XPlane::AirportElementKind::Beacon:
					case XPlane::AirportElementKind::LightFixture:	return SEDX_ICON_LIGHTBULB_O;
					default:									return SEDX_ICON_MAP_MARKER;
				}
			}
			if (EditorScene::Has<CameraComponent>(handle))
				return SEDX_ICON_VIDEO_CAMERA;
			if (EditorScene::Has<DirectionalLightComponent>(handle) || EditorScene::Has<PointLightComponent>(handle) || EditorScene::Has<SpotLightComponent>(handle))
				return SEDX_ICON_LIGHTBULB_O;
			if (entity.HasComponent<Renderable>())
				return SEDX_ICON_CUBE;
			return SEDX_ICON_OBJECT_GROUP;
		}
	}

	OutlinerPanel::OutlinerPanel(EditorContext *context) : m_Context(context)
	{
		m_Title = "OUTLINER";
	}

	void OutlinerPanel::RefreshLists()
	{
		const uint64_t version = Scene::GetStructureVersion();
		const bool structureChanged = version != m_ListVersion;
		if (structureChanged)
		{
			m_Roots.clear();
			for (Entity *entity : Scene::GetEntities())
			{
				if (!entity || !*entity)
					continue;

				// Roots only; children are drawn under their parent.
				const entt::entity handle = static_cast<entt::entity>(*entity);
				const RelationshipComponent *relationship = EditorScene::TryGet<RelationshipComponent>(handle);
				if (relationship && EditorScene::FindHandle(static_cast<uint64_t>(relationship->pParentHandle)) != entt::null)
					continue;
				m_Roots.push_back(handle);
			}
			m_ListVersion = version;
		}

		if (!m_Filter.IsActive())
		{
			m_Matches.clear();
			m_MatchFilter.clear();
			return;
		}

		if (!structureChanged && m_MatchFilter == m_Filter.InputBuf)
			return;

		// Searching flattens the tree so matches nested inside objects show up.
		m_Matches.clear();
		for (Entity *entity : Scene::GetEntities())
		{
			if (!entity || !*entity)
				continue;

			const entt::entity handle = static_cast<entt::entity>(*entity);
			if (m_Filter.PassFilter(EditorScene::GetName(handle)))
				m_Matches.push_back(handle);
		}
		m_MatchFilter = m_Filter.InputBuf;
	}

	void OutlinerPanel::OnUIRender(bool &isOpen)
	{
		if (EditorUI::BeginPanel("OUTLINER###Outliner", &isOpen))
		{
			if (ImGui::Button(EditorUI::IconLabel(SEDX_ICON_PLUS, "Add").c_str()))
				ImGui::OpenPopup("##AddEntity");
			EditorUI::ItemTooltip("Add an entity to the scene");

			if (ImGui::BeginPopup("##AddEntity"))
			{
				if (ImGui::MenuItem("Empty Entity"))
				{
					if (const uint64_t created = EditorScene::CreateEmpty("Empty Entity"); created != 0 && m_Context)
						m_Context->selectedEntity = created;
				}
				ImGui::EndPopup();
			}

			ImGui::SameLine();
			EditorUI::SearchFilter("##OutlinerFilter", m_Filter);

			ImGui::Separator();

			uint64_t pendingDelete = 0;
			if (ImGui::BeginChild("##OutlinerList"))
			{
				RefreshLists();

				// Rows: a tree node when the entity has children (X-Plane objects, groups), a leaf otherwise. Rows outside
				// the view are not submitted (a group can hold thousands of objects): a run of them becomes one Dummy of
				// their height. Open trees are always drawn so the rows below them stay in place.
				const auto draw_rows = [&](const auto &drawEntity, const size_t count, const auto &handleAt, const bool asTree)
				{
					const float itemSpacing = ImGui::GetStyle().ItemSpacing.y;
					float rowAdvance = 0.0f; // height of one closed row, measured from the first one drawn
					uint32_t skipped = 0;
					const auto flush = [&]()
					{
						if (skipped == 0)
							return;
						ImGui::Dummy(ImVec2(0.0f, std::max(0.0f, static_cast<float>(skipped) * rowAdvance - itemSpacing)));
						skipped = 0;
					};

					for (size_t i = 0; i < count; ++i)
					{
						const entt::entity handle = handleAt(i);
						if (handle == entt::null)
							continue;

						const uint64_t id = EditorScene::GetID(handle);
						const bool open = asTree && ImGui::TreeNodeGetOpen(ImGui::GetID(reinterpret_cast<void *>(static_cast<uintptr_t>(id))));
						if (rowAdvance > 0.0f && !open)
						{
							const ImVec2 cursor = ImGui::GetCursorScreenPos();
							const float top = cursor.y + static_cast<float>(skipped) * rowAdvance;
							if (!ImGui::IsRectVisible(ImVec2(cursor.x, top), ImVec2(cursor.x + 1.0f, top + rowAdvance)))
							{
								++skipped;
								continue;
							}
						}

						flush();
						const float before = ImGui::GetCursorPosY();
						drawEntity(drawEntity, handle, asTree);
						if (!open)
							rowAdvance = ImGui::GetCursorPosY() - before;
					}
					flush();
				};

				// Every selected target's entity shows as selected (the active one is what the PROPERTIES panel shows)
				std::unordered_set<uint64_t> selectedIds;
				for (const EditorSelection::Target &target : EditorSelection::Get())
				{
					if (const uint64_t entity = EditorSelection::Resolve(target))
						selectedIds.insert(entity);
				}

				// One row and, when it is open, its children.
				const auto draw_entity = [&](const auto &self, const entt::entity handle, const bool asTree) -> void
				{
					Entity *entity = Scene::GetStoredEntity(handle);
					const uint64_t id = EditorScene::GetID(handle);
					if (!entity || id == 0)
						return;

					const RelationshipComponent *relationship = EditorScene::TryGet<RelationshipComponent>(handle);
					const std::vector<UUID> children = (asTree && relationship) ? relationship->pChildren : std::vector<UUID>{};
					const bool selected = (m_Context && m_Context->selectedEntity == id) || selectedIds.contains(id);

					ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_OpenOnDoubleClick | ImGuiTreeNodeFlags_SpanAvailWidth;
					if (selected)
						flags |= ImGuiTreeNodeFlags_Selected;
					if (children.empty())
						flags |= ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen;

					// Inactive entities (hidden by LOD or ANIM_hide) draw dimmed.
					const bool active = entity->GetActive();
					if (!active)
						ImGui::PushStyleColor(ImGuiCol_Text, ImGui::GetStyleColorVec4(ImGuiCol_TextDisabled));
					const bool open = ImGui::TreeNodeEx(reinterpret_cast<void *>(static_cast<uintptr_t>(id)), flags, "%s",
						EditorUI::IconLabel(EntityIcon(*entity), EditorScene::GetName(handle)).c_str());
					if (!active)
						ImGui::PopStyleColor();

					if (ImGui::IsItemClicked() && !ImGui::IsItemToggledOpen() && m_Context)
					{
						// Ctrl toggles, Shift adds, a plain click replaces the selection
						const ImGuiIO &io = ImGui::GetIO();
						const EditorSelection::Mode mode = io.KeyCtrl ? EditorSelection::Mode::Toggle : io.KeyShift ? EditorSelection::Mode::Add : EditorSelection::Mode::Replace;
						if (const std::optional<EditorSelection::Target> target = EditorSelection::FromEntity(id))
							EditorSelection::Select(m_Context, *target, mode);
					}
					if (ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left) && !ImGui::IsItemToggledOpen())
						EditorTools::FrameSelection();

					ImGui::PushID(reinterpret_cast<void *>(static_cast<uintptr_t>(id)));
					if (ImGui::BeginPopupContextItem("##EntityMenu"))
					{
						if (ImGui::MenuItem("Delete"))
							pendingDelete = id;
						if (EditorScene::Has<XPlane::XPlaneObjectComponent>(handle) && ImGui::MenuItem("Frame"))
							XPlane::SceneObjects::FrameObject(id);
						if (EditorScene::Has<XPlane::AirportElementComponent>(handle) && ImGui::MenuItem("Frame"))
							XPlane::SceneAirports::FrameEntity(id);
						ImGui::EndPopup();
					}
					ImGui::PopID();

					if (open && !children.empty())
					{
						draw_rows(self, children.size(), [&children](const size_t i) { return EditorScene::FindHandle(static_cast<uint64_t>(children[i])); }, true);
						ImGui::TreePop();
					}
				};

				if (m_Filter.IsActive())
					draw_rows(draw_entity, m_Matches.size(), [this](const size_t i) { return m_Matches[i]; }, false);
				else
					draw_rows(draw_entity, m_Roots.size(), [this](const size_t i) { return m_Roots[i]; }, true);

				// Clicking empty space clears the selection.
				if (m_Context && ImGui::IsWindowHovered() && ImGui::IsMouseClicked(ImGuiMouseButton_Left) && !ImGui::IsAnyItemHovered())
					EditorSelection::Clear(m_Context);
			}
			ImGui::EndChild();

			if (pendingDelete != 0)
			{
				EditorScene::Destroy(pendingDelete);

				if (m_Context && m_Context->selectedEntity == pendingDelete)
					m_Context->selectedEntity = 0;
			}
		}
		EditorUI::EndPanel();
	}

}

// -------------------------------------------------------
