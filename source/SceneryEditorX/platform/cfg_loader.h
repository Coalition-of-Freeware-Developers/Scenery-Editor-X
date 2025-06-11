/**
* -------------------------------------------------------
* Scenery Editor X
* -------------------------------------------------------
* Copyright (c) 2025 Thomas Ray 
* Copyright (c) 2025 Coalition of Freeware Developers
* -------------------------------------------------------
* cfg_loader.h
* -------------------------------------------------------
* Created: 7/4/2025
* -------------------------------------------------------
*/
#pragma once
#include <map>
#include <string>

/// ---------------------------------------------------------

namespace SceneryEditorX::IO
{
	/**
	 * @brief Segment identifier for common configuration.
	 */
	#define	S_Segment_Common	"[Common]"
	
	/**
	 * @brief Segment identifier for logger configuration.
	 */
	#define S_Segment_Logger	"[Logger]"
	
	class ConfigLoader
	{
	public:
        enum Segment : uint8_t
		{
			Segment_COMMON = 0,
			Segment_LOGGER = 1
		};

		static ConfigLoader* instance();
	
		int init(const std::string & cfgPath);
		void fini();
		void reset();
		// for debug.
		std::string toString();
	
		int getCfgByName(std::string &value, const std::string & name, int segment = Segment_COMMON);
		int getCfgByName(int& value, const std::string & name, int segment = Segment_COMMON);
		int GetConfigByName(bool& value, const std::string & name, int segment = Segment_COMMON);
	private:
		ConfigLoader() = default;
		
		ConfigLoader(const ConfigLoader &) = delete;
		ConfigLoader& operator=(const ConfigLoader &) = delete;
		
		std::string	m_commentChar = "#";
		std::string m_kvSplit = "=";
	
		std::map<std::string, std::string>		m_commonMap;
		std::map<std::string, std::string>		m_loggerMap;
	};

} // namespace SceneryEditorX::IO

// ---------------------------------------------------------
