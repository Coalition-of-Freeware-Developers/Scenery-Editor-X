/**
 * -------------------------------------------------------
 * Scenery Editor X
 * -------------------------------------------------------
 * Copyright (c) 2026 Thomas Ray 
 * Copyright (c) 2026 Coalition of Freeware Developers
 * -------------------------------------------------------
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 * -------------------------------------------------------
 * cfg_loader.h
 * -------------------------------------------------------
 * Created: 7/4/2025
 * -------------------------------------------------------
 */
#pragma once
#include <map>
#include <string>

// ---------------------------------------------------------

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

		static ConfigLoader* Instance();

		int Init(const std::string & cfgPath);
		void Fini();
		void Reset();
		// for debug.
		std::string ToString();

		int GetCfgByName(std::string &value, const std::string & name, int segment = Segment_COMMON);
		int GetCfgByName(int& value, const std::string & name, int segment = Segment_COMMON);
		int GetConfigByName(bool& value, const std::string & name, int segment = Segment_COMMON);
	private:
        ConfigLoader() = default;
		ConfigLoader(const ConfigLoader &) = delete;
		ConfigLoader& operator=(const ConfigLoader &) = delete;

		std::string	m_CommentChar = "#";
		std::string m_KvSplit = "=";

		std::map<std::string, std::string>	m_CommonMap;
		std::map<std::string, std::string>	m_LoggerMap;
	};

}

// ---------------------------------------------------------
