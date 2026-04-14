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
 * scene_viewport.cpp
 * -------------------------------------------------------
 * Created: 01/08/2025
 * -------------------------------------------------------
 */
#include "scene_viewport.h"
#include "asset_browser.h"
#include "properties.h"
#include "Editor/core/editor_layer.h"
#include <SceneryEditorX/core/input/input.h>
#include <SceneryEditorX/renderer/renderer.h>
#include <SceneryEditorX/renderer/ui/ui.h>
#include <SceneryEditorX/renderer/ui/ui_layer.h>
#include <SceneryEditorX/renderer/ui/ui_widget.h>
#include <SceneryEditorX/renderer/ui/actions/drag_drop.h>
#include <SceneryEditorX/renderer/ui/actions/gizmos.h>
#include <SceneryEditorX/scene/entity.h>
#include <SceneryEditorX/scene/scene.h>
#include <SceneryEditorX/settings/settings.h>

// -------------------------------------------------------

namespace SceneryEditorX
{

    static bool s_FirstFrame = true;
    static uint32_t s_WidthPrevious = 0;
    static uint32_t s_HeightPrevious = 0;

    SceneViewport::SceneViewport(const char *name, EditorLayer *editor) : m_ViewportName(name)
    {
        m_Title = name;
        m_InitialSize = Vec2(400, 250);
        m_Flags |= ImGuiWindowFlags_NoScrollbar;
        m_Padding = Vec2(2.0f);
    }

    void SceneViewport::OnTickVisible()
    {
        // get viewport size
        uint32_t width = static_cast<uint32_t>(ImGui::GetContentRegionAvail().x);
        uint32_t height = static_cast<uint32_t>(ImGui::GetContentRegionAvail().y);

        // update engine's viewport
        static bool resolutionSet = false;
        if (!s_FirstFrame) // during the first frame the viewport is not yet initialized (it's size will be something weird)
        {
            if (s_WidthPrevious != width || s_HeightPrevious != height)
            {
                if (width > 0 && height > 0)
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
        ImVec2 offset = ImGui::GetCursorPos();
        offset.y += 34; // TODO: this is probably the tab bar height, find a way to get it properly

        // draw the image after a potential resolution change call has been made
        UI::Image(Renderer::GetRenderTarget(Renderer_RenderTarget::frame_output),
                  ImVec2(static_cast<float>(width), static_cast<float>(height)));

        // handle model drop
        if (auto payload = DragDropPayload::ReceiveDragDropPayload(DragPayloadType::Model))
        {
            if (AssetBrowser *assetBrowser = m_Editor->GetPanel<AssetBrowser>())
            {
                assetBrowser->ShowMeshImportDialog(std::get<const char *>(payload->GetData()));
            }
        }

        // handle prefab drop
        if (auto payload = DragDropPayload::ReceiveDragDropPayload(DragPayloadType::Prefab))
        {
            if (const char *filePath = std::get<const char *>(payload->GetData()))
            {
                Entity entity = Scene::CreateEntity();
                std::string name = std::filesystem::path(filePath).stem().string();
                entity.SetObjectName(name);
            }
        }

        Camera *camera = Scene::GetCamera();

        // double-click to focus on entity
        if (camera && ImGui::IsMouseDoubleClicked(0) && ImGui::IsItemHovered() && Gizmo::AllowObjectSelection())
        {
            Properties::Inspect(camera->GetSelectedEntity());
            if (camera->GetSelectedEntity())
            {
                camera->SetViewTarget(camera->eye, camera->GetSelectedEntity()->GetPosition());
            }
        }
        // mouse picking (with multi-select via Ctrl handled in Pick())
        else if (camera && ImGui::IsMouseClicked(0) && ImGui::IsItemHovered() && Gizmo::AllowObjectSelection())
        {
            camera->GetSelectedEntity();

            // when ctrl is held, Pick() already handled multi-selection via ToggleSelection(),
            // so we only update the properties panel without overwriting the camera's selection
            if (Input::IsKeyPressed(KeyCode::LeftControl) ||
                Input::IsKeyPressed(KeyCode::RightControl))
            {
                Properties::Inspect(camera->GetSelectedEntity());
            }
            else
            {
                Properties::Inspect(camera->GetSelectedEntity());
            }
        }

        // Ctrl+D to duplicate selected entities
        if (camera && ImGui::IsWindowFocused() &&
            Input::IsKeyPressed(KeyCode::LeftControl) &&
            Input::IsKeyDown(KeyCode::D))
        {
            const std::vector<Entity *> &selectedEntities = camera->GetSelectedEntities();
            if (!selectedEntities.empty())
            {
                // clone all selected entities
                std::vector<Entity *> clonedEntities;
                for (Entity *entity : selectedEntities)
                {
                    if (entity)
                    {
                        if (Entity *cloned = entity->Clone(this))
                        {
                            clonedEntities.push_back(cloned);
                        }
                    }
                }

                // select the cloned entities instead
                if (!clonedEntities.empty())
                {
                    camera->ClearSelection();
                    for (Entity *cloned : clonedEntities)
                    {
                        camera->GetSelectedEntities().push_back(cloned);
                    }

                    Properties::Inspect(clonedEntities[0]);
                }
            }
        }

        // entity transform gizmo (will only show if entities have been picked)
        if (Gizmo::AllowObjectSelection())
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
                        Vec3 dirToEntity = primarySelected->GetPosition() - cameraEntity->GetPosition();
                        Normalize(dirToEntity);
                        if (Dot(dirToEntity, cameraEntity->GetForward()) >= 0.0f) // skip when the camera is facing away
                        {
                            Gizmo::Tick();
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

    void SceneViewport::SetVisible(bool visible)
    {
        m_IsVisible = visible;
    }

    void SceneViewport::OnEvent(Event &e)
    {
        EditorPanel::OnEvent(e);
    }

    void SceneViewport::OnVisible()
    {
        m_IsVisible = true;
    }

    void SceneViewport::OnInvisible()
    {
        m_IsVisible = false;
    }

    void SceneViewport::OnPreBegin()
    {
        const ImVec2 min = ImGui::GetCursorScreenPos();
        const ImVec2 avail = ImGui::GetContentRegionAvail();

        m_ViewportBounds[0] = Vec2(min.x, min.y);
        m_ViewportBounds[1] = Vec2(min.x + avail.x, min.y + avail.y);
    }
}

// -------------------------------------------------------
