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
#include <Editor/settings/editor_settings.h>
#include <Editor/ui/actions/gizmos.h>
#include <Editor/ui/source/imgui/imgui.h>
#include <Editor/ui/source/imgui/imgui_internal.h>
#include <Editor/ui/source/imguizmo/ImGuizmo.h>
#include <SceneryEditorX/renderer/renderer.h>
#include <SceneryEditorX/scene/entity.h>
#include <SceneryEditorX/scene/scene.h>

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

		// -------------------------------------------------------
		// Fullscreen dockspace host window
		// -------------------------------------------------------
		const ImGuiViewport* vp = ImGui::GetMainViewport();
		ImGui::SetNextWindowPos(vp->Pos);
		ImGui::SetNextWindowSize(vp->Size);
		ImGui::SetNextWindowViewport(vp->ID);

		constexpr ImGuiWindowFlags hostFlags =
			ImGuiWindowFlags_NoDocking |
			ImGuiWindowFlags_NoTitleBar |
			ImGuiWindowFlags_NoCollapse |
			ImGuiWindowFlags_NoResize |
			ImGuiWindowFlags_NoMove |
			ImGuiWindowFlags_NoBringToFrontOnFocus |
			ImGuiWindowFlags_NoNavFocus |
			ImGuiWindowFlags_NoBackground;

		ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
		ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
		bool dockspaceOpen = true;
		ImGui::Begin("##DockspaceHost", &dockspaceOpen, hostFlags);
		ImGui::PopStyleVar(3);

		ImGuiID dockspaceID = ImGui::GetID("MainDockspace");
		#if defined(IMGUI_HAS_DOCK)
		if (!ImGui::DockBuilderGetNode(dockspaceID))
		{
			ImGui::DockBuilderRemoveNode(dockspaceID);
			ImGui::DockBuilderAddNode(dockspaceID, ImGuiDockNodeFlags_None);
			ImGui::DockBuilderSetNodeSize(dockspaceID, vp->Size);

			ImGuiID remaining = dockspaceID;

			// Right panel (World + Properties)
			ImGuiID rightID;
			ImGui::DockBuilderSplitNode(remaining, ImGuiDir_Right, 0.18f, &rightID, &remaining);
			ImGuiID propertiesID;
			ImGui::DockBuilderSplitNode(rightID, ImGuiDir_Down, 0.55f, &propertiesID, &rightID);

			// Bottom panel (Console + Assets)
			ImGuiID bottomID;
			ImGui::DockBuilderSplitNode(remaining, ImGuiDir_Down, 0.22f, &bottomID, &remaining);
			ImGuiID assetsID;
			ImGui::DockBuilderSplitNode(bottomID, ImGuiDir_Right, 0.60f, &assetsID, &bottomID);

			// Dock windows
			ImGui::DockBuilderDockWindow("Viewport",    remaining);
			ImGui::DockBuilderDockWindow("World",       rightID);
			ImGui::DockBuilderDockWindow("Properties",  propertiesID);
			ImGui::DockBuilderDockWindow("Console",     bottomID);
			ImGui::DockBuilderDockWindow("Assets",      assetsID);
			ImGui::DockBuilderFinish(dockspaceID);
		}
		#endif

		ImGui::DockSpace(dockspaceID, ImVec2(0.0f, 0.0f), ImGuiDockNodeFlags_PassthruCentralNode);
		ImGui::End(); // DockspaceHost

		// -------------------------------------------------------
		// Menu bar
		// -------------------------------------------------------
		UI_DrawMenubar();

		// -------------------------------------------------------
		// Viewport panel
		// -------------------------------------------------------
		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
		ImGui::Begin("Viewport");
		ImGui::PopStyleVar();
		{
			ImVec2 size = ImGui::GetContentRegionAvail();
			if (size.x > 0 && size.y > 0)
			{
				Renderer::SetViewport(size.x, size.y);
			}
			// Gizmo overlay
			Gizmo::Tick();
		}
		ImGui::End(); // Viewport

		// -------------------------------------------------------
		// World hierarchy panel
		// -------------------------------------------------------
		ImGui::Begin("World");
		{
			for (Entity* entity : Scene::GetEntities())
			{
				if (!entity) continue;
				const bool selected = false; // TODO: tie to selection system
			 if (ImGui::Selectable(entity->Name().c_str(), selected))
				{
					// TODO: set selection
				}
			}
		}
		ImGui::End(); // World

		// -------------------------------------------------------
		// Properties panel
		// -------------------------------------------------------
		ImGui::Begin("Properties");
		{
			// TODO: display selected entity components
			ImGui::TextDisabled("Select an entity in the World panel");
		}
		ImGui::End(); // Properties

		// -------------------------------------------------------
		// Console panel
		// -------------------------------------------------------
		ImGui::Begin("Console");
		{
			// TODO: connect to logging system
			ImGui::TextDisabled("Console output will appear here");
		}
		ImGui::End(); // Console

		// -------------------------------------------------------
		// Assets panel
		// -------------------------------------------------------
		ImGui::Begin("Assets");
		{
			// TODO: integrate with AssetManager / content browser
			ImGui::TextDisabled("Project assets will appear here");
		}
		ImGui::End(); // Assets
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
		if (!EditorSettings::Get().enableGridSnapping)
			return 0.0f;

		const auto& settings = EditorSettings::Get();
		switch (m_GizmoType)
		{
			case ImGuizmo::TRANSLATE: return settings.translationSnapValue;
			case ImGuizmo::ROTATE:    return settings.rotationSnapValue;
			case ImGuizmo::SCALE:     return settings.scaleSnapValue;
			default:                  return 0.0f;
		}
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
