/**
* -------------------------------------------------------
* Scenery Editor X
* -------------------------------------------------------
* Copyright (c) 2025 Thomas Ray 
* Copyright (c) 2025 Coalition of Freeware Developers
* -------------------------------------------------------
* elevate_perms.h
* -------------------------------------------------------
* Created: 16/3/2025
* -------------------------------------------------------
*/

#pragma once

//#define _WIN32_WINNT 0x0601
#include <shellapi.h>

// -------------------------------------------------------

// Function to check if the application is running as administrator
bool RunningAsAdmin()
{
    BOOL isAdmin = FALSE;       // Assume the application is not running as administrator
    HANDLE hToken = NULL;       // Handle to the process token

    // Open the process token
    if (OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY, &hToken))                                    // Get the process token
    {
        TOKEN_ELEVATION elevation;                                                                      // Token elevation structure
        DWORD size;                                                                                     // Size of the elevation token

        if (GetTokenInformation(hToken, TokenElevation, &elevation, sizeof(elevation), &size))          // Get the elevation information
        {
            isAdmin = elevation.TokenIsElevated;                                                        // Set the return value
        }

        CloseHandle(hToken);                                                                            // Close the token handle
    }
    else
    {
        LAUNCHER_LOG_ERROR("Failed to open process token. Error code: {}", GetLastError());
		//ErrMsg("Failed to open process token. Error code: " + GetLastError()); // Display an error message
    }

    return isAdmin;
}

// Function to relaunch the application with administrator privileges
void RelaunchAsAdmin()
{
    TCHAR szPath[MAX_PATH];                                             // Buffer to store the path

    if (GetModuleFileName(NULL, szPath, MAX_PATH))                      // If the path could be retrieved successfully
    {
        SHELLEXECUTEINFO sei{};                                         // Shell execute information structure
        sei.cbSize = sizeof(SHELLEXECUTEINFO);                          // Size of the structure
        sei.lpVerb = TEXT("runas");                                     // Request elevation
        sei.lpFile = szPath;                                            // Path to the current executable
        sei.lpParameters = TEXT("--elevated");                          // Pass an argument to detect elevated mode
        sei.nShow = SW_SHOWNORMAL;                                      // Show the window
        sei.fMask = SEE_MASK_NOCLOSEPROCESS;                            // Ensure the process handle is not closed

        LAUNCHER_LOG_INFO("Preparing to relaunch as administrator.");
        //std::cerr << "Preparing to relaunch as administrator." << std::endl;
        //Log::Shutdown(); // Shutdown logging before relaunch

        // If ShellExecuteEx fails, log the error code and return from the function
        if (!ShellExecuteEx(&sei))                                      // Relaunch the application with elevated privileges
        {
            DWORD error = GetLastError();                               // Get the error code from the last operation
            LAUNCHER_LOG_ERROR("Failed to elevate privileges. Error code: {}", error);
			//ErrMsg("Failed to elevate privileges. Error code: " + error); // Display an error message
            //std::cerr << "Failed to elevate privileges. Error code: " << error << std::endl;
            return; // Return without terminating the current process
        }
        else  // If ShellExecuteEx succeeds
        {
            LAUNCHER_LOG_INFO("Relaunched successfully. Terminating current process.");
            //std::cerr << "Relaunched successfully. Terminating current process."  << std::endl;

            //Log::Shutdown(); // Shutdown logging before relaunch


#ifdef SEDX_DEBUG // Attach the debugger to the new elevated process if running in debug mode
            if (IsDebuggerPresent())
            {
                LAUNCHER_LOG_INFO("Debugger is present. Attaching to the new elevated process.");
                //std::cerr << "Debugger is present. Attaching to the new elevated process." << std::endl;

                if (sei.hProcess != NULL && !DebugActiveProcess(GetProcessId(sei.hProcess)))
                {
                    LAUNCHER_LOG_ERROR("Failed to attach debugger. Process handle is NULL.");
					//ErrMsg("Failed to attach debugger. Process handle is NULL."); // Display an error message
                    //std::cerr << "Failed to attach debugger. Process handle is NULL." << std::endl;
                }
            }
#endif

            //Log::Shutdown();                          // Shutdown logging
            //TerminateProcess(GetCurrentProcess(), 0); // Terminate the current process
        }
    }
    else // If the path could not be retrieved
    {
        LAUNCHER_LOG_ERROR("Failed to get module file name.");
		//ErrMsg("Failed to get module file name."); // Display an error message
        //std::cerr << "Failed to get module file name." << std::endl;

        Log::Shutdown();    // Shutdown logging
        EXIT_FAILURE;       // Exit the application
    }
}

// Function to check for administrator privileges and relaunch if necessary
int adminCheck()
{
    // Check if the application is running with the "--elevated" argument

    int argc;                                                       // Argument count
    LPWSTR *argv = CommandLineToArgvW(GetCommandLineW(), &argc);    // Get the command line arguments
    bool isElevated = false;                                        // Flag to check if the application is running with elevated privileges
    for (int i = 0; i < argc; ++i)                                  // Iterate through the arguments
    {
        if (wcscmp(argv[i], L"--elevated") == 0)                    // Check if the "--elevated" argument is present
        {
            isElevated = true;                                      // Set the flag to true
            break;                                                  // Break out of the loop
        }
    }
    LocalFree(argv);                                                // Free the allocated memory for the arguments

    if (!isElevated && !RunningAsAdmin())                           // Check if the application is running as administrator
    {
#ifdef _DEBUG
        LAUNCHER_LOG_INFO("Running in debug mode. Skipping relaunch as admin.");
        //std::cerr << "Running in debug mode. Skipping relaunch as admin." << std::endl;
#else
        LAUNCHER_LOG_INFO("Administrator privileges are required. Relaunching as admin...");
        //std::cerr << "Administrator privileges are required. Relaunching as admin..." << std::endl;
        RelaunchAsAdmin();                                          // Relaunch the application with admin privileges
        return 0;                                                   // Exit the current process after relaunching
#endif
    }

    // Proceed with admin-required functions
    LAUNCHER_LOG_INFO("Running with administrator privileges. Proceeding with tasks...");
    //std::cerr << "Running with administrator privileges. Proceeding with tasks..." << std::endl;


    return 0;
}

