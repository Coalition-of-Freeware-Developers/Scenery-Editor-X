#pragma once

#include <windows.h>
#include <shellapi.h>
#include <Logging.hpp>

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
        // Use ShellExecute to relaunch with elevated privileges
        SHELLEXECUTEINFO sei = { sizeof(SHELLEXECUTEINFO) };
        sei.lpVerb = TEXT("runas");     // Request elevation
        sei.lpFile = szPath;            // Path to the current executable
        sei.hwnd = NULL;
        sei.nShow = SW_NORMAL;

        if (!ShellExecuteEx(&sei))
        {
            DWORD error = GetLastError();
            if (error == ERROR_CANCELLED)
            {
                spdlog::error("User declined the UAC prompt.");
            }
            else
            {
                spdlog::error("Failed to elevate privileges. Error: {}", error);
            }
        }
    }
}

int main()
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
