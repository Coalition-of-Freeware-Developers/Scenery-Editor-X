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
 * tr.h
 * -------------------------------------------------------
 * Created: 13/11/2026
 * -------------------------------------------------------
 */
#pragma once
#include "i18n.h"
#include "message.h"
#include <concepts>
#include <cstddef>
#include <cstdint>
#include <string>
#include <type_traits>

// -------------------------------------------------------

/**
 * Translating user-facing text. Mark every English string the user reads with one of these functions; each takes a
 * string literal (or a Msg made from one) and tools/i18n/sedx_i18n.py extracts it into resources/locale/sedx.pot.
 *
 *   ImGui::TextUnformatted(Tr("No airport loaded"));               display text
 *   ImGui::Button(TrL("Export##pack"));                            labels that are ImGui IDs (i18n_imgui.h)
 *   status = TrF("Exported {} tiles to {}", count, folder);        formatted (std::format fields, any order)
 *   ImGui::Text("%s", TrN("{} item", "{} items", n)) ...           plural, or TrNF to format it too
 *   Tr(Ctx("verb", "Open"))                                        a context for short, ambiguous words
 *   static constexpr Msg KINDS[] = { TrNoop("Runway"), ... };      tables translated later with Tr(KINDS[i])
 *
 * Put a "// TRANSLATORS: ..." comment on the line above a string that needs explaining (what a placeholder holds,
 * where the text appears, a length limit); the extractor passes it on to translators.
 *
 * Functions that take a Text (EditorUI's helpers, for example) translate literals by themselves:
 * EditorUI::PropertyLabel("Heading") needs no Tr(). Text made at run time (names, paths) is shown as it is.
 *
 * These are functions, not macros: a TR() macro would rewrite Skia's SkRect::TR() in any file that includes both.
 */
namespace SceneryEditorX
{
	/* @brief The translation of a message (display text: Text, tooltips, column headers, status text). */
	inline const char *Tr(const Msg &message)
	{
		return I18n::Translate(message);
	}

	/* @brief The singular or plural form for n, in the active language's plural rules. */
	inline const char *TrN(const Msg &singular, const Msg &plural, const std::int64_t n)
	{
		return I18n::TranslatePlural(singular, plural, n);
	}

	/**
	 * @brief A translated, formatted message: TrF("{} of {} files", done, total). Arguments are numbers, bools,
	 * chars and strings; convert anything else (paths with VFS::ToUtf8) first.
	 */
	template <typename... Args>
	std::string TrF(const Msg &pattern, const Args &...args)
	{
		const I18n::FormatArg packed[sizeof...(Args) + 1] = { I18n::FormatArg(args)..., I18n::FormatArg() };
		return I18n::FormatMessage(pattern, std::span<const I18n::FormatArg>(packed, sizeof...(Args)));
	}

	/**
	 * @brief A translated plural message formatted with n as its first argument:
	 * TrNF("{} file selected", "{} files selected", count), TrNF("{0} file in {1}", "{0} files in {1}", n, folder).
	 */
	template <typename... Args>
	std::string TrNF(const Msg &singular, const Msg &plural, const std::int64_t n, const Args &...args)
	{
		const I18n::FormatArg packed[sizeof...(Args) + 2] = { I18n::FormatArg(n), I18n::FormatArg(args)..., I18n::FormatArg() };
		return I18n::FormatPluralMessage(singular, plural, n, std::span<const I18n::FormatArg>(packed, sizeof...(Args) + 1));
	}

	/**
	 * @class Text
	 * @brief A parameter type for functions that show text: a string literal (or Msg) is translated, any other string
	 * (const char*, std::string, a char buffer) is shown as it is. A literal picks the translating constructor on its
	 * own, so a helper declared as `void PropertyLabel(Text label)` translates every literal it is called with.
	 *
	 * Text from a ?: of two literals is a const char* and isn't translated: write cond ? Text("A") : Text("B"), or use
	 * Msg. tools/i18n/sedx_i18n.py finds the functions with Text parameters and extracts the literals passed to them.
	 */
	class Text
	{
	public:
		constexpr Text() = default;
		constexpr Text(const Msg &message) : m_Message(message), m_Translated(true) {}

		template <std::size_t N>
		consteval Text(const char (&literal)[N]) : m_Message(literal), m_Translated(true) {}

		/* @brief A buffer filled at run time: shown as it is. */
		template <std::size_t N>
		Text(char (&buffer)[N]) : m_Plain(buffer) {}

		/* @brief A string made at run time (a name, a path): shown as it is. nullptr shows nothing. */
		template <typename T>
			requires std::same_as<std::remove_cvref_t<T>, const char *> || std::same_as<std::remove_cvref_t<T>, char *>
		Text(T &&text) : m_Plain(text ? text : "") {}

		Text(const std::string &text) : m_Plain(text.c_str()) {}
		Text(std::nullptr_t) {}

		/* @brief The text to show: the translation of a literal, else the string as given. */
		[[nodiscard]] const char *Get() const { return m_Translated ? I18n::Translate(m_Message) : (m_Plain ? m_Plain : ""); }

		/* @brief The string as written (English for a literal). */
		[[nodiscard]] const char *Source() const { return m_Translated ? m_Message.text : (m_Plain ? m_Plain : ""); }

		/* @brief True for a literal (or Msg), which is translated. */
		[[nodiscard]] bool IsTranslated() const { return m_Translated; }
		[[nodiscard]] const Msg &GetMsg() const { return m_Message; }	// Not GetMessage: a Windows macro
		[[nodiscard]] bool IsEmpty() const { return Source()[0] == '\0'; }

	private:
		Msg m_Message;
		const char *m_Plain = nullptr;
		bool m_Translated = false;
	};
}

// -------------------------------------------------------
