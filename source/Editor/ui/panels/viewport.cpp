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
 * viewport.cpp
 * -------------------------------------------------------
 * Created: 1/8/2025
 * -------------------------------------------------------
 */
#include "viewport.h"
#include <Editor/ui/ui_widget.h>
#include <SceneryEditorX/core/input/input.h>
#include <SceneryEditorX/renderer/renderer.h>
#include <SceneryEditorX/scene/entity.h>

// -------------------------------------------------------

using namespace SceneryEditorX;

namespace UI
{
	static bool s_FirstFrame         = true;
	static uint32_t s_WidthPrevious  = 0;
	static uint32_t s_HeightPrevious = 0;

	Viewport::Viewport(const char *name, Editor *editor) : Widget(editor), m_ViewportName(name)
	{
		m_InitialSize = Vec2(400, 250);
		m_Flags |= ImGuiWindowFlags_NoScrollbar;
		m_Padding = Vec2(2.0f);
	}
	
	void Viewport::OnTickVisible()
	{
		// get viewport size
		uint32_t width = static_cast<uint32_t>(ImGui::GetContentRegionAvail().x);
		uint32_t height = static_cast<uint32_t>(ImGui::GetContentRegionAvail().y);
	
		// update engine's viewport
		static bool resolutionSet = Settings::HasLoadedUserSettingsFromFile();
		if (!s_FirstFrame) // during the first frame the viewport is not yet initialized (it's size will be something weird)
		{
			if (s_WidthPrevious != width || s_HeightPrevious != height)
			{
				if (RHI_Device::IsValidResolution(width, height))
				{
					Renderer::SetViewport(static_cast<float>(width), static_cast<float>(height));
	
					if (!resolutionSet)
					{
						// only set the render and output resolutions once
						// they are expensive operations, and we don't want to do it frequently
						Renderer::SetOutputResolution(width, height);
	
						resolutionSet = true;
					}
	
					s_WidthPrevious = width;
					s_HeightPrevious = height;
				}
			}
		}
		s_FirstFrame = false;
	
		// let the input system know about the position of this viewport within the editor
		// this will allow the system to properly calculate a relative mouse position
		Vec2 offset = ImGui::GetCursorPos();
		offset.y += 34; // TODO: this is probably the tab bar height, find a way to get it properly
		Input::SetEditorViewportOffset(offset);
	
		// draw the image after a potential resolution change call has been made
		ImGuiSp::image(Renderer::GetRenderTarget(Renderer_RenderTarget::frame_output),
			ImVec2(static_cast<float>(width), static_cast<float>(height)));

		// let the input system know if the mouse is within the viewport
		Input::SetMouseIsInViewport(ImGui::IsItemHovered());
	
		// handle model drop
		if (auto payload = ImGuiSp::receive_drag_drop_payload(ImGuiSp::DragPayloadType::Model))
		{
			m_Editor->GetWidget<AssetBrowser>()->ShowMeshImportDialog(std::get<const char *>(payload->data));
		}
	
		// handle prefab drop
		if (auto payload = ImGuiSp::receive_drag_drop_payload(ImGuiSp::DragPayloadType::Prefab))
		{
			if (const char *filePath = std::get<const char *>(payload->data))
			{
				Entity *entity = Scene::CreateEntity();
				std::string name = IO::FileSystem::GetFileNameWithoutExtensionFromFilePath(filePath);
				entity->SetObjectName(name);
				if (Prefab::LoadFromFile(filePath, entity))
				{
					entity->SetPrefabFilePath(filePath);
				}
				else
				{
					Scene::DestroyEntity(*entity);
				}
			}
		}
	
		Camera *camera = Scene::GetCamera();
	
		// double-click to focus on entity
		if (camera && ImGui::IsMouseDoubleClicked(0) && ImGui::IsItemHovered() &&
			ImGui::TransformGizmo::allow_picking())
		{
			camera->Pick();
			m_Editor->GetWidget<WorldViewer>()->SetSelectedEntity(camera->GetSelectedEntity());
			if (camera->GetSelectedEntity())
			{
				camera->FocusOnSelectedEntity();
			}
		}
		// mouse picking (with multi-select via Ctrl handled in Pick())
		else if (camera && ImGui::IsMouseClicked(0) && ImGui::IsItemHovered() &&
				 ImGui::TransformGizmo::allow_picking())
		{
			camera->Pick();
	
			// when ctrl is held, Pick() already handled multi-selection via ToggleSelection(),
			// so we only update the properties panel without overwriting the camera's selection
			if (Input::IsKeyPressed(KeyCode::LeftControl) || Input::IsKeyPressed(KeyCode::RightControl))
			{
				Properties::Inspect(camera->GetSelectedEntity());
			}
			else
			{
				m_Editor->GetWidget<WorldViewer>()->SetSelectedEntity(camera->GetSelectedEntity());
			}
		}
	
		// Ctrl+D to duplicate selected entities
		if (camera && ImGui::IsWindowFocused() && Input::IsKeyPressed(KeyCode::LeftControl) && Input::IsKeyDown(KeyCode::D))
		{
			const std::vector<Entity *> &selected_entities = camera->GetSelectedEntities();
			if (!selected_entities.empty())
			{
				// clone all selected entities
				std::vector<Entity *> cloned_entities;
				for (Entity *entity : selected_entities)
				{
					if (entity)
					{
						if (Entity *cloned = entity->Clone())
						{
							cloned_entities.push_back(cloned);
						}
					}
				}
	
				// select the cloned entities instead
				if (!cloned_entities.empty())
				{
					camera->ClearSelection();
					for (SceneryEditorX::Entity *cloned : cloned_entities)
					{
						camera->AddToSelection(cloned);
					}
					m_Editor->GetWidget<WorldViewer>()->SetSelectedEntity(cloned_entities[0]);
				}
			}
		}
	
		// entity transform gizmo (will only show if entities have been picked)
		if (cvar_transform_handle.GetValueAs<bool>())
		{
			if (camera) // skip if no camera
			{
				const std::vector<Entity *> &selectedEntities = camera->GetSelectedEntities();
				if (!selectedEntities.empty()) // skip if no entities are selected
				{
					// use the first selected entity for direction check
					if (Entity *primarySelected = selectedEntities[0])
					{
						Entity *cameraEntity = camera->GetEntity();
						xMath::Vec3 dirToEntity = primarySelected->GetPosition() - cameraEntity->GetPosition();
						dirToEntity.Normalize();
						if (dirToEntity.Dot(cameraEntity->GetForward()) >=
							0.0f) // skip when the camera is facing away
						{
							ImGui::TransformGizmo::tick();
						}
					}
				}
			}
		}
	
		// check if the engine wants cursor control
		if (camera)
		{
			ImGui::GetIO().ConfigFlags |= ImGuiConfigFlags_NoMouseCursorChange;
		}
		else
		{
			ImGui::GetIO().ConfigFlags &= ~ImGuiConfigFlags_NoMouseCursorChange;
		}
	}
	
	void Viewport::SetVisible(bool visible)
	{

	}

}

// -------------------------------------------------------
