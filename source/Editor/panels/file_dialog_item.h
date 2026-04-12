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
 * file_dialog_item.h
 * -------------------------------------------------------
 * Created: 18/03/2026
 * -------------------------------------------------------
 */
#pragma once
#include <SceneryEditorX/renderer/vulkan/image_resource.h>

// ---------------------------------------------------------

/**
 * @enum FileDialog_Type
 * @brief 
 */
enum FileDialog_Type : uint8_t
{
    FileDialog_Type_Browser,
    FileDialog_Type_FileSelection
};

/**
 * @enum FileDialog_Operation
 * @brief 
 */
enum FileDialog_Operation : uint8_t
{
    FileDialog_Op_Open,
    FileDialog_Op_Load,
    FileDialog_Op_Save
};

/**
 * @enum FileDialog_Filter
 * @brief 
 */
enum FileDialog_Filter : uint8_t
{
    FileDialog_Filter_All,
    FileDialog_Filter_World,
    FileDialog_Filter_Model
};

/**
 * @enum FileDialog_SortColumn
 * @brief 
 */
enum FileDialog_SortColumn : uint8_t
{
    Sort_Name,
    Sort_Type,
    Sort_Modified
};

/**
 * @enum FileDialog_ViewMode
 * @brief 
 */
enum FileDialog_ViewMode : uint8_t
{
    View_Grid,
    View_List
};

namespace SceneryEditorX
{
    /**
     * @brief 
     */
    class FileDialogItem
    {
    public:
        FileDialogItem(const std::string& path, ImageResource* icon);
        const auto& GetPath() const { return m_Path; }
        const auto& GetPathRelative() const { return m_PathRelative; }
        const auto& GetLabel() const { return m_Label; }
        UUID32 GetId() const { return m_Id32; }
        ImageResource* GetIcon() const { return m_Icon; }
        auto IsDirectory() const { return m_IsDirectory; }
        auto GetTimeSinceLastClickMs() const { return static_cast<float>(m_TimeSinceLastClick.count()); }
        void Clicked();

    private:
        ImageResource* m_Icon;
        UUID32 m_Id32;
        std::string m_Path;
        std::string m_PathRelative;
        std::string m_Label;
        bool m_IsDirectory;
        std::chrono::duration<double, std::milli> m_TimeSinceLastClick;
        std::chrono::time_point<std::chrono::high_resolution_clock> m_LastClickTime;
    };
}

// ---------------------------------------------------------
