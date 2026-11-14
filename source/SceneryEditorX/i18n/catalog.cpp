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
 * catalog.cpp
 * -------------------------------------------------------
 * Created: 13/11/2026
 * -------------------------------------------------------
 */
#include "catalog.h"
#include "languages.h"
#include "message.h"

// -------------------------------------------------------

namespace SceneryEditorX::I18n
{
	std::unique_ptr<Catalog> Catalog::FromPo(const PoFile &file, const std::string_view tag, std::vector<std::string> *problems)
	{
		const auto report = [problems](std::string message)
		{
			if (problems)
				problems->push_back(std::move(message));
		};

		auto catalog = std::unique_ptr<Catalog>(new Catalog());
		catalog->m_Tag = std::string(tag);

		for (const PoError &error : file.errors)
			report("line " + std::to_string(error.line) + ": " + error.message);

		// Plural rule: the header's, checked against the language table
		const LanguageInfo *language = FindLanguage(tag);
		const std::string_view header = file.GetHeader("Plural-Forms");
		std::string error;
		std::optional<PluralRule> rule = header.empty() ? std::nullopt : PluralRule::Parse(header, &error);
		if (!header.empty() && !rule)
			report("Plural-Forms '" + std::string(header) + "': " + error);
		if (rule && language && rule->Count() != language->pluralCount)
		{
			report("Plural-Forms has nplurals=" + std::to_string(rule->Count()) + " but " + std::string(language->englishName) + " has " +
				std::to_string(language->pluralCount) + " forms: using the built-in rule");
			rule.reset();
		}
		if (!rule && language)
			rule = PluralRule::Parse(language->pluralForms);
		if (rule)
			catalog->m_Plural = *rule;

		for (const PoEntry &entry : file.entries)
		{
			++catalog->m_Stats.total;
			if (entry.fuzzy)
				++catalog->m_Stats.fuzzy;
			if (!entry.IsTranslated())
				continue;
			if (entry.hasPlural && entry.msgstr.size() != static_cast<std::size_t>(catalog->m_Plural.Count()))
			{
				report("line " + std::to_string(entry.line) + ": '" + entry.msgid + "' has " + std::to_string(entry.msgstr.size()) +
					" plural forms, the language has " + std::to_string(catalog->m_Plural.Count()));
				continue;
			}

			Entry value;
			value.context = entry.context;
			value.hasContext = entry.hasContext;
			value.msgid = entry.msgid;
			value.plural = entry.hasPlural;
			value.forms = entry.msgstr;

			const std::uint64_t key = HashKey(entry.context, entry.hasContext, entry.msgid);
			const auto [it, inserted] = catalog->m_Entries.try_emplace(key, std::move(value));
			if (!inserted)
			{
				const Entry &existing = it->second;
				if (existing.msgid == entry.msgid && existing.context == entry.context && existing.hasContext == entry.hasContext)
					report("line " + std::to_string(entry.line) + ": '" + entry.msgid + "' is in the catalog twice; the first is used");
				else
					report("line " + std::to_string(entry.line) + ": '" + entry.msgid + "' has the same hash as '" + existing.msgid + "'; it stays English");
				continue;
			}
			++catalog->m_Stats.translated;
		}
		return catalog;
	}

	const Catalog::Entry *Catalog::Lookup(const std::uint64_t key, const std::string_view context, const bool hasContext, const std::string_view msgid) const
	{
		const auto it = m_Entries.find(key);
		if (it == m_Entries.end())
			return nullptr;
		const Entry &entry = it->second;
		if (entry.hasContext != hasContext || entry.msgid != msgid || (hasContext && entry.context != context))
			return nullptr;
		return &entry;
	}

	const std::string *Catalog::Find(const std::uint64_t key, const std::string_view context, const bool hasContext, const std::string_view msgid) const
	{
		const Entry *entry = Lookup(key, context, hasContext, msgid);
		return entry ? &entry->forms.front() : nullptr;
	}

	const std::string *Catalog::FindPlural(const std::uint64_t key, const std::string_view context, const bool hasContext, const std::string_view msgid,
		const std::uint64_t n) const
	{
		const Entry *entry = Lookup(key, context, hasContext, msgid);
		if (!entry || !entry->plural)
			return nullptr;
		const std::size_t index = m_Plural.Index(n);
		return index < entry->forms.size() ? &entry->forms[index] : nullptr;
	}
}

// -------------------------------------------------------
