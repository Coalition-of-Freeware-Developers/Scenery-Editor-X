/**
* -------------------------------------------------------
* Scenery Editor X
* -------------------------------------------------------
* Copyright (c) 2025 Thomas Ray
* Copyright (c) 2025 Coalition of Freeware Developers
* -------------------------------------------------------
* string_utils.cpp
* -------------------------------------------------------
* Created: 20/6/2025
* -------------------------------------------------------
*/
#include <algorithm>
#include <fstream>
#include <regex>
#include <SceneryEditorX/utils/string_utils.h>
#include <utility>

/// -------------------------------------------------------

namespace SceneryEditorX::Utils
{
    namespace String
    {
		bool EqualsIgnoreCase(const std::string_view a, const std::string_view b)
		{
			if (a.size() != b.size())
				return false;

			return std::ranges::equal(a,b,[](const char a, const char b)
            {
                return std::tolower(a) == std::tolower(b);
            });
		}

		std::string& ToLower(std::string& string)
		{
			std::ranges::transform(string, string.begin(),[](const unsigned char c) { return std::tolower(c); });
			return string;
		}

		std::string ToLowerCopy(const std::string_view string)
		{
			std::string result(string);
			ToLower(result);
			return result;
		}

		std::string& ToUpper(std::string& string)
		{
            std::ranges::transform(string, string.begin(),[](const unsigned char c) { return std::toupper(c); });
			return string;
		}

		std::string ToUpperCopy(const std::string_view string)
		{
			std::string result(string);
			ToUpper(result);
			return result;
		}

		void Erase(std::string& str, const char* chars)
		{
			for (size_t i = 0; i < strlen(chars); i++)
				str.erase(std::ranges::remove(str, chars[i]).begin(), str.end());
		}

		void Erase(std::string& str, const std::string& chars)
		{
			Erase(str, chars.c_str());
		}

		std::string SubStr(const std::string& string, size_t offset, size_t count)
		{
			if (offset == std::string::npos)
				return string;

			if (offset >= string.length())
				return string;

			return string.substr(offset, count);
		}

        /// -------------------------------------------------------

		const std::string WHITESPACE = " \n\r\t\f\v";

        /// -------------------------------------------------------

		std::string TrimWhitespace(const std::string& str)
		{
			const size_t start = str.find_first_not_of(WHITESPACE);
			const std::string trimmed = (start == std::string::npos) ? "" : str.substr(start);

			const size_t end = trimmed.find_last_not_of(WHITESPACE);
			return (end == std::string::npos) ? "" : trimmed.substr(0, end + 1);
		}

		std::string RemoveWhitespace(const std::string& str)
		{
			std::string result = str;
			Erase(result, WHITESPACE);
			return result;
		}

		std::string GetCurrentTimeString(bool includeDate, bool useDashes)
		{
			const time_t currentTime = time(nullptr);
			std::stringstream timeString;
			const tm* timeBuffer = localtime(&currentTime);
			timeString << std::put_time(timeBuffer, includeDate ? "%Y:%m:%d:%T" : "%T");
			std::string str = timeString.str();

			if (useDashes)
				std::ranges::replace(str, ':', '-');

			return str;
		}

		int32_t CompareCase(std::string_view a, std::string_view b)
		{
        #ifdef SEDX_PLATFORM_WINDOWS
			return _stricmp(a.data(), b.data());
		#else
			return strcasecmp(a.data(), b.data());
		#endif
		}

	}

	std::string_view GetFilename(const std::string_view filepath)
	{
		const std::vector<std::string> parts = SplitString(filepath, "/\\");

		if (!parts.empty())
			return parts[parts.size() - 1];

		return "";
	}

	std::string GetExtension(const std::string& filename)
	{
        if (std::vector<std::string> parts = SplitString(filename, '.'); parts.size() > 1)
			return parts[parts.size() - 1];

		return "";
	}

	std::string RemoveExtension(const std::string& filename)
	{
		return filename.substr(0, filename.find_last_of('.'));
	}

	std::vector<std::string> SplitStringAndKeepDelims(std::string str)
	{
		const static std::regex re(R"((^\W|^\w+)|(\w+)|[:()])", std::regex_constants::optimize);

		std::regex_iterator rit(str.begin(), str.end(), re);
		std::regex_iterator<std::string::iterator> rend;
		std::vector<std::string> result;

		while (rit != rend)
		{
			result.emplace_back(rit->str());
			++rit;
		}
		return result;
	}

	std::vector<std::string> SplitString(const std::string_view string, const std::string_view& delimiters)
	{
		size_t first = 0;
		std::vector<std::string> result;
		while (first <= string.size())
		{
			const auto second = string.find_first_of(delimiters, first);

			if (first != second)
				result.emplace_back(string.substr(first, second - first));

			if (second == std::string_view::npos)
				break;

			first = second + 1;
		}

		return result;
	}

	std::vector<std::string> SplitString(const std::string_view string, const char delimiter)
	{
		return SplitString(string, std::string(1, delimiter));
	}

	std::string SplitAtUpperCase(std::string_view string, std::string_view delimiter, bool ifLowerCaseOnTheRight /*= true*/)
	{
		std::string str(string);
		for (int i = static_cast<int>(string.size()) - 1; i > 0; --i)
            if (const auto rightIsLower = [&] { return std::cmp_less(i, string.size()) && std::islower(str[i + 1]); }; std::isupper(str[i]) && (!ifLowerCaseOnTheRight || rightIsLower()))
                str.insert(i, delimiter);

        return str;
	}

	std::string BytesToString(uint64_t bytes)
	{
		constexpr uint64_t GB = 1024 * 1024 * 1024;
		constexpr uint64_t MB = 1024 * 1024;
		constexpr uint64_t KB = 1024;

		char buffer[32 + 1] {};

		if (bytes >= GB)
			snprintf(buffer, sizeof(buffer), "%.2f GB", (float)bytes / (float)GB);
		else if (bytes >= MB)
			snprintf(buffer, sizeof(buffer), "%.2f MB", (float)bytes / (float)MB);
		else if (bytes >= KB)
			snprintf(buffer, sizeof(buffer), "%.2f KB", (float)bytes / (float)KB);
		else
			snprintf(buffer, sizeof(buffer), "%.2f bytes", (float)bytes);

		return std::string(buffer);
	}

	std::string DurationToString(std::chrono::duration<double> duration)
	{
		const auto durations = BreakDownDuration<std::chrono::minutes, std::chrono::seconds, std::chrono::milliseconds>(duration);

		std::stringstream durSs;
		durSs << std::setfill('0') << std::setw(1) << std::get<0>(durations).count() << ':'
			<< std::setfill('0') << std::setw(2) << std::get<1>(durations).count() << '.'
			<< std::setfill('0') << std::setw(3) << std::get<2>(durations).count();
		return durSs.str();
	}

	std::string TemplateToParenthesis(std::string_view name)
	{
		std::string str(name);

		if (!Utils::contains(name, "<") || !Utils::contains(name, ">"))
			return str;

		const auto i = str.find('<');
		if (i > 1 && str[i - 1] != ' ')
			str.insert(i, " ");

		str[i + 2] = std::toupper(str[i + 2]);

		return Utils::replace(str, "<", "(", ">", ")");
	}

	std::string CreateUserFriendlyTypeName(std::string_view name)
	{
		return TemplateToParenthesis(SplitAtUpperCase(RemoveNamespace(name)));
	}

	int SkipBOM(std::istream& in)
	{
		char test[4] = {};
		in.seekg(0, std::ios::beg);
		in.read(test, 3);
		if (strcmp(test, "\xEF\xBB\xBF") == 0)
		{
			in.seekg(3, std::ios::beg);
			return 3;
		}
		in.seekg(0, std::ios::beg);
		return 0;
	}

	/// Returns an empty string when failing.
	std::string ReadFileAndSkipBOM(const std::filesystem::path& filepath)
	{
		std::string result;
		std::ifstream in(filepath, std::ios::in | std::ios::binary);
		if (in)
		{
			in.seekg(0, std::ios::end);
			auto fileSize = in.tellg();
			const int skippedChars = SkipBOM(in);

			fileSize -= skippedChars - 1;
			result.resize(fileSize);
			in.read(result.data() + 1, fileSize);
			/// Add a dummy tab to beginning of file.
			result[0] = '\t';
		}
		in.close();
		return result;
	}

}

/// -------------------------------------------------------
