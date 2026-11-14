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
 * pseudo.cpp
 * -------------------------------------------------------
 * Created: 13/11/2026
 * -------------------------------------------------------
 */
#include "pseudo.h"
#include <cstring>

// -------------------------------------------------------

namespace SceneryEditorX::I18n
{
	namespace
	{
		// Accented look-alikes from Latin-1 and Latin Extended-A (plus U+0192), all in the UI's Open Sans fonts: Ɓ, ƀ and Ƒ
		// (Latin Extended-B) are not, so B, b and F stay plain
		constexpr char32_t UPPER[26] = {
			0x00C5, U'B', 0x00C7, 0x010E, 0x00C9, U'F', 0x011C, 0x0124, 0x00CE, 0x0134, 0x0136, 0x0139, U'M',
			0x00D1, 0x00D6, 0x00DE, U'Q', 0x0154, 0x0160, 0x0166, 0x00DB, U'V', 0x0174, U'X', 0x00DD, 0x017D };
		constexpr char32_t LOWER[26] = {
			0x00E5, U'b', 0x00E7, 0x010F, 0x00E9, 0x0192, 0x011D, 0x0125, 0x00EE, 0x0135, 0x0137, 0x013A, U'm',
			0x00F1, 0x00F6, 0x00FE, U'q', 0x0155, 0x0161, 0x0167, 0x00FB, U'v', 0x0175, U'x', 0x00FD, 0x017E };
		constexpr const char *PADDING = "\xC2\xB7";	// U+00B7 middle dot

		void AppendUtf8(std::string &out, const char32_t c)
		{
			if (c < 0x80)
				out += static_cast<char>(c);
			else if (c < 0x800)
			{
				out += static_cast<char>(0xC0 | (c >> 6));
				out += static_cast<char>(0x80 | (c & 0x3F));
			}
			else
			{
				out += static_cast<char>(0xE0 | (c >> 12));
				out += static_cast<char>(0x80 | ((c >> 6) & 0x3F));
				out += static_cast<char>(0x80 | (c & 0x3F));
			}
		}

		bool In(const char *set, const char c)
		{
			return c != '\0' && std::strchr(set, c) != nullptr;
		}

		/* @brief Length of a printf conversion at text[0] == '%' ("%-5.2f" -> 6), or 0 when it isn't one. */
		std::size_t PrintfLength(const std::string_view text)
		{
			std::size_t i = 1;
			if (i < text.size() && text[i] == '%')
				return 2;
			while (i < text.size() && In("-+ #0", text[i]))
				++i;
			while (i < text.size() && ((text[i] >= '0' && text[i] <= '9') || text[i] == '*' || text[i] == '.'))
				++i;
			while (i < text.size() && In("hlLqjzt", text[i]))
				++i;
			if (i < text.size() && In("diouxXeEfFgGaAcspn", text[i]))
				return i + 1;
			return 0;
		}
	}

	std::string PseudoLocalize(std::string_view text)
	{
		std::string_view suffix;
		const std::size_t marker = text.find("##");
		if (marker != std::string_view::npos)
		{
			suffix = text.substr(marker);
			text = text.substr(0, marker);
		}
		if (text.empty())
			return std::string(suffix);

		std::string body;
		body.reserve(text.size() * 2);
		std::size_t letters = 0;
		for (std::size_t i = 0; i < text.size();)
		{
			const char c = text[i];
			if (c == '{')
			{
				if (i + 1 < text.size() && text[i + 1] == '{')
				{
					body += "{{";
					i += 2;
					continue;
				}
				const std::size_t close = text.find('}', i);
				const std::size_t end = close == std::string_view::npos ? text.size() : close + 1;
				body.append(text.substr(i, end - i));
				i = end;
				continue;
			}
			if (c == '}' && i + 1 < text.size() && text[i + 1] == '}')
			{
				body += "}}";
				i += 2;
				continue;
			}
			if (c == '%')
			{
				if (const std::size_t length = PrintfLength(text.substr(i)))
				{
					body.append(text.substr(i, length));
					i += length;
					continue;
				}
			}
			if (c >= 'A' && c <= 'Z')
			{
				AppendUtf8(body, UPPER[c - 'A']);
				++letters;
			}
			else if (c >= 'a' && c <= 'z')
			{
				AppendUtf8(body, LOWER[c - 'a']);
				++letters;
			}
			else
			{
				body += c;
				// Count each character (not each UTF-8 byte) that isn't a line break
				if ((static_cast<unsigned char>(c) & 0xC0) != 0x80 && c != '\n')
					++letters;
			}
			++i;
		}

		const std::size_t padding = (letters * 2 + 4) / 5;	// About 40%, at least one
		std::string result = "[!! ";
		result += body;
		result += ' ';
		for (std::size_t i = 0; i < (padding == 0 ? 1 : padding); ++i)
			result += PADDING;
		result += "!!]";
		result.append(suffix);
		return result;
	}
}

// -------------------------------------------------------
