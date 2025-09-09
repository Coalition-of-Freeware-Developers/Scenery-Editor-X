/**
* -------------------------------------------------------
* Scenery Editor X
* -------------------------------------------------------
* Copyright (c) 2025 Thomas Ray 
* Copyright (c) 2025 Coalition of Freeware Developers
* -------------------------------------------------------
* viewport.cpp
* -------------------------------------------------------
* Created: 5/9/2025
* -------------------------------------------------------
*/
#include "viewport.h"
#include <SceneryEditorX/renderer/renderer.h>

/// -------------------------------------------------------

namespace
{
	bool first_frame = true;
	uint32_t width_previous = 0;
	uint32_t height_previous = 0;
}

Viewport::Viewport(Editor *editor) : UI_Panel(editor)
{
    m_title = "Viewport";
    initialSize = Vec2(400, 250);
    m_flags |= ImGuiWindowFlags_NoScrollbar;
    m_padding = Vec2(2.0f);
}

void Viewport::OnTickVisible()
{
    // get viewport size
    uint32_t width = static_cast<uint32_t>(ImGui::GetContentRegionAvail().x);
    uint32_t height = static_cast<uint32_t>(ImGui::GetContentRegionAvail().y);

    // update engine's viewport
    static bool resolution_set = Settings::HasLoadedUserSettingsFromFile();
    if (!first_frame) // during the first frame the viewport is not yet initialized (it's size will be something weird)
    {
        if (width_previous != width || height_previous != height)
        {
            if (RHI_Device::IsValidResolution(width, height))
            {
                Renderer::SetViewport(static_cast<float>(width), static_cast<float>(height));

                if (!resolution_set)
                {
                    // only set the render and output resolutions once
                    // they are expensive operations, and we don't want to do it frequently
                    Renderer::SetResolutionOutput(width, height);
                    resolution_set = true;
                }

                width_previous = width;
                height_previous = height;
            }
        }
    }
    first_frame = false;

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
        m_editor->GetWidget<AssetBrowser>()->ShowMeshImportDialog(std::get<const char *>(payload->data));
    }

    Camera *camera = World::GetCamera();

    // mouse picking
    if (camera && ImGui::IsMouseClicked(0) && ImGui::IsItemHovered() && ImGui::TransformGizmo::allow_picking())
    {
        camera->Pick();
        m_editor->GetWidget<WorldViewer>()->SetSelectedEntity(camera->GetSelectedEntity());
    }

    // entity transform gizmo (will only show if an entity has been picked)
    if (Renderer::GetOption<bool>(Renderer_Option::TransformHandle))
    {
        ImGui::TransformGizmo::tick();
    }

    // check if the engine wants cursor control
    if (camera && camera->GetFlag(CameraFlags::IsControlled))
    {
        ImGui::GetIO().ConfigFlags |= ImGuiConfigFlags_NoMouseCursorChange;
    }
    else
    {
        ImGui::GetIO().ConfigFlags &= ~ImGuiConfigFlags_NoMouseCursorChange;
    }
}


/// -------------------------------------------------------
