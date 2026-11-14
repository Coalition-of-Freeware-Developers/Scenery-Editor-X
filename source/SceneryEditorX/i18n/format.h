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
 * format.h
 * -------------------------------------------------------
 * Created: 13/11/2026
 * -------------------------------------------------------
 */
#pragma once
#include <concepts>
#include <cstdint>
#include <span>
#include <string>
#include <string_view>
#include <vector>

// -------------------------------------------------------

/**
 * Formatting of translated messages. The patterns use std::format's replacement fields, which translators may
 * reorder ("{1} in {0}"), but the formatting is done here rather than by std::vformat: a pattern comes from a
 * catalog file at run time, so a bad one must give an error instead of an exception, and libc++ on the macOS
 * versions the editor supports can't format floating point with std::format at all.
 *
 * Supported: "{}" and "{N}" (not mixed in one pattern), "{{" and "}}", and the spec
 * [[fill]align][sign][#][0][width][.precision][type] with align < > ^, sign + - space, and the types
 * d x X o b B c (integers), f F e E g G a A (floating point) and s. Numbers always use '.' and never group digits.
 */
namespace SceneryEditorX::I18n
{
	/**
	 * @class FormatArg
	 * @brief One argument of a formatted message: a number, bool, character or string (strings are referenced, not
	 * copied, so an argument lives only as long as the call that formats it).
	 */
	class FormatArg
	{
	public:
		enum class Kind : std::uint8_t
		{
			Empty,
			Signed,
			Unsigned,
			Float,
			Double,
			Boolean,
			Char,
			String
		};

		constexpr FormatArg() = default;

		template <std::integral T>
			requires (!std::same_as<T, bool> && !std::same_as<T, char>)
		constexpr FormatArg(const T value)
		{
			if constexpr (std::is_signed_v<T>)
			{
				m_Kind = Kind::Signed;
				m_Signed = static_cast<std::int64_t>(value);
			}
			else
			{
				m_Kind = Kind::Unsigned;
				m_Unsigned = static_cast<std::uint64_t>(value);
			}
		}

		template <std::floating_point T>
		constexpr FormatArg(const T value) : m_Kind(std::same_as<T, float> ? Kind::Float : Kind::Double), m_Double(static_cast<double>(value)) {}

		constexpr FormatArg(const bool value) : m_Kind(Kind::Boolean), m_Unsigned(value ? 1 : 0) {}
		constexpr FormatArg(const char value) : m_Kind(Kind::Char), m_Signed(static_cast<unsigned char>(value)) {}
		FormatArg(const char *value) : m_Kind(Kind::String), m_String(value ? value : "") {}
		FormatArg(const std::string_view value) : m_Kind(Kind::String), m_String(value) {}
		FormatArg(const std::string &value) : m_Kind(Kind::String), m_String(value) {}

		[[nodiscard]] Kind GetKind() const { return m_Kind; }
		[[nodiscard]] std::int64_t GetSigned() const { return m_Signed; }
		[[nodiscard]] std::uint64_t GetUnsigned() const { return m_Unsigned; }
		[[nodiscard]] double GetDouble() const { return m_Double; }
		[[nodiscard]] std::string_view GetString() const { return m_String; }

	private:
		Kind m_Kind = Kind::Empty;
		std::int64_t m_Signed = 0;
		std::uint64_t m_Unsigned = 0;
		double m_Double = 0.0;
		std::string_view m_String;
	};

	/* @brief A replacement field of a pattern: which argument and its spec (the text after ':'). */
	struct FormatField
	{
		std::size_t index = 0;
		std::string spec;
	};

	/**
	 * @brief Reads the replacement fields of a pattern, in order (automatic "{}" fields get their implied indices).
	 * @return False, with what's wrong in error, for an unclosed '{', a lone '}', mixed "{}" / "{N}" or a bad spec.
	 */
	bool ParseFormatFields(std::string_view pattern, std::vector<FormatField> &fields, std::string *error = nullptr);

	/**
	 * @brief Appends the formatted pattern to out. On an error (a bad pattern, a missing argument, a spec that doesn't
	 * fit the argument's type) out is left as it was, false is returned and error says why. Never throws.
	 */
	bool FormatTo(std::string &out, std::string_view pattern, std::span<const FormatArg> args, std::string *error = nullptr);
}

// -------------------------------------------------------
