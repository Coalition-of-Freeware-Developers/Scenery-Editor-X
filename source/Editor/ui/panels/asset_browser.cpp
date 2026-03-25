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
 * asset_browser.h
 * -------------------------------------------------------
 * Created: 18/03/2026
 * -------------------------------------------------------
 */
#include "asset_browser.h"
#include "file_dialog.h"
#include "properties.h"
#include "viewport.h"
#include <filesystem>
#include <string>
#include <Editor/core/editor.h>
#include <Editor/ui/ui.h>
#include <SceneryEditorX/core/resource/resource_cache.h>
#include <SceneryEditorX/core/threading/thread_pool.h>
#include <SceneryEditorX/scene/material.h>
#include <SceneryEditorX/scene/mesh.h>

// ---------------------------------------------------------

using namespace SceneryEditorX;

static bool s_ShowFileDialogView         = true;
static bool s_ShowFileDialogLoad         = false;
static bool s_MeshImportDialog_IsVisible = false;
static uint32_t s_MeshImportDialogFlags  = 0;
static std::string s_MeshImportFilePath;
static Scope<FileDialog> s_FileDialogView;
static Scope<FileDialog> s_FileDialogLoad;

static void MeshImportDialogCheckbox(const MeshFlags option, const char* label, const char* tooltip = nullptr)
{
    bool enabled = s_MeshImportDialogFlags & static_cast<uint32_t>(option);

    if (ImGui::Checkbox(label, &enabled))
    {
        if (enabled)
        {
            s_MeshImportDialogFlags |= static_cast<uint32_t>(option);
        }
        else
        {
            s_MeshImportDialogFlags &= ~static_cast<uint32_t>(option);
        }
    }

    if (tooltip != nullptr)
    {
        ::UI::Tooltip(tooltip);
    }
}

static void MeshImportDialog(Editor *editor)
{
    if (s_MeshImportDialog_IsVisible)
    {
        const Vec2 center = editor->GetWidget<::UI::Viewport>()->GetCenter();
        ImGui::SetNextWindowPos(ImVec2(center.x, center.y), ImGuiCond_FirstUseEver, ImVec2(0.5f, 0.5f));

        // Begin
        if (ImGui::Begin("Mesh import options", &s_MeshImportDialog_IsVisible, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoDocking | ImGuiWindowFlags_NoCollapse))
        {
            MeshImportDialogCheckbox(MeshFlags::ImportRemoveRedundantData,
                "Remove redundant data",
                "Join identical vertices, remove redundant materials, duplicate meshes, zeroed normals and invalid UVs.");

            MeshImportDialogCheckbox(MeshFlags::PostProcessNormalizeScale,
                "Normalize scale",
                "Scale the mesh so that it's not bigger than a cubic unit."
            );

            MeshImportDialogCheckbox(MeshFlags::ImportCombineMeshes,
                "Combine meshes",
                "Join some meshes, remove some nodes and pre-transform vertices."
            );

            MeshImportDialogCheckbox(MeshFlags::ImportLights,
                "Import lights",
                "Some models might define lights, they can be imported as well."
            );

            MeshImportDialogCheckbox(MeshFlags::PostProcessOptimize,
                "Optimize",
                "Performs a variety of optimizations aimed at reduce cache misses, overdraw and so on..."
            );

            // Ok button
            if (::UI::ButtonCenteredOnLine("Ok", 0.5f))
            {
                ThreadPool::Submit([]()
                {
                    ResourceCache::Load<Mesh>(s_MeshImportFilePath, s_MeshImportDialogFlags);
                });

                s_MeshImportDialog_IsVisible = false;
            }
        }

        ImGui::End();
    }
}

// ---------------------------------------------------------

AssetBrowser::AssetBrowser(Editor *editor) : Widget(editor)
{
    m_Title           = "Assets";
    s_FileDialogView  = CreateScope<FileDialog>(false, FileDialog_Type_Browser,       FileDialog_Op_Load, FileDialog_Filter_All);
    s_FileDialogLoad  = CreateScope<FileDialog>(true,  FileDialog_Type_FileSelection, FileDialog_Op_Load, FileDialog_Filter_Model);
    m_Flags          |= ImGuiWindowFlags_NoScrollbar;

    // just clicked, not selected (double-clicked, end of dialog)
    s_FileDialogView->SetCallbackOnItemClicked([this](const std::string& str) { OnPathClicked(str); });
}

void AssetBrowser::OnTickVisible()
{
    if (::UI::Button("Import"))
    {
        s_ShowFileDialogLoad = true;
    }

    ImGui::SameLine();
    
    // view
    s_FileDialogView->Show(&s_ShowFileDialogView, m_Editor);

    // show load file dialog, true if a selection is made
    if (s_FileDialogLoad->Show(&s_ShowFileDialogLoad, m_Editor, nullptr, &s_MeshImportFilePath))
    {
        s_ShowFileDialogLoad = false;
        ShowMeshImportDialog(s_MeshImportFilePath);
    }

    MeshImportDialog(m_Editor);
}

void AssetBrowser::ShowMeshImportDialog(const std::string& file_path)
{
    if (IO::FileManager::IsModel(file_path))
    {
        s_MeshImportDialog_IsVisible = true;
        s_MeshImportDialogFlags      = Mesh::GetDefaultFlags();
        s_MeshImportFilePath         = file_path;
    }
}

void AssetBrowser::OnPathClicked(const std::string& path)
{
    if (!IO::FileSystem::IsFile(path))
        return;

    /*if (std::filesystem::path(path).extension() == ".material")
    {
        auto materialAsset = CreateRef<MaterialAsset>();
        materialAsset->Load(path);

        if (const auto material = materialAsset->GetMaterial(); material)
        {
            Properties::Inspect(material);
        }
    }*/
}


// ---------------------------------------------------------
