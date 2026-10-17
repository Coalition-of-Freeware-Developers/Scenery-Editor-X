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
#include <Editor/core/editor_scene.h>
#include <Editor/core/editor_ui.h>
#include <SceneryEditorX/renderer/font/font_awesome.h>
#include <SceneryEditorX/scene/components/renderable.h>

// -------------------------------------------------------

namespace SceneryEditorX
{
	namespace
	{
		const char *EntityIcon(Entity &entity)
		{
			const entt::entity handle = static_cast<entt::entity>(entity);
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
				// Snapshot: deleting while iterating would invalidate Scene's entity list.
				for (Entity *entity : Scene::GetEntities())
				{
					if (!entity || !*entity)
						continue;

					const entt::entity handle = static_cast<entt::entity>(*entity);
					const uint64_t id = EditorScene::GetID(handle);
					if (id == 0)
						continue;

					const char *name = EditorScene::GetName(handle);
					if (!m_Filter.PassFilter(name))
						continue;

					const bool selected = m_Context && m_Context->selectedEntity == id;

					ImGui::PushID(reinterpret_cast<void *>(static_cast<uintptr_t>(id)));
					if (ImGui::Selectable(EditorUI::IconLabel(EntityIcon(*entity), name).c_str(), selected, ImGuiSelectableFlags_SpanAvailWidth) && m_Context)
						m_Context->selectedEntity = id;

					if (ImGui::BeginPopupContextItem("##EntityMenu"))
					{
						if (ImGui::MenuItem("Delete"))
							pendingDelete = id;
						ImGui::EndPopup();
					}
					ImGui::PopID();
				}

				// Clicking empty space clears the selection.
				if (m_Context && ImGui::IsWindowHovered() && ImGui::IsMouseClicked(ImGuiMouseButton_Left) && !ImGui::IsAnyItemHovered())
					m_Context->selectedEntity = 0;
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
