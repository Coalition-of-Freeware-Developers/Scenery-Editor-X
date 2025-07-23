/**
* -------------------------------------------------------
* Scenery Editor X
* -------------------------------------------------------
* Copyright (c) 2025 Thomas Ray
* Copyright (c) 2025 Coalition of Freeware Developers
* -------------------------------------------------------
* user_settings.cpp
* -------------------------------------------------------
* Created: 11/7/2025
* -------------------------------------------------------
*/
#include <algorithm>
#include <chrono>
#include <ctime>
#include <filesystem>
#include <iomanip>
#include <locale>
#include <SceneryEditorX/logging/logging.hpp>
#include <SceneryEditorX/platform/settings/settings.h>
#include <SceneryEditorX/platform/settings/user_settings.h>
#include <SceneryEditorX/project/project_settings.h>
#include <SceneryEditorX/utils/string_utils.h>
#include <sstream>

/// -------------------------------------------------------

///< Platform-specific includes for regional settings
#ifdef SEDX_PLATFORM_WINDOWS
	#include <windows.h>
	#include <winnls.h>
#elif defined(SEDX_PLATFORM_LINUX)
	#include <langinfo.h>
	#include <locale.h>
#elif defined(SEDX_PLATFORM_MAC)
	#include <CoreFoundation/CoreFoundation.h>
	#include <locale.h>
#endif

/// -------------------------------------------------------

namespace SceneryEditorX
{
	/// ----------------------------------------------------
	/// Date/Time Formatting Implementation
	/// ----------------------------------------------------
	
	std::string GetSystemDateTimeFormat()
	{
	    ///< Default format: DD/MM/YYYY HH:MM:SS
	    std::string defaultFormat = "%d/%m/%Y %H:%M:%S";
	
	    try
	    {
	    #ifdef SEDX_PLATFORM_WINDOWS
	        {
	            ///< Get Windows regional settings
	            char shortDateFormat[256] = {};
	            char timeFormat[256] = {};
	
	            ///< Get short date format
	            if (GetLocaleInfoA(LOCALE_USER_DEFAULT, LOCALE_SSHORTDATE, shortDateFormat, sizeof(shortDateFormat)) > 0)
	            {
	                ///< Get time format
	                if (GetLocaleInfoA(LOCALE_USER_DEFAULT, LOCALE_STIMEFORMAT, timeFormat, sizeof(timeFormat)) > 0)
	                {
	                    ///< Convert Windows format to strftime format
	                    std::string dateFormat(shortDateFormat);
	                    std::string fullTimeFormat(timeFormat);
	
	                    ///< Replace Windows format specifiers with strftime equivalents
                        dateFormat = Utils::replace(
                            dateFormat,
                            "dddd", "%A",  ///< Full weekday name
                            "ddd", "%a",				///< Abbreviated weekday name
                            "dd", "%d",					///< Day of month (01-31)
                            "d", "%#d",					///< Day of month (1-31) no leading zero
                            "MMMM", "%B",				///< Full month name
                            "MMM", "%b",				///< Abbreviated month name
                            "MM", "%m",					///< Month (01-12)
                            "M", "%#m",					///< Month (1-12) no leading zero
                            "yyyy", "%Y",				///< Full year (e.g., 2025)
                            "yy", "%y"					///< Two-digit year (e.g., 25)
                        );
	
	                    fullTimeFormat = Utils::replace(fullTimeFormat,
	                        "HH", "%H",    ///< Hour 24-hour format (00-23)
	                        "H", "%#H",					///< Hour 24-hour format (0-23) no leading zero
	                        "hh", "%I",					///< Hour 12-hour format (01-12)
	                        "h", "%#I",					///< Hour 12-hour format (1-12) no leading zero
	                        "mm", "%M",					///< Minutes (00-59)
	                        "m", "%#M",					///< Minutes (0-59) no leading zero
	                        "ss", "%S",					///< Seconds (00-59)
	                        "s", "%#S",					///< Seconds (0-59) no leading zero
	                        "tt", "%p",					///< AM/PM indicator
	                        "t", "%p"					///< AM/PM indicator (single char)
	                    );
	
	                    std::string systemFormat = dateFormat + " " + fullTimeFormat;
	                    SEDX_CORE_TRACE_TAG("USER_PREFS", "Using Windows regional format: {}", systemFormat);
	                    return systemFormat;
	                }
	            }
	            SEDX_CORE_WARN_TAG("USER_PREFS", "Failed to get Windows regional format, using default");
	        }
	#elif defined(SEDX_PLATFORM_LINUX)
	{
	    ///< Use locale-specific format on Linux
	    setlocale(LC_TIME, "");
	    char* dateFormat = nl_langinfo(D_FMT);
	    char* timeFormat = nl_langinfo(T_FMT);
	
	    if (dateFormat && timeFormat)
	    {
	        std::string systemFormat = std::string(dateFormat) + " " + std::string(timeFormat);
	        SEDX_CORE_TRACE_TAG("USER_PREFS", "Using Linux regional format: {}", systemFormat);
	        return systemFormat;
	    }
	    SEDX_CORE_WARN_TAG("USER_PREFS", "Failed to get Linux regional format, using default");
	}
	#elif defined(SEDX_PLATFORM_MAC)
	{
	    ///< Use Core Foundation on macOS
	    CFLocaleRef currentLocale = CFLocaleCopyCurrent();
	    CFDateFormatterRef formatter = CFDateFormatterCreate(NULL, currentLocale, kCFDateFormatterShortStyle, kCFDateFormatterMediumStyle);
	
	    if (formatter)
	    {
	        CFStringRef formatString = CFDateFormatterGetFormat(formatter);
	        if (formatString)
	        {
	            char buffer[256];
	            if (CFStringGetCString(formatString, buffer, sizeof(buffer), kCFStringEncodingUTF8))
	            {
	                std::string systemFormat(buffer);
	                SEDX_CORE_TRACE_TAG("USER_PREFS", "Using macOS regional format: {}", systemFormat);
	                CFRelease(formatter);
	                CFRelease(currentLocale);
	                return systemFormat;
	            }
	        }
	        CFRelease(formatter);
	    }
	    CFRelease(currentLocale);
	    SEDX_CORE_WARN_TAG("USER_PREFS", "Failed to get macOS regional format, using default");
	}
	#endif
	    }
	    catch (const std::exception& e)
	    {
	        SEDX_CORE_ERROR_TAG("USER_PREFS", "Exception getting system date format: {}", e.what());
	    }
	
	    SEDX_CORE_TRACE_TAG("USER_PREFS", "Using default format: {}", defaultFormat);
	    return defaultFormat;
	}
	
	bool IsSystem12HourFormat()
	{
	    try
	    {
	#ifdef SEDX_PLATFORM_WINDOWS
	        {
	            char timeFormat[256] = {};
	            if (GetLocaleInfoA(LOCALE_USER_DEFAULT, LOCALE_STIMEFORMAT, timeFormat, sizeof(timeFormat)) > 0)
	            {
	                std::string format(timeFormat);
	                ///< Check for AM/PM indicator
	                return format.find("tt") != std::string::npos || format.find('t') != std::string::npos;
	            }
	        }
	#elif defined(SEDX_PLATFORM_LINUX) || defined(SEDX_PLATFORM_MAC)
	{
	    setlocale(LC_TIME, "");
	    char* timeFormat = nl_langinfo(T_FMT);
	    if (timeFormat)
	    {
	        std::string format(timeFormat);
	        ///< Check for AM/PM indicators
	        return format.find("%p") != std::string::npos || format.find("%P") != std::string::npos;
	    }
	}
	#endif
	    }
	    catch (const std::exception& e)
	    {
	        SEDX_CORE_ERROR_TAG("USER_PREFS", "Exception checking time format: {}", e.what());
	    }
	
	    ///< Default to 24-hour format
	    return false;
	}
	
	std::string TimeToString(time_t time, const bool useSystemFormat)
	{
	    if (time == 0)
            return "";

        try
	    {
            const std::tm* timeInfo = std::localtime(&time);
            if (!timeInfo)
	        {
	            SEDX_CORE_ERROR_TAG("USER_PREFS", "Failed to convert time_t to tm structure");
	            return "";
	        }
	
	        std::stringstream ss;
	
	        if (useSystemFormat)
	        {
	            ///< Try to use system format
	            const std::string systemFormat = GetSystemDateTimeFormat();
	            ss << std::put_time(timeInfo, systemFormat.c_str());
	        }
	        else
	        {
	            ///< Use our standard DD/MM/YYYY HH:MM:SS format
	            ss << std::put_time(timeInfo, "%d/%m/%Y %H:%M:%S");
	        }
	
	        std::string result = ss.str();
	        SEDX_CORE_TRACE_TAG("USER_PREFS", "Converted time {} to string: '{}'", time, result);
	        return result;
	    }
	    catch (const std::exception& e)
	    {
	        SEDX_CORE_ERROR_TAG("USER_PREFS", "Exception converting time to string: {}", e.what());
	        return "";
	    }
	}
	
	time_t StringToTime(const std::string& timeString)
	{
	    if (timeString.empty())
            return 0;

        try
	    {
	        std::tm tm = {};
	        std::istringstream ss(timeString);
	        /// Try multiple date formats
	        std::vector<std::string> formats = {
	            "%d/%m/%Y %H:%M:%S",  /// DD/MM/YYYY HH:MM:SS (our standard format)
	            "%d:%m:%Y %H:%M:%S",  /// DD:MM:YYYY HH:MM:SS (old format for backward compatibility)
	            "%Y-%m-%d %H:%M:%S",  /// ISO format YYYY-MM-DD
	            "%m/%d/%Y %H:%M:%S",  /// US format MM/DD/YYYY
	            "%d-%m-%Y %H:%M:%S",  /// DD-MM-YYYY with dashes
	        };
	
	        /// Try system format first if available
	        if (const std::string systemFormat = GetSystemDateTimeFormat(); !systemFormat.empty())
                formats.insert(formats.begin(), systemFormat);

            for (const auto& format : formats)
	        {
	            ss.clear();
	            ss.str(timeString);
	            ss >> std::get_time(&tm, format.c_str());
	
	            if (!ss.fail())
	            {
                    if (time_t result = std::mktime(&tm); result != -1)
	                {
	                    SEDX_CORE_TRACE_TAG("USER_PREFS", "Parsed time string '{}' with format '{}' -> {}", timeString, format, result);
	                    return result;
	                }
	            }
	        }
	
	        /// If all parsing attempts failed, log warning
	        SEDX_CORE_WARN_TAG("USER_PREFS", "Failed to parse time string: '{}'", timeString);
	        return 0;
	    }
	    catch (const std::exception& e)
	    {
	        SEDX_CORE_ERROR_TAG("USER_PREFS", "Exception parsing time string '{}': {}", timeString, e.what());
	        return 0;
	    }
	}
	
	/// -------------------------------------------------------
	
	UserPreferences::UserPreferences() : m_ConfigPath("config/user_preferences.cfg")
	{
	    InitializeSettings();
	    LoadPreferences();
	}
	
	UserPreferences::UserPreferences(std::filesystem::path configPath) : m_ConfigPath(std::move(configPath))
	{
	    InitializeSettings();
	    LoadPreferences();
	}
	
	UserPreferences::~UserPreferences()
	{
	    /// Save preferences on destruction
	    SavePreferences();
	}
	
	void UserPreferences::SetShowWelcomeScreen(const bool show)
	{
	    if (m_ShowWelcomeScreen != show)
	    {
	        m_ShowWelcomeScreen = show;
	        SEDX_CORE_INFO_TAG("USER_PREFS", "Welcome screen setting changed: {}", show ? "enabled" : "disabled");
	    }
	}
	
	void UserPreferences::SetStartupProject(const std::string& projectPath)
	{
	    if (m_StartupProject != projectPath)
	    {
	        m_StartupProject = projectPath;
	        SEDX_CORE_INFO_TAG("USER_PREFS", "Startup project changed: {}", projectPath.empty() ? "none" : projectPath);
	    }
	}
	
	void UserPreferences::AddRecentProject(const RecentProject& project)
	{
	    /// Remove existing entry for this project path if it exists
	    RemoveRecentProject(project.filePath);
	
	    /// Add the new entry
	    m_RecentProjects[project.lastOpened] = project;
	
	    /// Trim list to maximum size
	    TrimRecentProjects();
	
	    SEDX_CORE_INFO_TAG("USER_PREFS", "Added recent project: {}", project.name);
	}
	
	void UserPreferences::RemoveRecentProject(const std::string& projectPath)
	{
	    const auto it = std::ranges::find_if(m_RecentProjects,
	        [&projectPath](const auto& pair) { return pair.second.filePath == projectPath; });
	
	    if (it != m_RecentProjects.end())
	    {
	        SEDX_CORE_INFO_TAG("USER_PREFS", "Removed recent project: {}", it->second.name);
	        m_RecentProjects.erase(it);
	    }
	}
	
	void UserPreferences::ClearRecentProjects()
	{
	    size_t count = m_RecentProjects.size();
	    m_RecentProjects.clear();
	    SEDX_CORE_INFO_TAG("USER_PREFS", "Cleared {} recent projects", count);
	}
	
	bool UserPreferences::LoadPreferences()
	{
	    if (!m_Settings)
	    {
	        SEDX_CORE_ERROR_TAG("USER_PREFS", "Settings not initialized");
	        return false;
	    }
	
	    try
	    {
	        SEDX_CORE_INFO_TAG("USER_PREFS", "Loading user preferences from: {}", m_ConfigPath.string());
	
	        /// Load basic preferences with defaults
	        m_ShowWelcomeScreen = m_Settings->GetBoolOption("user.show_welcome_screen", true);
	        m_StartupProject = m_Settings->GetStringOption("user.startup_project", "");
	
	        /// Load recent projects
	        LoadRecentProjectsFromSettings();
	
	        SEDX_CORE_INFO_TAG("USER_PREFS", "User preferences loaded successfully");
	        SEDX_CORE_INFO_TAG("USER_PREFS", "  Welcome screen: {}", m_ShowWelcomeScreen ? "enabled" : "disabled");
	        SEDX_CORE_INFO_TAG("USER_PREFS", "  Startup project: {}", m_StartupProject.empty() ? "none" : m_StartupProject);
	        SEDX_CORE_INFO_TAG("USER_PREFS", "  Recent projects: {}", m_RecentProjects.size());
	
	        return true;
	    }
	    catch (const std::exception& e)
	    {
	        SEDX_CORE_ERROR_TAG("USER_PREFS", "Failed to load user preferences: {}", e.what());
	        return false;
	    }
	}
	
	bool UserPreferences::SavePreferences()
	{
	    if (!m_Settings)
	    {
	        SEDX_CORE_ERROR_TAG("USER_PREFS", "Settings not initialized");
	        return false;
	    }
	
	    try
	    {
	        SEDX_CORE_INFO_TAG("USER_PREFS", "Saving user preferences to: {}", m_ConfigPath.string());
	
	        /// Save basic preferences
	        m_Settings->AddBoolOption("user.show_welcome_screen", m_ShowWelcomeScreen);
	        m_Settings->AddStringOption("user.startup_project", m_StartupProject);
	
	        /// Save recent projects
	        SaveRecentProjectsToSettings();
	
	        /// Write to file
	        m_Settings->WriteSettings();
	
	        SEDX_CORE_INFO_TAG("USER_PREFS", "User preferences saved successfully");
	        return true;
	    }
	    catch (const std::exception& e)
	    {
	        SEDX_CORE_ERROR_TAG("USER_PREFS", "Failed to save user preferences: {}", e.what());
	        return false;
	    }
	}
	
	void UserPreferences::InitializeSettings()
	{
	    try
	    {
	        /// Ensure the config directory exists
            if (const std::filesystem::path configDir = m_ConfigPath.parent_path(); !configDir.empty() && !std::filesystem::exists(configDir))
	        {
	            std::filesystem::create_directories(configDir);
	            SEDX_CORE_INFO_TAG("USER_PREFS", "Created config directory: {}", configDir.string());
	        }
	
	        /// Create the ApplicationSettings instance
	        m_Settings = CreateRef<ApplicationSettings>(m_ConfigPath);
	
	        /// Try to read existing settings, if file doesn't exist it will be created
	        if (!m_Settings->ReadSettings())
	        {
	            SEDX_CORE_INFO_TAG("USER_PREFS", "Creating new user preferences file: {}", m_ConfigPath.string());
	
	            /// Set default values
	            m_Settings->AddBoolOption("user.show_welcome_screen", true);
	            m_Settings->AddStringOption("user.startup_project", "");
	            m_Settings->AddIntOption("user.recent_projects.count", 0);
	
	            /// Write initial file
	            m_Settings->WriteSettings();
	        }
	
	        SEDX_CORE_INFO_TAG("USER_PREFS", "Settings initialized for: {}", m_ConfigPath.string());
	
	        /// Test time conversion functions (only in debug builds)
	#ifdef SEDX_DEBUG
	        {
	            time_t currentTime = std::time(nullptr);
	            std::string timeString = TimeToString(currentTime);
	            time_t convertedBack = StringToTime(timeString);
	
	            SEDX_CORE_TRACE_TAG("USER_PREFS", "Time conversion test: {} -> '{}' -> {}",
	                currentTime, timeString, convertedBack);
	
	            if (std::abs(static_cast<double>(currentTime - convertedBack)) > 1.0)
                    SEDX_CORE_WARN_TAG("USER_PREFS", "Time conversion accuracy issue detected");
            }
	#endif
	    }
	    catch (const std::exception& e)
	    {
	        SEDX_CORE_ERROR_TAG("USER_PREFS", "Failed to initialize settings: {}", e.what());
	    }
	}
	
	void UserPreferences::LoadRecentProjectsFromSettings()
	{
	    m_RecentProjects.clear();
	
	    try
	    {
	        /// Get the count of recent projects
	        int projectCount = m_Settings->GetIntOption("user.recent_projects.count", 0);
	
	        SEDX_CORE_TRACE_TAG("USER_PREFS", "Loading {} recent projects", projectCount);
	
	        for (auto i = 0; i < projectCount; ++i)
	        {
	            std::string basePath = "user.recent_projects.project_" + std::to_string(i);
	
	            /// Load project data
	            std::string name = m_Settings->GetStringOption(basePath + ".name", "");
	            std::string filePath = m_Settings->GetStringOption(basePath + ".file_path", "");
	            std::string lastOpenedStr = m_Settings->GetStringOption(basePath + ".last_opened", "");
	
	            /// Convert string back to time_t, with fallback for old integer format
	            time_t lastOpened = StringToTime(lastOpenedStr);
	            if (lastOpened == 0 && !lastOpenedStr.empty())
	            {
	                /// Fallback: Try to read as old integer format for backward compatibility
                    if (const auto oldTimestamp = static_cast<int64_t>(m_Settings->GetIntOption(basePath + ".last_opened", 0)); oldTimestamp > 0)
	                {
	                    lastOpened = static_cast<time_t>(oldTimestamp);
	                    SEDX_CORE_INFO_TAG("USER_PREFS", "Converted old timestamp format for project: {}", name);
	                }
	            }
	
	            /// Validate the project data
	            if (!name.empty() && !filePath.empty() && lastOpened > 0)
	            {
	                RecentProject project;
	                project.name = name;
	                project.filePath = filePath;
	                project.lastOpened = lastOpened;
	
	                m_RecentProjects[project.lastOpened] = project;
	
	                SEDX_CORE_TRACE_TAG("USER_PREFS", "Loaded recent project: {} at {} (opened: {})",
	                    name, filePath, lastOpenedStr);
	            }
	            else
	            {
	                SEDX_CORE_WARN_TAG("USER_PREFS", "Skipped invalid recent project at index {} (name: '{}', path: '{}', time: '{}')", i, name, filePath, lastOpenedStr);
	            }
	        }
	
	        SEDX_CORE_INFO_TAG("USER_PREFS", "Loaded {} valid recent projects", m_RecentProjects.size());
	    }
	    catch (const std::exception& e)
	    {
	        SEDX_CORE_ERROR_TAG("USER_PREFS", "Failed to load recent projects: {}", e.what());
	    }
	}
	
	void UserPreferences::SaveRecentProjectsToSettings()
	{
	    try
	    {
	        /// Clear existing recent project settings (handle both old int format and new string format)
	        const int oldCount = m_Settings->GetIntOption("user.recent_projects.count", 0);
	        for (auto i = 0; i < oldCount; ++i)
	        {
	            std::string basePath = "user.recent_projects.project_" + std::to_string(i);
	            m_Settings->RemoveOption(basePath + ".name");
	            m_Settings->RemoveOption(basePath + ".file_path");
	            m_Settings->RemoveOption(basePath + ".last_opened");
	        }
	
	        /// Save current recent projects with human-readable timestamps
	        /// Format: DD/MM/YYYY HH:MM:SS (e.g., "11/07/2025 14:30:45")
	        int index = 0;
	        for (const auto &[name, filePath, lastOpened] : m_RecentProjects | std::views::values)
	        {
	            std::string basePath = "user.recent_projects.project_" + std::to_string(index);
	
	            m_Settings->AddStringOption(basePath + ".name", name);
	            m_Settings->AddStringOption(basePath + ".file_path", filePath);
	            m_Settings->AddStringOption(basePath + ".last_opened", TimeToString(lastOpened, false));
	
	            SEDX_CORE_TRACE_TAG("USER_PREFS", "Saved recent project: {} at {} (opened: {})", name, filePath, TimeToString(lastOpened, false));
	            ++index;
	        }
	
	        /// Update the count
	        m_Settings->AddIntOption("user.recent_projects.count", static_cast<int>(m_RecentProjects.size()));
	
	        SEDX_CORE_INFO_TAG("USER_PREFS", "Saved {} recent projects to settings", m_RecentProjects.size());
	    }
	    catch (const std::exception& e)
	    {
	        SEDX_CORE_ERROR_TAG("USER_PREFS", "Failed to save recent projects: {}", e.what());
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
	
	    SEDX_CORE_INFO_TAG("USER_PREFS", "Trimmed {} old recent projects, keeping latest {}", removedCount, UserPreferences::MAX_RECENT_PROJECTS);
	}
	
	/// ----------------------------------------------------
	/// Convenience Functions
	/// ----------------------------------------------------
	
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
 * Example of how recent projects will be stored in the .cfg file:
 *
 * user = {
 *     show_welcome_screen = true;
 *     startup_project = "";
 *     recent_projects = {
 *         count = 3;
 *         project_0 = {
 *             name = "My Project";
 *             file_path = "C:/Projects/MyProject.edx";
 *             last_opened = "11/07/2025 14:30:45";
 *         };
 *         project_1 = {
 *             name = "Another Project";
 *             file_path = "C:/Projects/AnotherProject.edx";
 *             last_opened = "10/07/2025 09:15:22";
 *         };
 *         project_2 = {
 *             name = "Test Project";
 *             file_path = "C:/Projects/TestProject.edx";
 *             last_opened = "09/07/2025 16:45:10";
 *         };
 *     };
 * };
 */
