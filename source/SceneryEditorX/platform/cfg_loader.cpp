/**
* -------------------------------------------------------
* Scenery Editor X
* -------------------------------------------------------
* Copyright (c) 2025 Thomas Ray 
* Copyright (c) 2025 Coalition of Freeware Developers
* -------------------------------------------------------
* cfg_loader.cpp
* -------------------------------------------------------
* Created: 7/4/2025
* -------------------------------------------------------
*/
#include <fstream>
#include <SceneryEditorX/platform/cfg_loader.h>

/// ---------------------------------------------------------

namespace SceneryEditorX::IO
{
	/**
	 * @brief Trims leading and trailing whitespace characters from a given string.
	 * 
	 * This function removes any leading and trailing spaces, tabs, newlines, and carriage returns
	 * from the input string. If the string is empty, it returns the string as is.
	 * 
	 * @param s The string to be trimmed.
	 * @return std::string& A reference to the trimmed string.
	 */
	static std::string& trim(std::string& s)
	{
		if (s.empty())
		{
			return s;
		}
		s.erase(0, s.find_first_not_of(" \t\n\r"));
		s.erase(s.find_last_not_of(" \t\n\r") + 1);
		return s;
	}
	
	/**
	 * @brief Gets the singleton instance of the CfgLoader.
	 * 
	 * This function returns a pointer to the singleton instance of the ConfigLoader class.
	 * The instance is created on the first call to this function and reused on subsequent calls.
	 * 
	 * @return CfgLoader* A pointer to the singleton instance of the CfgLoader.
	 */
	ConfigLoader *ConfigLoader::instance()
	{
		static ConfigLoader ldr;
		return &ldr;
	}
	
	/**
	 * @brief Initializes the configuration loader with the given configuration file path.
	 * 
	 * This function reads the configuration file specified by the cfgPath parameter.
	 * It parses the file line by line, ignoring empty lines and comments, and extracts
	 * key-value pairs. The key-value pairs are stored in the appropriate segment map
	 * (common or logger) based on the segment identifier found in the file.
	 * 
	 * @param cfgPath The path to the configuration file.
	 * @return int Returns 0 on success, -1 if the file could not be opened.
	 */
	int ConfigLoader::init(const std::string & cfgPath)
	{
		std::ifstream in(cfgPath.c_str(), std::ios::in | std::ios::binary);
		if (!in.is_open())
		{
			return -1;
		}
		std::string rawLine;
		std::string line;
		std::string k;
		std::string v;
		int lineNO = 0;
		int seg = 0;
		std::size_t posComment = std::string::npos;
		std::size_t posKeyValueSplit = std::string::npos;
		while (!in.eof())
		{
			lineNO++;
			getline(in, rawLine);
			line = rawLine;
			if (trim(line).empty())
			{
				continue;
			}
			posComment = line.find(m_commentChar);
			if (posComment != std::string::npos)
			{
				line = std::string(line, 0, posComment);
			}
			line = std::string(line, 0, posComment);
			if (trim(line).empty())
			{
				continue;
			}
			if (line == S_Segment_Common)
			{
				seg = Segment_COMMON;
				continue;
			}
			else if (line == S_Segment_Logger)
			{
				seg = Segment_LOGGER;
				continue;
			}
			posKeyValueSplit = line.find(m_kvSplit);
			if (posKeyValueSplit == std::string::npos)
			{
				continue;
			}
			k = std::string(line, 0, posKeyValueSplit);
			v = std::string(line, posKeyValueSplit + 1, line.length());
			trim(k);
			if (k.empty())
			{
				continue;
			}
			trim(v);
			if (seg == Segment_COMMON)
			{
				m_commonMap[k] = v;
			}
			else if (seg == Segment_LOGGER)
			{
				m_loggerMap[k] = v;
			}
		}
	
		in.close();
	
		return 0;
	}
	
	/**
	 * @brief Finalizes the configuration loader.
	 * 
	 * This function resets the configuration loader by clearing all stored configuration data.
	 * It is typically called when the configuration loader is no longer needed.
	 */
	void ConfigLoader::fini()
	{
		reset();
	}
	
	/**
	 * @brief Resets the configuration loader.
	 * 
	 * This function clears all stored configuration data from both the common and logger maps.
	 * It is typically called to reinitialize the configuration loader with new data.
	 */
	void ConfigLoader::reset()
	{
		m_commonMap.clear();
		m_loggerMap.clear();
	}
	
	/**
	 * @brief Converts the configuration data to a string representation.
	 * 
	 * This function creates a string representation of the configuration data stored
	 * in the common and logger maps. It formats the data with section headers for
	 * "Common" and "Logger" segments, and lists each key-value pair under the respective
	 * section.
	 * 
	 * @return std::string A string representation of the configuration data.
	 */
	std::string ConfigLoader::toString()
	{
		std::string s;
		s += "===== Common =====\n";
		auto it = m_commonMap.begin();
		for (; it != m_commonMap.end(); ++it)
		{
			s += it->first + " = " + it->second + "\n";
		}
		s += "===== Logger =====\n";
		it = m_loggerMap.begin();
		for (; it != m_loggerMap.end(); ++it)
		{
			s += it->first + " = " + it->second + "\n";
		}
		s += "\b";
		return s;
	}
	
	/**
	 * @brief Retrieves a configuration value by name.
	 * 
	 * This function searches for a configuration value by its name in the specified segment.
	 * If the name is found, the corresponding value is assigned to the provided reference
	 * and the function returns 0. If the name is not found, the function returns -1.
	 * 
	 * @param value A reference to a string where the retrieved value will be stored.
	 * @param name The name of the configuration key to search for.
	 * @param segment The segment to search in (Segment_COMMON or Segment_LOGGER).
	 * @return int Returns 0 if the configuration key is found, -1 otherwise.
	 */
	int ConfigLoader::getCfgByName(std::string &value, const std::string & name, int segment)
	{
		if (segment == Segment_COMMON)
		{
            if (auto it = m_commonMap.find(name.c_str()); it != m_commonMap.end())
			{
				value = it->second;
				return 0;
			}
		}
		else if (segment == Segment_LOGGER)
		{
            if (auto it = m_loggerMap.find(name.c_str()); it != m_loggerMap.end())
			{
				value = it->second;
				return 0;
			}
		}
		return -1;
	}
	
	/**
	 * @brief Retrieves a configuration value by name and converts it to an integer.
	 * 
	 * This function searches for a configuration value by its name in the specified segment.
	 * If the name is found, the corresponding value is converted to an integer and assigned
	 * to the provided reference. The function returns 0 on success. If the name is not found
	 * or the value cannot be converted to an integer, the function returns -1.
	 * 
	 * @param value A reference to an integer where the retrieved value will be stored.
	 * @param name The name of the configuration key to search for.
	 * @param segment The segment to search in (Segment_COMMON or Segment_LOGGER).
	 * @return int Returns 0 if the configuration key is found and successfully converted, -1 otherwise.
	 */
	int ConfigLoader::getCfgByName(int& value, const std::string & name, int segment)
	{
		std::string s;
		if (segment == Segment_COMMON)
		{
			auto it = m_commonMap.find(name.c_str());
			if (it != m_commonMap.end())
			{
				s = it->second;
			}
		}
		else if (segment == Segment_LOGGER)
		{
			auto it = m_loggerMap.find(name.c_str());
			if (it != m_loggerMap.end())
			{
				s = it->second;
			}
		}
		if (s.empty())
		{
			return -1;
		}
		int num = 0;
        int flag = 1;
		for (int i = 0; i < s.length(); ++i)
		{
			int c = s.at(i);
			if (c == '-' && i == 0)
			{
				flag = -1;
				continue;
			}
			c -= '0';
			if (c >= 0 && c <= 9)
			{
				num *= 10;
				num += c;
			}
			else
			{
				return -1;
			}
		}
		value = num * flag;
		return 0;
	}
	
	/**
	 * @brief Retrieves a configuration value by name and converts it to a boolean.
	 * 
	 * This function searches for a configuration value by its name in the specified segment.
	 * If the name is found, the corresponding value is converted to a boolean and assigned
	 * to the provided reference. The function returns 0 on success. If the name is not found
	 * or the value cannot be converted to a boolean, the function returns -1.
	 * 
	 * @param value A reference to a boolean where the retrieved value will be stored.
	 * @param name The name of the configuration key to search for.
	 * @param segment The segment to search in (Segment_COMMON or Segment_LOGGER).
	 * @return int Returns 0 if the configuration key is found and successfully converted, -1 otherwise.
	 */
	int ConfigLoader::GetConfigByName(bool & value, const std::string & name, int segment)
	{
		std::string s;
		if (segment == Segment_COMMON)
		{
            if (auto it = m_commonMap.find(name.c_str()); it != m_commonMap.end())
			{
				s = it->second;
				if (s == "false" || s == "FALSE" || s == "False" || s == "0")
				{
					value = false;
					return 0;
				}
			}
		}
		else if (segment == Segment_LOGGER)
		{
            if (auto it = m_loggerMap.find(name.c_str()); it != m_loggerMap.end())
			{
				s = it->second;
				if (s == "true" || s == "TRUE" || s == "True" || s == "1")
				{
					value = true;
					return 0;
				}
			}
		}
		return -1;
	}
	
} // namespace SceneryEditorX::IO

/// ---------------------------------------------------------
