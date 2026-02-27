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
 * editor.h
 * -------------------------------------------------------
 * Created: 13/4/2025
 * -------------------------------------------------------
 */
#pragma once
#include "Editor/projects/project.h"
#include "Editor/ui/panels/ui_panel.h"
#include <Editor/core/viewport.h>
#include <SceneryEditorX/core/application/application.h>
#include <SceneryEditorX/core/events/key_events.h>
#include <SceneryEditorX/core/events/mouse_events.h>
#include <SceneryEditorX/core/platform/settings/user_settings.h>
#include <SceneryEditorX/core/window/window.h>
#include <SceneryEditorX/renderer/vulkan/render_context.h>
#include <SceneryEditorX/renderer/vulkan/renderer.h>
#include <SceneryEditorX/ui/ui.h>
#include <SceneryEditorX/ui/ui_context.h>

// ---------------------------------------------------------

namespace SceneryEditorX
{
    #define MAX_PROJECT_NAME_LENGTH 255
    #define MAX_PROJECT_FILEPATH_LENGTH 512

    // ---------------------------------------------------------

    /**
	 * @class Editor
	 * @brief Main application class for the Scenery Editor X.
	 *
	 * The Editor class serves as the central controller for the editor,
	 * managing the lifecycle of the application and coordinating between the graphics engine,
	 * UI system, asset management, and viewport rendering. It handles initialization,
	 * main loop execution, frame rendering, and resource management.
	 */
    class Editor : public Application
    {
    public:
        explicit Editor(const PlatformContext& context);
        explicit Editor(const PlatformContext& context, const Ref<UserPreferences> &userPreferences);
        virtual ~Editor() override;

        void Run() override;
        void Tick() override;
        void Stop() override;

        void OnRender() override;
        void OnUpdate() override;
        void OnShutdown() override;
        void InitEditor();

        void OnEvent(Event &event);
        bool OnKeyPressedEvent(KeyPressedEvent& e);
		bool OnMouseButtonPressed(MouseButtonPressedEvent& e);

        static bool HasArgument(const std::string &argument);

        //void OnCreateMeshFromMeshSource(Entity entity, Ref<MeshSource> meshSource);
        //void SceneHierarchyInvalidMetadataCallback(Entity entity, AssetHandle handle);
        //void SceneHierarchySetEditorCameraTransform(Entity entity);

        void OpenProject();
        void OpenProject(const std::filesystem::path &filepath);
        void CreateProject(const std::filesystem::path &projectPath);
        void EmptyProject();
        void UpdateCurrentProject();
        void SaveProject();
        void CloseProject(bool unloadProject = true);

        void NewScene(const std::string &name = "UntitledAirport");
        bool OpenScene();
        bool OpenScene(const std::filesystem::path &filepath, bool checkAutoSave = true);
        void SaveScene();
        void SaveSceneAuto();
        void SaveSceneAs();

    private:
        static void ProcessClArgs();
        static void UpdateWindowTitle(const std::string &sceneName);
        void OnInit() override;

        Ref<UserPreferences> m_UserPreferences;
        Ref<RenderContext> renderContext;

        static Renderer s_GfxEngine; // Graphics engine instance
		std::vector<Ref<UI_Panel>> m_Panels;
        bool m_ShowStatisticsPanel = false;
        // std::vector<Ref<Viewport>> m_EditorViewports;
        // Ref<::Project::DefaultProject> activeProject;
        // Scope<PanelManager> m_PanelManager;
        // Ref<EditorConsolePanel> m_ConsolePanel;

        // ---------------------------------------------------------

        float GetSnapValue();
        float UI_DrawTitlebar();

        void UI_DrawMenubar();
        void UI_HandleManualWindowResize();
        bool UI_TitleBarHitTest(int x, int y) const;

        // Popups
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
        void UI_StatisticsPanel();
        void UI_BuildAssetPackDialog();

        // Viewports
        /*
        Ref<Viewport> GetMainViewport();
        void SetMainViewport(const std::string &viewportName);
        */

        //void DeleteEntity(Entity entity);
        //void QueueSceneTransition(AssetHandle scene);

        void BuildProjectData();
        void BuildShaderPack();
        void BuildSoundBank();
        void BuildAssetPack();
        void BuildAll();
        void RegenerateProjectScriptSolution(const std::filesystem::path &projectPath);
        void ReloadCSharp();
        void FocusLogPanel();

        uint32_t m_TitleBarTargetColor;
        uint32_t m_TitleBarActiveColor;
        uint32_t m_TitleBarPreviousColor;

        int m_GizmoType = -1; // -1 = no gizmo

        // ImGui Tools
        bool m_ShowMetricsTool = false;
        bool m_ShowStackTool = false;
        bool m_ShowStyleEditor = false;
        bool m_ProjectUpdateNeeded = false;
        bool m_ShowProjectUpdatedPopup = false;
        bool m_GizmoWorldOrientation = true;
        bool m_TitleBarHovered = false;
        bool m_AnimateTitleBarColor = true;

        struct LoadAutoSavePopupData
        {
            std::string m_FilePath;
            std::string m_FilePathAuto;
        } m_LoadAutoSavePopupData;

        /* 
         * Time (in seconds) since scene was last saved.
         * Counts up only when scene is in Edit mode.
         * If exceeds 300s then scene is automatically saved
         */
        float m_TimeSinceLastSave = 0.0f;
        float m_RequiredProjectVersion = 0.0f;
        float m_AssetUpdatePerf = 0.0f;

    #ifdef SEDX_PLATFORM_WINDOWS
		typedef std::wstring WatcherString;
    #else
		typedef std::string WatcherString;
    #endif
		//std::unique_ptr<filewatch::FileWatch<WatcherString>> m_ScriptFileWatcher = nullptr;
    };

}

// -------------------------------------------------------
