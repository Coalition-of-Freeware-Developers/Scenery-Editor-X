/**
* -------------------------------------------------------
* Scenery Editor X
* -------------------------------------------------------
* Copyright (c) 2025 Thomas Ray
* Copyright (c) 2025 Coalition of Freeware Developers
* -------------------------------------------------------
* string_utils.h
* -------------------------------------------------------
* Created: 14/7/2025
* -------------------------------------------------------
*/
#pragma once
#include <algorithm>
#include <cassert>
#include <chrono>
#include <filesystem>
#include <memory>
#include <string>
#include <string_view>
#include <vector>

/// -------------------------------------------------------

namespace SceneryEditorX::Utils
{
	namespace String
    {
		bool EqualsIgnoreCase(std::string_view a, std::string_view b);
		std::string &ToLower(std::string &string);
		std::string ToLowerCopy(std::string_view string);
		std::string &ToUpper(std::string &string);
		std::string ToUpperCopy(std::string_view string);
		void Erase(std::string &str, const char *chars);
		void Erase(std::string &str, const std::string &chars);
		std::string SubStr(const std::string &string, size_t offset, size_t count = std::string::npos);
		std::string TrimWhitespace(const std::string &str);
		std::string RemoveWhitespace(const std::string &str);
		std::string GetCurrentTimeString(bool includeDate = false, bool useDashes = false);
		int32_t CompareCase(std::string_view a, std::string_view b);
	}

	/// ==============================================================================

	/// Insert delimiter before each upper case character.
    std::string SplitAtUpperCase(std::string_view string, std::string_view delimiter = " ", bool ifLowerCaseOnTheRight = true);
    std::string BytesToString(uint64_t bytes);

    int SkipBOM(std::istream &in);
    std::string ReadFileAndSkipBOM(const std::filesystem::path &filepath);

	std::string_view GetFilename(std::string_view filepath);
    std::string GetExtension(const std::string &filename);
    std::string RemoveExtension(const std::string &filename);
#if 0 /// Replaced by constexpr version
	bool StartsWith(const std::string& string, const std::string& start);
#endif

    /// Keeps delimiters except for spaces, used for shaders
    std::vector<std::string> SplitStringAndKeepDelims(std::string str);
    std::vector<std::string> SplitString(std::string_view string, const std::string_view &delimiters);
    std::vector<std::string> SplitString(std::string_view string, char delimiter);

	///< Helper functions
	inline bool isWhitespace(const char c) { return c == ' ' || (c <= 13 && c >= 9); }
	inline bool isDigit(const char c) { return static_cast<uint32_t>(c - '0') < 10; }

	/// Replaces all occurrences of a one or more substrings.
	/// The arguments must be a sequence of pairs of strings, where the first of each pair is the string to
	/// look for, followed by its replacement.
	template <typename StringType, typename... OtherReplacements>
	std::string replace(StringType textToSearch, std::string_view firstToReplace, std::string_view firstReplacement, OtherReplacements&&... otherPairsOfStringsToReplace);

	/// Returns a string with any whitespace trimmed from its start and end.
	std::string trim(std::string textToTrim);

	/// Returns a string with any whitespace trimmed from its start and end.
	std::string_view trim(std::string_view textToTrim);

	/// Returns a string with any whitespace trimmed from its start and end.
	std::string_view trim(const char* textToTrim);

	/// Returns a string with any whitespace trimmed from its start.
	std::string trimStart(std::string textToTrim);

	/// Returns a string with any whitespace trimmed from its start.
	std::string_view trimStart(std::string_view textToTrim);

	/// Returns a string with any whitespace trimmed from its start.
	std::string_view trimStart(const char* textToTrim);

	/// Returns a string with any whitespace trimmed from its end.
	std::string trimEnd(std::string textToTrim);

	/// Returns a string with any whitespace trimmed from its end.
	std::string_view trimEnd(std::string_view textToTrim);

	/// Returns a string with any whitespace trimmed from its end.
	std::string_view trimEnd(const char* textToTrim);

	/// If the given character is at the start and end of the string, it trims it away.
	std::string removeOuterCharacter(std::string text, char outerChar);

	inline std::string removeDoubleQuotes(std::string text) { return removeOuterCharacter(std::move(text), '"'); }
	inline std::string removeSingleQuotes(std::string text) { return removeOuterCharacter(std::move(text), '\''); }

	inline std::string addDoubleQuotes(std::string text) { return "\"" + std::move(text) + "\""; }
	inline std::string addSingleQuotes(std::string text) { return "'" + std::move(text) + "'"; }

	std::string toLowerCase(std::string);
	std::string toUpperCase(std::string);

	template <typename IsDelimiterChar>
	std::vector<std::string> splitString(std::string_view textToSplit, IsDelimiterChar&& isDelimiterChar, bool includeDelimitersInResult);

	template <typename CharStartsDelimiter, typename CharIsInDelimiterBody>
	std::vector<std::string> splitString(std::string_view textToSplit, CharStartsDelimiter&& isDelimiterStart, CharIsInDelimiterBody&& isDelimiterBody, bool includeDelimitersInResult);

	std::vector<std::string> splitString(std::string_view textToSplit, char delimiterCharacter, bool includeDelimitersInResult);

	std::vector<std::string> splitAtWhitespace(std::string_view text, bool keepDelimiters = false);

	/// Splits a string at newline characters, returning an array of strings.
	std::vector<std::string> splitIntoLines(std::string_view text, bool includeNewLinesInResult);

	/// Joins some kind of array of strings into a single string, adding the given separator
	/// between them (but not adding it at the start or end)
	template <typename ArrayOfStrings>
	std::string joinStrings(const ArrayOfStrings& strings, std::string_view separator);

	/// Returns true if this text contains the given sub-string.
	bool contains(std::string_view text, std::string_view possibleSubstring);
	/// Returns true if this text starts with the given character.
	bool startsWith(std::string_view text, char possibleStart);
	/// Returns true if this text starts with the given sub-string.
	bool startsWith(std::string_view text, std::string_view possibleStart);
	/// Returns true if this text ends with the given sub-string.
	bool endsWith(std::string_view text, char possibleEnd);
	/// Returns true if this text ends with the given sub-string.
	bool endsWith(std::string_view text, std::string_view possibleEnd);

	/// Calculates the Levenshtein distance between two strings.
	template <typename StringType>
	size_t getLevenshteinDistance(const StringType& string1, const StringType& string2);

	/// Converts a hex character to a number 0-15, or -1 if it's not a valid hex digit.
	int hexToInt(uint32_t unicodeChar);

	/// Returns a hex string for the given value.
	/// If the minimum number of digits is non-zero, it will be zero-padded to fill this length;
	template <typename IntegerType>
	std::string createHexString(IntegerType value, int minNumDigits = 0);

	/// Returns a truncated, easy-to-read version of a time as hours, seconds or milliseconds,
	/// depending on its magnitude. The use-cases include things like logging or console app output.
	std::string getDurationDescription(std::chrono::duration<double, std::micro>);

	/// Returns an easy-to-read description of a size in bytes. Depending on the magnitude,
	/// it might choose different units such as GB, MB, KB or just bytes.
	std::string getByteSizeDescription(uint64_t sizeInBytes);

    /// ==============================================================================

    template <class... Durations, class DurationIn>
    std::tuple<Durations...> BreakDownDuration(DurationIn d)
    {
        std::tuple<Durations...> retrieval;
        using discard = int[];
        (void)discard{0, (void((std::get<Durations>(retrieval) =
            std::chrono::duration_cast<Durations>(d), d -= std::chrono::duration_cast<DurationIn>(std::get<Durations>(retrieval)))), 0)...};

        return retrieval;
    }

    /// ==============================================================================

    std::string DurationToString(std::chrono::duration<double> duration);

    /// ==============================================================================

    template <typename IsAlreadyUsedFn>
    std::string AddSuffixToMakeUnique(const std::string &name, IsAlreadyUsedFn &&isUsed)
    {
        auto nameToUse = name;
        int suffix = 1;

        while (isUsed(nameToUse))
            nameToUse = name + "_" + std::to_string(++suffix);

        return nameToUse;
    }

    /// ==============================================================================

    /// Get<float> -> Get (Float)
    std::string TemplateToParenthesis(std::string_view name);

    /// Useful for 'Described' types to display type name in GUI or to serialize.
    std::string CreateUserFriendlyTypeName(std::string_view name);

    /// ==============================================================================

    /// constexpr utilities
    constexpr bool StartsWith(const std::string_view t, const std::string_view s)
    {
        const auto len = s.length();
        return t.length() >= len && t.substr(0, len) == s;
    }

    constexpr bool EndsWith(const std::string_view t, const std::string_view s)
    {
        const auto len1 = t.length();
        const auto len2 = s.length();
        return len1 >= len2 && t.substr(len1 - len2) == s;
    }

    constexpr size_t GetNumberOfTokens(std::string_view source, const std::string_view delimiter)
    {
        size_t count = 1;
        auto pos = source.begin();
        while (pos != source.end())
        {
            if (std::string_view(&*pos, delimiter.size()) == delimiter)
                ++count;

            ++pos;
        }
        return count;
    }

    /// ==============================================================================

    template <size_t N>
    constexpr std::array<std::string_view, N> SplitString(std::string_view source, const std::string_view delimiter)
    {
        std::array<std::string_view, N> tokens;

        auto tokenStart = source.begin();
        auto pos = tokenStart;

        size_t i = 0;

        while (pos != source.end())
        {
            if (std::string_view(&*pos, delimiter.size()) == delimiter)
            {
                tokens[i] = std::string_view(&*tokenStart, (pos - tokenStart));
                tokenStart = pos += delimiter.size();
                ++i;
            }
            else
            {
                ++pos;
            }
        }

        if (pos != source.begin())
            tokens[N - 1] = std::string_view(&*tokenStart, pos - tokenStart);

        return tokens;
    }

    constexpr std::string_view RemoveNamespace(const std::string_view name)
    {
        if (const auto pos = name.find_last_of(':'); pos == std::string_view::npos)
            return name;

        return name.substr(name.find_last_of(':') + 1);
    }

    constexpr std::string_view RemoveOuterNamespace(const std::string_view name)
    {
        const auto first = name.find_first_of(':');
        if (first == std::string_view::npos)
            return name;

        if (first < name.size() - 1 && name[first + 1] == ':')
            return name.substr(first + 2);

        return name.substr(first + 1);
    }

    /// ==============================================================================

    template <size_t N>
    constexpr std::array<std::string_view, N> RemoveNamespace(std::array<std::string_view, N> memberList)
    {
        for (std::string_view &fullName : memberList)
            fullName = RemoveNamespace(fullName);

        return memberList;
    }

    constexpr std::string_view RemovePrefixAndSuffix(std::string_view name)
    {
        if (StartsWith(name, "in_"))
            name.remove_prefix(sizeof("in_") - 1);
        else if (StartsWith(name, "out_"))
            name.remove_prefix(sizeof("out_") - 1);

        if (EndsWith(name, "_Raw"))
            name.remove_suffix(sizeof("_Raw") - 1);

        return name;
    }

    /// -------------------------------------------------------

	inline int hexToInt(const uint32_t unicodeChar)
	{
	    const auto d1 = unicodeChar - static_cast<uint32_t>('0'); if (d1 < 10u)  return static_cast<int>(d1);
	    const auto d2 = d1 + static_cast<uint32_t>('0' - 'a'); if (d2 < 6u)   return static_cast<int>(d2 + 10);
        if (const auto d3 = d2 + static_cast<uint32_t>('a' - 'A'); d3 < 6u)   return static_cast<int>(d3 + 10);
	    return -1;
	}

	template <typename IntegerType>
	std::string createHexString(IntegerType value, int minNumDigits)
	{
	    static_assert(std::is_integral_v<IntegerType>, "Need to pass integers into this method");
	    auto intvalue = static_cast<std::make_unsigned_t<IntegerType>>(intvalue);
	    assert(minNumDigits <= 32);

	    char hex[40];
	    const auto end = hex + sizeof(hex) - 1;
	    auto d = end;
	    *d = 0;

	    for (;;)
	    {
	        *--d = "0123456789abcdef"[static_cast<uint32_t>(intvalue) & 15u];
	        intvalue = static_cast<decltype(intvalue)>(intvalue >> 4);
	        --minNumDigits;

	        if (intvalue == 0 && minNumDigits <= 0)
	            return std::string(d, end);
	    }
	}

	template <typename StringType, typename... OtherReplacements>
	std::string replace(StringType textToSearch, std::string_view firstToReplace, std::string_view firstReplacement, OtherReplacements&&... otherPairsOfStringsToReplace)
	{
	    static_assert((sizeof...(otherPairsOfStringsToReplace) & 1u) == 0, "This function expects a list of pairs of strings as its arguments");

	    if constexpr (std::is_same_v<const StringType, const std::string_view> || std::is_same_v<const StringType, const char* const>)
            return replace(std::string(textToSearch), firstToReplace, firstReplacement, std::forward<OtherReplacements>(otherPairsOfStringsToReplace)...);
        else if constexpr (sizeof...(otherPairsOfStringsToReplace) == 0)
	    {
	        size_t pos = 0;

	        for (;;)
	        {
	            pos = textToSearch.find(firstToReplace, pos);

	            if (pos == std::string::npos)
	                return textToSearch;

	            textToSearch.replace(pos, firstToReplace.length(), firstReplacement);
	            pos += firstReplacement.length();
	        }
	    }
	    else
            return replace(replace(std::move(textToSearch), firstToReplace, firstReplacement), std::forward<OtherReplacements>(otherPairsOfStringsToReplace)...);
    }

	inline std::string      trim      (std::string      textToTrim)			{ return trimStart(trimEnd(std::move(textToTrim))); }
	inline std::string_view trim	  (const std::string_view textToTrim)   { return trimStart(trimEnd(textToTrim)); }
	inline std::string_view trim      (const char* textToTrim)				{ return trim      (std::string_view(textToTrim)); }
	inline std::string_view trimStart (const char* textToTrim)				{ return trimStart (std::string_view(textToTrim)); }
	inline std::string_view trimEnd   (const char* textToTrim)				{ return trimEnd   (std::string_view(textToTrim)); }

	inline std::string trimStart(std::string textToTrim)
	{
	    auto i = textToTrim.begin();

	    if (i == textToTrim.end())        return {};
	    if (!isWhitespace(*i))    return textToTrim;

	    for (;;)
	    {
	        ++i;

	        if (i == textToTrim.end())        return {};
	        if (!isWhitespace(*i))    return { i, textToTrim.end() };
	    }
	}

	inline std::string_view trimStart(std::string_view textToTrim)
	{
	    size_t i = 0;

	    for (const auto c : textToTrim)
	    {
	        if (!isWhitespace(c))
	        {
	            textToTrim.remove_prefix(i);
	            return textToTrim;
	        }

	        ++i;
	    }

	    return {};
	}

	inline std::string trimEnd(std::string textToTrim)
	{
	    for (auto i = textToTrim.end();;)
	    {
	        if (i == textToTrim.begin())
	            return {};

	        --i;

	        if (!isWhitespace(*i))
	        {
	            textToTrim.erase(i + 1, textToTrim.end());
	            return textToTrim;
	        }
	    }
	}

	inline std::string_view trimEnd(const std::string_view textToTrim)
	{
	    for (auto i = textToTrim.length(); i != 0; --i)
	        if (!isWhitespace(textToTrim[i - 1]))
	            return textToTrim.substr(0, i);

	    return {};
	}

	inline std::string removeOuterCharacter(std::string text, const char outerChar)
	{
	    if (text.length() >= 2 && text.front() == outerChar && text.back() == outerChar)
	        return text.substr(1, text.length() - 2);

	    return text;
	}

	inline std::string toLowerCase(std::string s)
	{
	    std::ranges::transform(s, s.begin(), [](auto c) { return static_cast<char>(std::tolower(static_cast<unsigned char>(c))); });
	    return s;
	}

	inline std::string toUpperCase(std::string s)
	{
	    std::ranges::transform(s, s.begin(), [](auto c) { return static_cast<char>(std::toupper(static_cast<unsigned char>(c))); });
	    return s;
	}

	template <typename CharStartsDelimiter, typename CharIsInDelimiterBody>
	std::vector<std::string> splitString(std::string_view textToSplit, CharStartsDelimiter&& isDelimiterStart, CharIsInDelimiterBody&& isDelimiterBody, const bool includeDelimitersInResult)
	{
	    std::vector<std::string> tokens;
	    auto tokenStart = textToSplit.begin();
	    auto pos = tokenStart;

	    while (pos != textToSplit.end())
	    {
	        if (isDelimiterStart(*pos))
	        {
	            auto delimiterStart = pos++;

	            while (pos != textToSplit.end() && isDelimiterBody(*pos))
	                ++pos;

	            if (pos != textToSplit.begin())
	                tokens.emplace_back(tokenStart, includeDelimitersInResult ? pos : delimiterStart);

	            tokenStart = pos;
	        }
	        else
	        {
	            ++pos;
	        }
	    }

	    if (pos != textToSplit.begin())
	        tokens.emplace_back(tokenStart, pos);

	    return tokens;
	}

	template <typename IsDelimiterChar>
	std::vector<std::string> splitString(std::string_view textToSplit, IsDelimiterChar&& isDelimiterChar, const bool includeDelimitersInResult)
	{
	    std::vector<std::string> tokens;
	    auto tokenStart = textToSplit.begin();
	    auto pos = tokenStart;

	    while (pos != textToSplit.end())
	    {
	        if (isDelimiterChar(*pos))
	        {
	            tokens.emplace_back(tokenStart, includeDelimitersInResult ? pos + 1 : pos);
	            tokenStart = ++pos;
	        }
	        else
	        {
	            ++pos;
	        }
	    }

	    if (pos != textToSplit.begin())
	        tokens.emplace_back(tokenStart, pos);

	    return tokens;
	}

	inline std::vector<std::string> splitString(const std::string_view textToSplit, const char delimiterCharacter, const bool includeDelimitersInResult)
	{
	    return splitString(textToSplit, [=](const char c) { return c == delimiterCharacter; }, includeDelimitersInResult);
	}

	inline std::vector<std::string> splitAtWhitespace(const std::string_view text, const bool keepDelimiters)
	{
	    return splitString(text,
	                      [](const char c) { return isWhitespace(c); },
	                      [](const char c) { return isWhitespace(c); }, keepDelimiters);
	}

	inline std::vector<std::string> splitIntoLines(const std::string_view text, const bool includeNewLinesInResult)
	{
	    return splitString(text, '\n', includeNewLinesInResult);
	}

	template <typename ArrayOfStrings>
    std::string joinStrings(const ArrayOfStrings& strings, const std::string_view separator)
	{
	    if (strings.empty())
	        return {};

	    auto spaceNeeded = separator.length() * strings.size();

	    for (auto& s : strings)
	        spaceNeeded += s.length();

	    std::string result(strings.front());
	    result.reserve(spaceNeeded);

	    for (size_t i = 1; i < strings.size(); ++i)
	    {
	        result += separator;
	        result += strings[i];
	    }

	    return result;
	}

	inline bool contains(const std::string_view text, const std::string_view possibleSubstring)   { return text.find(possibleSubstring) != std::string::npos; }
	inline bool startsWith(const std::string_view text, const char possibleStart)             { return !text.empty() && text.front() == possibleStart; }
	inline bool endsWith(const std::string_view text, const char possibleEnd)               { return !text.empty() && text.back() == possibleEnd; }

	inline bool startsWith(const std::string_view text, const std::string_view possibleStart)
	{
	    const auto len = possibleStart.length();
	    return text.length() >= len && text.substr(0, len) == possibleStart;
	}

	inline bool endsWith(const std::string_view text, const std::string_view possibleEnd)
	{
	    const auto len1 = text.length();
        const auto len2 = possibleEnd.length();
        return len1 >= len2 && text.substr(len1 - len2) == possibleEnd;
	}

	inline std::string getDurationDescription(const std::chrono::duration<double, std::micro> d)
	{
	    const auto microseconds = std::chrono::duration_cast<std::chrono::microseconds>(d).count();

	    if (microseconds < 0)    return "-" + getDurationDescription(-d);
	    if (microseconds == 0)   return "0 sec";

	    std::string result;

	    auto addLevel = [&](const int64_t size, const std::string_view units, const int64_t decimalScale, const int64_t modulo) -> bool
	    {
	        if (microseconds < size)
	            return false;

	        if (!result.empty())
	            result += ' ';

	        const auto scaled = (microseconds * decimalScale + size / 2) / size;
	        auto whole = scaled / decimalScale;

	        if (modulo != 0)
	            whole = whole % modulo;

	        result += std::to_string(whole);

	        if (const auto fraction = scaled % decimalScale)
	        {
	            result += '.';
	            result += static_cast<char>('0' + (fraction / 10));

	            if (fraction % 10 != 0)
	                result += static_cast<char>('0' + (fraction % 10));
	        }

	        result += whole == 1 && units.length() > 3 && units.back() == 's' ? units.substr(0, units.length() - 1) : units;
	        return true;
	    };

	    const bool hours = addLevel(60000000ll * 60ll, " hours", 1, 0);
	    const bool mins  = addLevel(60000000ll,        " min", 1, hours ? 60 : 0);

	    if (hours)
	        return result;

	    if (mins) addLevel(1000000, " sec", 1, 60);
	    else if (!addLevel(1000000,   " sec", 100, 0))
	    {
		    if (!addLevel(1000,  " ms", 100, 0))
			    addLevel(1,       " microseconds", 100, 0);
	    }

	    return result;
	}

	template <typename StringType>
	size_t getLevenshteinDistance(const StringType& string1, const StringType& string2)
	{
	    if (string1.empty())  return string2.length();
	    if (string2.empty())  return string1.length();

	    auto calculate = [](size_t* costs, const size_t numCosts, const StringType& s1, const StringType& s2) -> size_t
	    {
	        for (size_t i = 0; i < numCosts; ++i)
	            costs[i] = i;

	        size_t p1 = 0;

	        for (auto c1 : s1)
	        {
	            auto corner = p1;
	            *costs = p1 + 1;
	            size_t p2 = 0;

	            for (auto c2 : s2)
	            {
	                auto upper = costs[p2 + 1];
	                costs[p2 + 1] = c1 == c2 ? corner : std::min({costs[p2],upper, corner}) + 1;
	                ++p2;
	                corner = upper;
	            }

	            ++p1;
	        }

	        return costs[numCosts - 1];
	    };

	    auto sizeNeeded = string2.length() + 1;
	    constexpr size_t maxStackSize = 96;

	    if (sizeNeeded <= maxStackSize)
	    {
	        size_t costs[maxStackSize];
	        return calculate(costs, sizeNeeded, string1, string2);
	    }

	    std::unique_ptr<size_t[]> costs(new size_t[sizeNeeded]);
	    return calculate(costs.get(), sizeNeeded, string1, string2);
	}

	inline std::string getByteSizeDescription(const uint64_t sizeInBytes)
	{
	    auto intToStringWith1DecPlace = [](const uint64_t n, const uint64_t divisor) -> std::string
	    {
	        const auto scaled = (n * 10 + divisor / 2) / divisor;
	        auto result = std::to_string(scaled / 10);

	        if (const auto fraction = scaled % 10)
	        {
	            result += '.';
	            result += static_cast<char>('0' + fraction);
	        }

	        return result;
	    };

	    static constexpr uint64_t maxValue = std::numeric_limits<uint64_t>::max() / 10;

	    if (sizeInBytes >= 0x40000000)  return intToStringWith1DecPlace(std::min(maxValue, sizeInBytes), 0x40000000) + " GB";
	    if (sizeInBytes >= 0x100000)    return intToStringWith1DecPlace(sizeInBytes, 0x100000) + " MB";
	    if (sizeInBytes >= 0x400)       return intToStringWith1DecPlace(sizeInBytes, 0x400)    + " KB";
	    if (sizeInBytes != 1)           return std::to_string(sizeInBytes) + " bytes";

	    return "1 byte";
	}

    /// -------------------------------------------------------

}

/// -------------------------------------------------------
