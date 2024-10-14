#pragma once

#include <map>
#include <string>

#define	S_Segment_Common	"[Common]"
#define S_Segment_Logger	"[Logger]"

class CfgLoader
{
public:
	enum
	{
		Segment_COMMON = 0,
		Segment_LOGGER = 1
	};
	static CfgLoader* instance();

	int init(std::string const& cfgPath);
	void fini();
	void reset();
	// for debug.
	std::string toString();

	int getCfgByName(std::string &value, std::string const& name, int segment = Segment_COMMON);
	int getCfgByName(int& value, std::string const& name, int segment = Segment_COMMON);
	int getCfgByName(bool& value, std::string const& name, int segment = Segment_COMMON);
private:
	CfgLoader() = default;
	
	CfgLoader(CfgLoader const &) = delete;
	CfgLoader& operator=(CfgLoader const&) = delete;
	
	std::string	m_commentChar = "#";
	std::string m_kvSplit = "=";

	std::map<std::string, std::string>		m_commonMap;
	std::map<std::string, std::string>		m_loggerMap;
};
