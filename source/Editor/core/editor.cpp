/**
* -------------------------------------------------------
* Scenery Editor X
* -------------------------------------------------------
* Copyright (c) 2025 Thomas Ray
* Copyright (c) 2025 Coalition of Freeware Developers
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
#include <imgui_impl_glfw.h>
#include <imgui_internal.h>
#include <Editor/settings/editor_settings.h>
#include <SceneryEditorX/core/application/application.h>
#include <SceneryEditorX/core/window/window.h>
#include <SceneryEditorX/platform/settings/settings.h>
#include <SceneryEditorX/project/project.h>
#include <SceneryEditorX/renderer/render_context.h>
#include <SceneryEditorX/ui/ui.h>
#include <SceneryEditorX/ui/ui_context.h>

// ---------------------------------------------------------

namespace SceneryEditorX
{
    // Static renderer instance (declared in editor.h)
    EditorApplication::GraphicsEngine EditorApplication::gfxEngine;
    EditorApplication::~EditorApplication() = default;

    #define MAX_PROJECT_NAME_LENGTH 255
	#define MAX_PROJECT_FILEPATH_LENGTH 512

	static char* s_ProjectNameBuffer = new char[MAX_PROJECT_NAME_LENGTH];
	static char* s_OpenProjectFilePathBuffer = new char[MAX_PROJECT_FILEPATH_LENGTH];
	static char* s_NewProjectFilePathBuffer = new char[MAX_PROJECT_FILEPATH_LENGTH];

	#define SCENE_HIERARCHY_PANEL_ID "SceneHierarchyPanel"
	#define ECS_DEBUG_PANEL_ID "ECSDebugPanel"
	#define CONSOLE_PANEL_ID "EditorConsolePanel"
	#define CONTENT_BROWSER_PANEL_ID "ContentBrowserPanel"
	#define PROJECT_SETTINGS_PANEL_ID "ProjectSettingsPanel"
	#define ASSET_MANAGER_PANEL_ID "AssetManagerPanel"
	#define MATERIALS_PANEL_ID "MaterialsPanel"
	#define APPLICATION_SETTINGS_PANEL_ID "ApplicationSettingsPanel"
	#define SCRIPT_ENGINE_DEBUG_PANEL_ID "ScriptEngineDebugPanel"
	#define SCENE_RENDERER_PANEL_ID "SceneRendererPanel"

    static std::filesystem::path s_ProjectSolutionPath = "";

    // -------------------------------------------------------

	namespace UI
    {
        class UIContextImpl;
    }

    /**
	* -------------------------------------------------------
	* EditorApplication static Variables
	* -------------------------------------------------------
	*/

    // Unique pointer to the application window
    static Scope<Window> g_Window;

    /**
	 * -------------------------------------------------------
	 * FORWARD FUNCTION DECLARATIONS
	 * -------------------------------------------------------
	 */

    //static void initVulkan(GraphicsEngine &gfxEngine);

    // -------------------------------------------------------

    /*
    Editor::~Editor()
    {
        if (ImGui::GetCurrentContext())
        {
            ImGui::Shutdown();
            ImGui_ImplGlfw_Shutdown();
            ImGui::DestroyContext();
        }

        Renderer::Shutdown();
        renderContext.Reset();
    };
    */

    EditorApplication::EditorApplication() = default;

    void EditorApplication::Run()
    {
        //const Ref<Window> &window = GetWindow();
        const auto start = std::chrono::high_resolution_clock::now();

        InitEditor();

        const auto end = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();

        MainLoop();
    }

    void EditorApplication::InitEditor()
    {
        EDITOR_INFO("Scenery Editor X is starting...");
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

    void EditorApplication::Create()
    {

        //Window::SetTitle("Scenery Editor X | " + assetManager.GetProjectName());
        //gfxEngine.CreateInstance(editorWindow);

        //createViewportResources();

        /// Initialize UI components
        ui.InitGUI();

        //camera->extent = {viewportSize.x, viewportSize.y};
    }

    void EditorApplication::MainLoop()
    {
        Renderer::GetRenderData();
        while (!Application::Get().GetWindow().GetShouldClose())
		{
			Application::Get().OnUpdate();
			// Update editor application state
			Update();
			// Render a frame
			DrawFrame();
		}

    }

    void EditorApplication::Update()
    {
        // Update the viewport size if it has changed
		/*
        if (viewportData.viewportResized)
            return;
        // Update viewport size from the new size
        viewportData.SetViewportSize(newViewportSize.GetViewportSize());
        viewportData.viewportResized = false;
        */
    }
    void EditorApplication::DrawFrame()
    {
    }

    void EditorApplication::UpdateWindowTitle(const std::string &sceneName)
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
    void EditorApplication::RecreateFrameResources()
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

    void Editor::OnAttach()
    {

		/*
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
		*/

		/*
		m_PanelManager->AddPanel<ProjectSettingsWindow>(PanelCategory::Edit, PROJECT_SETTINGS_PANEL_ID, "Project Settings", false);
		m_PanelManager->AddPanel<ApplicationSettingsPanel>(PanelCategory::Edit, APPLICATION_SETTINGS_PANEL_ID, "Application Settings", false);
		m_PanelManager->AddPanel<ECSDebugPanel>(PanelCategory::View, ECS_DEBUG_PANEL_ID, "ECS Debug", false, m_EditorScene);
		m_ConsolePanel = m_PanelManager->AddPanel<EditorConsolePanel>(PanelCategory::View, CONSOLE_PANEL_ID, "Log", true);
		m_PanelManager->AddPanel<MaterialsPanel>(PanelCategory::View, MATERIALS_PANEL_ID, "Materials", true);
		m_PanelManager->AddPanel<AssetManagerPanel>(PanelCategory::View, ASSET_MANAGER_PANEL_ID, "Asset Manager", false);

		Ref<SceneRendererPanel> sceneRendererPanel = m_PanelManager->AddPanel<SceneRendererPanel>(PanelCategory::View, SCENE_RENDERER_PANEL_ID, "Scene Renderer", true);
		*/

		/*
		if (!m_UserPreferences->StartupProject.empty())
			OpenProject(m_UserPreferences->StartupProject);
		else
			SEDX_CORE_VERIFY(false, "No project provided!");
		*/

		/*
		if (!Project::GetActive())
			EmptyProject();
			*/

		// AssetManager::UnloadAllAssetPacks();
		// AssetManager::AddAssetPack(assetPack);

		/*
		for (Ref<Viewport>& viewport : m_EditorViewports)
		{
			viewport->Init(m_CurrentScene);
		}
		*/

		// TODO: do we need to do this for all viewports? when we set new main viewport
		// sceneRendererPanel->SetContext(GetMainViewport()->GetRenderer());

		// AssetEditorPanel::RegisterDefaultEditors();

		/*
		if (m_UserPreferences->ShowWelcomeScreen)
			UI_ShowWelcomePopup();
		*/

    }

    void Editor::OnUpdate(DeltaTime dt)
    {
        Module::OnUpdate(dt);
    }

    void Editor::OnUIRender()
    {
        Module::OnUIRender();
    }

    void Editor::OnEvent(Event &event)
    {
        Module::OnEvent(event);
    }

    bool Editor::OnKeyPressedEvent(KeyPressedEvent &e)
    {
        return false;
    }

    bool Editor::OnMouseButtonPressed(MouseButtonPressedEvent &e)
    {
        return false;
    }

    void EditorApplication::OpenProject()
    {
    }

    void EditorApplication::OpenProject(const std::filesystem::path &filepath)
    {
    }

    void EditorApplication::CreateProject(const std::filesystem::path &projectPath)
    {
    }

    void EditorApplication::EmptyProject()
    {
    }

    void EditorApplication::UpdateCurrentProject()
    {
    }

    void EditorApplication::SaveProject()
    {
    }

    void EditorApplication::CloseProject(bool unloadProject)
    {
    }

    void EditorApplication::NewScene(const std::string &name)
    {
    }

    bool EditorApplication::OpenScene()
    {
        return false;
    }

    bool EditorApplication::OpenScene(const std::filesystem::path &filepath, const bool checkAutoSave)
    {
        return false;
    }

    void EditorApplication::SaveScene()
    {

    }

    void EditorApplication::SaveSceneAuto()
    {

    }

    void EditorApplication::SaveSceneAs()
    {

    }

    void Editor::UI_DrawMenubar()
    {
    }

    float Editor::UI_DrawTitlebar()
    {
        return 0;
    }

    void Editor::UI_HandleManualWindowResize()
    {
    }

    bool Editor::UI_TitleBarHitTest(int x, int y) const
    {
        return false;
    }

    void Editor::UI_ShowNewProjectPopup()
    {
    }

    void Editor::UI_ShowLoadAutoSavePopup()
    {
    }

    void Editor::UI_ShowCreateAssetsFromMeshSourcePopup()
    {
    }

    void Editor::UI_ShowInvalidAssetMetadataPopup()
    {
    }

    void Editor::UI_ShowNoMeshPopup()
    {
    }

    void Editor::UI_ShowNoSkeletonPopup()
    {
    }

    void Editor::UI_ShowNoAnimationPopup()
    {
    }

    void Editor::UI_ShowNewScenePopup()
    {
    }

    void Editor::UI_ShowWelcomePopup()
    {
    }

    void Editor::UI_ShowAboutPopup()
    {
    }

    void Editor::UI_BuildAssetPackDialog()
    {
    }

    /*
    Ref<Viewport> Editor::GetMainViewport()
    {
        return {};
    }
    */

    /*
    void Editor::SetMainViewport(const std::string &viewportName)
    {
    }
    */

    /*
    void Editor::UI_StatisticsPanel()
    {
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

    /*
    void Editor::DeleteEntity(Entity entity)
    {
    }
    */

    /*
    void Editor::QueueSceneTransition(AssetHandle scene)
    {
    }
    */

    /*
    void Editor::BuildProjectData()
    {
    }
    */

    /*
    void Editor::BuildShaderPack()
    {
    }
    */

    /*
    void Editor::BuildSoundBank()
    {
    }
    */

    /*
    void Editor::BuildAssetPack()
    {
    }
    */

    /*
    void Editor::BuildAll()
    {
    }
    */

    /*
    void Editor::RegenerateProjectScriptSolution(const std::filesystem::path &projectPath)
    {
    }
    */

    /*
    void Editor::ReloadCSharp()
    {
    }
    */

    /*
    void Editor::FocusLogPanel()
    {
    }
    */

    static auto operator<(const ImVec2 &lhs, const ImVec2 &rhs)
    {
        return lhs.x < rhs.x && lhs.y < rhs.y;
    }

    /// -------------------------------------------------------


}

/// -------------------------------------------------------
