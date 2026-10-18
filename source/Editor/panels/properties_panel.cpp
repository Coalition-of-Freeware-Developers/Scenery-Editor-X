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
#include <Editor/core/editor_theme.h>
#include <Editor/core/editor_ui.h>
#include <Editor/core/editor_xplane.h>
#include <SceneryEditorX/renderer/font/font_awesome.h>
#include <SceneryEditorX/renderer/ui/source/imgui/misc/cpp/imgui_stdlib.h>
#include <SceneryEditorX/scene/components/renderable.h>
#include <SceneryEditorX/xplane/xp_scene.h>

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

		/* One texture row: file name with a loaded / missing marker and the format in the tooltip. */
		void TextureRow(const char *label, const std::string &reference, const XPlane::TextureInfo &info, const bool loaded)
		{
			if (reference.empty())
				return;

			EditorUI::PropertyLabel(label);
			ImGui::PushStyleColor(ImGuiCol_Text, EditorTheme::ToStyle(loaded ? EditorTheme::Palette::Text : EditorTheme::Palette::Error));
			ImGui::TextWrapped("%s %s", loaded ? SEDX_ICON_FILE_IMAGE_O : SEDX_ICON_EXCLAMATION_TRIANGLE, info.path.empty() ? reference.c_str() : info.path.filename().string().c_str());
			ImGui::PopStyleColor();

			char tooltip[512];
			if (loaded)
				snprintf(tooltip, sizeof(tooltip), "%s\n%ux%u, %u mips, %s%s", info.path.string().c_str(), info.width, info.height, info.mipCount,
					info.fromDds ? "DDS" : "PNG/RGBA8", info.hasAlpha ? ", alpha" : "");
			else
				snprintf(tooltip, sizeof(tooltip), "%s\n%s", reference.c_str(), info.error.empty() ? "Not loaded" : info.error.c_str());
			EditorUI::ItemTooltip(tooltip);
		}

		/* "X-PLANE OBJECT" section for an entity that belongs to a placed OBJ. */
		void DrawXPlaneObject(EditorContext *context, const uint64_t selected)
		{
			uint64_t rootId = 0;
			XPlane::XPlaneObjectComponent *object = XPlane::SceneObjects::FindObject(selected, &rootId);
			if (!object || !object->asset || !EditorUI::SectionHeader("X-PLANE OBJECT"))
				return;

			const XPlane::ObjectAsset &asset = *object->asset;
			const XPlane::Obj8Data &data = asset.GetData();
			const XPlane::ObjectTextures &textures = asset.GetTextures();

			if (rootId != selected)
			{
				ImGui::TextDisabled("Part of");
				ImGui::SameLine();
				if (ImGui::SmallButton(EditorScene::GetName(EditorScene::FindHandle(rootId))) && context)
					context->selectedEntity = rootId;
			}

			if (EditorUI::BeginPropertyGrid("##XPlaneObject"))
			{
				EditorUI::PropertyLabel("File");
				ImGui::TextWrapped("%s", asset.GetPath().filename().string().c_str());
				EditorUI::ItemTooltip(asset.GetPath().string().c_str());

				if (!object->virtualPath.empty())
				{
					EditorUI::PropertyLabel("Library path");
					ImGui::TextWrapped("%s", object->virtualPath.c_str());
				}

				EditorUI::PropertyLabel("Geometry");
				ImGui::Text("%u triangles, %zu batches, %u materials", data.GetTriangleCount(), asset.GetBatches().size(), asset.GetMaterialCount());

				if (!data.lights.empty())
				{
					EditorUI::PropertyLabel("Lights");
					ImGui::Text("%zu of %zu resolved: %u billboards, %u spill", asset.GetLights().size(), data.lights.size(), asset.GetBillboardCount(),
						asset.GetSpillCount());
					EditorUI::ItemTooltip("Named lights come from X-Plane's lights.txt; they show at night or with \"Force lights on\"");
				}

				EditorUI::PropertyLabel("Visible");
				ImGui::Checkbox("##Visible", &object->visible);

				EditorUI::PropertyLabel("Animate");
				ImGui::BeginDisabled(!data.IsAnimated());
				ImGui::Checkbox("##Animate", &object->animate);
				ImGui::EndDisabled();
				EditorUI::ItemTooltip("Off freezes the animation groups in their current pose");

				if (!data.lods.empty())
				{
					EditorUI::PropertyLabel("LOD");
					const auto lod_label = [&data](const int value) -> std::string
					{
						if (value == -1)
							return "Auto (camera distance)";
						if (value == -2)
							return "All LODs";
						const XPlane::Obj8Lod &lod = data.lods[static_cast<size_t>(value)];
						return "LOD " + std::to_string(value) + ": " + std::to_string(static_cast<int>(lod.nearDistance)) + " - " + std::to_string(static_cast<int>(lod.farDistance)) + " m";
					};

					ImGui::SetNextItemWidth(-FLT_MIN);
					if (ImGui::BeginCombo("##Lod", lod_label(object->lodOverride).c_str()))
					{
						for (int value = -1; value < static_cast<int>(data.lods.size()); ++value)
						{
							if (ImGui::Selectable(lod_label(value).c_str(), object->lodOverride == value))
								object->lodOverride = value;
							if (value == -1 && ImGui::Selectable(lod_label(-2).c_str(), object->lodOverride == -2))
								object->lodOverride = -2;
						}
						ImGui::EndCombo();
					}
				}

				EditorUI::PropertyLabel("In view");
				ImGui::Text("%.0f m away, %u / %zu batches", object->cameraDistance, object->visibleBatches, asset.GetBatches().size());
				if (!asset.GetLights().empty())
					ImGui::Text("%u billboards, %u spill lit", object->visibleBillboards, object->litSpills);

				TextureRow("Texture", data.texture, textures.albedoInfo, textures.albedo.Get() != nullptr);
				TextureRow("Normal map", data.textureNormal, textures.normalInfo, textures.normal.Get() != nullptr);
				TextureRow("Lit texture", data.textureLit, textures.litInfo, textures.lit.Get() != nullptr);
				TextureRow("Draped", data.textureDraped, textures.drapedInfo, textures.drapedAlbedo.Get() != nullptr);

				EditorUI::EndPropertyGrid();
			}

			if (ImGui::Button(EditorUI::IconLabel(SEDX_ICON_CROSSHAIRS, "Frame").c_str()))
				XPlane::SceneObjects::FrameObject(rootId);
			EditorUI::ItemTooltip("Move the camera to look at the object");

			if (data.IsAnimated())
			{
				ImGui::SeparatorText("Animation");
				EditorXPlane::DrawDatarefTable("##ObjectDatarefs", &data.datarefs);
				if (ImGui::TreeNode("Simulation"))
				{
					EditorXPlane::DrawSimulationControls();
					ImGui::TreePop();
				}
			}
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

				DrawXPlaneObject(m_Context, m_Context ? m_Context->selectedEntity : 0);

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
					listComponent("X-Plane Object", EditorScene::Has<XPlane::XPlaneObjectComponent>(handle));
				}
			}
		}
		EditorUI::EndPanel();
	}

}

// -------------------------------------------------------
