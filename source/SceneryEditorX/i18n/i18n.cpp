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
 * i18n.cpp
 * -------------------------------------------------------
 * Created: 13/11/2026
 * -------------------------------------------------------
 */
#include "i18n.h"
#include "languages.h"
#include "pseudo.h"
#include <algorithm>
#include <atomic>
#include <map>
#include <memory>
#include <mutex>
#include <unordered_map>
#include <unordered_set>

// -------------------------------------------------------

namespace SceneryEditorX::I18n
{
	namespace
	{
		/* What lookups read: replaced (never changed) on each language switch, and kept until Reset(). */
		struct ActiveSet
		{
			std::string tag;
			const Catalog *primary = nullptr;
			const Catalog *base = nullptr;		// pt for pt_BR
			const LanguageInfo *info = nullptr;
			bool pseudo = false;
		};

		struct Registered
		{
			CatalogFile file;
			const Catalog *catalog = nullptr;	// Owned by State::catalogs
			bool attempted = false;
			bool failed = false;
		};

		struct State
		{
			std::mutex mutex;	// Registry, catalogs, sets, sink
			std::map<std::string, Registered> registry;	// By tag
			std::vector<std::unique_ptr<Catalog>> catalogs;
			std::vector<std::unique_ptr<ActiveSet>> sets;
			LogSink sink;

			std::atomic<const ActiveSet *> active { nullptr };
			std::atomic<std::uint64_t> generation { 1 };

			std::atomic<bool> recordMissing { false };
			std::mutex missingMutex;
			std::unordered_map<std::uint64_t, MissingEntry> missing;

			std::mutex pseudoMutex;
			std::unordered_map<std::uint64_t, std::unique_ptr<std::string>> pseudo;

			std::mutex formatMutex;
			std::unordered_set<std::uint64_t> formatReported;
		};

		State &S()
		{
			static State state;
			return state;
		}

		void Log(const LogLevel level, const std::string &message)
		{
			LogSink sink;
			{
				std::lock_guard lock(S().mutex);
				sink = S().sink;
			}
			if (sink)
				sink(level, message);
		}

		/* @brief Reads and parses a registered catalog once; State::mutex must be held. */
		const Catalog *LoadLocked(Registered &registered, std::vector<std::string> &messages)
		{
			if (registered.attempted)
				return registered.catalog;
			registered.attempted = true;

			std::optional<std::string> text = registered.file.read ? registered.file.read() : std::nullopt;
			if (!text)
			{
				registered.failed = true;
				messages.push_back("Can't read the " + registered.file.tag + " catalog " + registered.file.origin);
				return nullptr;
			}

			const PoFile file = ParsePo(*text);
			std::vector<std::string> problems;
			std::unique_ptr<Catalog> catalog = Catalog::FromPo(file, registered.file.tag, &problems);
			for (const std::string &problem : problems)
				messages.push_back(registered.file.origin + ": " + problem);
			registered.catalog = catalog.get();
			S().catalogs.push_back(std::move(catalog));
			return registered.catalog;
		}

		void Activate(std::unique_ptr<ActiveSet> set)
		{
			State &s = S();
			const ActiveSet *pointer = set.get();
			s.sets.push_back(std::move(set));
			s.active.store(pointer, std::memory_order_release);
			s.generation.fetch_add(1, std::memory_order_acq_rel);
		}

		void RecordMissing(const Msg &singular, const Msg *plural)
		{
			State &s = S();
			if (!s.recordMissing.load(std::memory_order_relaxed))
				return;
			std::lock_guard lock(s.missingMutex);
			MissingEntry &entry = s.missing[singular.key];
			if (entry.count++ == 0)
			{
				entry.context = std::string(singular.Context());
				entry.hasContext = singular.HasContext();
				entry.msgid = std::string(singular.Id());
				if (plural)
					entry.msgidPlural = std::string(plural->Id());
			}
		}

		const char *Pseudo(const std::uint64_t key, const std::string_view text)
		{
			State &s = S();
			std::lock_guard lock(s.pseudoMutex);
			std::unique_ptr<std::string> &cached = s.pseudo[key];
			if (!cached)
				cached = std::make_unique<std::string>(PseudoLocalize(text));
			return cached->c_str();
		}

		const std::string *Find(const ActiveSet &set, const std::uint64_t key, const std::string_view context, const bool hasContext, const std::string_view msgid)
		{
			if (set.primary)
				if (const std::string *found = set.primary->Find(key, context, hasContext, msgid))
					return found;
			if (set.base)
				if (const std::string *found = set.base->Find(key, context, hasContext, msgid))
					return found;
			return nullptr;
		}

		std::uint64_t Magnitude(const std::int64_t n)
		{
			return n < 0 ? 0 - static_cast<std::uint64_t>(n) : static_cast<std::uint64_t>(n);
		}

		std::string Escape(const std::string_view text)
		{
			std::string out;
			for (const char c : text)
			{
				switch (c)
				{
					case '"': out += "\\\""; break;
					case '\\': out += "\\\\"; break;
					case '\n': out += "\\n"; break;
					case '\t': out += "\\t"; break;
					default: out += c; break;
				}
			}
			return out;
		}

		std::string Format(const Msg &source, const std::string_view translated, const std::span<const FormatArg> args)
		{
			std::string out, error;
			if (FormatTo(out, translated, args, &error))
				return out;

			if (translated.data() != source.text)
			{
				bool first = false;
				{
					std::lock_guard lock(S().formatMutex);
					first = S().formatReported.insert(source.key).second;
				}
				if (first)
					Log(LogLevel::Warning, "The " + GetLanguage() + " translation of \"" + std::string(source.Id()) + "\" doesn't format (" + error +
						"); showing English");
				out.clear();
				if (FormatTo(out, source.Text(), args, &error))
					return out;
			}
			// The English pattern itself is wrong: a programming error, shown as written
			return std::string(source.Text());
		}
	}

	// -------------------------------------------------------

	void SetLogSink(LogSink sink)
	{
		std::lock_guard lock(S().mutex);
		S().sink = std::move(sink);
	}

	void SetCatalogFiles(std::vector<CatalogFile> files)
	{
		State &s = S();
		std::lock_guard lock(s.mutex);
		std::map<std::string, Registered> registry;
		for (CatalogFile &file : files)
		{
			file.tag = NormalizeTag(file.tag);
			if (file.tag.empty())
				continue;
			const auto it = registry.find(file.tag);
			if (it != registry.end() && it->second.file.user && !file.user)
				continue;	// A translator's file wins over the bundled one
			Registered registered;
			registered.file = std::move(file);
			registry[registered.file.tag] = std::move(registered);
		}
		s.registry = std::move(registry);
	}

	std::vector<std::string> GetAvailableTags()
	{
		std::lock_guard lock(S().mutex);
		std::vector<std::string> tags;
		for (const auto &[tag, registered] : S().registry)
			tags.push_back(tag);
		return tags;
	}

	std::vector<LanguageChoice> GetLanguageChoices()
	{
		std::vector<LanguageChoice> choices;
		std::vector<std::string> messages;
		{
			std::lock_guard lock(S().mutex);
			for (auto &[tag, registered] : S().registry)
			{
				const Catalog *catalog = LoadLocked(registered, messages);
				LanguageChoice choice;
				choice.tag = tag;
				choice.origin = registered.file.origin;
				choice.user = registered.file.user;
				choice.failed = registered.failed;
				choice.info = FindLanguage(tag);
				choice.englishName = choice.info ? std::string(choice.info->englishName) : tag;
				choice.nativeName = choice.info ? std::string(choice.info->NativeName()) : choice.englishName;
				if (catalog)
					choice.stats = catalog->GetStats();
				choices.push_back(std::move(choice));
			}
		}
		for (const std::string &message : messages)
			Log(LogLevel::Warning, message);
		return choices;
	}

	bool SetLanguage(const std::string_view requested)
	{
		const std::string tag = NormalizeTag(requested);
		State &s = S();

		if (tag.empty() || tag == SOURCE_LANGUAGE)
		{
			std::lock_guard lock(s.mutex);
			auto set = std::make_unique<ActiveSet>();
			set->tag = std::string(SOURCE_LANGUAGE);
			set->info = FindLanguage(SOURCE_LANGUAGE);
			Activate(std::move(set));
			return true;
		}

		if (NormalizeTag(PSEUDO_LANGUAGE) == tag)
		{
			std::lock_guard lock(s.mutex);
			auto set = std::make_unique<ActiveSet>();
			set->tag = std::string(PSEUDO_LANGUAGE);
			set->pseudo = true;
			Activate(std::move(set));
			return true;
		}

		std::vector<std::string> messages;
		bool ok = false;
		std::string loadedFrom;
		std::size_t translated = 0, total = 0;
		{
			std::lock_guard lock(s.mutex);
			const auto it = s.registry.find(tag);
			if (it != s.registry.end())
			{
				const Catalog *primary = LoadLocked(it->second, messages);
				if (primary)
				{
					auto set = std::make_unique<ActiveSet>();
					set->tag = tag;
					set->primary = primary;
					set->info = FindLanguage(tag);
					const std::string base = BaseLanguage(tag);
					if (base != tag)
					{
						const auto baseIt = s.registry.find(base);
						if (baseIt != s.registry.end())
							set->base = LoadLocked(baseIt->second, messages);
					}
					loadedFrom = it->second.file.origin;
					translated = primary->GetStats().translated;
					total = primary->GetStats().total;
					Activate(std::move(set));
					ok = true;
				}
			}
			else
				messages.push_back("No catalog for language '" + tag + "'");
		}
		for (const std::string &message : messages)
			Log(LogLevel::Warning, message);
		if (ok)
			Log(LogLevel::Info, "Language " + tag + ": " + std::to_string(translated) + " of " + std::to_string(total) + " messages translated (" + loadedFrom + ")");
		return ok;
	}

	std::string GetLanguage()
	{
		const ActiveSet *set = S().active.load(std::memory_order_acquire);
		return set ? set->tag : std::string(SOURCE_LANGUAGE);
	}

	const LanguageInfo *GetLanguageInfo()
	{
		const ActiveSet *set = S().active.load(std::memory_order_acquire);
		return set ? set->info : FindLanguage(SOURCE_LANGUAGE);
	}

	std::uint64_t GetGeneration()
	{
		return S().generation.load(std::memory_order_acquire);
	}

	bool Reload()
	{
		const std::string tag = GetLanguage();
		{
			State &s = S();
			std::lock_guard lock(s.mutex);
			for (const std::string &name : { tag, BaseLanguage(tag) })
			{
				const auto it = s.registry.find(name);
				if (it == s.registry.end())
					continue;
				it->second.attempted = false;	// The old catalog stays alive until Reset (pointers may still be in use)
				it->second.failed = false;
				it->second.catalog = nullptr;
			}
		}
		// Pseudo-locale strings don't depend on catalogs and may still be in use: they stay
		return SetLanguage(tag);
	}

	const char *Translate(const Msg &message)
	{
		const ActiveSet *set = S().active.load(std::memory_order_acquire);
		if (!set || message.IsEmpty() || (!set->primary && !set->pseudo))
			return message.text;
		if (set->pseudo)
			return Pseudo(message.key, message.Id());
		if (const std::string *found = Find(*set, message.key, message.Context(), message.HasContext(), message.Id()))
			return found->c_str();
		RecordMissing(message, nullptr);
		return message.text;
	}

	const char *TranslatePlural(const Msg &singular, const Msg &plural, const std::int64_t n)
	{
		const Msg &english = n == 1 ? singular : plural;
		const ActiveSet *set = S().active.load(std::memory_order_acquire);
		if (!set || singular.IsEmpty() || (!set->primary && !set->pseudo))
			return english.text;
		if (set->pseudo)
			return Pseudo(HashAppend(singular.key, n == 1 ? "\x01" "1" : "\x01" "n"), english.Id());

		const std::uint64_t magnitude = Magnitude(n);
		for (const Catalog *catalog : { set->primary, set->base })
			if (catalog)
				if (const std::string *found = catalog->FindPlural(singular.key, singular.Context(), singular.HasContext(), singular.Id(), magnitude))
					return found->c_str();
		RecordMissing(singular, &plural);
		return english.text;
	}

	std::string_view TranslateRuntime(const std::string_view text, const std::string_view context, const bool hasContext)
	{
		const ActiveSet *set = S().active.load(std::memory_order_acquire);
		const std::string_view id = text.substr(0, VisibleLength(text));
		if (!set || id.empty() || (!set->primary && !set->pseudo))
			return text;
		const std::uint64_t key = HashKey(context, hasContext, id);
		if (set->pseudo)
			return Pseudo(key, id);
		if (const std::string *found = Find(*set, key, context, hasContext, id))
			return *found;
		return text;
	}

	std::string FormatMessage(const Msg &pattern, const std::span<const FormatArg> args)
	{
		const char *translated = Translate(pattern);
		// Translate returns the literal itself when there's no translation; its "##" part (rare in a pattern) is kept
		const std::string_view text = translated == pattern.text ? pattern.Text() : std::string_view(translated);
		return Format(pattern, text, args);
	}

	std::string FormatPluralMessage(const Msg &singular, const Msg &plural, const std::int64_t n, const std::span<const FormatArg> args)
	{
		const Msg &english = n == 1 ? singular : plural;
		const char *translated = TranslatePlural(singular, plural, n);
		const std::string_view text = translated == english.text ? english.Text() : std::string_view(translated);
		return Format(english, text, args);
	}

	void SetRecordMissing(const bool record)
	{
		S().recordMissing.store(record, std::memory_order_relaxed);
	}

	bool IsRecordingMissing()
	{
		return S().recordMissing.load(std::memory_order_relaxed);
	}

	std::vector<MissingEntry> GetMissing()
	{
		std::vector<MissingEntry> entries;
		{
			std::lock_guard lock(S().missingMutex);
			for (const auto &[key, entry] : S().missing)
				entries.push_back(entry);
		}
		std::sort(entries.begin(), entries.end(), [](const MissingEntry &a, const MissingEntry &b)
		{
			return a.count != b.count ? a.count > b.count : a.msgid < b.msgid;
		});
		return entries;
	}

	std::string MissingAsPo()
	{
		std::string out;
		for (const MissingEntry &entry : GetMissing())
		{
			out += "#. Used " + std::to_string(entry.count) + " times while " + GetLanguage() + " was active\n";
			if (entry.hasContext)
				out += "msgctxt \"" + Escape(entry.context) + "\"\n";
			out += "msgid \"" + Escape(entry.msgid) + "\"\n";
			if (!entry.msgidPlural.empty())
				out += "msgid_plural \"" + Escape(entry.msgidPlural) + "\"\nmsgstr[0] \"\"\n\n";
			else
				out += "msgstr \"\"\n\n";
		}
		return out;
	}

	void Reset()
	{
		State &s = S();
		std::lock_guard lock(s.mutex);
		s.active.store(nullptr, std::memory_order_release);
		s.generation.fetch_add(1, std::memory_order_acq_rel);
		s.registry.clear();
		s.sets.clear();
		s.catalogs.clear();
		{
			std::lock_guard missing(s.missingMutex);
			s.missing.clear();
		}
		{
			std::lock_guard pseudo(s.pseudoMutex);
			s.pseudo.clear();
		}
		{
			std::lock_guard format(s.formatMutex);
			s.formatReported.clear();
		}
		s.recordMissing.store(false, std::memory_order_relaxed);
	}
}

// -------------------------------------------------------
