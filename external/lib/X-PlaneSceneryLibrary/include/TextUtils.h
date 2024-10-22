//Module:	TextUtils
//Author:	Connor Russell
//Date:		10/8/2024 7:40:54 PM
//Purpose:	Provide a simple functions to aid in parsing text

//Compile once
#pragma once

//Includes
#include <iostream>
#include <string>
#include <vector>

namespace TextUtils
{
	/// <summary>
	/// Reads an entire line into tokens based on delimiting chars. Delimiting char/newline are removed from stream.
	/// </summary>
	/// <param name="inString">String to read from</param>
	/// <param name="DelimitingChars">Delimiting chars (should not include \n)</param>
	/// <param name="OutTokens">Pointer to vector of strings that will hold the tokens (not cleared before adding tokens)</param>
	std::vector<std::string> TokenizeString(std::string& InString, std::vector<char> DelimitingChars);

	/// <summary>
	/// Trims whitespace from the beginning and end of a string. Does not modify the original string. Whitespace is ' ', '\t', '\n', '\r'
	/// </summary>
	/// <param name="InString">String to trim</param>
	/// <returns>Trimmed string</returns>
	std::string TrimWhitespace(const std::string& InString);

#ifdef _DEBUG
	void TestTokenizer();
#endif
};
