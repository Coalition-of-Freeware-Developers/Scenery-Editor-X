/**
* -------------------------------------------------------
* Scenery Editor X
* -------------------------------------------------------
* Copyright (c) 2025 Thomas Ray 
* Copyright (c) 2025 Coalition of Freeware Developers
* -------------------------------------------------------
* viewports.cpp
* -------------------------------------------------------
* Created: 29/3/2025
* -------------------------------------------------------
*/
#include <imgui/imgui.h>
#include <imgui/imgui_internal.h>
#include <SceneryEditorX/renderer/vk_util.h>
#include <SceneryEditorX/ui/ui.h>
#include <SceneryEditorX/ui/ui_manager.h>

// -------------------------------------------------------

// Temporary stub for ImGuizmo until you integrate the actual library
namespace ImGuizmo 
{
    // Simple stub implementation
    bool IsUsing() { return false; }

}

// -------------------------------------------------------

namespace UI
{
    static bool ViewportPanel(glm::ivec2& newViewportSize, bool& viewportHovered, VkImageView imageView)
	{
	    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, {0, 0});
	    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0);
    
        bool isOpen = true;
        if (ImGui::Begin("Viewport", &isOpen, ImGuiWindowFlags_NoScrollbar))
        {
            // Get current viewport size
            ImVec2 viewportSize = ImGui::GetContentRegionAvail();
            newViewportSize = {static_cast<int>(viewportSize.x), static_cast<int>(viewportSize.y)};
            
            // Display the viewport image
            if (imageView != VK_NULL_HANDLE)
            {
                ImGui::Image((ImTextureID)imageView, viewportSize);
            }

			viewportHovered = ImGui::IsWindowHovered();
            
            // Set up ImGuizmo uncomment when integrating ImGuizmo
            /*
            ImGuizmo::SetDrawlist();
            ImGuizmo::SetRect(ImGui::GetWindowPos().x,
                              ImGui::GetWindowPos().y,
                              viewportSize.x,
                              viewportSize.y);

			*/

            // Check if viewport is hovered
            //viewportHovered = ImGui::IsWindowHovered() && !ImGuizmo::IsUsing();

			if (!ImGuizmo::IsUsing())
            {
                viewportHovered = ImGui::IsWindowHovered();
            }
        }


        ImGui::End();
        ImGui::PopStyleVar(2);
        
        return viewportHovered;
    }

	void UIManager::SetupDockspace(bool* p_open)
    {
        static bool opt_fullscreen = true;
        static ImGuiDockNodeFlags dockspace_flags = ImGuiDockNodeFlags_None;

        // We are using the ImGuiWindowFlags_NoDocking flag to make the parent window not dockable into,
        // because it would be confusing to have two docking targets within each other.
        ImGuiWindowFlags window_flags = ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_NoDocking;
        if (opt_fullscreen)
        {
            const ImGuiViewport* viewport = ImGui::GetMainViewport();
            ImGui::SetNextWindowPos(viewport->WorkPos);
            ImGui::SetNextWindowSize(viewport->WorkSize);
            ImGui::SetNextWindowViewport(viewport->ID);
            ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
            ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
            window_flags |= ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove;
            window_flags |= ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus;
        }

        // Important: note that we proceed even if Begin() returns false (aka window is collapsed).
        // This is because we want to keep our DockSpace() active. If a DockSpace() is inactive,
        // all active windows docked into it will lose their parent and become undocked.
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
        ImGui::Begin("DockSpace Demo", p_open, window_flags);
        ImGui::PopStyleVar();

        if (opt_fullscreen)
            ImGui::PopStyleVar(2);

        // Submit the DockSpace
        ImGuiIO& io = ImGui::GetIO();
        if (io.ConfigFlags & ImGuiConfigFlags_DockingEnable)
        {
            ImGuiID dockspace_id = ImGui::GetID("MyDockSpace");
            ImGui::DockSpace(dockspace_id, ImVec2(0.0f, 0.0f), dockspace_flags);

            // Set up default docking layout if first time
            static bool first_time = true;
            if (first_time)
            {
                first_time = false;
                ImGui::DockBuilderRemoveNode(dockspace_id);
                ImGui::DockBuilderAddNode(dockspace_id, dockspace_flags | ImGuiDockNodeFlags_DockSpace);
                ImGui::DockBuilderSetNodeSize(dockspace_id, io.DisplaySize);

                // Split the dockspace
                ImGuiID dock_main_id = dockspace_id;
                ImGuiID dock_id_right = ImGui::DockBuilderSplitNode(dock_main_id, ImGuiDir_Right, 0.25f, nullptr, &dock_main_id);
                ImGuiID dock_id_left = ImGui::DockBuilderSplitNode(dock_main_id, ImGuiDir_Left, 0.25f, nullptr, &dock_main_id);
                ImGuiID dock_id_bottom = ImGui::DockBuilderSplitNode(dock_main_id, ImGuiDir_Down, 0.25f, nullptr, &dock_main_id);

                // Dock windows
                ImGui::DockBuilderDockWindow("Viewport", dock_main_id);
                ImGui::DockBuilderDockWindow("Layer Stack", dock_id_right);
                ImGui::DockBuilderDockWindow("Asset Browser", dock_id_bottom);
                ImGui::DockBuilderDockWindow("Settings", dock_id_left);

                ImGui::DockBuilderFinish(dockspace_id);
            }
        }

        // Your main menu bar should be here
        if (ImGui::BeginMenuBar())
        {
            MainMenuBar();
            ImGui::EndMenuBar();
        }

        ImGui::End();
    }

    void UIManager::ViewportWindow(glm::ivec2& viewportSize, bool& viewportHovered, VkImageView imageView)
    {
        if (showViewport)
        {
            ViewportPanel(viewportSize, viewportHovered, imageView);
        }
    }

} // namespace UI

// -------------------------------------------------------
