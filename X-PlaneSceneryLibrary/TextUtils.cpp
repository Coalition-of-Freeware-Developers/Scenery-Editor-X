//Module:	Tokenizer
//Author:	Connor Russell
//Date:		10/8/2024 7:43:36 PM
//Purpose:	Implements TextUtils.h

//Compile once
#pragma once

//Include necessary headers
#include "TextUtils.h"

#include <sstream>
#include <algorithm>

/// <summary>
/// Reads a string from a stream until a given character is reached. Returns the character that was read, or ASCII EOF (4) if the end of the file was reached
/// </summary>
/// <param name="inStream">Stream to get the next token from</param>
/// <param name="OutString">Pointer to string object to hold the output</param>
/// <param name="Delimeter">Delimetining chars to read to, and are thrown out at start. String must be sorted beforehand so we can do a binary search. Must be a standard ASCII character, no UTF-8</param>
/// <returns>The delimiting character that ended the token</returns>
inline char ReadNextToken(std::istream& inStream, std::string* OutString, std::vector<char> Delimeters)
{
    //Define a token buffer
    std::string strTokenBuffer;

    //Define a delimiter token
    char chrDelimeter = 0;

    //Define flags
    bool bStartedToken = false; //Whether we've started the token
	bool bNextIsUtf8 = false;   //Whether the next char is a UTF-8 sequence. We have two flags because when we cle
	bool bInUtf8 = false;       //Whether we're in a UTF-8 sequence

    //Read until we can't
    while (inStream.good())
    {
        chrDelimeter = inStream.get();

        //Check if we've started token
        if (bStartedToken)
        {
			//Check if this is a delimeter, if we're not in a UTF-8 sequence
            if (!bInUtf8 && std::binary_search(Delimeters.begin(), Delimeters.end(), chrDelimeter))
            {
                *OutString = strTokenBuffer;
                return chrDelimeter;
            }

            //Otherwise, this char is good, add it to the token buffer
            strTokenBuffer += chrDelimeter;
        }

        //Otherwise check if this is not a delimeter (or is utf8 which automatically means it's not a delimeter). If that is the case, set that we've started token
        else if (bInUtf8 || !std::binary_search(Delimeters.begin(), Delimeters.end(), chrDelimeter))
        {
            //Save this char, then set token started
            strTokenBuffer += chrDelimeter;
            bStartedToken = true;
        }

        //Set whether we are in a utf-8 sequence. We can get away with not checking earlier because there will be no delimeter that matches.
        if (chrDelimeter & 0b10000000) { bInUtf8 = true; }
		else { bInUtf8 = false; }

        //Peek to set flags
        inStream.peek();
    }

    //We reached the end of the file. Return the token
    *OutString = strTokenBuffer;

	//Return -1 to indicate EOF
    return -1;
}

/// <summary>
/// Reads an entire line into tokens based on delimiting chars. Delimiting char/newline are removed from stream.
/// </summary>
/// <param name="inString">String to read from</param>
/// <param name="DelimitingChars">Delimiting chars (should not include \n)</param>
/// <param name="OutTokens">Pointer to vector of strings that will hold the tokens (not cleared before adding tokens)</param>
std::vector<std::string> TextUtils::TokenizeString(std::string& InString, std::vector<char> DelimitingChars)
{
    //Get the stream
    std::stringstream inStream(InString);

    //Buffers
    std::string strTokenBuffer;         //Current token
	std::vector<std::string> OutTokens; //Output tokens

	//Sort the delimiters
	std::sort(DelimitingChars.begin(), DelimitingChars.end());

    //Read until the end of the stream
    while (ReadNextToken(inStream, &strTokenBuffer, DelimitingChars) != -1)
    {
        if (strTokenBuffer.size() > 0) { OutTokens.push_back(strTokenBuffer); }
		strTokenBuffer.clear();
    }

    //Add the last token
	if (strTokenBuffer.size() > 0) { OutTokens.push_back(strTokenBuffer); }

    //Peek to set flags
    inStream.peek();

	return OutTokens;
}

/// <summary>
/// Trims whitespace from the beginning and end of a string. Does not modify the original string. Whitespace is ' ', '\t', '\n', '\r'
/// </summary>
/// <param name="InString">String to trim</param>
/// <returns>Trimmed string</returns>
std::string TextUtils::TrimWhitespace(const std::string& InString)
{
    //Find the start ane end
    size_t idxStart = InString.find_first_not_of(" \t\n\r");
    size_t idxEnd = InString.find_last_not_of(" \t\n\r");

    //Trim appropriately
    if (idxStart != std::string::npos && idxEnd != std::string::npos)
    {
        return InString.substr(idxStart, idxEnd - idxStart + 1);
    }
    else if (idxStart != std::string::npos)
    {
        return InString.substr(idxStart);
    }
    else if (idxEnd != std::string::npos)
    {
        return InString.substr(0, idxEnd + 1);
    }
    else
    {
        return InString;
    }
}


#ifdef _DEBUG

void TextUtils::TestTokenizer()
{
    //Test the tokenizer
    std::string testString = "This is a test,string\nhello;world";
    std::vector<char> delimiter = {',', ';', '\n', '\r'};

    //Add some fake utf-8 characters
    testString[1] = 0b10000000;
    testString[2] = ' ';	//This is actually a delimiter but we want to test that it gets ignored because it's in a UTF-8 sequence

    auto tokens = TextUtils::TokenizeString(testString, delimiter);

    for (auto s : tokens)
    {
        std::cout << s << std::endl;
    }

    //Wait
    std::cout << "Press any key to exit" << std::endl;
    std::cin.get();
}

#endif