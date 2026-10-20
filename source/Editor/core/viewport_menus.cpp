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
 * viewport_menus.cpp
 * -------------------------------------------------------
 * Created: 19/10/2026
 * -------------------------------------------------------
 */
#include "viewport_menus.h"
#include "editor_camera.h"
#include "editor_commands.h"
#include "editor_context.h"
#include "editor_history.h"
#include "editor_project.h"
#include "editor_theme.h"
#include "editor_tools.h"
#include "editor_ui.h"
#include "facade_tool.h"
#include "node_tool.h"
#include <algorithm>
#include <array>
#include <cctype>
#include <SceneryEditorX/renderer/font/font_awesome.h>
#include <SceneryEditorX/renderer/ui/source/imgui/imgui_internal.h>

// -------------------------------------------------------

namespace SceneryEditorX::ViewportMenus
{
	using EditorSelection::Target;
	using EditorSelection::TargetType;

	namespace
	{
		/* @brief One line of a menu panel. */
		struct Entry
		{
			enum class Kind : uint8_t
			{
				Command,	// An EditorCommands id
				Action,		// A one-off closure (it captured where the menu opened)
				Submenu,
				Separator,
				Text
			};

			Kind kind = Kind::Command;
			std::string id;
			std::string label;
			const char *icon = nullptr;
			std::function<void()> run;		// Action: what it does; Submenu: its items
			bool enabled = true;
		};

		struct Panel
		{
			std::string title;
			std::vector<Entry> entries;
		};

		enum Quadrant : uint8_t
		{
			TopLeft,
			TopRight,
			BottomLeft,
			BottomRight
		};

		using Panels = std::array<Panel, 4>;

		Entry Command(std::string id)
		{
			Entry entry;
			entry.kind = Entry::Kind::Command;
			entry.id = std::move(id);
			return entry;
		}

		Entry Action(std::string label, const char *icon, std::function<void()> run, const bool enabled = true)
		{
			Entry entry;
			entry.kind = Entry::Kind::Action;
			entry.label = std::move(label);
			entry.icon = icon;
			entry.run = std::move(run);
			entry.enabled = enabled;
			return entry;
		}

		Entry Submenu(std::string label, const char *icon, std::function<void()> items, const bool enabled = true)
		{
			Entry entry;
			entry.kind = Entry::Kind::Submenu;
			entry.label = std::move(label);
			entry.icon = icon;
			entry.run = std::move(items);
			entry.enabled = enabled;
			return entry;
		}

		Entry Separator()
		{
			Entry entry;
			entry.kind = Entry::Kind::Separator;
			return entry;
		}

		Entry Text(std::string text)
		{
			Entry entry;
			entry.kind = Entry::Kind::Text;
			entry.label = std::move(text);
			return entry;
		}

		struct QuadState
		{
			bool requestOpen = false;
			bool open = false;
			bool closeRequested = false;
			ImVec2 at;
			std::optional<Target> target;
			std::optional<Point3> ground;
			Panels panels;
		};

		struct HotboxState
		{
			bool visible = false;
			bool focus = false;
			bool pinned = false;	// Shown without Space (PinHotbox)
			ImVec2 center;
			Panels panels;
		};

		struct State
		{
			std::vector<std::pair<std::string, MenuDrawer>> mainMenus;
			QuadState quad;
			HotboxState hotbox;
		};

		State &S()
		{
			static State state;
			return state;
		}

		std::string Upper(std::string text)
		{
			for (char &c : text)
				c = static_cast<char>(std::toupper(static_cast<unsigned char>(c)));
			return text;
		}

		const ProjectFile::SceneryItem *FindItem(const Target &target)
		{
			if (target.type != TargetType::ProjectItem)
				return nullptr;
			ProjectFile::ProjectDocument *document = EditorProjects::GetDocument();
			return document ? document->FindItem(target.itemId) : nullptr;
		}

		/* @brief Creation on empty ground: the facade tool's modes, dropping the armed facade where the menu opened, the library. */
		Panel CreatePanel(EditorContext *context, const std::optional<Point3> &ground)
		{
			Panel panel{ "CREATE", {} };
			for (const char *id : { "facade.draw.rectangle", "facade.draw.polygon", "facade.draw.circle", "facade.draw.line" })
				panel.entries.push_back(Command(id));
			const std::string &armed = FacadeTool::GetSettings().resource;
			if (ground && !armed.empty())
			{
				const float position[3] = { static_cast<float>(ground->x), 0.0f, static_cast<float>(ground->z) };
				const std::array<float, 3> at{ position[0], position[1], position[2] };
				const size_t slash = armed.find_last_of('/');
				panel.entries.push_back(Action("Place " + (slash == std::string::npos ? armed : armed.substr(slash + 1)) + " Here", SEDX_ICON_PLUS_SQUARE,
					[context, armed, at]() { FacadeTool::Drop(context, armed, at.data()); }, EditorProjects::GetDocument() != nullptr));
			}
			panel.entries.push_back(Separator());
			panel.entries.push_back(Submenu("Facade Library", SEDX_ICON_BOOK, []() { FacadeTool::DrawLibraryMenu(); }));
			panel.entries.push_back(Submenu("Recent Facades", SEDX_ICON_STAR, []()
			{
				const std::vector<std::string> &recent = FacadeTool::GetRecent();
				if (recent.empty())
					ImGui::TextDisabled("None yet");
				for (const std::string &resource : recent)
				{
					if (ImGui::MenuItem(resource.c_str(), nullptr, resource == FacadeTool::GetSettings().resource))
					{
						FacadeTool::SetResource(resource);
						EditorTools::SetTool(EditorTools::Tool::Facade);
					}
				}
			}, !FacadeTool::GetRecent().empty()));
			return panel;
		}

		/* @brief The panels for what the menu opened on: its own tools on the left, display and transform on the right. */
		Panels BuildPanels(EditorContext *context, const std::optional<Target> &target, const std::optional<Point3> &ground)
		{
			Panels panels;
			panels[TopRight] = { "DISPLAY", { Command("edit.frame"), Command("item.hide"), Command("item.unhide_all"), Command("item.lock"), Separator(),
				Command("facade.labels"), Command("view.reset_camera") } };
			panels[BottomRight] = { "TRANSFORM", { Command("tool.select"), Command("tool.move"), Command("tool.rotate"), Command("tool.scale"), Command("tool.nodes"),
				Command("facade.tool"), Separator(), Command("item.duplicate"), Command("edit.delete"), Command("edit.deselect") } };

			const std::vector<std::string> facades = FacadeTool::GetSelectedFacades();
			const ProjectFile::SceneryItem *item = target ? FindItem(*target) : nullptr;
			if (!facades.empty() && item && item->kind == ProjectFile::ItemKind::Facade)
			{
				// A facade: its walls and heights on top, rebuilds below
				const std::vector<uint32_t> &walls = FacadeTool::GetSelectedWalls();
				const std::string wallsLabel = walls.empty() ? "Wall Type (every wall)" : walls.size() == 1 ? "Wall Type (wall " + std::to_string(walls.front() + 1) + ")" :
					"Wall Type (" + std::to_string(walls.size()) + " walls)";
				panels[TopLeft] = { "FACADE", {
					Submenu(wallsLabel, SEDX_ICON_TH_LIST, [facades]() { FacadeTool::DrawWallTypeMenu(facades); }),
					Submenu("Height", SEDX_ICON_ARROWS_V, [facades]() { FacadeTool::DrawHeightMenu(facades); }),
					Submenu("Replace With", SEDX_ICON_RETWEET, [facades]() { FacadeTool::DrawReplaceMenu(facades); }),
					Command("facade.pick_walls"), Command("facade.front"), Separator(), Command("facade.use"), Command("facade.inspect") } };
				panels[BottomLeft] = { "REBUILD", {
					Command("facade.subdivide"), Command("facade.square"), Command("facade.reverse"),
					Submenu("Add Around", SEDX_ICON_PLUS_SQUARE, [context, facades]()
					{
						// Sidewalks first (the 3D sidewalk library), then any facade
						const float offset = FacadeTool::GetSettings().sidewalkOffset;
						ImGui::TextDisabled("A new facade around the outline, %.2f m out", offset);
						static std::vector<std::string> s_Items;
						static EditorContext *s_Context = nullptr;
						s_Items = facades;
						s_Context = context;
						FacadeTool::DrawLibraryMenu([](const std::string &resource) { FacadeTool::AddAround(s_Context, s_Items, resource, FacadeTool::GetSettings().sidewalkOffset); });
					}),
					Command("tool.nodes") } };
				return panels;
			}

			if (item)
			{
				// Another project item: its kind's edits, and creation
				panels[TopLeft] = { Upper(ProjectFile::GetItemKindName(item->kind)), {} };
				if (!item->name.empty())
					panels[TopLeft].entries.push_back(Text(item->name));
				if (ProjectFile::HasNodes(item->kind))
				{
					panels[TopLeft].entries.push_back(Command("tool.nodes"));
					panels[TopLeft].entries.push_back(Command("nodes.reverse"));
				}
				panels[TopLeft].entries.push_back(Command("item.duplicate"));
				panels[TopLeft].entries.push_back(Command("item.hide"));
				panels[BottomLeft] = CreatePanel(context, ground);
				return panels;
			}

			if (target && target->type == TargetType::AirportElement)
			{
				panels[TopLeft] = { "AIRPORT", { Text(EditorSelection::Describe(*target)), Command("tool.nodes"), Command("nodes.reverse"), Command("edit.frame") } };
				panels[BottomLeft] = CreatePanel(context, ground);
				return panels;
			}

			// Empty ground (or a plain scene entity): creation, and the project
			panels[TopLeft] = CreatePanel(context, ground);
			panels[BottomLeft] = { "EDIT", { Command("edit.undo"), Command("edit.redo"), Separator(), Command("file.save") } };
			if (target)
				panels[BottomLeft].entries.insert(panels[BottomLeft].entries.begin(), Text(EditorSelection::Describe(*target)));
			return panels;
		}

		/* @brief Draws an entry; true when something ran (the menu closes). */
		bool DrawEntry(const Entry &entry, EditorContext *context)
		{
			switch (entry.kind)
			{
				case Entry::Kind::Command:
					return EditorCommands::MenuItem(entry.id, context);
				case Entry::Kind::Action:
				{
					const std::string label = entry.icon ? EditorUI::IconLabel(entry.icon, entry.label.c_str()) : entry.label;
					if (ImGui::MenuItem(label.c_str(), nullptr, false, entry.enabled) && entry.run)
					{
						entry.run();
						return true;
					}
					return false;
				}
				case Entry::Kind::Submenu:
				{
					const std::string label = entry.icon ? EditorUI::IconLabel(entry.icon, entry.label.c_str()) : entry.label;
					if (ImGui::BeginMenu(label.c_str(), entry.enabled))
					{
						if (entry.run)
							entry.run();
						ImGui::EndMenu();
					}
					return false;
				}
				case Entry::Kind::Separator:
					ImGui::Separator();
					return false;
				case Entry::Kind::Text:
					ImGui::TextDisabled("%s", entry.label.c_str());
					return false;
				default:
					return false;
			}
		}

		/* @brief Width an entry needs as a menu item, pixels. */
		float MeasureEntry(const Entry &entry)
		{
			const ImGuiStyle &style = ImGui::GetStyle();
			std::string label = entry.label;
			const char *icon = entry.icon;
			std::string shortcut;
			if (entry.kind == Entry::Kind::Command)
			{
				const EditorCommands::Command *command = EditorCommands::Find(entry.id);
				if (!command)
					return 0.0f;
				label = command->label;
				icon = command->icon;
				shortcut = command->shortcut;
			}
			const std::string text = icon ? EditorUI::IconLabel(icon, label.c_str()) : label;
			float width = ImGui::CalcTextSize(text.c_str()).x + style.ItemSpacing.x * 2.0f + ImGui::GetFontSize() * 1.5f;	// check mark / arrow
			if (!shortcut.empty())
				width += ImGui::CalcTextSize(shortcut.c_str()).x + style.ItemSpacing.x * 2.0f;
			return width;
		}

		ImVec2 MeasurePanel(const Panel &panel)
		{
			const ImGuiStyle &style = ImGui::GetStyle();
			const float dpi = style.FontScaleDpi;
			float width = ImGui::CalcTextSize(panel.title.c_str()).x + style.ItemSpacing.x * 4.0f;
			float height = ImGui::GetTextLineHeightWithSpacing();	// Title
			for (const Entry &entry : panel.entries)
			{
				width = std::max(width, MeasureEntry(entry));
				height += entry.kind == Entry::Kind::Separator ? style.ItemSpacing.y + 1.0f : ImGui::GetTextLineHeightWithSpacing();
			}
			// Window padding (5 px top and bottom) and the borders
			return ImVec2(std::max(150.0f * dpi, width + 16.0f * dpi), height + 12.0f * dpi);
		}

		void DrawTitle(const Panel &panel)
		{
			ImDrawList *draw = ImGui::GetWindowDrawList();
			const ImVec2 min = ImGui::GetCursorScreenPos();
			const float height = ImGui::GetTextLineHeightWithSpacing();
			draw->AddRectFilled(ImVec2(min.x - 4.0f, min.y), ImVec2(min.x + ImGui::GetContentRegionAvail().x + 4.0f, min.y + height), EditorTheme::ToU32(EditorTheme::Palette::AccentDim), 2.0f);
			// Moving the cursor is only allowed before an item (ImGui asserts on a move that would grow the window by itself)
			ImGui::SetCursorScreenPos(ImVec2(min.x + 2.0f, min.y + 1.0f));
			ImGui::TextUnformatted(panel.title.c_str());
		}

		// -------------------------------------------------------

		void DrawQuadMenu(EditorContext *context)
		{
			QuadState &quad = S().quad;
			constexpr const char *POPUP = "##ViewportQuadMenu";
			if (quad.requestOpen)
			{
				quad.requestOpen = false;
				quad.panels = BuildPanels(context, quad.target, quad.ground);
				ImGui::OpenPopup(POPUP);
			}

			const float dpi = ImGui::GetStyle().FontScaleDpi;
			const float gap = 8.0f * dpi;
			std::array<ImVec2, 4> sizes;
			for (size_t q = 0; q < 4; ++q)
				sizes[q] = quad.panels[q].entries.empty() ? ImVec2(0.0f, 0.0f) : MeasurePanel(quad.panels[q]);
			const float left = std::max(sizes[TopLeft].x, sizes[BottomLeft].x), right = std::max(sizes[TopRight].x, sizes[BottomRight].x);
			const float top = std::max(sizes[TopLeft].y, sizes[TopRight].y), bottom = std::max(sizes[BottomLeft].y, sizes[BottomRight].y);
			const ImVec2 total(left + right + gap, top + bottom + gap);

			// The panels meet at the cursor, moved in when that would leave the window
			const ImGuiViewport *viewport = ImGui::GetMainViewport();
			ImVec2 origin(quad.at.x - left - gap * 0.5f, quad.at.y - top - gap * 0.5f);
			origin.x = std::clamp(origin.x, viewport->WorkPos.x, std::max(viewport->WorkPos.x, viewport->WorkPos.x + viewport->WorkSize.x - total.x));
			origin.y = std::clamp(origin.y, viewport->WorkPos.y, std::max(viewport->WorkPos.y, viewport->WorkPos.y + viewport->WorkSize.y - total.y));
			const ImVec2 center(origin.x + left + gap * 0.5f, origin.y + top + gap * 0.5f);

			ImGui::SetNextWindowPos(origin);
			ImGui::SetNextWindowSize(total);
			ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
			ImGui::PushStyleVar(ImGuiStyleVar_PopupBorderSize, 0.0f);
			ImGui::PushStyleColor(ImGuiCol_PopupBg, ImVec4(0.0f, 0.0f, 0.0f, 0.0f));
			const bool open = ImGui::BeginPopup(POPUP, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoSavedSettings);
			ImGui::PopStyleColor();
			ImGui::PopStyleVar(2);
			quad.open = open;
			if (!open)
				return;

			bool ran = false;
			for (size_t q = 0; q < 4; ++q)
			{
				const Panel &panel = quad.panels[q];
				if (panel.entries.empty())
					continue;
				const bool isLeft = q == TopLeft || q == BottomLeft, isTop = q == TopLeft || q == TopRight;
				const ImVec2 size(isLeft ? left : right, sizes[q].y);
				// Left panels hug the center from the left, top panels from above (3ds Max's layout)
				const ImVec2 at(isLeft ? center.x - gap * 0.5f - size.x : center.x + gap * 0.5f, isTop ? center.y - gap * 0.5f - size.y : center.y + gap * 0.5f);
				ImGui::SetCursorScreenPos(at);
				ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(6.0f * dpi, 5.0f * dpi));
				ImGui::PushStyleColor(ImGuiCol_ChildBg, EditorTheme::ToStyle(ImVec4(EditorTheme::Palette::ToolStrip.x, EditorTheme::Palette::ToolStrip.y, EditorTheme::Palette::ToolStrip.z, 0.97f)));
				ImGui::PushID(static_cast<int>(q));
				if (ImGui::BeginChild("##Quadrant", size, ImGuiChildFlags_Borders | ImGuiChildFlags_AlwaysUseWindowPadding, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse))
				{
					if (isTop)
						DrawTitle(panel);
					for (size_t e = 0; e < panel.entries.size(); ++e)
					{
						ImGui::PushID(static_cast<int>(e));
						ran |= DrawEntry(panel.entries[e], context);
						ImGui::PopID();
					}
					if (!isTop)
						DrawTitle(panel);
				}
				ImGui::EndChild();
				ImGui::PopID();
				ImGui::PopStyleColor();
				ImGui::PopStyleVar();
			}

			// Where it was opened
			ImDrawList *draw = ImGui::GetForegroundDrawList();
			draw->AddCircleFilled(quad.at, 3.0f * dpi, IM_COL32(255, 154, 31, 255));

			if (ran || quad.closeRequested || ImGui::IsKeyPressed(ImGuiKey_Escape, false))
			{
				quad.closeRequested = false;
				ImGui::CloseCurrentPopup();
			}
			ImGui::EndPopup();
		}

		// -------------------------------------------------------

		/* @brief A row of menus (a child window with a menu bar, so menus open downward) centered on x. */
		void MenuRow(const char *id, const float centerX, const float y, const std::vector<std::pair<std::string, std::function<void()>>> &menus, const char *middle = nullptr)
		{
			const ImGuiStyle &style = ImGui::GetStyle();
			float width = style.WindowPadding.x * 2.0f;
			for (const auto &[name, items] : menus)
				width += ImGui::CalcTextSize(name.c_str()).x + style.ItemSpacing.x * 2.0f;
			if (middle)
				width += ImGui::CalcTextSize(middle).x + style.ItemSpacing.x * 4.0f;
			const float height = ImGui::GetFrameHeight() + 2.0f;
			ImGui::SetCursorScreenPos(ImVec2(centerX - width * 0.5f, y));
			// Theme colors are sRGB: converted for the sRGB swapchain like every other panel's
			const ImVec4 background = EditorTheme::ToStyle(ImVec4(EditorTheme::Palette::MenuBar.x, EditorTheme::Palette::MenuBar.y, EditorTheme::Palette::MenuBar.z, 0.96f));
			ImGui::PushStyleColor(ImGuiCol_MenuBarBg, background);
			ImGui::PushStyleColor(ImGuiCol_ChildBg, background);
			if (ImGui::BeginChild(id, ImVec2(width, height), ImGuiChildFlags_Borders, ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse))
			{
				if (ImGui::BeginMenuBar())
				{
					const size_t half = middle ? (menus.size() + 1) / 2 : menus.size();
					for (size_t i = 0; i < menus.size(); ++i)
					{
						if (middle && i == half)
						{
							ImGui::PushStyleColor(ImGuiCol_Text, EditorTheme::ToStyle(EditorTheme::Palette::AccentHovered));
							ImGui::TextUnformatted(middle);
							ImGui::PopStyleColor();
						}
						if (ImGui::BeginMenu(menus[i].first.c_str()))
						{
							menus[i].second();
							ImGui::EndMenu();
						}
					}
					if (middle && half >= menus.size())
						ImGui::TextUnformatted(middle);
					ImGui::EndMenuBar();
				}
			}
			ImGui::EndChild();
			ImGui::PopStyleColor(2);
		}

		void HideHotbox()
		{
			HotboxState &hotbox = S().hotbox;
			if (!hotbox.visible)
				return;
			hotbox.visible = false;
			hotbox.pinned = false;
			ImGui::ClosePopupsExceptModals();
		}

		void DrawHotbox(EditorContext *context)
		{
			State &s = S();
			HotboxState &hotbox = s.hotbox;
			const ImGuiIO &io = ImGui::GetIO();

			// Held Space over the viewport (not while typing, flying with the right button or with the quad menu up)
			if (!hotbox.visible)
			{
				const bool start = ImGui::IsKeyPressed(ImGuiKey_Space, false) && !io.WantTextInput && context && context->viewportHovered && !s.quad.open &&
					!ImGui::IsAnyMouseDown() && !EditorTools::IsBusy();
				if (!start)
					return;
				hotbox.visible = true;
				hotbox.focus = true;
				hotbox.center = io.MousePos;
				const EditorSelection::Target *active = EditorSelection::GetActive();
				hotbox.panels = BuildPanels(context, active ? std::optional<Target>(*active) : std::nullopt, std::nullopt);
			}
			if (!hotbox.pinned && (!ImGui::IsKeyDown(ImGuiKey_Space) || io.WantTextInput))
			{
				HideHotbox();
				return;
			}

			// A see-through window over everything, the rows centered on where Space went down
			const ImGuiViewport *viewport = ImGui::GetMainViewport();
			ImGui::SetNextWindowPos(viewport->Pos);
			ImGui::SetNextWindowSize(viewport->Size);
			ImGui::SetNextWindowViewport(viewport->ID);
			if (hotbox.focus)
			{
				ImGui::SetNextWindowFocus();
				hotbox.focus = false;
			}
			ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
			ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
			const bool open = ImGui::Begin("##Hotbox", nullptr, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoSavedSettings |
				ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoDocking | ImGuiWindowFlags_NoNav);
			ImGui::PopStyleVar(2);
			if (open)
			{
				ImDrawList *draw = ImGui::GetWindowDrawList();
				draw->AddRectFilled(viewport->Pos, ImVec2(viewport->Pos.x + viewport->Size.x, viewport->Pos.y + viewport->Size.y), IM_COL32(0, 0, 0, 120));	// Dims the editor while the hotbox is up

				const float dpi = ImGui::GetStyle().FontScaleDpi;
				const float row = ImGui::GetFrameHeight() + 10.0f * dpi;
				const float x = std::clamp(hotbox.center.x, viewport->Pos.x + 300.0f * dpi, viewport->Pos.x + viewport->Size.x - 300.0f * dpi);
				const float y = std::clamp(hotbox.center.y, viewport->Pos.y + row * 2.0f, viewport->Pos.y + viewport->Size.y - row * 3.0f);

				// Row 1: the menu bar's menus
				std::vector<std::pair<std::string, std::function<void()>>> mains;
				for (const auto &[name, drawer] : s.mainMenus)
					mains.emplace_back(name, drawer);
				MenuRow("##HotboxMain", x, y - row * 1.5f, mains);

				// Row 2: recent commands | the editor | facades
				std::vector<std::pair<std::string, std::function<void()>>> middle;
				middle.emplace_back("Recent Commands", [context]()
				{
					const std::vector<std::string> &recent = EditorCommands::GetRecent();
					if (recent.empty())
						ImGui::TextDisabled("Commands run from the menus show here");
					for (const std::string &id : recent)
						EditorCommands::MenuItem(id, context);
				});
				middle.emplace_back("Facades", []()
				{
					if (ImGui::BeginMenu(EditorUI::IconLabel(SEDX_ICON_BOOK, "Library").c_str()))
					{
						FacadeTool::DrawLibraryMenu();
						ImGui::EndMenu();
					}
					const std::vector<std::string> &recent = FacadeTool::GetRecent();
					if (!recent.empty())
						ImGui::SeparatorText("Recent");
					for (const std::string &resource : recent)
					{
						if (ImGui::MenuItem(resource.c_str(), nullptr, resource == FacadeTool::GetSettings().resource))
						{
							FacadeTool::SetResource(resource);
							EditorTools::SetTool(EditorTools::Tool::Facade);
						}
					}
				});
				MenuRow("##HotboxMiddle", x, y - row * 0.5f, middle, "  SCENERY EDITOR X  ");

				// Row 3: the selection's menus (the quad menu's panels)
				std::vector<std::pair<std::string, std::function<void()>>> context_menus;
				for (const Panel &panel : hotbox.panels)
				{
					if (panel.entries.empty())
						continue;
					context_menus.emplace_back(panel.title, [&panel, context]()
					{
						for (size_t e = 0; e < panel.entries.size(); ++e)
						{
							ImGui::PushID(static_cast<int>(e));
							DrawEntry(panel.entries[e], context);
							ImGui::PopID();
						}
					});
				}
				MenuRow("##HotboxContext", x, y + row * 0.5f, context_menus);

				const char *hint = "Hold Space: menus at the cursor  |  release to close";
				const ImVec2 hintSize = ImGui::CalcTextSize(hint);
				draw->AddText(ImVec2(x - hintSize.x * 0.5f, y + row * 1.7f), EditorTheme::ToU32(EditorTheme::Palette::TextLabel), hint);
			}
			ImGui::End();
		}
	}

	// -------------------------------------------------------

	void SetMainMenus(std::vector<std::pair<std::string, MenuDrawer>> menus)
	{
		S().mainMenus = std::move(menus);
	}

	void OpenQuadMenu(EditorContext *context, const ImVec2 &screen, const std::optional<Target> &target, const std::optional<Point3> &ground)
	{
		(void)context;
		State &s = S();
		HideHotbox();
		s.quad.requestOpen = true;
		s.quad.at = screen;
		s.quad.target = target;
		s.quad.ground = ground;
	}

	void Draw(EditorContext *context)
	{
		DrawQuadMenu(context);
		DrawHotbox(context);
	}

	bool IsOpen()
	{
		return IsQuadMenuOpen() || IsHotboxOpen();
	}

	bool IsQuadMenuOpen()
	{
		const State &s = S();
		return s.quad.open || s.quad.requestOpen;
	}

	bool IsHotboxOpen()
	{
		return S().hotbox.visible;
	}

	void PinHotbox(EditorContext *context, const ImVec2 &center)
	{
		State &s = S();
		HotboxState &hotbox = s.hotbox;
		hotbox.visible = true;
		hotbox.pinned = true;
		hotbox.focus = true;
		hotbox.center = center;
		const EditorSelection::Target *active = EditorSelection::GetActive();
		hotbox.panels = BuildPanels(context, active ? std::optional<Target>(*active) : std::nullopt, std::nullopt);
	}

	void CloseMenus()
	{
		State &s = S();
		s.quad.requestOpen = false;
		if (s.quad.open)
			s.quad.closeRequested = true;
		HideHotbox();
	}

	void RegisterCommands()
	{
		using EditorTools::Tool;
		for (int t = 0; t < static_cast<int>(Tool::Count); ++t)
		{
			const Tool tool = static_cast<Tool>(t);
			if (tool == Tool::Facade)
				continue;	// FacadeTool registers "facade.tool"
			std::string id = "tool." + std::string(EditorTools::GetToolName(tool));
			std::ranges::transform(id, id.begin(), [](const char c) { return static_cast<char>(std::tolower(static_cast<unsigned char>(c))); });
			EditorCommands::Register({ id, std::string(EditorTools::GetToolName(tool)) + " Tool", EditorTools::GetToolIcon(tool), EditorTools::GetToolKey(tool), "", nullptr,
				[tool](EditorContext *) { return EditorTools::GetTool() == tool; },
				[tool](EditorContext *) { EditorTools::SetTool(tool); } });
		}

		const auto hasSelection = [](EditorContext *) { return !EditorSelection::IsEmpty(); };
		EditorCommands::Register({ "edit.undo", "Undo", SEDX_ICON_UNDO, "Ctrl+Z", "", [](EditorContext *) { return EditorHistory::CanUndo(); }, nullptr,
			[](EditorContext *) { EditorHistory::Undo(); } });
		EditorCommands::Register({ "edit.redo", "Redo", SEDX_ICON_REPEAT, "Ctrl+Y", "", [](EditorContext *) { return EditorHistory::CanRedo(); }, nullptr,
			[](EditorContext *) { EditorHistory::Redo(); } });
		EditorCommands::Register({ "edit.delete", "Delete", SEDX_ICON_TRASH, "Del", "", hasSelection, nullptr,
			[](EditorContext *context) { EditorTools::DeleteSelection(context); } });
		EditorCommands::Register({ "edit.deselect", "Deselect", SEDX_ICON_TIMES, "Esc", "", hasSelection, nullptr,
			[](EditorContext *context) { EditorSelection::Clear(context); } });
		EditorCommands::Register({ "edit.frame", "Frame Selection", SEDX_ICON_CROSSHAIRS, "F", "", hasSelection, nullptr,
			[](EditorContext *) { EditorTools::FrameSelection(); } });
		EditorCommands::Register({ "file.save", "Save Project", SEDX_ICON_FLOPPY_O, "Ctrl+S", "", [](EditorContext *) { return EditorProjects::GetDocument() != nullptr; }, nullptr,
			[](EditorContext *) { EditorProjects::Save(); } });
		EditorCommands::Register({ "view.reset_camera", "Reset Camera", SEDX_ICON_HOME, "Home", "", [](EditorContext *) { return EditorCamera::Get() != nullptr; }, nullptr,
			[](EditorContext *) { EditorCamera::Reset(); } });
		EditorCommands::Register({ "nodes.reverse", "Reverse Direction", SEDX_ICON_EXCHANGE, "", "Lines, strings and facades run (and face) the other way",
			[](EditorContext *) { return NodeTool::HasShapes(); }, nullptr,
			[](EditorContext *) { NodeTool::ReverseSelectedShapes(); } });
	}
}

// -------------------------------------------------------
