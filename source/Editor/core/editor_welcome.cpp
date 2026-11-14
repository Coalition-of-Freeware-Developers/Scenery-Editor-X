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
 * editor_welcome.cpp
 * -------------------------------------------------------
 * Created: 21/10/2026
 * -------------------------------------------------------
 */
#include "editor_welcome.h"
#include <Editor/core/editor_theme.h>
#include <Editor/core/editor_tools.h>
#include <Editor/core/editor_ui.h>
#include <Editor/core/viewport_camera.h>
#include <Editor/settings/editor_preferences.h>
#include <SceneryEditorX/core/application/application.h>
#include <SceneryEditorX/core/localization.h>
#include <SceneryEditorX/i18n/i18n.h>
#include <SceneryEditorX/i18n/pseudo.h>
#include <SceneryEditorX/i18n/tr.h>
#include <SceneryEditorX/renderer/ui/i18n_imgui.h>
#include <SceneryEditorX/renderer/font/font_awesome.h>
#include <SceneryEditorX/renderer/ui/resources/fonts.h>

// -------------------------------------------------------

namespace SceneryEditorX::EditorWelcome
{
	namespace
	{
		constexpr const char *WELCOME_MODAL = "Welcome to Scenery Editor X";

		struct State
		{
			bool showWelcome = false;			// Open the modal next frame
			bool preferencesOpen = false;
			std::optional<PreferencesPage> selectPage;	// Selected once, when the window is opened on a page
			NavigationKeymap highlighted = NavigationKeymap::Blender;

			// Languages for the Interface page, read when the page opens and after a language change
			std::vector<I18n::LanguageChoice> languages;
			std::uint64_t languagesGeneration = 0;
			std::string systemLanguage;	// What "system" resolves to (asked once per refresh, not every frame)
		};

		State &S()
		{
			static State state;
			return state;
		}

		ImVec4 Color(const ImVec4 &srgb)
		{
			return EditorTheme::ToStyle(srgb);
		}

		/* @brief The first character of UTF-8 text. */
		unsigned FirstCodePoint(const std::string_view text)
		{
			if (text.empty())
				return 0;
			const unsigned char lead = static_cast<unsigned char>(text[0]);
			if (lead < 0x80 || text.size() < 2)
				return lead;
			if ((lead & 0xE0) == 0xC0)
				return ((lead & 0x1Fu) << 6) | (static_cast<unsigned char>(text[1]) & 0x3Fu);
			if ((lead & 0xF0) == 0xE0 && text.size() >= 3)
				return ((lead & 0x0Fu) << 12) | ((static_cast<unsigned char>(text[1]) & 0x3Fu) << 6) | (static_cast<unsigned char>(text[2]) & 0x3Fu);
			return 0xFFFD;
		}

		/* @brief A language's own name when the UI font can draw it ("Deutsch", "\u65E5\u672C\u8A9E"), else its English name. */
		std::string LanguageName(const I18n::LanguageInfo *info, const std::string_view fallback)
		{
			if (!info)
				return std::string(fallback);
			ImFont *font = UI::Fonts::Find("Default");
			const unsigned first = FirstCodePoint(info->NativeName());
			const bool drawable = first < 0x80 || (font && first <= 0xFFFF && font->IsGlyphInFont(static_cast<ImWchar>(first)));
			if (!drawable)
				return std::string(info->englishName);
			if (info->NativeName() == info->englishName)
				return std::string(info->englishName);
			return std::string(info->NativeName()) + " (" + std::string(info->englishName) + ")";
		}

		/* @brief Interface page: the language and how much of each translation there is. */
		void DrawInterfacePage()
		{
			State &s = S();
			if (s.languages.empty() || s.languagesGeneration != I18n::GetGeneration())
			{
				s.languages = I18n::GetLanguageChoices();
				s.languagesGeneration = I18n::GetGeneration();
				s.systemLanguage = Localization::ResolveSystemLanguage();
			}

			const std::string setting = Localization::GetLanguageSetting();
			const bool showIncomplete = Localization::GetShowIncomplete();
			const std::string &system = s.systemLanguage;
			const std::string systemName = LanguageName(I18n::FindLanguage(system), system);

			struct Option
			{
				std::string setting;
				std::string label;	// Shown, then "###" and the setting (a stable ID)
			};
			std::vector<Option> options;
			options.push_back({ std::string(Localization::SYSTEM), TrF("System default: {}", systemName) });
			options.push_back({ std::string(I18n::SOURCE_LANGUAGE), "English" });
			for (const I18n::LanguageChoice &language : s.languages)
			{
				if (language.failed)
					continue;
				const int percent = static_cast<int>(language.stats.Completion() * 100.0f + 0.5f);
				const bool listed = showIncomplete || language.stats.Completion() >= Localization::LISTED_COMPLETION || language.tag == setting;
				if (!listed)
					continue;
				// TRANSLATORS: A language in the list of Preferences > Interface: {0} is its name, {1} how much of it is translated
				const std::string name = LanguageName(language.info, language.englishName);
				std::string label = language.user ? TrF("{0} - {1}% translated (your file)", name, percent) : TrF("{0} - {1}% translated", name, percent);
				options.push_back({ language.tag, std::move(label) });
			}
			const std::string pseudoLabel = Tr("Pseudo-locale (finds untranslated and cut-off text)");
			if (showIncomplete)
				options.push_back({ std::string(I18n::PSEUDO_LANGUAGE), pseudoLabel });

			std::string preview = setting == I18n::PSEUDO_LANGUAGE ? pseudoLabel : setting;
			for (const Option &option : options)
				if (option.setting == setting)
					preview = option.label;

			// The label column fits its longest label in the active language (German is about a third longer than English)
			const float dpi = ImGui::GetStyle().FontScaleDpi;
			const float labels = std::max(ImGui::CalcTextSize(Tr("Language")).x, ImGui::CalcTextSize(Tr("Show incomplete translations")).x);
			if (EditorUI::BeginPropertyGrid("##InterfaceOptions", std::max(190.0f, labels / dpi + 16.0f)))
			{
				EditorUI::PropertyLabel("Language");
				if (ImGui::BeginCombo("##language", preview.c_str()))
				{
					for (const Option &option : options)
					{
						const std::string label = option.label + "###" + option.setting;
						if (ImGui::Selectable(label.c_str(), option.setting == setting))
							Localization::RequestLanguage(option.setting);
					}
					ImGui::EndCombo();
				}
				EditorUI::ItemTooltip("The language of menus, panels and messages. It changes at once; scenery data (airport names, "
					"file names, X-Plane art and datarefs) stays as it is");

				EditorUI::PropertyLabel("Show incomplete translations");
				bool show = showIncomplete;
				if (ImGui::Checkbox("##showIncomplete", &show))
					Localization::SetShowIncomplete(show);
				EditorUI::ItemTooltip("Lists languages that are less than 40% translated, and the pseudo-locale. Text that isn't "
					"translated yet stays in English");
				EditorUI::EndPropertyGrid();
			}

			ImGui::Spacing();
			ImGui::SeparatorText(Tr("Translations"));
			ImGui::PushTextWrapPos(0.0f);
			ImGui::TextUnformatted(Tr("Scenery Editor X is translated by volunteers. To help, or to fix a translation, see "
				"docs/localization-translator-guide.md. To test your own copy of a language file, put it in the locale folder "
				"of the editor's settings folder and use Reload Translations."));
			ImGui::PopTextWrapPos();
			if (ImGui::Button(TrL("Reload Translations")))
				Localization::RequestReload();
		}

		/* @brief A keymap's binding table. */
		void DrawBindings(const NavigationKeymap keymap, const float height)
		{
			constexpr ImGuiTableFlags flags = ImGuiTableFlags_RowBg | ImGuiTableFlags_BordersInnerV | ImGuiTableFlags_ScrollY | ImGuiTableFlags_SizingStretchProp;
			if (!ImGui::BeginTable("##Bindings", 2, flags, ImVec2(0.0f, height)))
				return;
			ImGui::TableSetupScrollFreeze(0, 1);
			ImGui::TableSetupColumn("Action", ImGuiTableColumnFlags_WidthStretch, 0.4f);
			ImGui::TableSetupColumn("Mouse / keys", ImGuiTableColumnFlags_WidthStretch, 0.6f);
			ImGui::TableHeadersRow();
			for (const ViewportCamera::Binding &binding : ViewportCamera::GetBindings(keymap))
			{
				ImGui::TableNextRow();
				ImGui::TableNextColumn();
				ImGui::TextUnformatted(binding.action.c_str());
				ImGui::TableNextColumn();
				ImGui::TextColored(Color(EditorTheme::Palette::TextLabel), "%s", binding.keys.c_str());
			}
			ImGui::EndTable();
		}

		/* @brief The keymaps as cards side by side; a click highlights one. True when one was double-clicked. */
		bool DrawKeymapCards(NavigationKeymap &highlighted)
		{
			const float dpi = ImGui::GetStyle().FontScaleDpi;
			const float spacing = ImGui::GetStyle().ItemSpacing.x;
			const float width = (ImGui::GetContentRegionAvail().x - spacing * 2.0f) / 3.0f;
			const float height = 96.0f * dpi;
			bool chosen = false;
			for (int k = 0; k < static_cast<int>(NavigationKeymap::Count); ++k)
			{
				const NavigationKeymap keymap = static_cast<NavigationKeymap>(k);
				if (k > 0)
					ImGui::SameLine();
				ImGui::PushID(k);
				const ImVec2 min = ImGui::GetCursorScreenPos();
				const ImVec2 max(min.x + width, min.y + height);
				if (ImGui::InvisibleButton("##Card", ImVec2(width, height)))
					highlighted = keymap;
				if (ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left))
				{
					highlighted = keymap;
					chosen = true;
				}
				const bool selected = highlighted == keymap;
				const bool hovered = ImGui::IsItemHovered();

				ImDrawList *drawList = ImGui::GetWindowDrawList();
				drawList->AddRectFilled(min, max, EditorTheme::ToU32(hovered ? EditorTheme::Palette::FrameHovered : EditorTheme::Palette::Frame), 4.0f * dpi);
				if (selected)
					drawList->AddRect(min, max, EditorTheme::ToU32(EditorTheme::Palette::Accent), 4.0f * dpi, 0, 2.0f * dpi);

				const ImVec2 padding(10.0f * dpi, 8.0f * dpi);
				UI::Fonts::PushFont("Bold");
				drawList->AddText(ImVec2(min.x + padding.x, min.y + padding.y), EditorTheme::ToU32(selected ? EditorTheme::Palette::AccentHovered : EditorTheme::Palette::Text),
					GetNavigationKeymapName(keymap));
				UI::Fonts::PopFont();
				const float lineHeight = ImGui::GetTextLineHeightWithSpacing();
				ImGui::PushClipRect(min, max, true);
				const ImVec2 textPos(min.x + padding.x, min.y + padding.y + lineHeight + 4.0f * dpi);
				drawList->AddText(nullptr, 0.0f, textPos, EditorTheme::ToU32(EditorTheme::Palette::TextLabel), ViewportCamera::GetKeymapSummary(keymap), nullptr, width - padding.x * 2.0f);
				ImGui::PopClipRect();
				ImGui::PopID();
			}
			return chosen;
		}

		void Choose(const NavigationKeymap keymap)
		{
			EditorPreferences &preferences = EditorPreferences::Get();
			preferences.keymap = keymap;
			preferences.keymapChosen = true;
			EditorPreferences::Save();
			EDITOR_INFO_TAG("Preferences", "Viewport navigation: {} keymap", GetNavigationKeymapName(keymap));
		}

		void DrawWelcome()
		{
			State &s = S();
			if (s.showWelcome)
			{
				s.highlighted = EditorPreferences::Get().keymap;
				ImGui::OpenPopup(WELCOME_MODAL);
				s.showWelcome = false;
			}

			const float dpi = ImGui::GetStyle().FontScaleDpi;
			ImGui::SetNextWindowPos(ImGui::GetMainViewport()->GetCenter(), ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
			ImGui::SetNextWindowSize(ImVec2(720.0f * dpi, 0.0f), ImGuiCond_Appearing);
			if (!ImGui::BeginPopupModal(WELCOME_MODAL, nullptr, ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_AlwaysAutoResize))
				return;

			ImGui::PushTextWrapPos(ImGui::GetCursorPosX() + 700.0f * dpi);
			ImGui::TextUnformatted("How should the viewport camera follow your mouse? Pick the 3D package your hands already know.");
			ImGui::TextColored(Color(EditorTheme::Palette::TextLabel), "Numpad views work in all of them: 7 top, 1 front, 3 side, 5 orthographic, . frames the selection. "
				"Change this any time in Edit > Preferences.");
			ImGui::PopTextWrapPos();
			ImGui::Spacing();

			const bool doubleClicked = DrawKeymapCards(s.highlighted);
			ImGui::Spacing();
			ImGui::SeparatorText(GetNavigationKeymapName(s.highlighted));
			DrawBindings(s.highlighted, 230.0f * dpi);
			ImGui::Spacing();

			const std::string start = std::string("Start with ") + GetNavigationKeymapName(s.highlighted);
			if (ImGui::Button(EditorUI::IconLabel(SEDX_ICON_CHECK, start.c_str()).c_str(), ImVec2(260.0f * dpi, 0.0f)) || doubleClicked ||
				ImGui::IsKeyPressed(ImGuiKey_Enter, false) || ImGui::IsKeyPressed(ImGuiKey_KeypadEnter, false))
			{
				Choose(s.highlighted);
				ImGui::CloseCurrentPopup();
			}
			ImGui::EndPopup();
		}

		void DrawPreferences()
		{
			State &s = S();
			if (!s.preferencesOpen)
				return;

			const float dpi = ImGui::GetStyle().FontScaleDpi;
			ImGui::SetNextWindowPos(ImGui::GetMainViewport()->GetCenter(), ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
			ImGui::SetNextWindowSize(ImVec2(680.0f * dpi, 560.0f * dpi), ImGuiCond_FirstUseEver);
			if (!ImGui::Begin(TrL("Preferences###EditorPreferences"), &s.preferencesOpen, ImGuiWindowFlags_NoDocking | ImGuiWindowFlags_NoCollapse))
			{
				ImGui::End();
				return;
			}

			EditorPreferences &preferences = EditorPreferences::Get();
			bool changed = false;
			const auto pageFlags = [&s](const PreferencesPage page)
			{
				return s.selectPage == page ? ImGuiTabItemFlags_SetSelected : ImGuiTabItemFlags_None;
			};
			if (ImGui::BeginTabBar("##PreferenceTabs"))
			{
				if (ImGui::BeginTabItem(TrL("Navigation"), nullptr, pageFlags(PreferencesPage::Navigation)))
				{
					ImGui::Spacing();
					NavigationKeymap keymap = preferences.keymap;
					if (DrawKeymapCards(keymap) || keymap != preferences.keymap)
					{
						preferences.keymap = keymap;
						preferences.keymapChosen = true;
						changed = true;
					}
					ImGui::Spacing();
					if (EditorUI::BeginPropertyGrid("##NavigationOptions", 190.0f))
					{
						EditorUI::PropertyLabel("Invert orbit horizontally");
						changed |= ImGui::Checkbox("##invertX", &preferences.invertOrbitX);
						EditorUI::ItemTooltip("Off: the scene turns with the mouse (Blender, 3ds Max, Maya). On: the camera moves with it");
						EditorUI::PropertyLabel("Invert orbit vertically");
						changed |= ImGui::Checkbox("##invertY", &preferences.invertOrbitY);
						EditorUI::PropertyLabel("Invert look up / down");
						changed |= ImGui::Checkbox("##invertLook", &preferences.invertLookY);
						EditorUI::ItemTooltip("Right-button look: on, moving the mouse up looks down (flight-stick style)");
						EditorUI::PropertyLabel("Smooth view changes");
						changed |= ImGui::Checkbox("##smooth", &preferences.smoothViews);
						EditorUI::ItemTooltip("Glide to top / front / side views and framed selections instead of jumping");
						EditorUI::PropertyLabel("Orbit speed");
						ImGui::SetNextItemWidth(-FLT_MIN);
						changed |= ImGui::SliderFloat("##orbitSpeed", &preferences.orbitSpeed, 0.2f, 3.0f, "%.2fx");
						EditorUI::PropertyLabel("Zoom speed");
						ImGui::SetNextItemWidth(-FLT_MIN);
						changed |= ImGui::SliderFloat("##zoomSpeed", &preferences.zoomSpeed, 0.2f, 3.0f, "%.2fx");
						EditorUI::EndPropertyGrid();
					}
					ImGui::SeparatorText("Controls");
					DrawBindings(preferences.keymap, ImGui::GetContentRegionAvail().y);
					ImGui::EndTabItem();
				}

				if (ImGui::BeginTabItem(TrL("Tools"), nullptr, pageFlags(PreferencesPage::Tools)))
				{
					ImGui::Spacing();
					constexpr ImGuiTableFlags flags = ImGuiTableFlags_RowBg | ImGuiTableFlags_BordersInnerV | ImGuiTableFlags_ScrollY | ImGuiTableFlags_SizingStretchProp;
					if (ImGui::BeginTable("##ToolKeys", 2, flags))
					{
						ImGui::TableSetupScrollFreeze(0, 1);
						ImGui::TableSetupColumn(Tr("Action"), ImGuiTableColumnFlags_WidthStretch, 0.4f);
						ImGui::TableSetupColumn(Tr("Keys"), ImGuiTableColumnFlags_WidthStretch, 0.6f);
						ImGui::TableHeadersRow();
						const auto row = [](const char *action, const char *keys)
						{
							ImGui::TableNextRow();
							ImGui::TableNextColumn();
							ImGui::TextUnformatted(action);
							ImGui::TableNextColumn();
							ImGui::TextColored(Color(EditorTheme::Palette::TextLabel), "%s", keys);
						};
						for (int t = 0; t < static_cast<int>(EditorTools::Tool::Count); ++t)
						{
							const EditorTools::Tool tool = static_cast<EditorTools::Tool>(t);
							row(EditorTools::GetToolName(tool), EditorTools::GetToolKey(tool));
						}
						row("Select / add / remove", "Click, Shift+click, Ctrl+click, drag a box");
						row("Gizmo local / world axes", "X");
						row("Duplicate", "Ctrl+D");
						row("Delete", "Del");
						row("Deselect", "Esc");
						row("Quad menu", "Right-click");
						row("Hotbox", "Hold Space");
						row("Undo / redo", "Ctrl+Z  /  Ctrl+Y");
						row("New / open / save project", "Ctrl+N  /  Ctrl+O  /  Ctrl+S");
						row("Export scenery pack", "Ctrl+E");
						ImGui::EndTable();
					}
					ImGui::EndTabItem();
				}

				if (ImGui::BeginTabItem(TrL("Interface"), nullptr, pageFlags(PreferencesPage::Interface)))
				{
					ImGui::Spacing();
					DrawInterfacePage();
					ImGui::EndTabItem();
				}
				ImGui::EndTabBar();
			}
			s.selectPage.reset();
			ImGui::End();

			if (changed)
				EditorPreferences::Save();
		}
	}

	void Init()
	{
		// Scripted runs (self-tests, captures, exports) pass flags and must not stop at a modal; --welcome shows it anyway
		bool flags = false, force = false;
		if (const PlatformContext *platform = Application::Get().GetPlatformContext())
		{
			const std::vector<std::string> &args = platform->GetCommandLineArgs();
			for (size_t i = 0; i < args.size(); ++i)
			{
				const std::string &arg = args[i];
				force |= arg == "--welcome";
				flags |= arg.starts_with("--");
				// --preferences [navigation|tools|interface]: opens Preferences on a page (screenshots, translator context)
				if (arg == "--preferences")
				{
					const std::string page = i + 1 < args.size() && !args[i + 1].starts_with("--") ? args[++i] : std::string();
					OpenPreferences(page == "interface" ? PreferencesPage::Interface : page == "tools" ? PreferencesPage::Tools : PreferencesPage::Navigation);
				}
			}
		}
		S().showWelcome = force || (!flags && !EditorPreferences::Get().keymapChosen);
	}

	void Draw()
	{
		DrawWelcome();
		DrawPreferences();
	}

	void OpenPreferences(const PreferencesPage page)
	{
		S().preferencesOpen = true;
		S().selectPage = page;
		ImGui::SetWindowFocus("Preferences###EditorPreferences");
	}

	void OpenWelcome()
	{
		S().showWelcome = true;
	}
}

// -------------------------------------------------------
