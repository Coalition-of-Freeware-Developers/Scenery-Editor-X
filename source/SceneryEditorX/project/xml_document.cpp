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
 * xml_document.cpp
 * -------------------------------------------------------
 * Created: 18/10/2026
 * -------------------------------------------------------
 */
#include "xml_document.h"
#include <algorithm>
#include <cstdint>

// -------------------------------------------------------

namespace SceneryEditorX::ProjectFile
{
	namespace
	{
		constexpr int MAX_DEPTH = 256;

		bool IsSpace(const char c) { return c == ' ' || c == '\t' || c == '\r' || c == '\n'; }

		bool IsNameChar(const char c)
		{
			return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') || c == '_' || c == '-' || c == '.' || c == ':' ||
				static_cast<uint8_t>(c) >= 0x80;
		}

		void AppendUtf8(std::string &out, const uint32_t code)
		{
			if (code < 0x80)
				out.push_back(static_cast<char>(code));
			else if (code < 0x800)
			{
				out.push_back(static_cast<char>(0xC0 | (code >> 6)));
				out.push_back(static_cast<char>(0x80 | (code & 0x3F)));
			}
			else if (code < 0x10000)
			{
				out.push_back(static_cast<char>(0xE0 | (code >> 12)));
				out.push_back(static_cast<char>(0x80 | ((code >> 6) & 0x3F)));
				out.push_back(static_cast<char>(0x80 | (code & 0x3F)));
			}
			else
			{
				out.push_back(static_cast<char>(0xF0 | (code >> 18)));
				out.push_back(static_cast<char>(0x80 | ((code >> 12) & 0x3F)));
				out.push_back(static_cast<char>(0x80 | ((code >> 6) & 0x3F)));
				out.push_back(static_cast<char>(0x80 | (code & 0x3F)));
			}
		}

		class Parser
		{
		public:
			explicit Parser(const std::string_view text) : m_Text(text) {}

			bool Parse(XmlElement &root, std::string *error)
			{
				if (m_Text.size() >= 3 && static_cast<uint8_t>(m_Text[0]) == 0xEF && static_cast<uint8_t>(m_Text[1]) == 0xBB && static_cast<uint8_t>(m_Text[2]) == 0xBF)
					m_Position = 3;	// UTF-8 byte order mark

				bool haveRoot = false;
				while (m_Ok)
				{
					SkipSpace();
					if (m_Position >= m_Text.size())
						break;
					if (!Peek("<"))
						return Fail(haveRoot ? "text after the root element" : "text before the root element", error);
					if (SkipMarkup())
						continue;
					if (haveRoot)
						return Fail("a second root element", error);
					ParseElement(root, 0);
					haveRoot = true;
				}
				if (!m_Ok)
					return Fail(m_Error, error);
				if (!haveRoot)
					return Fail("no root element", error);
				return true;
			}

		private:
			bool Fail(const std::string &message, std::string *error)
			{
				if (error)
				{
					const size_t end = std::min(m_ErrorPosition == std::string_view::npos ? m_Position : m_ErrorPosition, m_Text.size());
					const size_t line = 1 + static_cast<size_t>(std::count(m_Text.begin(), m_Text.begin() + static_cast<std::ptrdiff_t>(end), '\n'));
					*error = "line " + std::to_string(line) + ": " + message;
				}
				return false;
			}

			void Error(const std::string &message)
			{
				if (m_Ok)
				{
					m_Ok = false;
					m_Error = message;
					m_ErrorPosition = m_Position;
				}
			}

			[[nodiscard]] bool Peek(const std::string_view token) const
			{
				return m_Text.compare(m_Position, token.size(), token) == 0;
			}

			void SkipSpace()
			{
				while (m_Position < m_Text.size() && IsSpace(m_Text[m_Position]))
					++m_Position;
			}

			bool SkipPast(const std::string_view terminator)
			{
				const size_t end = m_Text.find(terminator, m_Position);
				if (end == std::string_view::npos)
				{
					Error("unterminated " + std::string(terminator == "-->" ? "comment" : terminator == "?>" ? "processing instruction" : "markup"));
					return false;
				}
				m_Position = end + terminator.size();
				return true;
			}

			/* @brief Skips a comment, declaration / processing instruction or DOCTYPE at the cursor. */
			bool SkipMarkup()
			{
				if (Peek("<!--"))
					return SkipPast("-->") || true;
				if (Peek("<?"))
					return SkipPast("?>") || true;
				if (Peek("<!DOCTYPE") || Peek("<!doctype"))
				{
					// Internal subsets ([...]) are skipped bracket-aware
					int depth = 0;
					for (; m_Position < m_Text.size(); ++m_Position)
					{
						const char c = m_Text[m_Position];
						if (c == '[')
							++depth;
						else if (c == ']')
							--depth;
						else if (c == '>' && depth <= 0)
						{
							++m_Position;
							return true;
						}
					}
					Error("unterminated DOCTYPE");
					return true;
				}
				return false;
			}

			std::string ParseName()
			{
				const size_t start = m_Position;
				while (m_Position < m_Text.size() && IsNameChar(m_Text[m_Position]))
					++m_Position;
				if (m_Position == start)
					Error("expected a name");
				return std::string(m_Text.substr(start, m_Position - start));
			}

			/* @brief Decodes entity and character references in raw text. */
			void Decode(const std::string_view raw, std::string &out)
			{
				if (out.empty())
					out.reserve(raw.size());
				size_t i = 0;
				while (i < raw.size())
				{
					const size_t amp = raw.find('&', i);
					if (amp == std::string_view::npos)
					{
						out.append(raw.substr(i));
						break;
					}
					out.append(raw.substr(i, amp - i));
					const size_t semicolon = raw.find(';', amp);
					if (semicolon == std::string_view::npos || semicolon - amp > 12)
					{
						Error("unterminated entity reference");
						return;
					}
					const std::string_view entity = raw.substr(amp + 1, semicolon - amp - 1);
					if (entity == "lt")
						out.push_back('<');
					else if (entity == "gt")
						out.push_back('>');
					else if (entity == "amp")
						out.push_back('&');
					else if (entity == "quot")
						out.push_back('"');
					else if (entity == "apos")
						out.push_back('\'');
					else if (!entity.empty() && entity[0] == '#')
					{
						uint32_t code = 0;
						const bool hex = entity.size() > 1 && (entity[1] == 'x' || entity[1] == 'X');
						const std::string_view digits = entity.substr(hex ? 2 : 1);
						bool valid = !digits.empty();
						for (const char c : digits)
						{
							int digit = -1;
							if (c >= '0' && c <= '9')
								digit = c - '0';
							else if (hex && c >= 'a' && c <= 'f')
								digit = c - 'a' + 10;
							else if (hex && c >= 'A' && c <= 'F')
								digit = c - 'A' + 10;
							if (digit < 0 || code > 0x10FFFF)
							{
								valid = false;
								break;
							}
							code = code * (hex ? 16u : 10u) + static_cast<uint32_t>(digit);
						}
						if (!valid || code > 0x10FFFF)
						{
							Error("bad character reference &" + std::string(entity) + ";");
							return;
						}
						AppendUtf8(out, code);
					}
					else
					{
						Error("unknown entity &" + std::string(entity) + ";");
						return;
					}
					i = semicolon + 1;
				}
			}

			void ParseElement(XmlElement &element, const int depth)
			{
				if (depth > MAX_DEPTH)
					return Error("elements nested too deeply");

				++m_Position;	// '<'
				element.name = ParseName();
				if (!m_Ok)
					return;

				// Attributes
				while (m_Ok)
				{
					SkipSpace();
					if (m_Position >= m_Text.size())
						return Error("unterminated start tag <" + element.name + ">");
					if (Peek("/>"))
					{
						m_Position += 2;
						return;
					}
					if (Peek(">"))
					{
						++m_Position;
						break;
					}

					std::string key = ParseName();
					if (!m_Ok)
						return;
					SkipSpace();
					if (!Peek("="))
						return Error("expected '=' after attribute " + key);
					++m_Position;
					SkipSpace();
					if (m_Position >= m_Text.size() || (m_Text[m_Position] != '"' && m_Text[m_Position] != '\''))
						return Error("attribute " + key + " needs a quoted value");
					const char quote = m_Text[m_Position++];
					const size_t end = m_Text.find(quote, m_Position);
					if (end == std::string_view::npos)
						return Error("unterminated value of attribute " + key);
					std::string value;
					Decode(m_Text.substr(m_Position, end - m_Position), value);
					m_Position = end + 1;
					element.attributes.emplace_back(std::move(key), std::move(value));
				}

				// Content
				while (m_Ok)
				{
					const size_t lt = m_Text.find('<', m_Position);
					if (lt == std::string_view::npos)
						return Error("element <" + element.name + "> is not closed");
					// Indentation between children isn't kept (big files would copy it over and over)
					if (const std::string_view segment = m_Text.substr(m_Position, lt - m_Position); !std::ranges::all_of(segment, IsSpace))
						Decode(segment, element.text);
					m_Position = lt;

					if (Peek("</"))
					{
						m_Position += 2;
						const std::string closing = ParseName();
						if (!m_Ok)
							return;
						if (closing != element.name)
							return Error("</" + closing + "> closes <" + element.name + ">");
						SkipSpace();
						if (!Peek(">"))
							return Error("expected '>' after </" + closing);
						++m_Position;
						break;
					}
					if (Peek("<![CDATA["))
					{
						m_Position += 9;
						const size_t end = m_Text.find("]]>", m_Position);
						if (end == std::string_view::npos)
							return Error("unterminated CDATA section");
						element.text.append(m_Text.substr(m_Position, end - m_Position));
						m_Position = end + 3;
						continue;
					}
					if (SkipMarkup())
						continue;

					element.children.emplace_back();
					ParseElement(element.children.back(), depth + 1);
				}

				// Text between child elements is only indentation in WorldEditor files
				const auto first = std::find_if_not(element.text.begin(), element.text.end(), IsSpace);
				const auto last = std::find_if_not(element.text.rbegin(), element.text.rend(), IsSpace).base();
				element.text = first < last ? std::string(first, last) : std::string();
			}

			std::string_view m_Text;
			size_t m_Position = 0;
			bool m_Ok = true;
			std::string m_Error;
			size_t m_ErrorPosition = std::string_view::npos;
		};
	}

	// -------------------------------------------------------

	const std::string *XmlElement::FindAttribute(const std::string_view key) const
	{
		for (const auto &[attributeName, value] : attributes)
		{
			if (attributeName == key)
				return &value;
		}
		return nullptr;
	}

	std::string_view XmlElement::Attribute(const std::string_view key, const std::string_view fallback) const
	{
		const std::string *value = FindAttribute(key);
		return value ? std::string_view(*value) : fallback;
	}

	const XmlElement *XmlElement::FindChild(const std::string_view childName) const
	{
		for (const XmlElement &child : children)
		{
			if (child.name == childName)
				return &child;
		}
		return nullptr;
	}

	XmlElement &XmlElement::Set(std::string key, std::string value)
	{
		for (auto &[attributeName, existing] : attributes)
		{
			if (attributeName == key)
			{
				existing = std::move(value);
				return *this;
			}
		}
		attributes.emplace_back(std::move(key), std::move(value));
		return *this;
	}

	XmlElement &XmlElement::AddChild(std::string childName)
	{
		children.emplace_back();
		children.back().name = std::move(childName);
		return children.back();
	}

	bool ParseXml(const std::string_view text, XmlElement &root, std::string *error)
	{
		root = XmlElement{};
		Parser parser(text);
		return parser.Parse(root, error);
	}

	std::string EscapeXml(const std::string_view text)
	{
		std::string out;
		out.reserve(text.size());
		for (const char c : text)
		{
			switch (c)
			{
				case '<': out.append("&lt;"); break;
				case '>': out.append("&gt;"); break;
				case '"': out.append("&quot;"); break;
				case '&': out.append("&amp;"); break;
				default:
					if (static_cast<uint8_t>(c) >= 0x20 || c == '\t' || c == '\r' || c == '\n')
						out.push_back(c);
					break;
			}
		}
		return out;
	}

	void WriteXml(const XmlElement &element, std::string &out, const int indent)
	{
		// The text as ParseXml reads it back (escaped, which drops control characters, and trimmed), so writing what
		// was read gives the same document
		std::string text = EscapeXml(element.text);
		text.erase(text.begin(), std::find_if_not(text.begin(), text.end(), IsSpace));
		text.erase(std::find_if_not(text.rbegin(), text.rend(), IsSpace).base(), text.end());

		out.append(static_cast<size_t>(indent) * 2, ' ');
		out.push_back('<');
		out.append(element.name);
		for (const auto &[key, value] : element.attributes)
		{
			out.push_back(' ');
			out.append(key);
			out.append("=\"");
			out.append(EscapeXml(value));
			out.push_back('"');
		}
		if (element.children.empty() && text.empty())
		{
			out.append("/>\n");
			return;
		}
		out.push_back('>');
		if (element.children.empty())
		{
			out.append(text);
			out.append("</");
			out.append(element.name);
			out.append(">\n");
			return;
		}
		out.push_back('\n');
		if (!text.empty())
		{
			out.append(static_cast<size_t>(indent + 1) * 2, ' ');
			out.append(text);
			out.push_back('\n');
		}
		for (const XmlElement &child : element.children)
			WriteXml(child, out, indent + 1);
		out.append(static_cast<size_t>(indent) * 2, ' ');
		out.append("</");
		out.append(element.name);
		out.append(">\n");
	}
}

// -------------------------------------------------------
