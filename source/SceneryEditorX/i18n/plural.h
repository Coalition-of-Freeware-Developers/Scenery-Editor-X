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
 * plural.h
 * -------------------------------------------------------
 * Created: 13/11/2026
 * -------------------------------------------------------
 */
#pragma once
#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

// -------------------------------------------------------

namespace SceneryEditorX::I18n
{
	/**
	 * @class PluralRule
	 * @brief A gettext Plural-Forms rule ("nplurals=3; plural=(n%10==1 && n%100!=11 ? 0 : ...);"), compiled once.
	 *
	 * The expression may use n, integer literals, ! * / % + - < <= > >= == != && || ?: and parentheses, with C's
	 * precedence. Like gettext, n and every value are unsigned 64-bit; dividing by zero gives 0, and the result is
	 * clamped to [0, nplurals - 1].
	 */
	class PluralRule
	{
	public:
		/* @brief English: two forms, the first for n == 1. */
		PluralRule();

		/* @brief Parses a Plural-Forms header value; nullopt (with the reason) when it isn't valid. */
		static std::optional<PluralRule> Parse(std::string_view pluralForms, std::string *error = nullptr);

		/* @brief Parses a plural expression for nplurals forms. */
		static std::optional<PluralRule> ParseExpression(int nplurals, std::string_view expression, std::string *error = nullptr);

		/* @brief Number of forms (nplurals). */
		[[nodiscard]] int Count() const { return m_Count; }

		/* @brief Which form to use for n. */
		[[nodiscard]] std::size_t Index(std::uint64_t n) const;

	private:
		enum class Op : std::uint8_t
		{
			N, Number, Not, Negate, Mul, Div, Mod, Add, Sub, Less, LessEqual, Greater, GreaterEqual, Equal, NotEqual, And, Or, Select
		};

		struct Node
		{
			Op op = Op::Number;
			std::uint64_t value = 0;
			int a = -1, b = -1, c = -1;
		};

		[[nodiscard]] std::uint64_t Evaluate(int node, std::uint64_t n) const;

		std::vector<Node> m_Nodes;
		int m_Root = -1;
		int m_Count = 2;

		friend class PluralParser;
	};
}

// -------------------------------------------------------
