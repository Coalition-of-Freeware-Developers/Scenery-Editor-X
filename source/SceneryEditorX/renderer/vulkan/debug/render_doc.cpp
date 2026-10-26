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
 * render_doc.cpp
 * -------------------------------------------------------
 * Created: 29/03/2026
 * -------------------------------------------------------
 */
#include "render_doc.h"
#include <vector>
#include <SceneryEditorX/utils/system_detection.h>

#ifndef RENDERDOC_ENABLED
	#define RENDERDOC_ENABLED 0
#else 
	#ifdef SEDX_PLATFORM_WINDOWS
		#define RENDERDOC_ENABLED 1
		#include "renderdoc/app/renderdoc_app.h"
	#elif RENDERDOC_ENABLED & !SEDX_PLATFORM_WINDOWS
		#include "renderdoc_app.h"
	#endif
#endif

// -----------------------------------------------------------------

namespace SceneryEditorX
{
	static RENDERDOC_Version rdc_version = eRENDERDOC_API_Version_1_6_0;
	static RENDERDOC_API_1_6_0* rdc_api  = nullptr;
	static void* rdc_module              = nullptr;

#if defined(_WIN32) // windows
	static std::vector<std::wstring> get_renderdoc_dll_paths()
	{
		std::vector<std::wstring> dll_paths;

		// 1. Check the standard installation path first (most reliable)
		const std::wstring standard_path = L"C:\\Program Files\\RenderDoc\\renderdoc.dll";
		WIN32_FIND_DATA find_file_data = { 0 };
		HANDLE file_handle = FindFirstFile(standard_path.c_str(), &find_file_data);
		if (file_handle != INVALID_HANDLE_VALUE)
		{
			dll_paths.push_back(standard_path);
			FindClose(file_handle);
			return dll_paths; // Found it, no need to scrape registry
		}

		// 2. Fallback: Search the registry (Installer Folders)
		// This is messy and can return many paths, some of which might be plugin folders
		static const wchar_t* installer_folders_path = TEXT("SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Installer\\Folders");

		HKEY hkey;
		if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, installer_folders_path, 0, KEY_READ, &hkey) != ERROR_SUCCESS) 
			return dll_paths;

		TCHAR    achClass[MAX_PATH] = TEXT(""); // buffer for class name
		DWORD    cchClassName = MAX_PATH;       // size of class string
		DWORD    cSubKeys = 0;                  // number of subkeys
		DWORD    cbMaxSubKey;                   // longest subkey size
		DWORD    cchMaxClass;                   // longest class string
		DWORD    c_values;                      // number of values for keyPath
		DWORD    cchMaxValue;                   // longest value name
		DWORD    cbMaxValueData;                // longest value data
		DWORD    cbSecurityDescriptor;          // size of security descriptor
		FILETIME ftLastWriteTime;               // last write time

		DWORD query_result = RegQueryInfoKey(
			hkey,                  // keyPath handle
			achClass,              // buffer for class name
			&cchClassName,         // size of class string
			nullptr,               // reserved
			&cSubKeys,             // number of subkeys
			&cbMaxSubKey,          // longest subkey size
			&cchMaxClass,          // longest class string
			&c_values,             // number of values for this keyPath
			&cchMaxValue,          // longest value name
			&cbMaxValueData,       // longest value data
			&cbSecurityDescriptor, // security descriptor
			&ftLastWriteTime);     // last write time

		constexpr uint32_t MAX_VALUE_NAME = 8192;
		TCHAR ach_value[MAX_VALUE_NAME];
		wchar_t enum_value[MAX_VALUE_NAME] = TEXT("");

		if (c_values)
		{
			for (DWORD i = 0, retCode = ERROR_SUCCESS; i < c_values; i++)
			{
				DWORD cchValue = MAX_VALUE_NAME;
				ach_value[0] = '\0';
				DWORD type = REG_SZ;
				DWORD size;
				memset(enum_value, '\0', MAX_VALUE_NAME);

				retCode = RegEnumValue(hkey, i,
					ach_value,
					&cchValue,
					nullptr,
					&type,
					nullptr,
					&size);

				if (type != REG_SZ || retCode != ERROR_SUCCESS)
					continue;

				std::wstring path(ach_value);
				if (path.find(L"RenderDoc") != std::wstring::npos)
				{
					// Ensure valid directory path formatting
					if (!path.empty() && path.back() != L'\\' && path.back() != L'/')
					{
						path += L"\\";
					}

					const std::wstring rdc_dll_path = path + TEXT("renderdoc.dll");
					
					WIN32_FIND_DATA find_data = { 0 };
					HANDLE handle = FindFirstFile(rdc_dll_path.c_str(), &find_data);
					if (handle != INVALID_HANDLE_VALUE)
					{
						dll_paths.push_back(rdc_dll_path);
						FindClose(handle);
					}
				}
			}
		}

		RegCloseKey(hkey);

		return dll_paths;
	}
#endif

	void RenderDoc::OnPreDeviceCreation()
	{
		// load renderdoc module and get a pointer to it's api
		if (rdc_api == nullptr)
		{
			pRENDERDOC_GetAPI rdc_get_api = nullptr;
#if defined(SEDX_PLATFORM_WINDOWS) // windows
			// if renderdoc is already injected into the engine, use the existing module
			rdc_module = ::GetModuleHandleA("renderdoc.dll");

			// if renderdoc is not injected, load the module now
			if (rdc_module == nullptr)
			{
				std::vector<std::wstring> RDocDllPaths = get_renderdoc_dll_paths();
				SEDX_CORE_ASSERT(!RDocDllPaths.empty(), "Could not find any install locations for renderdoc.dll");
				
				// Try to load paths until one works
				for (const auto& path : RDocDllPaths)
				{
					rdc_module = ::LoadLibraryW(path.c_str());
					if (rdc_module) break;
				}
			}

			SEDX_CORE_ASSERT(rdc_module != nullptr, "Failed to get RenderDoc module");

			// get the address of RENDERDOC_GetAPI
			rdc_get_api = (pRENDERDOC_GetAPI)::GetProcAddress(static_cast<HMODULE>(rdc_module), "RENDERDOC_GetAPI");
#else // linux
			SEDX_CORE_ASSERT(false, "Not implemented");
#endif
			SEDX_CORE_ASSERT(rdc_get_api != nullptr, "Failed to RENDERDOC_GetAPI function address from renderdoc.dll");
			SEDX_CORE_ASSERT(rdc_get_api(rdc_version, (void**)&rdc_api) != 0, "Failed to get RenderDoc API pointer");
		}

		SEDX_CORE_ASSERT(rdc_api != nullptr, "RenderDoc API has not been initialized");

		// disable muting of validation/debug layer messages
		rdc_api->SetCaptureOptionU32(eRENDERDOC_Option_APIValidation, 1);
		rdc_api->SetCaptureOptionU32(eRENDERDOC_Option_DebugOutputMute, 0);
		rdc_api->SetCaptureOptionU32(eRENDERDOC_Option_VerifyBufferAccess, 1);
	}

	void RenderDoc::Shutdown()
	{
		if (rdc_module != nullptr)
		{
#if defined(_WIN32) // windows
			::FreeLibrary(static_cast<HMODULE>(rdc_module));
#else
			SEDX_CORE_ASSERT(false, "Not implemented");
#endif
		}
	}

	void RenderDoc::FrameCapture()
	{
		SEDX_CORE_ASSERT(rdc_api != nullptr, "RenderDoc is not initialized");

		// capture the next frame
		rdc_api->TriggerCapture();
		//rdc_api->TriggerMultiFrameCapture(2);

		LaunchRenderDocUi();
	}

	void RenderDoc::StartCapture()
	{
		SEDX_CORE_ASSERT(rdc_api != nullptr, "RenderDoc is not initialized");

		rdc_api->StartFrameCapture(nullptr, nullptr);
	}

	void RenderDoc::EndCapture()
	{
		SEDX_CORE_ASSERT(rdc_api != nullptr, "RenderDoc is not initialized");

		rdc_api->EndFrameCapture(nullptr, nullptr);

		LaunchRenderDocUi();
	}

	void RenderDoc::LaunchRenderDocUi()
	{
		SEDX_CORE_ASSERT(rdc_api != nullptr, "RenderDoc is not initialized");

		// if the renderdoc ui is already running, make sure it's visible
		if (rdc_api->IsTargetControlConnected())
		{
			SEDX_CORE_INFO_TAG("RenderDoc","Bringing RenderDoc to foreground...");
			rdc_api->ShowReplayUI();
		}
		// if the renderdoc ui is not running, launch it and connect
		else
		{
			SEDX_CORE_INFO_TAG("RenderDoc","Launching RenderDoc...");
			// Returns the PID of the replay UI if successful, 0 if not.
			if (rdc_api->LaunchReplayUI(1, "") == 0)
			{
				SEDX_CORE_ERROR_TAG("RenderDoc","Failed to launch RenderDoc");
			}
		}
	}
}

// -----------------------------------------------------------------
