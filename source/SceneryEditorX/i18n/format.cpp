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
 * format.cpp
 * -------------------------------------------------------
 * Created: 13/11/2026
 * -------------------------------------------------------
 */
#include "format.h"
#include <SceneryEditorX/utils/float_chars.h>
#include <cmath>
#include <cstdio>

// -------------------------------------------------------

namespace SceneryEditorX::I18n
{
	namespace
	{
		constexpr int MAX_WIDTH = 1000;
		constexpr int MAX_PRECISION = 300;

		struct Spec
		{
			std::string fill = " ";	// One code point, UTF-8
			char align = 0;			// '<' '>' '^' or 0 (the type's default)
			char sign = '-';		// '-' '+' ' '
			bool alternate = false;	// '#'
			bool zero = false;		// '0'
			int width = 0;
			int precision = -1;
			char type = 0;
		};

		bool Fail(std::string *error, std::string message)
		{
			if (error)
				*error = std::move(message);
			return false;
		}

		std::size_t Utf8Length(const unsigned char lead)
		{
			if (lead < 0x80)
				return 1;
			if ((lead & 0xE0) == 0xC0)
				return 2;
			if ((lead & 0xF0) == 0xE0)
				return 3;
			if ((lead & 0xF8) == 0xF0)
				return 4;
			return 1;
		}

		std::size_t CountCodePoints(const std::string_view text)
		{
			std::size_t count = 0;
			for (const char c : text)
				if ((static_cast<unsigned char>(c) & 0xC0) != 0x80)
					++count;
			return count;
		}

		std::string_view FirstCodePoints(const std::string_view text, const std::size_t count)
		{
			std::size_t i = 0, seen = 0;
			while (i < text.size() && seen < count)
			{
				i += Utf8Length(static_cast<unsigned char>(text[i]));
				++seen;
			}
			return text.substr(0, i < text.size() ? i : text.size());
		}

		bool ReadNumber(const std::string_view spec, std::size_t &i, int &value, const int limit)
		{
			value = 0;
			const std::size_t start = i;
			while (i < spec.size() && spec[i] >= '0' && spec[i] <= '9')
			{
				value = value * 10 + (spec[i] - '0');
				if (value > limit)
					return false;
				++i;
			}
			return i > start;
		}

		bool ParseSpec(const std::string_view spec, Spec &out, std::string *error)
		{
			out = Spec();
			std::size_t i = 0;
			const auto isAlign = [](const char c) { return c == '<' || c == '>' || c == '^'; };

			// [[fill]align]: the fill is any one code point but '{' and '}'
			if (!spec.empty())
			{
				const std::size_t fillLength = Utf8Length(static_cast<unsigned char>(spec[0]));
				if (fillLength < spec.size() && isAlign(spec[fillLength]) && spec[0] != '{' && spec[0] != '}')
				{
					out.fill = std::string(spec.substr(0, fillLength));
					out.align = spec[fillLength];
					i = fillLength + 1;
				}
				else if (isAlign(spec[0]))
				{
					out.align = spec[0];
					i = 1;
				}
			}
			if (i < spec.size() && (spec[i] == '+' || spec[i] == '-' || spec[i] == ' '))
				out.sign = spec[i++];
			if (i < spec.size() && spec[i] == '#')
			{
				out.alternate = true;
				++i;
			}
			if (i < spec.size() && spec[i] == '0')
			{
				out.zero = true;
				++i;
			}
			if (i < spec.size() && spec[i] >= '1' && spec[i] <= '9' && !ReadNumber(spec, i, out.width, MAX_WIDTH))
				return Fail(error, "width too large in '{:" + std::string(spec) + "}'");
			if (i < spec.size() && spec[i] == '.')
			{
				++i;
				if (!ReadNumber(spec, i, out.precision, MAX_PRECISION))
					return Fail(error, "bad precision in '{:" + std::string(spec) + "}'");
			}
			if (i < spec.size())
			{
				const char type = spec[i++];
				const std::string_view types = "dxXobBcfFeEgGaAs";
				if (types.find(type) == std::string_view::npos)
					return Fail(error, "unknown type '" + std::string(1, type) + "' in '{:" + std::string(spec) + "}'");
				out.type = type;
			}
			if (i != spec.size())
				return Fail(error, "unsupported spec '{:" + std::string(spec) + "}'");
			return true;
		}

		void Pad(std::string &out, const std::string_view body, const Spec &spec, const char defaultAlign)
		{
			const std::size_t length = CountCodePoints(body);
			const std::size_t width = static_cast<std::size_t>(spec.width);
			if (length >= width)
			{
				out.append(body);
				return;
			}
			const std::size_t padding = width - length;
			const char align = spec.align ? spec.align : defaultAlign;
			const std::size_t before = align == '>' ? padding : align == '^' ? padding / 2 : 0;
			for (std::size_t i = 0; i < before; ++i)
				out += spec.fill;
			out.append(body);
			for (std::size_t i = before; i < padding; ++i)
				out += spec.fill;
		}

		/* @brief Sign or nothing, as the spec asks for a value of this sign. */
		std::string_view SignOf(const bool negative, const Spec &spec)
		{
			if (negative)
				return "-";
			if (spec.sign == '+')
				return "+";
			if (spec.sign == ' ')
				return " ";
			return {};
		}

		/* @brief Pads a number: zeros after the sign and prefix with '0' (and no alignment), else the fill. */
		void PadNumber(std::string &out, const std::string_view prefix, const std::string_view digits, const Spec &spec)
		{
			if (spec.zero && !spec.align)
			{
				const std::size_t length = prefix.size() + digits.size();
				out.append(prefix);
				for (std::size_t i = length; i < static_cast<std::size_t>(spec.width); ++i)
					out += '0';
				out.append(digits);
				return;
			}
			std::string body(prefix);
			body.append(digits);
			Pad(out, body, spec, '>');
		}

		void AppendUtf8(std::string &out, const std::uint32_t c)
		{
			if (c < 0x80)
				out += static_cast<char>(c);
			else if (c < 0x800)
			{
				out += static_cast<char>(0xC0 | (c >> 6));
				out += static_cast<char>(0x80 | (c & 0x3F));
			}
			else if (c < 0x10000)
			{
				out += static_cast<char>(0xE0 | (c >> 12));
				out += static_cast<char>(0x80 | ((c >> 6) & 0x3F));
				out += static_cast<char>(0x80 | (c & 0x3F));
			}
			else
			{
				out += static_cast<char>(0xF0 | (c >> 18));
				out += static_cast<char>(0x80 | ((c >> 12) & 0x3F));
				out += static_cast<char>(0x80 | ((c >> 6) & 0x3F));
				out += static_cast<char>(0x80 | (c & 0x3F));
			}
		}

		bool FormatInteger(std::string &out, const bool negative, const std::uint64_t magnitude, const Spec &spec, std::string *error)
		{
			if (spec.precision >= 0)
				return Fail(error, "an integer can't have a precision");
			if (spec.type == 'c')
			{
				if (negative || magnitude > 0x10FFFF || spec.sign != '-' || spec.alternate || spec.zero)
					return Fail(error, "bad character value or spec");
				std::string text;
				AppendUtf8(text, static_cast<std::uint32_t>(magnitude));
				Pad(out, text, spec, '<');
				return true;
			}

			unsigned base = 10;
			bool upper = false;
			std::string_view prefix;
			switch (spec.type)
			{
				case 0: case 'd': break;
				case 'x': base = 16; prefix = "0x"; break;
				case 'X': base = 16; prefix = "0X"; upper = true; break;
				case 'o': base = 8; prefix = "0"; break;
				case 'b': base = 2; prefix = "0b"; break;
				case 'B': base = 2; prefix = "0B"; break;
				default: return Fail(error, std::string("type '") + spec.type + "' doesn't fit an integer");
			}

			char buffer[72];
			std::size_t at = sizeof(buffer);
			std::uint64_t value = magnitude;
			const char *digits = upper ? "0123456789ABCDEF" : "0123456789abcdef";
			do
			{
				buffer[--at] = digits[value % base];
				value /= base;
			} while (value != 0);

			std::string head(SignOf(negative, spec));
			if (spec.alternate && !(base == 8 && magnitude == 0))
				head.append(prefix);
			PadNumber(out, head, std::string_view(buffer + at, sizeof(buffer) - at), spec);
			return true;
		}

		bool FormatFloating(std::string &out, const double value, const bool single, const Spec &spec, std::string *error)
		{
			const bool negative = std::signbit(value);
			const double magnitude = std::fabs(value);
			std::string digits;

			if (spec.type == 0 && spec.precision < 0)
			{
				// Shortest text that reads back as the value, like std::format
				char buffer[64];
				const auto result = single
					? Utils::FloatChars::ToChars(buffer, buffer + sizeof(buffer), static_cast<float>(magnitude))
					: Utils::FloatChars::ToChars(buffer, buffer + sizeof(buffer), magnitude);
				if (result.ec != std::errc())
					return Fail(error, "number too long");
				digits.assign(buffer, result.ptr);
				if (spec.alternate && digits.find_first_of(".en") == std::string::npos)
					digits += '.';
			}
			else
			{
				char conversion = spec.type ? spec.type : 'g';
				const int precision = spec.precision >= 0 ? spec.precision : 6;
				if (conversion == 's' || conversion == 'd' || conversion == 'x' || conversion == 'X' || conversion == 'o' ||
					conversion == 'b' || conversion == 'B' || conversion == 'c')
					return Fail(error, std::string("type '") + spec.type + "' doesn't fit a floating-point number");

				char format[16];
				const bool hasPrecision = conversion != 'a' && conversion != 'A' ? true : spec.precision >= 0;
				std::snprintf(format, sizeof(format), "%%%s%s%c", spec.alternate ? "#" : "", hasPrecision ? ".*" : "", conversion);
				const int length = hasPrecision ? std::snprintf(nullptr, 0, format, precision, magnitude) : std::snprintf(nullptr, 0, format, magnitude);
				if (length < 0)
					return Fail(error, "number formatting failed");
				digits.resize(static_cast<std::size_t>(length) + 1);
				if (hasPrecision)
					std::snprintf(digits.data(), digits.size(), format, precision, magnitude);
				else
					std::snprintf(digits.data(), digits.size(), format, magnitude);
				digits.resize(static_cast<std::size_t>(length));
				// std::format writes hexadecimal floats without the 0x
				if ((conversion == 'a' || conversion == 'A') && digits.size() > 2 && digits[0] == '0' && (digits[1] == 'x' || digits[1] == 'X'))
					digits.erase(0, 2);
			}

			const std::string_view sign = SignOf(negative, spec);
			if (!std::isfinite(value))
			{
				std::string body(sign);
				body += digits;
				Pad(out, body, spec, '>');
				return true;
			}
			PadNumber(out, sign, digits, spec);
			return true;
		}

		bool FormatString(std::string &out, std::string_view text, const Spec &spec, std::string *error)
		{
			if (spec.type != 0 && spec.type != 's')
				return Fail(error, std::string("type '") + spec.type + "' doesn't fit a string");
			if (spec.sign != '-' || spec.alternate || spec.zero)
				return Fail(error, "a string can't have a sign, '#' or '0'");
			if (spec.precision >= 0)
				text = FirstCodePoints(text, static_cast<std::size_t>(spec.precision));
			Pad(out, text, spec, '<');
			return true;
		}

		bool FormatOne(std::string &out, const FormatArg &arg, const Spec &spec, std::string *error)
		{
			const bool integerType = spec.type != 0 && std::string_view("dxXobBc").find(spec.type) != std::string_view::npos;
			switch (arg.GetKind())
			{
				case FormatArg::Kind::Signed:
				{
					const std::int64_t value = arg.GetSigned();
					const std::uint64_t magnitude = value < 0 ? 0 - static_cast<std::uint64_t>(value) : static_cast<std::uint64_t>(value);
					return FormatInteger(out, value < 0, magnitude, spec, error);
				}
				case FormatArg::Kind::Unsigned:
					return FormatInteger(out, false, arg.GetUnsigned(), spec, error);
				case FormatArg::Kind::Float:
				case FormatArg::Kind::Double:
					return FormatFloating(out, arg.GetDouble(), arg.GetKind() == FormatArg::Kind::Float, spec, error);
				case FormatArg::Kind::Boolean:
					if (integerType && spec.type != 'c')
						return FormatInteger(out, false, arg.GetUnsigned(), spec, error);
					return FormatString(out, arg.GetUnsigned() ? "true" : "false", spec, error);
				case FormatArg::Kind::Char:
					if (integerType && spec.type != 'c')
						return FormatInteger(out, false, static_cast<std::uint64_t>(arg.GetSigned()), spec, error);
					{
						Spec text = spec;
						text.type = 0;
						const char c = static_cast<char>(arg.GetSigned());
						return FormatString(out, std::string_view(&c, 1), text, error);
					}
				case FormatArg::Kind::String:
					return FormatString(out, arg.GetString(), spec, error);
				case FormatArg::Kind::Empty:
					break;
			}
			return Fail(error, "missing argument");
		}

		/**
		 * @brief Walks a pattern, calling literal(text) for plain text and field(index, spec) for each field; stops with
		 * false at the first problem in the pattern or the first false a callback returns.
		 */
		template <typename Literal, typename Field>
		bool Walk(const std::string_view pattern, Literal &&literal, Field &&field, std::string *error)
		{
			std::size_t next = 0;
			bool automatic = false, manual = false;
			std::size_t i = 0;
			while (i < pattern.size())
			{
				const char c = pattern[i];
				if (c == '{')
				{
					if (i + 1 < pattern.size() && pattern[i + 1] == '{')
					{
						literal(std::string_view("{"));
						i += 2;
						continue;
					}
					const std::size_t close = pattern.find('}', i + 1);
					if (close == std::string_view::npos)
						return Fail(error, "'{' without '}'");
					const std::string_view inside = pattern.substr(i + 1, close - i - 1);
					if (inside.find('{') != std::string_view::npos)
						return Fail(error, "nested '{' in a field");
					const std::size_t colon = inside.find(':');
					const std::string_view id = inside.substr(0, colon);
					const std::string_view spec = colon == std::string_view::npos ? std::string_view() : inside.substr(colon + 1);
					std::size_t index = 0;
					if (id.empty())
					{
						automatic = true;
						index = next++;
					}
					else
					{
						manual = true;
						for (const char d : id)
						{
							if (d < '0' || d > '9' || index > 1000)
								return Fail(error, "bad field '{" + std::string(inside) + "}'");
							index = index * 10 + static_cast<std::size_t>(d - '0');
						}
					}
					if (automatic && manual)
						return Fail(error, "'{}' and '{N}' mixed in one message");
					if (!field(index, spec))
						return false;
					i = close + 1;
					continue;
				}
				if (c == '}')
				{
					if (i + 1 < pattern.size() && pattern[i + 1] == '}')
					{
						literal(std::string_view("}"));
						i += 2;
						continue;
					}
					return Fail(error, "'}' without '{'");
				}
				const std::size_t end = pattern.find_first_of("{}", i);
				const std::size_t stop = end == std::string_view::npos ? pattern.size() : end;
				literal(pattern.substr(i, stop - i));
				i = stop;
			}
			return true;
		}
	}

	// -------------------------------------------------------

	bool ParseFormatFields(const std::string_view pattern, std::vector<FormatField> &fields, std::string *error)
	{
		fields.clear();
		return Walk(pattern, [](std::string_view) {}, [&](const std::size_t index, const std::string_view spec)
		{
			Spec parsed;
			if (!ParseSpec(spec, parsed, error))
				return false;
			fields.push_back({ index, std::string(spec) });
			return true;
		}, error);
	}

	bool FormatTo(std::string &out, const std::string_view pattern, const std::span<const FormatArg> args, std::string *error)
	{
		std::string result;
		result.reserve(pattern.size() + 16 * args.size());
		const bool ok = Walk(pattern, [&](const std::string_view text) { result.append(text); },
			[&](const std::size_t index, const std::string_view spec)
		{
			if (index >= args.size())
				return Fail(error, "field {" + std::to_string(index) + "} has no argument (" + std::to_string(args.size()) + " given)");
			Spec parsed;
			if (!ParseSpec(spec, parsed, error))
				return false;
			return FormatOne(result, args[index], parsed, error);
		}, error);
		if (ok)
			out += result;
		return ok;
	}
}

// -------------------------------------------------------
