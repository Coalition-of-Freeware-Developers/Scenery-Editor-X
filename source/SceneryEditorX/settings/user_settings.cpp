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
 * user_settings.cpp
 * -------------------------------------------------------
 * Created: 11/7/2025
 * -------------------------------------------------------
 */
#include "user_settings.h"

#include <algorithm>
#include <ctime>
#include <filesystem>
#include <iomanip>
#include <ranges>
#include <sstream>
#include <SceneryEditorX/filesystem/vfs.h>
#include <SceneryEditorX/logging/logging.hpp>
#include <SceneryEditorX/project/project_settings.h>

// -------------------------------------------------------

// Platform-specific includes for regional settings
#ifdef SEDX_PLATFORM_WINDOWS
	#include <windows.h>
	#include <winnls.h>
#elif defined(SEDX_PLATFORM_LINUX)
	#include <langinfo.h>
	#include <locale.h>
#elif defined(SEDX_PLATFORM_MAC)
	#include <CoreFoundation/CoreFoundation.h>
#endif

// -------------------------------------------------------

namespace SceneryEditorX
{
	namespace
	{
		constexpr const char *LOG_TAG = "UserPreferences";
		constexpr const char *DEFAULT_FORMAT = "%d/%m/%Y %H:%M:%S";		// DD/MM/YYYY HH:MM:SS
		constexpr const char *RECENT_GROUP = "user.recent_projects";
		constexpr const char *RECENT_ENTRY = "project_";

		bool ToLocalTm(const time_t time, std::tm &out)
		{
		#ifdef SEDX_PLATFORM_WINDOWS
			return localtime_s(&out, &time) == 0;
		#else
			return localtime_r(&time, &out) != nullptr;
		#endif
		}

		bool ToUtcTm(const time_t time, std::tm &out)
		{
		#ifdef SEDX_PLATFORM_WINDOWS
			return gmtime_s(&out, &time) == 0;
		#else
			return gmtime_r(&time, &out) != nullptr;
		#endif
		}

		time_t FromUtcTm(std::tm &tm)
		{
		#ifdef SEDX_PLATFORM_WINDOWS
			return _mkgmtime(&tm);
		#else
			return timegm(&tm);
		#endif
		}

		/**
		 * @brief A Windows ("dd/MM/yyyy h:mm:ss tt") or ICU ("d/M/yy, h:mm:ss a", macOS) date/time pattern as a
		 * strftime format. Each run of one letter is one field, so "dd" never turns into a day and a stray 'd'.
		 */
		std::string PatternToStrftime(const std::string_view pattern)
		{
			std::string out;
			for (size_t i = 0; i < pattern.size();)
			{
				const char c = pattern[i];
				if (c == '\'')
				{
					// 'Literal text' ('' is a quote)
					size_t end = i + 1;
					for (; end < pattern.size(); ++end)
					{
						if (pattern[end] != '\'')
							out += pattern[end] == '%' ? std::string("%%") : std::string(1, pattern[end]);
						else if (end + 1 < pattern.size() && pattern[end + 1] == '\'')
							out += pattern[++end];
						else
							break;
					}
					i = end + 1;
					continue;
				}

				size_t run = 1;
				while (i + run < pattern.size() && pattern[i + run] == c)
					++run;
				switch (c)
				{
					case 'd':			out += run <= 2 ? "%d" : run == 3 ? "%a" : "%A"; break;		// Day of month, weekday names
					case 'E':			out += run <= 3 ? "%a" : "%A"; break;						// ICU weekday
					case 'M': case 'L':	out += run <= 2 ? "%m" : run == 3 ? "%b" : "%B"; break;		// Month, month names
					case 'y':			out += run == 2 ? "%y" : "%Y"; break;
					case 'H':			out += "%H"; break;
					case 'h':			out += "%I"; break;
					case 'm':			out += "%M"; break;
					case 's':			out += "%S"; break;
					case 't': case 'a':	out += "%p"; break;											// AM/PM (Windows, ICU)
					case 'g': case 'G':	break;														// Era
					case '%':			for (size_t k = 0; k < run; ++k) out += "%%"; break;
					default:			out.append(run, c); break;
				}
				i += run;
			}
			return out;
		}

	#ifdef SEDX_PLATFORM_WINDOWS
		/* @brief A locale string as UTF-8 (the pictures can hold literal text, e.g. "yyyy'年'M'月'd'日'"). */
		std::string GetLocaleText(const LCTYPE type)
		{
			wchar_t text[256] = {};
			const int length = GetLocaleInfoW(LOCALE_USER_DEFAULT, type, text, static_cast<int>(std::size(text)));
			if (length <= 1)
				return {};
			const int bytes = WideCharToMultiByte(CP_UTF8, 0, text, length - 1, nullptr, 0, nullptr, nullptr);
			std::string utf8(static_cast<size_t>(std::max(bytes, 0)), '\0');
			if (bytes > 0)
				WideCharToMultiByte(CP_UTF8, 0, text, length - 1, utf8.data(), bytes, nullptr, nullptr);
			return utf8;
		}
	#endif

		bool StartsWith(const std::string_view text, const std::string_view prefix)
		{
			return text.substr(0, prefix.size()) == prefix;
		}

		/* @brief The key two spellings of one project file share (case-insensitive on Windows and macOS). */
		std::string ProjectKey(const std::string &path)
		{
			return path.empty() ? std::string() : VFS::PathKey(VFS::FromUtf8(path));
		}
	}

	// ----------------------------------------------------
	/// Date/Time Formatting Implementation
	// ----------------------------------------------------

	std::string GetSystemDateTimeFormat()
	{
		try
		{
		#ifdef SEDX_PLATFORM_WINDOWS
			const std::string date = GetLocaleText(LOCALE_SSHORTDATE);
			const std::string time = GetLocaleText(LOCALE_STIMEFORMAT);
			if (!date.empty() && !time.empty())
				return PatternToStrftime(date) + " " + PatternToStrftime(time);
			SEDX_CORE_WARN_TAG(LOG_TAG, "Failed to get Windows regional format, using default");
		#elif defined(SEDX_PLATFORM_LINUX)
			// The user's LC_TIME without changing the process locale (setlocale would affect every thread)
			if (const locale_t locale = newlocale(LC_TIME_MASK, "", static_cast<locale_t>(nullptr)))
			{
				const std::string date = nl_langinfo_l(D_FMT, locale);
				const std::string time = nl_langinfo_l(T_FMT, locale);
				freelocale(locale);
				if (!date.empty() && !time.empty())
					return date + " " + time;	// Already strftime formats
			}
			SEDX_CORE_WARN_TAG(LOG_TAG, "Failed to get Linux regional format, using default");
		#elif defined(SEDX_PLATFORM_MAC)
			// Core Foundation gives an ICU pattern ("d/M/yy, h:mm:ss a")
			CFLocaleRef currentLocale = CFLocaleCopyCurrent();
			CFDateFormatterRef formatter = CFDateFormatterCreate(nullptr, currentLocale, kCFDateFormatterShortStyle, kCFDateFormatterMediumStyle);
			std::string pattern;
			if (formatter)
			{
				if (CFStringRef formatString = CFDateFormatterGetFormat(formatter))
				{
					char buffer[256];
					if (CFStringGetCString(formatString, buffer, sizeof(buffer), kCFStringEncodingUTF8))
						pattern = buffer;
				}
				CFRelease(formatter);
			}
			CFRelease(currentLocale);
			if (!pattern.empty())
				return PatternToStrftime(pattern);
			SEDX_CORE_WARN_TAG(LOG_TAG, "Failed to get macOS regional format, using default");
		#endif
		}
		catch (const std::exception& e)
		{
			SEDX_CORE_ERROR_TAG(LOG_TAG, "Exception getting system date format: {}", e.what());
		}

		return DEFAULT_FORMAT;
	}

	bool IsSystem12HourFormat()
	{
		// %p (AM/PM), %r (12-hour time) and %I (12-hour hour) only appear in 12-hour formats
		const std::string format = GetSystemDateTimeFormat();
		return format.find("%p") != std::string::npos || format.find("%P") != std::string::npos ||
			format.find("%r") != std::string::npos || format.find("%I") != std::string::npos;
	}

	std::string TimeToString(const time_t time, const bool useSystemFormat)
	{
		if (time == 0)
			return "";

		std::tm timeInfo{};
		if (!ToLocalTm(time, timeInfo))
		{
			SEDX_CORE_ERROR_TAG(LOG_TAG, "Failed to convert time_t to tm structure");
			return "";
		}

		try
		{
			std::stringstream ss;
			const std::string format = useSystemFormat ? GetSystemDateTimeFormat() : DEFAULT_FORMAT;
			ss << std::put_time(&timeInfo, format.c_str());
			return ss.str();
		}
		catch (const std::exception& e)
		{
			SEDX_CORE_ERROR_TAG(LOG_TAG, "Exception converting time to string: {}", e.what());
			return "";
		}
	}

	std::string TimeToIso8601(const time_t time)
	{
		std::tm timeInfo{};
		if (time <= 0 || !ToUtcTm(time, timeInfo))
			return "";
		char text[32];
		return std::strftime(text, sizeof(text), "%Y-%m-%dT%H:%M:%SZ", &timeInfo) > 0 ? std::string(text) : std::string();
	}

	time_t StringToTime(const std::string& timeString)
	{
		if (timeString.empty())
			return 0;

		try
		{
			// ISO 8601: "...Z" is UTC (what the editor writes), without a zone it is local time
			{
				std::tm tm{};
				std::istringstream ss(timeString);
				ss >> std::get_time(&tm, "%Y-%m-%dT%H:%M:%S");
				if (!ss.fail())
				{
					std::string rest;
					std::getline(ss, rest);
					if (rest == "Z" || rest == "z")
					{
						if (const time_t result = FromUtcTm(tm); result != -1)
							return result;
					}
					else if (rest.empty())
					{
						tm.tm_isdst = -1;
						if (const time_t result = std::mktime(&tm); result != -1)
							return result;
					}
				}
			}

			// What older versions wrote, in local time. Fixed formats only: the regional one could read a day as a month.
			static constexpr const char *LEGACY_FORMATS[] = {
				"%d/%m/%Y %H:%M:%S",	// DD/MM/YYYY HH:MM:SS (the format before ISO 8601)
				"%d:%m:%Y %H:%M:%S",	// DD:MM:YYYY HH:MM:SS (old format for backward compatibility)
				"%Y-%m-%d %H:%M:%S",	// ISO date with a space
				"%d-%m-%Y %H:%M:%S",	// DD-MM-YYYY with dashes
				"%m/%d/%Y %H:%M:%S",	// US MM/DD/YYYY, only reached when the day can't be a month
			};
			for (const char *format : LEGACY_FORMATS)
			{
				std::tm tm{};
				std::istringstream ss(timeString);
				ss >> std::get_time(&tm, format);
				if (ss.fail())
					continue;
				std::string rest;
				std::getline(ss, rest);
				if (!rest.empty())
					continue;
				tm.tm_isdst = -1;	// Let the C library work out daylight saving for that date
				if (const time_t result = std::mktime(&tm); result != -1)
					return result;
			}

			SEDX_CORE_WARN_TAG(LOG_TAG, "Failed to parse time string: '{}'", timeString);
			return 0;
		}
		catch (const std::exception& e)
		{
			SEDX_CORE_ERROR_TAG(LOG_TAG, "Exception parsing time string '{}': {}", timeString, e.what());
			return 0;
		}
	}

	// -------------------------------------------------------

	UserPreferences::UserPreferences()
	{
		Init(DEFAULT_LOCATION);
	}

	UserPreferences::UserPreferences(std::filesystem::path configPath)
	{
		Init(VFS::ToUtf8(configPath));
	}

	UserPreferences::~UserPreferences()
	{
		// Save what changed since the last save (the editor saves after each change; this catches the rest)
		if (m_Dirty && m_Writable)
			SavePreferences();
	}

	void UserPreferences::SetShowWelcomeScreen(const bool show)
	{
		if (m_ShowWelcomeScreen != show)
		{
			m_ShowWelcomeScreen = show;
			m_Dirty = true;
			SEDX_CORE_TRACE_TAG(LOG_TAG, "Welcome screen setting changed: {}", show ? "enabled" : "disabled");
		}
	}

	void UserPreferences::SetStartupProject(const std::string& projectPath)
	{
		if (m_StartupProject != projectPath)
		{
			m_StartupProject = projectPath;
			m_Dirty = true;
			SEDX_CORE_TRACE_TAG(LOG_TAG, "Startup project changed: {}", projectPath.empty() ? "none" : projectPath);
		}
	}

	void UserPreferences::AddRecentProject(const RecentProject& project)
	{
		RecentProject entry = project;
		if (entry.lastOpened <= 0)
			entry.lastOpened = std::time(nullptr);

		// Remove existing entry for this project path if it exists
		RemoveRecentProject(entry.filePath);

		// The list is keyed by time: two projects opened in the same second keep their order (the newer one first)
		while (m_RecentProjects.contains(entry.lastOpened))
			++entry.lastOpened;
		m_RecentProjects[entry.lastOpened] = entry;
		m_Dirty = true;

		// Trim list to maximum size
		TrimRecentProjects();

		SEDX_CORE_TRACE_TAG(LOG_TAG, "Added recent project: {}", entry.name);
	}

	void UserPreferences::RemoveRecentProject(const std::string& projectPath)
	{
		const std::string key = ProjectKey(projectPath);
		const auto it = std::ranges::find_if(m_RecentProjects,
			[&](const auto& pair) { return pair.second.filePath == projectPath || (!key.empty() && ProjectKey(pair.second.filePath) == key); });

		if (it != m_RecentProjects.end())
		{
			SEDX_CORE_TRACE_TAG(LOG_TAG, "Removed recent project: {}", it->second.name);
			m_RecentProjects.erase(it);
			m_Dirty = true;
		}
	}

	void UserPreferences::ClearRecentProjects()
	{
		const size_t count = m_RecentProjects.size();
		m_RecentProjects.clear();
		m_Dirty |= count > 0;
		SEDX_CORE_TRACE_TAG(LOG_TAG, "Cleared {} recent projects", count);
	}

	bool UserPreferences::LoadPreferences()
	{
		std::string error;
		bool loaded = false;
		m_Writable = true;
		m_Dirty = false;
		switch (m_File.Load(&error))
		{
			case ConfigFile::LoadResult::Loaded:
				loaded = true;
				break;

			case ConfigFile::LoadResult::Missing:
				SEDX_CORE_TRACE_TAG(LOG_TAG, "No user preferences yet: {}", m_File.GetLocation());
				break;

			case ConfigFile::LoadResult::Invalid:
			{
				// Keep what the user had before the defaults replace it
				std::string backupError;
				if (m_File.Backup(".invalid", &backupError))
				{
					SEDX_CORE_WARN_TAG(LOG_TAG, "{} - kept as {}.invalid, using the defaults", error, m_File.GetLocation());
				}
				else
				{
					SEDX_CORE_WARN_TAG(LOG_TAG, "{} - using the defaults, the file is left as it is ({})", error, backupError);
					m_Writable = false;
				}
				break;
			}

			case ConfigFile::LoadResult::Failed:
			default:
				SEDX_CORE_ERROR_TAG(LOG_TAG, "Failed to load user preferences: {}", error);
				m_Writable = false;
				break;
		}

		RemoveLegacySections();

		// Load basic preferences with defaults
		m_ShowWelcomeScreen = m_File.GetBoolOption("user.show_welcome_screen", true);
		m_StartupProject = m_File.GetStringOption("user.startup_project", "");

		// Load recent projects
		LoadRecentProjectsFromSettings();
		m_Dirty = m_Dirty || !loaded || m_File.IsDirty();	// A new file, a cleaned-up one or a trimmed list gets saved

		SEDX_CORE_TRACE_TAG(LOG_TAG, "User preferences loaded from {}: welcome screen {}, startup project {}, {} recent projects",
			VFS::ToUtf8(m_File.GetPhysicalPath()), m_ShowWelcomeScreen ? "enabled" : "disabled",
			m_StartupProject.empty() ? "none" : m_StartupProject, m_RecentProjects.size());
		return loaded;
	}

	bool UserPreferences::SavePreferences()
	{
		if (!m_Writable)
		{
			SEDX_CORE_WARN_TAG(LOG_TAG, "{} not saved: it couldn't be read, and writing would lose what's in it", m_File.GetLocation());
			return false;
		}

		/// Save basic preferences
		m_File.AddBoolOption("user.show_welcome_screen", m_ShowWelcomeScreen);
		m_File.AddStringOption("user.startup_project", m_StartupProject);

		/// Save recent projects
		SaveRecentProjectsToSettings();

		/// Write to file (only when something changed, or there is no file yet)
		if (!m_File.IsDirty() && m_File.FileExists())
		{
			m_Dirty = false;
			return true;
		}

		std::string error;
		if (!m_File.Save(&error))
		{
			SEDX_CORE_ERROR_TAG(LOG_TAG, "Failed to save user preferences: {}", error);
			return false;
		}

		m_Dirty = false;
		SEDX_CORE_TRACE_TAG(LOG_TAG, "User preferences saved to {}", VFS::ToUtf8(m_File.GetPhysicalPath()));
		return true;
	}

	void UserPreferences::Init(std::string location)
	{
		m_File.SetLocation(std::move(location));

		// First launch writes the defaults (so the file is there to find), an upgraded file its cleaned-up version
		LoadPreferences();
		if (m_Dirty && m_Writable)
			SavePreferences();
	}

	void UserPreferences::RemoveLegacySections()
	{
		// Versions up to 0.2.30 shared one document between every settings file, so this one got a copy of settings.cfg
		bool removed = false;
		for (const char *section : { "application", "x_plane", "ui", "project" })
			removed |= m_File.RemoveOption(section);
		if (removed)
			SEDX_CORE_INFO_TAG(LOG_TAG, "Removed the copy of settings.cfg an older version left in {}", m_File.GetLocation());
	}

	void UserPreferences::LoadRecentProjectsFromSettings()
	{
		m_RecentProjects.clear();

		// Entries by name, not by "count": an entry past a stale count still loads
		for (const std::string &entryName : m_File.GetChildNames(RECENT_GROUP))
		{
			if (!StartsWith(entryName, RECENT_ENTRY))
				continue;

			const std::string basePath = std::string(RECENT_GROUP) + "." + entryName;

			// Load project data
			RecentProject project;
			project.name = m_File.GetStringOption(basePath + ".name", "");
			project.filePath = m_File.GetStringOption(basePath + ".file_path", "");
			const std::string lastOpenedStr = m_File.GetStringOption(basePath + ".last_opened", "");

			// ISO 8601 (or an older text format), else the integer timestamp very old versions wrote
			project.lastOpened = StringToTime(lastOpenedStr);
			if (project.lastOpened == 0 && lastOpenedStr.empty())
				project.lastOpened = static_cast<time_t>(m_File.GetInt64Option(basePath + ".last_opened", 0));

			// Validate the project data
			if (project.name.empty() || project.filePath.empty() || project.lastOpened <= 0)
			{
				SEDX_CORE_WARN_TAG(LOG_TAG, "Skipped invalid recent project {} (name: '{}', path: '{}', time: '{}')", entryName, project.name, project.filePath, lastOpenedStr);
				continue;
			}

			// A file listed twice keeps its latest entry
			const std::string key = ProjectKey(project.filePath);
			const auto duplicate = std::ranges::find_if(m_RecentProjects, [&](const auto &pair) { return ProjectKey(pair.second.filePath) == key; });
			if (duplicate != m_RecentProjects.end())
			{
				if (duplicate->second.lastOpened >= project.lastOpened)
					continue;
				m_RecentProjects.erase(duplicate);
			}

			while (m_RecentProjects.contains(project.lastOpened))
				--project.lastOpened;
			m_RecentProjects[project.lastOpened] = project;
		}

		TrimRecentProjects();
		SEDX_CORE_TRACE_TAG(LOG_TAG, "Loaded {} valid recent projects", m_RecentProjects.size());
	}

	void UserPreferences::SaveRecentProjectsToSettings()
	{
		/// Save current recent projects, newest first, with the DD/MM/YYYY HH:MM:SS timestamps every version reads
		/// (local time). StringToTime reads ISO 8601 too, so TimeToIso8601 can take over once older builds are gone.
		int index = 0;
		for (const auto &[name, filePath, lastOpened] : m_RecentProjects | std::views::values)
		{
			const std::string basePath = std::string(RECENT_GROUP) + "." + RECENT_ENTRY + std::to_string(index);
			m_File.AddStringOption(basePath + ".name", name);
			m_File.AddStringOption(basePath + ".file_path", filePath);
			m_File.AddStringOption(basePath + ".last_opened", TimeToString(lastOpened, false));
			++index;
		}

		/// Update the count
		m_File.AddIntOption(std::string(RECENT_GROUP) + ".count", index);

		/// Entries past the end (the list got shorter) and anything else in the group go
		for (const std::string &entryName : m_File.GetChildNames(RECENT_GROUP))
		{
			if (entryName == "count")
				continue;
			int entryIndex = -1;
			if (StartsWith(entryName, RECENT_ENTRY))
			{
				const std::string_view digits = std::string_view(entryName).substr(std::char_traits<char>::length(RECENT_ENTRY));
				entryIndex = 0;
				for (const char c : digits)
					entryIndex = c >= '0' && c <= '9' && entryIndex < 100000 ? entryIndex * 10 + (c - '0') : -1;
				if (digits.empty())
					entryIndex = -1;
			}
			if (entryIndex < 0 || entryIndex >= index)
				m_File.RemoveOption(std::string(RECENT_GROUP) + "." + entryName);
		}
	}

	void UserPreferences::TrimRecentProjects()
	{
		if (m_RecentProjects.size() <= MAX_RECENT_PROJECTS)
			return;

		/// Since the map is ordered by timestamp (descending), we just need to remove the oldest entries
		auto it = m_RecentProjects.begin();
		std::advance(it, MAX_RECENT_PROJECTS);

		size_t removedCount = 0;
		while (it != m_RecentProjects.end())
		{
			it = m_RecentProjects.erase(it);
			++removedCount;
		}
		m_Dirty = true;

		SEDX_CORE_TRACE_TAG(LOG_TAG, "Trimmed {} old recent projects, keeping latest {}", removedCount, UserPreferences::MAX_RECENT_PROJECTS);
	}

	// ----------------------------------------------------
	// Convenience Functions
	// ----------------------------------------------------

	Ref<UserPreferences> CreateUserPreferences()
	{
		return CreateRef<UserPreferences>();
	}

	Ref<UserPreferences> CreateUserPreferences(const std::filesystem::path& configPath)
	{
		return CreateRef<UserPreferences>(configPath);
	}

}

/**
 * Example of how recent projects are stored in user_preferences.cfg:
 *
 * user = {
 *     show_welcome_screen = true;
 *     startup_project = "";
 *     recent_projects = {
 *         project_0 = {
 *             name = "My Project";
 *             file_path = "C:/Projects/MyProject.edx";
 *             last_opened = "24/10/2026 13:45:10";
 *         };
 *         project_1 = {
 *             name = "Another Project";
 *             file_path = "/home/me/Projects/AnotherProject.edx";
 *             last_opened = "23/10/2026 09:15:22";
 *         };
 *         count = 2;
 *     };
 * };
 */
