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
#include "child_window.h"
#include "editor_context.h"
#include <functional>
#include <memory>
#include <string>
#include <utility>
#include <vector>
#include <SceneryEditorX/core/events/key_events.h>
#include <SceneryEditorX/core/events/mouse_events.h>
#include <SceneryEditorX/core/layers/layer.h>
#include <SceneryEditorX/renderer/ui/panel_manager.h>
#include <SceneryEditorX/renderer/ui/ui_renderer.h>
#include <SceneryEditorX/renderer/ui/ui_widget.h>
#include <SceneryEditorX/scene/camera.h>
#include <SceneryEditorX/scene/scene.h>
#include <SceneryEditorX/environment/environment_state.h>
#include <SceneryEditorX/settings/user_settings.h>

// ---------------------------------------------------------

namespace SceneryEditorX
{
	class EditorLogSink;

	/**
	 * @class EditorLayer
	 * @brief The editor shell: menu bar, tool strips, status bar and the docked panel layout.
	 *
	 * The layout follows Adobe Substance 3D Painter: tool strip on the far left, PROPERTIES on the left,
	 * the VIEWPORT in the middle with the ASSETS shelf (and CONSOLE) under it, OUTLINER top-right with the
	 * SCENE SETTINGS / CAMERA / RENDERER tabs under it, a panel toggle strip on the far right and a status
	 * bar along the bottom. Window > Reset Layout rebuilds that arrangement.
	 *
	 * Panels are registered with the PanelManager in RegisterPanels(); add new editor features there.
	 */
	class EditorLayer : public Layer, public RefCounted
	{
	public:
		explicit EditorLayer(const Ref<UserPreferences> &userPreferences);
		virtual ~EditorLayer() override;

		void OnAttach() override;
		void OnDetach() override;
		void OnUIRender() override;
		void OnEvent(Event &event) override;

		/* @brief Fonts kept for older panels that push them directly (set once the UI fonts are loaded). */
		inline static ImFont* fontNormal = nullptr;
		inline static ImFont* fontBold   = nullptr;

	private:
		void RegisterPanels();
		void AttachLogSink();
		void DetachLogSink();

		void DrawMenuBar();
		/* @brief The menu bar's menus (name, items): drawn in the bar and in the viewport's Space hotbox. */
		void BuildMainMenus();
		void DrawEditMenu();
		void DrawSceneMenu();
		void DrawXPlaneMenu();
		void DrawFacadeMenu();
		void DrawWindowMenu();
		void DrawViewportMenu();
		void DrawStatusBar();
		void DrawToolStrip();
		void DrawPanelStrip();
		void DrawDockSpace();
		void DrawAboutPopup();
		void DrawDebugWindows();
		void HandleShortcuts();

		void BuildDefaultLayout(ImGuiID dockspaceId);
		void DeleteSelectedEntity();
		void ReadStartupObjects();
		void PlaceStartupObjects();
		/* @brief One --env-* flag with its value into the startup environment; false when the flag isn't one. */
		static bool ParseEnvironmentFlag(const std::string &flag, const std::string &value, Environment::EnvironmentState &state);
		static void LoadEnvironmentTables();
		/* @brief Per frame: the world environment's place, observer altitude and time-dependent surface. */
		void UpdateEnvironment();
		void UpdateWindowTitle() const;
		bool OnKeyPressed(KeyPressedEvent &event);

		/* @brief Menu checkbox / strip toggle for a registered panel's open state. */
		PanelData *FindPanel(const char *panelId) const;

		Ref<UserPreferences> m_UserPreferences;
		Scope<PanelManager> m_PanelManager;
		EditorContext m_Context;
		std::shared_ptr<EditorLogSink> m_LogSink;

		std::vector<std::pair<std::string, std::function<void()>>> m_MainMenus;

		bool m_ShowAbout = false;
		bool m_ShowMetrics = false;
		bool m_ShowStyleEditor = false;
		bool m_ShowDemo = false;
		int m_FocusViewportFrames = 0;	// Frames left to focus the viewport after (re)building the layout

		// X-Plane objects requested on the command line (--xp-object <file.obj>, --xp-asset <virtual path>);
		// --xp-time <hours> and --xp-lights-on set the dataref environment (night lighting), --xp-position x,y,z moves them
		std::vector<std::string> m_StartupObjectFiles;
		std::vector<std::string> m_StartupAssets;
		bool m_HasStartupPosition = false;			// --xp-position x,y,z: where the startup objects go (default: origin)
		float m_StartupPosition[3] = { 0.0f, 0.0f, 0.0f };

		// --entity-bench: Entity handle lookup / copy timing and allocation count once the scene has settled
		bool m_EntityBenchPending = false;
		std::string m_FocusPanel;			// --focus-panel <id>: brought to the front once the layout exists
		int m_FocusPanelFrames = 0;
		size_t m_EntityBenchCount = 0;
		int m_EntityBenchStableFrames = 0;
	};
}

// ---------------------------------------------------------
