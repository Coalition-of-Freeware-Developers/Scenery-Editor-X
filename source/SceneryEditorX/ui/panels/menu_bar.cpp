/**
* -------------------------------------------------------
* Scenery Editor X
* -------------------------------------------------------
* Copyright (c) 2025 Thomas Ray
* Copyright (c) 2025 Coalition of Freeware Developers
* -------------------------------------------------------
* menu_bar.cpp
* -------------------------------------------------------
* Created: 29/3/2025
* -------------------------------------------------------
*/
#include <Editor/ui/ui_manager.h>
#include <SceneryEditorX/ui/ui.h>
#include <imgui/imgui.h>

/// -------------------------------------------------------

namespace SceneryEditorX::UI
{

	void UIManager::MainMenuBar()
	{
        if (ImGui::BeginMainMenuBar())
		{
            if (ImGui::BeginMenu("File"))
            {
                if (ImGui::MenuItem("New", "Ctrl+N"))
                {
                    showCreateProjectModal = true;
                }

                if (ImGui::MenuItem("Open", "Ctrl+O")) { /* Prompt the user to select a file from a file manager */ }
                if (ImGui::BeginMenu("Open Recent"))
                {
                    ImGui::MenuItem("KHVN | Tweed New Haven", nullptr);
                    ImGui::MenuItem("EGLL | London Heathrow", nullptr);
                    ImGui::MenuItem("EINN | Shannon Intl.", nullptr);
                    ImGui::MenuItem("EIDW | Dublin Intl.", nullptr);
                    ImGui::MenuItem("KMEM | Memphis Intl.", nullptr);
                    ImGui::EndMenu();
                }

                ImGui::Separator();

                if (ImGui::BeginMenu("Import", "Ctrl+I"))
                {
                    ImGui::MenuItem("Import Scenery Gateway", nullptr);
                    ImGui::MenuItem("Import WED Project", nullptr);
                    ImGui::Separator();
                    ImGui::MenuItem("Convert WED Project");
                    ImGui::EndMenu();
                }

                if (ImGui::BeginMenu("Export", "Ctrl+I"))
                {
                    ImGui::MenuItem("Export to Scenery Gateway", nullptr);
                    ImGui::MenuItem("Export to X-Plane", nullptr);
                    ImGui::Separator();

                    if (ImGui::BeginMenu("Export to File"))
                    {
                        ImGui::MenuItem("Export '.apt'", nullptr);
                        ImGui::MenuItem("Export '.dsf'", nullptr);
                        ImGui::EndMenu();
                    }
                    ImGui::MenuItem("Export Terrain", nullptr);
                    ImGui::EndMenu();
                }

                ImGui::Separator();

                if (ImGui::MenuItem("Save", "Ctrl+S")) { /* Do something here */ }
                if (ImGui::MenuItem("Save As..", "Ctrl+Shift+S")) { /* Do something here */ }

                ImGui::Separator();

                if (ImGui::MenuItem("Exit", "Alt+F4"))
                {
                    showExitModal = true;
                   //glfwSetWindowShouldClose(window, true);
                }

                ImGui::EndMenu();

            }

            if (ImGui::BeginMenu("Edit"))
            {
                if (ImGui::MenuItem("Undo", "Ctrl+Z")) { /* Do something here */ }
                if (ImGui::MenuItem("Redo", "Ctrl+Y")) { /* Do something here */ }
                ImGui::Separator();
                if (ImGui::MenuItem("Cut", "Ctrl+X")) { /* Do something here */ }
                if (ImGui::MenuItem("Copy", "Ctrl+C")) { /* Do something here */ }
                if (ImGui::MenuItem("Paste", "Ctrl+V")) { /* Do something here */ }
                if (ImGui::MenuItem("Duplicate", "Ctrl+D")) { /* Do something here */ }
                ImGui::EndMenu();
            }

            if (ImGui::BeginMenu("Window"))
            {
                if (ImGui::BeginMenu("Views"))
                {
                    static bool enabled = true;
                    ImGui::Text("Camera Types");
                    ImGui::MenuItem("Perspective View", nullptr, &enabled);
                    ImGui::MenuItem("Orthoscopic View", nullptr, &enabled);
                    ImGui::Separator();
                    ImGui::MenuItem("Center on Airport", nullptr);
                    ImGui::MenuItem("Tower View", nullptr, &enabled);
                    ImGui::MenuItem("Walk Around", nullptr, &enabled);
                    ImGui::MenuItem("Free Cam", nullptr, &enabled);
                    ImGui::EndMenu();
                }

                ImGui::Separator();

                if (ImGui::BeginMenu("Toolbars"))
                {
                    static bool enabled = true;
                    ImGui::MenuItem("Content Browser", nullptr, &enabled);
                    ImGui::MenuItem("Layer Stack", nullptr, &enabled);
                    ImGui::MenuItem("Errors", nullptr, &enabled);
                    ImGui::MenuItem("Properties", nullptr, &enabled);
                    ImGui::EndMenu();
                }

                ImGui::Separator();
                static bool enabled = true;
                if (ImGui::MenuItem("Show Grid", "G", &enabled)) { /* Do something here */ }
                if (ImGui::MenuItem("Show Axis", nullptr, &enabled)) { /* Do something here */ }
                ImGui::Separator();
                if (ImGui::MenuItem("Save UI", nullptr)) { /* Do something here */}
                if (ImGui::MenuItem("Reset UI", nullptr)) { /* Do something here */}
                ImGui::EndMenu();
            }

			if (ImGui::BeginMenu("Viewport"))
            {
                static bool enabled = true;
                ImGui::Text("Terrain");
                ImGui::MenuItem("Show Terrain", nullptr, &enabled);
                ImGui::MenuItem("Terrain Wireframe", "Ctrl+W", &enabled);
                ImGui::Separator();
                ImGui::Text("Models");
                ImGui::MenuItem("Scene Wireframe", nullptr, &enabled);
                ImGui::EndMenu();
            }

            if (ImGui::BeginMenu("Plugins"))
            {
                if (ImGui::MenuItem("Plugin Manager", nullptr))
                {
                    //showPluginManager = true;
                }
                ImGui::Separator();
                ImGui::MenuItem("Install Plugin", nullptr);
                if (ImGui::MenuItem("Plugin Config", nullptr))
                {
                    //showPluginConfig = true;
                }
                ImGui::EndMenu();
            }

			if (ImGui::BeginMenu("Options"))
            {
                static bool enabled = true;
                if (ImGui::MenuItem("Toggle Snaps", "S", &enabled)) { /* Do something here */ }
                if (ImGui::BeginMenu("Snaps"))
                {
                    ImGui::MenuItem("Toggle Grid Snap", nullptr, &enabled);
                    ImGui::MenuItem("Toggle Vertex Snap", nullptr, &enabled);
                    ImGui::MenuItem("Toggle Surface Snap", nullptr, &enabled);
                    ImGui::EndMenu();
                }
                ImGui::EndMenu();
			}

			if (ImGui::MenuItem("Settings"))
            {
                showSettingsPanel = true;
            }

            if (ImGui::BeginMenu("Help"))
            {
                static bool showHelp = true;
                ImGui::MenuItem("Show Help", nullptr, &showHelp);
                if (ImGui::MenuItem("Check for Updates", nullptr)) { /* Do something here */}
                (ImGui::MenuItem("Release Notes", nullptr));
                (ImGui::MenuItem("Bug Report", nullptr));
                if (ImGui::MenuItem("About", nullptr))
                {
                    showAboutModal = true;
                }
                ImGui::EndMenu();
			}
        ImGui::EndMainMenuBar();
        }
    }

	/// MenuBar which allows you to specify its rectangle
	bool BeginMenuBar(const ImRect& barRectangle)
	{
		ImGuiWindow* window = ImGui::GetCurrentWindow();
		if (window->SkipItems)
			return false;
		/*if (!(window->Flags & ImGuiWindowFlags_MenuBar))
			return false;*/

		IM_ASSERT(!window->DC.MenuBarAppending);

        ///TODO: Misleading to use a group for that backup/restore
		ImGui::BeginGroup(); /// Backup position on layer 0
		ImGui::PushID("##menubar");

		const ImVec2 padding = window->WindowPadding;

		/**
		 * @note We don't clip with current window clipping rectangle as it is already set to the area below. However we clip with window full rect.
		 * We remove 1 worth of rounding to Max.x to that text in long menus and small windows don't tend to display over the lower-right rounded area, which looks particularly glitchy.
         */
		ImRect bar_rect = RectOffset(barRectangle, 0.0f, padding.y);// window->MenuBarRect();
		ImRect clip_rect(IM_ROUND(ImMax(window->Pos.x, bar_rect.Min.x + window->WindowBorderSize + window->Pos.x - 10.0f)), IM_ROUND(bar_rect.Min.y + window->WindowBorderSize + window->Pos.y),
			IM_ROUND(ImMax(bar_rect.Min.x + window->Pos.x, bar_rect.Max.x - ImMax(window->WindowRounding, window->WindowBorderSize))), IM_ROUND(bar_rect.Max.y + window->Pos.y));

		clip_rect.ClipWith(window->OuterRectClipped);
		ImGui::PushClipRect(clip_rect.Min, clip_rect.Max, false);

		/// We overwrite CursorMaxPos because BeginGroup sets it to CursorPos (essentially the .EmitItem hack in EndMenuBar() would need something analogous here, maybe a BeginGroupEx() with flags).
		window->DC.CursorPos = window->DC.CursorMaxPos = ImVec2(bar_rect.Min.x + window->Pos.x, bar_rect.Min.y + window->Pos.y);
		window->DC.LayoutType = ImGuiLayoutType_Horizontal;
		window->DC.NavLayerCurrent = ImGuiNavLayer_Menu;
		window->DC.MenuBarAppending = true;
		ImGui::AlignTextToFramePadding();
		return true;
	}

	void EndMenuBar()
	{
		ImGuiWindow* window = ImGui::GetCurrentWindow();
		if (window->SkipItems)
			return;
		ImGuiContext& g = *GImGui;

		/// Nav: When a move request within one of our child menu failed, capture the request to navigate among our siblings.
		if (ImGui::NavMoveRequestButNoResultYet() && (g.NavMoveDir == ImGuiDir_Left || g.NavMoveDir == ImGuiDir_Right) && (g.NavWindow->Flags & ImGuiWindowFlags_ChildMenu))
		{
			/// Try to find out if the request is for one of our child menu
			ImGuiWindow* nav_earliest_child = g.NavWindow;
			while (nav_earliest_child->ParentWindow && (nav_earliest_child->ParentWindow->Flags & ImGuiWindowFlags_ChildMenu))
				nav_earliest_child = nav_earliest_child->ParentWindow;
			if (nav_earliest_child->ParentWindow == window && nav_earliest_child->DC.ParentLayoutType == ImGuiLayoutType_Horizontal && (g.NavMoveFlags & ImGuiNavMoveFlags_Forwarded) == 0)
			{
				/// To do so we claim focus back, restore NavId and then process the movement request for yet another frame.
				/// This involve a one-frame delay which isn't very problematic in this situation. We could remove it by scoring in advance for multiple window (probably not worth bothering)
                constexpr ImGuiNavLayer layer = ImGuiNavLayer_Menu;
				IM_ASSERT(window->DC.NavLayersActiveMaskNext & (1 << layer)); /// Sanity check
				ImGui::FocusWindow(window);
				ImGui::SetNavID(window->NavLastIds[layer], layer, 0, window->NavRectRel[layer]);
				g.NavCursorVisible = false; /// Hide highlight for the current frame so we don't see the intermediary selection.
				g.NavHighlightItemUnderNav = false;
				g.NavMousePosDirty = true;
				ImGui::NavMoveRequestForward(g.NavMoveDir, g.NavMoveClipDir, g.NavMoveFlags, g.NavMoveScrollFlags); /// Repeat
			}
		}

		IM_MSVC_WARNING_SUPPRESS(6011); /// Static Analysis false positive "warning C6011: Dereferencing NULL pointer 'window'"
		// IM_ASSERT(window->Flags & ImGuiWindowFlags_MenuBar); /// NOTE: Needs to be commented out
		IM_ASSERT(window->DC.MenuBarAppending);
		ImGui::PopClipRect();
		ImGui::PopID();
		window->DC.MenuBarOffset.x = window->DC.CursorPos.x - window->Pos.x; /// Save horizontal position so next append can reuse it. This is kinda equivalent to a per-layer CursorPos.
		g.GroupStack.back().EmitItem = false;
		ImGui::EndGroup(); // Restore position on layer 0
		window->DC.LayoutType = ImGuiLayoutType_Vertical;
		window->DC.NavLayerCurrent = ImGuiNavLayer_Main;
		window->DC.MenuBarAppending = false;
	}

    bool ContextMenuHeader(const char *label, ImGuiTreeNodeFlags flags)
    {
        bool opened = false;
        if (ImGui::CollapsingHeader(label, flags))
        {
            opened = true;

            auto *window = ImGui::GetCurrentWindow();
            ImGuiContext &g = *GImGui;
            const ImGuiStyle &style = g.Style;
            const bool display_frame = (flags & ImGuiTreeNodeFlags_Framed) != 0;
            const ImVec2 padding =
                (display_frame || (flags & ImGuiTreeNodeFlags_FramePadding))
                    ? style.FramePadding
                    : ImVec2(style.FramePadding.x, ImMin(window->DC.CurrLineTextBaseOffset, style.FramePadding.y));

            const float text_offset_x =
                g.FontSize + (display_frame ? padding.x * 3 : padding.x * 2); /// Collapser arrow width + Spacing
            const ImVec2 label_size = ImGui::CalcTextSize(label);

            ImGui::SameLine();
            ImGui::SetCursorPosX(0.0f);
            const ImVec2 pos = ImGui::GetCursorScreenPos();
            const ImVec2 start =
                ImVec2(text_offset_x + label_size.x + padding.x * 4.0f, ImGui::GetFrameHeight() * 0.5f) + pos;
            const ImVec2 end = pos + ImVec2(ImGui::GetWindowWidth() - padding.x - window->ScrollbarSizes.x,
                                            ImGui::GetFrameHeight() * 0.5f);
            window->DrawList->AddLine(start, end, ImGui::GetColorU32(ImGuiCol_Separator));

            /// next line
            ImGui::Dummy(ImVec2(0.0f, ImGui::GetFrameHeight()));
        }
        return opened;
    }


}

/// -------------------------------------------------------

