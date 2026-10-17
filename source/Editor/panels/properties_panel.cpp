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
 * properties_panel.cpp
 * -------------------------------------------------------
 * Created: 17/10/2026
 * -------------------------------------------------------
 */
#include "properties_panel.h"
#include <Editor/core/editor_scene.h>
#include <Editor/core/editor_ui.h>
#include <SceneryEditorX/renderer/ui/source/imgui/misc/cpp/imgui_stdlib.h>
#include <SceneryEditorX/scene/components/renderable.h>

// -------------------------------------------------------

namespace SceneryEditorX
{
	namespace
	{
		/* Edits a Vec3 through a float[3] copy, so nothing depends on the vector's memory layout. */
		bool DragVec3(const char *id, Vec3 &value, const float speed, const char *format = "%.3f")
		{
			float components[3] = { value.x, value.y, value.z };
			if (!ImGui::DragFloat3(id, components, speed, 0.0f, 0.0f, format))
				return false;

			value = Vec3(components[0], components[1], components[2]);
			return true;
		}
	}

	PropertiesPanel::PropertiesPanel(EditorContext *context) : m_Context(context)
	{
		m_Title = "PROPERTIES";
	}

	void PropertiesPanel::OnUIRender(bool &isOpen)
	{
		if (EditorUI::BeginPanel("PROPERTIES###Properties", &isOpen))
		{
			const entt::entity handle = EditorScene::FindHandle(m_Context ? m_Context->selectedEntity : 0);

			if (handle == entt::null)
			{
				EditorUI::EmptyState("No properties found");
			}
			else
			{
				if (EditorUI::SectionHeader("GENERAL PROPERTIES") && EditorUI::BeginPropertyGrid("##General"))
				{
					if (TagComponent *tag = EditorScene::TryGet<TagComponent>(handle))
					{
						EditorUI::PropertyLabel("Name");
						ImGui::InputText("##Name", &tag->pTag);
					}

					EditorUI::PropertyLabel("ID");
					ImGui::TextDisabled("%llu", static_cast<unsigned long long>(EditorScene::GetID(handle)));

					EditorUI::EndPropertyGrid();
				}

				if (TransformComponent *transform = EditorScene::TryGet<TransformComponent>(handle))
				{
					if (EditorUI::SectionHeader("TRANSFORM") && EditorUI::BeginPropertyGrid("##Transform"))
					{
						EditorUI::PropertyLabel("Position");
						DragVec3("##Position", transform->translation, 0.05f);

						// Stored in radians, edited in degrees.
						EditorUI::PropertyLabel("Rotation");
						Vec3 degrees = transform->GetRotationEuler() * xMath::RAD_TO_DEG;
						if (DragVec3("##Rotation", degrees, 0.5f, "%.1f"))
							transform->SetRotationEuler(degrees * xMath::DEG_TO_RAD);

						EditorUI::PropertyLabel("Scale");
						DragVec3("##Scale", transform->scale, 0.01f);

						EditorUI::EndPropertyGrid();
					}
				}

				if (EditorUI::SectionHeader("COMPONENTS"))
				{
					auto listComponent = [](const char *name, const bool present)
					{
						if (present)
							ImGui::BulletText("%s", name);
					};

					Entity *entity = EditorScene::FindEntity(handle);
					listComponent("Transform", EditorScene::Has<TransformComponent>(handle));
					listComponent("Relationship", EditorScene::Has<RelationshipComponent>(handle));
					listComponent("Camera", EditorScene::Has<CameraComponent>(handle));
					listComponent("Renderable", entity && entity->HasComponent<Renderable>());
					listComponent("Directional Light", EditorScene::Has<DirectionalLightComponent>(handle));
					listComponent("Point Light", EditorScene::Has<PointLightComponent>(handle));
					listComponent("Spot Light", EditorScene::Has<SpotLightComponent>(handle));
				}
			}
		}
		EditorUI::EndPanel();
	}

}

// -------------------------------------------------------
