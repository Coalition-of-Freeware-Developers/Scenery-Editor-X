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
#include "i18n_imgui.h"
#include "ui_impl.h"
#include "resources/fonts.h"
#include "source/imgui/imgui.h"
#include "source/imgui/backends/imgui_impl_sdl3.h"
#include <SceneryEditorX/core/localization.h>
#include <SceneryEditorX/i18n/font_coverage.h>
#include <SceneryEditorX/core/resource/resource_cache.h>
#include <SceneryEditorX/core/window/window.h>
#include <SceneryEditorX/filesystem/vfs.h>
#include <SceneryEditorX/logging/asserts.h>
#include <SceneryEditorX/renderer/font/font_awesome.h>
#include <cstdio>
#include <fstream>
#include <iterator>
#include <memory>
#include <set>

// -------------------------------------------------------

namespace SceneryEditorX
{
	namespace
	{
		// The dock layout and window placement, in ImGui's own format. Read and written through the VFS (ImGui's
		// file IO is off): atomic writes, and the same roaming settings folder on Windows, macOS and Linux.
		constexpr const char *INI_FILE = "user:/editor.ini";

		// Every UI font's default size; the renderer backend rasterizes other sizes on demand
		constexpr float FONT_SIZE = 15.0f;

		// The UI fonts a language font is merged into (each must cover the language, or headers show '?')
		constexpr const char *UI_FONTS[] = { "Default", "Light", "Medium", "SemiBold", "Bold" };

		void LoadIniSettings()
		{
			std::string text;
			if (VFS::Get().Read(INI_FILE, text))
				ImGui::LoadIniSettingsFromMemory(text.data(), text.size());
		}

		void SaveIniSettings()
		{
			size_t size = 0;
			const char *text = ImGui::SaveIniSettingsToMemory(&size);
			ImGui::GetIO().WantSaveIniSettings = false;

			std::string error;
			if (!VFS::Get().Write(INI_FILE, std::string_view(text, size), &error))
				SEDX_CORE_WARN_TAG("UILayer", "Could not save the window layout: {}", error);
		}
	}

	UILayer::UILayer(const bool enableViewports) : Layer("UILayer"), m_EnableViewports(enableViewports)
	{
	}

	void UILayer::OnAttach()
	{
		IMGUI_CHECKVERSION();
		ImGui::CreateContext();

		ImGuiIO &io = ImGui::GetIO();
		io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
		io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
		if (m_EnableViewports)
			io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;

		io.ConfigWindowsMoveFromTitleBarOnly = true;	// Dragging inside a panel (e.g. the viewport) must not move it
		io.ConfigViewportsNoTaskBarIcon = true;			// Floating panels don't clutter the OS taskbar

		// The layout comes from user:/editor.ini through the VFS; ImGui asks for saves with io.WantSaveIniSettings
		io.IniFilename = nullptr;
		LoadIniSettings();

		ImGui::StyleColorsDark();
		ImGui::GetStyle().FontScaleDpi = Window::GetDpiScale();

		LoadFonts();
		MergeLanguageFont();
		I18n::ApplyImGuiLocalization();

		SDL_Window *window = Window::GetWindow();
		SEDX_CORE_ASSERT(window != nullptr, "SDL_Window is null when initializing ImGui SDL backend");
		if (!ImGui_ImplSDL3_InitForVulkan(window))
		{
			SEDX_CORE_ERROR_TAG("UILayer", "Failed to initialize ImGui's SDL3 platform backend");
			ImGui::DestroyContext();
			return;
		}

		if (!UI::InitializeBackend())
		{
			ImGui_ImplSDL3_Shutdown();
			ImGui::DestroyContext();
			return;
		}

		Window::SetNativeEventHook([this](const SDL_Event &event)
		{
			if (m_AllowInput)
				ImGui_ImplSDL3_ProcessEvent(&event);
		});

		m_Initialized = true;
		SEDX_CORE_INFO_TAG("UILayer", "Dear ImGui {} initialized (docking: on, viewports: {})", IMGUI_VERSION, m_EnableViewports ? "on" : "off");
	}

	void UILayer::OnDetach()
	{
		if (!m_Initialized)
			return;

		Window::SetNativeEventHook(nullptr);
		SaveIniSettings();	// ImGui's own save at DestroyContext is off with its file IO
		UI::ShutdownBackend();
		ImGui_ImplSDL3_Shutdown();
		ImGui::DestroyContext();
		m_UIPanels.clear();
		m_Initialized = false;
	}

	void UILayer::Tick()
	{
		for (const Ref<UI::EditorPanel> &panel : m_UIPanels)
			panel->OnTick();
	}

	void UILayer::BeginRendering()
	{
		if (!m_Initialized)
			return;

		// A language chosen in Preferences takes effect here, between frames: its font may have to be merged first
		if (Localization::ApplyPendingChange())
		{
			MergeLanguageFont();
			I18n::ApplyImGuiLocalization();
		}

		UI::NewFrame();
		ImGui_ImplSDL3_NewFrame();
		UI::ApplyScriptedMouse();
		UI::ApplyScriptedKeys();
		ImGui::NewFrame();
	}

	void UILayer::EndRendering()
	{
		if (!m_Initialized)
			return;

		// Builds the draw data the render thread records into the swapchain pass this frame.
		ImGui::Render();

		// A moved panel or a docking change: ImGui asks (io.IniSavingRate after the last change) for the layout to be saved
		if (ImGui::GetIO().WantSaveIniSettings)
			SaveIniSettings();

		// Panels dragged out of the main window live in their own OS windows and swapchains. ImGui renders and
		// presents those here; the render thread is idle while the main thread runs, so the queue is ours.
		if (ImGui::GetIO().ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
		{
			ImGui::UpdatePlatformWindows();
			ImGui::RenderPlatformWindowsDefault();
		}
	}

	void UILayer::AllowInputEvents(const bool allowEvents)
	{
		m_AllowInput = allowEvents;
		if (!m_Initialized)
			return;

		ImGuiIO &io = ImGui::GetIO();
		if (allowEvents)
			io.ConfigFlags &= ~(ImGuiConfigFlags_NoMouse | ImGuiConfigFlags_NoKeyboard);
		else
			io.ConfigFlags |= ImGuiConfigFlags_NoMouse | ImGuiConfigFlags_NoKeyboard;
	}

	UIRenderer *UILayer::Get()
	{
		return m_UIRenderer.get();
	}

	void UILayer::LoadFonts()
	{
		const std::string fontDir = ResourceCache::GetResourceDirectory(ResourceDirectory::Fonts) + "/";
		if (!IO::FileSystem::Exists(fontDir + "opensans/OpenSans-Regular.ttf"))	// UTF-8, like ImGui's font file names
		{
			SEDX_CORE_WARN_TAG("UILayer", "UI fonts not found under '{}', falling back to the ImGui default font", fontDir);
			return;
		}

		constexpr float baseSize = FONT_SIZE;

		UI::Fonts::Add({ .FontName = "Default", .FilePath = fontDir + "opensans/OpenSans-Regular.ttf", .Size = baseSize }, true);

		static constexpr ImWchar s_IconRanges[] = { SEDX_ICON_MIN, SEDX_ICON_MAX, 0 };
		UI::Fonts::Add({ .FontName = "FontAwesome", .FilePath = fontDir + "fontawesome-webfont.ttf", .Size = baseSize, .GlyphRanges = s_IconRanges, .MergeWithLast = true });

		UI::Fonts::Add({ .FontName = "Light",    .FilePath = fontDir + "opensans/OpenSans-Light.ttf",    .Size = baseSize });
		UI::Fonts::Add({ .FontName = "Medium",   .FilePath = fontDir + "opensans/OpenSans-Medium.ttf",   .Size = baseSize });
		UI::Fonts::Add({ .FontName = "SemiBold", .FilePath = fontDir + "opensans/OpenSans-SemiBold.ttf", .Size = baseSize });
		UI::Fonts::Add({ .FontName = "Bold",     .FilePath = fontDir + "opensans/OpenSans-Bold.ttf",     .Size = baseSize });
	}

	void UILayer::MergeLanguageFont()
	{
		const I18n::LanguageInfo *language = I18n::GetLanguageInfo();
		if (!language || !language->NeedsCjkFont() || !UI::Fonts::Find("Default"))
			return;

		// Merged once per script and kept until the editor closes: switching from Japanese to Chinese later keeps the
		// Japanese font first, so shared Han characters keep its shapes until a restart. So the search below (file reads,
		// fc-match on Linux) runs at most once per script per session, when a language change is applied between frames,
		// never per frame
		static std::set<I18n::Script> merged;
		if (!merged.insert(language->script).second)
			return;

		// The first candidate that has every character of the language's sample (the user's own choice whatever it has);
		// else the one that lacks the fewest. A file name, or fontconfig's answer, doesn't promise the characters.
		const std::u32string sample = I18n::DecodeUtf8(language->Sample());
		std::optional<Localization::FontFile> font;
		std::unique_ptr<std::string> data;
		std::size_t fewestMissing = sample.size() + 1;
		for (const Localization::FontFile &candidate : Localization::FindLanguageFonts(*language))
		{
			std::ifstream file(VFS::FromUtf8(candidate.path), std::ios::binary);
			auto bytes = std::make_unique<std::string>(std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>());
			const std::optional<std::size_t> missing = I18n::CountMissingGlyphs(*bytes, candidate.face, sample);
			if (!missing)
			{
				SEDX_CORE_WARN_TAG("UILayer", "Skipping {} (face {}) for {}: not a font this can read", candidate.path, candidate.face, language->englishName);
				continue;
			}
			if (*missing > 0 && !candidate.chosen)
				SEDX_CORE_INFO_TAG("UILayer", "{} (face {}) lacks {} of the {} sample characters", candidate.path, candidate.face, *missing, language->englishName);
			if (*missing < fewestMissing)
			{
				fewestMissing = *missing;
				font = candidate;
				data = std::move(bytes);
			}
			if (*missing == 0 || candidate.chosen)
				break;
		}
		if (!font || (!font->chosen && !sample.empty() && fewestMissing == sample.size()))
		{
			SEDX_CORE_WARN_TAG("UILayer", "No font for {} found: install the {} language fonts of the operating system, or set ui.language_font "
				"in settings.cfg to a font file. Its text shows as '?' until then", language->englishName, language->englishName);
			return;
		}
		if (fewestMissing > 0)
			SEDX_CORE_WARN_TAG("UILayer", "Using {} (face {}) for {}, which lacks {} of its sample characters", font->path, font->face,
				language->englishName, fewestMissing);

		// Read once and shared by the five UI fonts; the atlas doesn't own it, so it stays for the whole session
		static std::vector<std::unique_ptr<std::string>> fontData;

		ImGuiIO &io = ImGui::GetIO();
		for (const char *name : UI_FONTS)
		{
			ImFont *target = UI::Fonts::Find(name);
			if (!target)
				continue;
			ImFontConfig config;
			config.MergeMode = true;
			config.DstFont = target;
			config.FontNo = static_cast<ImU32>(font->face);
			config.FontDataOwnedByAtlas = false;
			std::snprintf(config.Name, sizeof(config.Name), "%.*s (%s)", static_cast<int>(language->englishName.size()), language->englishName.data(), name);
			io.Fonts->AddFontFromMemoryTTF(data->data(), static_cast<int>(data->size()), FONT_SIZE, &config);
		}

		SEDX_CORE_INFO_TAG("UILayer", "Merged {} (face {}, {:.1f} MB) into the UI fonts for {}", font->path, font->face,
			static_cast<double>(data->size()) / (1024.0 * 1024.0), language->englishName);
		fontData.push_back(std::move(data));
	}

} // namespace SceneryEditorX

// -------------------------------------------------------
