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
 * string_utils.h
 * -------------------------------------------------------
 * Created: 14/7/2025
 * -------------------------------------------------------
 */
#pragma once
#include <algorithm>
#include <array>
#include <chrono>
#include <filesystem>
#include <memory>
#include <string>
#include <string_view>
#include <vector>

// -------------------------------------------------------

namespace SceneryEditorX::Utils
{
	/**
	 * @namespace String
	 * @brief Utility functions for string manipulation and operations.
	 */
	namespace String
	{
		/**
		 * @brief Compare two strings for equality ignoring character case.
		 *
		 * Performs a case-insensitive comparison of the contents of the two
		 * string views and returns true when they represent the same sequence
		 * of characters regardless of letter case.
		 *
		 * @param a First string to compare.
		 * @param b Second string to compare.
		 * @return True if strings are equal case-insensitively, false otherwise.
		 */
		bool EqualsIgnoreCase(std::string_view a, std::string_view b);

		/**
		 * @brief Convert a string in-place to lower-case.
		 *
		 * Modifies the provided std::string converting all characters to their
		 * lower-case equivalents using the C locale. Returns a reference to the
		 * modified string for convenience.
		 *
		 * @param string The string to convert to lower-case (modified in-place).
		 * @return A reference to the converted string.
		 */
		std::string &ToLower(std::string &string);

		/**
		 * @brief Return a lower-case copy of the given string.
		 *
		 * Does not modify the input. The returned std::string contains the
		 * lower-case representation of the provided argument.
		 *
		 * @param string The string to copy and convert to lower-case.
		 * @return A new std::string containing the lower-case characters.
		 */
		std::string ToLower(const std::string &string);

		/**
		 * @brief Return a lower-case copy of the given string view.
		 *
		 * Convenience overload that accepts a string_view and returns a newly
		 * allocated std::string with all characters converted to lower-case.
		 *
		 * @param string The string view to convert.
		 * @return A new std::string containing the lower-case characters.
		 */
		std::string ToLowerCopy(std::string_view string);

		/**
		 * @brief Convert a string in-place to upper-case.
		 *
		 * Modifies the provided std::string converting all characters to their
		 * upper-case equivalents using the C locale. Returns a reference to the
		 * modified string for convenience.
		 *
		 * @param string The string to convert to upper-case (modified in-place).
		 * @return A reference to the converted string.
		 */
		std::string &ToUpper(std::string &string);

		/**
		 * @brief Return an upper-case copy of the given string view.
		 *
		 * Convenience overload that accepts a string_view and returns a newly
		 * allocated std::string with all characters converted to upper-case.
		 *
		 * @param string The string view to convert.
		 * @return A new std::string containing the upper-case characters.
		 */
		std::string ToUpperCopy(std::string_view string);

		/**
		 * @brief Remove all characters from a string that are present in the given set.
		 *
		 * Each character in the null-terminated "chars" array is removed from
		 * the input string wherever it occurs.
		 *
		 * @param str The string to modify in-place.
		 * @param chars A null-terminated C-string containing characters to erase.
		 */
		void Erase(std::string &str, const char *chars);

		/**
		 * @brief Remove all characters from a string that are present in the given set.
		 *
		 * Each character contained in the "chars" string will be removed from
		 * the input string wherever it occurs.
		 *
		 * @param str The string to modify in-place.
		 * @param chars A string containing characters to erase.
		 */
		void Erase(std::string &str, const std::string &chars);

		/**
		 * @brief Safe substring helper that returns a std::string.
		 *
		 * Extracts a substring starting at @p offset of length @p count. If
		 * @p offset is beyond the end of the string an empty string is returned.
		 * If @p count is std::string::npos or extends past the end, the remainder
		 * of the string is returned.
		 *
		 * @param string The source string.
		 * @param offset The starting position for the substring.
		 * @param count The maximum number of characters to include.
		 * @return A newly allocated substring.
		 */
		std::string SubStr(const std::string &string, size_t offset, size_t count = std::string::npos);

		/**
		 * @brief Return a copy of the string with leading and trailing whitespace removed.
		 *
		 * Whitespace characters are those matched by IsWhitespace defined in this
		 * header (space, tab, carriage return, newline, vertical tab, form feed).
		 *
		 * @param str The input string.
		 * @return A new std::string with leading and trailing whitespace removed.
		 */
		std::string TrimWhitespace(const std::string &str);

		/**
		 * @brief Return a copy of the string with all whitespace characters removed.
		 *
		 * Removes every character that IsWhitespace would consider whitespace,
		 * producing a new compacted string with no whitespace.
		 *
		 * @param str The input string.
		 * @return A new std::string with all whitespace removed.
		 */
		std::string RemoveWhitespace(const std::string &str);

		/**
		 * @brief Create a human readable time string representing now.
		 *
		 * When @p includeDate is false the returned string contains only the
		 * current time (hours, minutes, seconds). When true the date is prepended.
		 * If @p useDashes is true the date/time separators use '-' instead of
		 * other punctuation to produce file-system friendly strings.
		 *
		 * Examples: "14:23:05", "2026-03-24 14:23:05".
		 *
		 * @param includeDate Whether to include the date portion.
		 * @param useDashes Whether to use dashes for separators (file friendly).
		 * @return A formatted string representing the current date/time.
		 */
		std::string GetCurrentTimeString(bool includeDate = false, bool useDashes = false);

		/**
		 * @brief Compare two strings in a case-sensitive manner.
		 *
		 * Returns a negative value if @p a is lexicographically less than @p b,
		 * zero if they are equal, or a positive value if @p a is greater than @p b.
		 * The comparison is performed using the native char ordering and is
		 * case-sensitive.
		 *
		 * @param a First string to compare.
		 * @param b Second string to compare.
		 * @return Negative/zero/positive to indicate ordering of a vs b.
		 */
		int32_t CompareCase(std::string_view a, std::string_view b);
	}

	// =================================================================================================

	/**
	 * @brief Inserts a delimiter before each upper-case character in a string.
	 *
	 * Useful for converting camelCase or PascalCase identifiers into a
	 * human-readable form. When @p ifLowerCaseOnTheRight is true, the function
	 * only splits when the character to the right is lower-case (prevents
	 * breaking sequences of upper-case acronyms).
	 *
	 * @param string The input text to process.
	 * @param delimiter The delimiter to insert before upper-case letters.
	 * @param ifLowerCaseOnTheRight If true, only split when right-side char is lower-case.
	 * @return A new std::string with delimiters inserted.
	 */
	std::string SplitAtUpperCase(std::string_view string, std::string_view delimiter = " ", bool ifLowerCaseOnTheRight = true);

	/**
	 * @brief Return a compact string representation for a byte count.
	 *
	 * Produces human-friendly descriptions such as "1.2 MB", "512 KB" or
	 * "42 bytes" depending on the magnitude.
	 *
	 * @param bytes The number of bytes to describe.
	 * @return A human-readable size string.
	 */
	std::string BytesToString(uint64_t bytes);

	/**
	 * @brief Skip a UTF BOM (Byte Order Mark) from a stream if present.
	 *
	 * Reads the beginning of the stream to detect common BOM sequences (UTF-8,
	 * UTF-16 LE/BE) and adjusts the stream position to the first non-BOM byte.
	 *
	 * @param in Input stream to examine and modify the read position of.
	 * @return The number of bytes skipped (0 if no BOM was present).
	 */
	int SkipBOM(std::istream &in);

	/**
	 * @brief Read an entire text file into a string, skipping any leading BOM.
	 *
	 * If the file cannot be opened an empty string is returned. The function
	 * ensures the returned string does not contain the BOM sequence.
	 *
	 * @param filepath The path to the file to read.
	 * @return The file contents as a std::string (BOM removed if present).
	 */
	std::string ReadFileAndSkipBOM(const std::filesystem::path &filepath);

	/**
	 * @brief Extract the filename portion from a path-like string.
	 *
	 * Returns the substring after the last path separator ('/' or '\\') if
	 * present; otherwise returns the original input.
	 *
	 * @param filepath The path or filename to examine.
	 * @return A std::string_view pointing to the filename portion.
	 */
	std::string_view GetFilename(std::string_view filepath);

	/**
	 * @brief Return the file extension (without the dot) of a filename.
	 *
	 * If no extension is found an empty string is returned.
	 *
	 * @param filename The filename to examine.
	 * @return The extension without the leading '.' or empty string.
	 */
	std::string GetExtension(const std::string &filename);

	/**
	 * @brief Remove the file extension from a filename and return the base name.
	 *
	 * If no extension is found the original filename is returned.
	 *
	 * @param filename The filename to process.
	 * @return The filename without its extension.
	 */
	std::string RemoveExtension(const std::string &filename);

	#if 0 // Replaced by constexpr version
		bool StartsWith(const std::string& string, const std::string& start);
	#endif

	/**
	 * @brief Keeps delimiters except for spaces, used for shaders
	 * @param str The string to split
	 * @return A vector of strings, split by delimiters and keeping the delimiters as separate entries (except for spaces)
	 */
	std::vector<std::string> SplitStringAndKeepDelims(std::string str);

	/**
	 * @brief Split a string using any of the characters in @p delimiters as separators.
	 *
	 * Consecutive delimiter characters are treated as separators and do not
	 * produce empty tokens.
	 *
	 * @param string The input text to split.
	 * @param delimiters A view containing delimiter characters.
	 * @return A vector of token strings.
	 */
	std::vector<std::string> SplitString(std::string_view string, const std::string_view &delimiters);

	/**
	 * @brief Split a string by a single character delimiter.
	 *
	 * @param string The input text to split.
	 * @param delimiter The character to split on.
	 * @return A vector of token strings.
	 */
	std::vector<std::string> SplitString(std::string_view string, char delimiter);

	/**
	 * @brief Return true if the character is ASCII whitespace (space, tab, newline etc.).
	 *
	 * @param c Character to test.
	 * @return True if c is a whitespace character.
	 */
	inline bool IsWhitespace(const char c) { return c == ' ' || (c <= 13 && c >= 9); }

	/**
	 * @brief Return true if the character is an ASCII digit (0-9).
	 *
	 * @param c Character to test.
	 * @return True if c is a digit.
	 */
	inline bool IsDigit(const char c) { return static_cast<uint32_t>(c - '0') < 10; }

	/**
	 * @brief Replaces all occurrences of one or more substrings.
	 * The arguments must be a sequence of pairs of strings, where the first of each pair is the string to
	 * look for, followed by its replacement.
	 * @param textToSearch The text in which to perform the replacements.
	 * @param firstToReplace The first substring to look for and replace.
	 * @param firstReplacement The replacement for the first substring.
	 * @param otherPairsOfStringsToReplace Additional pairs of substrings to look for and their replacements.
	 * @return A string with the replacements made.
	 */
	template <typename StringType, typename... OtherReplacements>
	std::string Replace(StringType textToSearch, std::string_view firstToReplace, std::string_view firstReplacement, OtherReplacements&&... otherPairsOfStringsToReplace);

	/**
	 * @brief Returns a string with any whitespace trimmed from its start and end.
	 * @param textToTrim The string to trim.
	 * @return A string with whitespace removed from both ends.
	 */
	std::string Trim(std::string textToTrim);

	/**
	 * @brief Returns a string with any whitespace trimmed from its start and end.
	 * @param textToTrim The string to trim.
	 * @return A string with whitespace removed from both ends.
	 */
	std::string_view Trim(std::string_view textToTrim);

	/**
	 * @brief Returns a string with any whitespace trimmed from its start and end.
	 * @param textToTrim The string to trim.
	 * @return A string with whitespace removed from both ends.
	 */
	std::string_view Trim(const char* textToTrim);

	/**
	 * @brief Returns a string with any whitespace trimmed from its start.
	 * @param textToTrim The string to trim.
	 * @return A string with whitespace removed from the start.
	 */
	std::string TrimStart(std::string textToTrim);

	/**
	 * @brief Returns a string with any whitespace trimmed from its start.
	 * @param textToTrim The string to trim.
	 * @return A string with whitespace removed from the start.
	 */
	std::string_view TrimStart(std::string_view textToTrim);

	/**
	 * @brief Returns a string with any whitespace trimmed from its start.
	 * @param textToTrim The string to trim.
	 * @return A string with whitespace removed from the start.
	 */
	std::string_view TrimStart(const char* textToTrim);

	/**
	 * @brief Returns a string with any whitespace trimmed from its end.
	 * @param textToTrim The string to trim.
	 * @return A string with whitespace removed from the end.
	 */
	std::string TrimEnd(std::string textToTrim);

	/**
	 * @brief Returns a string with any whitespace trimmed from its end.
	 * @param textToTrim The string to trim.
	 * @return A string with whitespace removed from the end.
	 */
	std::string_view TrimEnd(std::string_view textToTrim);

	/**
	 * @brief Returns a string with any whitespace trimmed from its end.
	 * @param textToTrim The string to trim.
	 * @return A string with whitespace removed from the end.
	 */
	std::string_view TrimEnd(const char* textToTrim);

	/**
	 * @brief If the given character is at the start and end of the string, it trims it away.
	 * @param text The string to trim.
	 * @param outerChar The character to remove from the start and end.
	 * @return A string with the outer character removed from the start and end.
	 */
	std::string RemoveOuterChar(std::string text, char outerChar);

	/**
	 * @brief Remove surrounding double-quotes if present.
	 * @param text Input text.
	 * @return Text without outer double-quotes.
	 */
	inline std::string RemoveDoubleQuotes(std::string text) { return RemoveOuterChar(std::move(text), '"'); }

	/**
	 * @brief Remove surrounding single-quotes if present.
	 * @param text Input text.
	 * @return Text without outer single-quotes.
	 */
	inline std::string RemoveSingleQuotes(std::string text) { return RemoveOuterChar(std::move(text), '\''); }

	/**
	 * @brief Add double-quotes around the text.
	 * @param text Input text.
	 * @return Quoted text.
	 */
	inline std::string AddDoubleQuotes(std::string text) { return "\"" + std::move(text) + "\""; }

	/**
	 * @brief Add single-quotes around the text.
	 * @param text Input text.
	 * @return Quoted text.
	 */
	inline std::string AddSingleQuotes(std::string text) { return "'" + std::move(text) + "'"; }

	/**
	 * @brief Convert a string to lower-case and return it.
	 * @return Lower-case copy of the input string.
	 */
	std::string ToLowerCase(std::string);

	/**
	 * @brief Convert a string to upper-case and return it.
	 * @return Upper-case copy of the input string.
	 */
	std::string ToUpperCase(std::string);

	/**
	 * @brief Split a string using a predicate to detect delimiters.
	 * @tparam IsDelimiterChar Predicate type that returns true for delimiter characters.
	 * @param textToSplit The text to split.
	 * @param isDelimiterChar Predicate called for each character to determine whether it is a delimiter.
	 * @param includeDelimitersInResult When true, delimiter characters are included as separate tokens in the result.
	 * @return Vector of tokens.
	 */
	template <typename IsDelimiterChar>
	std::vector<std::string> SplitStr(std::string_view textToSplit, IsDelimiterChar&& isDelimiterChar, bool includeDelimitersInResult);

	/**
	 * @brief Split a string by multi-character delimiters defined by start/body predicates.
	 * @tparam CharStartsDelimiter Predicate detecting the start of a delimiter sequence.
	 * @tparam CharIsInDelimiterBody Predicate detecting characters that are part of a delimiter sequence.
	 * @param textToSplit Text to split.
	 * @param isDelimiterStart Predicate for the first character of a delimiter.
	 * @param isDelimiterBody Predicate for subsequent characters that remain part of the delimiter.
	 * @param includeDelimitersInResult When true, delimiter runs are included as separate tokens.
	 * @return Vector of tokens.
	 */
	template <typename CharStartsDelimiter, typename CharIsInDelimiterBody>
	std::vector<std::string> SplitStr(std::string_view textToSplit, CharStartsDelimiter&& isDelimiterStart, CharIsInDelimiterBody&& isDelimiterBody, bool includeDelimitersInResult);

	/**
	 * @brief Split a string by a single character delimiter.
	 * @param textToSplit The text to split.
	 * @param delimiterCharacter Character used as delimiter.
	 * @param includeDelimitersInResult When true, delimiter characters are included as separate tokens in the result.
	 * @return Vector of tokens.
	 */
	std::vector<std::string> SplitStr(std::string_view textToSplit, char delimiterCharacter, bool includeDelimitersInResult);

	/**
	 * @brief Split at whitespace characters, optionally keeping the delimiters.
	 * @param text The text to split.
	 * @param keepDelimiters If true whitespace tokens are included in the result.
	 * @return Vector of tokens.
	 */
	std::vector<std::string> SplitAtWhitespace(std::string_view text, bool keepDelimiters = false);

	/**
	 * @brief Split text into lines.
	 * @param text Input text to split by '\n'.
	 * @param includeNewLinesInResult If true, newline characters are kept as separate tokens.
	 * @return Vector of lines or tokens.
	 */
	std::vector<std::string> SplitIntoLines(std::string_view text, bool includeNewLinesInResult);

	/**
	 * @brief Join an array of strings using a separator.
	 * @tparam ArrayOfStrings Container type holding strings or string_views.
	 * @param strings The strings to join.
	 * @param separator The separator to place between elements.
	 * @return The joined string.
	 */
	template <typename ArrayOfStrings>
	std::string JoinStr(const ArrayOfStrings& strings, std::string_view separator);

	/**
	 * @brief Check whether text contains a substring.
	 * @param text The text to search in.
	 * @param possibleSubstring The substring to look for.
	 * @return True if found, false otherwise.
	 */
	bool Contains(std::string_view text, std::string_view possibleSubstring);

	/**
	 * @brief Check whether text starts with a specific character.
	 * @param text The text to test.
	 * @param possibleStart The character to test for at the start.
	 * @return True if text starts with possibleStart.
	 */
	bool StartsWith(std::string_view text, char possibleStart);

	/**
	 * @brief Check whether text ends with a specific character.
	 * @param text The text to test.
	 * @param possibleEnd The character to test for at the end.
	 * @return True if text ends with possibleEnd.
	 */
	bool EndsWith(std::string_view text, char possibleEnd);

	/**
	 * @brief Compute the Levenshtein edit distance between two strings.
	 * @tparam StringType Type of the string-like containers (std::string, std::string_view, etc.).
	 * @param string1 First string.
	 * @param string2 Second string.
	 * @return The Levenshtein distance (number of edits required).
	 */
	template <typename StringType>
	size_t GetLevenshteinDistance(const StringType& string1, const StringType& string2);

	/**
	 * @brief Convert a single hexadecimal character to its integer value.
	 * @param unicodeChar The character to convert (ASCII code point expected).
	 * @return Value in range 0..15, or -1 if the character is not a hex digit.
	 */
	int HexToInt(uint32_t unicodeChar);

	/**
	 * @brief Format an integer as a lower-case hexadecimal string.
	 * @tparam IntegerType An integral type.
	 * @param value The integer value to format.
	 * @param minNumDigits Minimum number of hex digits (left-padded with zeros if necessary).
	 * @return A string containing the hexadecimal representation.
	 */
	template <typename IntegerType>
	std::string CreateHexStr(IntegerType value, int minNumDigits = 0);

	/**
	 * @brief Produce a succinct description of a duration for display.
	 * @param d Duration expressed in microseconds.
	 * @return A human-readable duration description like "1.2 ms" or "3 sec".
	 */
	std::string GetDurationDescription(std::chrono::duration<double, std::micro>);

	/**
	 * @brief Produce a human-friendly description of a byte count.
	 * @param sizeInBytes Number of bytes to describe.
	 * @return A string such as "1.2 MB", "512 KB" or "42 bytes".
	 */
	std::string GetByteSizeDescription(uint64_t sizeInBytes);

	/**
	 * @brief Break down a duration into multiple components of specified types.
	 * @tparam Durations Types of the components to extract (e.g., std::chrono::minutes, std::chrono::seconds).
	 * @tparam DurationIn Type of the input duration.
	 * @param d The input duration to break down.
	 * @return A tuple containing the extracted components.
	 */
	template <class... Durations, class DurationIn>
	std::tuple<Durations...> BreakDownDuration(DurationIn d)
	{
		std::tuple<Durations...> retrieval;
		typedef int discard[];
		(void)discard{0, (void((std::get<Durations>(retrieval) =
			std::chrono::duration_cast<Durations>(d), d -= std::chrono::duration_cast<DurationIn>(std::get<Durations>(retrieval)))), 0)...};

		return retrieval;
	}

	/**
	 * @brief Convert a duration into a string using whole seconds (and fraction) where appropriate.
	 * @param duration Duration to format.
	 * @return A text representation of the duration (e.g. "3.4s").
	 */
	std::string DurationToString(std::chrono::duration<double> duration);

	/**
	 * @brief Generate a unique name by appending a numeric suffix until unused.
	 * @tparam IsAlreadyUsedFn Callable that returns true when a name is already taken.
	 * @param name Base name to use.
	 * @param isUsed Function called to check whether a candidate name is taken.
	 * @return A name guaranteed to be unused according to isUsed.
	 */
	template <typename IsAlreadyUsedFn>
	std::string AddSuffixToMakeUnique(const std::string &name, IsAlreadyUsedFn &&isUsed)
	{
		auto nameToUse = name;
		int suffix = 1;

		while (isUsed(nameToUse))
			nameToUse = name + "_" + ToString(++suffix);

		return nameToUse;
	}

	/**
	 * @brief Convert template-like names into a parenthesised type form.
	 *
	 * Examples: "Get<float>" -> "Get (Float)".
	 *
	 * @param name The input name.
	 * @return A transformed, more human friendly name.
	 */
	std::string TemplateToParenthesis(std::string_view name);

	// Useful for 'Described' types to display type name in GUI or to serialize.
	/**
	 * @brief Create a user-friendly representation of a type name.
	 *
	 * Strips namespaces and normalizes certain template decorations so types
	 * look appropriate for display in UI elements.
	 *
	 * @param name The type name to transform.
	 * @return A cleaned, user friendly type name.
	 */
	std::string CreateUserFriendlyTypeName(std::string_view name);

	// =================================================================================================

	// constexpr utilities
	/**
	 * @brief Check whether a string view starts with another string view.
	 * @param t The full text.
	 * @param s The prefix to test for.
	 * @return True when t starts with s.
	 */
	constexpr bool StartsWith(const std::string_view t, const std::string_view s)
	{
		const auto len = s.length();
		return t.length() >= len && t.substr(0, len) == s;
	}

	/**
	 * @brief Check whether a string view ends with another string view.
	 * @param t The full text.
	 * @param s The suffix to test for.
	 * @return True when t ends with s.
	 */
	constexpr bool EndsWith(const std::string_view t, const std::string_view s)
	{
		const auto len1 = t.length();
		const auto len2 = s.length();
		return len1 >= len2 && t.substr(len1 - len2) == s;
	}

	/**
	 * @brief Count tokens in a string view separated by a multi-character delimiter.
	 * @param source The source text.
	 * @param delimiter The delimiter sequence.
	 * @return Number of tokens expected when splitting by delimiter.
	 */
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

	/**
	 * @brief Split a compile-time fixed number of tokens by a delimiter.
	 */
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
		{
			tokens[N - 1] = std::string_view(&*tokenStart, pos - tokenStart);
		}

		return tokens;
	}

	/**
	 * @brief Remove C++-style namespace prefixes from a name.
	 */
	constexpr std::string_view RemoveNamespace(const std::string_view name)
	{
		if (const auto pos = name.find_last_of(':'); pos == std::string_view::npos)
			return name;

		return name.substr(name.find_last_of(':') + 1);
	}

	/**
	 * @brief Remove the outermost namespace component from a name.
	 */
	constexpr std::string_view RemoveOuterNamespace(const std::string_view name)
	{
		const auto first = name.find_first_of(':');
		if (first == std::string_view::npos)
			return name;

		if (first < name.size() - 1 && name[first + 1] == ':')
			return name.substr(first + 2);

		return name.substr(first + 1);
	}

	/**
	 * @brief Remove namespace qualifiers from a compile-time array of names.
	 */
	template <size_t N>
	constexpr std::array<std::string_view, N> RemoveNamespace(std::array<std::string_view, N> memberList)
	{
		for (std::string_view &fullName : memberList)
		{
			fullName = RemoveNamespace(fullName);
		}

		return memberList;
	}

	/**
	 * @brief Strip common in_/out_ prefixes and _Raw suffix used in codegen.
	 */
	constexpr std::string_view RemovePrefixAndSuffix(std::string_view name)
	{
		if (StartsWith(name, "in_"))
		{
			name.remove_prefix(sizeof("in_") - 1);
		}
		else if (StartsWith(name, "out_"))
		{
			name.remove_prefix(sizeof("out_") - 1);
		}

		if (EndsWith(name, "_Raw"))
		{
			name.remove_suffix(sizeof("_Raw") - 1);
		}

		return name;
	}

	// -------------------------------------------------------

	/**
	 * @brief Replace occurrences of substrings according to pairs of search/replace strings.
	 */
	template <typename StringType, typename... OtherReplacements>
	std::string Replace(StringType textToSearch, std::string_view firstToReplace, std::string_view firstReplacement, OtherReplacements &&...otherPairsOfStringsToReplace)
	{
		static_assert((sizeof...(otherPairsOfStringsToReplace) & 1u) == 0, "This function expects a list of pairs of strings as its arguments");

		if constexpr (std::is_same_v<const StringType, const std::string_view> || std::is_same_v<const StringType, const char *const>)
		{
			return replace(std::string(textToSearch), firstToReplace, firstReplacement, std::forward<OtherReplacements>(otherPairsOfStringsToReplace)...);
		}
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
		{
			return replace(replace(std::move(textToSearch), firstToReplace, firstReplacement), std::forward<OtherReplacements>(otherPairsOfStringsToReplace)...);
		}
	}

	/**
	 * @brief Returns a string with any whitespace trimmed from its start and end.
	 * @param textToTrim The string to trim.
	 * @return A string with whitespace removed from both ends.
	 */
	std::string Trim(std::string textToTrim);

	/**
	 * @brief Returns a string with any whitespace trimmed from its start and end.
	 * @param textToTrim The string to trim.
	 * @return A string with whitespace removed from both ends.
	 */
	std::string_view Trim(const std::string_view textToTrim);

	/**
	 * @brief Returns a string with any whitespace trimmed from its start and end.
	 * @param textToTrim The string to trim.
	 * @return A string with whitespace removed from both ends.
	 */
	std::string_view Trim(const char *textToTrim);

	/**
	 * @brief Returns a string with any whitespace trimmed from its start.
	 * @param textToTrim The string to trim.
	 * @return A string with whitespace removed from the start.
	 */
	std::string TrimStart(std::string textToTrim);

	/**
	 * @brief Returns a string with any whitespace trimmed from its start.
	 * @param textToTrim The string to trim.
	 * @return A string with whitespace removed from the start.
	 */
	std::string_view TrimStart(std::string_view textToTrim);

	/**
	 * @brief Returns a string with any whitespace trimmed from its start.
	 * @param textToTrim The string to trim.
	 * @return A string with whitespace removed from the start.
	 */
	std::string_view TrimStart(const char *textToTrim);

	/**
	 * @brief Returns a string with any whitespace trimmed from its end.
	 * @param textToTrim The string to trim.
	 * @return A string with whitespace removed from the end.
	 */
	std::string TrimEnd(std::string textToTrim);

	/**
	 * @brief Returns a string with any whitespace trimmed from its end.
	 * @param textToTrim The string to trim.
	 * @return A string with whitespace removed from the end.
	 */
	std::string_view TrimEnd(const std::string_view textToTrim);

	/**
	 * @brief Returns a string with any whitespace trimmed from its end.
	 * @param textToTrim The string to trim.
	 * @return A string with whitespace removed from the end.
	 */
	std::string_view TrimEnd(const char *textToTrim);

	/**
	 * @brief If the given character is at the start and end of the string, it trims it away.
	 * @param text The string to trim.
	 * @param outerChar The character to remove from the start and end.
	 * @return A string with the outer character removed from the start and end.
	 */
	std::string RemoveOuterChar(std::string text, const char outerChar);

	/**
	 * @brief Convert a string to lower-case and return it.
	 * @return Lower-case copy of the input string.
	 */
	std::string ToLowerCase(std::string s);

	/**
	 * @brief Convert a string to upper-case and return it.
	 * @return Upper-case copy of the input string.
	 */
	std::string ToUpperCase(std::string s);

	/**
	 * @brief Split a string using a predicate to detect delimiters.
	 * @tparam IsDelimiterChar Predicate type that returns true for delimiter characters.
	 * @param textToSplit The text to split.
	 * @param isDelimiterChar Predicate called for each character to determine whether it is a delimiter.
	 * @param includeDelimitersInResult When true, delimiter characters are included as separate tokens in the result.
	 * @return Vector of tokens.
	 */
	template <typename IsDelimiterChar>
	std::vector<std::string> SplitStr(std::string_view textToSplit, IsDelimiterChar &&isDelimiterChar, const bool includeDelimitersInResult)
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
		{
			tokens.emplace_back(tokenStart, pos);
		}

		return tokens;
	}

	/**
	 * @brief Split a string by multi-character delimiters defined by start/body predicates.
	 * @tparam CharStartsDelimiter Predicate detecting the start of a delimiter sequence.
	 * @tparam CharIsInDelimiterBody Predicate detecting characters that are part of a delimiter sequence.
	 * @param textToSplit Text to split.
	 * @param isDelimiterStart Predicate for the first character of a delimiter.
	 * @param isDelimiterBody Predicate for subsequent characters that remain part of the delimiter.
	 * @param includeDelimitersInResult When true, delimiter runs are included as separate tokens.
	 * @return Vector of tokens.
	 */
	template <typename CharStartsDelimiter, typename CharIsInDelimiterBody>
	std::vector<std::string> SplitStr(std::string_view textToSplit, CharStartsDelimiter &&isDelimiterStart, CharIsInDelimiterBody &&isDelimiterBody, const bool includeDelimitersInResult)
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
				{
					++pos;
				}

				if (pos != textToSplit.begin())
				{
					tokens.emplace_back(tokenStart, includeDelimitersInResult ? pos : delimiterStart);
				}

				tokenStart = pos;
			}
			else
			{
				++pos;
			}
		}

		if (pos != textToSplit.begin())
		{
			tokens.emplace_back(tokenStart, pos);
		}

		return tokens;
	}

	/**
	 * @brief Split a string by a single character delimiter.
	 * @param textToSplit The text to split.
	 * @param delimiterCharacter Character used as delimiter.
	 * @param includeDelimitersInResult When true, delimiter characters are included as separate tokens in the result.
	 * @return Vector of tokens.
	 */
	std::vector<std::string> SplitStr(const std::string_view textToSplit, const char delimiterCharacter, const bool includeDelimitersInResult);

	/**
	 * @brief Split at whitespace characters, optionally keeping the delimiters.
	 * @param text The text to split.
	 * @param keepDelimiters If true whitespace tokens are included in the result.
	 * @return Vector of tokens.
	 */
	std::vector<std::string> SplitAtWhitespace(const std::string_view text, const bool keepDelimiters);

	/**
	 * @brief Split text into lines.
	 * @param text Input text to split by '\n'.
	 * @param includeNewLinesInResult If true, newline characters are kept as separate tokens.
	 * @return Vector of lines or tokens.
	 */
	std::vector<std::string> SplitIntoLines(const std::string_view text, const bool includeNewLinesInResult);

	/**
	 * @brief Join an array of strings using a separator.
	 * @tparam ArrayOfStrings Container type holding strings or string_views.
	 * @param strings The strings to join.
	 * @param separator The separator to place between elements.
	 * @return The joined string.
	 */
	template <typename ArrayOfStrings>
	std::string JoinStr(const ArrayOfStrings &strings, const std::string_view separator)
	{
		if (strings.empty())
			return {};

		auto spaceNeeded = separator.length() * strings.size();

		for (auto &s : strings)
		{
			spaceNeeded += s.length();
		}

		std::string result(strings.front());
		result.reserve(spaceNeeded);

		for (size_t i = 1; i < strings.size(); ++i)
		{
			result += separator;
			result += strings[i];
		}

		return result;
	}

	/**
	 * @brief Check whether text contains a substring.
	 * @param text The text to search in.
	 * @param possibleSubstring The substring to look for.
	 * @return True if found, false otherwise.
	 */
	bool Contains(const std::string_view text, const std::string_view possibleSubstring);

	/**
	 * @brief Check whether text starts with a specific character.
	 * @param text The text to test.
	 * @param possibleStart The character to test for at the start.
	 * @return True if text starts with possibleStart.
	 */
	bool StartsWith(const std::string_view text, const char possibleStart);

	/**
	 * @brief 
	 * @param text 
	 * @param possibleStart 
	 * @return 
	 */
	bool StartsWithStr(const std::string_view text, const std::string_view possibleStart);

	/**
	 * @brief Check whether text ends with a specific character.
	 * @param text The text to test.
	 * @param possibleEnd The character to test for at the end.
	 * @return True if text ends with possibleEnd.
	 */
	bool EndsWith(const std::string_view text, const char possibleEnd);

	/**
	 * @brief 
	 * @param text 
	 * @param possibleEnd 
	 * @return 
	 */
	bool EndsWithStr(const std::string_view text, const std::string_view possibleEnd);

	/**
	 * @brief Compute the Levenshtein edit distance between two strings.
	 * @tparam StringType Type of the string-like containers (std::string, std::string_view, etc.).
	 * @param string1 First string.
	 * @param string2 Second string.
	 * @return The Levenshtein distance (number of edits required).
	 */
	template <typename StringType>
	size_t GetLevenshteinDistance(const StringType &string1, const StringType &string2)
	{
		if (string1.empty())
		{
			return string2.length();
		}
		if (string2.empty())
		{
			return string1.length();
		}

		auto calculate = [](size_t *costs, const size_t numCosts, const StringType &s1, const StringType &s2) -> size_t {
			for (size_t i = 0; i < numCosts; ++i)
			{
				costs[i] = i;
			}

			size_t p1 = 0;

			for (auto c1 : s1)
			{
				auto corner = p1;
				*costs = p1 + 1;
				size_t p2 = 0;

				for (auto c2 : s2)
				{
					auto upper = costs[p2 + 1];
					costs[p2 + 1] = c1 == c2 ? corner : xMath::Min({costs[p2], upper, corner}) + 1;
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

	/**
	 * @brief Convert a hex character to a number 0-15, or -1 if it's not a valid hex digit.
	 * @param unicodeChar The character to convert (ASCII code point expected).
	 * @return Value in range 0..15, or -1 if the character is not a hex digit.
	 */
	inline int HexToInt(const uint32_t unicodeChar)
	{
		const auto d1 = unicodeChar - static_cast<uint32_t>('0');
		if (d1 < 10u)
		{
			return static_cast<int>(d1);
		}
		const auto d2 = d1 + static_cast<uint32_t>('0' - 'a');
		if (d2 < 6u)
		{
			return static_cast<int>(d2 + 10);
		}
		if (const auto d3 = d2 + static_cast<uint32_t>('a' - 'A'); d3 < 6u)
		{
			return static_cast<int>(d3 + 10);
		}
		return -1;
	}

	/**
	 * @brief Format an integer as a lower-case hexadecimal string.
	 * @tparam IntegerType An integral type.
	 * @param value The integer value to format.
	 * @param minNumDigits Minimum number of hex digits (left-padded with zeros if necessary).
	 * @return A string containing the hexadecimal representation.
	 */
	template <typename IntegerType>
	std::string CreateHexStr(IntegerType value, int minNumDigits)
	{
		static_assert(std::is_integral_v<IntegerType>, "Need to pass integers into this method");
		auto intvalue = static_cast<std::make_unsigned_t<IntegerType>>(value);
		SEDX_CORE_ASSERT(minNumDigits <= 32);

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
			{
				return {d, end};
			}
		}
	}

	/**
	 * @brief Produce a succinct description of a duration for display.
	 * @param d Duration expressed in microseconds.
	 * @return A human-readable duration description like "1.2 ms" or "3 sec".
	 */
	inline std::string GetDurationDescription(const std::chrono::duration<double, std::micro> d)
	{
		const auto microseconds = std::chrono::duration_cast<std::chrono::microseconds>(d).count();

		if (microseconds < 0)
		{
			return "-" + GetDurationDescription(-d);
		}
		if (microseconds == 0)
		{
			return "0 sec";
		}

		std::string result;

		auto add_level = [&](const int64_t size, const std::string_view units, const int64_t decimalScale, const int64_t modulo) -> bool {
			if (microseconds < size)
			{
				return false;
			}

			if (!result.empty())
			{
				result += ' ';
			}

			const auto scaled = (microseconds * decimalScale + size / 2) / size;
			auto whole = scaled / decimalScale;

			if (modulo != 0)
			{
				whole = whole % modulo;
			}

			result += ToString(whole);

			if (const auto fraction = scaled % decimalScale)
			{
				result += '.';
				result += static_cast<char>('0' + (fraction / 10));

				if (fraction % 10 != 0)
					result += static_cast<char>('0' + (fraction % 10));
			}

			result +=
				whole == 1 && units.length() > 3 && units.back() == 's' ? units.substr(0, units.length() - 1) : units;
			return true;
		};

		const bool hours = add_level(60000000ll * 60ll, " hours", 1, 0);
		const bool mins = add_level(60000000ll, " min", 1, hours ? 60 : 0);

		if (hours)
		{
			return result;
		}

		if (mins)
		{
			add_level(1000000, " sec", 1, 60);
		}
		else if (!add_level(1000000, " sec", 100, 0))
		{
			if (!add_level(1000, " ms", 100, 0))
			{
				add_level(1, " microseconds", 100, 0);
			}
		}

		return result;
	}

	/**
	 * @brief Produce a human-friendly description of a byte count.
	 * @param sizeInBytes Number of bytes to describe.
	 * @return A string such as "1.2 MB", "512 KB" or "42 bytes".
	 */
	inline std::string GetByteSizeDescription(const uint64_t sizeInBytes)
	{
		auto int_to_str1_dec_place = [](const uint64_t n, const uint64_t divisor) -> std::string {
			const auto scaled = (n * 10 + divisor / 2) / divisor;
			auto result = ToString(scaled / 10);

			if (const auto fraction = scaled % 10)
			{
				result += '.';
				result += static_cast<char>('0' + fraction);
			}

			return result;
		};

		static constexpr uint64_t MAX_VALUE = 1844674407370955161ull;

		if (sizeInBytes >= 0x40000000)
		{
			return int_to_str1_dec_place(xMath::Min(MAX_VALUE, sizeInBytes), 0x40000000) + " GB";
		}
		if (sizeInBytes >= 0x100000)
		{
			return int_to_str1_dec_place(sizeInBytes, 0x100000) + " MB";
		}
		if (sizeInBytes >= 0x400)
		{
			return int_to_str1_dec_place(sizeInBytes, 0x400) + " KB";
		}
		if (sizeInBytes != 1)
		{
			return std::to_string(sizeInBytes) + " bytes";
		}

		return "1 byte";
	}

}

// -------------------------------------------------------
