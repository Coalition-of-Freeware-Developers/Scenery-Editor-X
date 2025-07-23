//Module:	TextUtils
//Author:	Connor Russell
//Date:		10/8/2024 7:40:54 PM
//Purpose:	Provide a simple functions to aid in parsing text
#pragma once
#include <string>
#include <vector>

namespace TextUtils
{
	/**
	 * @brief Reads an entire line into tokens based on delimiting chars. Delimiting char/newline are removed from stream.
	 *
	 * @param InString String to read from
	 * @param InStream Input stream to read from
	 * @param DelimitingChars Characters to use as delimiters. If empty, defaults to whitespace (' ', '\t', '\n', '\r')
	 * @returns Pointer to vector of strings that will hold the tokens (not cleared before adding tokens)
     */
	std::vector<std::string> TokenizeString(const std::string &InString, std::vector<char> DelimitingChars);
	
	/**
	 * @brief Trims whitespace from the beginning and end of a string. Does not modify the original string. Whitespace is ' ', '\t', '\n', '\r'
	 *
	 * @param InString String to trim
	 * @returns A new string with leading and trailing whitespace removed
     */
	std::string TrimWhitespace(const std::string &InString);

#ifdef _DEBUG
    void TestTokenizer();
#endif
}; // namespace TextUtils
