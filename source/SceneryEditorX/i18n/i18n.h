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
 * i18n.h
 * -------------------------------------------------------
 * Created: 13/11/2026
 * -------------------------------------------------------
 */
#pragma once
#include "catalog.h"
#include "format.h"
#include "languages.h"
#include "message.h"
#include <cstdint>
#include <functional>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <vector>

// -------------------------------------------------------

/**
 * The translation service: which catalogs exist, which language is active, and the lookups behind Tr() and friends
 * (tr.h). Code that shows text calls the tr.h functions; this API is for the code that sets languages up (the
 * application's localization glue, Preferences, tests).
 *
 * Threads: lookups may run on any thread. Catalogs are never freed before shutdown (or Reset()), so the const char*
 * a lookup returns stays valid for the session. SetLanguage, SetCatalogFiles and Reload are for the UI thread.
 *
 * Memory: a catalog is read once and kept, so switching languages back and forth costs nothing after the first time.
 * Reload() keeps the catalog it replaces (a few hundred KB) until exit, because its strings may still be on screen or
 * held by another thread; a reload of a file that hasn't changed, or can't be read, keeps the catalog in use instead of
 * adding one, so only a translator's actual edits add copies.
 */
namespace SceneryEditorX::I18n
{
	/* @brief The source language: no catalog, the English text in the code. */
	inline constexpr std::string_view SOURCE_LANGUAGE = "en";

	enum class LogLevel : std::uint8_t
	{
		Info,
		Warning,
		Error
	};

	/* @brief Where the service reports catalogs it loaded and problems it found (the application points it at the log). */
	using LogSink = std::function<void(LogLevel, std::string_view)>;
	void SetLogSink(LogSink sink);

	/**
	 * @struct CatalogFile
	 * @brief A catalog that can be chosen: its tag and how to read its PO text (read lazily, when it's first needed).
	 */
	struct CatalogFile
	{
		std::string tag;			// File form: "de", "pt_BR"
		std::string origin;			// Where it comes from, for messages (a path)
		std::function<std::optional<std::string>()> read;
		bool user = false;			// A translator's override (user:/locale) rather than a bundled catalog
	};

	/**
	 * @struct LanguageChoice
	 * @brief A language that can be chosen, for Preferences.
	 */
	struct LanguageChoice
	{
		std::string tag;
		std::string englishName;	// From the language table, or the tag
		std::string nativeName;		// UTF-8; the English name for an unknown language
		std::string origin;
		CatalogStats stats;
		bool user = false;
		bool failed = false;		// The file couldn't be read
		const LanguageInfo *info = nullptr;
	};

	/**
	 * @brief Sets the catalogs that can be chosen. A user catalog replaces a bundled one with the same tag. The active
	 * language keeps its loaded catalog until SetLanguage or Reload. A file registered again with the same origin keeps
	 * the catalog read from it (what Reload compares against); one that couldn't be read is tried again.
	 */
	void SetCatalogFiles(std::vector<CatalogFile> files);

	/* @brief The tags of the catalogs that can be chosen (file form, sorted), English and the pseudo-locale not included. */
	std::vector<std::string> GetAvailableTags();

	/* @brief Every catalog that can be chosen with its completion (reads the ones not read yet). */
	std::vector<LanguageChoice> GetLanguageChoices();

	/**
	 * @brief Makes a language active: "en" (no catalog), "qps-ploc" (the pseudo-locale) or the tag of a catalog file;
	 * a regional catalog falls back to its base language's catalog when there is one (pt_BR -> pt), then to English.
	 * @return False when the tag is unknown or its catalog can't be read; the active language doesn't change then.
	 */
	bool SetLanguage(std::string_view tag);

	/* @brief The active language's tag ("en" when none). */
	std::string GetLanguage();

	/* @brief The language table's entry for the active language (nullptr for the pseudo-locale or an unknown tag). */
	const LanguageInfo *GetLanguageInfo();

	/* @brief Changes every time translations may have changed (language switch, reload): caches compare it. */
	std::uint64_t GetGeneration();

	/**
	 * @brief Reads the active language's catalog files again (a translator's edit). False when one can't be read: the
	 * version read before then stays in use, and the language stays listed as it was.
	 */
	bool Reload();

	// --- Lookups (any thread) ---

	/* @brief The translation of a message, or its text when there is none. */
	const char *Translate(const Msg &message);

	/* @brief The plural form for n: the catalog's, or English's (singular for 1, plural otherwise). */
	const char *TranslatePlural(const Msg &singular, const Msg &plural, std::int64_t n);

	/* @brief The translation of text that isn't a literal (hashed at run time), or the text itself. Slower than Translate. */
	std::string_view TranslateRuntime(std::string_view text, std::string_view context = {}, bool hasContext = false);

	/**
	 * @brief Translates a pattern and formats it with args. A translation that fails to format is replaced by the
	 * English pattern for that call, and reported once per message; never throws.
	 */
	std::string FormatMessage(const Msg &pattern, std::span<const FormatArg> args);

	/* @brief The same with the plural form for n (n is usually also args[0]). */
	std::string FormatPluralMessage(const Msg &singular, const Msg &plural, std::int64_t n, std::span<const FormatArg> args);

	// --- Missing translations (for translators and the pseudo-locale) ---

	struct MissingEntry
	{
		std::string context;
		bool hasContext = false;
		std::string msgid;
		std::string msgidPlural;
		std::uint64_t count = 0;
	};

	/* @brief Records each message a non-English language had no translation for (off by default). */
	void SetRecordMissing(bool record);
	[[nodiscard]] bool IsRecordingMissing();
	std::vector<MissingEntry> GetMissing();

	/* @brief The missing messages as PO entries with empty translations, to paste into a catalog. */
	std::string MissingAsPo();

	/**
	 * @brief Tests only: forgets every catalog and goes back to English. It frees the strings lookups returned, so it must
	 * never run while another thread may translate or still holds a translation; the editor never calls it.
	 */
	void Reset();
}

// -------------------------------------------------------
