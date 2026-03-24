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
 * file_dialog.cpp
 * -------------------------------------------------------
 * Created: 18/03/2026
 * -------------------------------------------------------
 */
#include "file_dialog.h"
#include <fstream>
#include <Editor/ui/ui.h>
#include <Editor/ui/source/imgui/imgui_internal.h>
#include <SceneryEditorX/core/resource/resource_cache.h>
#include <SceneryEditorX/core/threading/thread_pool.h>
#include <SceneryEditorX/core/window/window.h>
#include <SceneryEditorX/logging/logging.hpp>
#include <SceneryEditorX/scene/entity.h>
#include <SceneryEditorX/utils/string_utils.h>

// ---------------------------------------------------------

using namespace SceneryEditorX;

namespace
{
    constexpr const char* EXTENSION_WORLD = ".world";
    constexpr const char* EXTENSION_LUA = ".lua";
    constexpr const char* EXTENSION_MATERIAL = ".material";
    constexpr const char* NewLuaScriptContents = "-- New Lua script\n";

    std::string GetExtensionFromFilePath(const std::string& path)
    {
        return std::filesystem::path(path).extension().string();
    }

    bool HasExtension(const std::string& path, std::initializer_list<const char*> extensions)
    {
        const std::string extension = SceneryEditorX::Utils::String::ToLowerCopy(GetExtensionFromFilePath(path));
        for (const char* allowed : extensions)
        {
            if (extension == allowed)
                return true;
        }
        return false;
    }

    bool IsSupportedImageFile(const std::string& path)	{ return HasExtension(path, { ".png", ".jpg", ".jpeg", ".tga", ".bmp", ".dds", ".ktx" }); }
    bool IsSupportedAudioFile(const std::string& path)	{ return HasExtension(path, { ".wav", ".ogg", ".mp3", ".flac" }); }
    bool IsSupportedModelFile(const std::string& path)	{ return HasExtension(path, { ".obj", ".fbx", ".gltf", ".glb", ".3ds" }); }
    bool IsSupportedFontFile(const std::string& path)	{ return HasExtension(path, { ".ttf", ".ttc", ".otf" }); }
    bool IsEngineMaterialFile(const std::string& path)	{ return HasExtension(path, { EXTENSION_MATERIAL }); }
    bool IsEnginePrefabFile(const std::string& path)	{ return HasExtension(path, { ".prefab" }); }
    bool IsEngineWorldFile(const std::string& path)		{ return HasExtension(path, { EXTENSION_WORLD }); }
    bool IsEngineLuaFile(const std::string& path)		{ return HasExtension(path, { EXTENSION_LUA }); }

    std::vector<std::string> GetDirectoriesInDirectory(const std::string& path)
    {
        std::vector<std::string> directories;
        for (const auto& entry : std::filesystem::directory_iterator(path))
        {
            if (entry.is_directory())
                directories.emplace_back(entry.path().string());
        }
        return directories;
    }

    void WriteTextFile(const std::string& path, const std::string& content)
    {
        std::ofstream out(path, std::ios::out | std::ios::trunc);
        if (out.is_open())
            out << content;
    }
}


#define OPERATION_NAME (m_operation == FileDialog_Op_Open) ? "Open"      : (m_operation == FileDialog_Op_Load)   ? "Load"        : (m_operation == FileDialog_Op_Save) ? "Save" : "View"
#define FILTER_NAME    (m_filter == FileDialog_Filter_All) ? "All (*.*)" : (m_filter == FileDialog_Filter_Model) ? "Model (*.*)" : "World (*.world)"

// visual configuration
const float ITEM_SIZE_MIN       = 50.0f;
const float ITEM_SIZE_MAX       = 200.0f;
const float CARD_ROUNDING       = 6.0f;
const float TOOLBAR_HEIGHT      = 36.0f;
const float BREADCRUMB_HEIGHT   = 28.0f;
const float SEARCH_BAR_HEIGHT   = 32.0f;
const float GRID_ITEM_PADDING   = 8.0f;
const float LIST_ROW_HEIGHT     = 28.0f;
const float ICON_BUTTON_SIZE    = 24.0f;
const float STATUS_BAR_HEIGHT   = 26.0f;
const float BOTTOM_PANEL_HEIGHT = 44.0f;

// colors - will be derived from style
ImU32 col_card_bg;
ImU32 col_card_bg_hover;
ImU32 col_card_bg_selected;
ImU32 col_card_border;
ImU32 col_card_border_hover;
ImU32 col_shadow;
ImU32 col_accent;
ImU32 col_text;
ImU32 col_text_dim;
ImU32 col_toolbar_bg;
ImU32 col_separator;

static void UpdateColors()
{
    ImGuiStyle& style     = ImGui::GetStyle();
    col_card_bg           = ImGui::ColorConvertFloat4ToU32(ImVec4(0.12f, 0.12f, 0.13f, 1.0f));
    col_card_bg_hover     = ImGui::ColorConvertFloat4ToU32(ImVec4(0.18f, 0.18f, 0.20f, 1.0f));
    col_card_bg_selected  = ImGui::ColorConvertFloat4ToU32(ImVec4(0.15f, 0.25f, 0.35f, 1.0f));
    col_card_border       = ImGui::ColorConvertFloat4ToU32(ImVec4(0.25f, 0.25f, 0.28f, 1.0f));
    col_card_border_hover = ImGui::ColorConvertFloat4ToU32(style.Colors[ImGuiCol_CheckMark]);
    col_shadow            = IM_COL32(0, 0, 0, 50);
    col_accent            = ImGui::ColorConvertFloat4ToU32(style.Colors[ImGuiCol_CheckMark]);
    col_text              = ImGui::ColorConvertFloat4ToU32(style.Colors[ImGuiCol_Text]);
    col_text_dim          = ImGui::ColorConvertFloat4ToU32(style.Colors[ImGuiCol_TextDisabled]);
    col_toolbar_bg        = ImGui::ColorConvertFloat4ToU32(ImVec4(0.15f, 0.15f, 0.16f, 1.0f));
    col_separator         = ImGui::ColorConvertFloat4ToU32(ImVec4(0.20f, 0.20f, 0.22f, 1.0f));
}

FileDialog::FileDialog(const bool standaloneWindow, const FileDialog_Type type, const FileDialog_Operation operation, const FileDialog_Filter filter)
{
    m_type                            = type;
    m_operation                       = operation;
    m_filter                          = filter;
    m_title                           = OPERATION_NAME;
    m_is_window                       = standaloneWindow;
    m_item_size                       = xMath::Vec2(100.0f, 100.0f);
    m_is_dirty                        = true;
    m_selection_made                  = false;
    m_callback_on_item_clicked        = nullptr;
    m_callback_on_item_double_clicked = nullptr;
    m_current_path                    = ResourceCache::GetProjectDirectory();
    m_root_path                       = "..";
    m_sort_column                     = Sort_Name;
    m_sort_ascending                  = true;
    m_view_mode                       = View_Grid;
    m_history_index                   = 0;
    m_history.push_back(m_current_path);
    m_selected_item_id                = SceneryEditorX::UUID32(0);
    m_hover_animation                 = 0.0f;
    m_is_renaming                     = false;
    m_rename_item_id                  = SceneryEditorX::UUID32(0);
    m_context_menu_id                 = SceneryEditorX::UUID32(0);
}

void FileDialog::SetOperation(const FileDialog_Operation operation)
{
    m_operation = operation;
    m_title     = OPERATION_NAME;
}

void FileDialog::SetCurrentPath(const std::string & path)
{
    if (IO::FileSystem::IsFile(path))
    {
        m_current_path = IO::FileSystem::GetDirectoryFromFilePath(path);
    }
    else if (IO::FileSystem::IsDirectory(path))
    {
        m_current_path = path;
    }

    if (!m_current_path.empty())
    {
        m_is_dirty = true;
        m_history.push_back(m_current_path);
        m_history_index = m_history.size() - 1;
    }
}

bool FileDialog::Show(bool* is_visible, EditorLayer* editor, std::string * directory /*= nullptr*/, std::string * file_path /*= nullptr*/)
{
    if (!(*is_visible))
    {
        m_is_dirty = true;
        return false;
    }

    UpdateColors();

    m_selection_made     = false;
    m_is_hovering_item   = false;
    m_is_hovering_window = false;

    // calculate bottom offset before rendering so ShowMiddle knows the available space
    if (m_type == FileDialog_Type_Browser)
    {
        m_offset_bottom = STATUS_BAR_HEIGHT * Window::GetDpiScale();
    }
    else
    {
        m_offset_bottom = BOTTOM_PANEL_HEIGHT * Window::GetDpiScale();
    }

    ShowTop(is_visible, editor);
    ShowMiddle();
    ShowBottom(is_visible);

    if (m_is_window)
    {
        ImGui::End();
    }

    if (m_is_dirty)
    {
        if (IO::FileSystem::IsFile(m_current_path))
        {
            DialogUpdateFromDirectory(IO::FileSystem::GetDirectoryFromFilePath(m_current_path));
        }
        else
        {
            DialogUpdateFromDirectory(m_current_path);
        }
        m_is_dirty = false;
    }

    if (m_selection_made)
    {
        if (directory)
        {
            (*directory) = m_current_path;
        }
        if (file_path)
        {
            std::string dir = m_current_path;
            if (IO::FileSystem::IsFile(m_current_path))
            {
                dir = IO::FileSystem::GetDirectoryFromFilePath(m_current_path);
            }

            // ensure there's a separator between directory and filename
            if (!dir.empty() && dir.back() != '/' && dir.back() != '\\')
            {
                dir += "/";
            }
            (*file_path) = dir + m_input_box;
        }
    }

    EmptyAreaContextMenu();
    HandleKeyboardNavigation();

    return m_selection_made;
}

void FileDialog::ShowTop(bool* is_visible, EditorLayer* editor)
{
    if (m_is_window)
    {
        ImGui::SetNextWindowPos(ImGui::GetMainViewport()->GetCenter(), ImGuiCond_FirstUseEver, ImVec2(0.5f, 0.5f));
        ImGui::SetNextWindowSizeConstraints(ImVec2(700, 500), ImVec2(FLT_MAX, FLT_MAX));
        ImGui::SetNextWindowSize(ImVec2(900, 600), ImGuiCond_FirstUseEver);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
        ImGui::Begin(m_title.c_str(), is_visible, ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoDocking);
        ImGui::PopStyleVar();
        ImGui::SetWindowFocus();
    }

    ImDrawList* draw_list = ImGui::GetWindowDrawList();
    float window_width    = ImGui::GetContentRegionAvail().x;

    // consistent style for toolbar
    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(6, 4));
    ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 4.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(6, 4));

    // for standalone window, draw toolbar background and position from left
    if (m_is_window)
    {
        ImVec2 window_pos = ImGui::GetCursorScreenPos();
        draw_list->AddRectFilled(
            window_pos,
            ImVec2(window_pos.x + window_width, window_pos.y + TOOLBAR_HEIGHT),
            col_toolbar_bg
        );

        float button_height = ImGui::GetFrameHeight();
        float vertical_pad  = (TOOLBAR_HEIGHT - button_height) * 0.5f;
        ImGui::SetCursorPos(ImVec2(8, vertical_pad));
    }

    float button_height = ImGui::GetFrameHeight();

    // navigation buttons style
    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0, 0, 0, 0));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(1, 1, 1, 0.12f));
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(1, 1, 1, 0.18f));

    // navigation: back button
    bool can_go_back = m_history_index > 0;
    ImGui::BeginDisabled(!can_go_back);
    if (ImGui::Button("Back"))
    {
        m_history_index--;
        m_current_path = m_history[m_history_index];
        m_is_dirty     = true;
    }
    if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled))
        ImGui::SetTooltip("alt+left");
    ImGui::EndDisabled();
    ImGui::SameLine();

    // navigation: forward button
    bool can_go_forward = m_history_index < m_history.size() - 1;
    ImGui::BeginDisabled(!can_go_forward);
    if (ImGui::Button("Forward"))
    {
        m_history_index++;
        m_current_path = m_history[m_history_index];
        m_is_dirty     = true;
    }
    if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled))
        ImGui::SetTooltip("alt+right");
    ImGui::EndDisabled();
    ImGui::SameLine();

    // navigation: up button
    if (ImGui::Button("Up"))
    {
        std::string parent = std::filesystem::path(m_current_path).parent_path().string();
        if (!parent.empty() && parent.back() != '/' && parent.back() != '\\')
            parent += '/';
        if (!parent.empty() && parent != m_current_path)
        {
            m_current_path = parent;
            m_history.push_back(m_current_path);
            m_history_index = m_history.size() - 1;
            m_is_dirty      = true;
        }
    }
    if (ImGui::IsItemHovered())
        ImGui::SetTooltip("alt+up");
    ImGui::SameLine();

    // navigation: refresh button
    if (ImGui::Button("Refresh"))
    {
        m_is_dirty = true;
    }
    if (ImGui::IsItemHovered())
        ImGui::SetTooltip("f5");

    ImGui::SameLine(0, 12);

    // vertical separator
    {
        ImVec2 sep_pos = ImGui::GetCursorScreenPos();
        draw_list->AddLine(
            ImVec2(sep_pos.x, sep_pos.y + 2),
            ImVec2(sep_pos.x, sep_pos.y + button_height - 2),
            col_separator, 1.0f
        );
        ImGui::Dummy(ImVec2(1, button_height));
        ImGui::SameLine(0, 12);
    }

    // breadcrumb navigation
    {
        char accumulated_path[1024];
        accumulated_path[0] = '\0';

        char current_path[1024];
        strncpy_s(current_path, sizeof(current_path), m_current_path.c_str(), _TRUNCATE);

        const char* delimiters = "/\\";
        char* context          = nullptr;
        char* token            = strtok_s(current_path, delimiters, &context);
        bool first             = true;
        int segment_count      = 0;

        while (token)
        {
            if (strcmp(token, "..") == 0)
            {
                token = strtok_s(nullptr, delimiters, &context);
                continue;
            }

            if (first)
            {
                snprintf(accumulated_path, sizeof(accumulated_path), "%s/", token);
                first = false;
            }
            else
            {
                strncat_s(accumulated_path, sizeof(accumulated_path), token, _TRUNCATE);
                strncat_s(accumulated_path, sizeof(accumulated_path), "/", _TRUNCATE);
            }

            // chevron separator between breadcrumbs
            if (segment_count > 0)
            {
                ImGui::AlignTextToFramePadding();
                ImGui::TextColored(ImVec4(0.5f, 0.5f, 0.5f, 1.0f), "/");
                ImGui::SameLine();
            }

            // breadcrumb button
            ImGui::PushID(segment_count);
            if (ImGui::Button(token))
            {
                m_current_path = accumulated_path;
                m_history.push_back(m_current_path);
                m_history_index = m_history.size() - 1;
                m_is_dirty      = true;
            }
            ImGui::PopID();
            ImGui::SameLine();

            segment_count++;
            token = strtok_s(nullptr, delimiters, &context);
        }
    }

    // right side: view toggle and size slider (snapped to right edge)
    {
        // save current mode before buttons (mode may change during button click)
        bool is_grid_mode = (m_view_mode == View_Grid);
        bool is_list_mode = (m_view_mode == View_List);

        // calculate button widths
        float grid_btn_w  = ImGui::CalcTextSize("Grid").x + ImGui::GetStyle().FramePadding.x * 2;
        float list_btn_w  = ImGui::CalcTextSize("List").x + ImGui::GetStyle().FramePadding.x * 2;
        float slider_width = is_grid_mode ? 80.0f : 0.0f;
        float slider_gap   = is_grid_mode ? 8.0f : 0.0f;
        float item_spacing = ImGui::GetStyle().ItemSpacing.x;
        float total_width  = grid_btn_w + item_spacing + list_btn_w + slider_gap + slider_width;

        // position from right edge of window
        float window_w = ImGui::GetWindowWidth();
        float right_x  = window_w - total_width - 8.0f;
        ImGui::SetCursorPosX(right_x);

        // grid view button
        if (is_grid_mode)
            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(1, 1, 1, 0.15f));
        if (ImGui::Button("Grid"))
        {
            m_view_mode = View_Grid;
        }
        if (is_grid_mode)
            ImGui::PopStyleColor();
        ImGui::SameLine();

        // list view button
        if (is_list_mode)
            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(1, 1, 1, 0.15f));
        if (ImGui::Button("List"))
        {
            m_view_mode = View_List;
        }
        if (is_list_mode)
            ImGui::PopStyleColor();

        // size slider (grid view only)
        if (is_grid_mode)
        {
            ImGui::SameLine(0, slider_gap);
            ImGui::SetNextItemWidth(slider_width);
            ImGui::SliderFloat("##size", &m_item_size.x, ITEM_SIZE_MIN, ITEM_SIZE_MAX, "");
            if (ImGui::IsItemHovered())
            {
                ImGui::SetTooltip("icon size: %.0f", m_item_size.x);
            }
        }
    }

    // pop toolbar styles (3 colors for buttons, 3 style vars)
    ImGui::PopStyleColor(3);
    ImGui::PopStyleVar(3);

    // spacing after toolbar
    ImGui::Dummy(ImVec2(0, 4));

    // search bar row
    {
        ImGui::SetCursorPosX(8);

        // search input
        ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 4.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(8, 6));

        float search_width = ImGui::GetContentRegionAvail().x - 16;
        if (m_type != FileDialog_Type_Browser)
        {
            search_width -= 120; // space for filter dropdown
        }

        ImGui::SetNextItemWidth(search_width);

        // custom search field styling
        ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0.08f, 0.08f, 0.09f, 1.0f));

        // placeholder handling
        bool empty = m_search_filter.InputBuf[0] == '\0';
        if (empty)
        {
            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.5f, 0.5f, 0.5f, 1.0f));
        }

        m_search_filter.Draw("##search");

        if (empty)
        {
            ImGui::PopStyleColor();
            // draw placeholder text
            ImVec2 pos = ImGui::GetItemRectMin();
            ImGui::GetWindowDrawList()->AddText(
                ImVec2(pos.x + 8, pos.y + 6),
                IM_COL32(128, 128, 128, 180),
                "search files..."
            );
        }

        ImGui::PopStyleColor();
        ImGui::PopStyleVar(2);

        // filter dropdown (file selection mode only)
        if (m_type != FileDialog_Type_Browser)
        {
            ImGui::SameLine(0, 8);
            ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 4.0f);
            ImGui::SetNextItemWidth(100);
            if (ImGui::BeginCombo("##filter", FILTER_NAME))
            {
                if (ImGui::Selectable("All (*.*)", m_filter == FileDialog_Filter_All))
                {
                    m_filter   = FileDialog_Filter_All;
                    m_is_dirty = true;
                }
                if (ImGui::Selectable("Model (*.*)", m_filter == FileDialog_Filter_Model))
                {
                    m_filter   = FileDialog_Filter_Model;
                    m_is_dirty = true;
                }
                if (ImGui::Selectable("World (*.world)", m_filter == FileDialog_Filter_World))
                {
                    m_filter   = FileDialog_Filter_World;
                    m_is_dirty = true;
                }
                ImGui::EndCombo();
            }
            ImGui::PopStyleVar();
        }
    }

    // spacing before separator
    ImGui::Dummy(ImVec2(0, 8));

    // separator line
    ImDrawList* dl  = ImGui::GetWindowDrawList();
    ImVec2 sep_pos  = ImGui::GetCursorScreenPos();
    float sep_width = ImGui::GetContentRegionAvail().x;
    dl->AddLine(sep_pos, ImVec2(sep_pos.x + sep_width, sep_pos.y), col_separator);
    ImGui::Dummy(ImVec2(0, 1));
}

void FileDialog::ShowMiddle()
{
    const float content_width  = ImGui::GetContentRegionAvail().x;
    const float content_height = ImGui::GetContentRegionAvail().y - m_offset_bottom;
    ImGuiStyle& style          = ImGui::GetStyle();
    m_displayed_item_count     = 0;

    ImGui::PushStyleVar(ImGuiStyleVar_ChildBorderSize, 0.0f);
    ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.09f, 0.09f, 0.10f, 1.0f));

    if (ImGui::BeginChild("##content", ImVec2(content_width, content_height), false))
    {
        m_is_hovering_window = ImGui::IsWindowHovered(ImGuiHoveredFlags_AllowWhenBlockedByPopup | ImGuiHoveredFlags_AllowWhenBlockedByActiveItem);

        if (m_view_mode == View_List)
        {
            RenderListView();
        }
        else
        {
            RenderGridView();
        }
    }
    ImGui::EndChild();

    // drop target for entities dragged from the scene hierarchy - saves as a .prefab file
    if (m_type == FileDialog_Type_Browser)
    {
        if (ImGui::BeginDragDropTarget())
        {
            if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("ENTITY"))
            {
                if (payload->DataSize == sizeof(uint64_t))
                {
                    /*const uint64_t entity_id = *(const uint64_t*)payload->Data;
                    if (Entity* entity = Scene::GetEntityById(entity_id))
                    {
                        // save the entity as a .prefab file in the current browser directory
                        std::string prefab_path = m_current_path + "/" + entity->GetObjectName() + ".prefab";
                        if (Prefab::SaveToFile(entity, prefab_path))
                        {
                            entity->SetPrefabFilePath(prefab_path);
                            m_is_dirty = true;
                        }
                    }*/
                    (void)payload;
                }
            }
            ImGui::EndDragDropTarget();
        }
    }

    ImGui::PopStyleColor();
    ImGui::PopStyleVar();
}

void FileDialog::RenderGridView()
{
    const float content_width = ImGui::GetContentRegionAvail().x;
    const float icon_size     = m_item_size.x;
    const float label_height  = 20.0f;
    const float item_width    = icon_size + GRID_ITEM_PADDING * 2;
    const float item_height   = icon_size + label_height + GRID_ITEM_PADDING * 2;

    int columns = static_cast<int>((content_width - 16) / item_width);
    if (columns < 1) columns = 1;

    // initial padding
    ImGui::Dummy(ImVec2(0, 4));
    ImGui::Indent(8.0f);

    std::lock_guard lock(m_mutex_items);
    int col = 0;
    bool first_in_row = true;

    for (size_t i = 0; i < m_items.size(); i++)
    {
        auto& item = m_items[i];
        if (!m_search_filter.PassFilter(item.GetLabel().c_str()))
            continue;

        m_displayed_item_count++;

        if (!first_in_row)
        {
            ImGui::SameLine(0, 4);
        }
        first_in_row = false;

        ImGui::PushID(static_cast<int>(i));

        ImVec2 screen_pos = ImGui::GetCursorScreenPos();

        // card dimensions
        ImVec2 card_min = screen_pos;
        ImVec2 card_max = ImVec2(screen_pos.x + item_width - 4, screen_pos.y + item_height - 4);

        // invisible button for interaction - this is the only item we submit
        ImGui::InvisibleButton("##card", ImVec2(item_width - 4, item_height - 4));
        bool is_hovered  = ImGui::IsItemHovered();
        bool is_selected = (m_selected_item_id == item.GetId());

        // handle drag
        ItemDrag(&item);

        // draw card using draw list (no cursor manipulation)
        ImDrawList* draw_list = ImGui::GetWindowDrawList();

        // shadow (subtle)
        if (is_hovered || is_selected)
        {
            draw_list->AddRectFilled(
                ImVec2(card_min.x + 2, card_min.y + 2),
                ImVec2(card_max.x + 2, card_max.y + 2),
                col_shadow,
                CARD_ROUNDING
            );
        }

        // card background
        ImU32 bg_color = is_selected ? col_card_bg_selected : (is_hovered ? col_card_bg_hover : col_card_bg);
        draw_list->AddRectFilled(card_min, card_max, bg_color, CARD_ROUNDING);

        // card border (on hover or selection)
        if (is_selected)
        {
            draw_list->AddRect(card_min, card_max, col_card_border_hover, CARD_ROUNDING, 0, 2.0f);
        }
        else if (is_hovered)
        {
            draw_list->AddRect(card_min, card_max, col_card_border, CARD_ROUNDING, 0, 1.0f);
        }

        // icon - draw directly to draw list
        float icon_area = icon_size - GRID_ITEM_PADDING;
        if (ImageResource* texture = item.GetIcon())
        {
            if (texture->GetResourceState() == ResourceState::PreparedForGpu)
            {
                ImVec2 img_size(static_cast<float>(texture->GetWidth()), static_cast<float>(texture->GetHeight()));
                float scale = xMath::Min(icon_area / img_size.x, icon_area / img_size.y);
                img_size.x *= scale;
                img_size.y *= scale;

                // center icon horizontally and vertically within icon area
                float img_x = card_min.x + (item_width - 4 - img_size.x) * 0.5f;
                float img_y = card_min.y + GRID_ITEM_PADDING + (icon_area - img_size.y) * 0.5f;

                draw_list->AddImage(
                    reinterpret_cast<ImTextureID>(texture),
                    ImVec2(img_x, img_y),
                    ImVec2(img_x + img_size.x, img_y + img_size.y)
                );
            }
        }

        // label - positioned below the icon area
        const std::string & label = item.GetLabel();
        ImVec2 text_size    = ImGui::CalcTextSize(label.c_str());
        float label_max_w   = item_width - GRID_ITEM_PADDING * 2;
        float label_x       = card_min.x + (item_width - 4 - xMath::Min(text_size.x, label_max_w)) * 0.5f;
        float label_y       = card_min.y + GRID_ITEM_PADDING + icon_area + 4; // below icon

       // render label
        draw_list->AddText(ImVec2(label_x, label_y), col_text, label.c_str());

        // tooltip for truncated labels
        if (is_hovered && text_size.x > label_max_w)
        {
            ImGui::SetTooltip("%s", label.c_str());
        }

        // handle click
        if (ImGui::IsItemClicked(ImGuiMouseButton_Left))
        {
            item.Clicked();
            const bool is_single_click = item.GetTimeSinceLastClickMs() > 400;

            m_selected_item_id = item.GetId();
            m_input_box        = item.GetLabel();

            if (is_single_click)
            {
                if (m_callback_on_item_clicked)
                    m_callback_on_item_clicked(item.GetPath());
            }
            else
            {
                // double click
                m_current_path = item.GetPath();
                m_history.push_back(m_current_path);
                m_history_index  = m_history.size() - 1;
                m_is_dirty       = true;
                m_selection_made = !item.IsDirectory();

                if (m_type == FileDialog_Type_Browser && !item.IsDirectory())
                {
                    IO::FileSystem::OpenExternally(item.GetPath());
                }
                if (m_callback_on_item_double_clicked)
                {
                    m_callback_on_item_double_clicked(m_current_path);
                }
            }
        }

        if (ImGui::IsItemHovered(ImGuiHoveredFlags_RectOnly))
        {
            m_is_hovering_item  = true;
            m_hovered_item_path = item.GetPath();
        }

        ItemClick(&item);
        ItemContextMenu(&item);

        ImGui::PopID();

        // layout: new row when columns are full
        col++;
        if (col >= columns)
        {
            col = 0;
            first_in_row = true;
        }
    }

    ImGui::Unindent(8.0f);
}

void FileDialog::RenderListView()
{
    ImGui::PushStyleVar(ImGuiStyleVar_CellPadding, ImVec2(8, 4));

    if (ImGui::BeginTable("##files", 3, ImGuiTableFlags_Sortable | ImGuiTableFlags_Resizable | ImGuiTableFlags_RowBg | ImGuiTableFlags_ScrollY))
    {
        ImGui::TableSetupColumn("Name",     ImGuiTableColumnFlags_WidthStretch);
        ImGui::TableSetupColumn("Type",     ImGuiTableColumnFlags_WidthFixed, 100.0f);
        ImGui::TableSetupColumn("Modified", ImGuiTableColumnFlags_WidthFixed, 150.0f);
        ImGui::TableSetupScrollFreeze(0, 1);
        ImGui::TableHeadersRow();

        // handle sorting
        if (ImGuiTableSortSpecs* sorts_specs = ImGui::TableGetSortSpecs())
        {
            if (sorts_specs->SpecsDirty)
            {
                m_sort_column = sorts_specs->Specs[0].ColumnIndex == 0 ? Sort_Name :
                               (sorts_specs->Specs[0].ColumnIndex == 1 ? Sort_Type : Sort_Modified);
                m_sort_ascending        = sorts_specs->Specs[0].SortDirection == ImGuiSortDirection_Ascending;
                m_is_dirty              = true;
                sorts_specs->SpecsDirty = false;
            }
        }

        std::lock_guard lock(m_mutex_items);
        for (size_t i = 0; i < m_items.size(); i++)
        {
            auto& item = m_items[i];
            if (!m_search_filter.PassFilter(item.GetLabel().c_str()))
                continue;

            m_displayed_item_count++;

            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0);

            ImGui::PushID(static_cast<int>(i));

            bool is_selected = (m_selected_item_id == item.GetId());

            // selectable for the entire row
            if (ImGui::Selectable("##row", is_selected, ImGuiSelectableFlags_SpanAllColumns | ImGuiSelectableFlags_AllowDoubleClick, ImVec2(0, LIST_ROW_HEIGHT)))
            {
                item.Clicked();
                const bool is_single_click = item.GetTimeSinceLastClickMs() > 400;

                m_selected_item_id = item.GetId();
                m_input_box        = item.GetLabel();

                if (is_single_click)
                {
                    if (m_callback_on_item_clicked)
                        m_callback_on_item_clicked(item.GetPath());
                }
                else
                {
                    m_current_path = item.GetPath();
                    m_history.push_back(m_current_path);
                    m_history_index  = m_history.size() - 1;
                    m_is_dirty       = true;
                    m_selection_made = !item.IsDirectory();

                    if (m_type == FileDialog_Type_Browser && !item.IsDirectory())
                    {
                    IO::FileSystem::OpenExternally(item.GetPath());
                    }
                    if (m_callback_on_item_double_clicked)
                    {
                        m_callback_on_item_double_clicked(m_current_path);
                    }
                }
            }

            // drag source
            ItemDrag(&item);

            // hover state tracking
            if (ImGui::IsItemHovered(ImGuiHoveredFlags_RectOnly))
            {
                m_is_hovering_item  = true;
                m_hovered_item_path = item.GetPath();
            }

            ItemClick(&item);
            ItemContextMenu(&item);

            // icon
            ImGui::SameLine(0, 0);
            if (ImageResource* texture = item.GetIcon())
            {
                if (texture->GetResourceState() == ResourceState::PreparedForGpu)
                {
                    ImVec2 icon_size(20.0f, 20.0f);
                    ::UI::Image(texture, icon_size);
                    ImGui::SameLine(0, 8);
                }
            }

            // name
            ImGui::TextUnformatted(item.GetLabel().c_str());

            // type column
            ImGui::TableSetColumnIndex(1);
          const std::string typeLabel = item.IsDirectory() ? "Folder" : GetExtensionFromFilePath(item.GetPath());
            ImGui::TextColored(ImVec4(0.6f, 0.6f, 0.6f, 1.0f), "%s", typeLabel.c_str());

            // modified column
            ImGui::TableSetColumnIndex(2);
          const uint64_t lastWrite = IO::FileSystem::GetLastWriteTime(item.GetPath());
            ImGui::TextColored(ImVec4(0.5f, 0.5f, 0.5f, 1.0f), "%llu", static_cast<unsigned long long>(lastWrite));

            ImGui::PopID();
        }

        ImGui::EndTable();
    }

    ImGui::PopStyleVar();
}

void FileDialog::ShowBottom(bool* is_visible)
{
    ImDrawList* draw_list = ImGui::GetWindowDrawList();
    ImVec2 window_pos     = ImGui::GetWindowPos();
    ImVec2 window_size    = ImGui::GetWindowSize();
    float bar_y           = window_size.y - m_offset_bottom;

    // draw background bar
    ImVec2 bar_min = ImVec2(window_pos.x, window_pos.y + bar_y);
    ImVec2 bar_max = ImVec2(window_pos.x + window_size.x, window_pos.y + window_size.y);
    draw_list->AddRectFilled(bar_min, bar_max, col_toolbar_bg);
    draw_list->AddLine(bar_min, ImVec2(bar_max.x, bar_min.y), col_separator);

    if (m_type == FileDialog_Type_Browser)
    {
        // status bar: item count
        ImGui::SetCursorPos(ImVec2(12, bar_y + 5));
        ImGui::TextColored(ImVec4(0.6f, 0.6f, 0.6f, 1.0f),
            m_displayed_item_count == 1 ? "%d item" : "%d items", m_displayed_item_count);
    }
    else
    {
        // action bar: filename input, filter text, and buttons
        // calculate layout: [input field] [filter text] [Cancel] [Action]
        float frame_pad_x    = 16.0f;
        float button_spacing = 8.0f;
        float cancel_width   = ImGui::CalcTextSize("Cancel").x + frame_pad_x * 2;
        float action_width   = ImGui::CalcTextSize(OPERATION_NAME).x + frame_pad_x * 2;
        float buttons_total  = cancel_width + button_spacing + action_width + 12; // buttons + spacing + right margin
        float filter_width   = ImGui::CalcTextSize(FILTER_NAME).x + 16;
        float input_width    = window_size.x - buttons_total - filter_width - 24; // left margin + gaps

        ImGui::SetCursorPos(ImVec2(12, bar_y + 8));

        // filename input
        ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 4.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(8, 6));
        ImGui::SetNextItemWidth(input_width);
        char inputBuffer[1024] = {};
        strncpy_s(inputBuffer, sizeof(inputBuffer), m_input_box.c_str(), _TRUNCATE);
        if (ImGui::InputText("##filename", inputBuffer, sizeof(inputBuffer)))
        {
            m_input_box = inputBuffer;
        }
        ImGui::PopStyleVar(2);

        ImGui::SameLine(0, 8);

        // filter display
        ImGui::AlignTextToFramePadding();
        ImGui::TextColored(ImVec4(0.5f, 0.5f, 0.5f, 1.0f), FILTER_NAME);

        ImGui::SameLine(0, 8);

        // action buttons (auto-sized)
        ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 4.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(16, 6));

        // cancel button
        if (ImGui::Button("Cancel"))
        {
            m_selection_made = false;
            (*is_visible)    = false;
        }

        ImGui::SameLine(0, button_spacing);

        // primary action button (styled)
        ImGui::PushStyleColor(ImGuiCol_Button, ImGui::GetStyle().Colors[ImGuiCol_CheckMark]);
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.1f, 0.7f, 0.9f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.0f, 0.6f, 0.8f, 1.0f));

        if (ImGui::Button(OPERATION_NAME))
        {
            m_selection_made = true;
        }

        ImGui::PopStyleColor(3);
        ImGui::PopStyleVar(2);
    }
}

void FileDialog::RenderItem(FileDialogItem* item, const ImVec2& size, bool is_list_view)
{
    // legacy function kept for compatibility - actual rendering is now in RenderGridView/RenderListView
}

void FileDialog::ItemDrag(FileDialogItem* item) const
{
    if (!item || m_type != FileDialog_Type_Browser)
        return;

    if (ImGui::BeginDragDropSource(ImGuiDragDropFlags_SourceAllowNullID))
    {
       const auto set_payload = [this](const DragPayloadType type, const std::string & path_full, const std::string & path_relative)
        {
           m_drag_drop_payload = DragDropPayload(type, path_full.c_str(), path_relative.c_str());
            DragDropPayload::CreateDragDropPayload(m_drag_drop_payload);
        };

        const std::string& path_full     = item->GetPath();
        const std::string& path_relative = item->GetPathRelative();

        if (IsSupportedModelFile(path_full))  { set_payload(DragPayloadType::Model,    path_full, path_relative); }
        if (IsSupportedImageFile(path_full))  { set_payload(DragPayloadType::Texture,  path_full, path_relative); }
        if (IsSupportedAudioFile(path_full))  { set_payload(DragPayloadType::Audio,    path_full, path_relative); }
        if (IsEngineMaterialFile(path_full))  { set_payload(DragPayloadType::Material, path_full, path_relative); }
        if (IsEngineLuaFile(path_full))       { set_payload(DragPayloadType::Lua,      path_full, path_relative); }
        if (IsEnginePrefabFile(path_full))    { set_payload(DragPayloadType::Prefab,   path_full, path_relative); }

        // drag preview
        ImGui::BeginTooltip();
        ::UI::Image(item->GetIcon(), ImVec2(48, 48));
        ImGui::SameLine();
        ImGui::Text("%s", item->GetLabel().c_str());
        ImGui::EndTooltip();

        ImGui::EndDragDropSource();
    }
}

void FileDialog::ItemClick(FileDialogItem* item) const
{
    if (!item || !m_is_hovering_window)
        return;

    if (ImGui::IsItemClicked(ImGuiMouseButton_Right))
    {
        m_context_menu_id = item->GetId();
        ImGui::OpenPopup("##context_menu");
    }
}

void FileDialog::ItemContextMenu(FileDialogItem* item)
{
    if (m_context_menu_id != item->GetId())
        return;

    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(8, 8));
    ImGui::PushStyleVar(ImGuiStyleVar_PopupRounding, 6.0f);

    if (ImGui::BeginPopup("##context_menu"))
    {
        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(8, 6));

        if (ImGui::MenuItem("Rename"))
        {
            m_is_renaming    = true;
            m_rename_buffer  = item->GetLabel();
            m_rename_item_id = item->GetId();
        }

       if (IsEngineLuaFile(item->GetPath()))
        {
            ImGui::BeginDisabled();
            ImGui::MenuItem("Reload Script");
            ImGui::EndDisabled();
        }

        if (ImGui::MenuItem("Delete"))
        {
            IO::FileSystem::DeleteFile(item->GetPath());
            m_is_dirty = true;
        }

        ImGui::Separator();

        if (ImGui::MenuItem("Open in explorer"))
        {
           IO::FileSystem::OpenExternally(item->GetPath());
        }

        ImGui::PopStyleVar();
        ImGui::EndPopup();
    }

    ImGui::PopStyleVar(2);

    // rename dialog
    if (m_is_renaming && m_rename_item_id == item->GetId())
    {
        ImGui::OpenPopup("##rename_dialog");

        ImGui::SetNextWindowSize(ImVec2(300, 0), ImGuiCond_Always);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(16, 16));
        ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 8.0f);

        if (ImGui::BeginPopupModal("##rename_dialog", nullptr, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoTitleBar))
        {
            ImGui::Text("Rename");
            ImGui::Separator();
            ImGui::Spacing();

            ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 4.0f);
            ImGui::SetNextItemWidth(-1);
            char renameBuffer[1024] = {};
            strncpy_s(renameBuffer, sizeof(renameBuffer), m_rename_buffer.c_str(), _TRUNCATE);
            if (ImGui::InputText("##rename_input", renameBuffer, sizeof(renameBuffer)))
            {
                m_rename_buffer = renameBuffer;
            }
            ImGui::PopStyleVar();

            ImGui::Spacing();
            ImGui::Spacing();

            float button_width = 80.0f;
            float buttons_x    = ImGui::GetContentRegionAvail().x - button_width * 2 - 8;
            ImGui::SetCursorPosX(buttons_x);

            if (ImGui::Button("Cancel", ImVec2(button_width, 0)))
            {
                m_is_renaming = false;
                ImGui::CloseCurrentPopup();
            }

            ImGui::SameLine(0, 8);

            ImGui::PushStyleColor(ImGuiCol_Button, ImGui::GetStyle().Colors[ImGuiCol_CheckMark]);
            if (ImGui::Button("Rename", ImVec2(button_width, 0)))
            {
                std::string new_path = IO::FileSystem::GetDirectoryFromFilePath(item->GetPath()) + m_rename_buffer;
                IO::FileSystem::Rename(item->GetPath(), new_path);
                m_is_dirty    = true;
                m_is_renaming = false;
                ImGui::CloseCurrentPopup();
            }
            ImGui::PopStyleColor();

            ImGui::EndPopup();
        }

        ImGui::PopStyleVar(2);
    }
}

void FileDialog::DialogUpdateFromDirectory(const std::string& file_path)
{
    if (!IO::FileSystem::IsDirectory(file_path))
    {
        EDITOR_ERROR_TAG("File Dialog", "provided path doesn't point to a directory.");
        return;
    }

    std::lock_guard<std::mutex> lock(m_mutex_items);
    m_items.clear();
    m_selected_item_id = SceneryEditorX::UUID32(0);

    // directories first
    auto directories = GetDirectoriesInDirectory(file_path);
    for (const std::string & directory : directories)
    {
        m_items.emplace_back(directory, ResourceCache::GetIcon(IconType::Folder));
    }

    // then files based on filter
    std::vector<std::string> paths_anything = IO::FileSystem::GetFilesInDirectory(file_path);

    if (m_filter == FileDialog_Filter_All)
    {
        for (const std::string & path : paths_anything)
        {
         if (IsSupportedImageFile(path))
            {
                ThreadPool::Submit([this, path]()
                {
                    auto texture = ResourceCache::Load<ImageResource>(path);
                    std::lock_guard<std::mutex> lock(m_mutex_items);
                    m_items.emplace_back(path, texture.Get());
                });
            }
            else if (IsSupportedAudioFile(path))
            {
                m_items.emplace_back(path, ResourceCache::GetIcon(IconType::Audio));
            }
            else if (IsSupportedModelFile(path))
            {
                m_items.emplace_back(path, ResourceCache::GetIcon(IconType::Model));
            }
         else if (IsSupportedFontFile(path))
            {
                m_items.emplace_back(path, ResourceCache::GetIcon(IconType::Font));
            }
            else if (IsEngineMaterialFile(path))
            {
                m_items.emplace_back(path, ResourceCache::GetIcon(IconType::Material));
            }
          else if (IsEnginePrefabFile(path))
            {
                m_items.emplace_back(path, ResourceCache::GetIcon(IconType::Entity));
            }
           else if (IsEngineWorldFile(path))
            {
                m_items.emplace_back(path, ResourceCache::GetIcon(IconType::World));
            }
           else if (GetExtensionFromFilePath(path) == ".7z")
            {
                m_items.emplace_back(path, ResourceCache::GetIcon(IconType::Compressed));
            }
            else
            {
                m_items.emplace_back(path, ResourceCache::GetIcon(IconType::Undefined));
            }
        }
    }
    else if (m_filter == FileDialog_Filter_World)
    {
        for (const std::string & anything : paths_anything)
        {
          if (GetExtensionFromFilePath(anything) == EXTENSION_WORLD)
            {
                m_items.emplace_back(anything, ResourceCache::GetIcon(IconType::World));
            }
        }
    }
    else if (m_filter == FileDialog_Filter_Model)
    {
        for (const std::string & anything : paths_anything)
        {
         if (IsSupportedModelFile(anything))
            {
                m_items.emplace_back(anything, ResourceCache::GetIcon(IconType::Model));
            }
        }
    }

    // sort items
    std::ranges::sort(m_items, [this](const FileDialogItem& a, const FileDialogItem& b)
    {
        bool a_is_dir = IO::FileSystem::IsDirectory(a.GetPath());
        bool b_is_dir = IO::FileSystem::IsDirectory(b.GetPath());

        // directories always first
        if (a_is_dir != b_is_dir)
            return a_is_dir;

        if (m_sort_column == Sort_Name)
            return m_sort_ascending ? a.GetLabel() < b.GetLabel() : a.GetLabel() > b.GetLabel();

     if (m_sort_column == Sort_Type)
            return m_sort_ascending ? GetExtensionFromFilePath(a.GetPath()) < GetExtensionFromFilePath(b.GetPath()) :
                GetExtensionFromFilePath(a.GetPath()) > GetExtensionFromFilePath(b.GetPath());

        if (m_sort_column == Sort_Modified)
            return m_sort_ascending ? IO::FileSystem::GetLastWriteTime(a.GetPath()) < IO::FileSystem::GetLastWriteTime(b.GetPath()) : 
        IO::FileSystem::GetLastWriteTime(a.GetPath()) > IO::FileSystem::GetLastWriteTime(b.GetPath());

        return false;
    });
}

void FileDialog::EmptyAreaContextMenu()
{
    if (ImGui::IsMouseClicked(ImGuiMouseButton_Right) && m_is_hovering_window && !m_is_hovering_item)
    {
        ImGui::OpenPopup("##empty_context_menu");
    }

    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(8, 8));
    ImGui::PushStyleVar(ImGuiStyleVar_PopupRounding, 6.0f);

    if (ImGui::BeginPopup("##empty_context_menu"))
    {
        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(8, 6));

      if (ImGui::MenuItem("New folder"))
        {
            IO::FileSystem::CreateDir(m_current_path + "/New folder");
            m_is_dirty = true;
        }

      if (ImGui::MenuItem("New Lua script"))
        {
            WriteTextFile(m_current_path + "/new_lua_script" + std::string(EXTENSION_LUA), NewLuaScriptContents);
            m_is_dirty = true;
        }

        if (ImGui::MenuItem("New material"))
        {
            WriteTextFile(m_current_path + "/new_material" + std::string(EXTENSION_MATERIAL), "{}");
            m_is_dirty = true;
        }

        ImGui::Separator();

        if (ImGui::MenuItem("Open in explorer"))
        {
            IO::FileSystem::OpenExternally(m_current_path);
        }

        if (ImGui::MenuItem("Refresh"))
        {
            m_is_dirty = true;
        }

        ImGui::PopStyleVar();
        ImGui::EndPopup();
    }

    ImGui::PopStyleVar(2);
}

void FileDialog::HandleKeyboardNavigation()
{
    if (!m_is_hovering_window || m_is_renaming)
        return;

    // enter to confirm selection
    if (ImGui::IsKeyPressed(ImGuiKey_Enter) && !m_input_box.empty())
    {
        m_selection_made = true;
    }

    // escape to close (file selection mode only)
    if (ImGui::IsKeyPressed(ImGuiKey_Escape) && m_type == FileDialog_Type_FileSelection)
    {
        // handled by parent
    }

    // f5 to refresh
    if (ImGui::IsKeyPressed(ImGuiKey_F5))
    {
        m_is_dirty = true;
    }

    // alt+left for back
    if (ImGui::IsKeyPressed(ImGuiKey_LeftArrow) && ImGui::GetIO().KeyAlt && m_history_index > 0)
    {
        m_history_index--;
        m_current_path = m_history[m_history_index];
        m_is_dirty     = true;
    }

    // alt+right for forward
    if (ImGui::IsKeyPressed(ImGuiKey_RightArrow) && ImGui::GetIO().KeyAlt && m_history_index < m_history.size() - 1)
    {
        m_history_index++;
        m_current_path = m_history[m_history_index];
        m_is_dirty     = true;
    }

    // alt+up for parent directory
    if (ImGui::IsKeyPressed(ImGuiKey_UpArrow) && ImGui::GetIO().KeyAlt)
    {
        std::string parent = IO::FileSystem::GetParentDirectory(m_current_path);
        if (!parent.empty() && parent != m_current_path)
        {
            m_current_path = parent;
            m_history.push_back(m_current_path);
            m_history_index = m_history.size() - 1;
            m_is_dirty      = true;
        }
    }

    // backspace for parent directory
    if (ImGui::IsKeyPressed(ImGuiKey_Backspace) && !ImGui::GetIO().WantTextInput)
    {
        std::string parent = IO::FileSystem::GetParentDirectory(m_current_path);
        if (!parent.empty() && parent != m_current_path)
        {
            m_current_path = parent;
            m_history.push_back(m_current_path);
            m_history_index = m_history.size() - 1;
            m_is_dirty      = true;
        }
    }
}


// ---------------------------------------------------------
