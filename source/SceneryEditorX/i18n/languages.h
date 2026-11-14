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
 * languages.h
 * -------------------------------------------------------
 * Created: 13/11/2026
 * -------------------------------------------------------
 */
#pragma once
#include <cstdint>
#include <span>
#include <string>
#include <string_view>
#include <vector>

// -------------------------------------------------------

namespace SceneryEditorX::I18n
{
	/* @brief The script a language is written in: decides which font has to be merged into the UI fonts. */
	enum class Script : std::uint8_t
	{
		Latin,
		Greek,
		Cyrillic,
		HanSimplified,
		HanTraditional,
		Japanese,
		Korean,
		Arabic,
		Hebrew
	};

	/**
	 * @struct LanguageInfo
	 * @brief What the editor knows about a language: its catalog tag (Weblate's gettext naming), names, script, the
	 * gettext plural rule (from CLDR, as Weblate writes it) and a sample of the letters it needs (font coverage checks).
	 */
	struct LanguageInfo
	{
		std::string_view tag;			// File tag: "de", "pt_BR", "zh_Hans"
		std::string_view englishName;	// "German"
		std::u8string_view nativeName;	// "Deutsch"
		Script script = Script::Latin;
		int pluralCount = 2;			// nplurals
		std::string_view pluralForms;	// The full Plural-Forms value
		std::u8string_view sample;		// Letters the language needs beyond ASCII (font coverage checks)
		bool rightToLeft = false;		// Not offered until right-to-left layout exists

		/* @brief nativeName as UTF-8 chars. */
		[[nodiscard]] std::string_view NativeName() const { return { reinterpret_cast<const char *>(nativeName.data()), nativeName.size() }; }

		/* @brief sample as UTF-8 chars. */
		[[nodiscard]] std::string_view Sample() const { return { reinterpret_cast<const char *>(sample.data()), sample.size() }; }

		/* @brief True for Chinese, Japanese and Korean, which need a CJK font merged into the UI fonts. */
		[[nodiscard]] bool NeedsCjkFont() const;
	};

	/* @brief Every language the editor has names and plural rules for. */
	std::span<const LanguageInfo> GetKnownLanguages();

	/* @brief The language with this tag ("de", "pt-BR", "zh_hans": case and '-' / '_' don't matter), or nullptr. */
	const LanguageInfo *FindLanguage(std::string_view tag);

	/**
	 * @brief A tag in catalog-file form: language lowercase, script title case, region uppercase, joined by '_'.
	 * "pt-br" -> "pt_BR", "ZH-hans" -> "zh_Hans", "sr-latn-rs" -> "sr_Latn_RS". Encoding and modifier suffixes go.
	 */
	std::string NormalizeTag(std::string_view tag);

	/* @brief The language part of a tag: "pt_BR" -> "pt", "zh_Hans" -> "zh". */
	std::string BaseLanguage(std::string_view tag);

	/**
	 * @brief A POSIX locale name (LANG, LC_ALL, LC_MESSAGES, an entry of LANGUAGE) as a BCP-47 tag:
	 * "de_DE.UTF-8@euro" -> "de-DE"; "C", "POSIX" and "" -> "en".
	 */
	std::string PosixLocaleToBcp47(std::string_view locale);

	/**
	 * @brief Chooses the catalog for the user's preferred languages (most preferred first, BCP-47 or POSIX tags):
	 * the first preferred language that matches an available tag wins. Chinese goes by script: zh-CN, zh-SG and
	 * zh-Hans* are zh_Hans, zh-TW, zh-HK, zh-MO and zh-Hant* are zh_Hant. Then an exact match (pt-BR -> pt_BR), the
	 * base language (pt-BR -> pt), and any regional variant of it (pt -> pt_BR). English ("en") when nothing matches.
	 * @param available Catalog tags in file form.
	 * @return An entry of available, or "en".
	 */
	std::string MatchLanguage(std::span<const std::string> preferred, std::span<const std::string> available);
}

// -------------------------------------------------------
