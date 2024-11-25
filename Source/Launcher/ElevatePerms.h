#pragma once

#define _WIN32_WINNT 0x0601

#include <Logging.hpp>
#include <shellapi.h>
#include <spdlog/spdlog.h>
#include <windows.h>

// Function to check if the application is running as administrator
bool RunningAsAdmin()
{
    BOOL isAdmin = FALSE;
    HANDLE hToken = NULL;

    // Open the process token
    if (OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY, &hToken))
    {
        TOKEN_ELEVATION elevation;
        DWORD size;

        if (GetTokenInformation(hToken, TokenElevation, &elevation, sizeof(elevation), &size))
        {
            isAdmin = elevation.TokenIsElevated;
        }

        CloseHandle(hToken);
    }

    return isAdmin;
}

// Function to relaunch the application with administrator privileges
void RelaunchAsAdmin()
{
    TCHAR szPath[MAX_PATH];
    if (GetModuleFileName(NULL, szPath, MAX_PATH))
    {
        SHELLEXECUTEINFO sei{};
        sei.cbSize = sizeof(SHELLEXECUTEINFO);
        sei.lpVerb = TEXT("runas"); // Request elevation
        sei.lpFile = szPath;        // Path to the current executable
        sei.nShow = SW_SHOWNORMAL;
        sei.lpParameters = TEXT("--elevated"); // Pass an argument to detect elevated mode

        if (!ShellExecuteEx(&sei))
        {
            DWORD error = GetLastError();
            if (error == ERROR_CANCELLED)
            {
                spdlog::error("User declined the UAC prompt. Elevation aborted.");
                //CleanUp(); // Ensure proper cleanup
            }
            else
            {
                spdlog::error("Failed to elevate privileges. Error code: {}", error);
            }
            return;
        }

        spdlog::info("Successfully relaunched with administrator privileges. Terminating non-elevated process.");
        TerminateProcess(GetCurrentProcess(), 0); // Forcefully terminate the non-elevated process
    }
    else
    {
        DWORD error = GetLastError();
        spdlog::error("Failed to get the module file name. Error code: {}", error);
        return;
    }
}

// Function to check for administrator privileges and relaunch if necessary
int adminCheck()
{
    if (!RunningAsAdmin())
    {
        spdlog::info("Administrator privileges are required. Relaunching as admin...");
        RelaunchAsAdmin();
        return 0; // Exit the current process after relaunching
    }

    // Proceed with admin-required functions
    spdlog::info("Running with administrator privileges. Proceeding with tasks...");

    return 0;
}

