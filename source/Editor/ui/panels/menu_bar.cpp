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
 * menu_bar.cpp
 * -------------------------------------------------------
 * Created: 29/3/2025
 * -------------------------------------------------------
 */
#include "menu_bar.h"
#include "asset_browser.h"
#include "file_dialog.h"
#include "properties.h"
#include "texure_viewer.h"
#include "scene_viewport.h"
#include "Editor/ui/ui.h"
#include <SceneryEditorX/core/version.h>
#include <SceneryEditorX/core/resource/resource_cache.h>
#include <SceneryEditorX/core/threading/thread_pool.h>
#include <SceneryEditorX/core/window/window.h>
#include <SceneryEditorX/renderer/vulkan/debug/graphics_debug.h>
#include <SceneryEditorX/scene/scene.h>
#include "render_options.h"
#include "Editor/core/child_window.h"
#include "Editor/core/editor_layer.h"

#include <SceneryEditorX/renderer/renderer.h>

using namespace SceneryEditorX;

// -------------------------------------------------------

namespace
{
	bool show_file_dialog          = false;
	bool show_imgui_metrics_window = false;
	bool show_imgui_style_window   = false;
	bool show_imgui_demo_widow     = false;
	EditorLayer* editor                 = nullptr;
	std::string file_dialog_selection_path;
	Scope<FileDialog> file_dialog;

	template <class T>
	void MenuEntry()
	{
		T* widget = editor->GetWidget<T>();

		// menu item with checkmark based on widget->GetVisible()
		if (ImGui::MenuItem(widget->GetTitle(), nullptr, widget->GetVisible()))
		{
			// toggle visibility
			widget->SetVisible(!widget->GetVisible());
		}
	}

	namespace Windows
	{
		void ShowWorldSaveDialog()
		{
			file_dialog->SetOperation(FileDialog_Op_Save);

			// navigate to the directory of the currently loaded world
			const std::string& world_file_path = Scene::GetFilePath();
			if (!world_file_path.empty())
			{
				file_dialog->SetCurrentPath(world_file_path);
			}

			show_file_dialog = true;
		}

		void ShowWorldLoadDialog()
		{
			file_dialog->SetOperation(FileDialog_Op_Load);
			show_file_dialog = true;
		}

		void ExportWorld()
		{
			const std::string& world_file_path = Scene::GetFilePath();
			if (world_file_path.empty())
			{
				EDITOR_ERROR_TAG("Menubar","No world is currently loaded. Save the world first before exporting.");
				return;
			}

			ThreadPool::Submit([world_file_path]()
			{
				// get the world name and construct paths
				std::string world_name     = IO::FileSystem::GetFileNameWithoutExtensionFromFilePath(world_file_path);
				std::string world_dir      = IO::FileSystem::GetDirectoryFromFilePath(world_file_path);
				std::string resources_dir  = world_dir + world_name + "_resources";
				std::string archive_path   = world_dir + world_name + ".7z";

				// collect paths to include in the archive
				std::vector<std::string> paths_to_include;
				paths_to_include.push_back(world_file_path);

				// add resources directory if it exists
				if (IO::FileSystem::Exists(resources_dir))
				{
					paths_to_include.push_back(resources_dir);
				}

				// create the archive
				if (IO::FileSystem::CreateArchive(archive_path, paths_to_include))
				{
					EDITOR_INFO_TAG("Menubar","World exported to: %s", archive_path.c_str());
				}
			});
		}

		void DrawFileDialog()
		{
			if (show_file_dialog)
			{
				ImGui::SetNextWindowFocus();
			}

			if (file_dialog->Show(&show_file_dialog, editor, nullptr, &file_dialog_selection_path))
			{
				// load world
				if (file_dialog->GetOperation() == FileDialog_Op_Open || file_dialog->GetOperation() == FileDialog_Op_Load)
				{
					if (IO::FileSystem::IsValidExtension(file_dialog_selection_path, AssetType::Scene))
					{
						Scene::LoadFromFile(file_dialog_selection_path);
						show_file_dialog = false;
					}
				}

				// save world
				else if (file_dialog->GetOperation() == FileDialog_Op_Save)
				{
					if (file_dialog->GetFilter() == FileDialog_Filter_World)
					{
						ThreadPool::Submit([]()
						{
							Scene::SaveToFile(file_dialog_selection_path);
						});

						show_file_dialog = false;
					}
				}
			}
		}
	}

	namespace buttons_menu
	{
		void world()
		{
			if (ImGui::BeginMenu("World"))
			{
				if (ImGui::MenuItem("New"))
				{
					Scene::Shutdown();
				}

				ImGui::Separator();

				if (ImGui::MenuItem("Load"))
				{
					Windows::ShowWorldLoadDialog();
				}

				ImGui::Separator();

				if (ImGui::MenuItem("Save", "Ctrl+S"))
				{
					Windows::ShowWorldSaveDialog();
				}

				if (ImGui::MenuItem("Save As...", "Ctrl+S"))
				{
					Windows::ShowWorldSaveDialog();
				}

				ImGui::Separator();

				if (ImGui::MenuItem("Export"))
				{
					Windows::ExportWorld();
				}

				ImGui::EndMenu();
			}
		}

		void view()
		{
			if (ImGui::BeginMenu("View"))
			{

				if (ImGui::BeginMenu("Widgets"))
				{
					//menu_entry<Profiler>();
					//menu_entry<ShaderEditor>();
					//menu_entry<ScriptEditor>();
					MenuEntry<RenderOptions>();
					MenuEntry<TextureViewer>();
					//menu_entry<ResourceViewer>();
					MenuEntry<AssetBrowser>();
					//menu_entry<Console>();
					MenuEntry<Properties>();
				    MenuEntry<SceneViewport>();
					//menu_entry<WorldViewer>();

					ImGui::EndMenu();
				}

				if (ImGui::BeginMenu("ImGui"))
				{
					ImGui::MenuItem("Metrics", nullptr, &show_imgui_metrics_window);
					ImGui::MenuItem("Style", nullptr, &show_imgui_style_window);
					ImGui::MenuItem("Demo", nullptr, &show_imgui_demo_widow);

					ImGui::EndMenu();
				}

				ImGui::EndMenu();
			}
		}

		void help()
		{
			if (ImGui::BeginMenu("Help"))
			{
				//ImGui::MenuItem("About", nullptr, ChildWindow::GetVisiblityWindowAbout());

				if (ImGui::MenuItem("Support the Development", nullptr, nullptr))
				{
					IO::FileSystem::OpenUrl("https://buymeacoffee.com/the3dvehicleguy");
				}

				if (ImGui::MenuItem("Contributing", nullptr, nullptr))
				{
					IO::FileSystem::OpenUrl("https://github.com/Coalition-of-Freeware-Developers/Scenery-Editor-X");
				}

				if (ImGui::MenuItem("Report a bug", nullptr, nullptr))
				{
					IO::FileSystem::OpenUrl("https://github.com/PanosK92/SpartanEngine/issues/new/choose");
				}

				ImGui::EndMenu();
			}
		}
	}

	// forward declaration for buttons_titlebar
	namespace buttons_titlebar { float get_total_width(); }

	namespace buttons_toolbar
	{
		float button_size = 19.0f;
		std::unordered_map<ImageResource*, Widget*> widgets;

		// a button that when pressed will call "on press" and derives it's color (active/inactive) based on "get_visibility".
		void toolbar_button(ImageResource* icon_type, const char* tooltip_text, bool (*get_visibility)(Widget*), void (*on_press)(Widget*), Widget* widget = nullptr, float cursor_pos_x = -1.0f)
		{
			ImGui::SameLine();
			ImVec4 button_color = get_visibility(widget) ? ImGui::GetStyle().Colors[ImGuiCol_ButtonActive] : ImGui::GetStyle().Colors[ImGuiCol_Button];
			ImGui::PushStyleColor(ImGuiCol_Button, button_color);
			if (cursor_pos_x > 0.0f)
			{
				ImGui::SetCursorPosX(cursor_pos_x);
			}

			const ImGuiStyle& style   = ImGui::GetStyle();
			const float size_avail_y  = 2.0f * style.FramePadding.y + button_size;
			const float button_size_y = button_size + 2.0f * MenuBar::GetPaddingY();
			const float offset_y      = (button_size_y - size_avail_y) * 0.5f;

			ImGui::SetCursorPosY(offset_y);

			if (UI::ImageButton(icon_type, {button_size * Window::GetDpiScale(), button_size * Window::GetDpiScale()}, false))
			{
				on_press(widget);
			}

			ImGui::PopStyleColor();

			UI::Tooltip(tooltip_text);
		}

		void tick()
		{
			const ImGuiViewport* viewport = ImGui::GetMainViewport();
			const float size_avail_x      = viewport->Size.x;
			const float button_size_final = button_size * Window::GetDpiScale() + MenuBar::GetPaddingX() * 2.0f;
			float num_buttons             = 1.0f;
			float size_toolbar            = num_buttons * button_size_final;
			float cursor_pos_x            = (size_avail_x - size_toolbar) * 0.5f;

			// all the other buttons (offset to leave space for title bar buttons + separator gap)
			ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, { MenuBar::GetPaddingX() - 1.0f, MenuBar::GetPaddingY() - 5.0f });
			ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing,  { 4.0f , 0.0f });
			{
				num_buttons  = 8.0f;
				size_toolbar = num_buttons * button_size_final + (num_buttons - 1.0f) * ImGui::GetStyle().ItemSpacing.x;
				float titlebar_buttons_width = buttons_titlebar::get_total_width();
				cursor_pos_x = size_avail_x - size_toolbar - titlebar_buttons_width;

				// buttons from custom functionality
				{
					// screenshot button
					static auto screenshot_visible = [](Widget*) { return false; };
					static auto screenshot_press   = [](Widget*)
					{
						EDITOR_WARN_TAG("Menubar", "Screenshot functionality is currently disabled.");
						//Renderer::Screenshot();
					};
					toolbar_button(ResourceCache::GetIcon(IconType::Screenshot), "Takes a screenshot and saves it to the executable's folder",
						screenshot_visible,
						screenshot_press,
						nullptr,
						cursor_pos_x
					);

					// renderdoc button
					static auto renderdoc_visible = [](Widget*) { return false; };
					static auto renderdoc_press   = [](Widget*)
					{
						if (Debugging::IsRenderdocEnabled())
						{
							EDITOR_WARN_TAG("Menubar", "RenderDoc functionality is currently disabled.");
							//RenderDoc::FrameCapture();
						}
						else
						{
							EDITOR_WARN_TAG("Menubar","RenderDoc integration is disabled. To enable, go to \"Debugging.h\", and set \"is_renderdoc_enabled\" to \"true\"");
						}
					};
					toolbar_button(ResourceCache::GetIcon(IconType::RenderDoc), "Captures the next frame and then launches RenderDoc",
						renderdoc_visible,
						renderdoc_press,
						nullptr
					);

					// world selection
					/*static auto world_visible = [](Widget*) { return ChildWindow::GetVisibilityWorlds(); };
					static auto world_press   = [](Widget*) { ChildWindow::SetVisibilityWorlds(!ChildWindow::GetVisibilityWorlds()); };
					toolbar_button(ResourceCache::GetIcon(IconType::Terrain), "World selection window",
						world_visible,
						world_press,
						nullptr
					);*/
				}

				// buttons from widgets
				for (auto& widget_it : widgets)
				{
					Widget* widget = widget_it.second;
					ImageResource* widget_icon = widget_it.first;
					static auto is_widget_visible  = [](Widget* widget) { return widget->GetVisible(); };
					static auto set_widget_visible = [](Widget* widget) { widget->SetVisible(true); };
					toolbar_button(widget_icon, widget->GetTitle(), is_widget_visible, set_widget_visible, widget);
				}
			}
			ImGui::PopStyleVar(2);
		}
	}

	// window buttons: minimize, maximize, close for custom title bar
	namespace buttons_titlebar
	{
		const float icon_size_base     = 12.0f;  // base icon size
		const float button_padding_x   = 18.0f;  // horizontal padding around each button
		const float button_padding_y   = 8.0f;   // vertical padding
		const float separator_gap      = 20.0f;  // gap between toolbar and window controls

		float get_total_width()
		{
			// 3 buttons width + separator gap + margin
			float dpi = Window::GetDpiScale();
			float margin = 2.0f;
			return (3.0f * (icon_size_base + button_padding_x * 2.0f) + separator_gap + margin) * dpi;
		}

		void tick(float menubar_height)
		{
			const float dpi = Window::GetDpiScale();

			const float icon_size_scaled = icon_size_base * dpi;
			const float button_width     = icon_size_scaled + button_padding_x * 2.0f * dpi;
			const xMath::Vec2 icon_size = xMath::Vec2(icon_size_scaled, icon_size_scaled);
			// calculate vertical centering
			const float button_height = icon_size_scaled + button_padding_y * 2.0f * dpi;
			const float offset_y = (menubar_height - button_height) * 0.5f;

			// position first button - use window width and account for small margin
			const float window_width = ImGui::GetWindowWidth();
			const float margin = 2.0f * dpi;  // small margin from edge
			float start_x = window_width - (3.0f * button_width) - margin;
			ImGui::SetCursorPosX(start_x);
			ImGui::SetCursorPosY(offset_y);

			// minimize button
			ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0, 0, 0, 0));
			ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(1, 1, 1, 0.1f));
			ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(1, 1, 1, 0.2f));
			ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(button_padding_x * dpi, button_padding_y * dpi));

			if (UI::ImageButton(ResourceCache::GetIcon(IconType::Minimize), icon_size, false))
			{
				Window::Minimize();
			}

			ImGui::SameLine(0, 0);
			ImGui::SetCursorPosY(offset_y);

			// maximize/restore button
			if (UI::ImageButton(ResourceCache::GetIcon(IconType::Maximize), icon_size, false))
			{
				Window::Maximize();
			}

			ImGui::PopStyleColor(3);

			ImGui::SameLine(0, 0);
			ImGui::SetCursorPosY(offset_y);

			// close button with red hover
			ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0, 0, 0, 0));
			ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.9f, 0.2f, 0.2f, 1.0f));
			ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.7f, 0.1f, 0.1f, 1.0f));

			if (UI::ImageButton(ResourceCache::GetIcon(IconType::Close), icon_size, false))
			{
				Window::SetShouldClose(true);
			}

			ImGui::PopStyleColor(3);
			ImGui::PopStyleVar();
		}
	}
}

void MenuBar::Initialize(EditorLayer* editor)
{
	file_dialog = CreateScope<FileDialog>(true, FileDialog_Type_FileSelection, FileDialog_Op_Open, FileDialog_Filter_World);

	//buttons_toolbar::widgets[ResourceCache::GetIcon(IconType::Profiler)]      = editor->GetWidget<Profiler>();
	//buttons_toolbar::widgets[ResourceCache::GetIcon(IconType::ResourceCache)] = editor->GetWidget<ResourceViewer>();
	//buttons_toolbar::widgets[ResourceCache::GetIcon(IconType::Shader)]        = editor->GetWidget<ShaderEditor>();
	buttons_toolbar::widgets[ResourceCache::GetIcon(IconType::Gear)]          = editor->GetWidget<RenderOptions>();
	buttons_toolbar::widgets[ResourceCache::GetIcon(IconType::Texture)]       = editor->GetWidget<TextureViewer>();
}

void MenuBar::Tick()
{
#pragma region MenuBar
	{
		ImGuiStyle& style = ImGui::GetStyle();
		ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(style.FramePadding.x, 8.0f));

		if (ImGui::BeginMainMenuBar())
		{
			// get menu bar height for hit test configuration
			float menubar_height = ImGui::GetWindowHeight();

			// configure hit test regions for custom title bar
			Window::SetTitleBarHeight(menubar_height);
			Window::SetTitleBarButtonWidth(buttons_titlebar::get_total_width());

			// layout values
			float dpi              = Window::GetDpiScale();
			float icon_size        = 16.0f * dpi;
			float padding_x        = 6.0f * dpi;
			float frame_padding_y  = ImGui::GetStyle().FramePadding.y;
			float text_height      = ImGui::GetTextLineHeight();
			float menu_item_height = text_height + frame_padding_y * 2.0f;
			float menu_y           = (menubar_height - menu_item_height) * 0.5f;
			float icon_y           = (menubar_height - icon_size) * 0.5f;

			// logo
			ImGui::SetCursorPosX(padding_x);
			ImGui::SetCursorPosY(icon_y);
			if (ImageResource *logo = ResourceCache::GetIcon(IconType::Logo))
			{
				ImGui::Image(reinterpret_cast<ImTextureID>(logo), ImVec2(icon_size, icon_size));
			}
			ImGui::SameLine(0, padding_x * 0.5f);

			// title with version
			static char title[64] = {};
			if (title[0] == '\0')
			{
				snprintf(title, sizeof(title), "Scenery Editor X - v%s", SEDX_VERSION_STRING);
			}
			ImGui::SetCursorPosY(menu_y);
			ImGui::MenuItem(title, nullptr, false, false);
			ImGui::SameLine(0, padding_x * 2.0f);

			ImGui::SetCursorPosY(menu_y);
			buttons_menu::world();
			ImGui::SetCursorPosY(menu_y);
			buttons_menu::view();
			ImGui::SetCursorPosY(menu_y);
			buttons_menu::help();

			// display current world name
			{
				const std::string & world_name = Scene::GetName();
				if (!world_name.empty())
				{
					ImGui::SameLine(0, padding_x * 2.0f);
					ImGui::SetCursorPosY(menu_y);
					ImGui::TextDisabled("|");
					ImGui::SameLine(0, padding_x);
					ImGui::SetCursorPosY(menu_y);
					ImGui::TextDisabled("%s", world_name.c_str());
				}
			}

			buttons_toolbar::tick();

			// render window control buttons (minimize, maximize, close)
			buttons_titlebar::tick(menubar_height);

			// update title bar hovered state for hit test callback
			// this allows sdl to make the title bar draggable only when no imgui items are hovered
			{
				bool any_item_hovered = ImGui::IsAnyItemHovered() || ImGui::IsAnyItemActive() || ImGui::IsPopupOpen(nullptr, ImGuiPopupFlags_AnyPopup);
				Window::SetTitleBarHovered(any_item_hovered);

				// double-click on empty space to maximize/restore
				bool mouse_in_menubar = ImGui::IsWindowHovered(ImGuiHoveredFlags_AllowWhenBlockedByActiveItem);
				if (mouse_in_menubar && !any_item_hovered && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left))
				{
					Window::Maximize();
				}
			}

			ImGui::EndMainMenuBar();
		}

		ImGui::PopStyleVar();
	}
#pragma endregion

#pragma region Windows
	// windows
	{
		if (show_imgui_metrics_window)
		{
			ImGui::ShowMetricsWindow();
		}

		if (show_imgui_demo_widow)
		{
			ImGui::ShowDemoWindow(&show_imgui_demo_widow);
		}

		//editor->GetWidget<Style>()->SetVisible(show_imgui_style_window);
	}
#pragma endregion

	Windows::DrawFileDialog();
}

void MenuBar::ShowWorldSaveDialog()
{
	Windows::ShowWorldSaveDialog();
}

void MenuBar::ShowWorldLoadDialog()
{
	Windows::ShowWorldLoadDialog();
}

/*
namespace UI
{
	void UIManager::MainMenuBar()
	{
		if (ImGui::BeginMainMenuBar())
		{
			if (ImGui::BeginMenu("File"))
			{
				if (ImGui::MenuItem("New", "Ctrl+N"))
				{
					showCreateProjectModal = true;
				}

				if (ImGui::MenuItem("Open", "Ctrl+O")) { /* Prompt the user to select a file from a file manager #1# }
				if (ImGui::BeginMenu("Open Recent"))
				{
					ImGui::MenuItem("KHVN | Tweed New Haven", nullptr);
					ImGui::MenuItem("EGLL | London Heathrow", nullptr);
					ImGui::MenuItem("EINN | Shannon Intl.", nullptr);
					ImGui::MenuItem("EIDW | Dublin Intl.", nullptr);
					ImGui::MenuItem("KMEM | Memphis Intl.", nullptr);
					ImGui::EndMenu();
				}

				ImGui::Separator();

				if (ImGui::BeginMenu("Import", "Ctrl+I"))
				{
					ImGui::MenuItem("Import Scenery Gateway", nullptr);
					ImGui::MenuItem("Import WED Project", nullptr);
					ImGui::Separator();
					ImGui::MenuItem("Convert WED Project");
					ImGui::EndMenu();
				}

				if (ImGui::BeginMenu("Export", "Ctrl+I"))
				{
					ImGui::MenuItem("Export to Scenery Gateway", nullptr);
					ImGui::MenuItem("Export to X-Plane", nullptr);
					ImGui::Separator();

					if (ImGui::BeginMenu("Export to File"))
					{
						ImGui::MenuItem("Export '.apt'", nullptr);
						ImGui::MenuItem("Export '.dsf'", nullptr);
						ImGui::EndMenu();
					}
					ImGui::MenuItem("Export Terrain", nullptr);
					ImGui::EndMenu();
				}

				ImGui::Separator();

				if (ImGui::MenuItem("Save", "Ctrl+S")) { /* Do something here #1# }
				if (ImGui::MenuItem("Save As..", "Ctrl+Shift+S")) { /* Do something here #1# }

				ImGui::Separator();

				if (ImGui::MenuItem("Exit", "Alt+F4"))
				{
					showExitModal = true;
				   //glfwSetWindowShouldClose(window, true);
				}

				ImGui::EndMenu();

			}

			if (ImGui::BeginMenu("Edit"))
			{
				if (ImGui::MenuItem("Undo", "Ctrl+Z")) { /* Do something here #1# }
				if (ImGui::MenuItem("Redo", "Ctrl+Y")) { /* Do something here #1# }
				ImGui::Separator();
				if (ImGui::MenuItem("Cut", "Ctrl+X")) { /* Do something here #1# }
				if (ImGui::MenuItem("Copy", "Ctrl+C")) { /* Do something here #1# }
				if (ImGui::MenuItem("Paste", "Ctrl+V")) { /* Do something here #1# }
				if (ImGui::MenuItem("Duplicate", "Ctrl+D")) { /* Do something here #1# }
				ImGui::EndMenu();
			}
			
			if (ImGui::BeginMenu("Window"))
			{
				if (ImGui::BeginMenu("Views"))
				{
					static bool enabled = true;
					ImGui::Text("Camera Types");
					ImGui::MenuItem("Perspective View", nullptr, &enabled);
					ImGui::MenuItem("Orthoscopic View", nullptr, &enabled);
					ImGui::Separator();
					ImGui::MenuItem("Center on Airport", nullptr);
					ImGui::MenuItem("Tower View", nullptr, &enabled);
					ImGui::MenuItem("Walk Around", nullptr, &enabled);
					ImGui::MenuItem("Free Cam", nullptr, &enabled);
					ImGui::EndMenu();
				}

				ImGui::Separator();
			   
				if (ImGui::BeginMenu("Toolbars"))
				{
					static bool enabled = true;
					ImGui::MenuItem("Content Browser", nullptr, &enabled);
					ImGui::MenuItem("Layer Stack", nullptr, &enabled);
					ImGui::MenuItem("Errors", nullptr, &enabled);
					ImGui::MenuItem("Properties", nullptr, &enabled);
					ImGui::EndMenu();
				}

				ImGui::Separator();
				static bool enabled = true;
				if (ImGui::MenuItem("Show Grid", "G", &enabled)) { /* Do something here #1# }
				if (ImGui::MenuItem("Show Axis", nullptr, &enabled)) { /* Do something here #1# }
				ImGui::Separator();
				if (ImGui::MenuItem("Save UI", nullptr)) { /* Do something here #1#}
				if (ImGui::MenuItem("Reset UI", nullptr)) { /* Do something here #1#}
				ImGui::EndMenu();
			}
			
			if (ImGui::BeginMenu("Viewport"))
			{
				static bool enabled = true;
				ImGui::Text("Terrain");
				ImGui::MenuItem("Show Terrain", nullptr, &enabled);
				ImGui::MenuItem("Terrain Wireframe", "Ctrl+W", &enabled);
				ImGui::Separator();
				ImGui::Text("Models");
				ImGui::MenuItem("Scene Wireframe", nullptr, &enabled);
				ImGui::EndMenu();
			}

			if (ImGui::BeginMenu("Plugins"))
			{
				if (ImGui::MenuItem("Plugin Manager", nullptr))
				{
					//showPluginManager = true;
				}
				ImGui::Separator();
				ImGui::MenuItem("Install Plugin", nullptr);
				if (ImGui::MenuItem("Plugin Config", nullptr))
				{
					//showPluginConfig = true;
				}
				ImGui::EndMenu();
			}
			
			if (ImGui::BeginMenu("Options"))
			{
				static bool enabled = true;
				if (ImGui::MenuItem("Toggle Snaps", "S", &enabled)) { /* Do something here #1# }
				if (ImGui::BeginMenu("Snaps"))
				{
					ImGui::MenuItem("Toggle Grid Snap", nullptr, &enabled);
					ImGui::MenuItem("Toggle Vertex Snap", nullptr, &enabled);
					ImGui::MenuItem("Toggle Surface Snap", nullptr, &enabled);
					ImGui::EndMenu();
				}
				ImGui::EndMenu();
			}
			
			if (ImGui::MenuItem("Settings"))
			{
				showSettingsPanel = true;
			}

			if (ImGui::BeginMenu("Help"))
			{
				static bool showHelp = true;
				ImGui::MenuItem("Show Help", nullptr, &showHelp);
				if (ImGui::MenuItem("Check for Updates", nullptr)) { /* Do something here #1#}
				(ImGui::MenuItem("Release Notes", nullptr));
				(ImGui::MenuItem("Bug Report", nullptr));
				if (ImGui::MenuItem("About", nullptr))
				{
					showAboutModal = true;
				}
				ImGui::EndMenu();
			}
		ImGui::EndMainMenuBar();
		}
	}

} // namespace UI
*/

// -------------------------------------------------------

