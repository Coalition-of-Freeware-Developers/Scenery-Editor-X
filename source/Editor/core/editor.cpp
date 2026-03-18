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
 * editor.cpp
 * -------------------------------------------------------
 * Created: 13/4/2025
 * -------------------------------------------------------
 */
#include "editor.h"
#include "editor_layer.h"
#include "Editor/projects/project.h"
#include "Editor/settings/editor_settings.h"
#include "Editor/ui/ui_impl.h"

#include <ImGuizmo.h>
#include <imgui.h>
#include <imgui_impl_sdl3.h>
#include <imgui_internal.h>
#include <SceneryEditorX/core/application/application.h>
#include <SceneryEditorX/core/resource/resource_cache.h>
#include <SceneryEditorX/core/window/window.h>
#include <SceneryEditorX/project/project.h>

// ---------------------------------------------------------

namespace SceneryEditorX
{
	// Static editor instance (declared in editor.h)
	Editor* editor = nullptr;
	
	// ---------------------------------------------------------

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

	static std::filesystem::path s_ProjectSolutionPath = "";
	static std::vector<std::string> s_ClArguments;
	static uint32_t s_ClArg_flags = 0;

	static float s_FontSize  = 18.0f;
	static float s_FontScale = 1.0f;


	static void ProcessEvent(Event &event)
	{
		SDL_Event* event_sdl = static_cast<SDL_Event*>(std::get<void*>(event));
		ImGui_ImplSDL3_ProcessEvent(event_sdl);
	}


	// -------------------------------------------------------

	namespace UI
	{
		class UIContextImpl;
	}

	namespace
	{
		void WriteCiTestFile(const uint32_t value)
		{
			if (Editor::HasArgument("-ci_test"))
			{
				std::ofstream file("ci_test.txt");
				if (file.is_open())
				{
					file << value;
					file.close();
				}
			}
		}

	}

	/*
	Editor::Editor(const std::vector<std::string> &args) : Application(args)
	{
		arguments = args;

		const auto start = std::chrono::high_resolution_clock::now();
		renderContext = RenderContext::Get();

		// Initialize the renderer (this creates the RenderDispatcher as well)
		Renderer::SetRenderData(Application::Get().GetWindow().GetRenderData());

		// TODO: Move project loading to a separate function
		// activeProject->ReadProjCache();
		// assetManager.LoadProject(cacheData.projectPath, cacheData.binPath);
		// m_UserPreferences->GetRecentProjects();
		// scene = assetManager.GetInitialScene();
		// camera = assetManager.GetMainCamera(scene);

		// m_TitleBarActiveColor = m_TitleBarTargetColor = Colors::Theme::titlebarGreen;
		Renderer::Init();

		// ImGui::CreateContext(); // TODO: Not sure if this is the right location for this. Maybe move to UI initialization.

		const auto end = std::chrono::high_resolution_clock::now();
		auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
	}
	*/

	Editor::Editor(const PlatformContext &context) : Application(context)
	{
		s_ClArguments = context.GetCommandLineArgs();
		ProcessClArgs(); // Process command line arguments to set internal flags before initialization

		const auto start = std::chrono::high_resolution_clock::now();
		
		SEDX_CORE_INFO_TAG("Editor", "=== Initializing Editor with PlatformContext ===");
		SEDX_CORE_INFO_TAG("Editor", "Working Directory: {}", context.GetWorkingDirectory());
		SEDX_CORE_INFO_TAG("Editor", "Temp Directory: {}", context.GetTempDirectory());

		// TODO: Move project loading to a separate function
		// activeProject->ReadProjCache();
		// assetManager.LoadProject(cacheData.projectPath, cacheData.binPath);
		// m_UserPreferences->GetRecentProjects();
		// scene = assetManager.GetInitialScene();
		// camera = assetManager.GetMainCamera(scene);

		// m_TitleBarActiveColor = m_TitleBarTargetColor = Colors::Theme::titlebarGreen;

		PushLayer(new SceneryEditorX::EditorLayer());

		const auto end = std::chrono::high_resolution_clock::now();
		auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
		SEDX_CORE_INFO_TAG("Editor", "Editor initialization complete ({} ms)", duration);
	}

	Editor::Editor(const PlatformContext& context, const Ref<UserPreferences> &userPreferences) : Application(context), m_UserPreferences(userPreferences)
	{
		s_ClArguments = context.GetCommandLineArgs();
		ProcessClArgs(); // Process command line arguments to set internal flags before initialization

		const auto start = std::chrono::high_resolution_clock::now();
		
		SEDX_CORE_INFO_TAG("Editor", "=== Initializing Editor with PlatformContext and UserPreferences ===");
		SEDX_CORE_INFO_TAG("Editor", "Working Directory: {}", context.GetWorkingDirectory());
		SEDX_CORE_INFO_TAG("Editor", "Temp Directory: {}", context.GetTempDirectory());

		// TODO: Move project loading to a separate function
		// activeProject->ReadProjCache();
		// assetManager.LoadProject(cacheData.projectPath, cacheData.binPath);
		// m_UserPreferences->GetRecentProjects();
		// scene = assetManager.GetInitialScene();
		// camera = assetManager.GetMainCamera(scene);

		// m_TitleBarActiveColor = m_TitleBarTargetColor = Colors::Theme::titlebarGreen;
		if (!Window::IsVisible())
		{
			SEDX_CORE_WARN_TAG("Editor", "Main window is not visible after creation.");
		}

		PushLayer(new SceneryEditorX::EditorLayer());

		const auto end = std::chrono::high_resolution_clock::now();
		auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
		SEDX_CORE_INFO_TAG("Editor", "Editor initialization complete ({} ms)", duration);
	}

	Editor::~Editor()
	{
		if (ImGui::GetCurrentContext())
		{
			ImGui::Shutdown();
			ImGui_ImplSDL3_Shutdown();
			ImGui::DestroyContext();
		}

	}

	void Editor::Run()
	{
		SEDX_CORE_INFO_TAG("Editor", "=== Starting Editor Main Loop ===");

		Application::Run();

		SEDX_CORE_INFO_TAG("Editor", "=== Editor Main Loop Ended ===");
	}

	void Editor::Tick()
	{
		//SEDX_PROFILE_SCOPE("Editor::Tick");

		// Per-frame editor logic
		/*
		if (m_ShowStatisticsPanel)
		{
			UI_StatisticsPanel();
		}
		*/
		
		/*// Tick project systems
		if (Project::GetActive())
		{
			UpdateCurrentProject();
		}*/

	}

	void Editor::Stop()
	{
		Application::Stop();
	}

	void Editor::OnRender()
	{

	}

	void Editor::OnUpdate()
	{

	}

	void Editor::OnShutdown()
	{
		Application::OnShutdown();
	}

	void Editor::InitEditor()
	{
	    SEDX_CORE_INFO_TAG("EDITOR", "Setting up ImGui docking layout");

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
		
		const std::string dir_fonts = ResourceCache::GetResourceDirectory(ResourceDirectory::Fonts) + "/";
		fontNormal            = io.Fonts->AddFontFromFileTTF((dir_fonts + "OpenSans/OpenSans-Medium.ttf").c_str(), s_FontSize * Window::GetDpiScale());
		fontBold              = io.Fonts->AddFontFromFileTTF((dir_fonts + "OpenSans/OpenSans-Bold.ttf").c_str(), s_FontSize * Window::GetDpiScale(), &config);
		io.FontGlobalScale     = s_FontScale;

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

	void Editor::OnEvent(Event &event)
	{
	}

	void Editor::UpdateWindowTitle(const std::string &sceneName)
	{
		const std::string title = std::format("{0} ({1}) - Scenery Editor X {2}", sceneName, Project::GetActive()->GetConfig().name, SEDX_VERSION);
		Application::Get().GetWindow().SetTitle(title);
		SEDX_CORE_TRACE_TAG("Editor", "Window title updated to: {}", title);
	}

	void Editor::OnInit()
	{
		SEDX_CORE_INFO("=== Editor OnInit ===");

		// Initialize ImGui after window is created
		InitEditor();

		/*// Load default or startup project
		if (m_UserPreferences && !m_UserPreferences->StartupProject.empty())
		{
			OpenProject(m_UserPreferences->StartupProject);
		}
		else
		{
			EmptyProject();
		}
		*/
		SEDX_CORE_TRACE_TAG("Editor", "Initialization complete");
	}


	/**
	 * @brief Recreates frame-related resources.
	 *
	 * This method is called when the viewport size has changed to rebuild
	 * swap chain images, framebuffers, and other resources needed for rendering.
	 * It properly handles cleanup of old resources and initialization of new ones
	 * based on the current viewport dimensions.
	 */
	/*
	void Editor::RecreateFrameResources()
	{
		// Wait for the device to finish all operations
		if (device != VK_NULL_HANDLE)
			vkDeviceWaitIdle(device);

		// Clean up existing viewport resources
		CleanupViewportResources();

		// Tick viewport size from the new size
		viewportData.SetViewportSize(newViewportSize.GetViewportSize());
		viewportData.viewportResized = false;

		// Create new viewport resources with updated dimensions
		CreateViewportResources();

		// Log the viewport recreation
		EDITOR_INFO("Viewport resources recreated with size: {}x{}",
			viewportData.GetViewportSize().width,
			viewportData.GetViewportSize().height);
	}
	*/

	/*
	void Editor::OnAttach()
	{
		using namespace glm;

		memset(s_ProjectNameBuffer, 0, MAX_PROJECT_NAME_LENGTH);
		memset(s_OpenProjectFilePathBuffer, 0, MAX_PROJECT_FILEPATH_LENGTH);
		memset(s_NewProjectFilePathBuffer, 0, MAX_PROJECT_FILEPATH_LENGTH);

		// Should we ever want to actually show editor layer panels in Hazel::Runtime
		// then these lines need to be added to RuntimeLayer::Attach()
		EditorResources::Init();

		for (int i = 0; i < 4; ++i)
		{
			std::string name = "Viewport " + std::to_string(i + 1);
			auto viewport = Ref<Viewport>::Create(name, this);

			m_EditorViewports.emplace_back(viewport);
		}

		if (!m_EditorViewports.empty())
		{
			m_EditorViewports[0]->SetIsVisible(true);
			SetMainViewport(m_EditorViewports[0]->GetName());
		}

		/////////// Configure Panels ///////////
		m_PanelManager = CreateScope<PanelManager>();
		Ref<SceneHierarchyPanel> sceneHierarchyPanel = m_PanelManager->AddPanel<SceneHierarchyPanel>(PanelCategory::View, SCENE_HIERARCHY_PANEL_ID, "Scene Hierarchy", true, m_EditorScene);
		sceneHierarchyPanel->SetEntityDeletedCallback([this](Entity entity) { OnEntityDeleted(entity); });
		sceneHierarchyPanel->SetMeshAssetConvertCallback([this](Entity entity, Ref<MeshSource> meshSource) { OnCreateMeshFromMeshSource(entity, meshSource); });
		sceneHierarchyPanel->SetInvalidMetadataCallback([this](Entity entity, AssetHandle handle) { SceneHierarchyInvalidMetadataCallback(entity, handle); });
		sceneHierarchyPanel->AddEntityContextMenuPlugin([this](Entity entity) { SceneHierarchySetEditorCameraTransform(entity); });

		Ref<ContentBrowserPanel> contentBrowser = m_PanelManager->AddPanel<ContentBrowserPanel>(PanelCategory::View, CONTENT_BROWSER_PANEL_ID, "Content Browser", true);
		contentBrowser->RegisterItemActivateCallbackForType(AssetType::Scene, [this](const AssetMetadata& metadata)
		{
			OpenScene(Project::GetEditorAssetManager()->GetFileSystemPath(metadata));
		});

		contentBrowser->RegisterItemActivateCallbackForType(AssetType::ScriptFile, [this](const AssetMetadata& metadata)
		{
			IO::FileSystem::OpenExternally(Project::GetEditorAssetManager()->GetFileSystemPath(metadata));
		});

		contentBrowser->RegisterAssetCreatedCallback([this](const AssetMetadata& metadata)
		{
			if (metadata.Type == AssetType::ScriptFile)
				RegenerateProjectScriptSolution(Project::GetProjectDirectory());
		});

		contentBrowser->RegisterAssetDeletedCallback([this](const AssetMetadata& metadata)
		{
			if (metadata.Type == AssetType::ScriptFile)
				RegenerateProjectScriptSolution(Project::GetProjectDirectory());
		});

		m_PanelManager->AddPanel<ProjectSettingsWindow>(PanelCategory::Edit, PROJECT_SETTINGS_PANEL_ID, "Project Settings", false);
		m_PanelManager->AddPanel<ApplicationSettingsPanel>(PanelCategory::Edit, APPLICATION_SETTINGS_PANEL_ID, "Application Settings", false);
		m_PanelManager->AddPanel<ECSDebugPanel>(PanelCategory::View, ECS_DEBUG_PANEL_ID, "ECS Debug", false, m_EditorScene);
		m_ConsolePanel = m_PanelManager->AddPanel<EditorConsolePanel>(PanelCategory::View, CONSOLE_PANEL_ID, "Log", true);
		m_PanelManager->AddPanel<MaterialsPanel>(PanelCategory::View, MATERIALS_PANEL_ID, "Materials", true);
		m_PanelManager->AddPanel<AssetManagerPanel>(PanelCategory::View, ASSET_MANAGER_PANEL_ID, "Asset Manager", false);

		Ref<SceneRendererPanel> sceneRendererPanel = m_PanelManager->AddPanel<SceneRendererPanel>(PanelCategory::View, SCENE_RENDERER_PANEL_ID, "Scene Renderer", true);

		if (!m_UserPreferences->StartupProject.empty())
			OpenProject(m_UserPreferences->StartupProject);
		else
			SEDX_CORE_VERIFY(false, "No project provided!");

		if (!Project::GetActive())
			EmptyProject();

		// AssetManager::UnloadAllAssetPacks();
		// AssetManager::AddAssetPack(assetPack);

		for (Ref<Viewport>& viewport : m_EditorViewports)
		{
			viewport->Init(m_CurrentScene);
		}

		// TODO: do we need to do this for all viewports? when we set new main viewport
		// sceneRendererPanel->SetContext(GetMainViewport()->GetRenderer());

		// AssetEditorPanel::RegisterDefaultEditors();

		if (m_UserPreferences->ShowWelcomeScreen)
			UI_ShowWelcomePopup();
	}
	*/

	void Editor::ProcessClArgs()
	{
		// Common simple flags that exist in the repo already:
		// -ci_test  -> used by CI to write a small indicator file (see WriteCiTestFile above)
		// You can add more handlers here for other flags (e.g. --headless, --log=level, --no-gui, etc.)

		SEDX_CORE_TRACE_TAG("Editor", "Processing command line arguments");

		// Example: existing helper writes a CI file when -ci_test is present
		if (HasArgument("-ci_test"))
		{
			// The helper in this TU will write "ci_test.txt" containing 1
			WriteCiTestFile(1);
		}

		// Iterate and parse key=value style args
		for (const auto &arg : s_ClArguments)
		{
			// --headless or -no-ui : run without showing UI (example usage, implement the mode as needed)
			if (arg == "--headless" || arg == "-no-ui")
			{
				SEDX_CORE_INFO_TAG("Editor", "Starting in headless/no-ui mode due to argument: {}", arg);
				// Set any internal flags or call methods to enter headless mode
				// e.g., Application::Get().SetHeadless(true);   // implement as needed
			}

			// Change logging level at startup
			// THIS HAS BEEN MOVED TO PLATFORM CONTEXT INIT
			/*
			constexpr std::string logPrefix = "--verbose";
			if (arg.starts_with(logPrefix))
			{
				std::string level = arg.substr(logPrefix.size());
				SEDX_CORE_INFO_TAG("Editor", "Requested log level: {}", level);
				// Use your logging API to set the level, e.g. Log::SetLevelFromString(level);
				// If you don't have such a helper, map strings to levels here and call Log::SetLevel(...)
			}
			*/

			// Example: --run-task=name  -> you could dispatch internal tasks or tests
			constexpr std::string_view taskPrefix = "--run-task=";
			if (arg.starts_with(taskPrefix))
			{
				std::string taskName = arg.substr(taskPrefix.size());
				SEDX_CORE_INFO_TAG("Editor", "Dispatching startup task: {}", taskName);
				// Dispatch your task: if (taskName == "build-shaders") BuildShaderPack();
			}
		}
	}

	/*
	bool Editor::UI_TitleBarHitTest(int x, int y) const
	{
		return false;
	}
	*/

	float Editor::GetSnapValue()
	{
		const auto& editorSettings = EditorSettings::Get();

		switch (m_GizmoType)
		{
			case ImGuizmo::OPERATION::TRANSLATE: return editorSettings.translationSnapValue;
			case ImGuizmo::OPERATION::ROTATE: return editorSettings.rotationSnapValue;
			case ImGuizmo::OPERATION::SCALE: return editorSettings.scaleSnapValue;
		}
		return 0.0f;
	}

	static auto operator<(const ImVec2 &lhs, const ImVec2 &rhs) { return lhs.x < rhs.x && lhs.y < rhs.y; }

	bool Editor::HasArgument(const std::string &argument)
	{
		for (const auto &arg : s_ClArguments)
		{
			if (arg == argument)
				return true;
		}

		return false;
	}

}

/// -------------------------------------------------------
