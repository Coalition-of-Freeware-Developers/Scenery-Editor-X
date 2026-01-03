/**
* -------------------------------------------------------
* Scenery Editor X
* -------------------------------------------------------
* Copyright (c) 2025 Thomas Ray
* Copyright (c) 2025 Coalition of Freeware Developers
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
	inline const std::string GetTempDir()
	{
	    std::string tempPath = "temp/";

	    TCHAR tempBuffer[MAX_PATH];
        DWORD temp_path_ret = GetTempPath(MAX_PATH, tempBuffer);
        if (temp_path_ret > MAX_PATH || temp_path_ret == 0)
	    {
	        tempPath = "temp/";
	    }
	    else
	    {
	        tempPath = std::string(tempBuffer) + "/";
	    }
	
	    return tempPath;
	}

    /**
     * @brief Convert a wide string to a standard string
     * @param wstr The wide string to convert
     * @return The converted standard UTF8 string
     */
    inline std::string wstr_to_str(const std::wstring &wstr)
	{
	    if (wstr.empty())
		{
			return std::string();
        }

	    auto wstr_len = static_cast<int>(wstr.size());
        auto str_len = WideCharToMultiByte(CP_UTF8, 0, wstr.data(), wstr_len, nullptr, 0, nullptr, nullptr);

        std::string str(str_len, 0);
        WideCharToMultiByte(CP_UTF8, 0, wstr.data(), wstr_len, str.data(), str_len, nullptr, nullptr);

        return str;
	}

    inline std::vector<std::string> GetArgs()
    {
        int argc;
        LPWSTR *argv = CommandLineToArgvW(GetCommandLineW(), &argc);

        // Ignore the first argument containing the application full path
        std::vector<std::wstring> arg_strings(argv + 1, argv + argc);
        std::vector<std::string> args;

        for (auto &arg : arg_strings)
        {
            args.push_back(wstr_to_str(arg));
        }

        return args;
    }


    WindowsContext::WindowsContext(HINSTANCE hInstance, HINSTANCE hPrevInstance, PSTR lpCmdLine, INT nCmdShow)	: 		PlatformContext()
	{
		m_WorkingDirectory = "";
		m_TempDirectory = GetTempDir();
		m_CommandLineArgs = GetArgs();

	    // Attempt to attach to the parent process console if it exists
        if (!AttachConsole(ATTACH_PARENT_PROCESS))
        {
            // No parent console, allocate a new one for this process
            if (!AllocConsole())
            {
                throw std::runtime_error{"AllocConsole error"};
            }
        }

        FILE *fp;
        freopen_s(&fp, "conin$", "r", stdin);
        freopen_s(&fp, "conout$", "w", stdout);
        freopen_s(&fp, "conout$", "w", stderr);
    }

}

// -----------------------------------------------
