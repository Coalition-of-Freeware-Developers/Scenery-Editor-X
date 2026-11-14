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
 * languages.cpp
 * -------------------------------------------------------
 * Created: 13/11/2026
 * -------------------------------------------------------
 */
#include "languages.h"
#include <cctype>

// -------------------------------------------------------

namespace SceneryEditorX::I18n
{
	namespace
	{
		/**
		 * One row per line, in this exact shape: tools/i18n/sedx_i18n.py reads the tags and plural counts from here to
		 * check catalogs, so the C++ table is the only copy. Non-ASCII text is written with \u escapes, which compile
		 * to UTF-8 in u8 literals whatever character set the compiler reads the file in. Plural rules are CLDR's, as
		 * Weblate writes them into new catalogs.
		 */
		constexpr LanguageInfo LANGUAGES[] =
		{
			{ "en", "English", u8"English", Script::Latin, 2, "nplurals=2; plural=(n != 1);", u8"" },
			{ "cs", "Czech", u8"Čeština", Script::Latin, 3, "nplurals=3; plural=(n==1) ? 0 : (n>=2 && n<=4) ? 1 : 2;", u8"áčďéěíňóřšťúůýž" },	// Čeština
			{ "da", "Danish", u8"Dansk", Script::Latin, 2, "nplurals=2; plural=(n != 1);", u8"æøåÆØÅ" },
			{ "de", "German", u8"Deutsch", Script::Latin, 2, "nplurals=2; plural=(n != 1);", u8"ÄÖÜäöüß" },
			{ "el", "Greek", u8"Ελληνικά", Script::Greek, 2, "nplurals=2; plural=(n != 1);", u8"αβγδάέήίόύώς" },	// Ελληνικά
			{ "es", "Spanish", u8"Español", Script::Latin, 2, "nplurals=2; plural=(n != 1);", u8"áéíñóúü¿¡" },	// Español
			{ "fi", "Finnish", u8"Suomi", Script::Latin, 2, "nplurals=2; plural=(n != 1);", u8"äöÄÖ" },
			{ "fr", "French", u8"Français", Script::Latin, 2, "nplurals=2; plural=(n > 1);", u8"àâçéèêëîïôûùüÿœæ" },	// Français
			{ "hu", "Hungarian", u8"Magyar", Script::Latin, 2, "nplurals=2; plural=(n != 1);", u8"áéíóöőúüű" },
			{ "it", "Italian", u8"Italiano", Script::Latin, 2, "nplurals=2; plural=(n != 1);", u8"àèéìòù" },
			{ "ja", "Japanese", u8"日本語", Script::Japanese, 1, "nplurals=1; plural=0;", u8"日本語のファイルを開く" },	// 日本語
			{ "ko", "Korean", u8"한국어", Script::Korean, 1, "nplurals=1; plural=0;", u8"한국어파일열기" },	// 한국어
			{ "nb_NO", "Norwegian Bokmal", u8"Norsk bokmål", Script::Latin, 2, "nplurals=2; plural=(n != 1);", u8"æøåÆØÅ" },	// Norsk bokmål
			{ "nl", "Dutch", u8"Nederlands", Script::Latin, 2, "nplurals=2; plural=(n != 1);", u8"ëïé" },
			{ "pl", "Polish", u8"Polski", Script::Latin, 3, "nplurals=3; plural=(n==1 ? 0 : n%10>=2 && n%10<=4 && (n%100<10 || n%100>=20) ? 1 : 2);", u8"ąćęłńóśźż" },
			{ "pt", "Portuguese", u8"Português", Script::Latin, 2, "nplurals=2; plural=(n > 1);", u8"áâãàçéêíóôõú" },	// Português
			{ "pt_BR", "Portuguese (Brazil)", u8"Português (Brasil)", Script::Latin, 2, "nplurals=2; plural=(n > 1);", u8"áâãàçéêíóôõú" },
			{ "pt_PT", "Portuguese (Portugal)", u8"Português (Portugal)", Script::Latin, 2, "nplurals=2; plural=(n != 1);", u8"áâãàçéêíóôõú" },
			{ "ro", "Romanian", u8"Română", Script::Latin, 3, "nplurals=3; plural=(n==1 ? 0 : (n==0 || (n%100 > 0 && n%100 < 20)) ? 1 : 2);", u8"ăâîșț" },	// Română
			{ "ru", "Russian", u8"Русский", Script::Cyrillic, 3, "nplurals=3; plural=(n%10==1 && n%100!=11 ? 0 : n%10>=2 && n%10<=4 && (n%100<10 || n%100>=20) ? 1 : 2);", u8"абвгдёжыэюя" },	// Русский
			{ "sk", "Slovak", u8"Slovenčina", Script::Latin, 3, "nplurals=3; plural=(n==1) ? 0 : (n>=2 && n<=4) ? 1 : 2;", u8"áäčďéíĺľňóôŕšťúýž" },	// Slovenčina
			{ "sv", "Swedish", u8"Svenska", Script::Latin, 2, "nplurals=2; plural=(n != 1);", u8"åäöÅÄÖ" },
			{ "tr", "Turkish", u8"Türkçe", Script::Latin, 2, "nplurals=2; plural=(n != 1);", u8"çğıöşüİ" },	// Türkçe
			{ "uk", "Ukrainian", u8"Українська", Script::Cyrillic, 3, "nplurals=3; plural=(n%10==1 && n%100!=11 ? 0 : n%10>=2 && n%10<=4 && (n%100<10 || n%100>=20) ? 1 : 2);", u8"ґєії" },	// Українська
			{ "zh_Hans", "Chinese (Simplified)", u8"简体中文", Script::HanSimplified, 1, "nplurals=1; plural=0;", u8"简体中文打开文件" },	// 简体中文
			{ "zh_Hant", "Chinese (Traditional)", u8"繁體中文", Script::HanTraditional, 1, "nplurals=1; plural=0;", u8"繁體中文開啟檔案" },	// 繁體中文
			{ "ar", "Arabic", u8"العربية", Script::Arabic, 6, "nplurals=6; plural=n==0 ? 0 : n==1 ? 1 : n==2 ? 2 : n%100>=3 && n%100<=10 ? 3 : n%100>=11 ? 4 : 5;", u8"", true },	// العربية
		};

		char Lower(const char c) { return static_cast<char>(std::tolower(static_cast<unsigned char>(c))); }
		char Upper(const char c) { return static_cast<char>(std::toupper(static_cast<unsigned char>(c))); }

		bool EqualsNoCase(const std::string_view a, const std::string_view b)
		{
			if (a.size() != b.size())
				return false;
			for (std::size_t i = 0; i < a.size(); ++i)
				if (Lower(a[i]) != Lower(b[i]))
					return false;
			return true;
		}

		bool IsAlpha(const std::string_view text)
		{
			for (const char c : text)
				if (!std::isalpha(static_cast<unsigned char>(c)))
					return false;
			return !text.empty();
		}

		bool IsDigits(const std::string_view text)
		{
			for (const char c : text)
				if (!std::isdigit(static_cast<unsigned char>(c)))
					return false;
			return !text.empty();
		}

		struct Parts
		{
			std::string language;
			std::string script;
			std::string region;
		};

		Parts Split(const std::string_view tag)
		{
			Parts parts;
			const std::string normalized = NormalizeTag(tag);
			std::string_view rest = normalized;
			bool first = true;
			while (!rest.empty())
			{
				const std::size_t end = rest.find('_');
				const std::string_view part = rest.substr(0, end);
				if (first)
					parts.language = part;
				else if (part.size() == 4 && IsAlpha(part) && parts.script.empty() && parts.region.empty())
					parts.script = part;
				else if ((part.size() == 2 && IsAlpha(part)) || (part.size() == 3 && IsDigits(part)))
				{
					if (parts.region.empty())
						parts.region = part;
				}
				first = false;
				if (end == std::string_view::npos)
					break;
				rest.remove_prefix(end + 1);
			}
			return parts;
		}

		const std::string *FindAvailable(const std::span<const std::string> available, const std::string_view tag)
		{
			for (const std::string &candidate : available)
				if (EqualsNoCase(NormalizeTag(candidate), tag))
					return &candidate;
			return nullptr;
		}
	}

	// -------------------------------------------------------

	bool LanguageInfo::NeedsCjkFont() const
	{
		return script == Script::HanSimplified || script == Script::HanTraditional || script == Script::Japanese || script == Script::Korean;
	}

	std::span<const LanguageInfo> GetKnownLanguages()
	{
		return LANGUAGES;
	}

	const LanguageInfo *FindLanguage(const std::string_view tag)
	{
		const std::string normalized = NormalizeTag(tag);
		for (const LanguageInfo &language : LANGUAGES)
			if (EqualsNoCase(language.tag, normalized))
				return &language;
		return nullptr;
	}

	std::string NormalizeTag(std::string_view tag)
	{
		// Encoding and modifier: "de_DE.UTF-8@euro"
		const std::size_t suffix = tag.find_first_of(".@");
		if (suffix != std::string_view::npos)
			tag = tag.substr(0, suffix);

		std::string result;
		bool first = true;
		while (!tag.empty())
		{
			const std::size_t end = tag.find_first_of("-_");
			const std::string_view part = tag.substr(0, end);
			if (!part.empty())
			{
				if (!first)
					result += '_';
				for (std::size_t i = 0; i < part.size(); ++i)
				{
					if (first)
						result += Lower(part[i]);
					else if (part.size() == 4 && IsAlpha(part))
						result += i == 0 ? Upper(part[i]) : Lower(part[i]);	// Script: Hans
					else if (part.size() == 2 || (part.size() == 3 && IsDigits(part)))
						result += Upper(part[i]);	// Region: BR, 419
					else
						result += Lower(part[i]);	// Variant
				}
				first = false;
			}
			if (end == std::string_view::npos)
				break;
			tag.remove_prefix(end + 1);
		}
		return result;
	}

	std::string BaseLanguage(const std::string_view tag)
	{
		const std::string normalized = NormalizeTag(tag);
		return normalized.substr(0, normalized.find('_'));
	}

	std::string PosixLocaleToBcp47(std::string_view locale)
	{
		const std::size_t suffix = locale.find_first_of(".@");
		if (suffix != std::string_view::npos)
			locale = locale.substr(0, suffix);
		if (locale.empty() || locale == "C" || locale == "POSIX")
			return "en";
		std::string result(locale);
		for (char &c : result)
			if (c == '_')
				c = '-';
		return result;
	}

	std::string MatchLanguage(const std::span<const std::string> preferred, const std::span<const std::string> available)
	{
		for (const std::string &wanted : preferred)
		{
			const std::string bcp47 = PosixLocaleToBcp47(wanted);
			Parts parts = Split(bcp47);
			if (parts.language.empty())
				continue;

			std::vector<std::string> candidates;
			if (parts.language == "zh")
			{
				// Chinese is chosen by script, never by region alone, and never across scripts
				if (parts.script.empty())
				{
					const bool traditional = parts.region == "TW" || parts.region == "HK" || parts.region == "MO";
					parts.script = traditional ? "Hant" : "Hans";
				}
				candidates.push_back("zh_" + parts.script);
				if (!parts.region.empty())
					candidates.push_back("zh_" + parts.region);
				for (const std::string &candidate : candidates)
					if (const std::string *found = FindAvailable(available, candidate))
						return *found;
				for (const std::string &candidate : available)
				{
					const Parts other = Split(candidate);
					if (other.language == "zh" && other.script == parts.script)
						return candidate;
				}
				continue;
			}

			if (!parts.script.empty() && !parts.region.empty())
				candidates.push_back(parts.language + "_" + parts.script + "_" + parts.region);
			if (!parts.region.empty())
				candidates.push_back(parts.language + "_" + parts.region);
			if (!parts.script.empty())
				candidates.push_back(parts.language + "_" + parts.script);
			candidates.push_back(parts.language);
			for (const std::string &candidate : candidates)
				if (const std::string *found = FindAvailable(available, candidate))
					return *found;

			// Any regional variant of the language: "pt" -> pt_BR
			for (const std::string &candidate : available)
				if (Split(candidate).language == parts.language)
					return candidate;

			// English before any other language: the source text is the English catalog
			if (parts.language == "en")
				return "en";
		}
		return "en";
	}
}

// -------------------------------------------------------
