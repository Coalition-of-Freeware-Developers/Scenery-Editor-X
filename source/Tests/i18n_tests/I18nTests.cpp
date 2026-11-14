/**
 * -------------------------------------------------------
 * Scenery Editor X
 * -------------------------------------------------------
 * Copyright (c) 2026 Thomas Ray
 * Copyright (c) 2026 Coalition of Freeware Developers
 * -------------------------------------------------------
 * I18nTests.cpp
 * -------------------------------------------------------
 * The translation core (source/SceneryEditorX/i18n): message keys, the PO parser, plural rules, the language table
 * and matching, formatting, the pseudo-locale and the translation service. Standard library only.
 * Non-ASCII text is written with \u escapes in u8 literals so the file compiles the same whatever source character
 * set a compiler assumes.
 * -------------------------------------------------------
 */
#include <catch2/catch_all.hpp>
#include <SceneryEditorX/i18n/catalog.h>
#include <SceneryEditorX/i18n/format.h>
#include <SceneryEditorX/i18n/i18n.h>
#include <SceneryEditorX/i18n/languages.h>
#include <SceneryEditorX/i18n/plural.h>
#include <SceneryEditorX/i18n/po.h>
#include <SceneryEditorX/i18n/pseudo.h>
#include <SceneryEditorX/i18n/tr.h>
#include <algorithm>
#include <clocale>
#include <cstring>
#include <atomic>
#include <chrono>
#include <thread>

using namespace SceneryEditorX;
using namespace SceneryEditorX::I18n;

namespace
{
	std::string U8(const std::u8string_view text)
	{
		return std::string(reinterpret_cast<const char *>(text.data()), text.size());
	}

	/* @brief Registers in-memory catalogs (tag -> PO text) as the service's catalog files. */
	void UseCatalogs(const std::vector<std::pair<std::string, std::string>> &catalogs, const bool user = false)
	{
		std::vector<CatalogFile> files;
		for (const auto &[tag, text] : catalogs)
			files.push_back({ tag, "memory:" + tag + ".po", [text] { return std::optional<std::string>(text); }, user });
		SetCatalogFiles(std::move(files));
	}

	struct ServiceGuard
	{
		ServiceGuard() { Reset(); }
		~ServiceGuard() { Reset(); SetLogSink(nullptr); }
	};

	const std::string GERMAN = U8(
		u8"msgid \"\"\n"
		u8"msgstr \"\"\n"
		u8"\"Language: de\\n\"\n"
		u8"\"Plural-Forms: nplurals=2; plural=(n != 1);\\n\"\n"
		u8"\n"
		u8"msgid \"Save\"\n"
		u8"msgstr \"Speichern\"\n"
		u8"\n"
		u8"msgctxt \"verb\"\n"
		u8"msgid \"Open\"\n"
		u8"msgstr \"Öffnen\"\n"
		u8"\n"
		u8"msgctxt \"state\"\n"
		u8"msgid \"Open\"\n"
		u8"msgstr \"Offen\"\n"
		u8"\n"
		u8"msgid \"PROPERTIES\"\n"
		u8"msgstr \"EIGENSCHAFTEN\"\n"
		u8"\n"
		u8"msgid \"{} file\"\n"
		u8"msgid_plural \"{} files\"\n"
		u8"msgstr[0] \"{} Datei\"\n"
		u8"msgstr[1] \"{} Dateien\"\n"
		u8"\n"
		u8"msgid \"{0} of {1}\"\n"
		u8"msgstr \"{1}: {0}\"\n"
		u8"\n"
		u8"#, fuzzy\n"
		u8"msgid \"Cancel\"\n"
		u8"msgstr \"Abbrechen\"\n"
		u8"\n"
		u8"msgid \"Broken {}\"\n"
		u8"msgstr \"Kaputt {1}\"\n"
		u8"\n"
		u8"msgid \"Untranslated\"\n"
		u8"msgstr \"\"\n");
}

// -------------------------------------------------------
// Message keys
// -------------------------------------------------------

TEST_CASE("Msg keys are hashed at compile time like catalog keys", "[i18n][message]")
{
	static constexpr Msg save("Save");
	static_assert(save.Id() == "Save");
	static_assert(save.key == HashKey({}, false, "Save"));
	CHECK(save.key == HashKey({}, false, std::string("Save")));

	static constexpr Msg label("Open##recent");
	static_assert(label.Id() == "Open");
	static_assert(label.Text() == "Open##recent");
	static_assert(label.key == save.key || label.key == HashKey({}, false, "Open"));

	static constexpr Msg panel("PROPERTIES###Properties");
	static_assert(panel.Id() == "PROPERTIES");

	static constexpr Msg id("##zoom");
	static_assert(id.IsEmpty());

	static constexpr Msg verb = Ctx("verb", "Open");
	static_assert(verb.HasContext() && verb.Context() == "verb" && verb.Id() == "Open");
	static_assert(verb.key != label.key);
	CHECK(verb.key == HashKey("verb", true, "Open"));
	// FNV-1a 64 reference values, also checked by tools/i18n/test_sedx_i18n.py (the extractor hashes keys the same way)
	static_assert(HashKey({}, false, "") == 0xCBF29CE484222325ull);
	static_assert(HashKey({}, false, "a") == 0xAF63DC4C8601EC8Cull);

	static constexpr Msg table[] = { TrNoop("Runway"), TrNoop("Taxiway") };
	static_assert(table[1].Id() == "Taxiway");
}

// -------------------------------------------------------
// PO parser
// -------------------------------------------------------

TEST_CASE("PO parser reads entries, header, plurals and comments", "[i18n][po]")
{
	const PoFile file = ParsePo(
		"# Translator comment at the top\n"
		"msgid \"\"\n"
		"msgstr \"\"\n"
		"\"Language: fr\\n\"\n"
		"\"Plural-Forms: nplurals=2; plural=(n > 1);\\n\"\n"
		"\n"
		"#. TRANSLATORS: a button\n"
		"#: source/Editor/core/editor_layer.cpp\n"
		"#, python-brace-format\n"
		"msgctxt \"verb\"\n"
		"msgid \"Open\"\n"
		"msgstr \"Ouvrir\"\n"
		"\n"
		"msgid \"\"\n"
		"\"Two \"\n"
		"\"lines\\n\"\n"
		"msgstr \"Deux lignes\\n\"\n"
		"\n"
		"msgid \"{} item\"\n"
		"msgid_plural \"{} items\"\n"
		"msgstr[0] \"{} \\u00e9l\\303\\251ment\"\n"
		"msgstr[1] \"{} \\x41\\101\"\n"
		"\n"
		"#~ msgid \"Gone\"\n"
		"#~ msgstr \"Parti\"\n");

	CHECK(file.errors.size() == 1);	// \u isn't a PO escape
	CHECK(file.hasHeader);
	CHECK(file.GetHeader("language") == "fr");
	CHECK(file.GetHeader("Plural-Forms") == "nplurals=2; plural=(n > 1);");
	REQUIRE(file.entries.size() == 3);

	const PoEntry &open = file.entries[0];
	CHECK(open.hasContext);
	CHECK(open.context == "verb");
	CHECK(open.msgid == "Open");
	CHECK(open.msgstr.front() == "Ouvrir");
	CHECK(open.extractedComments == std::vector<std::string> { "TRANSLATORS: a button" });
	CHECK(open.references == std::vector<std::string> { "source/Editor/core/editor_layer.cpp" });
	CHECK(open.flags == std::vector<std::string> { "python-brace-format" });
	CHECK(open.line == 11);

	CHECK(file.entries[1].msgid == "Two lines\n");
	const PoEntry &plural = file.entries[2];
	CHECK(plural.hasPlural);
	REQUIRE(plural.msgstr.size() == 2);
	CHECK(plural.msgstr[0] == "{} u00e9l\xC3\xA9ment");
	CHECK(plural.msgstr[1] == "{} AA");
}

TEST_CASE("PO parser handles CRLF, a BOM, fuzzy entries and reports errors with lines", "[i18n][po]")
{
	const PoFile crlf = ParsePo("\xEF\xBB\xBFmsgid \"A\"\r\nmsgstr \"B\"\r\n\r\n#, fuzzy\r\nmsgid \"C\"\r\nmsgstr \"D\"\r\n");
	CHECK(crlf.errors.empty());
	REQUIRE(crlf.entries.size() == 2);
	CHECK(crlf.entries[0].msgstr.front() == "B");
	CHECK(crlf.entries[1].fuzzy);
	CHECK_FALSE(crlf.entries[1].IsTranslated());
	CHECK_FALSE(crlf.hasHeader);

	const PoFile bad = ParsePo(
		"msgstr \"orphan\"\n"				// 1: msgstr without a msgid
		"\n"
		"msgid \"x\"\n"
		"msgstr \"unterminated\n"			// 4: not a closed string
		"\n"
		"bogus \"y\"\n"						// 6: unknown keyword
		"msgid \"a\" \"b\"\n");				// 7: an unescaped quote inside
	REQUIRE(bad.errors.size() >= 4);
	CHECK(bad.errors[0].line == 1);
	CHECK(bad.errors[1].line == 4);
	CHECK(bad.errors[2].line == 6);
	CHECK(bad.errors[3].line == 7);

	CHECK_NOTHROW(ParsePo(std::string("msgid \"\\")));
	CHECK_NOTHROW(ParsePo(std::string(1000, '"')));
	CHECK_NOTHROW(ParsePo("msgid_plural \"p\"\nmsgstr[0] \"q\"\nmsgstr[x] \"r\"\nmsgstr[] \"s\"\n"));
}

TEST_CASE("PO escapes", "[i18n][po]")
{
	std::string out, error;
	CHECK(UnescapePoString("a\\tb\\\\c\\\"d\\n\\101\\x42", out, &error));
	CHECK(out == "a\tb\\c\"d\nAB");
	out.clear();
	CHECK_FALSE(UnescapePoString("bad\\q", out, &error));
	CHECK_FALSE(error.empty());
	out.clear();
	CHECK_FALSE(UnescapePoString("end\\", out, &error));
}

// -------------------------------------------------------
// Plural rules
// -------------------------------------------------------

TEST_CASE("Plural rules of the language table match CLDR's categories", "[i18n][plural]")
{
	const auto rule = [](const std::string_view tag)
	{
		const LanguageInfo *language = FindLanguage(tag);
		REQUIRE(language != nullptr);
		std::string error;
		const auto parsed = PluralRule::Parse(language->pluralForms, &error);
		INFO(error);
		REQUIRE(parsed.has_value());
		return *parsed;
	};

	const auto russian = [](const std::uint64_t n) -> std::size_t
	{
		if (n % 10 == 1 && n % 100 != 11)
			return 0;
		if (n % 10 >= 2 && n % 10 <= 4 && (n % 100 < 12 || n % 100 > 14))
			return 1;
		return 2;
	};
	const auto polish = [](const std::uint64_t n) -> std::size_t
	{
		if (n == 1)
			return 0;
		if (n % 10 >= 2 && n % 10 <= 4 && (n % 100 < 12 || n % 100 > 14))
			return 1;
		return 2;
	};
	const auto arabic = [](const std::uint64_t n) -> std::size_t
	{
		if (n == 0) return 0;
		if (n == 1) return 1;
		if (n == 2) return 2;
		if (n % 100 >= 3 && n % 100 <= 10) return 3;
		if (n % 100 >= 11) return 4;
		return 5;
	};

	const PluralRule en = rule("en"), fr = rule("fr"), de = rule("de"), ru = rule("ru"), uk = rule("uk"), pl = rule("pl"), cs = rule("cs"),
		ro = rule("ro"), ar = rule("ar"), ja = rule("ja"), zh = rule("zh_Hans");
	std::vector<std::uint64_t> values;
	for (std::uint64_t n = 0; n <= 200; ++n)
		values.push_back(n);
	values.push_back(1000000);
	values.push_back(1000001);
	for (const std::uint64_t n : values)
	{
		INFO("n = " << n);
		CHECK(en.Index(n) == (n == 1 ? 0u : 1u));
		CHECK(de.Index(n) == (n == 1 ? 0u : 1u));
		CHECK(fr.Index(n) == (n > 1 ? 1u : 0u));	// 0 is singular in French
		CHECK(ru.Index(n) == russian(n));
		CHECK(uk.Index(n) == russian(n));
		CHECK(pl.Index(n) == polish(n));
		CHECK(cs.Index(n) == (n == 1 ? 0u : (n >= 2 && n <= 4) ? 1u : 2u));
		CHECK(ro.Index(n) == (n == 1 ? 0u : (n == 0 || (n % 100 > 0 && n % 100 < 20)) ? 1u : 2u));
		CHECK(ar.Index(n) == arabic(n));
		CHECK(ja.Index(n) == 0u);
		CHECK(zh.Index(n) == 0u);
	}
	CHECK(ru.Count() == 3);
	CHECK(ar.Count() == 6);
	CHECK(ja.Count() == 1);
}

TEST_CASE("Plural expressions: syntax, errors and edge cases", "[i18n][plural]")
{
	std::string error;
	const auto index = [](const std::optional<PluralRule> &rule, const std::uint64_t n) -> std::size_t
	{
		REQUIRE(rule.has_value());
		return rule->Index(n);
	};
	CHECK(index(PluralRule::Parse("nplurals=2; plural=n != 1;"), 1) == 0);
	CHECK(index(PluralRule::Parse("nplurals = 2 ; plural = ( n != 1 )"), 5) == 1);
	CHECK(index(PluralRule::ParseExpression(3, "n / 0 + n % 0"), 7) == 0);	// Division by zero gives 0
	CHECK(index(PluralRule::ParseExpression(2, "n"), 1000) == 1);			// Clamped to nplurals - 1
	CHECK(index(PluralRule::ParseExpression(3, "!(n == 1) + !!n"), 5) == 2);
	CHECK(index(PluralRule::ParseExpression(4, "n <= 2 ? n >= 1 ? n : 3 : 2"), 0) == 3);
	CHECK(index(PluralRule::ParseExpression(4, "n<2||n>3&&n<5"), 4) == 1);

	CHECK_FALSE(PluralRule::Parse("plural=n;", &error));
	CHECK_FALSE(PluralRule::Parse("nplurals=2; plural=(n != 1", &error));
	CHECK_FALSE(PluralRule::Parse("nplurals=2; plural=n ? 1;", &error));
	CHECK_FALSE(PluralRule::Parse("nplurals=0; plural=0;", &error));
	CHECK_FALSE(PluralRule::Parse("nplurals=2; plural=x;", &error));
	CHECK_FALSE(error.empty());
	CHECK_FALSE(PluralRule::ParseExpression(2, std::string(500, '(') + "n" + std::string(500, ')'), &error));	// Too deep, no crash
	CHECK_FALSE(PluralRule::ParseExpression(2, std::string(500, '!') + "n", &error));
	CHECK(PluralRule().Index(1) == 0);
	CHECK(PluralRule().Index(2) == 1);
}

// -------------------------------------------------------
// Languages
// -------------------------------------------------------

TEST_CASE("Language table rows are consistent", "[i18n][languages]")
{
	for (const LanguageInfo &language : GetKnownLanguages())
	{
		INFO(language.tag);
		CHECK(NormalizeTag(language.tag) == language.tag);
		CHECK(FindLanguage(language.tag) == &language);
		const auto rule = PluralRule::Parse(language.pluralForms);
		REQUIRE(rule.has_value());
		CHECK(rule->Count() == language.pluralCount);
		CHECK_FALSE(language.NativeName().empty());
	}
	CHECK(FindLanguage("pt-br") == FindLanguage("pt_BR"));
	CHECK(FindLanguage("ZH-hans")->script == Script::HanSimplified);
	CHECK(FindLanguage("ja")->NeedsCjkFont());
	CHECK_FALSE(FindLanguage("de")->NeedsCjkFont());
	CHECK(FindLanguage("ja")->NativeName() == U8(u8"日本語"));
	CHECK(FindLanguage("xx") == nullptr);
}

TEST_CASE("Tags normalize and match the user's preferred languages", "[i18n][languages]")
{
	CHECK(NormalizeTag("pt-br") == "pt_BR");
	CHECK(NormalizeTag("ZH-hans") == "zh_Hans");
	CHECK(NormalizeTag("sr-latn-rs") == "sr_Latn_RS");
	CHECK(NormalizeTag("de_DE.UTF-8@euro") == "de_DE");
	CHECK(NormalizeTag("es-419") == "es_419");
	CHECK(BaseLanguage("pt_BR") == "pt");
	CHECK(PosixLocaleToBcp47("de_DE.UTF-8@euro") == "de-DE");
	CHECK(PosixLocaleToBcp47("C") == "en");
	CHECK(PosixLocaleToBcp47("POSIX") == "en");
	CHECK(PosixLocaleToBcp47("C.UTF-8") == "en");

	const std::vector<std::string> available = { "de", "fr", "ja", "pt_BR", "zh_Hans", "zh_Hant" };
	const auto match = [&](std::vector<std::string> preferred) { return MatchLanguage(preferred, available); };
	CHECK(match({ "de-DE" }) == "de");
	CHECK(match({ "de_AT.UTF-8" }) == "de");
	CHECK(match({ "pt-PT" }) == "pt_BR");			// Any variant of the language
	CHECK(match({ "pt" }) == "pt_BR");
	CHECK(match({ "zh-CN" }) == "zh_Hans");
	CHECK(match({ "zh-SG" }) == "zh_Hans");
	CHECK(match({ "zh-TW" }) == "zh_Hant");
	CHECK(match({ "zh-HK" }) == "zh_Hant");
	CHECK(match({ "zh-Hant-CN" }) == "zh_Hant");	// Script wins over region
	CHECK(match({ "zh" }) == "zh_Hans");
	CHECK(match({ "en-US", "de-DE" }) == "en");	// English first: the source language
	CHECK(match({ "it-IT", "fr-CA" }) == "fr");	// First one that has a catalog
	CHECK(match({ "ko-KR" }) == "en");
	CHECK(match({}) == "en");
	CHECK(match({ "C" }) == "en");
	CHECK(MatchLanguage(std::vector<std::string> { "zh-TW" }, std::vector<std::string> { "zh_Hans" }) == "en");	// Never across scripts
	CHECK(MatchLanguage(std::vector<std::string> { "pt-BR" }, std::vector<std::string> { "pt", "pt_BR" }) == "pt_BR");
	CHECK(MatchLanguage(std::vector<std::string> { "pt-PT" }, std::vector<std::string> { "pt", "pt_BR" }) == "pt");
}

// -------------------------------------------------------
// Formatting
// -------------------------------------------------------

namespace
{
	template <typename... Args>
	std::string Fmt(const std::string_view pattern, const Args &...args)
	{
		const FormatArg packed[sizeof...(Args) + 1] = { FormatArg(args)..., FormatArg() };
		std::string out, error;
		if (!FormatTo(out, pattern, std::span<const FormatArg>(packed, sizeof...(Args)), &error))
			return "ERROR: " + error;
		return out;
	}
}

TEST_CASE("Formatting follows std::format's fields and specs", "[i18n][format]")
{
	CHECK(Fmt("plain") == "plain");
	CHECK(Fmt("{} and {}", 1, "two") == "1 and two");
	CHECK(Fmt("{1} before {0}", "a", "b") == "b before a");
	CHECK(Fmt("{0} {0}", 7) == "7 7");
	CHECK(Fmt("{{literal}} {}", 3) == "{literal} 3");
	CHECK(Fmt("{}", -42) == "-42");
	CHECK(Fmt("{}", std::uint64_t(18446744073709551615ull)) == "18446744073709551615");
	CHECK(Fmt("{}", std::int64_t(-9223372036854775807ll - 1)) == "-9223372036854775808");
	CHECK(Fmt("{:5}|{:<5}|{:^5}|{:>5}", 42, 42, 42, 42) == "   42|42   | 42  |   42");
	CHECK(Fmt("{:*^7}", "ab") == "**ab***");
	CHECK(Fmt("{:05}", -42) == "-0042");
	CHECK(Fmt("{:+}", 5) == "+5");
	CHECK(Fmt("{: }", 5) == " 5");
	CHECK(Fmt("{:x} {:X} {:#x} {:o} {:#o} {:b} {:#B}", 255, 255, 255, 8, 0, 5, 5) == "ff FF 0xff 10 0 101 0B101");
	CHECK(Fmt("{:#06x}", 255) == "0x00ff");
	CHECK(Fmt("{:c}", 65) == "A");

	CHECK(Fmt("{}", 0.1) == "0.1");
	CHECK(Fmt("{}", 0.1f) == "0.1");
	CHECK(Fmt("{}", 1234567.0) == "1234567");
	CHECK(Fmt("{}", 1e-7) == "1e-07");
	CHECK(Fmt("{:.2f}", 3.14159) == "3.14");
	CHECK(Fmt("{:.0f} m", 2.4) == "2 m");
	CHECK(Fmt("{:8.3f}|", -1.5) == "  -1.500|");
	CHECK(Fmt("{:08.2f}", -1.5) == "-0001.50");
	CHECK(Fmt("{:.3}", 1234.5) == "1.23e+03");
	CHECK(Fmt("{:e}", 1500.0) == "1.500000e+03");
	CHECK(Fmt("{:g}", 0.0001) == "0.0001");
	CHECK(Fmt("{}", -0.0) == "-0");

	CHECK(Fmt("{}", true) == "true");
	CHECK(Fmt("{:d}", true) == "1");
	CHECK(Fmt("{}", 'x') == "x");
	CHECK(Fmt("{:d}", 'A') == "65");
	CHECK(Fmt("{:.3}", "abcdef") == "abc");
	CHECK(Fmt("{:>4}", U8(u8"é")) == U8(u8"   é"));			// Width counts characters, not bytes
	CHECK(Fmt("{:.2}", U8(u8"日本語")) == U8(u8"日本"));
	CHECK(Fmt(U8(u8"{:·<4}"), "a") == U8(u8"a···"));	// A UTF-8 fill character
	CHECK(Fmt("{}", std::string("str")) == "str");
	CHECK(Fmt("{}", std::string_view("view")) == "view");
	CHECK(Fmt("{}", static_cast<const char *>(nullptr)).empty());
}

TEST_CASE("Formatting keeps '.' decimals whatever LC_NUMERIC is", "[i18n][format][locale]")
{
	// The editor never changes LC_NUMERIC, but the formatter mustn't depend on that: try a locale with a ',' decimal point
	const std::string saved = std::setlocale(LC_NUMERIC, nullptr) ? std::setlocale(LC_NUMERIC, nullptr) : "C";
	bool comma = false;
	for (const char *name : { "de_DE.UTF-8", "de_DE.utf8", "de_DE", "German_Germany.1252", "German", "fr_FR.UTF-8", "fr_FR" })
	{
		if (std::setlocale(LC_NUMERIC, name) && std::strcmp(std::localeconv()->decimal_point, ",") == 0)
		{
			comma = true;
			break;
		}
	}
	if (!comma)
	{
		std::setlocale(LC_NUMERIC, saved.c_str());
		SKIP("no locale with a ',' decimal point on this machine");
	}
	const std::string fixed = Fmt("{:.2f}", 3.14159), general = Fmt("{:g}", 0.5), exponent = Fmt("{:e}", 1500.0), hex = Fmt("{:a}", 1.5),
		shortest = Fmt("{}", 0.1);
	std::setlocale(LC_NUMERIC, saved.c_str());
	CHECK(fixed == "3.14");
	CHECK(general == "0.5");
	CHECK(exponent == "1.500000e+03");
	CHECK(hex.starts_with("1.8"));	// C libraries differ in the trailing zeros of %a; the point is the '.'
	CHECK(hex.find(',') == std::string::npos);
	CHECK(shortest == "0.1");
}

TEST_CASE("Formatting reports bad patterns instead of throwing", "[i18n][format]")
{
	CHECK(Fmt("{", 1).starts_with("ERROR"));
	CHECK(Fmt("}", 1).starts_with("ERROR"));
	CHECK(Fmt("{} {1}", 1, 2).starts_with("ERROR"));	// Mixed automatic and manual
	CHECK(Fmt("{2}", 1, 2).starts_with("ERROR"));		// No such argument
	CHECK(Fmt("{} {}", 1).starts_with("ERROR"));
	CHECK(Fmt("{:q}", 1).starts_with("ERROR"));
	CHECK(Fmt("{:.2}", 1).starts_with("ERROR"));		// Integers have no precision
	CHECK(Fmt("{:+}", "s").starts_with("ERROR"));
	CHECK(Fmt("{:d}", "s").starts_with("ERROR"));
	CHECK(Fmt("{:s}", 1.5).starts_with("ERROR"));
	CHECK(Fmt("{:99999}", 1).starts_with("ERROR"));
	CHECK(Fmt("{:.99999999f}", 1.0).starts_with("ERROR"));		// A translator's precision is capped (300)
	CHECK(Fmt("{:.301f}", 1.0).starts_with("ERROR"));
	CHECK(Fmt("{:.300f}", 1.0).size() == 302);
	CHECK(Fmt("{abc}", 1).starts_with("ERROR"));
	CHECK(Fmt("{:{}}", 1, 2).starts_with("ERROR"));	// Nested fields aren't supported

	std::vector<FormatField> fields;
	REQUIRE(ParseFormatFields("{} of {} ({:.1f}%)", fields));
	REQUIRE(fields.size() == 3);
	CHECK(fields[2].index == 2);
	CHECK(fields[2].spec == ".1f");
	REQUIRE(ParseFormatFields("{1} {0:>3}", fields));
	CHECK(fields[0].index == 1);
	CHECK(fields[1].spec == ">3");
	CHECK_FALSE(ParseFormatFields("{0} {}", fields));
}

// -------------------------------------------------------
// Pseudo-locale
// -------------------------------------------------------

TEST_CASE("Pseudo-locale accents letters and keeps fields, printf codes and IDs", "[i18n][pseudo]")
{
	const std::string save = PseudoLocalize("Save");
	CHECK(save.starts_with("[!! "));
	CHECK(save.ends_with("!!]"));
	CHECK(save.find(U8(u8"Šåvé")) != std::string::npos);	// Šåvé

	const std::string fields = PseudoLocalize("{} of {1:.2f} at %5.1f%% {{x}}");
	CHECK(fields.find("{} ") != std::string::npos);
	CHECK(fields.find("{1:.2f}") != std::string::npos);
	CHECK(fields.find("%5.1f%%") != std::string::npos);
	CHECK(fields.find("{{") != std::string::npos);

	const std::string id = PseudoLocalize("Open##recent");
	CHECK(id.ends_with("!!]##recent"));
	CHECK(PseudoLocalize("##only") == "##only");
	CHECK(PseudoLocalize("") == "");
	CHECK(PseudoLocalize("a\nb").find('\n') != std::string::npos);
}

// -------------------------------------------------------
// Catalogs and the service
// -------------------------------------------------------

TEST_CASE("Catalog keeps translated entries and checks plural forms", "[i18n][catalog]")
{
	std::vector<std::string> problems;
	const auto catalog = Catalog::FromPo(ParsePo(GERMAN), "de", &problems);
	CHECK(problems.empty());
	CHECK(catalog->GetStats().total == 9);
	CHECK(catalog->GetStats().translated == 7);	// Not the fuzzy one, nor the empty one
	CHECK(catalog->GetStats().fuzzy == 1);
	CHECK(*catalog->Find(HashKey({}, false, "Save"), {}, false, "Save") == "Speichern");
	CHECK(catalog->Find(HashKey({}, false, "Cancel"), {}, false, "Cancel") == nullptr);
	CHECK(catalog->Find(HashKey({}, false, "Save"), {}, false, "Other") == nullptr);	// A key collision isn't a match
	CHECK(*catalog->FindPlural(HashKey({}, false, "{} file"), {}, false, "{} file", 3) == "{} Dateien");

	// nplurals that disagrees with the language: the table's rule, and plural entries with the wrong count stay English
	problems.clear();
	const auto wrong = Catalog::FromPo(ParsePo(
		"msgid \"\"\nmsgstr \"Plural-Forms: nplurals=2; plural=(n != 1);\\n\"\n\n"
		"msgid \"a\"\nmsgid_plural \"b\"\nmsgstr[0] \"x\"\nmsgstr[1] \"y\"\n\n"
		"msgid \"c\"\nmsgstr \"d\"\nmsgid \"c\"\nmsgstr \"e\"\n"), "ru", &problems);
	CHECK(wrong->GetPluralRule().Count() == 3);
	CHECK(wrong->FindPlural(HashKey({}, false, "a"), {}, false, "a", 1) == nullptr);
	CHECK(*wrong->Find(HashKey({}, false, "c"), {}, false, "c") == "d");	// The first of two duplicates
	CHECK(problems.size() == 3);
	CHECK(wrong->GetStats().total == 2);		// The duplicate counts once
	CHECK(wrong->GetStats().translated == 1);	// The plural entry with the wrong form count stays English

	// A duplicate counts once whether or not either copy is translated; the same msgid with and without a context
	// are two messages
	const auto duplicates = Catalog::FromPo(ParsePo(
		"msgid \"x\"\nmsgstr \"\"\n\nmsgid \"x\"\nmsgstr \"X\"\n\n"
		"msgctxt \"\"\nmsgid \"x\"\nmsgstr \"Y\"\n\nmsgctxt \"x\"\nmsgid \"\"\nmsgstr \"Z\"\n"), "de");
	CHECK(duplicates->GetStats().total == 3);
	CHECK(duplicates->GetStats().translated == 2);
	CHECK(duplicates->GetStats().Completion() < 1.0f);
}

TEST_CASE("Reload keeps the catalog in use when a file can't be read or hasn't changed", "[i18n][service]")
{
	ServiceGuard guard;
	auto text = std::make_shared<std::optional<std::string>>(std::string("msgid \"Save\"\nmsgstr \"Sichern\"\n"));
	std::vector<CatalogFile> files;
	files.push_back({ "de", "memory:de.po", [text] { return *text; }, false });
	SetCatalogFiles(std::move(files));
	REQUIRE(SetLanguage("de"));
	const char *before = Tr("Save");
	CHECK(std::string(before) == "Sichern");

	// Unchanged: the same catalog, so the same string
	REQUIRE(Reload());
	CHECK(Tr("Save") == before);

	// Unreadable: Reload fails, the catalog read before stays in use and the language isn't listed as failed
	*text = std::nullopt;
	CHECK_FALSE(Reload());
	CHECK(GetLanguage() == "de");
	CHECK(Tr("Save") == before);
	const std::vector<LanguageChoice> choices = GetLanguageChoices();
	REQUIRE(choices.size() == 1);
	CHECK_FALSE(choices[0].failed);
	CHECK(choices[0].stats.translated == 1);

	// Edited: the new translation, and the old string stays valid
	*text = std::string("msgid \"Save\"\nmsgstr \"Speichern\"\n");
	REQUIRE(Reload());
	CHECK(std::string(Tr("Save")) == "Speichern");
	CHECK(std::string(before) == "Sichern");

	// The editor's Reload Translations lists the folders again (SetCatalogFiles) before Reload: the same files keep what
	// was read from them, so an unchanged file keeps its catalog and an unreadable one keeps the version in use
	const auto listFiles = [text]
	{
		std::vector<CatalogFile> list;
		list.push_back({ "de", "memory:de.po", [text] { return *text; }, false });
		return list;
	};
	const char *speichern = Tr("Save");
	SetCatalogFiles(listFiles());
	REQUIRE(Reload());
	CHECK(Tr("Save") == speichern);
	*text = std::nullopt;
	SetCatalogFiles(listFiles());
	CHECK_FALSE(Reload());
	CHECK(GetLanguage() == "de");
	CHECK(Tr("Save") == speichern);
	CHECK_FALSE(GetLanguageChoices().front().failed);

	// A translator's copy that appears is a different file: read
	std::vector<CatalogFile> withUser = listFiles();
	withUser.push_back({ "de", "user:de.po", [] { return std::optional<std::string>("msgid \"Save\"\nmsgstr \"Sichern!\"\n"); }, true });
	SetCatalogFiles(std::move(withUser));
	REQUIRE(Reload());
	CHECK(std::string(Tr("Save")) == "Sichern!");
	*text = std::string("msgid \"Save\"\nmsgstr \"Speichern\"\n");
	SetCatalogFiles(listFiles());	// And gone again: the bundled file
	REQUIRE(Reload());
	CHECK(std::string(Tr("Save")) == "Speichern");

	// Switching away and back reuses what was loaded
	REQUIRE(SetLanguage("en"));
	const char *english = Tr("Save");
	REQUIRE(SetLanguage("de"));
	CHECK(std::string(Tr("Save")) == "Speichern");
	REQUIRE(SetLanguage("en"));
	CHECK(Tr("Save") == english);
}

TEST_CASE("Translation service: languages, contexts, plurals, formatting", "[i18n][service]")
{
	ServiceGuard guard;
	std::vector<std::string> logged;
	SetLogSink([&logged](LogLevel, const std::string_view message) { logged.emplace_back(message); });
	UseCatalogs({ { "de", GERMAN } });

	CHECK(GetLanguage() == "en");
	CHECK(std::string(Tr("Save")) == "Save");
	CHECK(GetAvailableTags() == std::vector<std::string> { "de" });

	const std::uint64_t before = GetGeneration();
	REQUIRE(SetLanguage("de"));
	CHECK(GetGeneration() != before);
	CHECK(GetLanguage() == "de");
	CHECK(GetLanguageInfo() == FindLanguage("de"));

	CHECK(std::string(Tr("Save")) == "Speichern");
	CHECK(std::string(Tr(Ctx("verb", "Open"))) == U8(u8"Öffnen"));
	CHECK(std::string(Tr(Ctx("state", "Open"))) == "Offen");
	CHECK(std::string(Tr("Open")) == "Open");						// No entry without a context
	CHECK(std::string(Tr("Cancel")) == "Cancel");					// Fuzzy: English
	CHECK(std::string(Tr("Untranslated")) == "Untranslated");
	CHECK(std::string(Tr("PROPERTIES###Properties")) == "EIGENSCHAFTEN");	// Display text drops the ID
	CHECK(std::string(Tr("##zoom")) == "##zoom");

	CHECK(std::string(TrN("{} file", "{} files", 1)) == "{} Datei");
	CHECK(TrNF("{} file", "{} files", 1) == "1 Datei");
	CHECK(TrNF("{} file", "{} files", 0) == "0 Dateien");
	CHECK(TrNF("{} file", "{} files", -1) == "-1 Datei");
	CHECK(TrF("{0} of {1}", 3, 7) == "7: 3");

	// A translation that doesn't format falls back to English, reported once
	logged.clear();
	CHECK(TrF("Broken {}", 5) == "Broken 5");
	CHECK(TrF("Broken {}", 6) == "Broken 6");
	CHECK(std::count_if(logged.begin(), logged.end(), [](const std::string &m) { return m.find("Broken") != std::string::npos; }) == 1);

	// English formatting and plurals without a catalog
	REQUIRE(SetLanguage("en"));
	CHECK(TrNF("{} file", "{} files", 1) == "1 file");
	CHECK(TrNF("{} file", "{} files", 2) == "2 files");
	CHECK(TrF("{:.1f} m", 2.26) == "2.3 m");
	CHECK(TrF("Bad {", 1) == "Bad {");	// A broken English pattern is shown as written

	CHECK_FALSE(SetLanguage("xx"));
	CHECK(GetLanguage() == "en");
}

TEST_CASE("Translation service: fallbacks, overrides, pseudo-locale, stable pointers", "[i18n][service]")
{
	ServiceGuard guard;
	const std::string portuguese = "msgid \"Save\"\nmsgstr \"Guardar\"\n\nmsgid \"Close\"\nmsgstr \"Fechar\"\n";
	const std::string brazilian = "msgid \"Save\"\nmsgstr \"Salvar\"\n";
	UseCatalogs({ { "pt", portuguese }, { "pt-BR", brazilian } });
	REQUIRE(SetLanguage("pt_BR"));
	CHECK(std::string(Tr("Save")) == "Salvar");
	CHECK(std::string(Tr("Close")) == "Fechar");	// From the base language
	CHECK(std::string(Tr("Other")) == "Other");

	// A translator's file replaces the bundled one, whatever order they come in
	std::vector<CatalogFile> files;
	files.push_back({ "de", "user", [] { return std::optional<std::string>("msgid \"Save\"\nmsgstr \"Sichern\"\n"); }, true });
	files.push_back({ "de", "bundled", [] { return std::optional<std::string>(GERMAN); }, false });
	files.push_back({ "fr", "missing", [] { return std::optional<std::string>(); }, false });
	SetCatalogFiles(std::move(files));
	REQUIRE(SetLanguage("de"));
	const char *sichern = Tr("Save");
	CHECK(std::string(sichern) == "Sichern");
	CHECK_FALSE(SetLanguage("fr"));	// Unreadable: stays German
	CHECK(GetLanguage() == "de");

	const auto choices = GetLanguageChoices();
	REQUIRE(choices.size() == 2);
	CHECK(choices[0].tag == "de");
	CHECK(choices[0].user);
	CHECK(choices[0].nativeName == "Deutsch");
	CHECK(choices[1].failed);

	// Reload re-reads the file; the old translation's pointer stays valid
	REQUIRE(Reload());
	CHECK(std::string(sichern) == "Sichern");

	REQUIRE(SetLanguage("qps-ploc"));
	const std::string pseudo = Tr("Save");
	CHECK(pseudo.starts_with("[!! "));
	CHECK(std::string(TrN("{} file", "{} files", 2)).find("{}") != std::string::npos);
	CHECK(TrNF("{} file", "{} files", 2).find("2 ") != std::string::npos);
	CHECK(Tr("Save") == Tr("Save"));	// Cached
	CHECK(std::string(sichern) == "Sichern");
}

TEST_CASE("A catalog that couldn't be read is tried again when the files are listed again", "[i18n][service]")
{
	ServiceGuard guard;
	auto text = std::make_shared<std::optional<std::string>>();
	const auto files = [text]
	{
		std::vector<CatalogFile> list;
		list.push_back({ "de", "memory:de.po", [text] { return *text; }, false });
		return list;
	};
	SetCatalogFiles(files());
	CHECK_FALSE(SetLanguage("de"));
	CHECK(GetLanguageChoices().front().failed);

	*text = std::string("msgid \"Save\"\nmsgstr \"Speichern\"\n");
	SetCatalogFiles(files());
	REQUIRE(SetLanguage("de"));
	CHECK(std::string(Tr("Save")) == "Speichern");
}

TEST_CASE("Translation service records missing strings", "[i18n][service]")
{
	ServiceGuard guard;
	UseCatalogs({ { "de", GERMAN } });
	REQUIRE(SetLanguage("de"));
	SetRecordMissing(true);
	Tr("Untranslated");
	Tr("Untranslated");
	Tr(Ctx("menu", "Quit \"now\""));
	TrN("{} tree", "{} trees", 2);
	Tr("Save");	// Translated: not recorded
	const auto missing = GetMissing();
	REQUIRE(missing.size() == 3);
	CHECK(missing[0].msgid == "Untranslated");
	CHECK(missing[0].count == 2);
	const std::string po = MissingAsPo();
	CHECK(po.find("msgctxt \"menu\"\nmsgid \"Quit \\\"now\\\"\"") != std::string::npos);
	CHECK(po.find("msgid_plural \"{} trees\"") != std::string::npos);
	const PoFile parsed = ParsePo(po);
	CHECK(parsed.errors.empty());
	CHECK(parsed.entries.size() == 3);
}

TEST_CASE("Text translates literals and shows other strings as they are", "[i18n][text]")
{
	ServiceGuard guard;
	UseCatalogs({ { "de", GERMAN } });
	REQUIRE(SetLanguage("de"));

	const auto show = [](const Text &text) { return std::string(text.Get()); };
	CHECK(show("Save") == "Speichern");
	CHECK(show(Msg("Save")) == "Speichern");
	CHECK(show(Ctx("verb", "Open")) == U8(u8"Öffnen"));

	const char *dynamic = "Save";
	CHECK(show(dynamic) == "Save");
	CHECK(show(std::string("Save")) == "Save");
	char buffer[16] = "Save";
	CHECK(show(buffer) == "Save");
	CHECK(show(nullptr).empty());
	CHECK(show(static_cast<const char *>(nullptr)).empty());
	CHECK(Text().IsEmpty());
	CHECK(Text("Save").IsTranslated());
	CHECK_FALSE(Text(dynamic).IsTranslated());
	CHECK(std::string(Text("Save").Source()) == "Save");
}

TEST_CASE("Lookups are safe while the language changes on another thread", "[i18n][service][thread]")
{
	ServiceGuard guard;
	UseCatalogs({ { "de", GERMAN } });
	std::atomic<bool> stop { false };
	std::atomic<std::uint64_t> lookups { 0 };
	std::vector<std::thread> readers;
	for (int t = 0; t < 4; ++t)
	{
		readers.emplace_back([&]
		{
			while (!stop.load())
			{
				const std::string save = Tr("Save");
				const bool known = save == "Save" || save == "Speichern" || save.starts_with("[!! ");
				if (!known)
					std::abort();
				(void)TrNF("{} file", "{} files", 3);
				lookups.fetch_add(1);
			}
		});
	}
	// At least 300 switches, and until the readers have looked up plenty while it switched (or 10 s went by)
	const auto start = std::chrono::steady_clock::now();
	for (int i = 0; i < 300 || (lookups.load() < 20000 && std::chrono::steady_clock::now() - start < std::chrono::seconds(10)); ++i)
		SetLanguage(i % 3 == 0 ? "de" : i % 3 == 1 ? "qps-ploc" : "en");
	stop = true;
	for (std::thread &reader : readers)
		reader.join();
	CHECK(lookups.load() > 0);
}

TEST_CASE("A 5,000-entry catalog loads quickly", "[i18n][catalog][speed]")
{
	std::string po = "msgid \"\"\nmsgstr \"Language: de\\nPlural-Forms: nplurals=2; plural=(n != 1);\\n\"\n\n";
	for (int i = 0; i < 5000; ++i)
	{
		po += "#. A translator comment for entry " + std::to_string(i) + "\n#: source/Editor/panels/some_panel.cpp\n";
		if (i % 10 == 0)
			po += "msgid \"{} thing " + std::to_string(i) + "\"\nmsgid_plural \"{} things\"\nmsgstr[0] \"{} Ding\"\nmsgstr[1] \"{} Dinge\"\n\n";
		else
			po += "msgid \"Some user interface text number " + std::to_string(i) + "\"\nmsgstr \"Ein Text der Benutzeroberfl\\303\\244che " + std::to_string(i) + "\"\n\n";
	}
	const auto start = std::chrono::steady_clock::now();
	const auto catalog = Catalog::FromPo(ParsePo(po), "de");
	const double ms = std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - start).count();
	CHECK(catalog->GetStats().translated == 5000);
	WARN("5,000-entry catalog parsed in " << ms << " ms");
	CHECK(ms < 500.0);	// The target is 20 ms in a release build; this leaves room for slow CI machines and debug builds
}
