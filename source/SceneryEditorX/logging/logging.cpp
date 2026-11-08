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
 * logging.cpp
 * -------------------------------------------------------
 * Created: 25/1/2025
 * -------------------------------------------------------
 */
#include "logging.hpp"

#include "SceneryEditorX/filesystem/file_system.h"

#include <minwinbase.h>
#include <sysinfoapi.h>
#include <timezoneapi.h>
#include <cerrno>
#include <cstdio>
#include <SceneryEditorX/core/application/application_data.h>
#include <SceneryEditorX/filesystem/vfs.h>
#include <spdlog/sinks/base_sink.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <vulkan/vulkan.h>
#ifdef _WIN32
	#include <Windows.h>
	#include <io.h>
	#include <process.h>
	#include <share.h>
#else
	#include <sys/file.h>
	#include <unistd.h>
#endif

// -------------------------------------------------------

namespace SceneryEditorX
{

	namespace
	{
		/* Editors running at once, each with its own numbered log files, before Log::Init() gives up on the files. */
		constexpr uint32_t MAX_LOG_INSTANCES = 64;

		/* Opens for binary appending, shared like spdlog's files (crash diagnostics appends while it's open). */
		std::FILE *OpenForAppend(const std::filesystem::path &path)
		{
		#ifdef _WIN32
			return _wfsopen(path.c_str(), L"ab", _SH_DENYNO);
		#else
			return std::fopen(path.c_str(), "ab");
		#endif
		}

		/* Empties a file opened by OpenForAppend; it is in append mode, so the next line lands at the start. */
		void Truncate(std::FILE *file)
		{
		#ifdef _WIN32
			(void)_chsize_s(_fileno(file), 0);
		#else
			(void)ftruncate(fileno(file), 0);
		#endif
		}

		/*
		 * Claims an open log file for this process, without waiting: false while another running editor holds it.
		 * The lock goes with the file handle, so the OS drops it when the process closes the file, exits or crashes,
		 * and a claimed log never stays taken. It keeps other editors out, not readers or crash diagnostics' appends:
		 * on Windows it covers one byte far past any real log size, and on Linux and macOS flock() is advisory.
		 * A file system without locks (some network shares) can't tell instances apart; the log is used unlocked there.
		 */
		bool TryClaim(std::FILE *file)
		{
		#ifdef _WIN32
			const auto handle = reinterpret_cast<HANDLE>(_get_osfhandle(_fileno(file)));
			OVERLAPPED at{};
			at.Offset = 0xFFFFFFF0u;
			at.OffsetHigh = 0x7FFFFFFFu;
			if (LockFileEx(handle, LOCKFILE_EXCLUSIVE_LOCK | LOCKFILE_FAIL_IMMEDIATELY, 0, 1, 0, &at))
				return true;
			return GetLastError() != ERROR_LOCK_VIOLATION;
		#else
			if (flock(fileno(file), LOCK_EX | LOCK_NB) == 0)
				return true;
			return errno != EWOULDBLOCK;
		#endif
		}

		/* What every file of instance n ends with: nothing for the first editor, "_2", "_3", ... for the others. */
		std::string InstanceSuffix(const uint32_t instance)
		{
			return instance <= 1 ? std::string() : std::format("_{}", instance);
		}

		struct ClaimedLog
		{
			std::FILE *file = nullptr;
			std::filesystem::path path;
			uint32_t instance = 0;
		};

		/*
		 * Opens the first SceneryEditorX.log, SceneryEditorX_2.log, ... in the folder that no other running editor
		 * holds, claims it and empties it. Several editors used to share (and truncate) one SceneryEditorX.log.
		 */
		ClaimedLog ClaimMainLog(const std::filesystem::path &folder)
		{
			for (uint32_t instance = 1; instance <= MAX_LOG_INSTANCES; ++instance)
			{
				const std::filesystem::path path = folder / ("SceneryEditorX" + InstanceSuffix(instance) + ".log");
				std::FILE *file = OpenForAppend(path);
				if (!file)
					continue;

				if (!TryClaim(file))
				{
					std::fclose(file);
					continue;
				}

				Truncate(file);
				return { file, path, instance };
			}
			throw spdlog::spdlog_ex(std::format("Failed opening a log file in {}: SceneryEditorX.log to SceneryEditorX_{}.log are all in use or not writable",
				VFS::ToUtf8(folder), MAX_LOG_INSTANCES), errno);
		}

		/* Opens one of the claimed instance's other log files, emptied for this run. */
		std::FILE *CreateLogFile(const std::filesystem::path &path)
		{
			std::FILE *file = OpenForAppend(path);
			if (!file)
				throw spdlog::spdlog_ex("Failed opening file " + VFS::ToUtf8(path) + " for writing", errno);
			Truncate(file);
			return file;
		}

		/**
		 * @brief spdlog's basic_file_sink, writing to a file opened from a std::filesystem::path.
		 *
		 * spdlog takes file names as std::string and opens them with the narrow _fsopen on Windows (the vcpkg build
		 * has no SPDLOG_WCHAR_FILENAMES), so a log folder outside the ANSI code page, such as a profile in
		 * C:\Users\日本, can't be opened (and path::string() throws before that). OpenForAppend uses the
		 * wide-character API instead; on Linux and macOS paths are UTF-8 already. The sink owns the file and closes
		 * it (letting go of a claim on it) when the last logger using it goes.
		 */
		class PathFileSink final : public spdlog::sinks::base_sink<std::mutex>
		{
		public:
			explicit PathFileSink(std::FILE *file) : m_File(file) {}

			~PathFileSink() override
			{
				if (m_File)
					std::fclose(m_File);
			}

		protected:
			void sink_it_(const spdlog::details::log_msg &msg) override
			{
				spdlog::memory_buf_t formatted;
				formatter_->format(msg, formatted);
				std::fwrite(formatted.data(), 1, formatted.size(), m_File);
			}

			void flush_() override
			{
				std::fflush(m_File);
			}

		private:
			std::FILE *m_File = nullptr;
		};

		/* The loggers Log::Init() creates; spdlog's registry knows them by these names. */
		constexpr const char *LOGGER_NAMES[] = { "Core", "SceneryEditorX", "Vulkan", "Launcher" };

		uint32_t CurrentProcessId()
		{
		#ifdef _WIN32
			return static_cast<uint32_t>(_getpid());
		#else
			return static_cast<uint32_t>(getpid());
		#endif
		}
	}

	/**
	 * @brief Static member to hold the logger instance.
	 */
	std::shared_ptr<spdlog::logger> Log::m_CoreLogger;
	std::shared_ptr<spdlog::logger> Log::m_EditorLogger;
	std::shared_ptr<spdlog::logger> Log::m_ConsoleLogger;
	std::shared_ptr<spdlog::logger> Log::m_LauncherLogger;


	/**
	 * @brief Static member to hold the enabled tags.
	 */
	std::map<std::string, Log::TagDetails> Log::m_DefaultTagDetails =
	{
		{"Animation",			    TagDetails{.enabled = true,.levelFilter = Level::Warn}},
		{"Asset Pack",			TagDetails{.enabled = true,.levelFilter = Level::Warn}},
		{"AssetManager",		    TagDetails{.enabled = true,.levelFilter = Level::Info}},
		{"LibraryManager",		TagDetails{.enabled = true,.levelFilter = Level::Info}},
		{"AssetLoader",			TagDetails{.enabled = true,.levelFilter = Level::Warn}},
		{"AssetLoaderGLTF",		TagDetails{.enabled = true,.levelFilter = Level::Warn}},
		{"AssetLoaderOBJ",		TagDetails{.enabled = true,.levelFilter = Level::Warn}},
		{"AssetLoaderFBX",		TagDetails{.enabled = true,.levelFilter = Level::Warn}},
		{"AssetSystem",			TagDetails{.enabled = true,.levelFilter = Level::Info}},
		{"Assimp",				TagDetails{.enabled = true,.levelFilter = Level::Error}},
		{"Core",					TagDetails{.enabled = true,.levelFilter = Level::Trace}},
		{"SDL",					TagDetails{.enabled = true,.levelFilter = Level::Error}},
		{"Memory",				TagDetails{.enabled = true,.levelFilter = Level::Error}},
		{"Mesh",				    TagDetails{.enabled = true,.levelFilter = Level::Warn}},
		{"Project",				TagDetails{.enabled = true,.levelFilter = Level::Info}},
		{"Renderer",				TagDetails{.enabled = true,.levelFilter = Level::Info}},
		{"Scene",					TagDetails{.enabled = true,.levelFilter = Level::Info}},
		{"Scripting",				TagDetails{.enabled = true,.levelFilter = Level::Warn}},
		{"Timer",					TagDetails{.enabled = false,.levelFilter = Level::Trace}},
	};

	static std::string LevelToString(const Log::Level lvl)
	{
		switch (lvl)
		{
			case Log::Level::Trace:	return "Trace";
			case Log::Level::Info:	return "Info";
			case Log::Level::Warn:	return "Warn";
			case Log::Level::Error:	return "Error";
			case Log::Level::Fatal:	return "Fatal";
			default:
				return "Unknown";
		}
	}

	/**
	 * @brief Initializes the logging system with console and file sinks.
	 *
	 * This method sets up the logging system to output logs to both the console and this process's log files in
	 * the log folder. Each running editor gets its own files: the first SceneryEditorX.log, EditorConsoleOut.log and
	 * Launcher.log, the second SceneryEditorX_2.log, EditorConsoleOut_2.log and Launcher_2.log, and so on (see
	 * GetLogInstance()). It configures the log patterns for each sink, sets the logging level to trace and flushes
	 * on info level entries and above.
	 */
	void Log::Init()
	{
		// A second Init() without ShutDown(): let go of the files (and the claimed instance) first
		for (const char *name : LOGGER_NAMES)
			spdlog::drop(name);
		m_CoreLogger.reset();
		m_EditorLogger.reset();
		m_ConsoleLogger.reset();
		m_LauncherLogger.reset();
		m_LogFile.clear();
		m_LogInstance = 0;

		try
		{
			// The layout's log folder (VFS AppLocation::Logs: %APPDATA%\SceneryEditorX\logs on Windows)
			std::filesystem::path appLogDir = VFS::GetAppLocation(VFS::AppLocation::Logs);

			// Try to create the directory if it doesn't exist. Ignore errors here.
			std::error_code ec;
			std::filesystem::create_directories(appLogDir, ec);

			// Every file sink is opened once and shared by the loggers writing to that file: two FILE handles on one
			// file buffer separately, so their lines come out of order, and on Windows they overwrite each other.
			const ClaimedLog mainLog = ClaimMainLog(appLogDir);
			const auto mainFileSink = std::make_shared<PathFileSink>(mainLog.file);
			m_LogFile = mainLog.path;
			m_LogInstance = mainLog.instance;

			const std::string suffix = InstanceSuffix(mainLog.instance);
			const auto consoleFileSink = std::make_shared<PathFileSink>(CreateLogFile(appLogDir / ("EditorConsoleOut" + suffix + ".log")));
			const auto launcherFileSink = std::make_shared<PathFileSink>(CreateLogFile(appLogDir / ("Launcher" + suffix + ".log")));

			const auto coreStdoutSink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
			const auto editorStdoutSink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
			const auto consoleStdoutSink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
			const auto launcherStdoutSink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();

			// -------------------------------------------------------

			// Pattern for console sinks
			coreStdoutSink->set_pattern("%^[%T] %n: %v%$");
			editorStdoutSink->set_pattern("%^[%T] %n: %v%$");
			consoleStdoutSink->set_pattern("%^%v%$");
			launcherStdoutSink->set_pattern("%^[%T] %n: %v%$");

			// -------------------------------------------------------

			// Pattern for file sinks
			mainFileSink->set_pattern("[%T] [%l] %n: %v");
			consoleFileSink->set_pattern("%^%v%$");
			launcherFileSink->set_pattern("[%T] [%l] %n: %v");

			const std::vector<spdlog::sink_ptr> coreSinks = { mainFileSink, coreStdoutSink };
			const std::vector<spdlog::sink_ptr> editorSinks = { editorStdoutSink, mainFileSink };
			const std::vector<spdlog::sink_ptr> editorConsoleSinks = { consoleStdoutSink, consoleFileSink };
			const std::vector<spdlog::sink_ptr> launcherSinks = { launcherStdoutSink, launcherFileSink };

			// -------------------------------------------------------

			m_CoreLogger = std::make_shared<spdlog::logger>("Core", coreSinks.begin(), coreSinks.end());
			m_CoreLogger->set_level(spdlog::level::trace);
			m_CoreLogger->flush_on(spdlog::level::info); // Flush on info level and above

			m_EditorLogger = std::make_shared<spdlog::logger>("SceneryEditorX", editorSinks.begin(), editorSinks.end());
			m_EditorLogger->set_level(spdlog::level::trace);
			m_EditorLogger->flush_on(spdlog::level::info); // Flush on info level and above

			m_ConsoleLogger = std::make_shared<spdlog::logger>("Vulkan", editorConsoleSinks.begin(), editorConsoleSinks.end());
			m_ConsoleLogger->set_level(spdlog::level::trace);
			m_ConsoleLogger->flush_on(spdlog::level::info); // Flush on info level and above

			m_LauncherLogger = std::make_shared<spdlog::logger>("Launcher", launcherSinks.begin(), launcherSinks.end());
			m_LauncherLogger->set_level(spdlog::level::trace);
			m_LauncherLogger->flush_on(spdlog::level::info); // Flush on info level and above

			// -------------------------------------------------------

			// Register loggers with spdlog
			spdlog::register_logger(m_CoreLogger);
			spdlog::register_logger(m_EditorLogger);
			spdlog::register_logger(m_ConsoleLogger);
			spdlog::register_logger(m_LauncherLogger);
			SetDefaultTagSettings();
			SetGlobalLevel(m_InitialLevel); // Ensure the default global filter is Info (prevents trace spam)

			m_CoreLogger->info("Log system initialized successfully, writing {}", VFS::ToUtf8(m_LogFile));
			if (m_LogInstance > 1)
				m_CoreLogger->info("Other Scenery Editor X instances are running: this one is log instance {}", m_LogInstance);
			m_CoreLogger->flush();
		}
		catch (const std::exception& ex)	// spdlog_ex, or a path the file system refused: run on without the files
		{
			m_LogFile.clear();
			m_LogInstance = 0;
			std::cerr << "Log initialization failed: " << ex.what() << '\n';
		}

	}

	void Log::SetDefaultTagSettings()
	{
		m_EnabledTags = m_DefaultTagDetails;
		// ensure default tag entry exists and defaults to Info
		m_EnabledTags[""] = TagDetails{
			.enabled = true, 
			.levelFilter = Level::Info
		};
	}

	void Log::SetGlobalLevel(const Level level)
	{
		// Map Log::Level to spdlog level
		spdlog::level::level_enum spdLevel = spdlog::level::info;
		switch (level)
		{
			case Level::Trace: spdLevel = spdlog::level::trace; break;
			case Level::Info:  spdLevel = spdlog::level::info;  break;
			case Level::Warn:  spdLevel = spdlog::level::warn;  break;
			case Level::Error: spdLevel = spdlog::level::err;   break;
			case Level::Fatal: spdLevel = spdlog::level::critical; break;
		}

		if (m_CoreLogger) m_CoreLogger->set_level(spdLevel);
		if (m_EditorLogger) m_EditorLogger->set_level(spdLevel);
		if (m_ConsoleLogger) m_ConsoleLogger->set_level(spdLevel);
		if (m_LauncherLogger) m_LauncherLogger->set_level(spdLevel);

		// If enabling trace, lower tag filters so trace messages appear for all tags.
		if (level == Level::Trace)
		{
			// Lower existing tag filters to Trace so verbose messages can pass tag checks.
			for (auto &kv : m_EnabledTags)
			{
				kv.second.levelFilter = Level::Trace;
			}
			m_EnabledTags[""] = TagDetails{
				.enabled = true, 
				.levelFilter = Level::Trace
			};
			if (m_CoreLogger)
			{
				m_CoreLogger->info("============================================");
				m_CoreLogger->info("Verbose logging enabled (Trace)");
				m_CoreLogger->info("============================================");
			}
		}
		else
		{
			// Restore default per-tag filters from DefaultTagDetails_.
			m_EnabledTags = m_DefaultTagDetails;
			// ensure default tag entry exists with the requested base level
			m_EnabledTags[""] = TagDetails{
				.enabled = true, 
				.levelFilter = level
			};
			if (m_CoreLogger)
			{
				m_CoreLogger->info("Log level set to {}", LevelToString(level));
			}
		}
	}

	spdlog::level::level_enum Log::GetGlobalLogLevel()
	{
		// Prefer Core logger as the canonical global logger. Fall back to other loggers.
		if (m_CoreLogger)
			return m_CoreLogger->level();
		/*
		if (m_EditorLogger)
			return m_EditorLogger->level();
		if (m_ConsoleLogger)
			return m_ConsoleLogger->level();
		if (m_LauncherLogger)
			return m_LauncherLogger->level();
			*/
	
		// No loggers yet — return sensible default.
		return spdlog::level::info;
	}

	Log::Level Log::GetTagLevel(const std::string &tag)
	{
		// Look for explicit tag entry
		if (auto it = m_EnabledTags.find(tag); it != m_EnabledTags.end())
		{
			return it->second.levelFilter;
		}

		// Fallback to default/global tag entry (empty string)
		if (auto it = m_EnabledTags.find(""); it != m_EnabledTags.end())
		{
			return it->second.levelFilter;
		}

		// As a last resort return Info
		return Level::Info;
	}

	void Log::LogVulkanDebug(const std::string &message)
	{
		if (m_CoreLogger)
		{
			// Parse severity from the formatted message
			if (message.find("[ERROR]") != std::string::npos)
				m_CoreLogger->error(message);
			else if (message.find("[WARNING]") != std::string::npos)
				m_CoreLogger->warn(message);
			else if (message.find("[INFO]") != std::string::npos)
				m_CoreLogger->info(message);
			else if (message.find("[VERBOSE]") != std::string::npos)
				m_CoreLogger->debug(message);
			else if (message.find("error") != std::string::npos || message.find("ERROR") != std::string::npos)
				m_CoreLogger->error(message);
			else if (message.find("warning") != std::string::npos || message.find("WARNING") != std::string::npos)
				m_CoreLogger->warn(message);
			else if (message.find("performance") != std::string::npos || message.find("PERFORMANCE") != std::string::npos)
				m_CoreLogger->warn("PERFORMANCE: {}", message);
			else
				m_CoreLogger->trace(message);

			// Always flush to ensure messages are written immediately
			m_CoreLogger->flush();
		}
	}

	bool Log::ReportAssertion(const char* expr, const char* file, int line, const std::string& message) {

		std::string logMsg = std::format("Assertion Failed: {}\nExpression: {}\nFile: {}\nLine: {}", message, expr, file, line);

		if (m_CoreLogger)
		{
			m_CoreLogger->critical(logMsg);
			m_CoreLogger->flush(); // Force the logger to write to the file/console immediately
		}

		// 3. UI/User Decision (The "Ignore" logic)
		// For a Windows-based app use a Message Box:
		#ifdef SEDX_PLATFORM_WINDOWS
			int result = MessageBoxA(NULL, logMsg.c_str(), "Assertion Failed", 
									 MB_ABORTRETRYIGNORE | MB_ICONERROR | MB_TASKMODAL);
			
			if (result == IDIGNORE)
			{
				return false; // User clicked "Ignore" - continue execution
			}
			if (result == IDABORT)
			{
				exit(1); // Exit completely
			}
			return true; // IDRETRY - this triggers the __debugbreak()
		#elif defined(SEDX_PLATFORM_LINUX)
			return true; // Default to break on Linux (no standard message box, and typically running in a terminal where user can see the log)
		#else
			return true; // Default to break for unknown platforms
		#endif
	}

	/*
	void Log::LogVulkanResult(VkResult result, const std::string &operation)
	{
		if (CoreLogger)
		{
			if (result != VK_SUCCESS)
			{
				const char *resultString = vkErrorString(result);
				std::string message = "Vulkan operation '" + operation + "' returned " + resultString;

				if (result < 0)
				{ // Negative values are errors
					CoreLogger->error("{}", message);
				}
				else
				{ // Non-zero positive values are warnings/info
					CoreLogger->warn("{}", message);
				}

				CoreLogger->flush();
			}
			else
			{
				// Optionally log successful operations at trace level
				CoreLogger->trace("Vulkan operation '{}' completed successfully", operation);
			}
		}
	}
	*/

	/**
	 * @brief Get the name of the operating system.
	 * @return The name of the operating system as a string.
	 */
	// TODO: This needs to be refactored to a more modern approach to detect OS and architecture.
	[[nodiscard]] static constexpr std::string GetOsName()
	{
	#ifdef _WIN32
		return "Windows 32-bit";
	#elif _WIN64
		return "Windows 64-bit";
	#elif __APPLE__ || __MACH__
		return "Mac OSX";
	#elif __linux__
		return "Linux";
	#elif __unix || __unix__
		return "Unix";
	#else
		return "Other";
	#endif
	}

	// -------------------------------------------------------

	void Log::LogHeader()
	{
		AppData stats;
		// -------------------------------------------------------
		// TODO: Refactor this code to use enum case values for the different processor architectures. (Example: x86, x64, ARM/ AMD, Intel i9)
		SYSTEM_INFO sysInfo;
		GetSystemInfo(&sysInfo);

		// -------------------------------------------------------
		SYSTEMTIME systemTime;
		GetSystemTime(&systemTime);

		// -------------------------------------------------------

		// TODO: Add enum case values for the different time zones to return. (Example: EST,GMT,DST)
		TIME_ZONE_INFORMATION timeZoneInfo;
		GetTimeZoneInformation(&timeZoneInfo);
		std::wstring timeZoneNameWide = timeZoneInfo.StandardName[0] != L'\0' ? timeZoneInfo.StandardName : timeZoneInfo.DaylightName;

		// Convert wide string to narrow string using Windows API
		std::string timeZoneName;
		if (!timeZoneNameWide.empty())
		{
			if (int size = WideCharToMultiByte(CP_UTF8, 0, timeZoneNameWide.c_str(), -1, nullptr, 0, nullptr, nullptr); size > 0)
			{
				timeZoneName.resize(size - 1); // -1 to exclude null terminator
				WideCharToMultiByte(CP_UTF8, 0, timeZoneNameWide.c_str(), -1, timeZoneName.data(), size, nullptr, nullptr);
			}
		}
		// -------------------------------------------------------

		SEDX_CORE_INFO("============================================");
		SEDX_CORE_INFO("System Information");
		SEDX_CORE_INFO("Operating System: {}", GetOsName());
		SEDX_CORE_INFO("{:02}:{:02}:{:02} {:02}/{:02}/{:04}",
					 systemTime.wHour,
					 systemTime.wMinute,
					 systemTime.wSecond,
					 systemTime.wDay,
					 systemTime.wMonth,
					 systemTime.wYear);
		SEDX_CORE_INFO("Time Zone: {}", timeZoneName);
		SEDX_CORE_INFO("Processor Architecture: {}", sysInfo.wProcessorArchitecture);
		SEDX_CORE_INFO("Processor Cores: {}", sysInfo.dwNumberOfProcessors);
		SEDX_CORE_INFO("Page Size: {}", sysInfo.dwPageSize);
		SEDX_CORE_INFO("Processor Type: {}", sysInfo.dwProcessorType);
		SEDX_CORE_INFO("Minimum EditorApplication Address: {}", sysInfo.lpMinimumApplicationAddress);
		SEDX_CORE_INFO("Maximum EditorApplication Address: {}", sysInfo.lpMaximumApplicationAddress);
		SEDX_CORE_INFO("Active Processor Mask: {}", sysInfo.dwActiveProcessorMask);
		SEDX_CORE_INFO("============================================");
		SEDX_CORE_INFO("============================================");
		SEDX_CORE_INFO("Scenery Editor X");
		SEDX_CORE_INFO("Version: {}", SEDX_VERSION_STRING);
		SEDX_CORE_INFO("Build Date: {}", __DATE__);
		SEDX_CORE_INFO("Build Time: {}", __TIME__);
		SEDX_CORE_INFO("Coalition of Freeware Developers");
		SEDX_CORE_INFO("Copyright (C) 2026");
		SEDX_CORE_INFO("============================================");
		SEDX_CORE_INFO("Process ID: {}", CurrentProcessId());
		SEDX_CORE_INFO("Log instance: {} ({})", m_LogInstance, VFS::ToUtf8(m_LogFile.filename()));
		SEDX_CORE_INFO("============================================");
	}

	void Log::ShutDown()
	{
		if (m_CoreLogger)
		{
			m_CoreLogger->flush();
			spdlog::drop(m_CoreLogger->name()); // Explicitly drop by name
			m_CoreLogger.reset();
		}

		if (m_EditorLogger)
		{
			m_EditorLogger->flush();
			spdlog::drop(m_EditorLogger->name()); // Explicitly drop by name
			m_EditorLogger.reset();
		}

		if (m_ConsoleLogger)
		{
			m_ConsoleLogger->flush();
			spdlog::drop(m_ConsoleLogger->name()); // Explicitly drop by name
			m_ConsoleLogger.reset();
		}

		if (m_LauncherLogger)
		{
			m_LauncherLogger->flush();
			spdlog::drop(m_LauncherLogger->name()); // Explicitly drop by name
			m_LauncherLogger.reset();
		}

		spdlog::drop_all(); // Drop all loggers
		spdlog::shutdown();
	}

	// -------------------------------------------------------

	// ReSharper disable once CommentTypo
	// taken from Sam Lantiga: https://www.libsdl.org/tmp/SDL/test/testvulkan.c
	[[maybe_unused]] const char *Log::VkErrorString(const VkResult result)
	{
		switch (static_cast<int>(result))
		{
		case VK_SUCCESS:						return "VK_SUCCESS";
		case VK_NOT_READY:						return "VK_NOT_READY";
		case VK_TIMEOUT:						return "VK_TIMEOUT";
		case VK_EVENT_SET:						return "VK_EVENT_SET";
		case VK_EVENT_RESET:					return "VK_EVENT_RESET";
		case VK_INCOMPLETE:						return "VK_INCOMPLETE";
		case VK_ERROR_OUT_OF_HOST_MEMORY:		return "VK_ERROR_OUT_OF_HOST_MEMORY";
		case VK_ERROR_OUT_OF_DEVICE_MEMORY:		return "VK_ERROR_OUT_OF_DEVICE_MEMORY";
		case VK_ERROR_INITIALIZATION_FAILED:	return "VK_ERROR_INITIALIZATION_FAILED";
		case VK_ERROR_DEVICE_LOST:				return "VK_ERROR_DEVICE_LOST";
		case VK_ERROR_MEMORY_MAP_FAILED:		return "VK_ERROR_MEMORY_MAP_FAILED";
		case VK_ERROR_LAYER_NOT_PRESENT:		return "VK_ERROR_LAYER_NOT_PRESENT";
		case VK_ERROR_EXTENSION_NOT_PRESENT:	return "VK_ERROR_EXTENSION_NOT_PRESENT";
		case VK_ERROR_FEATURE_NOT_PRESENT:		return "VK_ERROR_FEATURE_NOT_PRESENT";
		case VK_ERROR_INCOMPATIBLE_DRIVER:		return "VK_ERROR_INCOMPATIBLE_DRIVER";
		case VK_ERROR_TOO_MANY_OBJECTS:			return "VK_ERROR_TOO_MANY_OBJECTS";
		case VK_ERROR_FORMAT_NOT_SUPPORTED:		return "VK_ERROR_FORMAT_NOT_SUPPORTED";
		case VK_ERROR_FRAGMENTED_POOL:			return "VK_ERROR_FRAGMENTED_POOL";
		case VK_ERROR_SURFACE_LOST_KHR:			return "VK_ERROR_SURFACE_LOST_KHR";
		case VK_ERROR_NATIVE_WINDOW_IN_USE_KHR: return "VK_ERROR_NATIVE_WINDOW_IN_USE_KHR";
		case VK_SUBOPTIMAL_KHR:					return "VK_SUBOPTIMAL_KHR";
		case VK_ERROR_OUT_OF_DATE_KHR:			return "VK_ERROR_OUT_OF_DATE_KHR";
		case VK_ERROR_INCOMPATIBLE_DISPLAY_KHR: return "VK_ERROR_INCOMPATIBLE_DISPLAY_KHR";
		case VK_ERROR_VALIDATION_FAILED_EXT:	return "VK_ERROR_VALIDATION_FAILED_EXT";
		case VK_ERROR_OUT_OF_POOL_MEMORY_KHR:	return "VK_ERROR_OUT_OF_POOL_MEMORY_KHR";
		case VK_ERROR_INVALID_SHADER_NV:		return "VK_ERROR_INVALID_SHADER_NV";
		default:
			break;
		}
		if (result < 0)
			return "VK_ERROR_<Unknown>";

		return "VK_<Unknown>";
	}

}

// -------------------------------------------------------
