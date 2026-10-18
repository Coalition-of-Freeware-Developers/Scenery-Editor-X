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
#include "editor_camera.h"
#include "editor_scene.h"
#include "editor_theme.h"
#include "editor_ui.h"
#include "editor_xplane.h"
#include <Editor/panels/asset_shelf_panel.h>
#include <Editor/panels/console_panel.h>
#include <Editor/panels/outliner_panel.h>
#include <Editor/panels/properties_panel.h>
#include <Editor/panels/scene_viewport.h>
#include <Editor/panels/settings_panels.h>
#include <Editor/panels/xplane_library_panel.h>
#include <algorithm>
#include <cstdlib>
#include <SceneryEditorX/core/application/application.h>
#include <SceneryEditorX/core/input/input.h>
#include <SceneryEditorX/core/resource/resource_cache.h>
#include <SceneryEditorX/core/time/fps_timer.h>
#include <SceneryEditorX/core/window/window.h>
#include <SceneryEditorX/logging/logging.hpp>
#include <SceneryEditorX/renderer/renderer.h>
#include <SceneryEditorX/renderer/font/font_awesome.h>
#include <SceneryEditorX/renderer/ui/resources/fonts.h>
#include <SceneryEditorX/renderer/ui/source/imgui/imgui_internal.h>
#include <SceneryEditorX/renderer/vulkan/debug/render_doc.h>
#include <SceneryEditorX/scene/entity.h>
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

		constexpr PanelInfo s_StripPanels[] =
		{
			PANEL_PROPERTIES, PANEL_OUTLINER, PANEL_XPLANE, PANEL_ASSETS, PANEL_CONSOLE, PANEL_SCENE_SETTINGS, PANEL_CAMERA, PANEL_RENDERER, PANEL_VIEWPORT
		};

		/* Bump when the default layout changes so existing users get the new arrangement once. */
		constexpr const char *DOCKSPACE_NAME = "SEDX_EditorDockspace_v3";

		constexpr ImGuiWindowFlags SIDE_BAR_FLAGS = ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse | ImGuiWindowFlags_NoSavedSettings;
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

		// X-Plane folder + library index (the first scan runs in the background)
		XPlane::LibraryManager::Init();
		ReadStartupObjects();
		RegisterPanels();
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
		XPlane::SceneObjects::Shutdown();
		XPlane::LibraryManager::Shutdown();
		DetachLogSink();
	}

	void EditorLayer::RegisterPanels()
	{
		m_PanelManager = CreateScope<PanelManager>();

		const std::filesystem::path resources = ResourceCache::GetResourceDirectory();

		m_PanelManager->AddPanel<SceneViewport>(PanelCategory::Viewer, PANEL_VIEWPORT.id, "Viewport", true, &m_Context);
		m_PanelManager->AddPanel<PropertiesPanel>(PanelCategory::Editor, PANEL_PROPERTIES.id, "Properties", true, &m_Context);
		m_PanelManager->AddPanel<OutlinerPanel>(PanelCategory::Editor, PANEL_OUTLINER.id, "Outliner", true, &m_Context);
		m_PanelManager->AddPanel<AssetShelfPanel>(PanelCategory::Editor, PANEL_ASSETS.id, "Assets", true, resources);
		m_PanelManager->AddPanel<ConsolePanel>(PanelCategory::Editor, PANEL_CONSOLE.id, "Console", true, m_LogSink);
		m_PanelManager->AddPanel<XPlaneLibraryPanel>(PanelCategory::Editor, PANEL_XPLANE.id, "X-Plane Library", true, &m_Context);
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
		XPlane::SceneObjects::Tick(ImGui::GetIO().DeltaTime);

		HandleShortcuts();

		// Order matters: each side bar shrinks the viewport work area the dockspace then fills.
		DrawMenuBar();
		DrawStatusBar();
		DrawToolStrip();
		DrawPanelStrip();
		DrawDockSpace();

		if (m_PanelManager)
			m_PanelManager->OnUIRender();

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
	}

	void EditorLayer::DrawMenuBar()
	{
		if (!ImGui::BeginMainMenuBar())
			return;

		if (ImGui::BeginMenu("File"))
		{
			ImGui::MenuItem("New Scene", nullptr, false, false);
			ImGui::MenuItem("Open Project...", "Ctrl+O", false, false);
			ImGui::MenuItem("Save Scene", "Ctrl+S", false, false);
			ImGui::Separator();
			if (ImGui::MenuItem("Exit", "Alt+F4"))
				Application::Get().Stop();
			ImGui::EndMenu();
		}

		if (ImGui::BeginMenu("Edit"))
		{
			ImGui::MenuItem("Undo", "Ctrl+Z", false, false);
			ImGui::MenuItem("Redo", "Ctrl+Y", false, false);
			ImGui::Separator();
			if (ImGui::MenuItem("Delete", "Del", false, m_Context.selectedEntity != 0))
				DeleteSelectedEntity();
			if (ImGui::MenuItem("Deselect", "Esc", false, m_Context.selectedEntity != 0))
				m_Context.selectedEntity = 0;
			ImGui::EndMenu();
		}

		if (ImGui::BeginMenu("Scene"))
		{
			if (ImGui::MenuItem("Add Empty Entity"))
			{
				if (const uint64_t created = EditorScene::CreateEmpty("Empty Entity"); created != 0)
					m_Context.selectedEntity = created;
			}
			ImGui::EndMenu();
		}

		if (ImGui::BeginMenu("X-Plane"))
		{
			if (ImGui::MenuItem(EditorUI::IconLabel(SEDX_ICON_CUBE, "Import Object (.obj)...").c_str()))
				EditorXPlane::ImportObjFile(&m_Context);

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
			ImGui::EndMenu();
		}

		if (ImGui::BeginMenu("Window"))
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
			ImGui::EndMenu();
		}

		if (ImGui::BeginMenu("Viewport"))
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
			ImGui::EndMenu();
		}

		if (ImGui::BeginMenu("Help"))
		{
			if (ImGui::MenuItem("About Scenery Editor X"))
				m_ShowAbout = true;
			ImGui::EndMenu();
		}

		ImGui::EndMainMenuBar();
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
			ImGui::BeginDisabled(EditorCamera::Get() == nullptr);

			if (EditorUI::StripButton(SEDX_ICON_REFRESH, "Orbit navigation\nLMB orbit, RMB pan, wheel zoom", EditorCamera::IsMode(ORBIT), buttonSize))
				EditorCamera::SetMode(ORBIT);
			if (EditorUI::StripButton(SEDX_ICON_VIDEO_CAMERA, "Fly navigation\nRMB look, WASD move, Q/E down/up", EditorCamera::IsMode(FLY), buttonSize))
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
		//  |            |                          | OUTLINER         |
		//  | PROPERTIES |        VIEWPORT          +------------------+
		//  |            |                          | SCENE SETTINGS / |
		//  |            +--------------------------+ CAMERA /         |
		//  |            | ASSETS / CONSOLE         | RENDERER         |
		//  +------------+--------------------------+------------------+
		ImGuiID center = dockspaceId;
		const ImGuiID left = ImGui::DockBuilderSplitNode(center, ImGuiDir_Left, 0.18f, nullptr, &center);
		ImGuiID rightTop = ImGui::DockBuilderSplitNode(center, ImGuiDir_Right, 0.24f, nullptr, &center);
		const ImGuiID rightBottom = ImGui::DockBuilderSplitNode(rightTop, ImGuiDir_Down, 0.72f, nullptr, &rightTop);
		const ImGuiID bottom = ImGui::DockBuilderSplitNode(center, ImGuiDir_Down, 0.28f, nullptr, &center);

		ImGui::DockBuilderDockWindow(PANEL_PROPERTIES.window, left);
		ImGui::DockBuilderDockWindow(PANEL_OUTLINER.window, rightTop);
		ImGui::DockBuilderDockWindow(PANEL_SCENE_SETTINGS.window, rightBottom);
		ImGui::DockBuilderDockWindow(PANEL_CAMERA.window, rightBottom);
		ImGui::DockBuilderDockWindow(PANEL_RENDERER.window, rightBottom);
		ImGui::DockBuilderDockWindow(PANEL_ASSETS.window, bottom);
		ImGui::DockBuilderDockWindow(PANEL_CONSOLE.window, bottom);
		ImGui::DockBuilderDockWindow(PANEL_XPLANE.window, bottom);
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
		if (ImGui::Shortcut(ImGuiKey_Delete, ImGuiInputFlags_RouteGlobal) && m_Context.selectedEntity != 0)
			DeleteSelectedEntity();

		if (ImGui::Shortcut(ImGuiKey_Escape, ImGuiInputFlags_RouteGlobal))
			m_Context.selectedEntity = 0;

		if (ImGui::Shortcut(ImGuiKey_Home, ImGuiInputFlags_RouteGlobal))
			EditorCamera::Reset();
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
		EditorScene::Destroy(m_Context.selectedEntity);
		m_Context.selectedEntity = 0;
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
