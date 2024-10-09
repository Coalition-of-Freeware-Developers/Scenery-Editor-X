#include "CfgLoader.h"

#include <fstream>

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

CfgLoader * CfgLoader::instance()
{
	static CfgLoader ldr;
	return &ldr;
}

int CfgLoader::init(std::string const & cfgPath)
{
	std::ifstream in(cfgPath.c_str(), std::ios::in | std::ios::binary);
	if (!in.is_open())
	{
		return -1;
	}
	std::string rawLine = "";
	std::string line = "";
	std::string k = "";
	std::string v = "";
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

void CfgLoader::fini()
{
	reset();
	return ;
}

void CfgLoader::reset()
{
	m_commonMap.clear();
	m_loggerMap.clear();
}

std::string CfgLoader::toString()
{
	std::string s = "";
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

int CfgLoader::getCfgByName(std::string &value, std::string const & name, int segment)
{
	if (segment == Segment_COMMON)
	{
		auto it = m_commonMap.find(name.c_str());
		if (it != m_commonMap.end())
		{
			value = it->second;
			return 0;
		}
	}
	else if (segment == Segment_LOGGER)
	{
		auto it = m_loggerMap.find(name.c_str());
		if (it != m_loggerMap.end())
		{
			value = it->second;
			return 0;
		}
	}
	return -1;
}

int CfgLoader::getCfgByName(int& value, std::string const & name, int segment)
{
	std::string s = "";
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
	int c = 0;
	int flag = 1;
	for (int i = 0; i < s.length(); ++i)
	{
		c = s.at(i);
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

int CfgLoader::getCfgByName(bool & value, std::string const & name, int segment)
{
	std::string s = "";
	if (segment == Segment_COMMON)
	{
		auto it = m_commonMap.find(name.c_str());
		if (it != m_commonMap.end())
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
		auto it = m_loggerMap.find(name.c_str());
		if (it != m_loggerMap.end())
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
