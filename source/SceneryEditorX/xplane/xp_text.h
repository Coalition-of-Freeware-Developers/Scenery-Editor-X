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
 * xp_text.h
 * -------------------------------------------------------
 * Created: 17/10/2026
 * -------------------------------------------------------
 */
#pragma once
#include <algorithm>
#include <charconv>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <string>
#include <string_view>
#include <vector>
#include <SceneryEditorX/filesystem/vfs.h>

// -------------------------------------------------------

namespace SceneryEditorX::XPlane
{
	/**
	 * Small text helpers shared by the X-Plane text-format parsers (OBJ8, library.txt, scenery_packs.ini).
	 * X-Plane files are 7-bit ASCII, whitespace separated, with CR, LF or CRLF line endings.
	 */

	/* @brief True for the separators X-Plane uses between words. */
	constexpr bool IsBlank(const char c)
	{
		return c == ' ' || c == '\t' || c == '\r' || c == '\n' || c == '\f' || c == '\v';
	}

	/* @brief Strips leading and trailing whitespace. */
	inline std::string_view Trim(std::string_view text)
	{
		while (!text.empty() && IsBlank(text.front()))
			text.remove_prefix(1);
		while (!text.empty() && IsBlank(text.back()))
			text.remove_suffix(1);
		return text;
	}

	/**
	 * @class LineReader
	 * @brief Iterates the lines of a text buffer, accepting CR, LF and CRLF endings.
	 */
	class LineReader
	{
	public:
		explicit LineReader(const std::string_view text) : m_Text(text) {}

		/* @brief Reads the next line (without its terminator). Returns false at the end of the buffer. */
		bool Next(std::string_view &line)
		{
			if (m_Position >= m_Text.size())
				return false;

			const size_t start = m_Position;
			size_t end = start;
			while (end < m_Text.size() && m_Text[end] != '\n' && m_Text[end] != '\r')
				++end;

			line = m_Text.substr(start, end - start);
			m_Position = end;
			if (m_Position < m_Text.size() && m_Text[m_Position] == '\r')
				++m_Position;
			if (m_Position < m_Text.size() && m_Text[m_Position] == '\n')
				++m_Position;

			++m_LineNumber;
			return true;
		}

		/* @brief 1-based number of the line Next() returned last. */
		[[nodiscard]] uint32_t GetLineNumber() const { return m_LineNumber; }

	private:
		std::string_view m_Text;
		size_t m_Position = 0;
		uint32_t m_LineNumber = 0;
	};

	/* @brief Splits a line into whitespace-separated words (views into the line). */
	inline void Tokenize(const std::string_view line, std::vector<std::string_view> &tokens)
	{
		tokens.clear();
		size_t i = 0;
		while (i < line.size())
		{
			while (i < line.size() && IsBlank(line[i]))
				++i;
			if (i >= line.size())
				break;

			const size_t start = i;
			while (i < line.size() && !IsBlank(line[i]))
				++i;
			tokens.push_back(line.substr(start, i - start));
		}
	}

	/**
	 * @brief The remainder of a line starting at word `firstToken` (0-based), trimmed. Used for paths, which may contain spaces.
	 */
	inline std::string_view RestOfLine(const std::string_view line, const size_t firstToken)
	{
		size_t i = 0;
		for (size_t token = 0; token < firstToken; ++token)
		{
			while (i < line.size() && IsBlank(line[i]))
				++i;
			while (i < line.size() && !IsBlank(line[i]))
				++i;
		}
		return Trim(line.substr(std::min(i, line.size())));
	}

	/* @brief Parses a float; returns false when the whole word isn't a number. Accepts a leading '+'. */
	inline bool TryParseFloat(std::string_view text, float &value)
	{
		if (!text.empty() && text.front() == '+')
			text.remove_prefix(1);
		if (text.empty())
			return false;

		const auto [ptr, ec] = std::from_chars(text.data(), text.data() + text.size(), value);
		return ec == std::errc() && ptr == text.data() + text.size();
	}

	/* @brief Parses a float, returning fallback when the word isn't a number. */
	inline float ParseFloat(const std::string_view text, const float fallback = 0.0f)
	{
		float value = 0.0f;
		return TryParseFloat(text, value) ? value : fallback;
	}

	/* @brief Parses an integer (a float token is truncated), returning fallback when the word isn't a number. */
	inline int ParseInt(std::string_view text, const int fallback = 0)
	{
		if (!text.empty() && text.front() == '+')
			text.remove_prefix(1);

		int value = 0;
		const auto [ptr, ec] = std::from_chars(text.data(), text.data() + text.size(), value);
		if (ec == std::errc() && ptr == text.data() + text.size())
			return value;

		float asFloat = 0.0f;
		return TryParseFloat(text, asFloat) ? static_cast<int>(asFloat) : fallback;
	}

	/* @brief ASCII lower-case copy. */
	inline std::string ToLower(const std::string_view text)
	{
		std::string result(text);
		for (char &c : result)
		{
			if (c >= 'A' && c <= 'Z')
				c = static_cast<char>(c - 'A' + 'a');
		}
		return result;
	}

	/**
	 * @brief Reads a whole file into memory. Returns false when it can't be opened. A path spelled in another case than
	 * the file on disk is found too (VFS::ReadFile), the way X-Plane reads packs made on Windows on any platform.
	 */
	inline bool ReadTextFile(const std::filesystem::path &path, std::string &text)
	{
		return VFS::ReadFile(path, text);
	}

	/**
	 * @brief Turns an X-Plane relative path (forward or back slashes, ':' on very old Mac files) into a filesystem path.
	 */
	inline std::filesystem::path ToNativeRelativePath(const std::string_view text)
	{
		std::string normalized(Trim(text));
		for (char &c : normalized)
		{
			if (c == '\\' || c == ':')
				c = '/';
		}
		return std::filesystem::path(normalized).make_preferred();
	}

}

// -------------------------------------------------------
