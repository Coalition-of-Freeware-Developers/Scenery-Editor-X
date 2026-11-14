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
 * po.h
 * -------------------------------------------------------
 * Created: 13/11/2026
 * -------------------------------------------------------
 */
#pragma once
#include <string>
#include <string_view>
#include <utility>
#include <vector>

// -------------------------------------------------------

namespace SceneryEditorX::I18n
{
	/**
	 * @struct PoEntry
	 * @brief One message of a GNU gettext PO file.
	 */
	struct PoEntry
	{
		std::string context;				// msgctxt
		bool hasContext = false;
		std::string msgid;
		std::string msgidPlural;			// msgid_plural
		bool hasPlural = false;
		std::vector<std::string> msgstr;	// msgstr, or msgstr[0..n-1] for a plural entry
		std::vector<std::string> translatorComments;	// "# ..."
		std::vector<std::string> extractedComments;		// "#. ..."
		std::vector<std::string> references;			// "#: ..."
		std::vector<std::string> flags;					// "#, fuzzy, python-brace-format" -> each flag
		bool fuzzy = false;
		int line = 0;						// 1-based line of its msgid

		/* @brief True when every form has a translation and the entry isn't fuzzy. */
		[[nodiscard]] bool IsTranslated() const;
	};

	struct PoError
	{
		int line = 0;
		std::string message;
	};

	/**
	 * @struct PoFile
	 * @brief A parsed PO file: its header fields (the entry with an empty msgid) and its messages, in file order.
	 * Obsolete entries ("#~") are skipped.
	 */
	struct PoFile
	{
		std::vector<std::pair<std::string, std::string>> header;	// "Language: de" -> { "Language", "de" }
		bool hasHeader = false;
		std::vector<PoEntry> entries;
		std::vector<PoError> errors;

		/* @brief A header field's value ("" when the header doesn't have it). Field names compare case-insensitively. */
		[[nodiscard]] std::string_view GetHeader(std::string_view field) const;
	};

	/**
	 * @brief Parses PO text: msgctxt, msgid, msgid_plural, msgstr and msgstr[n], continuation strings, C escapes, the
	 * comment kinds, CRLF line ends and a UTF-8 BOM. Never throws: problems are reported in PoFile::errors with their
	 * line numbers, and the entries that could be read are kept.
	 */
	PoFile ParsePo(std::string_view text);

	/* @brief Decodes the inside of a PO string ("a\"b\n"): false (with what's wrong) for a bad escape. */
	bool UnescapePoString(std::string_view escaped, std::string &out, std::string *error = nullptr);
}

// -------------------------------------------------------
