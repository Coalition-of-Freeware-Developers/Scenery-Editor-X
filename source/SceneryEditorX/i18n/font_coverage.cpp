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
 * font_coverage.cpp
 * -------------------------------------------------------
 * Created: 14/11/2026
 * -------------------------------------------------------
 */
#include "font_coverage.h"
#include <cstdint>

// -------------------------------------------------------

namespace SceneryEditorX::I18n
{
	namespace
	{
		/* Big-endian reads with bounds checks: font files come from the user's machine and may be anything. */
		struct Reader
		{
			std::string_view data;

			[[nodiscard]] bool Has(const std::size_t offset, const std::size_t size) const
			{
				return offset <= data.size() && size <= data.size() - offset;
			}

			[[nodiscard]] std::uint32_t U16(const std::size_t offset) const
			{
				return (static_cast<std::uint32_t>(static_cast<unsigned char>(data[offset])) << 8) |
					static_cast<std::uint32_t>(static_cast<unsigned char>(data[offset + 1]));
			}

			[[nodiscard]] std::uint32_t U32(const std::size_t offset) const
			{
				return (U16(offset) << 16) | U16(offset + 2);
			}
		};

		enum class Format
		{
			Segments,	// Format 4: the Basic Multilingual Plane
			Groups		// Format 12: all of Unicode
		};

		struct CharacterMap
		{
			Format format = Format::Segments;
			std::size_t offset = 0;
		};

		/* @brief Glyph index of a code point in a format 4 subtable (0: none). */
		std::uint32_t LookupSegments(const Reader &r, const std::size_t base, const char32_t c)
		{
			if (c > 0xFFFF)
				return 0;
			const std::size_t segX2 = r.U16(base + 6);
			const std::size_t segments = segX2 / 2;
			const std::size_t ends = base + 14;
			const std::size_t starts = ends + segX2 + 2;
			const std::size_t deltas = starts + segX2;
			const std::size_t rangeOffsets = deltas + segX2;
			if (!r.Has(rangeOffsets, segX2))
				return 0;
			// Segments are sorted by end code: the first that ends at or after c is the only one that can hold it
			for (std::size_t i = 0; i < segments; ++i)
			{
				if (c > r.U16(ends + 2 * i))
					continue;
				const std::uint32_t start = r.U16(starts + 2 * i);
				if (c < start)
					return 0;
				const std::uint32_t delta = r.U16(deltas + 2 * i);
				const std::uint32_t rangeOffset = r.U16(rangeOffsets + 2 * i);
				if (rangeOffset == 0)
					return (static_cast<std::uint32_t>(c) + delta) & 0xFFFF;
				const std::size_t at = rangeOffsets + 2 * i + rangeOffset + 2 * (static_cast<std::size_t>(c) - start);
				if (!r.Has(at, 2))
					return 0;
				const std::uint32_t glyph = r.U16(at);
				return glyph == 0 ? 0 : (glyph + delta) & 0xFFFF;
			}
			return 0;
		}

		/* @brief Glyph index of a code point in a format 12 subtable (0: none). */
		std::uint32_t LookupGroups(const Reader &r, const std::size_t base, const char32_t c)
		{
			const std::uint32_t groups = r.U32(base + 12);
			for (std::uint32_t k = 0; k < groups; ++k)
			{
				const std::size_t group = base + 16 + 12 * static_cast<std::size_t>(k);
				if (!r.Has(group, 12))
					return 0;
				const std::uint32_t first = r.U32(group), last = r.U32(group + 4);
				if (c >= first && c <= last)
					return r.U32(group + 8) + (static_cast<std::uint32_t>(c) - first);
			}
			return 0;
		}

		/* @brief The best Unicode character map of the face at fontOffset: format 12 over format 4. */
		std::optional<CharacterMap> FindCharacterMap(const Reader &r, const std::size_t fontOffset)
		{
			if (!r.Has(fontOffset, 12))
				return std::nullopt;
			const std::uint32_t version = r.U32(fontOffset);
			if (version != 0x00010000 && version != 0x4F54544F && version != 0x74727565)	// 1.0, 'OTTO', 'true'
				return std::nullopt;

			const std::size_t tables = r.U16(fontOffset + 4);
			std::size_t cmap = 0;
			for (std::size_t i = 0; i < tables; ++i)
			{
				const std::size_t record = fontOffset + 12 + 16 * i;
				if (!r.Has(record, 16))
					return std::nullopt;
				if (r.data.substr(record, 4) == "cmap")
				{
					cmap = r.U32(record + 8);
					break;
				}
			}
			if (cmap == 0 || !r.Has(cmap, 4))
				return std::nullopt;

			std::optional<CharacterMap> best;
			int bestScore = 0;
			const std::size_t subtables = r.U16(cmap + 2);
			for (std::size_t j = 0; j < subtables; ++j)
			{
				const std::size_t record = cmap + 4 + 8 * j;
				if (!r.Has(record, 8))
					break;
				const std::uint32_t platform = r.U16(record), encoding = r.U16(record + 2);
				const std::size_t offset = cmap + r.U32(record + 4);
				if (!r.Has(offset, 2))
					continue;
				const std::uint32_t format = r.U16(offset);
				const bool unicode = platform == 0 || (platform == 3 && (encoding == 1 || encoding == 10));
				int score = 0;
				if (unicode && format == 12 && r.Has(offset, 16))
					score = 2;
				else if (unicode && format == 4 && r.Has(offset, 14))
					score = 1;
				if (score > bestScore)
				{
					bestScore = score;
					best = CharacterMap { score == 2 ? Format::Groups : Format::Segments, offset };
				}
			}
			return best;
		}
	}

	// -------------------------------------------------------

	std::u32string DecodeUtf8(const std::string_view text)
	{
		std::u32string points;
		for (std::size_t i = 0; i < text.size();)
		{
			const unsigned char lead = static_cast<unsigned char>(text[i]);
			const std::size_t length = lead < 0x80 ? 1 : (lead & 0xE0) == 0xC0 ? 2 : (lead & 0xF0) == 0xE0 ? 3 : (lead & 0xF8) == 0xF0 ? 4 : 0;
			if (length == 0 || i + length > text.size())
			{
				points += U'�';
				++i;
				continue;
			}
			char32_t point = length == 1 ? lead : length == 2 ? lead & 0x1F : length == 3 ? lead & 0x0F : lead & 0x07;
			bool valid = true;
			for (std::size_t k = 1; k < length; ++k)
			{
				const unsigned char next = static_cast<unsigned char>(text[i + k]);
				valid &= (next & 0xC0) == 0x80;
				point = (point << 6) | (next & 0x3F);
			}
			if (!valid)
			{
				points += U'�';
				++i;
				continue;
			}
			points += point;
			i += length;
		}
		return points;
	}

	std::optional<std::size_t> CountMissingGlyphs(const std::string_view font, const int face, const std::u32string_view codePoints)
	{
		const Reader r { font };
		std::size_t fontOffset = 0;
		if (r.Has(0, 12) && font.substr(0, 4) == "ttcf")
		{
			const std::uint32_t faces = r.U32(8);
			if (face < 0 || static_cast<std::uint32_t>(face) >= faces || !r.Has(12 + 4 * static_cast<std::size_t>(face), 4))
				return std::nullopt;
			fontOffset = r.U32(12 + 4 * static_cast<std::size_t>(face));
		}
		else if (face != 0)
			return std::nullopt;

		const std::optional<CharacterMap> map = FindCharacterMap(r, fontOffset);
		if (!map)
			return std::nullopt;

		std::size_t missing = 0;
		for (const char32_t c : codePoints)
		{
			const std::uint32_t glyph = map->format == Format::Groups ? LookupGroups(r, map->offset, c) : LookupSegments(r, map->offset, c);
			if (glyph == 0)
				++missing;
		}
		return missing;
	}
}

// -------------------------------------------------------
