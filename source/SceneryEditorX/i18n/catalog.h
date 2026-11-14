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
 * catalog.h
 * -------------------------------------------------------
 * Created: 13/11/2026
 * -------------------------------------------------------
 */
#pragma once
#include "plural.h"
#include "po.h"
#include <cstdint>
#include <memory>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

// -------------------------------------------------------

namespace SceneryEditorX::I18n
{
	/* @brief How much of a catalog is translated (header excluded; fuzzy and empty translations count as missing). */
	struct CatalogStats
	{
		std::size_t total = 0;
		std::size_t translated = 0;
		std::size_t fuzzy = 0;

		/* @brief Translated share, 0..1 (1 for an empty catalog). */
		[[nodiscard]] float Completion() const { return total == 0 ? 1.0f : static_cast<float>(translated) / static_cast<float>(total); }
	};

	/**
	 * @class Catalog
	 * @brief The translations of one language, read from a PO file and never changed afterwards, so lookups need no
	 * lock. Entries are keyed by the same FNV-1a hash as Msg::key; each keeps its source text, so a hash collision
	 * is a miss (English) rather than a wrong translation.
	 */
	class Catalog
	{
	public:
		/**
		 * @brief Builds a catalog from a parsed PO file. Fuzzy and untranslated entries are left out (they stay
		 * English). The plural rule is the header's, unless it is missing, invalid or disagrees with the language
		 * table's form count, in which case the table's rule is used (and a problem is reported).
		 * @param tag The catalog's language tag (file form, "pt_BR").
		 * @param problems Receives what was wrong (parse errors, plural rule, duplicates), one line each.
		 */
		static std::unique_ptr<Catalog> FromPo(const PoFile &file, std::string_view tag, std::vector<std::string> *problems = nullptr);

		/* @brief The translation of a message, or nullptr. */
		[[nodiscard]] const std::string *Find(std::uint64_t key, std::string_view context, bool hasContext, std::string_view msgid) const;

		/* @brief The plural form of a message for n, or nullptr when the entry is missing or isn't a plural entry. */
		[[nodiscard]] const std::string *FindPlural(std::uint64_t key, std::string_view context, bool hasContext, std::string_view msgid, std::uint64_t n) const;

		[[nodiscard]] const std::string &GetTag() const { return m_Tag; }
		[[nodiscard]] const CatalogStats &GetStats() const { return m_Stats; }
		[[nodiscard]] const PluralRule &GetPluralRule() const { return m_Plural; }
		[[nodiscard]] std::size_t GetEntryCount() const { return m_Entries.size(); }

	private:
		struct Entry
		{
			std::string context;
			bool hasContext = false;
			std::string msgid;
			bool plural = false;
			std::vector<std::string> forms;	// One form, or nplurals forms
		};

		[[nodiscard]] const Entry *Lookup(std::uint64_t key, std::string_view context, bool hasContext, std::string_view msgid) const;

		std::string m_Tag;
		std::unordered_map<std::uint64_t, Entry> m_Entries;
		PluralRule m_Plural;
		CatalogStats m_Stats;
	};
}

// -------------------------------------------------------
