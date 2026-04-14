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
 * ui_layer.cpp
 * -------------------------------------------------------
 * Created: 09/04/2026
 * -------------------------------------------------------
 */
#include "ui_layer.h"
#include "ui_impl.h"
#include "ui_widget.h"
#include "SceneryEditorX/logging/asserts.h"
#include "SceneryEditorX/renderer/font/font_awesome.h"
#include "resources/fonts.h"
#include "source/imgui/imgui.h"
#include "source/imgui/imgui_internal.h"
#include "source/imgui/backends/imgui_impl_sdl3.h"
#include "source/imguizmo/ImGuizmo.h"
#include <Editor/panels/menu_bar.h>
#include <SceneryEditorX/core/application/application.h>
#include <SceneryEditorX/core/input/input.h>
#include <SceneryEditorX/core/resource/resource_cache.h>
#include <SceneryEditorX/core/window/window.h>

// -------------------------------------------------------

namespace SceneryEditorX
{

#pragma region Static Variables

	static float s_FontSize  = 18.0f;
	static float s_FontScale = 1.0f;

#pragma endregion

	/**
	 * @brief Initializes the ImGui platform interface by setting function pointers for window management and rendering. 
	 */
	static void InitPlatformInterface()
	{
		ImGuiPlatformIO &platformIo = ImGui::GetPlatformIO();
		if (ImGui::GetIO().ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
		{
			IM_ASSERT(platformIo.Platform_CreateVkSurface != NULL && "Platform needs to setup the CreateVkSurface handler.");
		}

		platformIo.Renderer_CreateWindow	= UI::WindowCreate;
		platformIo.Renderer_DestroyWindow	= UI::WindowDestroy;
		platformIo.Renderer_SetWindowSize	= UI::WindowResize;
		platformIo.Renderer_RenderWindow	= UI::WindowRender;
		platformIo.Renderer_SwapBuffers		= UI::WindowPresent;
	}

	/**
	 * @brief Compares two ImVec2 objects for ordering.
	 * @param lhs The left-hand side ImVec2.
	 * @param rhs The right-hand side ImVec2.
	 * @return True if lhs is less than rhs, false otherwise.
	 */
	static auto operator<(const ImVec2 &lhs, const ImVec2 &rhs)
	{
		return lhs.x < rhs.x && lhs.y < rhs.y;
	}

	/**
	 * @brief Begins the main ImGui window that serves as the root for the docking layout. 
	 * This window is borderless, non-movable, and fills the entire viewport work area. 
	 * It also sets up the docking space for the editor panels.
	 */
	static void BeginWindow()
	{
		// note: don't use ImGuiWindowFlags_MenuBar here since we use BeginMainMenuBar() separately
		const auto windowFlags =
			ImGuiWindowFlags_NoDocking             |
			ImGuiWindowFlags_NoTitleBar            |
			ImGuiWindowFlags_NoCollapse            |
			ImGuiWindowFlags_NoResize              |
			ImGuiWindowFlags_NoMove                |
			ImGuiWindowFlags_NoBringToFrontOnFocus |
			ImGuiWindowFlags_NoNavFocus;
	
		// set window position and size to the work area (excludes main menu bar)
		const ImGuiViewport* viewport = ImGui::GetMainViewport();
		ImGui::SetNextWindowPos(viewport->WorkPos);
		ImGui::SetNextWindowSize(viewport->WorkSize);
	
		// draw window border for borderless window (use full viewport for border around entire window)
		{
			ImDrawList* drawList = ImGui::GetForegroundDrawList();
			ImVec2 min = viewport->Pos;
			ImVec2 max = ImVec2(viewport->Pos.x + viewport->Size.x, viewport->Pos.y + viewport->Size.y);
			ImU32 borderColor = IM_COL32(40, 40, 42, 255);
			drawList->AddRect(min, max, borderColor, 0.0f, 0, 1.0f);
		}
	
		// set window style
		ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding,   0.0f);
		ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding,    ImVec2(0.0f, 0.0f));
	
		// begin window
		const char* name = "##main_window";
		bool open = true;
		ImGui::Begin(name, &open, windowFlags);
		ImGui::PopStyleVar(3);
	
		// begin dock space
		if (ImGui::GetIO().ConfigFlags & ImGuiConfigFlags_DockingEnable)
		{
			// dock space
			const auto window_id = ImGui::GetID(name);
			if (!ImGui::DockBuilderGetNode(window_id))
			{
				// reset current docking state
				ImGui::DockBuilderRemoveNode(window_id);
				ImGui::DockBuilderAddNode(window_id, ImGuiDockNodeFlags_None);
				ImGui::DockBuilderSetNodeSize(window_id, ImGui::GetMainViewport()->Size);
	
				// dockBuilderSplitNode(ImGuiID node_id, ImGuiDir split_dir, float size_ratio_for_node_at_dir, ImGuiID* out_id_dir, ImGuiID* out_id_other);
				ImGuiID dock_main_id       = window_id;
				ImGuiID dock_right_id      = ImGui::DockBuilderSplitNode(dock_main_id,  ImGuiDir_Right, 0.17f, nullptr, &dock_main_id);
				ImGuiID dock_right_down_id = ImGui::DockBuilderSplitNode(dock_right_id, ImGuiDir_Down,  0.6f,  nullptr, &dock_right_id);
				ImGuiID dock_down_id       = ImGui::DockBuilderSplitNode(dock_main_id,  ImGuiDir_Down,  0.22f, nullptr, &dock_main_id);
				ImGuiID dock_down_right_id = ImGui::DockBuilderSplitNode(dock_down_id,  ImGuiDir_Right, 0.3f,  nullptr, &dock_down_id);
	
				// dock windows
				ImGui::DockBuilderDockWindow("RenderOptions",   dock_right_id);
				ImGui::DockBuilderDockWindow("Properties",		dock_right_down_id);
				ImGui::DockBuilderDockWindow("Console",			dock_down_id);
				ImGui::DockBuilderDockWindow("Assets",			dock_down_right_id);
				ImGui::DockBuilderDockWindow("Viewport",		dock_main_id);
	
				ImGui::DockBuilderFinish(dock_main_id);
			}
	
			ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 0.0f);
			ImGui::DockSpace(window_id, ImVec2(0.0f, 0.0f), ImGuiDockNodeFlags_PassthruCentralNode);
			ImGui::PopStyleVar();
		}
	}
	
	void UILayer::OnAttach()
	{
		Window window = Application::Get().GetWindow();
		std::filesystem::path appdata = IO::FileSystem::GetPersistentStoragePath();

		IMGUI_CHECKVERSION();
		ImGui::CreateContext();

		// setup back-end capabilities flags
		ImGuiIO &io = ImGui::GetIO();
		io.ConfigFlags  |= ImGuiConfigFlags_NavEnableKeyboard;      // Enable Keyboard Controls
		io.ConfigFlags  |= ImGuiConfigFlags_DockingEnable;          // Enable Docking
		io.ConfigFlags  |= ImGuiConfigFlags_ViewportsEnable;		// Enable Multi-Viewport / Platform Windows
		io.ConfigFlags  |= ImGuiConfigFlags_NoMouseCursorChange;	// Cursor control is given to ImGui, but dynamically, from the engine
		io.BackendFlags |= ImGuiBackendFlags_RendererHasViewports;
		io.BackendFlags |= ImGuiBackendFlags_RendererHasVtxOffset;
		io.BackendRendererName = "UI Renderer";
		io.ConfigWindowsResizeFromEdges = true;						// Enable resizing windows from edges

		// Store the ini path in a member so io.IniFilename points to a stable string (C26815).
		m_IniFilePath  = (appdata / "editor.ini").string();
		io.IniFilename = m_IniFilePath.c_str();

		// Configure Fonts
		{
			// font_bold configuration
			ImFontConfig config; // config for bold font (mainly for use in headers)
			config.GlyphOffset.y = -2.0f;

			const std::string dir_fonts = ResourceCache::GetResourceDirectory(ResourceDirectory::Fonts) + "/";

			UI::FontConfiguration opensansLight;
			opensansLight.FontName = "Light";
			opensansLight.FilePath = dir_fonts + "opensans/OpenSans-Light.ttf";
			opensansLight.Size = 15.0f;
			UI::Fonts::Add(opensansLight, true);

			UI::FontConfiguration opensansDefault;
			opensansDefault.FontName = "Default";
			opensansDefault.FilePath = dir_fonts + "opensans/OpenSans-Regular.ttf";
			opensansDefault.Size = 15.0f;
			UI::Fonts::Add(opensansDefault, true);

			UI::FontConfiguration opensansMedium;
			opensansMedium.FontName = "Medium";
			opensansMedium.FilePath = dir_fonts + "opensans/OpenSans-Medium.ttf";
			opensansMedium.Size = 18.0f;
			UI::Fonts::Add(opensansMedium, true);

			UI::FontConfiguration opensansBold;
			opensansBold.FontName = "Bold";
			opensansBold.FilePath = dir_fonts + "opensans/OpenSans-Bold.ttf";
			opensansBold.Size = 18.0f;
			UI::Fonts::Add(opensansBold, true);

			/*
			m_Font_Normal = io.Fonts->AddFontFromFileTTF((dir_fonts + "opensans/OpenSans-Medium.ttf").c_str(),
														 s_FontSize * window.GetDpiScale());
			m_Font_Bold = io.Fonts->AddFontFromFileTTF((dir_fonts + "opensans/OpenSans-Bold.ttf").c_str(),
													   s_FontSize * window.GetDpiScale(),
													   &config);*/
			//io.FontGlobalScale = s_FontScale;

			static const ImWchar s_FontAwesomeRanges[] = {SEDX_ICON_MIN, SEDX_ICON_MAX, 0};
			UI::FontConfiguration fontAwesome;
			fontAwesome.FontName = "FontAwesome";
			fontAwesome.FilePath = dir_fonts + "fontawesome-webfont.ttf";
			fontAwesome.Size = 16.0f;
			fontAwesome.GlyphRanges = s_FontAwesomeRanges;
			fontAwesome.MergeWithLast = true;
			UI::Fonts::Add(fontAwesome);
		}

		/**
		 * Ensure ImGui has a valid initial DisplaySize (some backends update this per-frame).
		 * Guard against cases where backend didn't set it yet by using the window size.
		 */
		io.DisplaySize = ImVec2(static_cast<float>(Window::GetWidth()), static_cast<float>(Window::GetHeight()));

		// When viewports are enabled we tweak WindowRounding/WindowBg so platform windows can look identical to regular ones.
		ImGuiStyle& style = ImGui::GetStyle();
		if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
		{
			style.WindowRounding = 0.0f;
			style.Colors[ImGuiCol_WindowBg].w = 1.0f;
		}
		style.Colors[ImGuiCol_WindowBg] = ImVec4(0.15f, 0.15f, 0.15f, style.Colors[ImGuiCol_WindowBg].w);

				// initialize imgui backends only if backend not already set
		if (io.BackendPlatformUserData == nullptr)
		{
			// Use the actual SDL_Window* owned by our Window wrapper instead of the raw native handle.
			SEDX_CORE_ASSERT(window.GetWindow() != nullptr, "SDL_Window is null when initializing ImGui SDL backend");
			SEDX_CORE_ASSERT(ImGui_ImplSDL3_InitForVulkan(static_cast<SDL_Window*>(window.GetWindow())), "Failed to initialize ImGui's SDL3 Vulkan backend.");
		}
		else
		{
			EDITOR_WARN_TAG("UILayer", "ImGui backend already initialized; skipping ImGui_ImplSDL3_InitForVulkan");
		}

		m_UIRenderer = CreateScope<UIRenderer>();
		m_UIRenderer->Init();

		InitPlatformInterface();
	}
	
	void UILayer::OnDetach()
	{
		ImGui_ImplSDL3_Shutdown();
		ImGui::DestroyContext();
	}
	
	void UILayer::Tick()
	{

		for (Ref<UI::EditorPanel> &widget : m_UIPanels)
		{
			widget->OnTick();
		}

		MenuBar::Tick();
		//UI::ChildWindow::Tick();
	}

	void UILayer::BeginRendering()
	{
		// Guard: if backend hasn't provided a DisplaySize yet (can happen early on),
		// set a safe default from our window so ImGui::NewFrame() won't assert.
		{
			ImGuiIO &io = ImGui::GetIO();
			if (io.DisplaySize.x < 0.0f || io.DisplaySize.y < 0.0f)
				io.DisplaySize = ImVec2(static_cast<float>(Window::GetWidth()), static_cast<float>(Window::GetHeight()));
		}

		ImGui::SetMouseCursor(Input::GetCursorMode() == CursorMode::Normal ? ImGuiMouseCursor_Arrow : ImGuiMouseCursor_None);

		//m_UIRenderer->UpdateFontTexture();
		ImGui_ImplSDL3_NewFrame();

		ImGui::NewFrame();
		ImGuizmo::BeginFrame();

		BeginWindow();
	}

	void UILayer::EndRendering()
	{
	    // Finalize the ImGui frame — this populates GetDrawData() for the render
		// thread's Renderer::Tick() → UI::Render() call, and satisfies the
		// internal FrameCountEnded == FrameCount invariant that
		// UpdatePlatformWindows() asserts on.
		// NOTE: BeginFrame()/EndFrame()/SubmitAndPresent() are owned exclusively
		//       by WaitAndRender() on the render thread — do NOT call them here.
		ImGui::Render();
		UI::Render(ImGui::GetDrawData());

	    // Multi-viewport: propagate draw data to secondary OS windows.
		// Must come after ImGui::Render().
		// Update and Render additional Platform Windows
		if (ImGui::GetIO().ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
		{
			ImGui::UpdatePlatformWindows();
			ImGui::RenderPlatformWindowsDefault();
		}
	}

	void UILayer::AllowInputEvents(bool allowEvents)
	{

	}
	
	UIRenderer *UILayer::Get()
	{
		return m_UIRenderer.get();
	}

} // namespace SceneryEditorX

// -------------------------------------------------------
