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
 * localization.cpp
 * -------------------------------------------------------
 * Created: 13/11/2026
 * -------------------------------------------------------
 */
#include "localization.h"
#include <SceneryEditorX/core/resource/resource_cache.h>
#include <SceneryEditorX/filesystem/vfs.h>
#include <SceneryEditorX/i18n/i18n.h>
#include <SceneryEditorX/i18n/pseudo.h>
#include <SceneryEditorX/logging/logging.hpp>
#include <SceneryEditorX/settings/settings.h>
#include <SDL3/SDL_locale.h>
#include <SDL3/SDL_stdinc.h>
#include <algorithm>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <mutex>
#include <sstream>

#ifdef _WIN32
	#include <ShlObj.h>
#endif

// -------------------------------------------------------

namespace SceneryEditorX::Localization
{
	namespace
	{
		constexpr const char *LOG_TAG = "I18n";
		constexpr const char *KEY_LANGUAGE = "ui.language";
		constexpr const char *KEY_SHOW_INCOMPLETE = "ui.show_incomplete_translations";
		constexpr const char *KEY_FONT = "ui.language_font";
		constexpr const char *KEY_FONT_FACE = "ui.language_font_face";
		constexpr const char *LEGACY_LANGUAGE = "english";	// Written by older builds, never read by them

		struct State
		{
			Settings *settings = nullptr;
			std::string setting = std::string(SYSTEM);	// ui.language (or --language for this run)
			std::optional<std::string> pending;
			bool reload = false;
			bool showIncomplete = false;
		};

		State &S()
		{
			static State state;
			return state;
		}

		std::optional<std::string> ReadFile(const std::filesystem::path &path)
		{
			std::ifstream file(path, std::ios::binary);
			if (!file)
				return std::nullopt;
			return std::string(std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>());
		}

		/* @brief The <tag>.po files of a folder (none when it doesn't exist). */
		void AddFolder(std::vector<I18n::CatalogFile> &files, const std::filesystem::path &folder, const bool user)
		{
			std::error_code error;
			if (!std::filesystem::is_directory(folder, error))
				return;
			for (const auto &entry : std::filesystem::directory_iterator(folder, error))
			{
				const std::filesystem::path &path = entry.path();
				if (!entry.is_regular_file(error) || path.extension() != ".po")
					continue;
				const std::string tag = VFS::ToUtf8(path.stem());
				files.push_back({ tag, VFS::ToUtf8(path), [path] { return ReadFile(path); }, user });
			}
		}

		void RegisterCatalogs()
		{
			std::vector<I18n::CatalogFile> files;
			AddFolder(files, VFS::FromUtf8(ResourceCache::GetResourceDirectory()) / "locale", false);
			// A translator's copies, which replace the bundled ones of the same language
			VFS::Get().CreateFolders("user:/locale");
			if (const std::optional<std::filesystem::path> user = VFS::Get().Resolve("user:/locale"))
				AddFolder(files, *user, true);
			I18n::SetCatalogFiles(std::move(files));
		}

		std::string Resolve(const std::string &setting)
		{
			if (setting.empty() || setting == SYSTEM)
				return ResolveSystemLanguage();
			return setting;
		}

		bool Activate(const std::string &setting)
		{
			const std::string tag = Resolve(setting);
			if (I18n::SetLanguage(tag))
				return true;
			SEDX_CORE_WARN_TAG(LOG_TAG, "Language '{}' isn't available: using English", tag);
			I18n::SetLanguage(I18n::SOURCE_LANGUAGE);
			return false;
		}

		std::string Join(const std::vector<std::string> &items)
		{
			std::string text;
			for (const std::string &item : items)
				text += (text.empty() ? "" : ", ") + item;
			return text.empty() ? "none" : text;
		}

		bool Exists(const std::string &utf8)
		{
			std::error_code error;
			return std::filesystem::is_regular_file(VFS::FromUtf8(utf8), error);
		}

#if !defined(_WIN32) && !defined(__APPLE__)
		/**
		 * @brief fontconfig's choice for a language ("file\nindex" from fc-match), or nullopt. fc-match always names some
		 * font, often one without the language's characters: the caller checks its glyph coverage before using it.
		 * Without fontconfig (minimal installs, sandboxes such as Flatpak) the failure is logged and the known Noto
		 * paths are tried.
		 */
		std::optional<Localization::FontFile> FontConfigMatch(const std::string_view language)
		{
			const std::string command = "fc-match -f '%{file}\\n%{index}' 'sans-serif:lang=" + std::string(language) + "' 2>/dev/null";
			FILE *pipe = popen(command.c_str(), "r");
			if (!pipe)
			{
				SEDX_CORE_WARN_TAG(LOG_TAG, "Can't run fc-match to find a font for '{}' (is fontconfig installed?)", language);
				return std::nullopt;
			}
			std::string output;
			char buffer[512];
			while (std::fgets(buffer, sizeof(buffer), pipe))
				output += buffer;
			const int status = pclose(pipe);
			const std::size_t newline = output.find('\n');
			if (status != 0 || newline == std::string::npos)
			{
				SEDX_CORE_WARN_TAG(LOG_TAG, "fc-match found no font for '{}' (exit status {}; is fontconfig installed?)", language, status);
				return std::nullopt;
			}
			Localization::FontFile font { output.substr(0, newline), std::atoi(output.c_str() + newline + 1) };
			if (font.path.empty() || !Exists(font.path))
			{
				SEDX_CORE_WARN_TAG(LOG_TAG, "fc-match named '{}' for '{}', which doesn't exist", font.path, language);
				return std::nullopt;
			}
			return font;
		}
#endif
	}

	// -------------------------------------------------------

	void Init(Settings &settings, const std::vector<std::string> &args)
	{
		State &s = S();
		s.settings = &settings;

		I18n::SetLogSink([](const I18n::LogLevel level, const std::string_view message)
		{
			switch (level)
			{
				case I18n::LogLevel::Info: SEDX_CORE_INFO_TAG(LOG_TAG, "{}", message); break;
				case I18n::LogLevel::Warning: SEDX_CORE_WARN_TAG(LOG_TAG, "{}", message); break;
				case I18n::LogLevel::Error: SEDX_CORE_ERROR_TAG(LOG_TAG, "{}", message); break;
			}
		});

		// ui.language: older builds wrote "english" and never read it, so it means "follow the system"
		std::string setting = settings.GetStringOption(KEY_LANGUAGE, SYSTEM);
		if (setting.empty() || setting == LEGACY_LANGUAGE)
		{
			setting = std::string(SYSTEM);
			settings.AddStringOption(KEY_LANGUAGE, setting);
			settings.Save();
		}
		if (!settings.HasOption(KEY_SHOW_INCOMPLETE))
			settings.AddBoolOption(KEY_SHOW_INCOMPLETE, false);
		s.showIncomplete = settings.GetBoolOption(KEY_SHOW_INCOMPLETE, false);
		s.setting = setting;

		for (std::size_t i = 0; i < args.size(); ++i)
		{
			if (args[i] == "--language" && i + 1 < args.size())
				s.setting = args[++i];	// This run only: not saved
			else if (args[i] == "--i18n-log-missing")
				I18n::SetRecordMissing(true);
		}

		RegisterCatalogs();
		Activate(s.setting);
		SEDX_CORE_INFO_TAG(LOG_TAG, "Language {} (setting '{}'; system languages: {}; catalogs: {})", I18n::GetLanguage(), s.setting,
			Join(GetPreferredUiLanguages()), Join(I18n::GetAvailableTags()));
	}

	void Shutdown()
	{
		if (!I18n::IsRecordingMissing())
			return;
		const std::vector<I18n::MissingEntry> missing = I18n::GetMissing();
		const std::string path = "logs:/missing-translations-" + I18n::GetLanguage() + ".po";
		std::string error;
		if (VFS::Get().Write(path, I18n::MissingAsPo(), &error))
			SEDX_CORE_INFO_TAG(LOG_TAG, "{} strings had no {} translation: listed in {}", missing.size(), I18n::GetLanguage(), path);
		else
			SEDX_CORE_WARN_TAG(LOG_TAG, "Can't write {}: {}", path, error);
	}

	std::string GetLanguageSetting()
	{
		return S().setting;
	}

	std::vector<std::string> GetPreferredUiLanguages()
	{
		// SDL asks the system: GetUserPreferredUILanguages on Windows, CFLocaleCopyPreferredLanguages on macOS,
		// LANGUAGE / LC_ALL / LC_MESSAGES / LANG on Linux
		std::vector<std::string> languages;
		int count = 0;
		SDL_Locale **locales = SDL_GetPreferredLocales(&count);
		if (!locales)
			return languages;
		for (int i = 0; i < count && locales[i]; ++i)
		{
			if (!locales[i]->language)
				continue;
			std::string tag = locales[i]->language;
			if (locales[i]->country && locales[i]->country[0])
				tag += std::string("-") + locales[i]->country;
			languages.push_back(std::move(tag));
		}
		SDL_free(locales);
		return languages;
	}

	std::string ResolveSystemLanguage()
	{
		return I18n::MatchLanguage(GetPreferredUiLanguages(), I18n::GetAvailableTags());
	}

	void RequestLanguage(const std::string_view setting)
	{
		State &s = S();
		s.pending = std::string(setting);
		if (s.settings && setting != I18n::PSEUDO_LANGUAGE)	// The pseudo-locale is a test, not a choice to keep
		{
			s.settings->AddStringOption(KEY_LANGUAGE, setting);
			s.settings->Save();
		}
	}

	void RequestReload()
	{
		S().reload = true;
	}

	bool ApplyPendingChange()
	{
		State &s = S();
		if (!s.pending && !s.reload)
			return false;

		const std::string before = I18n::GetLanguage();
		const std::uint64_t generation = I18n::GetGeneration();
		if (s.reload)
		{
			s.reload = false;
			RegisterCatalogs();	// New files in user:/locale too
			if (!s.pending)
				s.pending = s.setting;
		}
		s.setting = *s.pending;
		s.pending.reset();
		Activate(s.setting);
		if (I18n::GetLanguage() != before)
			SEDX_CORE_INFO_TAG(LOG_TAG, "Language changed from {} to {}", before, I18n::GetLanguage());
		return I18n::GetGeneration() != generation;
	}

	bool GetShowIncomplete()
	{
		return S().showIncomplete;
	}

	void SetShowIncomplete(const bool show)
	{
		State &s = S();
		s.showIncomplete = show;
		if (s.settings)
		{
			s.settings->AddBoolOption(KEY_SHOW_INCOMPLETE, show);
			s.settings->Save();
		}
	}

	std::vector<FontFile> FindLanguageFonts(const I18n::LanguageInfo &language)
	{
		std::vector<FontFile> found;
		if (!language.NeedsCjkFont())
			return found;

		// A font the user chose (settings.cfg)
		if (State &s = S(); s.settings)
		{
			const std::string chosen = s.settings->GetStringOption(KEY_FONT);
			if (!chosen.empty())
			{
				if (Exists(chosen))
					found.push_back({ chosen, s.settings->GetIntOption(KEY_FONT_FACE, 0), true });
				else
					SEDX_CORE_WARN_TAG(LOG_TAG, "{} '{}' doesn't exist: looking for a system font", KEY_FONT, chosen);
			}
		}

		std::vector<FontFile> candidates;
		const I18n::Script script = language.script;
#if defined(_WIN32)
		// The regional UI fonts every Windows 10 / 11 has (Meiryo is an optional feature, so it only comes second)
		std::filesystem::path fonts = "C:/Windows/Fonts";
		PWSTR known = nullptr;
		if (SUCCEEDED(SHGetKnownFolderPath(FOLDERID_Fonts, 0, nullptr, &known)) && known)
			fonts = known;
		CoTaskMemFree(known);
		const auto add = [&](const char *file, const int face) { candidates.push_back({ VFS::ToUtf8(fonts / file), face }); };
		switch (script)
		{
			case I18n::Script::HanSimplified: add("msyh.ttc", 0); add("simsun.ttc", 0); break;
			case I18n::Script::HanTraditional: add("msjh.ttc", 0); add("mingliu.ttc", 0); break;
			case I18n::Script::Japanese: add("YuGothM.ttc", 0); add("meiryo.ttc", 0); add("msgothic.ttc", 0); break;
			case I18n::Script::Korean: add("malgun.ttf", 0); add("gulim.ttc", 0); break;
			default: break;
		}
#elif defined(__APPLE__)
		const auto add = [&](const std::u8string_view path, const int face)
		{
			candidates.push_back({ std::string(reinterpret_cast<const char *>(path.data()), path.size()), face });
		};
		switch (script)
		{
			case I18n::Script::HanSimplified:
				add(u8"/System/Library/Fonts/PingFang.ttc", 0);
				add(u8"/System/Library/Fonts/Hiragino Sans GB.ttc", 0);
				add(u8"/System/Library/Fonts/STHeiti Light.ttc", 1);
				break;
			case I18n::Script::HanTraditional:
				add(u8"/System/Library/Fonts/PingFang.ttc", 1);
				add(u8"/System/Library/Fonts/STHeiti Light.ttc", 0);
				break;
			case I18n::Script::Japanese:
				// Hiragino Sans (Hiragino Kaku Gothic) in three weights, then Hiragino Maru Gothic ProN. Not Hiragino Sans GB: that is
				// a Chinese font. The names are Japanese on disk.
				add(u8"/System/Library/Fonts/\u30D2\u30E9\u30AE\u30CE\u89D2\u30B4\u30B7\u30C3\u30AF W3.ttc", 0);
				add(u8"/System/Library/Fonts/\u30D2\u30E9\u30AE\u30CE\u89D2\u30B4\u30B7\u30C3\u30AF W4.ttc", 0);
				add(u8"/System/Library/Fonts/\u30D2\u30E9\u30AE\u30CE\u89D2\u30B4\u30B7\u30C3\u30AF W6.ttc", 0);
				add(u8"/System/Library/Fonts/\u30D2\u30E9\u30AE\u30CE\u4E38\u30B4 ProN W4.ttc", 0);
				break;
			case I18n::Script::Korean:
				add(u8"/System/Library/Fonts/AppleSDGothicNeo.ttc", 0);
				add(u8"/Library/Fonts/AppleGothic.ttf", 0);
				break;
			default: break;
		}
#else
		const char *fontconfigLanguage = script == I18n::Script::HanSimplified ? "zh-cn" : script == I18n::Script::HanTraditional ? "zh-tw" :
			script == I18n::Script::Japanese ? "ja" : "ko";
		if (std::optional<FontFile> matched = FontConfigMatch(fontconfigLanguage))
			candidates.push_back(*matched);
		// Noto Sans CJK collections: JP, KR, SC, TC faces in that order
		const int face = script == I18n::Script::Japanese ? 0 : script == I18n::Script::Korean ? 1 : script == I18n::Script::HanSimplified ? 2 : 3;
		for (const char *path : { "/usr/share/fonts/opentype/noto/NotoSansCJK-Regular.ttc", "/usr/share/fonts/noto-cjk/NotoSansCJK-Regular.ttc",
								  "/usr/share/fonts/google-noto-cjk/NotoSansCJK-Regular.ttc", "/usr/share/fonts/truetype/noto/NotoSansCJK-Regular.ttc" })
			candidates.push_back({ path, face });
#endif
		for (const FontFile &candidate : candidates)
			if (Exists(candidate.path))
				found.push_back(candidate);
		return found;
	}
}

// -------------------------------------------------------
