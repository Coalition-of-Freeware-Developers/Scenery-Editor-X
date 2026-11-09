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
 * float_chars.h
 * -------------------------------------------------------
 * Created: 08/11/2026
 * -------------------------------------------------------
 */
#pragma once
#include <charconv>
#include <clocale>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <limits>
#include <string>
#include <system_error>
#include <type_traits>
#include <version>
#if defined(_WIN32)
#include <locale.h>
#include <stdlib.h>
#else
#include <locale.h>
#include <stdlib.h>
#if defined(__APPLE__) || defined(__FreeBSD__)
#include <xlocale.h>
#endif
#endif

/**
 * std::from_chars / std::to_chars for float and double, also where the standard library has them but not for the
 * deployment target: Apple's libc++ marks floating point from_chars "introduced in macOS 26.0" and to_chars "introduced
 * in macOS 13.3", and Scenery Editor X targets macOS 13.0 (cmake/toolchains/sedx-mac.cmake), so calling them there
 * doesn't compile. FloatChars::FromChars / ToChars call the standard functions wherever they exist (libstdc++, the MSVC
 * STL, libc++ off Apple or on a new enough target) and otherwise the Fallback functions below, which follow the same
 * rules: the "C" locale, the same accepted text, and the same shortest text that reads back as the same value.
 *
 * SEDX_STD_FLOAT_*_AVAILABLE say whether this build can call the standard functions; SEDX_STD_FLOAT_* whether
 * FromChars / ToChars do. Define SEDX_FLOAT_CHARS_FALLBACK to use the fallback everywhere (how the whole test suites
 * run as they do on macOS, from any platform).
 */
#if defined(_LIBCPP_VERSION)
/*
 * libc++: its own availability macros where it has them, since they follow the deployment target exactly. They're
 * internal, so if a release renames them: libc++'s version and, on Apple, the first macOS that ships the functions
 * (from_chars 26.0, to_chars 13.3). __builtin_available can't help: libc++ marks them strict, an error at compile time.
 * test_float_chars.cpp fails if a toolchain known to have them stops calling them.
 */
#if defined(__APPLE__) && defined(__ENVIRONMENT_MACOS_VERSION_MIN_REQUIRED__)
#define SEDX_FLOAT_CHARS_MACOS_TARGET __ENVIRONMENT_MACOS_VERSION_MIN_REQUIRED__
#elif defined(__APPLE__)
#define SEDX_FLOAT_CHARS_MACOS_TARGET 0				// iOS and the like: the fallback unless libc++ says otherwise
#else
#define SEDX_FLOAT_CHARS_MACOS_TARGET 999999		// Not Apple: no deployment target to wait for
#endif
#if defined(_LIBCPP_AVAILABILITY_HAS_FROM_CHARS_FLOATING_POINT)
#define SEDX_STD_FLOAT_FROM_CHARS_AVAILABLE _LIBCPP_AVAILABILITY_HAS_FROM_CHARS_FLOATING_POINT
#elif _LIBCPP_VERSION >= 200000 && SEDX_FLOAT_CHARS_MACOS_TARGET >= 260000
#define SEDX_STD_FLOAT_FROM_CHARS_AVAILABLE 1
#else
#define SEDX_STD_FLOAT_FROM_CHARS_AVAILABLE 0
#endif
#if defined(_LIBCPP_AVAILABILITY_HAS_TO_CHARS_FLOATING_POINT)
#define SEDX_STD_FLOAT_TO_CHARS_AVAILABLE _LIBCPP_AVAILABILITY_HAS_TO_CHARS_FLOATING_POINT
#elif _LIBCPP_VERSION >= 170000 && SEDX_FLOAT_CHARS_MACOS_TARGET >= 130300
#define SEDX_STD_FLOAT_TO_CHARS_AVAILABLE 1
#else
#define SEDX_STD_FLOAT_TO_CHARS_AVAILABLE 0
#endif
#elif defined(__cpp_lib_to_chars)
#define SEDX_STD_FLOAT_FROM_CHARS_AVAILABLE 1
#define SEDX_STD_FLOAT_TO_CHARS_AVAILABLE 1
#else
#define SEDX_STD_FLOAT_FROM_CHARS_AVAILABLE 0
#define SEDX_STD_FLOAT_TO_CHARS_AVAILABLE 0
#endif

#if defined(SEDX_FLOAT_CHARS_FALLBACK)
#define SEDX_STD_FLOAT_FROM_CHARS 0
#define SEDX_STD_FLOAT_TO_CHARS 0
#else
#define SEDX_STD_FLOAT_FROM_CHARS SEDX_STD_FLOAT_FROM_CHARS_AVAILABLE
#define SEDX_STD_FLOAT_TO_CHARS SEDX_STD_FLOAT_TO_CHARS_AVAILABLE
#endif

/// -------------------------------------------------------

namespace SceneryEditorX::Utils::FloatChars
{
	/* The standard library's own versions in use (true) or the fallback (false); for tests and logs. */
	inline constexpr bool STD_FROM_CHARS = SEDX_STD_FLOAT_FROM_CHARS != 0;
	inline constexpr bool STD_TO_CHARS = SEDX_STD_FLOAT_TO_CHARS != 0;

	namespace Fallback
	{
		namespace Detail
		{
			inline bool IsDigit(const char c) { return c >= '0' && c <= '9'; }

			/* @brief Whether [at, last) starts with word (lower case), ignoring case. */
			inline bool StartsWith(const char *at, const char *last, const char *word)
			{
				for (; *word; ++word, ++at)
				{
					if (at == last)
						return false;
					const char c = (*at >= 'A' && *at <= 'Z') ? static_cast<char>(*at - 'A' + 'a') : *at;
					if (c != *word)
						return false;
				}
				return true;
			}

			/**
			 * @brief strtod / strtof in the "C" locale, whatever the process or thread locale is: correctly rounded. If
			 * that locale couldn't be made (out of memory), the plain ones: a null locale_t is undefined behaviour.
			 */
		#if defined(_WIN32)
			inline _locale_t CLocale()
			{
				static const _locale_t locale = _create_locale(LC_ALL, "C");
				return locale;
			}
			inline double Parse(const char *text, double *)
			{
				const _locale_t locale = CLocale();
				return locale ? _strtod_l(text, nullptr, locale) : std::strtod(text, nullptr);
			}
			inline float Parse(const char *text, float *)
			{
				const _locale_t locale = CLocale();
				return locale ? _strtof_l(text, nullptr, locale) : std::strtof(text, nullptr);
			}
		#else
			inline locale_t CLocale()
			{
				static const locale_t locale = newlocale(LC_ALL_MASK, "C", static_cast<locale_t>(0));
				return locale;
			}
			inline double Parse(const char *text, double *)
			{
				const locale_t locale = CLocale();
				return locale ? strtod_l(text, nullptr, locale) : std::strtod(text, nullptr);
			}
			inline float Parse(const char *text, float *)
			{
				const locale_t locale = CLocale();
				return locale ? strtof_l(text, nullptr, locale) : std::strtof(text, nullptr);
			}
		#endif

			/* @brief snprintf's %.*e / %.*f of value with '.' as the decimal point (snprintf uses the C locale's). */
			inline std::string Print(const double value, const char conversion, const int precision)
			{
				const char format[] = { '%', '.', '*', conversion, '\0' };
				char buffer[64];	// Not "small": windows.h (rpcndr.h) #defines small as char
				const int size = std::snprintf(buffer, sizeof(buffer), format, precision, value);
				std::string text;
				if (size < 0)
					return text;
				if (static_cast<size_t>(size) < sizeof(buffer))
					text.assign(buffer, static_cast<size_t>(size));
				else
				{
					text.resize(static_cast<size_t>(size) + 1);
					std::snprintf(text.data(), text.size(), format, precision, value);
					text.resize(static_cast<size_t>(size));
				}
				for (char &c : text)
				{
					if (!IsDigit(c) && c != '-' && c != '+' && c != 'e')
						c = '.';
				}
				return text;
			}

			/* @brief Adds one to (up) or takes one from a run of decimal digits; false when that carries out of it. */
			inline bool Step(std::string &digits, const bool up)
			{
				for (size_t i = digits.size(); i-- > 0;)
				{
					if (up ? digits[i] != '9' : digits[i] != '0')
					{
						digits[i] = static_cast<char>(digits[i] + (up ? 1 : -1));
						return true;
					}
					digits[i] = up ? '0' : '9';
				}
				return false;
			}

			/**
			 * @brief The next decimal above (up) or below a non-negative %e or %f text with as many digits after the
			 * point: "1.25e+03" -> "1.26e+03" / "1.24e+03", "1.00e+03" -> "9.99e+02", "9.99e+02" -> "1.00e+03",
			 * "0.99" -> "1.00". Empty when there is none (below zero).
			 */
			inline std::string Neighbour(const std::string &text, const bool up)
			{
				const size_t e = text.find('e');
				const std::string number = text.substr(0, e);
				const size_t dot = number.find('.');
				const size_t decimals = dot == std::string::npos ? 0 : number.size() - dot - 1;
				std::string digits = number;
				if (dot != std::string::npos)
					digits.erase(dot, 1);

				if (e == std::string::npos)
				{
					// Fixed: one unit of the last decimal, the integer part growing or shrinking as needed
					if (!Step(digits, up))
					{
						if (!up)
							return {};
						digits.insert(digits.begin(), '1');
					}
					size_t lead = 0;
					while (lead + 1 < digits.size() - decimals && digits[lead] == '0')
						++lead;
					digits.erase(0, lead);
					std::string out = digits.substr(0, digits.size() - decimals);
					if (decimals)
						out += '.' + digits.substr(digits.size() - decimals);
					return out;
				}

				// Scientific: as many significant digits, the exponent moving at a power of ten
				int exponent = std::atoi(text.c_str() + e + 1);
				const bool power = digits[0] == '1' && digits.find_first_not_of('0', 1) == std::string::npos;
				if (!up && power)
				{
					digits.assign(digits.size(), '9');
					--exponent;
				}
				else if (!Step(digits, up))
				{
					if (!up)
						return {};
					digits = '1' + std::string(digits.size() - 1, '0');
					++exponent;
				}
				std::string out(1, digits[0]);
				if (digits.size() > 1)
					out += '.' + digits.substr(1);
				char tail[16];
				std::snprintf(tail, sizeof(tail), "e%c%02d", exponent < 0 ? '-' : '+', exponent < 0 ? -exponent : exponent);
				return out + tail;
			}
		}

		/**
		 * @brief std::from_chars(first, last, value) with std::chars_format::general: an optional '-', then digits with an
		 * optional '.' and exponent, or inf / infinity / nan / nan(chars) in any case. No leading blanks or '+', no hex.
		 * result_out_of_range (value untouched) when the number is too large or too small for T.
		 */
		template <typename T>
		std::from_chars_result FromChars(const char *first, const char *last, T &value)
		{
			static_assert(std::is_same_v<T, float> || std::is_same_v<T, double>);
			using Detail::IsDigit;
			const char *at = first;
			const bool negative = at != last && *at == '-';
			if (negative)
				++at;

			if (at != last && !IsDigit(*at) && *at != '.')
			{
				if (Detail::StartsWith(at, last, "inf"))
				{
					const T infinity = std::numeric_limits<T>::infinity();
					value = negative ? -infinity : infinity;
					return { at + (Detail::StartsWith(at, last, "infinity") ? 8 : 3), std::errc() };
				}
				if (Detail::StartsWith(at, last, "nan"))
				{
					const char *end = at + 3;
					if (end != last && *end == '(')
					{
						const char *close = end + 1;
						while (close != last && (IsDigit(*close) || *close == '_' || (*close >= 'a' && *close <= 'z') || (*close >= 'A' && *close <= 'Z')))
							++close;
						if (close != last && *close == ')')
							end = close + 1;
					}
					const T nan = std::numeric_limits<T>::quiet_NaN();
					value = negative ? -nan : nan;
					return { end, std::errc() };
				}
				return { first, std::errc::invalid_argument };
			}

			bool anyDigit = false, nonZero = false;
			for (; at != last && IsDigit(*at); ++at)
			{
				anyDigit = true;
				nonZero |= *at != '0';
			}
			if (at != last && *at == '.')
			{
				for (++at; at != last && IsDigit(*at); ++at)
				{
					anyDigit = true;
					nonZero |= *at != '0';
				}
			}
			if (!anyDigit)
				return { first, std::errc::invalid_argument };
			if (at != last && (*at == 'e' || *at == 'E'))
			{
				const char *exponent = at + 1;
				if (exponent != last && (*exponent == '+' || *exponent == '-'))
					++exponent;
				if (exponent != last && IsDigit(*exponent))
				{
					while (exponent != last && IsDigit(*exponent))
						++exponent;
					at = exponent;
				}
			}

			const std::string text(first, at);
			const T parsed = Detail::Parse(text.c_str(), static_cast<T *>(nullptr));
			if (std::isinf(parsed) || (parsed == 0 && nonZero))
				return { at, std::errc::result_out_of_range };
			value = parsed;
			return { at, std::errc() };
		}

		/**
		 * @brief The text std::to_chars(first, last, value[, format]) writes: the fewest characters that FromChars reads
		 * back as value, the closest to it when several are as short. Without a format, %e or %f style, whichever is
		 * shorter (%f on a tie); format is std::chars_format::fixed or ::scientific (others give invalid_argument).
		 */
		template <typename T>
		std::string ShortestText(const T value, const bool hasFormat = false, const std::chars_format format = std::chars_format::general)
		{
			static_assert(std::is_same_v<T, float> || std::is_same_v<T, double>);
			if (std::isnan(value))
				return std::signbit(value) ? "-nan" : "nan";
			if (std::isinf(value))
				return value < 0 ? "-inf" : "inf";

			const bool negative = std::signbit(value);
			const T magnitude = std::fabs(value);
			const auto readsBack = [magnitude](const std::string &text)
			{
				T back = 0;
				const auto [end, ec] = FromChars(text.data(), text.data() + text.size(), back);
				return ec == std::errc() && end == text.data() + text.size() && back == magnitude;
			};
			// The closest text with this many digits after the point, or the one on the other side of the value
			const auto candidate = [&](const char conversion, const int precision, std::string &text)
			{
				text = Detail::Print(static_cast<double>(magnitude), conversion, precision);
				if (readsBack(text))
					return true;
				const double printed = Detail::Parse(text.c_str(), static_cast<double *>(nullptr));
				text = Detail::Neighbour(text, printed < static_cast<double>(magnitude));
				return !text.empty() && readsBack(text);
			};

			std::string scientific, fixed;
			if (!hasFormat || format == std::chars_format::scientific)
			{
				for (int precision = 0; precision < std::numeric_limits<T>::max_digits10; ++precision)
				{
					if (candidate('e', precision, scientific))
						break;
				}
				if (hasFormat)
					return (negative ? "-" : "") + scientific;
			}
			if (!hasFormat || format == std::chars_format::fixed)
			{
				std::string text;
				for (int precision = 0;; ++precision)
				{
					const bool found = candidate('f', precision, text);
					if (!hasFormat && text.size() > scientific.size() && Detail::Print(static_cast<double>(magnitude), 'f', precision).size() > scientific.size())
						break;		// Every longer %f is longer than the %e text
					if (found)
					{
						fixed = text;
						break;
					}
				}
				if (hasFormat)
					return (negative ? "-" : "") + fixed;
			}
			if (hasFormat)
				return {};
			const std::string &shortest = (!fixed.empty() && fixed.size() <= scientific.size()) ? fixed : scientific;
			return (negative ? "-" : "") + shortest;
		}

		/* @brief Copies text to [first, last) as std::to_chars does: value_too_large and last when it doesn't fit. */
		inline std::to_chars_result Write(char *first, char *last, const std::string &text)
		{
			if (text.empty())
				return { first, std::errc::invalid_argument };
			if (static_cast<size_t>(last - first) < text.size())
				return { last, std::errc::value_too_large };
			std::memcpy(first, text.data(), text.size());
			return { first + text.size(), std::errc() };
		}

		template <typename T>
		std::to_chars_result ToChars(char *first, char *last, const T value)
		{
			return Write(first, last, ShortestText(value));
		}

		template <typename T>
		std::to_chars_result ToChars(char *first, char *last, const T value, const std::chars_format format)
		{
			return Write(first, last, ShortestText(value, true, format));
		}
	}

	/// -------------------------------------------------------

	/**
	 * @brief std::from_chars for float / double (std::chars_format::general), wherever this build can call it. value is
	 * left as it was on any error, as the standard says (libc++ writes inf / 0 when the number is out of range).
	 */
	template <typename T>
	std::from_chars_result FromChars(const char *first, const char *last, T &value)
	{
		static_assert(std::is_same_v<T, float> || std::is_same_v<T, double>);
	#if SEDX_STD_FLOAT_FROM_CHARS
		T parsed = value;
		const std::from_chars_result result = std::from_chars(first, last, parsed);
		if (result.ec == std::errc())
			value = parsed;
		return result;
	#else
		return Fallback::FromChars(first, last, value);
	#endif
	}

	/* @brief std::to_chars for float / double: the shortest text that reads back as value. */
	template <typename T>
	std::to_chars_result ToChars(char *first, char *last, const T value)
	{
		static_assert(std::is_same_v<T, float> || std::is_same_v<T, double>);
	#if SEDX_STD_FLOAT_TO_CHARS
		return std::to_chars(first, last, value);
	#else
		return Fallback::ToChars(first, last, value);
	#endif
	}

	/* @brief std::to_chars for float / double in a format: std::chars_format::fixed or ::scientific everywhere. */
	template <typename T>
	std::to_chars_result ToChars(char *first, char *last, const T value, const std::chars_format format)
	{
		static_assert(std::is_same_v<T, float> || std::is_same_v<T, double>);
	#if SEDX_STD_FLOAT_TO_CHARS
		return std::to_chars(first, last, value, format);
	#else
		return Fallback::ToChars(first, last, value, format);
	#endif
	}
}
