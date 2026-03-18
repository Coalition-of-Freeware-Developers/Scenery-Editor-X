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
#include <Editor/ui/ui_widget.h>
#include <SceneryEditorX/core/application/application.h>
#include <SceneryEditorX/core/events/key_events.h>
#include <SceneryEditorX/core/events/mouse_events.h>
#include <SceneryEditorX/settings/user_settings.h>

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
		virtual ~Editor();

		void Run() override;
		void Tick() override;
		void Stop() override;

		void OnRender() override;
		void OnUpdate() override;
		void OnShutdown() override;
		static void InitEditor();

		void OnEvent(Event &event);
		bool OnKeyPressedEvent(KeyPressedEvent& e);
		bool OnMouseButtonPressed(MouseButtonPressedEvent& e);

		static bool HasArgument(const std::string &argument);

		template<typename T>
		T* GetWidget()
		{
			for (const auto& widget : m_Widgets)
			{
				if (T* widget_t = dynamic_cast<T*>(widget.Get()))
				{
					return widget_t;
				}
			}

			return nullptr;
		}

		//void OnCreateMeshFromMeshSource(Entity entity, Ref<MeshSource> meshSource);
		//void SceneHierarchyInvalidMetadataCallback(Entity entity, AssetHandle handle);
		//void SceneHierarchySetEditorCameraTransform(Entity entity);
		
		inline static ImFont* fontNormal = nullptr;
		inline static ImFont* fontBold   = nullptr;

	private:
		static void ProcessClArgs();
		float GetSnapValue();
		static void UpdateWindowTitle(const std::string &sceneName);
		void OnInit() override;

		Ref<UserPreferences> m_UserPreferences;

		std::vector<Ref<Widget>> m_Widgets;
		bool m_ShowStatisticsPanel = false;
		// std::vector<Ref<Viewport>> m_EditorViewports;
		// Ref<::Project::DefaultProject> activeProject;
		// Scope<PanelManager> m_PanelManager;
		// Ref<EditorConsolePanel> m_ConsolePanel;
	};

}

// -------------------------------------------------------
