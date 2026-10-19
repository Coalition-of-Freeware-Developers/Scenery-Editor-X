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
 * file_dialog.h
 * -------------------------------------------------------
 * Created: 18/03/2026
 * -------------------------------------------------------
 */
#pragma once
#include "file_dialog_item.h"
#include <SceneryEditorX/renderer/ui/actions/drag_drop.h>
#include <SceneryEditorX/renderer/ui/source/imgui/imgui.h>

// ---------------------------------------------------------

namespace SceneryEditorX
{
    class Layer;

    class FileDialog
    {
    public:
        FileDialog(bool standaloneWindow, FileDialog_Type type, FileDialog_Operation operation, FileDialog_Filter filter);

        // type & filter
        auto GetType() const
        {
            return m_type;
        }
        auto GetFilter() const
        {
            return m_filter;
        }

        // operation
        auto GetOperation() const
        {
            return m_operation;
        }
        void SetOperation(FileDialog_Operation operation);

        // path
        const std::string &GetCurrentPath() const
        {
            return m_current_path;
        }
        void SetCurrentPath(const std::string &path);
        void SetDirty()
        {
            m_is_dirty = true;
        }

        // shows the dialog and returns true if a selection was made
        bool Show(bool *is_visible, Layer *editor, std::string *directory = nullptr, std::string *file_path = nullptr);
        void SetCallbackOnItemClicked(const std::function<void(const std::string &)> &callback)
        {
            m_callback_on_item_clicked = callback;
        }
        void SetCallbackOnItemDoubleClicked(const std::function<void(const std::string &)> &callback)
        {
            m_callback_on_item_double_clicked = callback;
        }

    private:
        void ShowTop(bool *is_visible, Layer *editor);
        void ShowMiddle();
        void ShowBottom(bool *is_visible);

        // view rendering
        void RenderGridView();
        void RenderListView();
        void RenderItem(FileDialogItem *item, const ImVec2 &size, bool is_list_view);

        // item functionality handling
        void ItemDrag(FileDialogItem *item) const;
        void ItemClick(FileDialogItem *item) const;
        void ItemContextMenu(FileDialogItem *item);

        // misc
        void DialogUpdateFromDirectory(const std::string &path);
        void EmptyAreaContextMenu();
        void HandleKeyboardNavigation();

        // flags
        bool m_is_window;
        bool m_selection_made;
        bool m_is_dirty;
        bool m_is_hovering_item;
        bool m_is_hovering_window;
        std::string m_title;
        std::string m_input_box;
        std::string m_hovered_item_path;
        uint32_t m_displayed_item_count;

        // internal
        mutable UUID32 m_context_menu_id;
        mutable DragDropPayload m_drag_drop_payload;
        float m_offset_bottom = 0.0f;
        FileDialog_Type m_type;
        FileDialog_Operation m_operation;
        FileDialog_Filter m_filter;
        std::vector<FileDialogItem> m_items;
        xMath::Vec2 m_item_size;
        ImGuiTextFilter m_search_filter;
        std::string m_current_path;
        std::string m_root_path;
        std::mutex m_mutex_items;

        // navigation history
        std::vector<std::string> m_history;
        size_t m_history_index;

        // view and sorting
        FileDialog_ViewMode m_view_mode;
        FileDialog_SortColumn m_sort_column;
        bool m_sort_ascending;

        // selection
        UUID32 m_selected_item_id;
        float m_hover_animation;

        // renaming
        bool m_is_renaming;
        std::string m_rename_buffer;
        UUID32 m_rename_item_id;

        // callbacks
        std::function<void(const std::string &)> m_callback_on_item_clicked;
        std::function<void(const std::string &)> m_callback_on_item_double_clicked;
    };

}

// ---------------------------------------------------------
