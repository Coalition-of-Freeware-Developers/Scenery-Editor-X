//Module:	Tokenizer
//Author:	Connor Russell
//Date:		10/8/2024 7:40:54 PM
//Purpose:	Provide a simple function call to tokenize a string, or a line of a stream, into a vector of strings.

//Compile once
#pragma once

//Includes
#include <string>
#include <iostream>
#include <vector>

namespace Tokenizer
{
	/// <summary>
	/// Reads an entire line into tokens based on delimiting chars. Delimiting char/newline are removed from stream.
	/// </summary>
	/// <param name="inString">String to read from</param>
	/// <param name="DelimitingChars">Delimiting chars (should not include \n)</param>
	/// <param name="OutTokens">Pointer to vector of strings that will hold the tokens (not cleared before adding tokens)</param>
	std::vector<std::string> TokenizeString(std::string& InString, std::vector<char> DelimitingChars);

#ifdef _DEBUG
	void TestTokenizer();
#endif
};