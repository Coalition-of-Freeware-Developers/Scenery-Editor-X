/**
 * -------------------------------------------------------
 * Scenery Editor X
 * -------------------------------------------------------
 * Copyright (c) 2026 Thomas Ray
 * Copyright (c) 2026 Coalition of Freeware Developers
 * -------------------------------------------------------
 * FontCoverageTests.cpp
 * -------------------------------------------------------
 * The glyph coverage check that decides which font is merged into the UI fonts for a language
 * (i18n/font_coverage.h): cmap formats 4 and 12 in hand-built fonts and collections, the bundled Open Sans when the
 * tests run from the repository, and damaged data.
 * -------------------------------------------------------
 */
#include <catch2/catch_all.hpp>
#include <SceneryEditorX/i18n/font_coverage.h>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <random>
#include <tuple>
#include <vector>
#include <string>

using namespace SceneryEditorX::I18n;

namespace
{
	/* @brief Big-endian bytes, as fonts store them. */
	struct Bytes
	{
		std::string data;

		void U16(const std::uint32_t value)
		{
			data += static_cast<char>((value >> 8) & 0xFF);
			data += static_cast<char>(value & 0xFF);
		}
		void U32(const std::uint32_t value)
		{
			U16(value >> 16);
			U16(value & 0xFFFF);
		}
		void Tag(const char *tag) { data.append(tag, 4); }
		void Append(const std::string &more) { data += more; }
	};

	/**
	 * @brief A format 4 subtable: A..C through a delta (glyphs 1..3), x..y through the glyph array ('x' -> 5, 'y' -> none),
	 * and the 0xFFFF end segment.
	 */
	std::string Format4()
	{
		Bytes b;
		const std::uint32_t segments = 3;
		b.U16(4);
		b.U16(14 + 8 * segments + 2 + 4);	// length: header, four arrays + pad, glyph array of 2
		b.U16(0);
		b.U16(segments * 2);
		b.U16(4); b.U16(1); b.U16(2);		// searchRange, entrySelector, rangeShift (not read)
		b.U16('C'); b.U16('y'); b.U16(0xFFFF);			// endCode
		b.U16(0);										// reservedPad
		b.U16('A'); b.U16('x'); b.U16(0xFFFF);			// startCode
		b.U16((1 - 'A') & 0xFFFF); b.U16(0); b.U16(1);	// idDelta
		b.U16(0); b.U16(4); b.U16(0);					// idRangeOffset: the second points at the glyph array
		b.U16(5); b.U16(0);								// glyphIdArray
		return b.data;
	}

	/* @brief A format 12 subtable: U+65E5 -> glyph 10, U+1F600 -> glyph 11. */
	std::string Format12()
	{
		Bytes b;
		b.U16(12);
		b.U16(0);
		b.U32(16 + 2 * 12);
		b.U32(0);
		b.U32(2);
		b.U32(0x65E5); b.U32(0x65E5); b.U32(10);
		b.U32(0x1F600); b.U32(0x1F600); b.U32(11);
		return b.data;
	}

	/* @brief A font with only a cmap table, laid out at `base` in its file, with the given (platform, encoding, subtable). */
	std::string Font(const std::size_t base, const std::vector<std::tuple<int, int, std::string>> &subtables)
	{
		Bytes cmap;
		cmap.U16(0);
		cmap.U16(static_cast<std::uint32_t>(subtables.size()));
		std::uint32_t offset = 4 + 8 * static_cast<std::uint32_t>(subtables.size());
		std::string bodies;
		for (const auto &[platform, encoding, subtable] : subtables)
		{
			cmap.U16(static_cast<std::uint32_t>(platform));
			cmap.U16(static_cast<std::uint32_t>(encoding));
			cmap.U32(offset);
			offset += static_cast<std::uint32_t>(subtable.size());
			bodies += subtable;
		}
		cmap.Append(bodies);

		Bytes font;
		font.U32(0x00010000);
		font.U16(1);
		font.U16(16); font.U16(0); font.U16(0);
		font.Tag("cmap");
		font.U32(0);
		font.U32(static_cast<std::uint32_t>(base + 28));
		font.U32(static_cast<std::uint32_t>(cmap.data.size()));
		font.Append(cmap.data);
		return font.data;
	}

	std::string ReadFile(const std::filesystem::path &path)
	{
		std::ifstream file(path, std::ios::binary);
		return std::string(std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>());
	}

	/* @brief resources/fonts/<name> from the working folder or one of its parents (the repository root). */
	std::filesystem::path FindBundledFont(const char *name)
	{
		std::error_code error;
		std::filesystem::path folder = std::filesystem::current_path(error);
		for (int up = 0; up < 6 && !folder.empty(); ++up)
		{
			const std::filesystem::path candidate = folder / "resources" / "fonts" / name;
			if (std::filesystem::is_regular_file(candidate, error))
				return candidate;
			if (folder == folder.parent_path())
				break;
			folder = folder.parent_path();
		}
		return {};
	}
}

TEST_CASE("UTF-8 decodes to code points, bad bytes to U+FFFD", "[i18n][fonts]")
{
	CHECK(DecodeUtf8("Ab") == U"Ab");
	CHECK(DecodeUtf8("\xC3\xA9\xE6\x97\xA5\xF0\x9F\x98\x80") == U"é日\U0001F600");
	CHECK(DecodeUtf8("a\xFF" "b") == U"a�" U"b");
	CHECK(DecodeUtf8("\xE6\x97") == U"��");		// Cut off
	CHECK(DecodeUtf8("\xC3(") == U"�(");				// Not a continuation byte
	CHECK(DecodeUtf8("").empty());
}

TEST_CASE("Glyph coverage from cmap formats 4 and 12", "[i18n][fonts]")
{
	SECTION("Format 4")
	{
		const std::string font = Font(0, { { 3, 1, Format4() } });
		CHECK(CountMissingGlyphs(font, 0, U"ABC") == 0u);
		CHECK(CountMissingGlyphs(font, 0, U"ABCD") == 1u);
		CHECK(CountMissingGlyphs(font, 0, U"x") == 0u);		// Through the glyph array
		CHECK(CountMissingGlyphs(font, 0, U"y") == 1u);		// Glyph array entry 0: no glyph
		CHECK(CountMissingGlyphs(font, 0, U"@z日\U0001F600") == 4u);
		CHECK(CountMissingGlyphs(font, 0, U"") == 0u);
		CHECK_FALSE(CountMissingGlyphs(font, 1, U"A"));		// Not a collection: only face 0
	}
	SECTION("Format 12 wins over format 4")
	{
		const std::string font = Font(0, { { 3, 1, Format4() }, { 3, 10, Format12() } });
		CHECK(CountMissingGlyphs(font, 0, U"日\U0001F600") == 0u);
		CHECK(CountMissingGlyphs(font, 0, U"A") == 1u);		// Only the format 12 map is read
	}
	SECTION("Unicode platform subtables count; a Macintosh-only map doesn't")
	{
		CHECK(CountMissingGlyphs(Font(0, { { 0, 3, Format4() } }), 0, U"AB") == 0u);
		CHECK_FALSE(CountMissingGlyphs(Font(0, { { 1, 0, Format4() } }), 0, U"A"));
	}
}

TEST_CASE("Glyph coverage of a face in a collection", "[i18n][fonts]")
{
	// 'ttcf', version, 2 faces, their offsets; face 0 has only format 4, face 1 only format 12
	const std::size_t header = 12 + 8;
	const std::string first = Font(header, { { 3, 1, Format4() } });
	const std::string second = Font(header + first.size(), { { 3, 10, Format12() } });
	Bytes ttc;
	ttc.Tag("ttcf");
	ttc.U32(0x00010000);
	ttc.U32(2);
	ttc.U32(static_cast<std::uint32_t>(header));
	ttc.U32(static_cast<std::uint32_t>(header + first.size()));
	ttc.Append(first);
	ttc.Append(second);

	CHECK(CountMissingGlyphs(ttc.data, 0, U"AB日") == 1u);
	CHECK(CountMissingGlyphs(ttc.data, 1, U"AB日") == 2u);
	CHECK_FALSE(CountMissingGlyphs(ttc.data, 2, U"A"));
	CHECK_FALSE(CountMissingGlyphs(ttc.data, -1, U"A"));
}

TEST_CASE("Damaged font data never crashes", "[i18n][fonts]")
{
	const std::string font = Font(0, { { 3, 1, Format4() }, { 3, 10, Format12() } });
	CHECK_FALSE(CountMissingGlyphs("", 0, U"A"));
	CHECK_FALSE(CountMissingGlyphs("not a font at all", 0, U"A"));
	for (std::size_t size = 0; size < font.size(); ++size)
		(void)CountMissingGlyphs(std::string_view(font).substr(0, size), 0, U"AxQ日\U0001F600");

	std::mt19937 random(1234);
	for (int round = 0; round < 2000; ++round)
	{
		std::string damaged = font;
		for (int k = 0; k < 4; ++k)
			damaged[random() % damaged.size()] = static_cast<char>(random() & 0xFF);
		(void)CountMissingGlyphs(damaged, static_cast<int>(random() % 2), U"AxQ日\U0001F600");
	}
	SUCCEED("no crash");
}

TEST_CASE("The bundled Open Sans covers Latin, Greek and Cyrillic but not CJK", "[i18n][fonts]")
{
	const std::filesystem::path path = FindBundledFont("opensans/OpenSans-Regular.ttf");
	if (path.empty())
		SKIP("resources/fonts not found from the working folder (run from the repository)");
	const std::string font = ReadFile(path);
	REQUIRE(font.size() > 1000);
	CHECK(CountMissingGlyphs(font, 0, DecodeUtf8("Save \xC3\xA9\xC3\x9F\xC3\xB1 \xCE\xB1\xCE\xB2 \xD0\x96\xD1\x8F")) == 0u);	// é ß ñ α β Ж я
	CHECK(CountMissingGlyphs(font, 0, U"日本語") == 3u);	// 日本語
}
