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
 * plural.cpp
 * -------------------------------------------------------
 * Created: 13/11/2026
 * -------------------------------------------------------
 */
#include "plural.h"
#include <cctype>

// -------------------------------------------------------

namespace SceneryEditorX::I18n
{
	/**
	 * @class PluralParser
	 * @brief Precedence climbing over the expression's characters, building PluralRule's node list.
	 */
	class PluralParser
	{
	public:
		PluralParser(const std::string_view text, PluralRule &rule) : m_Text(text), m_Rule(rule) {}

		bool Parse(std::string *error)
		{
			m_Rule.m_Nodes.clear();
			const int root = Ternary(0);
			Skip();
			if (root < 0 || m_Pos != m_Text.size())
			{
				if (error)
					*error = m_Error.empty() ? "unexpected '" + std::string(m_Text.substr(m_Pos, 8)) + "' in the plural expression" : m_Error;
				return false;
			}
			m_Rule.m_Root = root;
			return true;
		}

	private:
		using Op = PluralRule::Op;

		// Deep enough for any real rule; stops a hostile file from exhausting the stack
		static constexpr int MAX_DEPTH = 64;

		void Skip()
		{
			while (m_Pos < m_Text.size() && std::isspace(static_cast<unsigned char>(m_Text[m_Pos])))
				++m_Pos;
		}

		bool Accept(const std::string_view token)
		{
			Skip();
			if (m_Text.substr(m_Pos, token.size()) != token)
				return false;
			// "<" must not take the first character of "<=", nor "!" that of "!=", nor "=" alone anything
			if (token.size() == 1 && m_Pos + 1 < m_Text.size() && m_Text[m_Pos + 1] == '=' && (token == "<" || token == ">" || token == "!"))
				return false;
			m_Pos += token.size();
			return true;
		}

		int Add(const Op op, const int a = -1, const int b = -1, const int c = -1, const std::uint64_t value = 0)
		{
			m_Rule.m_Nodes.push_back({ op, value, a, b, c });
			return static_cast<int>(m_Rule.m_Nodes.size()) - 1;
		}

		bool Fail(const std::string &message)
		{
			if (m_Error.empty())
				m_Error = message;
			return false;
		}

		int Ternary(const int depth)
		{
			if (depth > MAX_DEPTH)
				return Fail("the plural expression is nested too deeply"), -1;
			const int condition = Or(depth + 1);
			if (condition < 0)
				return -1;
			if (!Accept("?"))
				return condition;
			const int yes = Ternary(depth + 1);
			if (yes < 0)
				return -1;
			if (!Accept(":"))
				return Fail("expected ':' in the plural expression"), -1;
			const int no = Ternary(depth + 1);
			if (no < 0)
				return -1;
			return Add(Op::Select, condition, yes, no);
		}

		int Or(const int depth)
		{
			int left = And(depth);
			while (left >= 0 && Accept("||"))
			{
				const int right = And(depth);
				left = right < 0 ? -1 : Add(Op::Or, left, right);
			}
			return left;
		}

		int And(const int depth)
		{
			int left = Equality(depth);
			while (left >= 0 && Accept("&&"))
			{
				const int right = Equality(depth);
				left = right < 0 ? -1 : Add(Op::And, left, right);
			}
			return left;
		}

		int Equality(const int depth)
		{
			int left = Relational(depth);
			while (left >= 0)
			{
				Op op;
				if (Accept("=="))
					op = Op::Equal;
				else if (Accept("!="))
					op = Op::NotEqual;
				else
					break;
				const int right = Relational(depth);
				left = right < 0 ? -1 : Add(op, left, right);
			}
			return left;
		}

		int Relational(const int depth)
		{
			int left = Additive(depth);
			while (left >= 0)
			{
				Op op;
				if (Accept("<="))
					op = Op::LessEqual;
				else if (Accept(">="))
					op = Op::GreaterEqual;
				else if (Accept("<"))
					op = Op::Less;
				else if (Accept(">"))
					op = Op::Greater;
				else
					break;
				const int right = Additive(depth);
				left = right < 0 ? -1 : Add(op, left, right);
			}
			return left;
		}

		int Additive(const int depth)
		{
			int left = Multiplicative(depth);
			while (left >= 0)
			{
				Op op;
				if (Accept("+"))
					op = Op::Add;
				else if (Accept("-"))
					op = Op::Sub;
				else
					break;
				const int right = Multiplicative(depth);
				left = right < 0 ? -1 : Add(op, left, right);
			}
			return left;
		}

		int Multiplicative(const int depth)
		{
			int left = Unary(depth);
			while (left >= 0)
			{
				Op op;
				if (Accept("*"))
					op = Op::Mul;
				else if (Accept("/"))
					op = Op::Div;
				else if (Accept("%"))
					op = Op::Mod;
				else
					break;
				const int right = Unary(depth);
				left = right < 0 ? -1 : Add(op, left, right);
			}
			return left;
		}

		int Unary(const int depth)
		{
			if (depth > MAX_DEPTH)
				return Fail("the plural expression is nested too deeply"), -1;
			if (Accept("!"))
			{
				const int operand = Unary(depth + 1);
				return operand < 0 ? -1 : Add(Op::Not, operand);
			}
			if (Accept("-"))
			{
				const int operand = Unary(depth + 1);
				return operand < 0 ? -1 : Add(Op::Negate, operand);
			}
			return Primary(depth);
		}

		int Primary(const int depth)
		{
			Skip();
			if (m_Pos >= m_Text.size())
				return Fail("the plural expression ends early"), -1;
			const char c = m_Text[m_Pos];
			if (c == 'n')
			{
				++m_Pos;
				return Add(Op::N);
			}
			if (std::isdigit(static_cast<unsigned char>(c)))
			{
				std::uint64_t value = 0;
				while (m_Pos < m_Text.size() && std::isdigit(static_cast<unsigned char>(m_Text[m_Pos])))
				{
					value = value * 10 + static_cast<std::uint64_t>(m_Text[m_Pos] - '0');
					++m_Pos;
				}
				return Add(Op::Number, -1, -1, -1, value);
			}
			if (c == '(')
			{
				++m_Pos;
				const int inner = Ternary(depth + 1);
				if (inner < 0)
					return -1;
				if (!Accept(")"))
					return Fail("expected ')' in the plural expression"), -1;
				return inner;
			}
			return Fail("unexpected '" + std::string(1, c) + "' in the plural expression"), -1;
		}

		std::string_view m_Text;
		std::size_t m_Pos = 0;
		PluralRule &m_Rule;
		std::string m_Error;
	};

	// -------------------------------------------------------

	PluralRule::PluralRule()
	{
		// n != 1
		m_Nodes = { { Op::N }, { Op::Number, 1 }, { Op::NotEqual, 0, 0, 1 } };
		m_Root = 2;
		m_Count = 2;
	}

	std::optional<PluralRule> PluralRule::ParseExpression(const int nplurals, const std::string_view expression, std::string *error)
	{
		if (nplurals < 1 || nplurals > 16)
		{
			if (error)
				*error = "nplurals must be between 1 and 16";
			return std::nullopt;
		}
		PluralRule rule;
		rule.m_Count = nplurals;
		PluralParser parser(expression, rule);
		if (!parser.Parse(error))
			return std::nullopt;
		return rule;
	}

	std::optional<PluralRule> PluralRule::Parse(std::string_view pluralForms, std::string *error)
	{
		// "nplurals=N; plural=EXPR;" with optional spaces; the trailing ';' is optional
		const auto fail = [error](const char *message) -> std::optional<PluralRule>
		{
			if (error)
				*error = message;
			return std::nullopt;
		};
		const std::size_t npluralsAt = pluralForms.find("nplurals");
		if (npluralsAt == std::string_view::npos)
			return fail("Plural-Forms needs nplurals=N; plural=EXPR;");
		// "plural", spaces, '=' (gettext wants "plural=" exactly; spaces are allowed here)
		std::size_t pluralAt = std::string_view::npos, exprAt = std::string_view::npos;
		for (std::size_t at = pluralForms.find("plural", npluralsAt + 8); at != std::string_view::npos; at = pluralForms.find("plural", at + 6))
		{
			std::size_t eq = at + 6;
			while (eq < pluralForms.size() && pluralForms[eq] == ' ')
				++eq;
			if (eq < pluralForms.size() && pluralForms[eq] == '=')
			{
				pluralAt = at;
				exprAt = eq + 1;
				break;
			}
		}
		if (pluralAt == std::string_view::npos)
			return fail("Plural-Forms needs nplurals=N; plural=EXPR;");

		std::size_t pos = npluralsAt + 8;
		while (pos < pluralForms.size() && pluralForms[pos] == ' ')
			++pos;
		if (pos >= pluralForms.size() || pluralForms[pos] != '=')
			return fail("Plural-Forms: expected '=' after nplurals");
		++pos;
		while (pos < pluralForms.size() && pluralForms[pos] == ' ')
			++pos;
		int count = 0, digits = 0;
		while (pos < pluralForms.size() && std::isdigit(static_cast<unsigned char>(pluralForms[pos])) && digits < 3)
		{
			count = count * 10 + (pluralForms[pos++] - '0');
			++digits;
		}
		if (digits == 0)
			return fail("Plural-Forms: nplurals needs a number");

		std::string_view expression = pluralForms.substr(exprAt);
		while (!expression.empty() && (expression.back() == ';' || std::isspace(static_cast<unsigned char>(expression.back()))))
			expression.remove_suffix(1);
		return ParseExpression(count, expression, error);
	}

	std::uint64_t PluralRule::Evaluate(const int node, const std::uint64_t n) const
	{
		const Node &x = m_Nodes[static_cast<std::size_t>(node)];
		const auto a = [&] { return Evaluate(x.a, n); };
		const auto b = [&] { return Evaluate(x.b, n); };
		switch (x.op)
		{
			case Op::N: return n;
			case Op::Number: return x.value;
			case Op::Not: return a() == 0 ? 1 : 0;
			case Op::Negate: return 0 - a();
			case Op::Mul: return a() * b();
			case Op::Div: { const std::uint64_t d = b(); return d == 0 ? 0 : a() / d; }
			case Op::Mod: { const std::uint64_t d = b(); return d == 0 ? 0 : a() % d; }
			case Op::Add: return a() + b();
			case Op::Sub: return a() - b();
			case Op::Less: return a() < b() ? 1 : 0;
			case Op::LessEqual: return a() <= b() ? 1 : 0;
			case Op::Greater: return a() > b() ? 1 : 0;
			case Op::GreaterEqual: return a() >= b() ? 1 : 0;
			case Op::Equal: return a() == b() ? 1 : 0;
			case Op::NotEqual: return a() != b() ? 1 : 0;
			case Op::And: return (a() != 0 && b() != 0) ? 1 : 0;
			case Op::Or: return (a() != 0 || b() != 0) ? 1 : 0;
			case Op::Select: return a() != 0 ? b() : Evaluate(x.c, n);
		}
		return 0;
	}

	std::size_t PluralRule::Index(const std::uint64_t n) const
	{
		if (m_Root < 0)
			return 0;
		const std::uint64_t index = Evaluate(m_Root, n);
		return static_cast<std::size_t>(index < static_cast<std::uint64_t>(m_Count) ? index : static_cast<std::uint64_t>(m_Count - 1));
	}
}

// -------------------------------------------------------
