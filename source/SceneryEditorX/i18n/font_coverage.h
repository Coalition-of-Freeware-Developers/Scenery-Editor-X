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
 * font_coverage.h
 * -------------------------------------------------------
 * Created: 14/11/2026
 * -------------------------------------------------------
 */
#pragma once
#include <cstddef>
#include <optional>
#include <string>
#include <string_view>

// -------------------------------------------------------

namespace SceneryEditorX::I18n
{
	/* @brief The code points of UTF-8 text; a byte that doesn't start a valid sequence becomes U+FFFD. */
	std::u32string DecodeUtf8(std::string_view text);

	/**
	 * @brief How many of the code points a TrueType / OpenType font has no glyph for, read from its character map
	 * (cmap formats 4 and 12, the ones Unicode fonts use). Checked before a font is merged into the UI fonts, so a
	 * file that only looks right (a Chinese font offered for Japanese, whatever fontconfig returned) is passed over.
	 * @param font The whole font file: a .ttf / .otf, or a .ttc collection.
	 * @param face The face in a collection (0 for a single font).
	 * @return Nullopt when the data isn't a font with a Unicode character map, or has no such face.
	 */
	std::optional<std::size_t> CountMissingGlyphs(std::string_view font, int face, std::u32string_view codePoints);
}

// -------------------------------------------------------
