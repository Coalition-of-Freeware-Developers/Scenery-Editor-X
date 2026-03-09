/**
 * -------------------------------------------------------
 * Scenery EditorLayer X
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
 * editor_layer.cpp
 * -------------------------------------------------------
 * Created: 01/03/2026
 * -------------------------------------------------------
 */
#include "editor_layer.h"

#include <SceneryEditorX/renderer/renderer.h>

// ---------------------------------------------------------

namespace SceneryEditorX
{
	
	EditorLayer::~EditorLayer()
	{
	}

	void EditorLayer::OnAttach()
	{
	    Layer::OnAttach();
	    m_Camera.Init();
	    Renderer::SetCamera(&m_Camera);
	    SEDX_CORE_INFO_TAG("EditorLayer", "Camera initialized and registered with renderer");
	}

	void EditorLayer::OnDetach()
	{
	    Renderer::SetCamera(nullptr);
	    Layer::OnDetach();
	}

	void EditorLayer::Tick(DeltaTime dt)
	{
	    Layer::Tick(dt);
        // Camera tick is driven by Renderer::Tick() to keep matrix updates aligned
		// with the active render frame and avoid double-processing input.
	}

	void EditorLayer::OnUIRender()
	{
	    Layer::OnUIRender();
	}

	void EditorLayer::OnEvent(Event &event)
	{
	    Layer::OnEvent(event);
	}

	bool EditorLayer::OnKeyPressedEvent(KeyPressedEvent &e)
    {
        return false;
    }

    bool EditorLayer::OnMouseButtonPressed(MouseButtonPressedEvent &e)
	{
        return false;
	}

	float EditorLayer::GetSnapValue()
    {
        return 0.0f;
    }

    float EditorLayer::UI_DrawTitlebar()
	{
        return 0.0f;
	}

	void EditorLayer::UI_DrawMenubar()
	{
	}

	void EditorLayer::UI_HandleManualWindowResize()
	{
	}

	bool EditorLayer::UI_TitleBarHitTest(int x, int y) const
	{
        return false;
	}

	void EditorLayer::UI_ShowNewProjectPopup()
	{
	}

	void EditorLayer::UI_ShowLoadAutoSavePopup()
	{
	}

	void EditorLayer::UI_ShowCreateAssetsFromMeshSourcePopup()
	{
	}

	void EditorLayer::UI_ShowInvalidAssetMetadataPopup()
	{
	}

	void EditorLayer::UI_ShowNoMeshPopup()
	{
	}

	void EditorLayer::UI_ShowNoSkeletonPopup()
	{
	}

	void EditorLayer::UI_ShowNoAnimationPopup()
	{
	}

	void EditorLayer::UI_ShowNewScenePopup()
	{
	}

	void EditorLayer::UI_ShowWelcomePopup()
	{

	}

	void EditorLayer::UI_ShowAboutPopup()
	{
	}

	void EditorLayer::UI_StatisticsPanel()
	{
	}

	void EditorLayer::UI_BuildAssetPackDialog()
	{
	}

	void EditorLayer::BuildProjectData()
	{
	}

	void EditorLayer::BuildShaderPack()
	{
	}

	void EditorLayer::BuildSoundBank()
	{
	}

	void EditorLayer::BuildAssetPack()
	{
	}

	void EditorLayer::BuildAll()
	{
	}

	void EditorLayer::RegenerateProjectScriptSolution(const std::filesystem::path &projectPath)
	{
	}

	void EditorLayer::ReloadCSharp()
	{
	}

	void EditorLayer::FocusLogPanel()
	{
	}

	void EditorLayer::OpenProject()
    {
    }

    void EditorLayer::OpenProject(const std::filesystem::path &filepath)
    {
    }

    void EditorLayer::CreateProject(const std::filesystem::path &projectPath)
    {
    }

    void EditorLayer::EmptyProject()
    {
    }

    void EditorLayer::UpdateCurrentProject()
    {
    }

    void EditorLayer::SaveProject()
    {
    }

    void EditorLayer::CloseProject(bool unloadProject)
    {
    }

    void EditorLayer::NewScene(const std::string &name)
    {
    }

    bool EditorLayer::OpenScene()
    {
        return false;
    }

    bool EditorLayer::OpenScene(const std::filesystem::path &filepath, const bool checkAutoSave)
    {
        return false;
    }

    void EditorLayer::SaveScene()
    {

    }

    void EditorLayer::SaveSceneAuto()
    {

    }

    void EditorLayer::SaveSceneAs()
    {

    }

}

// ---------------------------------------------------------
