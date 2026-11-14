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
 * po.cpp
 * -------------------------------------------------------
 * Created: 13/11/2026
 * -------------------------------------------------------
 */
#include "po.h"
#include <algorithm>
#include <cctype>

// -------------------------------------------------------

namespace SceneryEditorX::I18n
{
	namespace
	{
		enum class Field
		{
			Nothing,
			Context,
			Id,
			IdPlural,
			Str
		};

		bool EqualsNoCase(const std::string_view a, const std::string_view b)
		{
			if (a.size() != b.size())
				return false;
			for (std::size_t i = 0; i < a.size(); ++i)
				if (std::tolower(static_cast<unsigned char>(a[i])) != std::tolower(static_cast<unsigned char>(b[i])))
					return false;
			return true;
		}

		std::string_view Trim(std::string_view text)
		{
			while (!text.empty() && (text.front() == ' ' || text.front() == '\t'))
				text.remove_prefix(1);
			while (!text.empty() && (text.back() == ' ' || text.back() == '\t' || text.back() == '\r'))
				text.remove_suffix(1);
			return text;
		}

		int HexValue(const char c)
		{
			if (c >= '0' && c <= '9')
				return c - '0';
			if (c >= 'a' && c <= 'f')
				return c - 'a' + 10;
			if (c >= 'A' && c <= 'F')
				return c - 'A' + 10;
			return -1;
		}

		/**
		 * @class Parser
		 * @brief Reads a PO file line by line, collecting the comments and strings of the entry being read and closing it
		 * when the next one starts (a comment or msgctxt/msgid after a msgstr) or the file ends.
		 */
		class Parser
		{
		public:
			explicit Parser(PoFile &file) : m_File(file) {}

			void Line(std::string_view line, const int number)
			{
				m_Number = number;
				line = Trim(line);
				if (line.empty())
				{
					Finish();
					return;
				}

				if (line[0] == '#')
				{
					Comment(line);
					return;
				}

				if (line[0] == '"')
				{
					Continuation(line);
					return;
				}

				std::size_t keywordEnd = 0;
				while (keywordEnd < line.size() && line[keywordEnd] != ' ' && line[keywordEnd] != '\t' && line[keywordEnd] != '"')
					++keywordEnd;
				const std::string_view keyword = line.substr(0, keywordEnd);
				const std::string_view rest = Trim(line.substr(keywordEnd));

				if (keyword == "msgctxt")
				{
					StartIfNeeded();
					if (m_Entry.hasContext || m_SeenId)
						Error("msgctxt must come first in its entry");
					m_Entry.hasContext = true;
					m_Field = Field::Context;
					ReadString(rest, m_Entry.context);
				}
				else if (keyword == "msgid")
				{
					StartIfNeeded();
					if (m_SeenId)
						Error("a second msgid in one entry");
					m_SeenId = true;
					m_Entry.line = number;
					m_Field = Field::Id;
					ReadString(rest, m_Entry.msgid);
				}
				else if (keyword == "msgid_plural")
				{
					if (!m_SeenId || m_SeenStr)
						Error("msgid_plural must follow msgid");
					m_Entry.hasPlural = true;
					m_Field = Field::IdPlural;
					ReadString(rest, m_Entry.msgidPlural);
				}
				else if (keyword == "msgstr" || keyword.starts_with("msgstr["))
				{
					if (!m_SeenId)
					{
						Error("msgstr without a msgid");
						m_SeenId = true;
					}
					m_SeenStr = true;
					std::size_t index = 0;
					if (keyword != "msgstr")
					{
						const std::string_view digits = keyword.substr(7, keyword.size() > 8 ? keyword.size() - 8 : 0);
						if (keyword.back() != ']' || digits.empty() || digits.size() > 3)
						{
							Error("bad msgstr index");
							m_Field = Field::Nothing;
							return;
						}
						for (const char c : digits)
						{
							if (!std::isdigit(static_cast<unsigned char>(c)))
							{
								Error("bad msgstr index");
								m_Field = Field::Nothing;
								return;
							}
							index = index * 10 + static_cast<std::size_t>(c - '0');
						}
						if (!m_Entry.hasPlural)
							Error("msgstr[n] in an entry without msgid_plural");
						if (index != m_Entry.msgstr.size())
							Error("msgstr[" + std::to_string(index) + "] out of order");
					}
					else if (m_Entry.hasPlural)
					{
						Error("a plural entry needs msgstr[n]");
					}
					if (m_Entry.msgstr.size() <= index)
						m_Entry.msgstr.resize(index + 1);
					m_StrIndex = index;
					m_Field = Field::Str;
					ReadString(rest, m_Entry.msgstr[index]);
				}
				else
				{
					Error("unknown keyword '" + std::string(keyword.substr(0, 32)) + "'");
					m_Field = Field::Nothing;
				}
			}

			void End()
			{
				Finish();
			}

		private:
			void Error(std::string message)
			{
				m_File.errors.push_back({ m_Number, std::move(message) });
			}

			/* A comment or msgctxt/msgid after an entry's msgstr starts the next entry. */
			void StartIfNeeded()
			{
				if (m_SeenStr)
					Finish();
			}

			void Comment(const std::string_view line)
			{
				if (line.starts_with("#~"))
				{
					// Obsolete entry: skipped whole, but it still separates entries
					Finish();
					m_Field = Field::Nothing;
					return;
				}
				StartIfNeeded();
				if (m_SeenId)
				{
					Error("a comment inside an entry");
					return;
				}
				m_HasContent = true;
				const auto text = [&line](const std::size_t skip) { return std::string(Trim(line.substr(std::min(skip, line.size())))); };
				if (line.starts_with("#."))
					m_Entry.extractedComments.push_back(text(2));
				else if (line.starts_with("#:"))
					m_Entry.references.push_back(text(2));
				else if (line.starts_with("#,"))
				{
					std::string_view flags = line.substr(2);
					while (!flags.empty())
					{
						const std::size_t comma = flags.find(',');
						const std::string_view flag = Trim(flags.substr(0, comma));
						if (!flag.empty())
						{
							m_Entry.flags.emplace_back(flag);
							if (flag == "fuzzy")
								m_Entry.fuzzy = true;
						}
						if (comma == std::string_view::npos)
							break;
						flags.remove_prefix(comma + 1);
					}
				}
				else if (line.starts_with("#|"))
				{
					// Previous msgid of a fuzzy entry: not needed at run time
				}
				else
					m_Entry.translatorComments.push_back(text(1));
			}

			void Continuation(const std::string_view line)
			{
				std::string *target = nullptr;
				switch (m_Field)
				{
					case Field::Context: target = &m_Entry.context; break;
					case Field::Id: target = &m_Entry.msgid; break;
					case Field::IdPlural: target = &m_Entry.msgidPlural; break;
					case Field::Str: target = &m_Entry.msgstr[m_StrIndex]; break;
					case Field::Nothing: break;
				}
				if (!target)
				{
					Error("a string outside an entry");
					return;
				}
				ReadString(line, *target);
			}

			/* Appends the quoted string that makes up the rest of a line. */
			void ReadString(const std::string_view rest, std::string &out)
			{
				m_HasContent = true;
				if (rest.size() < 2 || rest.front() != '"' || rest.back() != '"')
				{
					Error("expected a quoted string");
					return;
				}
				// The closing quote must be the last one: an unescaped quote inside means two strings on one line
				const std::string_view inside = rest.substr(1, rest.size() - 2);
				for (std::size_t i = 0; i < inside.size(); ++i)
				{
					if (inside[i] == '\\')
						++i;
					else if (inside[i] == '"')
					{
						Error("unescaped quote in a string");
						return;
					}
				}
				std::string error;
				if (!UnescapePoString(inside, out, &error))
					Error(error);
			}

			void Finish()
			{
				if (m_HasContent && !m_SeenId)
				{
					if (m_Entry.hasContext || m_SeenStr)
						Error("an entry without msgid");
					// Comments alone (e.g. a file's opening comment block) are not an entry
				}
				else if (m_SeenId)
				{
					if (!m_SeenStr)
						Error("msgid without msgstr");
					if (m_Entry.hasPlural && m_Entry.msgstr.empty())
						m_Entry.msgstr.emplace_back();
					if (m_Entry.msgstr.empty())
						m_Entry.msgstr.emplace_back();

					if (!m_Entry.hasContext && m_Entry.msgid.empty())
						Header();
					else
						m_File.entries.push_back(std::move(m_Entry));
				}
				m_Entry = PoEntry();
				m_Field = Field::Nothing;
				m_SeenId = m_SeenStr = m_HasContent = false;
				m_StrIndex = 0;
			}

			void Header()
			{
				if (m_File.hasHeader)
				{
					Error("a second header entry (empty msgid)");
					return;
				}
				m_File.hasHeader = true;
				std::string_view text = m_Entry.msgstr.front();
				while (!text.empty())
				{
					const std::size_t end = text.find('\n');
					const std::string_view line = text.substr(0, end);
					const std::size_t colon = line.find(':');
					if (colon != std::string_view::npos)
						m_File.header.emplace_back(std::string(Trim(line.substr(0, colon))), std::string(Trim(line.substr(colon + 1))));
					if (end == std::string_view::npos)
						break;
					text.remove_prefix(end + 1);
				}
			}

			PoFile &m_File;
			PoEntry m_Entry;
			Field m_Field = Field::Nothing;
			std::size_t m_StrIndex = 0;
			bool m_SeenId = false;
			bool m_SeenStr = false;
			bool m_HasContent = false;
			int m_Number = 0;
		};
	}

	// -------------------------------------------------------

	bool PoEntry::IsTranslated() const
	{
		if (fuzzy || msgstr.empty())
			return false;
		for (const std::string &form : msgstr)
			if (form.empty())
				return false;
		return true;
	}

	std::string_view PoFile::GetHeader(const std::string_view field) const
	{
		for (const auto &[name, value] : header)
			if (EqualsNoCase(name, field))
				return value;
		return {};
	}

	bool UnescapePoString(const std::string_view escaped, std::string &out, std::string *error)
	{
		bool ok = true;
		for (std::size_t i = 0; i < escaped.size(); ++i)
		{
			const char c = escaped[i];
			if (c != '\\')
			{
				out += c;
				continue;
			}
			if (++i >= escaped.size())
			{
				if (error)
					*error = "a string ends with a backslash";
				return false;
			}
			const char e = escaped[i];
			switch (e)
			{
				case 'n': out += '\n'; break;
				case 't': out += '\t'; break;
				case 'r': out += '\r'; break;
				case 'a': out += '\a'; break;
				case 'b': out += '\b'; break;
				case 'f': out += '\f'; break;
				case 'v': out += '\v'; break;
				case '\\': out += '\\'; break;
				case '"': out += '"'; break;
				case '\'': out += '\''; break;
				case '?': out += '?'; break;
				case 'x':
				{
					int value = 0, digits = 0;
					while (i + 1 < escaped.size() && digits < 2 && HexValue(escaped[i + 1]) >= 0)
					{
						value = value * 16 + HexValue(escaped[++i]);
						++digits;
					}
					if (digits == 0)
					{
						if (error)
							*error = "\\x without hex digits";
						ok = false;
					}
					else
						out += static_cast<char>(value);
					break;
				}
				default:
					if (e >= '0' && e <= '7')
					{
						int value = e - '0', digits = 1;
						while (i + 1 < escaped.size() && digits < 3 && escaped[i + 1] >= '0' && escaped[i + 1] <= '7')
						{
							value = value * 8 + (escaped[++i] - '0');
							++digits;
						}
						out += static_cast<char>(value & 0xFF);
					}
					else
					{
						if (error)
							*error = std::string("unknown escape \\") + e;
						out += e;
						ok = false;
					}
					break;
			}
		}
		return ok;
	}

	PoFile ParsePo(std::string_view text)
	{
		PoFile file;
		if (text.starts_with("\xEF\xBB\xBF"))
			text.remove_prefix(3);

		Parser parser(file);
		int number = 0;
		while (!text.empty())
		{
			++number;
			const std::size_t end = text.find('\n');
			parser.Line(text.substr(0, end), number);
			if (end == std::string_view::npos)
				break;
			text.remove_prefix(end + 1);
		}
		parser.End();
		return file;
	}
}

// -------------------------------------------------------
