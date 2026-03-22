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
 * file_dialog_item.cpp
 * -------------------------------------------------------
 * Created: 18/03/2026
 * -------------------------------------------------------
 */
#include "file_dialog_item.h"

// ---------------------------------------------------------

using namespace SceneryEditorX;

FileDialogItem::FileDialogItem(const std::string &path, ImageResource *icon)
{
    m_Path = path;
    m_PathRelative = IO::FileSystem::GetRelativePath(path);
    m_Icon = icon;
    static uint32_t id = 0;
    m_Id32 = SceneryEditorX::UUID32(id++);
    m_IsDirectory = IO::FileSystem::IsDirectory(path);
    m_Label = IO::FileSystem::GetFileNameFromFilePath(path);
}

void FileDialogItem::Clicked()
{
    const auto now = std::chrono::high_resolution_clock::now();
    m_TimeSinceLastClick = now - m_LastClickTime;
    m_LastClickTime = now;
}

// ---------------------------------------------------------

