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
#include "Editor/projects/project.h"
#include "Editor/settings/editor_settings.h"
#include "Editor/ui/ui_impl.h"
#include "Editor/ui/ui_layer.h"
#include "Editor/ui/actions/gizmos.h"
#include <Editor/ui/source/imgui/imgui.h>
#include <Editor/ui/source/imgui/imgui_internal.h>
#include <Editor/ui/source/imgui/backends/imgui_impl_sdl3.h>
#include <Editor/ui/source/imguizmo/ImGuizmo.h>
#include <SceneryEditorX/core/application/application.h>
#include <SceneryEditorX/core/input/input.h>
#include <SceneryEditorX/core/resource/resource_cache.h>
#include <SceneryEditorX/core/window/window.h>
#include <SceneryEditorX/project/project.h>
#include <SceneryEditorX/project/selection_manager.h>
#include <SceneryEditorX/renderer/renderer.h>
#include <SceneryEditorX/scene/entity.h>
#include <SceneryEditorX/scene/scene.h>

// ---------------------------------------------------------

namespace SceneryEditorX
{

	#define MAX_PROJECT_NAME_LENGTH 255
	#define MAX_PROJECT_FILEPATH_LENGTH 512
	
	// ---------------------------------------------------------

	static char* s_ProjectNameBuffer = new char[MAX_PROJECT_NAME_LENGTH];
	static char* s_OpenProjectFilePathBuffer = new char[MAX_PROJECT_FILEPATH_LENGTH];
	static char* s_NewProjectFilePathBuffer = new char[MAX_PROJECT_FILEPATH_LENGTH];

	#define SCENE_HIERARCHY_PANEL_ID		"SceneHierarchyPanel"
	#define ECS_DEBUG_PANEL_ID				"ECSDebugPanel"
	#define CONSOLE_PANEL_ID				"EditorConsolePanel"
	#define CONTENT_BROWSER_PANEL_ID		"ContentBrowserPanel"
	#define PROJECT_SETTINGS_PANEL_ID		"ProjectSettingsPanel"
	#define ASSET_MANAGER_PANEL_ID			"AssetManagerPanel"
	#define MATERIALS_PANEL_ID				"MaterialsPanel"
	#define APPLICATION_SETTINGS_PANEL_ID	"ApplicationSettingsPanel"
	#define SCRIPT_ENGINE_DEBUG_PANEL_ID	"ScriptEngineDebugPanel"
	#define SCENE_RENDERER_PANEL_ID			"SceneRendererPanel"

	static float s_FontSize  = 18.0f;
	static float s_FontScale = 1.0f;

	static bool s_SceneOpen = false;
	static bool s_ProjectOpen = false;
		
	static auto operator<(const ImVec2 &lhs, const ImVec2 &rhs)
	{
		return lhs.x < rhs.x && lhs.y < rhs.y;
	}

	EditorLayer::EditorLayer(const Ref<UserPreferences> &userPreferences) : m_UserPreferences(userPreferences)
	{
		// Validate recent projects list and remove entries whose paths no longer exist.
		if (m_UserPreferences)
		{
			// Get a snapshot of the recent projects (returned by value).
			const auto recent = m_UserPreferences->GetRecentProjects();

			for (const auto &entry : recent)
			{
				const RecentProject &proj = entry.second;
				if (proj.filePath.empty())
					continue;

				if (!std::filesystem::exists(proj.filePath))
				{
					EDITOR_TRACE_TAG("EditorLayer", "Removing non-existent recent project: {}", proj.filePath);
					m_UserPreferences->RemoveRecentProject(proj.filePath);
				}
			}
		}
	}

	EditorLayer::~EditorLayer()
	{
		if (ImGui::GetCurrentContext())
		{
		  ::UI::Shutdown();
			ImGui_ImplSDL3_Shutdown();
			ImGui::DestroyContext();
		}
	}

	void EditorLayer::OnAttach()
	{
		memset(s_ProjectNameBuffer, 0, MAX_PROJECT_NAME_LENGTH);
		memset(s_OpenProjectFilePathBuffer, 0, MAX_PROJECT_FILEPATH_LENGTH);
		memset(s_NewProjectFilePathBuffer, 0, MAX_PROJECT_FILEPATH_LENGTH);

		Layer::OnAttach();
		m_Camera.Init();
		Renderer::SetCamera(&m_Camera);
		EDITOR_INFO_TAG("EditorLayer", "Camera initialized and registered with renderer");

		// Initialize ImGui / editor UI layout
		InitEditor();
	}

	void EditorLayer::OnDetach()
	{
		Renderer::SetCamera(nullptr);
		CloseProject(false);
		Layer::OnDetach();
	}

	void EditorLayer::Tick()
	{
		DeltaTime dt;
		// Camera tick is driven by Renderer::Tick() to keep matrix updates aligned
		// with the active render frame and avoid double-processing input.
		
		if (const auto& project = Project::GetActive(); project && project->GetConfig().enableAutosave)
		{
			m_TimeSinceLastSave += dt.GetSeconds();
			if (m_TimeSinceLastSave > project->GetConfig().autosaveInterval)
			{
				SaveSceneAuto();
			}
		}
	}
	
	void EditorLayer::InitEditor()
	{
		EDITOR_INFO_TAG("EDITOR", "Setting up ImGui docking layout");

		ImGui::CreateContext();

		// configure ImGui
		ImGuiIO& io                      = ImGui::GetIO();
		io.ConfigFlags                  |= ImGuiConfigFlags_NavEnableKeyboard;
		io.ConfigFlags                  |= ImGuiConfigFlags_DockingEnable;
		io.ConfigFlags                  |= ImGuiConfigFlags_ViewportsEnable;
		io.ConfigFlags                  |= ImGuiConfigFlags_NoMouseCursorChange; // cursor control is given to ImGui, but dynamically, from the engine
		io.ConfigWindowsResizeFromEdges  = true;
		io.IniFilename                   = "editor.ini";

		// font_bold configuration
		ImFontConfig config; // config for bold font (mainly for use in headers)
		config.GlyphOffset.y = -2.0f;

		const std::filesystem::path fontDir = std::filesystem::path(ResourceCache::GetResourceDirectory(ResourceDirectory::Fonts));
		const std::filesystem::path normalPath = fontDir / "opensans" / "OpenSans-Medium.ttf";
		const std::filesystem::path boldPath = fontDir / "opensans" / "OpenSans-Bold.ttf";

		if (std::filesystem::exists(normalPath))
		{
			UILayer::fontNormal = io.Fonts->AddFontFromFileTTF(normalPath.string().c_str(), s_FontSize * Window::GetDpiScale());
		}
		else
		{
			EDITOR_ERROR_TAG("Editor", "Font not found: %s", normalPath.string().c_str());
			UILayer::fontNormal = io.Fonts->AddFontDefault();
		}

		if (std::filesystem::exists(boldPath))
		{
			UILayer::fontBold = io.Fonts->AddFontFromFileTTF(boldPath.string().c_str(), s_FontSize * Window::GetDpiScale(), &config);
		}
		else
		{
			EDITOR_WARN_TAG("Editor", "Bold font not found: %s", boldPath.string().c_str());
			UILayer::fontBold = nullptr;
		}

		io.FontGlobalScale = s_FontScale;

		// initialize imgui backends
		SEDX_CORE_ASSERT(ImGui_ImplSDL3_InitForVulkan(Window::GetWindow()), "Failed to initialize ImGui's SDL backend");
		::UI::Initialize();

			/*
		const auto window_flags = ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_NoDocking | ImGuiWindowFlags_NoTitleBar |
								  ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove |
								  ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus;

		// Set window position and size
		const ImGuiViewport *viewport = ImGui::GetMainViewport();
		ImGui::SetNextWindowPos(ImVec2(viewport->Pos.x, viewport->Pos.y));
		ImGui::SetNextWindowSize(ImVec2(viewport->Size.x, viewport->Size.y));

		// Set Window Style
		ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
		ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));

		// Begin Window
		const char *name = "##main_window";
		bool open = true;
		ImGui::Begin(name, &open, window_flags);
		ImGui::PopStyleVar(3);

		// Setup docking space
		if (ImGui::GetIO().ConfigFlags & ImGuiConfigFlags_DockingEnable)
		{
			const auto window_id = ImGui::GetID(name);
			if (!ImGui::DockBuilderGetNode(window_id))
			{
				// Reset Current Docking State
				ImGui::DockBuilderRemoveNode(window_id);
				ImGui::DockBuilderAddNode(window_id, ImGuiDockNodeFlags_None);
				ImGui::DockBuilderSetNodeSize(window_id, ImGui::GetMainViewport()->Size);

				// Create dock layout
				ImGuiID dock_main_id = window_id;
				ImGuiID dock_right_id =
					ImGui::DockBuilderSplitNode(dock_main_id, ImGuiDir_Right, 0.17f, nullptr, &dock_main_id);
				ImGuiID dock_right_down_id =
					ImGui::DockBuilderSplitNode(dock_right_id, ImGuiDir_Down, 0.6f, nullptr, &dock_right_id);
				ImGuiID dock_down_id =
					ImGui::DockBuilderSplitNode(dock_main_id, ImGuiDir_Down, 0.22f, nullptr, &dock_main_id);
				ImGuiID dock_down_right_id =
					ImGui::DockBuilderSplitNode(dock_down_id, ImGuiDir_Right, 0.3f, nullptr, &dock_down_id);

				// Dock Windows
				ImGui::DockBuilderDockWindow("World", dock_right_id);
				ImGui::DockBuilderDockWindow("Properties", dock_right_down_id);
				ImGui::DockBuilderDockWindow("Console", dock_down_id);
				ImGui::DockBuilderDockWindow("Assets", dock_down_right_id);
				ImGui::DockBuilderDockWindow("Viewport", dock_main_id);

				ImGui::DockBuilderFinish(dock_main_id);
			}

			ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 0.0f);
			ImGui::DockSpace(window_id, ImVec2(0.0f, 0.0f), ImGuiDockNodeFlags_PassthruCentralNode);
			ImGui::PopStyleVar();
		}

		ImGui::End();*/
	}

	void EditorLayer::OnRender()
	{
		if (!ImGui::GetCurrentContext())
			return;

		ImGui_ImplSDL3_NewFrame();
		ImGui::NewFrame();
		ImGuizmo::BeginFrame();

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
		#ifdef IMGUI_HAS_DOCK
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

				std::string displayName = "Unnamed";
				if (entity->HasComponent<TagComponent>())
				{
					displayName = entity->Name();
				}

				if (ImGui::Selectable(displayName.c_str(), selected))
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

		// Finish ImGui frame and handle multi-viewport rendering after all UI is created
		ImGui::Render();
		ImGuiIO& io = ImGui::GetIO();
		if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
		{
			ImGui::UpdatePlatformWindows();
			ImGui::RenderPlatformWindowsDefault();
		}
	}

	/*
	void EditorLayer::OnUIRender()
	{
		
		// ImGui + Dockspace Setup ------------------------------------------------------------------------------
		ImGuiIO& io = ImGui::GetIO();
		ImGuiStyle& style = ImGui::GetStyle();
		auto boldFont = io.Fonts->Fonts[0];
		auto largeFont = io.Fonts->Fonts[1];

	}*/

	void EditorLayer::OnEvent(Event &event)
	{
		Layer::OnEvent(event);
	}

	bool EditorLayer::OnKeyPressedEvent(KeyPressedEvent &e)
	{
		if (UI::IsWindowFocused("Viewport") || UI::IsWindowFocused("Scene Hierarchy"))
		{
			if ((m_ViewportPanelMouseOver || m_ViewportPanel2MouseOver) && !Input::IsMouseButtonDown(MouseButton::Right) && m_CurrentScene != m_RuntimeScene)
			{
				switch (e.GetKeyCode())
				{
					case KeyCode::Q:
						m_GizmoType = -1;
						break;
					case KeyCode::W:
						m_GizmoType = ImGuizmo::OPERATION::TRANSLATE;
						break;
					case KeyCode::E:
						m_GizmoType = ImGuizmo::OPERATION::ROTATE;
						break;
					case KeyCode::S:
						m_GizmoType = ImGuizmo::OPERATION::SCALE;
						break;
					case KeyCode::F:
					{
						if (SelectionManager::GetSelectionCount(SelectionContext::Scene) == 0)
							break;

						// TODO: Maybe compute average location to focus on? Or maybe cycle through all the selected entities?
						UUID selectedEntityID = SelectionManager::GetSelections(SelectionContext::Scene).front();
						Entity selectedEntity = m_CurrentScene->TryGetEntityWithUUID(selectedEntityID);

						// TODO: Impliment some way to focus on a selected entity.
						//m_Camera.Focus(m_CurrentScene->GetWorldSpaceTransform(selectedEntity).Translation);
						break;
					}
				}

			}

			switch (e.GetKeyCode())
			{
				case KeyCode::Escape:
					SelectionManager::DeselectAll();
					break;
				case KeyCode::Delete:
				{
					auto selectedEntities = SelectionManager::GetSelections(SelectionContext::Scene);
					for (auto entityID : selectedEntities)
					{
						// Can only delete entities that are not child of (dynamic) mesh
						auto entity = m_CurrentScene->TryGetEntityWithUUID(entityID);
						if(!entity.HasComponent<MeshTagComponent>())
						{
							DeleteEntity(entity);
						}
					}
					break;
				}
			}
		}

		if (Input::IsKeyDown(SEDX_KEY_LEFT_CONTROL) && !Input::IsMouseButtonDown(MouseButton::Right))
		{
			switch (e.GetKeyCode())
			{
				case KeyCode::D:
				{
					auto selectedEntities = SelectionManager::GetSelections(SelectionContext::Scene);
					for (const auto& entityID : selectedEntities)
					{
						Entity entity = m_CurrentScene->TryGetEntityWithUUID(entityID);

						// If the entity is a prefab, but pointing to an invalid prefab handle
						if (entity.HasComponent<PrefabComponent>())
						{
							auto prefabID = entity.GetComponent<PrefabComponent>().prefabId;
							if (!AssetManager::IsAssetHandleValid(prefabID))
							{

								break;
							}
						}

						/*
						Entity duplicate = m_CurrentScene->DuplicateEntity(entity);
						SelectionManager::Deselect(SelectionContext::Scene, entity.GetUUID());
						SelectionManager::Select(SelectionContext::Scene, duplicate.GetUUID());*/
					}
					break;
				}
				case KeyCode::G:
					// Toggle grid
					// TODO: this should probably be moved to a viewport renderer option and toggle the grid rendering in the viewport renderer
					break;
				case KeyCode::O:
					OpenProject();
					break;
				case KeyCode::S:
					SaveScene();
					break;
			}

			if (Input::IsKeyDown(SEDX_KEY_LEFT_SHIFT) && Input::IsKeyDown(SEDX_KEY_LEFT_CONTROL))
			{
				switch (e.GetKeyCode())
				{
					case KeyCode::S:
						SaveSceneAs();
						break;
					case KeyCode::A:
						//SelectAllInScene();
						break;
				}
			}
		}

		return false;
	}

	bool EditorLayer::OnMouseButtonPressed(MouseButtonPressedEvent &e)
	{
		if (m_CurrentScene == m_RuntimeScene)
			return false;

		if (e.GetMouseButton() != MouseButton::Left)
			return false;

		if (!m_ViewportPanelMouseOver && !m_ViewportPanel2MouseOver)
			return false;

		if (Input::IsKeyDown(KeyCode::LeftAlt) || Input::IsMouseButtonDown(MouseButton::Right))
			return false;

		if (ImGuizmo::IsOver())
			return false;

		ImGui::ClearActiveID();

		//std::vector<SelectionData> selectionData;

		auto [mouseX, mouseY] = GetMouseViewportSpace(m_ViewportPanelMouseOver);
		if (mouseX > -1.0f && mouseX < 1.0f && mouseY > -1.0f && mouseY < 1.0f)
		{
			/*
			const auto& camera = m_ViewportPanelMouseOver ? m_Camera : m_SecondEditorCamera;
			auto [origin, direction] = CastRay(camera, mouseX, mouseY);

			auto meshEntities = m_CurrentScene->GetAllEntitiesWith<Submesh>();
			for (auto e : meshEntities)
			{
				Entity entity = { e, m_CurrentScene.Get() };
				auto& mc = entity.GetComponent<Submesh>();
				if (auto mesh = AssetManager::GetAsset(mc->); mesh)
				{

					if (auto meshSource = AssetManager::GetAsset(mesh); meshSource)
					{
						auto& submeshes = meshSource->GetSubmeshes();
						auto& submesh = submeshes[mc.SubmeshIndex];
						
						Ray ray = {
							glm::inverse(transform) * glm::vec4(origin, 1.0f),
							glm::inverse(glm::mat3(transform)) * direction
						};

						float t;
						bool intersects = ray.IntersectsAABB(submesh.BoundingBox, t);
						if (intersects)
						{
							const auto& triangleCache = meshSource->GetTriangleCache(mc.SubmeshIndex);
							for (const auto& triangle : triangleCache)
							{
								if (ray.IntersectsTriangle(triangle.V0.Position, triangle.V1.Position, triangle.V2.Position, t))
								{
									selectionData.push_back({ entity, &submesh, t });
									break;
								}
							}
						}
					}						

				}
			}

			auto staticMeshEntities = m_CurrentScene->GetAllEntitiesWith<MeshComponent>();
			for (auto e : staticMeshEntities)
			{
				Entity entity = { e, m_CurrentScene.Get() };
				auto& smc = entity.GetComponent<MeshComponent>();
				if (auto staticMesh = AssetManager::GetAsset(smc.mesh); staticMesh)
				{
					/*
					if (auto meshSource = AssetManager::GetAsset(staticMesh->pHandle); meshSource)
					{
						auto& submeshes = meshSource->GetSubmeshes();
						for (uint32_t i = 0; i < submeshes.size(); i++)
						{
							auto& submesh = submeshes[i];
							glm::mat4 transform = m_CurrentScene->GetWorldSpaceTransformMatrix(entity);
							Ray ray = {
								glm::inverse(transform * submesh.Transform) * glm::vec4(origin, 1.0f),
								glm::inverse(glm::mat3(transform * submesh.Transform)) * direction
							};

							float t;
							bool intersects = ray.IntersectsAABB(submesh.BoundingBox, t);
							if (intersects)
							{
								const auto& triangleCache = meshSource->GetTriangleCache(i);
								for (const auto& triangle : triangleCache)
								{
									if (ray.IntersectsTriangle(triangle.V0.Position, triangle.V1.Position, triangle.V2.Position, t))
									{
										selectionData.push_back({ entity, &submesh, t });
										break;
									}
								}
							}
						}
					}
				}
			}
			
			std::sort(selectionData.begin(), selectionData.end(), [](auto& a, auto& b) { return a.Distance < b.Distance; });

			bool ctrlDown  = Input::IsKeyDown(KeyCode::LeftControl) || Input::IsKeyDown(KeyCode::RightControl);
			bool shiftDown = Input::IsKeyDown(KeyCode::LeftShift)   || Input::IsKeyDown(KeyCode::RightShift);
			if (!ctrlDown)
			{
				SelectionManager::DeselectAll();
			}

			if (!selectionData.empty())
			{
				Entity entity = selectionData.front().Entity;
				if (shiftDown)
				{
					while (entity.GetParent())
					{
						entity = entity.GetParent();
					}
				}
				if (SelectionManager::IsSelected(SelectionContext::Scene, entity.GetUUID()) && ctrlDown)
				{
					SelectionManager::Deselect(SelectionContext::Scene, entity.GetUUID());
				}
				else
				{
					SelectionManager::Select(SelectionContext::Scene, entity.GetUUID());
				}
			}*/
		}
		
		return false;
	}

	std::pair<float, float> EditorLayer::GetMouseViewportSpace(bool primaryViewport)
	{
		auto [mx, my] = ImGui::GetMousePos();
		const auto& viewportBounds = primaryViewport ? m_ViewportBounds : m_SecondViewportBounds;
		mx -= viewportBounds[0].x;
		my -= viewportBounds[0].y;
		auto viewportWidth = viewportBounds[1].x - viewportBounds[0].x;
		auto viewportHeight = viewportBounds[1].y - viewportBounds[0].y;

		return { (mx / viewportWidth) * 2.0f - 1.0f, ((my / viewportHeight) * 2.0f - 1.0f) * -1.0f };
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
		return m_TitleBarHovered;
	}

	void EditorLayer::UpdateWindowTitle(const std::string &sceneName)
	{
		const std::string title = std::format("{0} ({1}) - Scenery Editor X {2}", sceneName, Project::GetActive()->GetConfig().name, SEDX_VERSION);
		Application::Get().GetWindow().SetTitle(title);
		EDITOR_TRACE_TAG("Editor", "Window title updated to: {}", title);
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

	Ref<Viewport> EditorLayer::GetMainViewport()
	{
		return {};
	}

	void EditorLayer::SetMainViewport(const std::string &viewportName)
	{
	}

	void EditorLayer::DeleteEntity(Entity entity)
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
		if (Project::GetActive())
			CloseProject();

		Ref<Project> project = CreateRef<Project>();
		Project::SetActive(project);

		//m_PanelManager->OnProjectChanged(project);
		NewScene();

		//SelectionManager::DeselectAll();

		memset(s_ProjectNameBuffer, 0, MAX_PROJECT_NAME_LENGTH);
		memset(s_OpenProjectFilePathBuffer, 0, MAX_PROJECT_FILEPATH_LENGTH);
		memset(s_NewProjectFilePathBuffer, 0, MAX_PROJECT_FILEPATH_LENGTH);
	}

	void EditorLayer::UpdateCurrentProject()
	{
	}

	void EditorLayer::SaveProject()
	{
	}

	void EditorLayer::CloseProject(bool unloadProject)
	{
		s_ProjectOpen = false;
	}

	void EditorLayer::NewScene(const std::string &name)
	{
		//SelectionManager::DeselectAll();

		//ScriptEngine::SetSceneContext(nullptr, nullptr);
		m_EditorScene = CreateRef<Scene>(name, true);
		//m_PanelManager->SetSceneContext(m_EditorScene);
		//AssetEditorPanel::SetSceneContext(m_EditorScene);
		UpdateWindowTitle(name);
		m_SceneFilePath = std::string();

		m_Camera = Camera();
		m_CurrentScene = m_EditorScene;
		s_SceneOpen = true;
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

	bool EditorLayer::IsSceneOpen()
	{
		return s_SceneOpen;
	}

	bool EditorLayer::IsProjectOpen()
	{
		return s_ProjectOpen;
	}

	} // namespace SceneryEditorX

// ---------------------------------------------------------
