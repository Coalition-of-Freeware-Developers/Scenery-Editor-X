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
 * context.cpp
 * -------------------------------------------------------
 * Windows application context
 * -------------------------------------------------------
 * Created: 25/5/2025
 * -------------------------------------------------------
 */
#include <SceneryEditorX/core/platform/windows/context.h>
#include <cassert>
#include <stdexcept>

// -----------------------------------------------

namespace SceneryEditorX
{
	/**
	 * @brief Convert a wide string to a standard string
	 * @param wstr The wide string to convert
	 * @return The converted standard UTF8 string
	 */
	static std::string WstrToStr(const std::wstring &wstr)
	{
		if (wstr.empty())
		{
			return {};
		}
	
		auto wstrLen = static_cast<int>(wstr.size());
		auto strLen = WideCharToMultiByte(CP_UTF8, 0, wstr.data(), wstrLen, nullptr, 0, nullptr, nullptr);
	
		std::string str(strLen, 0);
		WideCharToMultiByte(CP_UTF8, 0, wstr.data(), wstrLen, str.data(), strLen, nullptr, nullptr);
	
		return str;
	}

	/**
	 * @brief Retrieves the path to the temporary directory for the current user.
	 * @return The path to the temporary directory.
	 */
	static std::string GetTempDir()
	{
		std::string tempPath = "temp/";
	
		WCHAR tempBuffer[MAX_PATH];
		DWORD tempPathRet = GetTempPathW(MAX_PATH, tempBuffer);
		if (tempPathRet > MAX_PATH || tempPathRet == 0)
		{
			tempPath = "temp/";
		}
		else
		{
			tempPath = WstrToStr(std::wstring(tempBuffer)) + "/";
		}
	
		return tempPath;
	}

	/**
	 * @brief Retrieves the command line arguments for the application.
	 * @return A vector of strings containing the command line arguments.
	 */
	static std::vector<std::string> GetArgs()
	{
		int argc;
		LPWSTR *argv = CommandLineToArgvW(GetCommandLineW(), &argc);
	
		// Ignore the first argument containing the application full path
		std::vector<std::wstring> argStrings(argv + 1, argv + argc);
		std::vector<std::string> args;

		args.reserve(argStrings.size());
		for (auto &arg : argStrings)
		{
			args.push_back(WstrToStr(arg));
		}
	
		return args;
	}

	/**
	 * @brief The folder holding resources/ for this executable: beside it (installed) or one up (bin/ in a checkout).
	 * @return The folder, or empty when neither has resources/shaders.
	 */
	static std::filesystem::path FindAppRoot()
	{
		std::wstring module(MAX_PATH, L'\0');
		DWORD length = 0;
		while ((length = GetModuleFileNameW(nullptr, module.data(), static_cast<DWORD>(module.size()))) == module.size())
			module.resize(module.size() * 2);
		if (length == 0)
			return {};
		module.resize(length);

		std::error_code ec;
		const std::filesystem::path exeFolder = std::filesystem::path(module).parent_path();
		for (const std::filesystem::path &folder : { exeFolder, exeFolder.parent_path() })
		{
			if (std::filesystem::is_directory(folder / "resources" / "shaders", ec))
				return folder;
		}
		return {};
	}

	WindowsContext::WindowsContext(HINSTANCE hInstance, HINSTANCE hPrevInstance, PSTR lpCmdLine, INT nCmdShow) : PlatformContext()
	{
		m_WorkingDirectory = "";
		m_TempDirectory = GetTempDir();
		m_CommandLineArgs = GetArgs();

		// Shaders, fonts and icons load from resources/ under the working directory, but Explorer starts a file
		// association (double-clicking a .edX) in the document's folder: work from the executable's root instead.
		std::error_code ec;
		m_LaunchDirectory = std::filesystem::current_path(ec);
		if (const std::filesystem::path root = FindAppRoot(); !root.empty())
			std::filesystem::current_path(root, ec);

	#ifdef SEDX_DEBUG
		// Attempt to attach to the parent process console if it exists
		if (!AttachConsole(ATTACH_PARENT_PROCESS))
		{
			// No parent console, allocate a new one for this process
			if (!AllocConsole())
			{
				throw std::runtime_error{"AllocConsole error"};
			}
		}
	#endif

		FILE *fp = nullptr;
		if (freopen_s(&fp, "conin$", "r", stdin) != 0)		SEDX_CORE_WARN_TAG("WindowsContext", "Failed to redirect stdin to console");
		if (freopen_s(&fp, "conout$", "w", stdout) != 0)	SEDX_CORE_WARN_TAG("WindowsContext", "Failed to redirect stdout to console");
		if (freopen_s(&fp, "conout$", "w", stderr) != 0)	SEDX_CORE_WARN_TAG("WindowsContext", "Failed to redirect stderr to console");
	}
	
} // namespace SceneryEditorX

// -----------------------------------------------
