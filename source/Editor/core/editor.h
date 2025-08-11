/**
* -------------------------------------------------------
* Scenery Editor X
* -------------------------------------------------------
* Copyright (c) 2025 Thomas Ray
* Copyright (c) 2025 Coalition of Freeware Developers
* -------------------------------------------------------
* editor.h
* -------------------------------------------------------
* Created: 13/4/2025
* -------------------------------------------------------
*/
#pragma once
#include <SceneryEditorX/renderer/vulkan/vk_data.h>
#include <SceneryEditorX/renderer/render_context.h>
#include <SceneryEditorX/core/window/window.h>
#include <SceneryEditorX/renderer/renderer.h>
#include <SceneryEditorX/ui/ui.h>
#include <SceneryEditorX/ui/ui_context.h>
#include <SceneryEditorX/core/events/key_events.h>
#include <SceneryEditorX/core/events/mouse_events.h>
#include <SceneryEditorX/platform/settings/user_settings.h>
#include <Editor/core/viewport.h>

/// ---------------------------------------------------------

namespace SceneryEditorX
{
    #define MAX_PROJECT_NAME_LENGTH 255
    #define MAX_PROJECT_FILEPATH_LENGTH 512

    /// ---------------------------------------------------------

    /**
	 * @class EditorApplication
	 * @brief Main application class for the Scenery Editor X.
	 *
	 * The EditorApplication class serves as the central controller for the editor,
	 * managing the lifecycle of the application and coordinating between the graphics engine,
	 * UI system, asset management, and viewport rendering. It handles initialization,
	 * main loop execution, frame rendering, and resource management.
	 */

    class Editor : public Module
    {
    public:
        Editor(const Ref<UserPreferences> &userPreferences);
        virtual ~Editor() override;
        virtual void OnAttach() override;
        virtual void OnDetach() override;
        virtual void OnUpdate(DeltaTime dt) override;

        virtual void OnUIRender() override;
        virtual void OnEvent(Event &event) override;

        bool OnKeyPressedEvent(KeyPressedEvent& e);
		bool OnMouseButtonPressed(MouseButtonPressedEvent& e);

		void OpenProject();
		void OpenProject(const std::filesystem::path& filepath);

        void CreateProject(const std::filesystem::path &projectPath);
		void EmptyProject();
		void UpdateCurrentProject();
		void SaveProject();
		void CloseProject(bool unloadProject = true);
		void NewScene(const std::string& name = "UntitledAirport");
		bool OpenScene();
		bool OpenScene(const std::filesystem::path& filepath, const bool checkAutoSave = true);
		void SaveScene();
		void SaveSceneAuto();
		void SaveSceneAs();

        //void OnCreateMeshFromMeshSource(Entity entity, Ref<MeshSource> meshSource);
        //void SceneHierarchyInvalidMetadataCallback(Entity entity, AssetHandle handle);
        //void SceneHierarchySetEditorCameraTransform(Entity entity);
    private:
        //friend class Viewport;

        void UpdateWindowTitle(const std::string &sceneName);
        void UI_DrawMenubar();
        /// Returns titlebar height
        float UI_DrawTitlebar();
        void UI_HandleManualWindowResize();
        bool UI_TitleBarHitTest(int x, int y) const;

        /// Popups
        void UI_ShowNewProjectPopup();
        void UI_ShowLoadAutoSavePopup();
        void UI_ShowCreateAssetsFromMeshSourcePopup();
        void UI_ShowInvalidAssetMetadataPopup();
        void UI_ShowNoMeshPopup();
        void UI_ShowNoSkeletonPopup();
        void UI_ShowNoAnimationPopup();
        void UI_ShowNewScenePopup();
        void UI_ShowWelcomePopup();
        void UI_ShowAboutPopup();

        void UI_BuildAssetPackDialog();

        /// Viewports
        Ref<Viewport> GetMainViewport();
        void SetMainViewport(const std::string &viewportName);

        /// Statistics Panel Rendering
        void UI_StatisticsPanel();

        float GetSnapValue();

        //void DeleteEntity(Entity entity);

        void QueueSceneTransition(AssetHandle scene);

        void BuildProjectData();
        void BuildShaderPack();
        void BuildSoundBank();
        void BuildAssetPack();
        void BuildAll();
        void RegenerateProjectScriptSolution(const std::filesystem::path &projectPath);
        void ReloadCSharp();
        void FocusLogPanel();

        Ref<UserPreferences> m_UserPreferences;

        //Scope<PanelManager> m_PanelManager;
        //Ref<EditorConsolePanel> m_ConsolePanel;
        bool m_ShowStatisticsPanel = false;

        std::vector<Ref<Viewport>> m_EditorViewports;

        Ref<Scene> m_RuntimeScene, m_EditorScene, m_SimulationScene, m_CurrentScene;
        std::string m_SceneFilePath;

        float m_AssetUpdatePerf = 0.0f;

        bool m_TitleBarHovered = false;
        uint32_t m_TitleBarTargetColor;
        uint32_t m_TitleBarActiveColor;
        uint32_t m_TitleBarPreviousColor;
        bool m_AnimateTitleBarColor = true;

        int m_GizmoType = -1; /// -1 = no gizmo
        bool m_GizmoWorldOrientation = true;

        /// ImGui Tools
        bool m_ShowMetricsTool = false;
        bool m_ShowStackTool = false;
        bool m_ShowStyleEditor = false;

        bool m_EditorCameraInRuntime = false;

        std::atomic_bool m_ShouldReloadCSharp = false;

        struct LoadAutoSavePopupData
        {
            std::string FilePath;
            std::string FilePathAuto;
        } m_LoadAutoSavePopupData;
		
		float m_TimeSinceLastSave = 0.0f; /// time (in seconds) since scene was last saved.  Counts up only when scene is in Edit mode. If exceeds 300s then scene is automatically saved

		float m_RequiredProjectVersion = 0.0f;
		bool m_ProjectUpdateNeeded = false;
		bool m_ShowProjectUpdatedPopup = false;

		std::thread m_AssetPackThread;
		//std::future<Ref<AssetPack>> m_AssetPackFuture;
		std::atomic<float> m_AssetPackBuildProgress = 0.0f;
		std::string m_AssetPackBuildMessage;
		bool m_BuildAllInProgress = false;
		bool m_AssetPackBuiltOK = false;
		
#ifdef SEDX_PLATFORM_WINDOWS
		using WatcherString = std::wstring;
#else
		using WatcherString = std::string;
#endif
		//std::unique_ptr<filewatch::FileWatch<WatcherString>> m_ScriptFileWatcher = nullptr;
    };


	class EditorApplication
	{
	public:
        EditorApplication();
        virtual ~EditorApplication();

        /**
         * @brief Initializes the editor components.
         *
         * Sets up the graphics engine, UI system, and other core components
         * required for the editor to function properly.
         */
        void InitEditor();

        /**
         * @brief Starts the editor application.
         *
         * Launches the main application loop and begins processing events.
         */
		void Run();

        /**
         * @brief Updates the editor state.
         *
         * Called each frame to update UI, process input, and manage the editor state.
         */
        void Update();

        /**
         * @brief Renders a single frame.
         *
         * Coordinates the rendering of UI and scene elements for the current frame.
         */
        void DrawFrame();

        /**
         * @brief Creates necessary resources for the editor.
         *
         * Initializes graphics resources, viewport, and other components
         * needed for editor operation.
         */
        void Create();

        /**
         * @brief Contains the main application loop.
         *
         * Manages the continuous execution of the update and render cycle.
         */
        void MainLoop();

        /**
         * @brief Gets the application window instance.
         *
         * @return A reference to the Window object managed by the graphics engine.
         */
        /*
        Ref<Window> GetWindow()
        {
            return GraphicsEngine::GetWindowContext();
        }
        */


	private:
	    void UpdateWindowTitle(const std::string& sceneName);

        using GraphicsEngine = Renderer;
        /**
         * @brief Graphics engine instance for rendering.
         *
         * Manages Vulkan resources, rendering operations, and the window surface.
         */
        GLOBAL GraphicsEngine gfxEngine;

        /**
         * @brief Command buffer for rendering operations.
         */
        //Ref<CommandBuffer> cmdBuffer;

        /**
         * @brief Reference to the swap chain for rendering.
         *
         * Manages the presentation of rendered frames to the display.
         */
        //Ref<SwapChain> vkSwapChain;

        //AssetManager assetManager;

        /**
         * @brief UI system for the editor interface.
         *
         * Handles drawing and interaction with the editor user interface.
         */
        UI::GUI ui;

	    /**
         * @brief Context for UI rendering and interaction.
         *
         * Provides state and resources needed for UI operations.
         */
        Ref<UI::UIContext> uiContext;

        /**
         * @brief Viewport configuration and state.
         *
         * Contains settings related to the editor's main viewport.
         */
        Viewport viewportData;

        /**
         * @brief Core rendering configuration and state.
         *
         * Holds settings and state information used throughout the rendering pipeline.
         */
        RenderData renderData;

        /**
         * @brief Vulkan device features enabled for the application.
         *
         * Specifies which Vulkan hardware features are used by the editor.
         */
        VulkanDeviceFeatures vkDeviceFeatures;

        /**
         * @brief Index of the current frame being rendered.
         *
         * Used to track frame-specific resources in the rendering cycle.
         */
        uint32_t currentFrame = 0;

		/// ---------------------------------------------------------

        /**
         * @brief New size for viewport when resizing occurs.
         *
         * Stores the target size for viewport recreation when dimensions change.
         */
        //Viewport newViewportSize = viewportData.GetViewportBounds();

	    /**
	     * @brief Counter for total frames rendered since application start.
	     */
	    uint32_t frameCount = 0;

        /**
         * @brief Handle to the logical Vulkan device.
         *
         * Direct access to the Vulkan device for resource management operations.
         */
	    VkDevice device = VK_NULL_HANDLE;

        /**
         * @brief Creates resources needed for viewport rendering.
         *
         * Allocates and initializes framebuffers, render targets, and other
         * resources required for rendering to the viewport.
         */
        void CreateViewportResources();

        /**
         * @brief Cleans up viewport rendering resources.
         *
         * Releases framebuffers, render targets, and other resources
         * associated with viewport rendering.
         */
        void CleanupViewportResources();

        /**
         * @brief Handles surface resize events.
         *
         * Called when the window or rendering surface changes size to update
         * viewport dimensions and recreate rendering resources as needed.
         *
         * @param width New surface width in pixels
         * @param height New surface height in pixels
         */
        void OnSurfaceUpdate(uint32_t width, uint32_t height);

        /**
         * @brief Recreates frame-related resources.
         *
         * Rebuilds swap chain images, framebuffers, and other resources
         * needed for rendering when the rendering context changes.
         */
        //void RecreateFrameResources();
    };

}

/// -------------------------------------------------------
