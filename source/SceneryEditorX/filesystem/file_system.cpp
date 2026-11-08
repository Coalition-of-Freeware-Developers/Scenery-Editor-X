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
#include "vfs.h"
#include <mutex>
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

	bool FileSystem::CreateDir(const std::filesystem::path &directory)
	{
		return std::filesystem::create_directories(directory);
	}

	bool FileSystem::CreateDir(const std::string &directory)
	{
		return CreateDir(VFS::FromUtf8(directory));
	}

	bool FileSystem::DirExists(const std::filesystem::path &directory)
	{
		return std::filesystem::exists(directory);
	}

	bool FileSystem::DirExists(const std::string &directory)
	{
		return DirExists(VFS::FromUtf8(directory));
	}

	bool FileSystem::CopyFile(const std::filesystem::path &filepath, const std::filesystem::path &dest)
	{
		return Copy(filepath, dest / filepath.filename());
	}

	bool FileSystem::Exists(const std::string &filepath)
	{
		return std::filesystem::exists(VFS::FromUtf8(filepath));
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
			const std::filesystem::path file = VFS::FromUtf8(path);
			if (std::filesystem::exists(file) && std::filesystem::is_regular_file(file))
				return true;
		}
		catch (std::filesystem::filesystem_error &e)
		{
			SEDX_CORE_WARN("{}, {}", e.what(), path.c_str());
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

	bool FileSystem::IsDirectory(const std::filesystem::path &filepath)
	{
		return std::filesystem::is_directory(filepath);
	}

	bool FileSystem::OpenDirectoryInExplorer(const std::filesystem::path &path)
	{
		std::error_code ec;
		const auto absolutePath = std::filesystem::canonical(path, ec);
		if (ec || !Exists(absolutePath))
			return false;

#ifdef SEDX_PLATFORM_WINDOWS
		ShellExecute(nullptr,
					 L"explore",
					 reinterpret_cast<LPCWSTR>(absolutePath.wstring().c_str()),
					 nullptr,
					 nullptr,
					 SW_SHOWNORMAL);
		return true;
#elif defined(SEDX_PLATFORM_MACOS)
		return system(std::format("open \"{}\"", absolutePath.string()).c_str()) == 0;
#else
		return system(std::format("xdg-open \"{}\"", absolutePath.string()).c_str()) == 0;
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
#elif defined(SEDX_PLATFORM_MACOS)
		return system(std::format("open \"{}\"", absolutePath.string()).c_str()) == 0;
#else
		std::string command = std::format("xdg-open \"{}\"", absolutePath.string());
		return system(command.c_str()) == 0;
#endif
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

	const std::vector<std::string> &FileSystem::InitAppLocations()
	{
		static std::once_flag once;
		static std::vector<std::string> report;
		std::call_once(once, []
		{
			const VFS::AppLayout layout = VFS::MakeAppLayout();
			VFS::SetAppLayout(layout);

			// Where earlier versions kept what now has its own place: caches out of the (roaming) settings folder and
			// the temp folder, recovery data next to the other machine-local data. Nothing at a target is overwritten.
			report = VFS::MoveLegacyFiles({
				{ layout.settings / "airport_index.bin", layout.cache / "airport_index.bin" },
				{ layout.settings / "gateway", layout.cache / "gateway" },
				{ layout.temp / "shader-cache", layout.cache / "shaders" },
				{ layout.settings / "project-cache", layout.data / "project-cache" } });

			// The temp "cache" folder startup used to create (nothing ever used it)
			std::error_code ec;
			if (std::filesystem::is_directory(layout.temp / "cache", ec) && std::filesystem::is_empty(layout.temp / "cache", ec))
				std::filesystem::remove(layout.temp / "cache", ec);

			std::string error;
			if (!VFS::MountAppLayout(VFS::Get(), layout, &error))
				report.push_back("Could not set up the editor's folders: " + error);
		});
		return report;
	}

	std::filesystem::path FileSystem::GetPersistentStoragePath()
	{
		return VFS::GetAppLocation(VFS::AppLocation::Settings);
	}

	std::filesystem::path FileSystem::GetAppDataRoaming()
	{
		const std::filesystem::path folder = VFS::GetAppLocation(VFS::AppLocation::Settings);
		VFS::CreateFolders(folder);		// best effort
		return folder;
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
		// UTF-8: generic_string() throws on MSVC for a folder outside the ANSI code page (an install under C:\Users\日本)
		return VFS::ToGenericUtf8(std::filesystem::current_path());
	}

	std::string FileSystem::GetRelativePath(const std::string &path)
	{
		if (VFS::FromUtf8(path).is_relative())
			return path;

		// create absolute paths
		const std::filesystem::path p = std::filesystem::absolute(VFS::FromUtf8(path));
		const std::filesystem::path r = std::filesystem::current_path();

		// if root paths are different, return absolute path
		if (p.root_path() != r.root_path())
			return VFS::ToGenericUtf8(p);

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

		return VFS::ToGenericUtf8(result);
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
		return VFS::ToGenericUtf8(VFS::FromUtf8(path).filename());
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
			SEDX_CORE_WARN("Failed to get file size: {}, {}", ec.message().c_str(), path.string().c_str());
			return 0;
		}

		if (size > static_cast<uint64_t>(std::numeric_limits<uint32_t>::max()))
		{
			SEDX_CORE_WARN("File size truncated to uint32_t: {} (%llu bytes)", path.string().c_str(), (unsigned long long)size);
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
					SEDX_CORE_WARN("Failed to inspect directory entry: {}", e.what());
					continue;
				}
			}
		}
		catch (const std::filesystem::filesystem_error &e)
		{
			SEDX_CORE_WARN("Failed to iterate directory '{}': {}", path.string().c_str(), e.what());
		}

		return count;
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
			SEDX_CORE_ERROR_TAG("File Manager", "Failed to delete path: {}. Error: {}", path.c_str(), e.what());
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
		std::string error;
		if (!VFS::CreateFolders(GetPersistentStoragePath(), &error))
			SEDX_CORE_ERROR_TAG("File Manager", "Could not create the settings folder: {}", error);
	}

}

// -------------------------------------------------------
