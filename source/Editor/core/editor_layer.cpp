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
 * editor_layer.cpp
 * -------------------------------------------------------
 * Created: 01/03/2026
 * -------------------------------------------------------
 */
#include "editor_layer.h"
#include "editor_airports.h"
#include "editor_project.h"
#include "editor_camera.h"
#include "editor_commands.h"
#include "editor_history.h"
#include "editor_scene.h"
#include "editor_selection.h"
#include "editor_tools.h"
#include "project_dialogs.h"
#include "tools_selftest.h"
#include "editor_theme.h"
#include "editor_ui.h"
#include "editor_xplane.h"
#include "facade_authoring.h"
#include "facade_tool.h"
#include "viewport_menus.h"
#include <Editor/panels/airport_panel.h>
#include <Editor/panels/asset_shelf_panel.h>
#include <Editor/panels/console_panel.h>
#include <Editor/panels/facade_editor_panel.h>
#include <Editor/panels/facade_inspector_panel.h>
#include <Editor/panels/outliner_panel.h>
#include <Editor/panels/project_panel.h>
#include <Editor/panels/properties_panel.h>
#include <Editor/panels/scene_viewport.h>
#include <Editor/panels/settings_panels.h>
#include <Editor/panels/xplane_library_panel.h>
#include <algorithm>
#include <chrono>
#include <cstdlib>
#include <type_traits>
#if defined(_MSC_VER) && defined(_DEBUG)
#include <crtdbg.h>
#endif
#include <SceneryEditorX/core/application/application.h>
#include <SceneryEditorX/core/frame_profiler.h>
#include <SceneryEditorX/core/input/input.h>
#include <SceneryEditorX/core/resource/resource_cache.h>
#include <SceneryEditorX/core/time/fps_timer.h>
#include <SceneryEditorX/core/window/window.h>
#include <SceneryEditorX/logging/logging.hpp>
#include <SceneryEditorX/renderer/gpu_scene.h>
#include <SceneryEditorX/renderer/renderer.h>
#include <SceneryEditorX/renderer/font/font_awesome.h>
#include <SceneryEditorX/renderer/ui/resources/fonts.h>
#include <SceneryEditorX/renderer/ui/source/imgui/imgui_internal.h>
#include <SceneryEditorX/renderer/vulkan/debug/render_doc.h>
#include <SceneryEditorX/scene/entity.h>
#include <SceneryEditorX/xplane/xp_airport_manager.h>
#include <SceneryEditorX/xplane/xp_datarefs.h>
#include <SceneryEditorX/xplane/xp_library_manager.h>
#include <SceneryEditorX/xplane/xp_scene.h>

// ---------------------------------------------------------

namespace SceneryEditorX
{
	namespace
	{
		/**
		 * Panel registry: PanelManager id, ImGui window name ("LABEL###Id") and strip icon.
		 * The window names must match what each panel passes to EditorUI::BeginPanel().
		 */
		struct PanelInfo
		{
			const char *id;
			const char *window;
			const char *icon;
		};

		constexpr PanelInfo PANEL_VIEWPORT			= { "Viewport",			"VIEWPORT###Viewport",				SEDX_ICON_DESKTOP };
		constexpr PanelInfo PANEL_PROPERTIES		= { "Properties",		"PROPERTIES###Properties",			SEDX_ICON_SLIDERS };
		constexpr PanelInfo PANEL_OUTLINER			= { "Outliner",			"OUTLINER###Outliner",				SEDX_ICON_LIST };
		constexpr PanelInfo PANEL_ASSETS			= { "Assets",			"ASSETS###Assets",					SEDX_ICON_FOLDER_OPEN };
		constexpr PanelInfo PANEL_CONSOLE			= { "Console",			"CONSOLE###Console",				SEDX_ICON_TERMINAL };
		constexpr PanelInfo PANEL_SCENE_SETTINGS	= { "SceneSettings",	"SCENE SETTINGS###SceneSettings",	SEDX_ICON_GLOBE };
		constexpr PanelInfo PANEL_CAMERA			= { "CameraSettings",	"CAMERA###CameraSettings",			SEDX_ICON_CAMERA };
		constexpr PanelInfo PANEL_RENDERER			= { "RendererStats",	"RENDERER###RendererStats",			SEDX_ICON_TACHOMETER };
		constexpr PanelInfo PANEL_XPLANE			= { "XPlaneLibrary",	"X-PLANE LIBRARY###XPlaneLibrary",	SEDX_ICON_PLANE };
		constexpr PanelInfo PANEL_AIRPORTS			= { "Airports",			"AIRPORTS###Airports",				SEDX_ICON_GLOBE };
		constexpr PanelInfo PANEL_PROJECT			= { "Project",			"PROJECT###Project",				SEDX_ICON_FILE_TEXT };
		constexpr PanelInfo PANEL_FACADES			= { "FacadeInspector",	"FACADES###FacadeInspector",		SEDX_ICON_BUILDING_O };
		constexpr PanelInfo PANEL_FACADE_EDITOR		= { "FacadeEditor",		"FACADE EDITOR###FacadeEditor",		SEDX_ICON_PICTURE_O };

		constexpr PanelInfo s_StripPanels[] =
		{
			PANEL_PROPERTIES, PANEL_OUTLINER, PANEL_PROJECT, PANEL_AIRPORTS, PANEL_XPLANE, PANEL_FACADES, PANEL_FACADE_EDITOR, PANEL_ASSETS, PANEL_CONSOLE, PANEL_SCENE_SETTINGS, PANEL_CAMERA, PANEL_RENDERER, PANEL_VIEWPORT
		};

		/* Bump when the default layout changes so existing users get the new arrangement once. */
		constexpr const char *DOCKSPACE_NAME = "SEDX_EditorDockspace_v7";

		constexpr ImGuiWindowFlags SIDE_BAR_FLAGS = ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse | ImGuiWindowFlags_NoSavedSettings;

		// --entity-bench: heap calls made by the bench thread (the Debug CRT reports every allocation to the hook)
		thread_local bool t_CountAllocations = false;
		uint64_t s_BenchAllocations = 0;
		uint64_t s_BenchBytes = 0;

#if defined(_MSC_VER) && defined(_DEBUG)
		int CountAllocationsHook(const int type, void *, const size_t size, const int blockType, long, const unsigned char *, int)
		{
			if (t_CountAllocations && blockType != _CRT_BLOCK && (type == _HOOK_ALLOC || type == _HOOK_REALLOC))
			{
				++s_BenchAllocations;
				s_BenchBytes += size;
			}
			return TRUE;
		}
#endif

		/**
		 * @brief Times building, copying and walking Entity handles by value over every scene entity and counts the
		 * heap allocations it makes (Debug only): TryGetEntityWithUUID, a copy and GetParent per entity.
		 */
		void RunEntityBench()
		{
			std::vector<UUID> ids;
			ids.reserve(Scene::GetEntities().size());
			for (const Entity *entity : Scene::GetEntities())
				ids.push_back(entity->GetUUID());

			for (int run = 0; run < 3; ++run)
			{
				uint64_t checksum = 0;
				s_BenchAllocations = 0;
				s_BenchBytes = 0;
#if defined(_MSC_VER) && defined(_DEBUG)
				const _CRT_ALLOC_HOOK previousHook = _CrtSetAllocHook(CountAllocationsHook);
#endif
				t_CountAllocations = true;
				const auto start = std::chrono::steady_clock::now();
				for (const UUID &id : ids)
				{
					const Entity handle = Scene::TryGetEntityWithUUID(id);
					const Entity copy = handle;
					const Entity parent = copy.GetParent();
					checksum += static_cast<uint32_t>(copy) + (parent ? 1u : 0u);
				}
				const double milliseconds = std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - start).count();
				t_CountAllocations = false;
#if defined(_MSC_VER) && defined(_DEBUG)
				_CrtSetAllocHook(previousHook);
#endif
				EDITOR_INFO_TAG("Perf", "Entity bench {}: {} x (TryGetEntityWithUUID + copy + GetParent) in {:.2f} ms ({:.0f} ns each), "
					"{} heap allocations ({} bytes), sizeof(Entity) {}, trivially copyable {} (checksum {})",
					run, ids.size(), milliseconds, ids.empty() ? 0.0 : milliseconds * 1.0e6 / static_cast<double>(ids.size()),
					s_BenchAllocations, s_BenchBytes, sizeof(Entity), std::is_trivially_copyable_v<Entity>, checksum);
			}

			// The scene recycles destroyed entities' runtime records: handles to destroyed entities must miss them, and
			// new entities (on recycled records) must start clean. Plugin is a plain Component without side effects.
			constexpr size_t COUNT = 256;
			std::vector<Entity> first;
			std::vector<const Entity*> firstStored;
			int failures = 0;
			for (size_t i = 0; i < COUNT; ++i)
			{
				const Entity entity = Scene::CreateEntity("EntityBenchRecycle");
				Entity copy = entity;
				copy.AddComponentByType(ComponentType::Plugin);
				Entity(static_cast<entt::entity>(entity), Scene::GetActive()).SetActive(false); // a bare handle reaches the record too
				const Entity *stored = Scene::GetStoredEntity(entity);
				failures += !stored || stored->GetActive() || !stored->GetComponentByType(ComponentType::Plugin) ||
					stored->GetComponentByType(ComponentType::Plugin)->GetEntity() != stored;
				first.push_back(entity);
				firstStored.push_back(stored);
			}
			for (const Entity &entity : first)
				Scene::DestroyEntity(entity);
			(void)Scene::GetEntities(); // compacts the list, which frees the records for reuse

			// Handles to destroyed entities (these still carry their old record's address) must stay dead, also once
			// new entities live on those records with a Plugin and the active flag set.
			const auto countDeadHandleFailures = [&first]()
			{
				int count = 0;
				for (const Entity &old : first)
					count += old.IsValid() || old.GetActive() || Scene::GetStoredEntity(old) != nullptr || old.GetComponentByType(ComponentType::Plugin) != nullptr;
				return count;
			};
			failures += countDeadHandleFailures();

			size_t reused = 0;
			std::vector<Entity> second;
			for (size_t i = 0; i < COUNT; ++i)
			{
				Entity entity = Scene::CreateEntity("EntityBenchRecycle");
				const Entity *stored = Scene::GetStoredEntity(entity);
				reused += std::ranges::find(firstStored, stored) != firstStored.end();
				failures += !stored || !stored->GetActive() || stored->GetComponentByType(ComponentType::Plugin) != nullptr;
				entity.AddComponentByType(ComponentType::Plugin);
				second.push_back(entity);
			}
			failures += countDeadHandleFailures();
			for (const Entity &entity : second)
				Scene::DestroyEntity(entity);

			EDITOR_INFO_TAG("Perf", "Entity recycle check: {} entities, {} recycled records, {} failures", COUNT, reused, failures);
		}
	}

	// ---------------------------------------------------------

	EditorLayer::EditorLayer(const Ref<UserPreferences> &userPreferences) : Layer("EditorLayer"), m_UserPreferences(userPreferences)
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

	EditorLayer::~EditorLayer() = default;

	void EditorLayer::OnAttach()
	{
		AttachLogSink();

		// X-Plane folder + library index (the first scan runs in the background), then the airport index and the Gateway client
		XPlane::LibraryManager::Init();
		EditorAirports::Init();
		EditorProjects::Init(m_UserPreferences);
		ReadStartupObjects();
		RegisterPanels();
		// Commands the menus offer (menu bar, right-click quad menu, Space hotbox), and the menus themselves
		ViewportMenus::RegisterCommands();
		FacadeTool::RegisterCommands();
		FacadeAuthoring::RegisterCommands();
		BuildMainMenus();
		EditorCamera::CaptureHome();
		UpdateWindowTitle();

		if (!ImGui::GetCurrentContext())
		{
			EDITOR_ERROR_TAG("EditorLayer", "No ImGui context: the UI layer failed to initialize, the editor UI is disabled");
			return;
		}

		EditorTheme::Apply(Window::GetDpiScale());

		const ImGuiIO &io = ImGui::GetIO();
		fontNormal = io.FontDefault;
		fontBold = io.Fonts->Fonts.Size > 1 ? UI::Fonts::Get("Bold") : io.FontDefault;

		EDITOR_INFO_TAG("EditorLayer", "Editor shell ready");
	}

	void EditorLayer::OnDetach()
	{
		// The UI layer (an overlay) detaches first, so ImGui is already gone here: no ImGui calls.
		Camera::SetInputEnabled(true);
		m_PanelManager.reset();
		EditorProjects::Shutdown();	// Keeps unsaved project work as recovery data while the scene still exists
		EditorAirports::Shutdown();
		XPlane::SceneObjects::Shutdown();
		XPlane::LibraryManager::Shutdown();
		DetachLogSink();
	}

	void EditorLayer::RegisterPanels()
	{
		m_PanelManager = CreateScope<PanelManager>();

		const std::filesystem::path resources = VFS::FromUtf8(ResourceCache::GetResourceDirectory());

		m_PanelManager->AddPanel<SceneViewport>(PanelCategory::Viewer, PANEL_VIEWPORT.id, "Viewport", true, &m_Context);
		m_PanelManager->AddPanel<PropertiesPanel>(PanelCategory::Editor, PANEL_PROPERTIES.id, "Properties", true, &m_Context);
		m_PanelManager->AddPanel<OutlinerPanel>(PanelCategory::Editor, PANEL_OUTLINER.id, "Outliner", true, &m_Context);
		m_PanelManager->AddPanel<AssetShelfPanel>(PanelCategory::Editor, PANEL_ASSETS.id, "Assets", true, resources);
		m_PanelManager->AddPanel<ConsolePanel>(PanelCategory::Editor, PANEL_CONSOLE.id, "Console", true, m_LogSink);
		m_PanelManager->AddPanel<XPlaneLibraryPanel>(PanelCategory::Editor, PANEL_XPLANE.id, "X-Plane Library", true, &m_Context);
		m_PanelManager->AddPanel<AirportPanel>(PanelCategory::Editor, PANEL_AIRPORTS.id, "Airports", true, &m_Context);
		m_PanelManager->AddPanel<ProjectPanel>(PanelCategory::Editor, PANEL_PROJECT.id, "Project", true, &m_Context);
		m_PanelManager->AddPanel<FacadeInspectorPanel>(PanelCategory::Editor, PANEL_FACADES.id, "Facades", true, &m_Context);
		m_PanelManager->AddPanel<FacadeEditorPanel>(PanelCategory::Editor, PANEL_FACADE_EDITOR.id, "Facade Editor", false, &m_Context);
		m_PanelManager->AddPanel<SceneSettingsPanel>(PanelCategory::Settings, PANEL_SCENE_SETTINGS.id, "Scene Settings", true);
		m_PanelManager->AddPanel<CameraSettingsPanel>(PanelCategory::Settings, PANEL_CAMERA.id, "Camera", true);
		m_PanelManager->AddPanel<RendererStatsPanel>(PanelCategory::Settings, PANEL_RENDERER.id, "Renderer", true);

		// Restore which panels were open last session.
		m_PanelManager->Deserialize();
	}

	void EditorLayer::AttachLogSink()
	{
		m_LogSink = std::make_shared<EditorLogSink>();
		m_LogSink->set_level(spdlog::level::info);

		for (const std::shared_ptr<spdlog::logger> *logger : { &Log::GetCoreLogger(), &Log::GetEditorLogger(), &Log::GetEditorConsoleLogger() })
		{
			if (*logger)
				(*logger)->sinks().push_back(m_LogSink);
		}
	}

	void EditorLayer::DetachLogSink()
	{
		if (!m_LogSink)
			return;

		for (const std::shared_ptr<spdlog::logger> *logger : { &Log::GetCoreLogger(), &Log::GetEditorLogger(), &Log::GetEditorConsoleLogger() })
		{
			if (*logger)
				std::erase((*logger)->sinks(), m_LogSink);
		}

		m_LogSink.reset();
	}

	// ---------------------------------------------------------

	void EditorLayer::OnUIRender()
	{
		// X-Plane objects: dataref clock, animation, LODs and night lighting, before the panels read the scene.
		PlaceStartupObjects();

		// --entity-bench runs once the scene has over 1000 entities and their count held for 300 frames (the startup
		// airport finished loading)
		if (m_EntityBenchPending)
		{
			const size_t entityCount = Scene::GetEntities().size();
			m_EntityBenchStableFrames = entityCount == m_EntityBenchCount ? m_EntityBenchStableFrames + 1 : 0;
			m_EntityBenchCount = entityCount;
			if (m_EntityBenchStableFrames >= 300 && entityCount > 1000)
			{
				m_EntityBenchPending = false;
				RunEntityBench();
			}
		}
		{
			SEDX_PERF_SCOPE("Main: SceneObjects::Tick");
			XPlane::SceneObjects::Tick(ImGui::GetIO().DeltaTime);
		}

		// Airports: the startup airport, scene rebuilds after edits, finished Gateway downloads
		{
			SEDX_PERF_SCOPE("Main: EditorAirports::Tick");
			EditorAirports::Tick(&m_Context);
		}

		// Project: the startup project, scene edits flowing into it, autosave, the window title
		{
			SEDX_PERF_SCOPE("Main: EditorProjects::Tick");
			EditorProjects::Tick(&m_Context);
			FacadeAuthoring::Tick();	// The facade editor's live preview
		}

		// Selection: follow what other panels selected last frame, drop what rebuilds removed
		EditorSelection::Sync(&m_Context);

		HandleShortcuts();

		// Order matters: each side bar shrinks the viewport work area the dockspace then fills.
		DrawMenuBar();
		DrawStatusBar();
		DrawToolStrip();
		DrawPanelStrip();
		DrawDockSpace();

		if (m_PanelManager)
		{
			SEDX_PERF_SCOPE("Main: panels");
			m_PanelManager->OnUIRender();
		}

		// The viewport's right-click quad menu and Space hotbox, over the panels
		ViewportMenus::Draw(&m_Context);

		// Inspect (menus, PROPERTIES, the library panel) brings the FACADES panel up
		if (FacadeTool::ConsumeOpenRequest())
		{
			if (PanelData *panel = FindPanel(PANEL_FACADES.id))
				panel->IsOpen = true;
			ImGui::SetWindowFocus(PANEL_FACADES.window);
		}
		// So do the facade editor's commands (new building / curb, edit) for the FACADE EDITOR panel
		if (FacadeAuthoring::ConsumeOpenRequest())
		{
			if (PanelData *panel = FindPanel(PANEL_FACADE_EDITOR.id))
				panel->IsOpen = true;
			ImGui::SetWindowFocus(PANEL_FACADE_EDITOR.window);
		}

		// Outlines for the selected / hovered objects (uploaded only when they changed)
		EditorSelection::UpdateHighlight();

		// After a layout build: open on Substance's default tabs (the X-Plane library browser and the first settings tab) and
		// give focus to the viewport. Newly docked windows otherwise select themselves as they get submitted.
		if (m_FocusViewportFrames > 0)
		{
			for (const PanelInfo &info : { PANEL_XPLANE, PANEL_SCENE_SETTINGS })
			{
				ImGuiWindow *window = ImGui::FindWindowByName(info.window);
				if (window && window->DockNode && window->DockNode->TabBar)
					window->DockNode->TabBar->NextSelectedTabId = window->TabId;
			}

			ImGui::SetWindowFocus(PANEL_VIEWPORT.window);
			--m_FocusViewportFrames;
		}

		DrawAboutPopup();
		DrawDebugWindows();
		EditorAirports::DrawModals(&m_Context);
		EditorProjects::DrawModals(&m_Context);
	}

	void EditorLayer::BuildMainMenus()
	{
		m_MainMenus.clear();
		m_MainMenus.emplace_back("File", [this]() { EditorProjects::DrawFileMenu(&m_Context); });
		m_MainMenus.emplace_back("Edit", [this]() { DrawEditMenu(); });
		m_MainMenus.emplace_back("Scene", [this]() { DrawSceneMenu(); });
		m_MainMenus.emplace_back("X-Plane", [this]() { DrawXPlaneMenu(); });
		m_MainMenus.emplace_back("Facade", [this]() { DrawFacadeMenu(); });
		m_MainMenus.emplace_back("Window", [this]() { DrawWindowMenu(); });
		m_MainMenus.emplace_back("Viewport", [this]() { DrawViewportMenu(); });
		m_MainMenus.emplace_back("Help", [this]()
		{
			if (ImGui::MenuItem("About Scenery Editor X"))
				m_ShowAbout = true;
			ImGui::Separator();
			ImGui::TextDisabled("Viewport: right-click for the quad menu, hold Space for the hotbox");
		});
		ViewportMenus::SetMainMenus(m_MainMenus);
	}

	void EditorLayer::DrawMenuBar()
	{
		if (!ImGui::BeginMainMenuBar())
			return;
		for (const auto &[name, items] : m_MainMenus)
		{
			if (ImGui::BeginMenu(name.c_str()))
			{
				items();
				ImGui::EndMenu();
			}
		}
		ImGui::EndMainMenuBar();
	}

	void EditorLayer::DrawEditMenu()
	{
		const std::string undo = EditorHistory::CanUndo() ? "Undo " + EditorHistory::GetUndoLabel() : std::string("Undo");
		const std::string redo = EditorHistory::CanRedo() ? "Redo " + EditorHistory::GetRedoLabel() : std::string("Redo");
		if (ImGui::MenuItem(undo.c_str(), "Ctrl+Z", false, EditorHistory::CanUndo()))
			EditorHistory::Undo();
		if (ImGui::MenuItem(redo.c_str(), "Ctrl+Y", false, EditorHistory::CanRedo()))
			EditorHistory::Redo();
		ImGui::Separator();
		if (ImGui::MenuItem("Delete", "Del", false, !EditorSelection::IsEmpty()))
			DeleteSelectedEntity();
		EditorCommands::MenuItem("item.duplicate", &m_Context);
		if (ImGui::MenuItem("Deselect", "Esc", false, !EditorSelection::IsEmpty()))
			EditorSelection::Clear(&m_Context);
		if (ImGui::MenuItem(EditorUI::IconLabel(SEDX_ICON_CROSSHAIRS, "Frame Selection").c_str(), "F", false, !EditorSelection::IsEmpty()))
			EditorTools::FrameSelection();
		ImGui::Separator();
		EditorCommands::MenuItem("item.hide", &m_Context);
		EditorCommands::MenuItem("item.unhide_all", &m_Context);
		EditorCommands::MenuItem("item.lock", &m_Context);
		ImGui::Separator();
		for (int t = 0; t < static_cast<int>(EditorTools::Tool::Count); ++t)
		{
			const EditorTools::Tool tool = static_cast<EditorTools::Tool>(t);
			if (ImGui::MenuItem(EditorUI::IconLabel(EditorTools::GetToolIcon(tool), EditorTools::GetToolName(tool)).c_str(), EditorTools::GetToolKey(tool), EditorTools::GetTool() == tool))
				EditorTools::SetTool(tool);
		}
	}

	void EditorLayer::DrawSceneMenu()
	{
		if (ImGui::MenuItem("Add Empty Entity"))
		{
			if (const uint64_t created = EditorScene::CreateEmpty("Empty Entity"); created != 0)
				EditorSelection::Select(&m_Context, EditorSelection::Target::ForEntity(created));
		}
	}

	void EditorLayer::DrawXPlaneMenu()
	{
		if (ImGui::MenuItem(EditorUI::IconLabel(SEDX_ICON_CUBE, "Import Object (.obj)...").c_str()))
			EditorXPlane::ImportObjFile(&m_Context);

		ImGui::Separator();
		if (PanelData *panel = FindPanel(PANEL_AIRPORTS.id))
		{
			if (ImGui::MenuItem(EditorUI::IconLabel(SEDX_ICON_GLOBE, "Airports").c_str(), nullptr, panel->IsOpen))
			{
				panel->IsOpen = true;
				ImGui::SetWindowFocus(PANEL_AIRPORTS.window);
			}
		}
		if (ImGui::MenuItem(EditorUI::IconLabel(SEDX_ICON_FOLDER_OPEN, "Open Airport (apt.dat)...").c_str()))
			EditorAirports::BrowseForAirport(&m_Context);
		{
			const XPlane::AirportDocument *document = EditorAirports::GetDocument();
			const bool open = document != nullptr;
			if (ImGui::MenuItem(EditorUI::IconLabel(SEDX_ICON_FLOPPY_O, "Save Airport").c_str(), "Ctrl+S", false, open && document->IsDirty()))
				EditorAirports::SaveAirport();
			if (ImGui::MenuItem("Save Airport As...", "Ctrl+Shift+S", false, open))
				EditorAirports::SaveAirportAs();
			if (ImGui::MenuItem(EditorUI::IconLabel(SEDX_ICON_CROSSHAIRS, "Frame Airport").c_str(), nullptr, false, open))
				EditorAirports::FrameAirport();
			if (ImGui::MenuItem(EditorUI::IconLabel(SEDX_ICON_TIMES, "Close Airport").c_str(), nullptr, false, open))
				EditorAirports::CloseAirport(&m_Context);
		}
		if (ImGui::MenuItem(EditorUI::IconLabel(SEDX_ICON_REFRESH, "Rescan Airports").c_str(), nullptr, false,
			XPlane::AirportManager::GetScanState() != XPlane::ScanState::Scanning))
			XPlane::AirportManager::Rescan();

		ImGui::Separator();
		if (PanelData *panel = FindPanel(PANEL_XPLANE.id))
		{
			if (ImGui::MenuItem(EditorUI::IconLabel(SEDX_ICON_BOOK, "Library Browser").c_str(), nullptr, panel->IsOpen))
			{
				panel->IsOpen = true;
				ImGui::SetWindowFocus(PANEL_XPLANE.window);
			}
		}
		if (ImGui::MenuItem(EditorUI::IconLabel(SEDX_ICON_FOLDER_OPEN, "Set X-Plane 12 Folder...").c_str()))
			EditorXPlane::BrowseForInstall();
		if (ImGui::MenuItem(EditorUI::IconLabel(SEDX_ICON_PLUS, "Track Library Folder...").c_str()))
			EditorXPlane::BrowseForTrackedFolder();
		if (ImGui::MenuItem(EditorUI::IconLabel(SEDX_ICON_REFRESH, "Rescan Libraries").c_str(), nullptr, false,
			XPlane::LibraryManager::GetScanState() != XPlane::ScanState::Scanning))
			XPlane::LibraryManager::Rescan();

		ImGui::Separator();
		const XPlane::InstallInfo &install = XPlane::LibraryManager::GetInstallInfo();
		ImGui::TextDisabled("%s", install.IsValid() ? install.root.string().c_str() : install.GetProblem().c_str());
	}

	void EditorLayer::DrawFacadeMenu()
	{
		// Making facades: buildings from a texture, sidewalks / curbs from a cross-section, editing one (a copy of a library facade)
		for (const char *id : { "facade.author.building", "facade.author.curb", "facade.author.edit", "facade.author.open" })
			EditorCommands::MenuItem(id, &m_Context);
		ImGui::Separator();

		// Drawing: the tool and its modes, what to draw with
		EditorCommands::MenuItem("facade.tool", &m_Context);
		for (const char *id : { "facade.draw.rectangle", "facade.draw.polygon", "facade.draw.circle", "facade.draw.line" })
			EditorCommands::MenuItem(id, &m_Context);
		if (ImGui::BeginMenu(EditorUI::IconLabel(SEDX_ICON_BOOK, "Library").c_str()))
		{
			FacadeTool::DrawLibraryMenu();
			ImGui::EndMenu();
		}
		if (ImGui::BeginMenu(EditorUI::IconLabel(SEDX_ICON_STAR, "Recent").c_str(), !FacadeTool::GetRecent().empty()))
		{
			for (const std::string &resource : FacadeTool::GetRecent())
			{
				if (ImGui::MenuItem(resource.c_str(), nullptr, resource == FacadeTool::GetSettings().resource))
				{
					FacadeTool::SetResource(resource);
					EditorTools::SetTool(EditorTools::Tool::Facade);
				}
			}
			ImGui::EndMenu();
		}

		// The selected facades
		ImGui::SeparatorText("Selected facades");
		const std::vector<std::string> facades = FacadeTool::GetSelectedFacades();
		if (ImGui::BeginMenu(EditorUI::IconLabel(SEDX_ICON_TH_LIST, "Wall Type").c_str(), !facades.empty()))
		{
			FacadeTool::DrawWallTypeMenu(facades);
			ImGui::EndMenu();
		}
		if (ImGui::BeginMenu(EditorUI::IconLabel(SEDX_ICON_ARROWS_V, "Height").c_str(), !facades.empty()))
		{
			FacadeTool::DrawHeightMenu(facades);
			ImGui::EndMenu();
		}
		if (ImGui::BeginMenu(EditorUI::IconLabel(SEDX_ICON_RETWEET, "Replace With").c_str(), !facades.empty()))
		{
			FacadeTool::DrawReplaceMenu(facades);
			ImGui::EndMenu();
		}
		for (const char *id : { "facade.pick_walls", "facade.front", "facade.subdivide", "facade.square", "facade.reverse", "facade.use", "facade.inspect" })
			EditorCommands::MenuItem(id, &m_Context);
		ImGui::Separator();
		EditorCommands::MenuItem("facade.labels", &m_Context);
		if (ImGui::MenuItem(EditorUI::IconLabel(SEDX_ICON_REFRESH, "Reload Facades").c_str(), nullptr, false, EditorProjects::GetDocument() != nullptr))
			EditorProjects::ReloadArt();
		EditorUI::ItemTooltip("Read the project's facades and other art again after editing them on disk, and redraw");
	}

	void EditorLayer::DrawWindowMenu()
	{
		for (const PanelInfo &info : s_StripPanels)
		{
			if (PanelData *panel = FindPanel(info.id))
				ImGui::MenuItem(EditorUI::IconLabel(info.icon, panel->Name).c_str(), nullptr, &panel->IsOpen);
		}

		ImGui::Separator();
		if (ImGui::MenuItem("Reset Layout"))
			m_Context.requestLayoutReset = true;

		ImGui::Separator();
		if (ImGui::BeginMenu("Dear ImGui"))
		{
			ImGui::MenuItem("Metrics / Debugger", nullptr, &m_ShowMetrics);
			ImGui::MenuItem("Style Editor", nullptr, &m_ShowStyleEditor);
			ImGui::MenuItem("Demo Window", nullptr, &m_ShowDemo);
			ImGui::EndMenu();
		}
	}

	void EditorLayer::DrawViewportMenu()
	{
		const bool hasCamera = EditorCamera::Get() != nullptr;
		if (ImGui::MenuItem("Orbit Navigation", nullptr, EditorCamera::IsMode(ORBIT), hasCamera))
			EditorCamera::SetMode(ORBIT);
		if (ImGui::MenuItem("Fly Navigation", nullptr, EditorCamera::IsMode(FLY), hasCamera))
			EditorCamera::SetMode(FLY);
		ImGui::Separator();
		if (ImGui::MenuItem("Perspective", nullptr, EditorCamera::IsProjection(CameraType::Perspective), hasCamera))
			EditorCamera::SetProjection(CameraType::Perspective);
		if (ImGui::MenuItem("Orthographic", nullptr, EditorCamera::IsProjection(CameraType::Orthographic), hasCamera))
			EditorCamera::SetProjection(CameraType::Orthographic);
		ImGui::Separator();
		if (ImGui::MenuItem("Reset Camera", "Home", false, hasCamera))
			EditorCamera::Reset();
	}

	void EditorLayer::DrawStatusBar()
	{
		using namespace EditorTheme;
		const float dpi = ImGui::GetStyle().FontScaleDpi;

		ImGui::PushStyleColor(ImGuiCol_WindowBg, ToStyle(Palette::MenuBar));
		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(10.0f * dpi, 3.0f * dpi));
		const bool open = ImGui::BeginViewportSideBar("##StatusBar", ImGui::GetMainViewport(), ImGuiDir_Down, StatusBarHeight * dpi, SIDE_BAR_FLAGS);
		ImGui::PopStyleVar();
		ImGui::PopStyleColor();

		if (open)
		{
			// Left: latest warning / error, like Substance's red status line. Click opens the console.
			LogEntry problem;
			if (m_LogSink && m_LogSink->GetLatestProblem(problem))
			{
				const ImVec4 color = ToStyle(problem.level >= spdlog::level::err ? Palette::Error : Palette::Warning);
				ImGui::PushStyleColor(ImGuiCol_Text, color);
				ImGui::Text("[%s] %s", problem.source.c_str(), problem.message.c_str());
				ImGui::PopStyleColor();
				if (ImGui::IsItemClicked())
				{
					if (PanelData *console = FindPanel(PANEL_CONSOLE.id))
						console->IsOpen = true;
					ImGui::SetWindowFocus(PANEL_CONSOLE.window);
				}
				EditorUI::ItemTooltip("Latest warning or error. Click to open the console.");
			}
			else
			{
				ImGui::TextDisabled("Ready");
			}

			// X-Plane objects loading in the background
			ImGui::SameLine(0.0f, 20.0f * dpi);
			EditorXPlane::DrawObjectLoadProgress(260.0f * dpi);

			// Right: frame time, GPU memory, version
			// GPU memory walks every allocation; twice a second is plenty for a status readout.
			static GPUMemoryStats s_Memory;
			static double s_MemoryQueriedAt = -1.0;
			if (const double now = ImGui::GetTime(); s_MemoryQueriedAt < 0.0 || now - s_MemoryQueriedAt > 0.5)
			{
				s_Memory = Renderer::GetGPUMemoryStats();
				s_MemoryQueriedAt = now;
			}

			const double frameMs = FPSTimer::GetDeltaTimeSmoothedMs();
			const GPUMemoryStats &memory = s_Memory;
			char stats[160];
			snprintf(stats, sizeof(stats), "%.0f fps  |  %.2f ms  |  GPU %.0f MiB  |  Version: %s (Vulkan)",
				frameMs > 0.0 ? 1000.0 / frameMs : 0.0, frameMs, static_cast<double>(memory.used) / (1024.0 * 1024.0), SEDX_VERSION_STRING);

			const float statsWidth = ImGui::CalcTextSize(stats).x;
			ImGui::SameLine(std::max(ImGui::GetCursorPosX(), ImGui::GetWindowWidth() - statsWidth - 10.0f * dpi));
			ImGui::TextDisabled("%s", stats);
		}
		ImGui::End();
	}

	void EditorLayer::DrawToolStrip()
	{
		using namespace EditorTheme;
		const float dpi = ImGui::GetStyle().FontScaleDpi;
		const float width = ToolStripWidth * dpi;
		const float padding = 4.0f * dpi;

		ImGui::PushStyleColor(ImGuiCol_WindowBg, ToStyle(Palette::ToolStrip));
		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(padding, 8.0f * dpi));
		ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(padding, 4.0f * dpi));
		const bool open = ImGui::BeginViewportSideBar("##ToolStrip", ImGui::GetMainViewport(), ImGuiDir_Left, width, SIDE_BAR_FLAGS);
		ImGui::PopStyleVar(2);
		ImGui::PopStyleColor();

		if (open)
		{
			const float buttonSize = width - padding * 2.0f;

			// Editing tools first, as in Substance's tool strip
			for (int t = 0; t < static_cast<int>(EditorTools::Tool::Count); ++t)
			{
				const EditorTools::Tool tool = static_cast<EditorTools::Tool>(t);
				ImGui::PushID(t);
				if (EditorUI::StripButton(EditorTools::GetToolIcon(tool), EditorTools::GetToolTooltip(tool), EditorTools::GetTool() == tool, buttonSize))
					EditorTools::SetTool(tool);
				ImGui::PopID();
			}

			ImGui::Separator();
			ImGui::BeginDisabled(EditorCamera::Get() == nullptr);

			if (EditorUI::StripButton(SEDX_ICON_REFRESH, "Orbit navigation\nAlt+LMB orbit, MMB pan, RMB look, wheel zoom", EditorCamera::IsMode(ORBIT), buttonSize))
				EditorCamera::SetMode(ORBIT);
			if (EditorUI::StripButton(SEDX_ICON_VIDEO_CAMERA, "Fly navigation\nRMB look, hold RMB + WASD move, Q/E down/up", EditorCamera::IsMode(FLY), buttonSize))
				EditorCamera::SetMode(FLY);

			ImGui::Separator();

			if (EditorUI::StripButton(SEDX_ICON_CUBE, "Perspective projection", EditorCamera::IsProjection(CameraType::Perspective), buttonSize))
				EditorCamera::SetProjection(CameraType::Perspective);
			if (EditorUI::StripButton(SEDX_ICON_TH_LARGE, "Orthographic projection", EditorCamera::IsProjection(CameraType::Orthographic), buttonSize))
				EditorCamera::SetProjection(CameraType::Orthographic);

			ImGui::Separator();

			if (EditorUI::StripButton(SEDX_ICON_CROSSHAIRS, "Reset camera (Home)", false, buttonSize))
				EditorCamera::Reset();

			ImGui::EndDisabled();
		}
		ImGui::End();
	}

	void EditorLayer::DrawPanelStrip()
	{
		using namespace EditorTheme;
		const float dpi = ImGui::GetStyle().FontScaleDpi;
		const float width = ToolStripWidth * dpi;
		const float padding = 4.0f * dpi;

		ImGui::PushStyleColor(ImGuiCol_WindowBg, ToStyle(Palette::ToolStrip));
		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(padding, 8.0f * dpi));
		ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(padding, 4.0f * dpi));
		const bool open = ImGui::BeginViewportSideBar("##PanelStrip", ImGui::GetMainViewport(), ImGuiDir_Right, width, SIDE_BAR_FLAGS);
		ImGui::PopStyleVar(2);
		ImGui::PopStyleColor();

		if (open)
		{
			const float buttonSize = width - padding * 2.0f;
			for (const PanelInfo &info : s_StripPanels)
			{
				PanelData *panel = FindPanel(info.id);
				if (!panel)
					continue;

				ImGui::PushID(info.id);
				if (EditorUI::StripButton(info.icon, panel->Name, panel->IsOpen, buttonSize, EditorUI::StripStyle::Tinted))
				{
					panel->IsOpen = !panel->IsOpen;
					m_PanelManager->Serialize();
				}
				ImGui::PopID();
			}
		}
		ImGui::End();
	}

	void EditorLayer::DrawDockSpace()
	{
		const ImGuiID dockspaceId = ImHashStr(DOCKSPACE_NAME);

		if (m_Context.requestLayoutReset || ImGui::DockBuilderGetNode(dockspaceId) == nullptr)
		{
			BuildDefaultLayout(dockspaceId);
			m_Context.requestLayoutReset = false;
		}

		ImGui::DockSpaceOverViewport(dockspaceId, ImGui::GetMainViewport(), ImGuiDockNodeFlags_None);
	}

	void EditorLayer::BuildDefaultLayout(const ImGuiID dockspaceId)
	{
		m_FocusViewportFrames = 2;

		// Every panel comes back for a reset, including ones the user had closed.
		for (const PanelInfo &info : s_StripPanels)
		{
			if (PanelData *panel = FindPanel(info.id))
				panel->IsOpen = true;
		}

		const ImGuiViewport *viewport = ImGui::GetMainViewport();
		ImGui::DockBuilderRemoveNode(dockspaceId);
		ImGui::DockBuilderAddNode(dockspaceId, ImGuiDockNodeFlags_DockSpace);
		ImGui::DockBuilderSetNodeSize(dockspaceId, viewport->WorkSize);

		//  +------------+--------------------------+------------------+
		//  |            |                          | OUTLINER/PROJECT |
		//  | PROPERTIES |        VIEWPORT          +------------------+
		//  |            |                          | SCENE SETTINGS / |
		//  |            +--------------------------+ CAMERA /         |
		//  |            | ASSETS / CONSOLE         | RENDERER/FACADES/|
		//  |            |                          | FACADE EDITOR    |
		//  +------------+--------------------------+------------------+
		ImGuiID center = dockspaceId;
		const ImGuiID left = ImGui::DockBuilderSplitNode(center, ImGuiDir_Left, 0.18f, nullptr, &center);
		ImGuiID rightTop = ImGui::DockBuilderSplitNode(center, ImGuiDir_Right, 0.24f, nullptr, &center);
		const ImGuiID rightBottom = ImGui::DockBuilderSplitNode(rightTop, ImGuiDir_Down, 0.72f, nullptr, &rightTop);
		const ImGuiID bottom = ImGui::DockBuilderSplitNode(center, ImGuiDir_Down, 0.28f, nullptr, &center);

		ImGui::DockBuilderDockWindow(PANEL_PROPERTIES.window, left);
		ImGui::DockBuilderDockWindow(PANEL_OUTLINER.window, rightTop);
		ImGui::DockBuilderDockWindow(PANEL_PROJECT.window, rightTop);
		ImGui::DockBuilderDockWindow(PANEL_SCENE_SETTINGS.window, rightBottom);
		ImGui::DockBuilderDockWindow(PANEL_CAMERA.window, rightBottom);
		ImGui::DockBuilderDockWindow(PANEL_RENDERER.window, rightBottom);
		ImGui::DockBuilderDockWindow(PANEL_FACADES.window, rightBottom);
		ImGui::DockBuilderDockWindow(PANEL_FACADE_EDITOR.window, rightBottom);
		ImGui::DockBuilderDockWindow(PANEL_ASSETS.window, bottom);
		ImGui::DockBuilderDockWindow(PANEL_CONSOLE.window, bottom);
		ImGui::DockBuilderDockWindow(PANEL_XPLANE.window, bottom);
		ImGui::DockBuilderDockWindow(PANEL_AIRPORTS.window, bottom);
		ImGui::DockBuilderDockWindow(PANEL_VIEWPORT.window, center);
		ImGui::DockBuilderFinish(dockspaceId);
	}

	void EditorLayer::DrawAboutPopup()
	{
		if (m_ShowAbout)
		{
			ImGui::OpenPopup("About Scenery Editor X");
			m_ShowAbout = false;
		}

		ImGui::SetNextWindowPos(ImGui::GetMainViewport()->GetCenter(), ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
		if (ImGui::BeginPopupModal("About Scenery Editor X", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
		{
			UI::Fonts::PushFont("Bold");
			ImGui::TextUnformatted("Scenery Editor X");
			UI::Fonts::PopFont();
			ImGui::TextDisabled("Version %s (%s, Vulkan)", SEDX_VERSION_STRING, Application::GetConfigurationName());
			ImGui::Spacing();
			ImGui::TextUnformatted("Scenery authoring for X-Plane 12.");
			ImGui::TextUnformatted("Coalition of Freeware Developers. MIT licensed.");
			ImGui::Spacing();
			ImGui::TextDisabled("Dear ImGui %s (docking)", IMGUI_VERSION);
			ImGui::Spacing();
			if (ImGui::Button("Close", ImVec2(120.0f * ImGui::GetStyle().FontScaleDpi, 0.0f)) || ImGui::IsKeyPressed(ImGuiKey_Escape))
				ImGui::CloseCurrentPopup();
			ImGui::EndPopup();
		}
	}

	void EditorLayer::DrawDebugWindows()
	{
		if (m_ShowMetrics)
			ImGui::ShowMetricsWindow(&m_ShowMetrics);

		if (m_ShowStyleEditor)
		{
			if (ImGui::Begin("Dear ImGui Style Editor", &m_ShowStyleEditor))
				ImGui::ShowStyleEditor();
			ImGui::End();
		}

		if (m_ShowDemo)
			ImGui::ShowDemoWindow(&m_ShowDemo);
	}

	void EditorLayer::HandleShortcuts()
	{
		// Global routes lose to whatever has focus, so typing in a text field never triggers these.
		// Tools: Del, Esc and (over the viewport) Q W E R V, F, X
		EditorTools::HandleShortcuts(&m_Context);

		if (ImGui::Shortcut(ImGuiMod_Ctrl | ImGuiKey_Z, ImGuiInputFlags_RouteGlobal))
			EditorHistory::Undo();
		if (ImGui::Shortcut(ImGuiMod_Ctrl | ImGuiKey_Y, ImGuiInputFlags_RouteGlobal) || ImGui::Shortcut(ImGuiMod_Ctrl | ImGuiMod_Shift | ImGuiKey_Z, ImGuiInputFlags_RouteGlobal))
			EditorHistory::Redo();

		if (ImGui::Shortcut(ImGuiKey_Home, ImGuiInputFlags_RouteGlobal))
			EditorCamera::Reset();

		// The open project (which saves its airport too), else the open airport
		if (ImGui::Shortcut(ImGuiMod_Ctrl | ImGuiKey_N, ImGuiInputFlags_RouteGlobal))
			EditorProjects::NewProject(&m_Context);
		if (ImGui::Shortcut(ImGuiMod_Ctrl | ImGuiKey_O, ImGuiInputFlags_RouteGlobal))
			EditorProjects::BrowseOpen(&m_Context);
		if (ImGui::Shortcut(ImGuiMod_Ctrl | ImGuiKey_E, ImGuiInputFlags_RouteGlobal))
			ProjectDialogs::OpenExport();
		if (ImGui::Shortcut(ImGuiMod_Ctrl | ImGuiMod_Shift | ImGuiKey_E, ImGuiInputFlags_RouteGlobal))
			ProjectDialogs::QuickExport();
		if (ImGui::Shortcut(ImGuiMod_Ctrl | ImGuiKey_S, ImGuiInputFlags_RouteGlobal))
		{
			if (EditorProjects::GetDocument())
				EditorProjects::Save();
			else if (EditorAirports::GetDocument())
				EditorAirports::SaveAirport();
		}
		if (ImGui::Shortcut(ImGuiMod_Ctrl | ImGuiMod_Shift | ImGuiKey_S, ImGuiInputFlags_RouteGlobal))
		{
			if (EditorProjects::GetDocument())
				EditorProjects::SaveAs();
			else if (EditorAirports::GetDocument())
				EditorAirports::SaveAirportAs();
		}
	}

	void EditorLayer::ReadStartupObjects()
	{
		const PlatformContext *platform = Application::Get().GetPlatformContext();
		if (!platform)
			return;

		const std::vector<std::string> &args = platform->GetCommandLineArgs();
		XPlane::DatarefEnvironment &environment = XPlane::Datarefs::GetEnvironment();
		for (size_t i = 0; i < args.size(); ++i)
		{
			if (args[i] == "--xp-lights-on")
				environment.forceLightsOn = true; // lights and LIT textures in daylight
			else if (args[i] == "--perf-log")
				Perf::SetEnabled(true); // frame timings in the log every two seconds
			else if (args[i] == "--no-gpu-scene")
				GpuScene::SetEnabled(false); // one CPU draw call per renderable (comparison / debugging)
			else if (args[i] == "--no-gpu-occlusion")
				GpuScene::SetOcclusionCulling(false); // GPU scene without Hi-Z occlusion culling (comparison)
			else if (args[i] == "--lock-camera")
				Camera::SetInputLocked(true); // the startup view stays put (captures, A/B measurements)
			else if (args[i] == "--entity-bench")
				m_EntityBenchPending = true; // Entity handle lookups / copies, once the scene has settled
			else if (args[i] == "--tools-selftest")
				ToolsSelfTest::Enable(); // drives the viewport tools (picking, moves, node edits, undo) and logs PASS / FAIL
			else if (args[i] == "--tools-selftest-showcase")
			{
				ToolsSelfTest::Enable();
				ToolsSelfTest::EnableShowcase(); // then shows the node tool's drawing for a while (frame captures) and undoes it
			}
			else if (args[i] == "--facade-showcase")
			{
				ToolsSelfTest::Enable();
				ToolsSelfTest::EnableFacadeShowcase(); // holds the facade tests' facades with wall labels and the quad menu open (frame captures)
			}
			else if (i + 1 >= args.size())
				break;
			else if (args[i] == "--xp-object")
				m_StartupObjectFiles.push_back(args[++i]);
			else if (args[i] == "--xp-asset")
				m_StartupAssets.push_back(args[++i]);
			else if (args[i] == "--xp-time")
				environment.localTimeHours = std::clamp(std::strtof(args[++i].c_str(), nullptr), 0.0f, 24.0f); // local time in hours
			else if (args[i] == "--xp-position")
			{
				// "x,y,z" in meters (+X east, +Y up, +Z south)
				const char *text = args[++i].c_str();
				char *end = nullptr;
				for (float &value : m_StartupPosition)
				{
					value = std::strtof(text, &end);
					text = *end == ',' ? end + 1 : end;
				}
				m_HasStartupPosition = true;
			}
		}
	}

	void EditorLayer::PlaceStartupObjects()
	{
		// Files load right away; library paths wait for the first library scan.
		const float *position = m_HasStartupPosition ? m_StartupPosition : nullptr;
		const auto frame = [position](const uint64_t root)
		{
			if (root != 0 && position)
				XPlane::SceneObjects::FrameObject(root); // placing at a position skips the framing
		};

		for (const std::string &file : m_StartupObjectFiles)
			frame(EditorXPlane::PlaceObjFile(file, std::string(), &m_Context, position));
		m_StartupObjectFiles.clear();

		if (!m_StartupAssets.empty() && XPlane::LibraryManager::GetGeneration() > 0)
		{
			for (const std::string &asset : m_StartupAssets)
				frame(EditorXPlane::PlaceLibraryAsset(asset, &m_Context, position));
			m_StartupAssets.clear();
		}
	}

	void EditorLayer::DeleteSelectedEntity()
	{
		EditorTools::DeleteSelection(&m_Context);
	}

	PanelData *EditorLayer::FindPanel(const char *panelId) const
	{
		return m_PanelManager ? m_PanelManager->GetPanelData(Hash::GenerateFNV1A(std::string_view(panelId))) : nullptr;
	}

	// ---------------------------------------------------------

	void EditorLayer::OnEvent(Event &event)
	{
		EventDispatcher dispatcher(event);
		dispatcher.Dispatch<KeyPressedEvent>([this](KeyPressedEvent &e) { return OnKeyPressed(e); });
	}

	bool EditorLayer::OnKeyPressed(KeyPressedEvent &event)
	{
		// Ctrl+F12: RenderDoc frame capture (works regardless of UI focus)
		if (event.GetKeyCode() == KeyCode::F12 && Input::IsKeyDown(SEDX_KEY_LEFT_CONTROL))
		{
			EDITOR_INFO_TAG("Editor", "RenderDoc frame capture requested via Ctrl+F12");
			RenderDoc::FrameCapture();
			return true;
		}

		return false;
	}

	void EditorLayer::UpdateWindowTitle() const
	{
		const std::string &sceneName = Scene::GetName();
		const std::string title = std::format("{} - Scenery Editor X {}", sceneName.empty() ? "Untitled Scene" : sceneName, SEDX_VERSION_STRING);
		Window::SetTitle(title);
	}

}

// ---------------------------------------------------------
