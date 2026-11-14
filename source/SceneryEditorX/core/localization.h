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
 * localization.h
 * -------------------------------------------------------
 * Created: 13/11/2026
 * -------------------------------------------------------
 */
#pragma once
#include <SceneryEditorX/i18n/languages.h>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

// -------------------------------------------------------

namespace SceneryEditorX
{
	class Settings;

	/**
	 * The editor's language: where catalogs come from, which one the user chose, and switching at a frame boundary.
	 *
	 * Catalogs are resources/locale/<tag>.po (bundled) and user:/locale/<tag>.po (a translator's copy, which wins).
	 * The choice is settings.cfg's ui.language: "system" (the operating system's preferred languages) or a tag;
	 * --language <tag> overrides it for one run. The translation functions themselves are in i18n/tr.h.
	 */
	namespace Localization
	{
		/* @brief ui.language's value for "follow the operating system". */
		inline constexpr std::string_view SYSTEM = "system";

		/**
		 * @brief Finds the catalogs and makes the chosen language active. Call once the settings are open and before the
		 * UI loads its fonts (they depend on the language's script). Reads --language and --i18n-log-missing from args.
		 */
		void Init(Settings &settings, const std::vector<std::string> &args);

		/* @brief Writes the strings that had no translation (with --i18n-log-missing) to logs:/. */
		void Shutdown();

		/* @brief ui.language: "system" or a tag. */
		std::string GetLanguageSetting();

		/* @brief The language "system" means on this machine (a catalog tag, or "en"). */
		std::string ResolveSystemLanguage();

		/* @brief The operating system's preferred UI languages, most preferred first (BCP-47 tags). */
		std::vector<std::string> GetPreferredUiLanguages();

		/**
		 * @brief Chooses a language ("system", a tag, "en" or the pseudo-locale "qps-ploc"): saved to settings.cfg and
		 * applied at the start of the next frame, so nothing is drawn half in one language and half in another.
		 */
		void RequestLanguage(std::string_view setting);

		/* @brief Reads the active language's catalog files again at the next frame (a translator's edit). */
		void RequestReload();

		/* @brief Applies a requested change. UI thread, before ImGui::NewFrame. True when the language changed. */
		bool ApplyPendingChange();

		/* @brief Whether Preferences lists languages that are less than half translated. */
		bool GetShowIncomplete();
		void SetShowIncomplete(bool show);

		/* @brief Catalogs translated at least this much are listed without "Show incomplete translations". */
		inline constexpr float LISTED_COMPLETION = 0.4f;

		/**
		 * @struct FontFile
		 * @brief A font file (UTF-8 path) and its face index in a collection (.ttc).
		 */
		struct FontFile
		{
			std::string path;
			int face = 0;
		};

		/**
		 * @brief The font to merge into the UI fonts for a script Open Sans doesn't cover (Chinese, Japanese, Korean):
		 * settings.cfg's ui.language_font when set, else the operating system's regional UI font, else a Noto Sans CJK
		 * install. Nullopt when the language needs none, or none was found.
		 */
		std::optional<FontFile> FindLanguageFont(const I18n::LanguageInfo &language);
	}
}

// -------------------------------------------------------
