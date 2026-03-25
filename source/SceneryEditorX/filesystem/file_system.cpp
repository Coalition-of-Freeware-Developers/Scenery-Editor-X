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
 * file_system.cpp
 * -------------------------------------------------------
 * Created: 24/03/2026
 * -------------------------------------------------------
 */
#include "file_system.h"
#include <codecvt>
#include <SDL3/SDL_misc.h>
#include <SDL3/SDL_process.h>
#include <SDL3/SDL_stdinc.h>
#include <SceneryEditorX/asset/asset_extensions.h>
#include <SceneryEditorX/utils/string_utils.h>

// -------------------------------------------------------

namespace SceneryEditorX::IO
{
	// create a silent process (no visible console window) without waiting
	// caller is responsible for calling SDL_WaitProcess and SDL_DestroyProcess
	// note: using STDIO_APP for stdout/stderr helps ensure no console window appears

	/**
	 * @brief  
	 * @param args 
	 * @return  
	 */
	static SDL_Process* CreateSilentProcess(const std::vector<std::string>& args)
	{
		std::vector<const char*> c_args;
		c_args.reserve(args.size());
		for (const auto& arg : args)
		{
			c_args.push_back(arg.c_str());
		}

		c_args.push_back(nullptr);

		SDL_PropertiesID props = SDL_CreateProperties();
		SDL_SetPointerProperty(props, SDL_PROP_PROCESS_CREATE_ARGS_POINTER, const_cast<char**>(c_args.data()));
		SDL_SetNumberProperty(props, SDL_PROP_PROCESS_CREATE_STDIN_NUMBER, SDL_PROCESS_STDIO_NULL);
		SDL_SetNumberProperty(props, SDL_PROP_PROCESS_CREATE_STDOUT_NUMBER, SDL_PROCESS_STDIO_APP);
		SDL_SetNumberProperty(props, SDL_PROP_PROCESS_CREATE_STDERR_NUMBER, SDL_PROCESS_STDIO_APP);
		SDL_SetBooleanProperty(props, SDL_PROP_PROCESS_CREATE_BACKGROUND_BOOLEAN, true);

		SDL_Process* process = SDL_CreateProcessWithProperties(props);
		SDL_DestroyProperties(props);
		return process;
	}

	// run a process silently (no visible console window) and wait for completion
	// note: always using STDIO_APP ensures no console window appears on any platform

	/**
	 * @brief 
	 * @param args 
	 * @param output 
	 */
	static void RunSilentProcess(const std::vector<std::string>& args, std::string * output = nullptr)
	{
		std::vector<const char*> c_args;
		c_args.reserve(args.size());
		for (const auto& arg : args)
		{
			c_args.push_back(arg.c_str());
		}

		c_args.push_back(nullptr);
	
		SDL_PropertiesID props = SDL_CreateProperties();
		SDL_SetPointerProperty(props, SDL_PROP_PROCESS_CREATE_ARGS_POINTER, const_cast<char**>(c_args.data()));
		SDL_SetNumberProperty(props, SDL_PROP_PROCESS_CREATE_STDIN_NUMBER, SDL_PROCESS_STDIO_NULL);
		SDL_SetNumberProperty(props, SDL_PROP_PROCESS_CREATE_STDOUT_NUMBER, SDL_PROCESS_STDIO_APP);
		SDL_SetNumberProperty(props, SDL_PROP_PROCESS_CREATE_STDERR_NUMBER, SDL_PROCESS_STDIO_APP);
		SDL_SetBooleanProperty(props, SDL_PROP_PROCESS_CREATE_BACKGROUND_BOOLEAN, true);
	
		SDL_Process* process = SDL_CreateProcessWithProperties(props);
		SDL_DestroyProperties(props);
	
		if (process)
		{
			// read and wait - this drains stdout/stderr and waits for completion
			size_t dataSize = 0;
			int exitCode = 0;
			char* data = static_cast<char*>(SDL_ReadProcess(process, &dataSize, &exitCode));
			if (output && data && dataSize > 0)
			{
				*output = std::string(data, dataSize);
			}

			if (data)
			{
				SDL_free(data);
			}
	
			SDL_DestroyProcess(process);
		}
	}

	std::filesystem::path FileSystem::GetWorkingDir()
	{
		return std::filesystem::current_path();
	}

	void FileSystem::SetWorkingDir(const std::filesystem::path &path)
	{
		std::filesystem::current_path(path);
	}

	bool FileSystem::CreateDir(const std::filesystem::path &directory)
	{
		return std::filesystem::create_directories(directory);
	}

	bool FileSystem::CreateDir(const std::string &directory)
	{
		return CreateDir(std::filesystem::path(directory));
	}

	bool FileSystem::DirExists(const std::filesystem::path &directory)
	{
		return std::filesystem::exists(directory);
	}

	bool FileSystem::DirExists(const std::string &directory)
	{
		return DirExists(std::filesystem::path(directory));
	}

	bool FileSystem::DeleteFile(const std::filesystem::path &filepath)
	{
		if (!Exists(filepath))
			return false;

		if (std::filesystem::is_directory(filepath))
		{
			return std::filesystem::remove_all(filepath) > 0;
		}

		return std::filesystem::remove(filepath);
	}

	bool FileSystem::MoveFile(const std::filesystem::path &filepath, const std::filesystem::path &dest)
	{
		return Move(filepath, dest / filepath.filename());
	}

	bool FileSystem::CopyFile(const std::filesystem::path &filepath, const std::filesystem::path &dest)
	{
		return Copy(filepath, dest / filepath.filename());
	}

	bool FileSystem::Exists(const std::string &filepath)
	{
		return std::filesystem::exists(std::filesystem::path(filepath));
	}

	bool FileSystem::Exists(const std::filesystem::path &filepath)
	{
		return std::filesystem::exists(filepath);
	}

	bool FileSystem::IsFile(const std::string &path)
	{
		if (path.empty())
			return false;

		try
		{
			if (std::filesystem::exists(path) && std::filesystem::is_regular_file(path))
				return true;
		}
		catch (std::filesystem::filesystem_error &e)
		{
			SEDX_CORE_WARN("%s, %s", e.what(), path.c_str());
		}

		return false;
	}

	FileStatus FileSystem::TryOpenFile(const std::filesystem::path &filePath)
	{
		if (filePath.empty())
			return FileStatus::Invalid;

		if (!Exists(filePath))
			return FileStatus::NotFound;

#ifdef SEDX_PLATFORM_WINDOWS
		const HANDLE handle = CreateFileW(reinterpret_cast<LPCWSTR>(filePath.wstring().c_str()),
										  GENERIC_READ,
										  FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
										  nullptr,
										  OPEN_EXISTING,
										  FILE_ATTRIBUTE_NORMAL,
										  nullptr);

		if (handle != INVALID_HANDLE_VALUE)
		{
			CloseHandle(handle);
			return FileStatus::Success;
		}

		switch (GetLastError())
		{
		case ERROR_SHARING_VIOLATION:
			return FileStatus::Locked;
		case ERROR_ACCESS_DENIED:
			return FileStatus::AccessDenied;
		case ERROR_FILE_NOT_FOUND:
		case ERROR_PATH_NOT_FOUND:
			return FileStatus::NotFound;
		default:
			return FileStatus::UnknownError;
		}
#else
		std::ifstream file(filePath, std::ios::binary);
		if (file.is_open())
		{
			file.close();
			return FileStatus::Success;
		}

		return FileStatus::UnknownError;
#endif
	}

	FileStatus FileSystem::TryOpenFileAndWait(const std::filesystem::path &filepath, uint64_t waitms)
	{
		FileStatus fileStatus = TryOpenFile(filepath);
		if (fileStatus == FileStatus::Locked)
		{
			using namespace std::chrono_literals;
			std::this_thread::sleep_for(operator""ms((unsigned long long)waitms));
			return TryOpenFile(filepath);
		}
		return fileStatus;
	}

	bool FileSystem::Move(const std::filesystem::path &oldFilepath, const std::filesystem::path &newFilepath)
	{
		if (!Exists(oldFilepath) || Exists(newFilepath))
			return false;

		std::error_code ec;
		std::filesystem::rename(oldFilepath, newFilepath, ec);
		if (!ec)
			return true;

		std::filesystem::copy_file(oldFilepath, newFilepath, std::filesystem::copy_options::none, ec);
		if (ec)
			return false;

		std::filesystem::remove(oldFilepath, ec);
		return !ec;
	}

	bool FileSystem::Copy(const std::filesystem::path &oldFilepath, const std::filesystem::path &newFilepath)
	{
		if (Exists(newFilepath))
			return false;

		std::filesystem::copy(oldFilepath, newFilepath);
		return true;
	}

	bool FileSystem::Rename(const std::filesystem::path &oldFilepath, const std::filesystem::path &newFilepath)
	{
		return Move(oldFilepath, newFilepath);
	}

	bool FileSystem::RenameFilename(const std::filesystem::path &oldFilepath, const std::string &newName)
	{
		const std::filesystem::path newPath =
			oldFilepath.parent_path() / std::filesystem::path(newName + oldFilepath.extension().string());
		return Rename(oldFilepath, newPath);
	}

	bool FileSystem::IsDirectory(const std::filesystem::path &filepath)
	{
		return std::filesystem::is_directory(filepath);
	}

	bool FileSystem::IsNewer(const std::filesystem::path &fileA, const std::filesystem::path &fileB)
	{
		// returns true <=> fileA was last modified more recently than fileB
		return std::filesystem::last_write_time(fileA) > std::filesystem::last_write_time(fileB);
	}

	bool FileSystem::ShowFileInExplorer(const std::filesystem::path &path)
	{
		const auto absolutePath = std::filesystem::canonical(path);
		if (!Exists(absolutePath))
			return false;

#ifdef SEDX_PLATFORM_WINDOWS
		std::string cmd = std::format("explorer.exe /select,\"{0}\"", absolutePath.string());
#elif defined(SEDX_PLATFORM_LINUX)
		std::string cmd = std::format("xdg-open \"{0}\"", dirname(absolutePath.string().data()));
#endif
		system(cmd.c_str());
		return true;
	}

	bool FileSystem::OpenDirectoryInExplorer(const std::filesystem::path &path)
	{
#ifdef SEDX_PLATFORM_WINDOWS
		auto absolutePath = std::filesystem::canonical(path);
		if (!Exists(absolutePath))
			return false;

		ShellExecute(nullptr,
					 L"explore",
					 reinterpret_cast<LPCWSTR>(absolutePath.wstring().c_str()),
					 nullptr,
					 nullptr,
					 SW_SHOWNORMAL);
		return true;
#elif defined(SEDX_PLATFORM_LINUX)
		return ShowFileInExplorer(path);
#endif
	}

	bool FileSystem::OpenExternally(const std::filesystem::path &path)
	{
		if (path.empty())
			return false;

		const std::filesystem::path absolutePath = std::filesystem::absolute(path);

		if (!Exists(absolutePath))
			return false;

#ifdef SEDX_PLATFORM_WINDOWS
		const HINSTANCE result = ShellExecute(nullptr,
											  L"open",
											  reinterpret_cast<LPCWSTR>(absolutePath.wstring().c_str()),
											  nullptr,
											  nullptr,
											  SW_SHOWNORMAL);

		return reinterpret_cast<intptr_t>(result) > 32;
#elif defined(SEDX_PLATFORM_LINUX)
		std::string command = std::format("xdg-open \"{}\"", absolutePath.string());
		return system(command.c_str()) == 0;
#else
		return false;
#endif
	}

	std::filesystem::path FileSystem::GetUniqueFileName(const std::filesystem::path &filepath)
	{
		if (!Exists(filepath))
			return filepath;

		int counter = 0;
		auto check_id = [&counter, filepath](const auto &checkID) -> std::filesystem::path {
			++counter;
			const std::string counterStr = [&counter] {
				if (counter < 10)
					return "0" + ToString(counter);

				return ToString(counter);
			}(); // Pad with 0 if < 10;

			std::string newFileName =
				std::format("{} ({})", Utils::RemoveExtension(filepath.filename().string()), counterStr);

			if (filepath.has_extension())
			{
				newFileName = std::format("{}{}", newFileName, filepath.extension().string());
			}

			if (std::filesystem::exists(filepath.parent_path() / newFileName))
			{
				return checkID(checkID);
			}

			return filepath.parent_path() / newFileName;
		};

		return check_id(check_id);
	}

	uint64_t FileSystem::GetLastWriteTime(const std::filesystem::path &filepath)
	{
		if (!Exists(filepath))
			return 0;

		const auto writeTime = std::filesystem::last_write_time(filepath);
		return static_cast<uint64_t>(writeTime.time_since_epoch().count());
	}

	bool FileSystem::IsEmptyOrWhitespace(const std::string &var)
	{
		// Check if it's empty
		if (var.empty())
			return true;

		// Check if it's made out of whitespace characters
		for (char _char : var)
		{
			if (!isspace(_char))
				return false;
		}

		return true;
	}

	bool FileSystem::IsAlphanumeric(const std::string &var)
	{
		if (IsEmptyOrWhitespace(var))
			return false;

		for (char _char : var)
		{
			if (!isalnum(_char))
				return false;
		}

		return true;
	}

	std::string FileSystem::RemoveIllegalCharacters(const std::string &text)
	{
		std::string textLegal = text;

		// Remove characters which are illegal for both names and paths
		std::string illegal = ":?\"<>|";
		for (auto it = textLegal.begin(); it < textLegal.end(); ++it)
		{
			if (illegal.find(*it) != std::string::npos)
			{
				*it = '_';
			}
		}

		// If this is a valid path, return it (otherwise it's a name)
		if (IsDirectory(textLegal))
			return textLegal;

		// Remove slashes which are illegal characters for names
		illegal = "\\/";
		for (auto it = textLegal.begin(); it < textLegal.end(); ++it)
		{
			if (illegal.find(*it) != std::string::npos)
			{
				*it = '_';
			}
		}

		return textLegal;
	}

	std::string FileSystem::GetStringBeforeExpression(const std::string &str, const std::string &exp)
	{
		// ("The quick brown fox", "brown") -> "The quick "
		const size_t position = str.find(exp);
		return position != std::string::npos ? str.substr(0, position) : "";
	}

	std::string FileSystem::GetStringAfterExpression(const std::string &str, const std::string &exp)
	{
		// ("The quick brown fox", "brown") -> "fox"
		const size_t position = str.find(exp);
		return position != std::string::npos ? str.substr(position + exp.length()) : "";
	}

	std::string FileSystem::GetStringBetweenExpressions(const std::string &str,
														const std::string &exp_a,
														const std::string &exp_b)
	{
		// ("The quick brown fox", "The ", " brown") -> "quick"
		const std::regex baseRegex(exp_a + "(.*)" + exp_b);

		if (std::smatch baseMatch; regex_search(str, baseMatch, baseRegex))
		{
			// The first sub_match is the whole string; the next
			// sub_match is the first parenthesized expression.
			if (baseMatch.size() == 2)
				return baseMatch[1].str();
		}

		return str;
	}

	std::string FileSystem::ConvertToUppercase(const std::string &lower)
	{
		std::string upper;
		for (const auto &character : lower)
		{
			const std::locale loc;
			upper += toupper(character, loc);
		}

		return upper;
	}

	std::string FileSystem::ReplaceExpression(const std::string &str, const std::string &from, const std::string &to)
	{
		return regex_replace(str, std::regex(from), to);
	}

	std::wstring FileSystem::StringToWstring(const std::string &str)
	{
		std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;
		return converter.from_bytes(str);
	}

	std::filesystem::path FileSystem::GetPersistentStoragePath()
	{
		// Provide a sensible default persistent storage path depending on platform
#ifdef SEDX_PLATFORM_WINDOWS
		if (const char *p = std::getenv("APPDATA"))
		{
			return std::filesystem::path(p) / "SceneryEditorX";
		}
		if (const char *p = std::getenv("USERPROFILE"))
		{
			return std::filesystem::path(p) / "AppData" / "Roaming" / "SceneryEditorX";
		}
#else
		if (const char *p = std::getenv("XDG_CONFIG_HOME"))
		{
			return std::filesystem::path(p) / "SceneryEditorX";
		}
		if (const char *p = std::getenv("HOME"))
		{
			return std::filesystem::path(p) / ".config" / "SceneryEditorX";
		}
#endif
		// Fallback to current working directory
		return std::filesystem::current_path() / "SceneryEditorX";
	}

	bool FileSystem::CheckEnvVariable(const std::string &key)
	{
		if (key.empty())
			return false;

		const char *val = std::getenv(key.c_str());
		return val != nullptr;
	}

	bool FileSystem::SetEnvVariable(const std::string &key, const std::string &value)
	{
		if (key.empty())
			return false;

#ifdef SEDX_PLATFORM_WINDOWS
		// Use the secure Windows CRT call
		int result = _putenv_s(key.c_str(), value.c_str());
		if (result != 0)
		{
			SEDX_CORE_ERROR_TAG("File Manager", "Failed to set environment variable: %s", key.c_str());
			return false;
		}
		return true;
#else
		// POSIX setenv
		if (setenv(key.c_str(), value.c_str(), 1) != 0)
		{
			SEDX_CORE_ERROR_TAG("File Manager", "Failed to set environment variable: %s", key.c_str());
			return false;
		}
		return true;
#endif
	}

	std::string FileSystem::GetEnvVariable(const std::string &key)
	{
		if (key.empty())
			return std::string();

		const char *val = std::getenv(key.c_str());
		if (!val)
			return std::string();

		return std::string(val);
	}

	std::string FileSystem::GetDirectoryFromFilePath(const std::string &path)
	{
		const size_t last_index = path.find_last_of("\\/");

		if (last_index != std::string::npos)
			return path.substr(0, last_index + 1);

		return "";
	}

	std::string FileSystem::GetWorkingDirectory()
	{
		return std::filesystem::current_path().generic_string();
	}

	std::string FileSystem::GetRootDirectory(const std::string &path)
	{
		return std::filesystem::path(path).root_directory().generic_string();
	}
	std::string FileSystem::GetRelativePath(const std::string &path)
	{
		if (std::filesystem::path(path).is_relative())
			return path;

		// create absolute paths
		const std::filesystem::path p = std::filesystem::absolute(path);
		const std::filesystem::path r = std::filesystem::absolute(GetWorkingDirectory());

		// if root paths are different, return absolute path
		if (p.root_path() != r.root_path())
			return p.generic_string();

		// initialize relative path
		std::filesystem::path result;

		// find out where the two paths diverge
		std::filesystem::path::const_iterator itr_path = p.begin();
		std::filesystem::path::const_iterator itr_relative_to = r.begin();
		while (*itr_path == *itr_relative_to && itr_path != p.end() && itr_relative_to != r.end())
		{
			++itr_path;
			++itr_relative_to;
		}

		// add "../" for each remaining token in relative_to
		if (itr_relative_to != r.end())
		{
			++itr_relative_to;
			while (itr_relative_to != r.end())
			{
				result /= "..";
				++itr_relative_to;
			}
		}

		// add remaining path
		while (itr_path != p.end())
		{
			result /= *itr_path;
			++itr_path;
		}

		return result.generic_string();
	}

	std::string FileSystem::GetParentDirectory(const std::string &path)
	{
		auto parentPath = std::filesystem::path(path).parent_path();

		// If there is no parent path, return path as is
		if (parentPath.empty())
			return path;

		return parentPath.generic_string();
	}

	std::string FileSystem::GetFileNameWithoutExtensionFromFilePath(const std::string &path)
	{
		const auto fileName = GetFileNameFromFilePath(path);
		const size_t lastIndex = fileName.find_last_of('.');

		if (lastIndex != std::string::npos)
		{
			return fileName.substr(0, lastIndex);
		}

		return "";
	}

	std::string FileSystem::GetFileNameFromFilePath(const std::string &path)
	{
		return std::filesystem::path(path).filename().generic_string();
	}

	std::vector<std::string> FileSystem::GetFilesInDirectory(const std::string &path)
	{
		std::vector<std::string> filePaths;
		const std::filesystem::directory_iterator it_end; // default construction yields past-the-end
		for (std::filesystem::directory_iterator it(path); it != it_end; ++it)
		{
			if (!std::filesystem::is_regular_file(it->status()))
			{
				continue;
			}

			try
			{
				// a crash is possible if the characters are
				// something that can't be converted, like Russian.
				filePaths.emplace_back(it->path().string());
			}
			catch (std::system_error &e)
			{
				SEDX_CORE_WARN("Failed to read a file path. %s", e.what());
			}
		}

		return filePaths;
	}

	uint32_t FileSystem::GetFileSize(const std::filesystem::path &path)
	{
		if (path.empty())
			return 0;

		std::error_code ec;
		if (!std::filesystem::exists(path, ec) || !std::filesystem::is_regular_file(path, ec))
			return 0;

		const uint64_t size = std::filesystem::file_size(path, ec);
		if (ec)
		{
			SEDX_CORE_WARN("Failed to get file size: %s, %s", ec.message().c_str(), path.string().c_str());
			return 0;
		}

		if (size > static_cast<uint64_t>(std::numeric_limits<uint32_t>::max()))
		{
			SEDX_CORE_WARN("File size truncated to uint32_t: %s (%llu bytes)", path.string().c_str(), (unsigned long long)size);
			return std::numeric_limits<uint32_t>::max();
		}

		return static_cast<uint32_t>(size);
	}

	uint32_t FileSystem::GetDirFileCount(const std::filesystem::path &path)
	{
		if (path.empty())
			return 0;

		uint32_t count = 0;
		try
		{
			const std::filesystem::directory_iterator it_end;
			for (std::filesystem::directory_iterator it(path); it != it_end; ++it)
			{
				try
				{
					if (std::filesystem::is_regular_file(it->status()))
						++count;
				}
				catch (const std::filesystem::filesystem_error &e)
				{
					SEDX_CORE_WARN("Failed to inspect directory entry: %s", e.what());
					continue;
				}
			}
		}
		catch (const std::filesystem::filesystem_error &e)
		{
			SEDX_CORE_WARN("Failed to iterate directory '%s': %s", path.string().c_str(), e.what());
		}

		return count;
	}

	std::optional<std::filesystem::path> FileSystem::GetUserProfileDir()
	{
		if (const char *p = std::getenv("USERPROFILE"))
		{
			return std::filesystem::path(p);
		}

		return std::nullopt;
	}

	bool FileSystem::Delete(const std::string &path)
	{
		try
		{
			if (std::filesystem::exists(path) && std::filesystem::remove_all(path))
				return true;
		}
		catch (std::filesystem::filesystem_error &e)
		{
			SEDX_CORE_ERROR_TAG("File Manager", "Failed to delete path: %s. Error: %s", path.c_str(), e.what());
		}

		return false;
	}

	bool FileSystem::IsExecutableInPath(const std::string &executable)
	{
		// get PATH using SDL3 cross-platform environment api
		const char *path_env = SDL_GetEnvironmentVariable(SDL_GetEnvironment(), "PATH");
		if (!path_env)
			return false;

		std::string path_str = path_env;

		// detect delimiter and suffix based on COMSPEC presence (runtime detection)
		const char *comspec = SDL_GetEnvironmentVariable(SDL_GetEnvironment(), "COMSPEC");
		char delimiter = comspec ? ';' : ':';
		std::string exe_suffix = comspec ? ".exe" : "";

		// split PATH and search for executable
		std::vector<std::string> paths;
		size_t start = 0;
		size_t end;
		while ((end = path_str.find(delimiter, start)) != std::string::npos)
		{
			paths.emplace_back(path_str.substr(start, end - start));
			start = end + 1;
		}
		paths.emplace_back(path_str.substr(start));

		for (const auto &dir : paths)
		{
			std::filesystem::path exe_path = std::filesystem::path(dir) / (executable + exe_suffix);
			std::error_code ec;
			if (std::filesystem::exists(exe_path, ec) && std::filesystem::is_regular_file(exe_path, ec))
				return true;
		}

		return false;
	}

	bool FileSystem::CreateArchive(const std::string &path, const std::vector<std::string> &includePaths)
	{
		if (includePaths.empty())
		{
			SEDX_CORE_ERROR_TAG("File Manager", "No paths provided to archive");
			return false;
		}

		/**
		 * TODO: Bundle 7z with the editor if the installer doesn't detect an existing user installation of 7Zip 
		 * and use a fixed relative path to ensure it is always found
		 */
		std::string seven_zip_exe;

		// find 7z executable - check all possible locations and names at runtime
		std::vector<std::string> candidates = {"7z.exe", "scripts/7z.exe", "7z", "7za"};

		for (const auto &candidate : candidates)
		{
			if (Exists(candidate) || IsExecutableInPath(candidate))
			{
				seven_zip_exe = candidate;
				break;
			}
		}

		if (seven_zip_exe.empty())
		{
			SEDX_CORE_ERROR_TAG("File Manager",
								"7z not found. Please ensure it exists in the current directory, scripts/, or PATH.");
			return false;
		}

		// delete existing archive if it exists
		if (Exists(path))
		{
			Delete(path);
		}

		SEDX_CORE_INFO_TAG("File Manager", "Creating archive: %s", path.c_str());

		// build command arguments: 7z a archive.7z file1 file2 dir1 ...
		std::vector<std::string> args = {seven_zip_exe, "a", path};
		for (const std::string &path : includePaths)
		{
			args.push_back(path);
		}
		// add silent flags
		args.emplace_back("-bso0");
		args.emplace_back("-bsp0");

		RunSilentProcess(args);

		// verify archive was created
		if (!Exists(path))
		{
			SEDX_CORE_ERROR_TAG("File Manager", "Failed to create archive: %s", path.c_str());
			return false;
		}

		SEDX_CORE_INFO_TAG("File Manager", "Archive created: %s", path.c_str());
		return true;
	}

	void FileSystem::OpenUrl(const char *url)
	{
		SDL_OpenURL(url);
	}

	void FileSystem::OpenUrl(const std::string &url)
	{
		SDL_OpenURL(url.c_str());
	}

	bool FileSystem::IsValidExtension(const std::string &path, SceneryEditorX::AssetType assetType)
	{
		if (path.empty())
			return false;

		// Get extension and normalize to lower-case for comparison
		std::string ext = Utils::String::ToLowerCopy(Utils::GetExtension(path));

		// Look up extension in the global asset extension map
		auto it = SceneryEditorX::s_AssetExtensionMap.find(ext);
		if (it == SceneryEditorX::s_AssetExtensionMap.end())
			return false;

		return it->second == assetType;
	}

	void FileSystem::GetAppData()
	{
		if (const char *appDataPath = getenv("APPDATA"))
		{
			std::string appDataDir(appDataPath);
			appDataDir += "\\SceneryEditorX";
			std::filesystem::create_directories(appDataDir);
		}
		else
		{
			SEDX_CORE_ERROR_TAG("File Manager", "Failed to get APPDATA environment variable");
		}
	}

}

// -------------------------------------------------------
