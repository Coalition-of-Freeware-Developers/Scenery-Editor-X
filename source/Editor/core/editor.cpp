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
#include <Editor/core/editor.h>
#include "Editor/projects/project.h"
#include <ImGuizmo.h>
#include <imgui.h>
#include <imgui_impl_sdl3.h>
#include <imgui_internal.h>
#include <Editor/settings/editor_settings.h>
#include <SceneryEditorX/core/application/application.h>
#include <SceneryEditorX/core/platform/settings/settings.h>
#include <SceneryEditorX/core/window/window.h>
#include <SceneryEditorX/project/project.h>
#include <SceneryEditorX/renderer/vulkan/render_context.h>
#include <SceneryEditorX/ui/ui.h>
#include <SceneryEditorX/ui/ui_context.h>

// ---------------------------------------------------------

namespace SceneryEditorX
{
    // Static renderer instance (declared in editor.h)
    Editor::GraphicsEngine Editor::gfxEngine;

    #define MAX_PROJECT_NAME_LENGTH 255
	#define MAX_PROJECT_FILEPATH_LENGTH 512

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

    // -------------------------------------------------------

	namespace UI
    {
        class UIContextImpl;
    }

    namespace
    {
        std::vector<std::string> arguments;
        uint32_t flags = 0;

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

    /**
	 * -------------------------------------------------------
	 * Editor static Variables
	 * -------------------------------------------------------
	 */

    // Unique pointer to the application window
    static Scope<Window> g_Window;

    /**
	 * -------------------------------------------------------
	 * FORWARD FUNCTION DECLARATIONS
	 * -------------------------------------------------------
	 */

    // static void initVulkan(GraphicsEngine &gfxEngine);

    // -------------------------------------------------------

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

    Editor::Editor(const PlatformContext& context) : Application(context)
    {
        arguments = context.GetCommandLineArgs();

        const auto start = std::chrono::high_resolution_clock::now();
        
        SEDX_CORE_INFO_TAG("EDITOR", "=== Initializing Editor with PlatformContext ===");
        SEDX_CORE_INFO_TAG("EDITOR", "  Working Directory: {}", context.GetWorkingDirectory());
        SEDX_CORE_INFO_TAG("EDITOR", "  Temp Directory: {}", context.GetTempDirectory());
        
        renderContext = RenderContext::Get();


        // TODO: Move project loading to a separate function
        // activeProject->ReadProjCache();
        // assetManager.LoadProject(cacheData.projectPath, cacheData.binPath);
        // m_UserPreferences->GetRecentProjects();
        // scene = assetManager.GetInitialScene();
        // camera = assetManager.GetMainCamera(scene);

        // m_TitleBarActiveColor = m_TitleBarTargetColor = Colors::Theme::titlebarGreen;
        Renderer::Init();

        const auto end = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
        
        SEDX_CORE_INFO_TAG("EDITOR", "✓ Editor initialization complete ({} ms)", duration);
    }

    Editor::Editor(const PlatformContext& context, const Ref<UserPreferences> &userPreferences) : Application(context), m_UserPreferences(userPreferences)
    {
        arguments = context.GetCommandLineArgs();

        const auto start = std::chrono::high_resolution_clock::now();
        
        SEDX_CORE_INFO_TAG("EDITOR", "=== Initializing Editor with PlatformContext and UserPreferences ===");
        SEDX_CORE_INFO_TAG("EDITOR", "  Working Directory: {}", context.GetWorkingDirectory());
        SEDX_CORE_INFO_TAG("EDITOR", "  Temp Directory: {}", context.GetTempDirectory());
        
        renderContext = RenderContext::Get();

        // TODO: Move project loading to a separate function
        // activeProject->ReadProjCache();
        // assetManager.LoadProject(cacheData.projectPath, cacheData.binPath);
        // m_UserPreferences->GetRecentProjects();
        // scene = assetManager.GetInitialScene();
        // camera = assetManager.GetMainCamera(scene);

        // m_TitleBarActiveColor = m_TitleBarTargetColor = Colors::Theme::titlebarGreen;
        Renderer::Init();

        const auto end = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
        
        SEDX_CORE_INFO_TAG("EDITOR", "✓ Editor initialization complete ({} ms)", duration);
    }

    Editor::~Editor()
    {
        if (ImGui::GetCurrentContext())
        {
            ImGui::Shutdown();
            ImGui_ImplSDL3_Shutdown();
            ImGui::DestroyContext();
        }

        Renderer::Shutdown();
        renderContext.Reset();
    }

    /**
     * @brief Main application loop
     */
    void Editor::Tick()
    {

        // Main loop
        while (!Get().GetWindow().GetShouldClose())
        {
			bool isEditor = true;
            Get().Tick();
            
			// Logic update
            {
                // ImGui
                if (isEditor)
                {
                    ImGui_ImplSDL3_NewFrame();
                    ImGui::NewFrame();
                }

            }

			// Render Update
			if (isEditor)
			{
                ImGui::Render();

			    // Child Windows
                if (ImGui::GetIO().ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
                {
                    ImGui::UpdatePlatformWindows();
                    ImGui::RenderPlatformWindowsDefault();
                }
            }
        }

    };

    void Editor::InitEditor()
    {
        const auto window_flags =
			ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_NoDocking |
			ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse |
			ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove |
			ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus;
    
        // Set window position and size - this keeps the MenuBar in the right place and at the right size
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

        // Begin Dock Space
        if (ImGui::GetIO().ConfigFlags & ImGuiConfigFlags_DockingEnable)
        {
            // Dock Space
            const auto window_id = ImGui::GetID(name);
            if (!ImGui::DockBuilderGetNode(window_id))
            {
                // Reset Current Docking State
                ImGui::DockBuilderRemoveNode(window_id);
                ImGui::DockBuilderAddNode(window_id, ImGuiDockNodeFlags_None);
                ImGui::DockBuilderSetNodeSize(window_id, ImGui::GetMainViewport()->Size);

                // dockBuilderSplitNode(ImGuiID node_id, ImGuiDir split_dir, float size_ratio_for_node_at_dir, ImGuiID* out_id_dir, ImGuiID* out_id_other);
                ImGuiID dock_main_id = window_id;
                ImGuiID dock_right_id = ImGui::DockBuilderSplitNode(dock_main_id, ImGuiDir_Right, 0.17f, nullptr, &dock_main_id);
                ImGuiID dock_right_down_id = ImGui::DockBuilderSplitNode(dock_right_id, ImGuiDir_Down, 0.6f, nullptr, &dock_right_id);
                ImGuiID dock_down_id = ImGui::DockBuilderSplitNode(dock_main_id, ImGuiDir_Down, 0.22f, nullptr, &dock_main_id);
                ImGuiID dock_down_right_id = ImGui::DockBuilderSplitNode(dock_down_id, ImGuiDir_Right, 0.3f, nullptr, &dock_down_id);

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
    
    }

    void Editor::UpdateWindowTitle(const std::string &sceneName)
    {
        const std::string title = std::format("{0} ({1}) - Scenery Editor X {2}", sceneName, Project::GetActive()->GetConfig().name, SEDX_VERSION);
        Application::Get().GetWindow().SetTitle(title);
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

        // Update viewport size from the new size
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

    void Editor::OpenProject()
    {
    }

    void Editor::OpenProject(const std::filesystem::path &filepath)
    {
    }

    void Editor::CreateProject(const std::filesystem::path &projectPath)
    {
    }

    void Editor::EmptyProject()
    {
    }

    void Editor::UpdateCurrentProject()
    {
    }

    void Editor::SaveProject()
    {
    }

    void Editor::CloseProject(bool unloadProject)
    {
    }

    void Editor::NewScene(const std::string &name)
    {
    }

    bool Editor::OpenScene()
    {
        return false;
    }

    bool Editor::OpenScene(const std::filesystem::path &filepath, const bool checkAutoSave)
    {
        return false;
    }

    void Editor::SaveScene()
    {

    }

    void Editor::SaveSceneAuto()
    {

    }

    void Editor::SaveSceneAs()
    {

    }

    /*
    bool Editor::UI_TitleBarHitTest(int x, int y) const
    {
        return false;
    }
    */

    /*
    float Editor::GetSnapValue()
    {
		const auto& editorSettings = EditorSettings::Get();

		switch (m_GizmoType)
		{
			case ImGuizmo::OPERATION::TRANSLATE: return editorSettings.TranslationSnapValue;
			case ImGuizmo::OPERATION::ROTATE: return editorSettings.RotationSnapValue;
			case ImGuizmo::OPERATION::SCALE: return editorSettings.ScaleSnapValue;
		}
		return 0.0f;
    }
    */

    static auto operator<(const ImVec2 &lhs, const ImVec2 &rhs) { return lhs.x < rhs.x && lhs.y < rhs.y; }

    bool Editor::HasArgument(const std::string &argument)
    {
        for (const auto &arg : arguments)
        {
            if (arg == argument)
                return true;
        }

        return false;
    }

}

/// -------------------------------------------------------
