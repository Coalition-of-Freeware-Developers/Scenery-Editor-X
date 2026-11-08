#include <catch2/catch_all.hpp>
#include <SceneryEditorX/utils/float_chars.h>
#include <bit>
#include <cmath>
#include <cstdint>
#include <limits>
#include <random>
#include <string>
#include <vector>

/**
 * FloatChars (utils/float_chars.h): std::from_chars / std::to_chars for float and double, with a fallback where Apple's
 * libc++ doesn't have them for the macOS 13.0 deployment target. Where the standard functions exist, the fallback is
 * compared with them character for character; everywhere (macOS included) the public functions are checked against
 * known text and read back what they write.
 */

using namespace SceneryEditorX::Utils;

namespace
{
	template <typename T>
	std::string Write(const T value)
	{
		char buffer[2048];
		const auto [end, ec] = FloatChars::ToChars(buffer, buffer + sizeof(buffer), value);
		REQUIRE(ec == std::errc());
		return std::string(buffer, end);
	}

	template <typename T>
	std::string Write(const T value, const std::chars_format format)
	{
		char buffer[2048];
		const auto [end, ec] = FloatChars::ToChars(buffer, buffer + sizeof(buffer), value, format);
		REQUIRE(ec == std::errc());
		return std::string(buffer, end);
	}

	template <typename T>
	bool SameValue(const T a, const T b)
	{
		if (std::isnan(a) || std::isnan(b))
			return std::isnan(a) && std::isnan(b) && std::signbit(a) == std::signbit(b);
		return std::bit_cast<std::conditional_t<sizeof(T) == 4, uint32_t, uint64_t>>(a) == std::bit_cast<std::conditional_t<sizeof(T) == 4, uint32_t, uint64_t>>(b);
	}

	/* @brief Finite values over every exponent (random bit patterns) plus the edges: zeros, denormals, limits, powers. */
	template <typename T>
	std::vector<T> Samples(const size_t randomCount, const int maxBinaryExponent = std::numeric_limits<int>::max())
	{
		using Bits = std::conditional_t<sizeof(T) == 4, uint32_t, uint64_t>;
		using Limits = std::numeric_limits<T>;
		std::vector<T> values = { T(0), -T(0), T(1), T(-1), T(0.1), T(0.2), T(0.3), T(1) / T(3), T(2) / T(3), T(0.5), T(1.5),
			T(100), T(1e4), T(1e7), T(123456789), T(9007199254740993.0), T(1e23), T(-2.5), T(1e-7), T(0.000123),
			Limits::denorm_min(), Limits::min(), std::nextafter(Limits::min(), T(0)), Limits::max(), Limits::epsilon() };
		for (int power = Limits::min_exponent - Limits::digits; power < Limits::max_exponent; ++power)
			values.push_back(std::ldexp(T(1), power));
		for (int power = Limits::min_exponent10; power <= Limits::max_exponent10; ++power)
		{
			const T ten = static_cast<T>(std::pow(10.0L, power));
			values.push_back(ten);
			values.push_back(std::nextafter(ten, T(0)));
			values.push_back(std::nextafter(ten, Limits::infinity()));
		}
		std::mt19937_64 random(20261108);
		while (values.size() < randomCount + 2000)
		{
			const T value = std::bit_cast<T>(static_cast<Bits>(random()));
			if (std::isfinite(value))
				values.push_back(value);
		}
		std::erase_if(values, [maxBinaryExponent](const T value) { return value != 0 && std::abs(std::ilogb(value)) > maxBinaryExponent; });
		return values;
	}

	const std::vector<std::string> &ReadCases()
	{
		static const std::vector<std::string> cases = { "0", "-0", "1", "-1", "0.1", ".5", "5.", "-.5e3", "1e", "1e+", "1e-",
			"1e5x", "1.5E-3", "00012.5000", "123456789012345678901234567890", "9007199254740993", "1e400", "-1e400", "1e-400",
			"1e-310", "1e-40", "1e-46", "3.4028235e38", "3.4028236e38", "2.2250738585072011e-308", "4.9406564584124654e-324",
			"2.4703282292062328e-324", "0.000000000000000000000000000000000000000000001", "inf", "-INF", "Infinity", "infinit",
			"nan", "-nan", "NaN(123)", "nan(", "nan()", "", "-", ".", "-.", "+1", " 1", "0x10", "-0x1p3", "1,5", "e5", "-e5", "E",
			"1.2.3", "12e3e4", "0.30000000000000004", "1.0000001", "16777217", "-0.0e-0" };
		return cases;
	}
}

TEST_CASE("FloatChars writes known text", "[conversions][float-chars]")
{
	CHECK(Write(0.1f) == "0.1");
	CHECK(Write(1.0f) == "1");
	CHECK(Write(100.0f) == "100");
	CHECK(Write(1e4f) == "10000");				// As short as 1e+04: fixed wins the tie
	CHECK(Write(1e7f) == "1e+07");
	CHECK(Write(1e30f) == "1e+30");
	CHECK(Write(-2.5f) == "-2.5");
	CHECK(Write(1e-7f) == "1e-07");
	CHECK(Write(123456.0f) == "123456");
	CHECK(Write(std::numeric_limits<float>::max()) == "3.4028235e+38");
	CHECK(Write(0.0f) == "0");
	CHECK(Write(-0.0) == "-0");
	CHECK(Write(0.3) == "0.3");
	CHECK(Write(0.1 + 0.2) == "0.30000000000000004");
	CHECK(Write(1e23) == "1e+23");
	CHECK(Write(5e-324) == "5e-324");
	CHECK(Write(std::numeric_limits<double>::infinity()) == "inf");
	CHECK(Write(-std::numeric_limits<float>::infinity()) == "-inf");

	CHECK(Write(0.1f, std::chars_format::fixed) == "0.1");
	CHECK(Write(1.5f, std::chars_format::fixed) == "1.5");
	CHECK(Write(1e-7f, std::chars_format::fixed) == "0.0000001");
	CHECK(Write(100.0f, std::chars_format::fixed) == "100");
	CHECK(Write(-0.25, std::chars_format::fixed) == "-0.25");
	CHECK(Write(0.0f, std::chars_format::fixed) == "0");
	CHECK(Write(0.1f, std::chars_format::scientific) == "1e-01");
	CHECK(Write(123.456, std::chars_format::scientific) == "1.23456e+02");
	CHECK(Write(0.0, std::chars_format::scientific) == "0e+00");

	char small[4];
	const auto [end, ec] = FloatChars::ToChars(small, small + sizeof(small), 0.125f);
	CHECK(ec == std::errc::value_too_large);	// "0.125" doesn't fit in 4 characters
	CHECK(end == small + sizeof(small));
}

TEST_CASE("FloatChars reads known text", "[conversions][float-chars]")
{
	const auto read = [](const std::string &text, double &value) { return FloatChars::FromChars(text.data(), text.data() + text.size(), value); };
	double value = 7;
	auto result = read("0.1", value);
	CHECK((result.ec == std::errc() && value == 0.1));
	result = read("-.5e3", value);
	CHECK((result.ec == std::errc() && value == -500.0));
	const std::string trailingPoint = "5.";
	result = read(trailingPoint, value);
	CHECK((result.ec == std::errc() && value == 5.0 && result.ptr == trailingPoint.data() + 2));

	const std::string exponentOnly = "1e+";
	value = 7;
	result = read(exponentOnly, value);
	CHECK(result.ec == std::errc());
	CHECK(result.ptr == exponentOnly.data() + 1);		// "e+" without digits isn't part of the number
	CHECK(value == 1.0);

	const std::string hex = "0x10";
	result = read(hex, value);
	CHECK((result.ec == std::errc() && value == 0.0 && result.ptr == hex.data() + 1));

	for (const char *bad : { "+1", " 1", ".", "-", "", "e5", "x" })
	{
		value = 7;
		const std::string text = bad;
		result = read(text, value);
		INFO(bad);
		CHECK(result.ec == std::errc::invalid_argument);
		CHECK(result.ptr == text.data());
		CHECK(value == 7);
	}

	const std::string huge = "1e400";
	value = 7;
	result = read(huge, value);
	CHECK(result.ec == std::errc::result_out_of_range);
	CHECK(result.ptr == huge.data() + huge.size());
	CHECK(value == 7);

	float single = 0;
	const std::string infinity = "-Infinity";
	const auto [end, ec] = FloatChars::FromChars(infinity.data(), infinity.data() + infinity.size(), single);
	CHECK(ec == std::errc());
	CHECK(end == infinity.data() + infinity.size());
	CHECK((std::isinf(single) && single < 0));
}

TEMPLATE_TEST_CASE("FloatChars reads back what it writes", "[conversions][float-chars]", float, double)
{
	for (const TestType value : Samples<TestType>(20000, 200))
	{
		for (const std::string &text : { Write(value), Write(value, std::chars_format::fixed), Write(value, std::chars_format::scientific) })
		{
			TestType back = 0;
			const auto [end, ec] = FloatChars::FromChars(text.data(), text.data() + text.size(), back);
			if (ec != std::errc() || end != text.data() + text.size() || !SameValue(back, value))
			{
				INFO(text);
				FAIL("doesn't read back as the value written");
			}
		}
	}
}

#if SEDX_STD_FLOAT_FROM_CHARS_AVAILABLE && SEDX_STD_FLOAT_TO_CHARS_AVAILABLE
TEMPLATE_TEST_CASE("FloatChars' fallback reads text as std::from_chars does", "[conversions][float-chars]", float, double)
{
	std::vector<std::string> texts = ReadCases();
	for (const TestType value : Samples<TestType>(5000, 200))
	{
		char buffer[2048];
		texts.emplace_back(buffer, std::to_chars(buffer, buffer + sizeof(buffer), value).ptr);
		texts.emplace_back(buffer, std::to_chars(buffer, buffer + sizeof(buffer), value, std::chars_format::fixed).ptr);
	}

	std::vector<std::string> mismatches;
	for (const std::string &text : texts)
	{
		TestType expected = TestType(7), actual = TestType(7);
		const auto want = std::from_chars(text.data(), text.data() + text.size(), expected);
		const auto got = FloatChars::Fallback::FromChars(text.data(), text.data() + text.size(), actual);
		// The value only on success: libc++ writes inf / 0 on result_out_of_range, where the standard leaves it
		if (want.ec != got.ec || want.ptr != got.ptr || (want.ec == std::errc() && !SameValue(expected, actual)))
		{
			mismatches.push_back('"' + text + "\": std ec " + std::to_string(static_cast<int>(want.ec)) + " at " + std::to_string(want.ptr - text.data())
				+ ", fallback ec " + std::to_string(static_cast<int>(got.ec)) + " at " + std::to_string(got.ptr - text.data()));
		}
	}
	for (const std::string &mismatch : mismatches)
		UNSCOPED_INFO(mismatch);
	CHECK(mismatches.empty());
}
#endif

#if SEDX_STD_FLOAT_TO_CHARS_AVAILABLE
TEMPLATE_TEST_CASE("FloatChars' fallback writes the text std::to_chars writes", "[conversions][float-chars]", float, double)
{
	// Doubles in fixed notation only up to 2^200 either way: below that %f needs hundreds of digits per try
	const std::vector<TestType> values = Samples<TestType>(20000);
	const int fixedLimit = sizeof(TestType) == 4 ? 1000 : 200;

	std::vector<std::string> mismatches;
	const auto compare = [&mismatches](const std::string &want, const std::string &got)
	{
		if (want != got && mismatches.size() < 20)
			mismatches.push_back(want + " != " + got);
	};
	for (const TestType value : values)
	{
		char buffer[2048];
		compare(std::string(buffer, std::to_chars(buffer, buffer + sizeof(buffer), value).ptr), FloatChars::Fallback::ShortestText(value));
		compare(std::string(buffer, std::to_chars(buffer, buffer + sizeof(buffer), value, std::chars_format::scientific).ptr),
			FloatChars::Fallback::ShortestText(value, true, std::chars_format::scientific));
		if (value == 0 || std::abs(std::ilogb(value)) <= fixedLimit)
			compare(std::string(buffer, std::to_chars(buffer, buffer + sizeof(buffer), value, std::chars_format::fixed).ptr),
				FloatChars::Fallback::ShortestText(value, true, std::chars_format::fixed));
	}
	INFO(mismatches.size() << " mismatches (std != fallback), first: " << (mismatches.empty() ? std::string() : mismatches.front()));
	for (const std::string &mismatch : mismatches)
		UNSCOPED_INFO(mismatch);
	CHECK(mismatches.empty());
}
#endif
