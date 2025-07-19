/**
* -------------------------------------------------------
* Scenery Editor X
* -------------------------------------------------------
* Copyright (c) 2025 Thomas Ray
* Copyright (c) 2025 Coalition of Freeware Developers
* -------------------------------------------------------
* encoding.cpp
* -------------------------------------------------------
* Created: 9/4/2025
* -------------------------------------------------------
*/
#include <algorithm>
#include <SceneryEditorX/core/encoding/encoding.hpp>

/// -------------------------------------------------------

namespace SceneryEditorX
{
    /**
     * @brief Base64 character set used for encoding binary data to text
     *
     * Contains A-Z, a-z, 0-9, +, / as the 64 characters
     */
    const std::string base64Chars = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

    /**
     * @brief Checks if a character is a valid Base64 character
     *
     * @param c The character to check
     * @return true if the character is part of the Base64 alphabet, false otherwise
     */
    LOCAL bool IsBase64(const unsigned char c)
    {
        return (c == 43 ||              /// +
                (c >= 47 && c <= 57) || /// /-9
                (c >= 65 && c <= 90) || /// A-Z
                (c >= 97 && c <= 122)); /// a-z
    }

    /**
     * @brief Encodes binary data to Base64 string representation
     *
     * @param input Pointer to the binary data to be encoded
     * @param length Length of the binary data in bytes
     * @return std::string containing the Base64 encoded representation
     *
     * @details The algorithm processes input data in 3-byte blocks, converting
     * each block into 4 Base64 characters. Padding with '=' is added if the input
     * length is not divisible by 3.
     */
    std::string Encoding::EncodeBase64(const unsigned char *input, size_t length)
	{
	    std::string ret;
	    int i = 0;
        unsigned char char_array_3[3];
	    unsigned char char_array_4[4];

	    while (length--)
		{
	        char_array_3[i++] = *(input++);
	        if (i == 3)
			{
	            char_array_4[0] = (char_array_3[0] & 0xfc) >> 2;
	            char_array_4[1] = ((char_array_3[0] & 0x03) << 4) + ((char_array_3[1] & 0xf0) >> 4);
	            char_array_4[2] = ((char_array_3[1] & 0x0f) << 2) + ((char_array_3[2] & 0xc0) >> 6);
	            char_array_4[3] = char_array_3[2] & 0x3f;

	            for (i = 0; (i < 4); i++)
                    ret += base64Chars[char_array_4[i]];

                i = 0;
	        }
	    }

	    if (i)
		{
            int j = 0;
            for (j = i; j < 3; j++)
                char_array_3[j] = '\0';

            char_array_4[0] = (char_array_3[0] & 0xfc) >> 2;
	        char_array_4[1] = ((char_array_3[0] & 0x03) << 4) + ((char_array_3[1] & 0xf0) >> 4);
	        char_array_4[2] = ((char_array_3[1] & 0x0f) << 2) + ((char_array_3[2] & 0xc0) >> 6);
	        char_array_4[3] = char_array_3[2] & 0x3f;

	        for (j = 0; (j < i + 1); j++)
                ret += base64Chars[char_array_4[j]];

            while ((i++ < 3))
                ret += '=';
        }

	    return ret;
	}

    /**
     * @brief Decodes a Base64 string back to its binary representation
     *
     * @param input The Base64 encoded string to decode
     * @return std::vector<uint8_t> containing the decoded binary data
     *
     * @details The function processes the input in 4-character blocks, converting
     * each block back to the original 3 bytes of binary data. The function handles
     * padding ('=') characters at the end of the input and stops processing when
     * encountered. Invalid Base64 characters are ignored.
     */
    std::vector<uint8_t> Encoding::DecodeBase64(const std::string &input)
	{
	    size_t in_len = input.size();
	    int i = 0;
        int in_ = 0;
        unsigned char char_array_3[3] = {0};
	    unsigned char char_array_4[4] = {0};

        std::vector<uint8_t> ret;

	    while (in_len-- && (input[in_] != '=') && IsBase64(input[in_]))
		{
	        char_array_4[i++] = input[in_]; in_++;
	        if (i == 4) {
	            for (i = 0; i < 4; i++)
                    char_array_4[i] = static_cast<unsigned char>(base64Chars.find(char_array_4[i]));

                char_array_3[0] = ((char_array_4[1] & 0x30) >> 4) + (char_array_4[0] << 2);
	            char_array_3[1] = ((char_array_4[1] & 0xf) << 4) + ((char_array_4[2] & 0x3c) >> 2);
	            char_array_3[2] = ((char_array_4[2] & 0x3) << 6) + char_array_4[3];

	            for (i = 0; (i < 3); i++)
                    ret.push_back(char_array_3[i]);

                i = 0;
	        }
	    }

	    if (i)
		{
            int j = 0;
            for (j = i; j < 4; j++)
	            char_array_4[j] = 0;

	        for (j = 0; j < 4; j++)
	            char_array_4[j] = static_cast<unsigned char>(base64Chars.find(char_array_4[j]));

	        char_array_3[0] = (char_array_4[0] << 2) + ((char_array_4[1] & 0x30) >> 4);
	        char_array_3[1] = ((char_array_4[1] & 0xf) << 4) + ((char_array_4[2] & 0x3c) >> 2);
	        char_array_3[2] = ((char_array_4[2] & 0x3) << 6) + char_array_4[3];

	        for (j = 0; (j < i - 1); j++)
			{
	            ret.push_back(char_array_3[j]);
	        }
	    }

	    return ret;
	}

    /**
     * @brief Hash function for generating UUIDs from a vector of integers.
     * (Currently commented out - implementation placeholder)
     *
     * @param invec Vector of uint32_t values to hash
     * @return uint32_t hash value generated from the input vector
     */
    uint32_t Encoding::HashUUID(const std::vector<uint32_t> &invec)
    {
        std::vector<uint32_t> vec = invec;
        std::ranges::sort(vec);
        std::size_t seed = vec.size();
        for (const auto &value : vec)
            seed ^= value + 0x9e3779b9 + (seed << 6) + (seed >> 2);

        return seed;
    }

}

/// -------------------------------------------------------
