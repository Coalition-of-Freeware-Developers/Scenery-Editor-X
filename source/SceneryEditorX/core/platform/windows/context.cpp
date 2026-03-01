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
	
	
	WindowsContext::WindowsContext(HINSTANCE hInstance, HINSTANCE hPrevInstance, PSTR lpCmdLine, INT nCmdShow) : PlatformContext()
	{
	    m_WorkingDirectory = "";
	    m_TempDirectory = GetTempDir();
	    m_CommandLineArgs = GetArgs();
	
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
        if (freopen_s(&fp, "conin$", "r", stdin) != 0)		SEDX_CORE_WARN_TAG("INIT", "Failed to redirect stdin to console");
        if (freopen_s(&fp, "conout$", "w", stdout) != 0)	SEDX_CORE_WARN_TAG("INIT", "Failed to redirect stdout to console");
        if (freopen_s(&fp, "conout$", "w", stderr) != 0)	SEDX_CORE_WARN_TAG("INIT", "Failed to redirect stderr to console");
    }
	
} // namespace SceneryEditorX

// -----------------------------------------------
