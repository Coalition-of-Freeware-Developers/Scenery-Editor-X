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
 * editor_layer.h
 * -------------------------------------------------------
 * Created: 01/03/2026
 * -------------------------------------------------------
 */
#pragma once
#include <Editor/ui/ui_widget.h>
#include <SceneryEditorX/core/events/key_events.h>
#include <SceneryEditorX/core/events/mouse_events.h>
#include <SceneryEditorX/core/layers/layer.h>
#include <SceneryEditorX/scene/camera.h>
#include <SceneryEditorX/scene/scene.h>
#include <SceneryEditorX/settings/user_settings.h>

// ---------------------------------------------------------

namespace SceneryEditorX
{
	class EditorLayer : public Layer, public RefCounted
	{
	public:
		EditorLayer(const Ref<UserPreferences>& userPreferences);
		virtual ~EditorLayer() override;

		void OnAttach() override;
		void OnDetach() override;
	    void Tick() override;
		void InitEditor();

		void OnRender() override;
		//void OnUIRender();
		void OnEvent(Event &event) override;
		bool OnKeyPressedEvent(KeyPressedEvent& e);
		bool OnMouseButtonPressed(MouseButtonPressedEvent& e);
		
		//static bool HasArgument(const std::string &argument);

		Ref<EditorLayer> Get() { return {this}; }

		void OpenProject();
		void OpenProject(const std::filesystem::path &filepath);
		void CreateProject(const std::filesystem::path &projectPath);
		void EmptyProject();
		void UpdateCurrentProject();
		static void SaveProject();
		static void CloseProject(bool unloadProject = true);

		void NewScene(const std::string &name = "UntitledAirport");
		bool OpenScene();
		bool OpenScene(const std::filesystem::path &filepath, bool checkAutoSave = true);
		void SaveScene();
		void SaveSceneAuto();
		void SaveSceneAs();

		static bool IsSceneOpen();
		static bool IsProjectOpen();

		/**
		 * @brief Retrieves a pointer to the first widget of type T in the editor's widget list. Returns nullptr if no such widget is found.
		 * @tparam T The type of the widget to retrieve.
		 * @return A pointer to the first widget of type T, or nullptr if no such widget is found.
		 */
		template<typename T>
		T* GetWidget()
		{
			for (const auto& widget : m_Widgets)
			{
				if (T* widgetT = dynamic_cast<T*>(widget.Get()))
				{
					return widgetT;
				}
			}

			return nullptr;
		}

	private:
		Ref<Scene> m_EditorScene;
		Ref<Scene> m_CurrentScene;
		Ref<Scene> m_RuntimeScene;
		std::vector<Ref<Widget>> m_Widgets;

		Vec2 m_ViewportBounds[2];
		Vec2 m_SecondViewportBounds[2];
		std::pair<float, float> GetMouseViewportSpace(bool primaryViewport);

		float GetSnapValue();
		float UI_DrawTitlebar();

		void UI_DrawMenubar();
		void UI_HandleManualWindowResize();
		bool UI_TitleBarHitTest(int x, int y) const;
		static void UpdateWindowTitle(const std::string &sceneName);

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
		Ref<Viewport> GetMainViewport();
		void SetMainViewport(const std::string &viewportName);
		
		bool m_ViewportPanelMouseOver		= false;
		bool m_ViewportPanelFocused			= false;
		bool m_AllowViewportCameraEvents	= false;
		bool m_ViewportPanel2MouseOver		= false;
		bool m_ViewportPanel2Focused		= false;
		bool m_ShowSecondViewport			= false;

		void DeleteEntity(Entity entity);
		void BuildProjectData();
		void BuildShaderPack();
		void BuildSoundBank();
		void BuildAssetPack();
		void BuildAll();

		Camera m_Camera;
		Ref<UserPreferences> m_UserPreferences;
		std::string m_SceneFilePath;

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
		bool m_ShowStatisticsPanel = false;

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

// ---------------------------------------------------------
