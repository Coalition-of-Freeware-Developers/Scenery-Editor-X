#pragma once

#include "atlbase.h"
#include "Logging.hpp"
#include "../launcher-resource.h"
#include <cstring>
#include <minwindef.h>
#include <spdlog/spdlog.h>
#include <winerror.h>
#include <winnt.h>
#include <winreg.h>

// Define an empty object map
//ATL::_ATL_OBJMAP_ENTRY ObjectMap[] = {NULL, NULL};

// Function to register the EDX file association from calling edXReg.rgs file
/*
void RegisterEDXAssociation()
{
    CComModule _Module; // ATL module object for handling registry operations
    HRESULT hr;

    // Initialize the module
    hr = _Module.Init(ObjectMap, GetModuleHandle(NULL));
    if (FAILED(hr))
    {
        MessageBox(NULL, "Failed to initialize the module for EDX project association", "Error", MB_OK | MB_ICONERROR);
        return;
    }

    // Update the registry using the .rgs file
    hr = _Module.UpdateRegistryFromResource(IDR_REGISTRY_EDX_FORMAT, TRUE);
    if (FAILED(hr))
    {
        MessageBox(NULL, "Failed to register EDX project file association", "Error", MB_OK | MB_ICONERROR);
    }

    // Clean up
    _Module.Term();
}
*/

// Function to register the EDX Library file association from calling edXLibReg.rgs file
/*
void RegisterLibraryAssociation()
{
    CComModule _Module; // ATL module object for handling registry operations
    HRESULT hr;

    // Initialize the module
    hr = _Module.Init(ObjectMap, GetModuleHandle(NULL));
    if (FAILED(hr))
    {
        MessageBox(NULL, "Failed to initialize the module for EDX library association", "Error", MB_OK | MB_ICONERROR);
        return;
    }

    // Update the registry using the .rgs file
    hr = _Module.UpdateRegistryFromResource(IDR_REGISTRY_LIBRARY_FORMAT, TRUE);
    if (FAILED(hr))
    {
        MessageBox(NULL, "Failed to register EDX Library file association", "Error", MB_OK | MB_ICONERROR);
    }

    // Clean up
    _Module.Term();
}
*/

/**
 * @brief Registers the EDX file association in the Windows registry.
 * 
 * This function creates registry keys and sets values to associate the ".edx" file extension 
 * with the "SceneryEditorX" application. It also defines the command to open these files 
 * with the SceneryEditorX executable.
 */
void RegisterEDXAssociation()
{
    spdlog::info("Registering EDX file association.");
    HKEY hKey; // Handle to the registry key

    /**
    * @brief Creates a registry key for the ".edx" file extension and associates it with "SceneryEditorX".
    * 
    * This function creates a registry key under HKEY_CLASSES_ROOT for the ".edx" file extension and sets its value 
    * to "SceneryEditorX". This allows the operating system to recognize files with the ".edx" extension and associate 
    * them with the SceneryEditorX application.
    * 
    * @return void
    */
    if (RegCreateKey(HKEY_CLASSES_ROOT, ".edx", &hKey) == ERROR_SUCCESS)
    {
        if (RegSetValue(hKey, nullptr, REG_SZ, "SceneryEditorX", static_cast<DWORD>(strlen("SceneryEditorX") + 1)) ==
            ERROR_SUCCESS)
        {
            spdlog::info("Associated .edx with SceneryEditorX.");
        }
        else
        {
            spdlog::error("Failed to set value for .edx.");
        }
        RegCloseKey(hKey);
    }
    else
    {
        spdlog::error("Failed to create key for .edx.");
        return;
    }

    /**
    * @brief Creates a registry key for the "SceneryEditorX" application and sets its description.
    * 
    * This function creates a registry key under HKEY_CLASSES_ROOT for the "SceneryEditorX" application 
    * and sets its value to "Scenery Editor X Project File". This allows the operating system to recognize 
    * the application and associate it with the specified description.
    * 
    * @return void
    */
    if (RegCreateKey(HKEY_CLASSES_ROOT, "SceneryEditorX", &hKey) == ERROR_SUCCESS)
    {
        const char *description = "Scenery Editor X Project File";
        if (RegSetValue(hKey, nullptr, REG_SZ, description, static_cast<DWORD>(strlen(description) + 1)) ==
            ERROR_SUCCESS)
        {
            spdlog::info("Registered SceneryEditorX file description.");
        }
        else
        {
            spdlog::error("Failed to set description for SceneryEditorX.");
        }
        RegCloseKey(hKey);
    }
    else
    {
        spdlog::error("Failed to create key for SceneryEditorX.");
        return;
    }

    /**
    * @brief Creates a registry key for the open command of SceneryEditorX.
    * 
    * This function creates a registry key under HKEY_CLASSES_ROOT for the open command of the 
    * SceneryEditorX application. It sets the command to open files with the SceneryEditorX executable.
    * 
    * @return void
    */
    if (RegCreateKey(HKEY_CLASSES_ROOT, "SceneryEditorX\\shell\\open\\command", &hKey) == ERROR_SUCCESS)
    {
        const char *command = "\"C:\\Program Files\\Scenery Editor X\\SceneryEditorX.exe\" \"%1\"";
        if (RegSetValue(hKey, nullptr, REG_SZ, command, static_cast<DWORD>(strlen(command) + 1)) == ERROR_SUCCESS)
        {
            spdlog::info("Registered open command for SceneryEditorX.");
        }
        else
        {
            spdlog::error("Failed to set open command for SceneryEditorX.");
        }
        RegCloseKey(hKey);
    }
    else
    {
        spdlog::error("Failed to create key for open command.");
    }
}

/**
 * @brief Registers the EDX Library file association in the Windows registry.
 * 
 * This function creates registry keys and sets values to associate the ".edx.lib" file extension 
 * with the "SceneryEditorXLib" application. It also defines the command to open these files 
 * with the SceneryEditorX executable.
 */
void RegisterLibraryAssociation()
{
    spdlog::info("Registering EDX Library file association.");
    HKEY hKey;
    if (RegCreateKey(HKEY_CLASSES_ROOT, ".edx.lib", &hKey) == ERROR_SUCCESS)
    {
        RegSetValue(hKey, nullptr, REG_SZ, "SceneryEditorXLib", 15);
        RegCloseKey(hKey);
    }
    if (RegCreateKey(HKEY_CLASSES_ROOT, "SceneryEditorXLib", &hKey) == ERROR_SUCCESS)
    {
        const char *libvalue = "Scenery Editor X Library";
        RegSetValue(hKey, nullptr, REG_SZ, libvalue, static_cast<DWORD>(strlen(libvalue)));
        RegCloseKey(hKey);
    }
    if (RegCreateKey(HKEY_CLASSES_ROOT, "SceneryEditorXLib\\shell\\open\\command", &hKey) == ERROR_SUCCESS)
    {
        RegSetValue(hKey, nullptr, REG_SZ, "SceneryEditorX.exe %1", 18);
        RegCloseKey(hKey);
    }
}

/**
 * @brief Registers the Scenery Editor X application in the Windows registry.
 * 
 * This function creates a registry key under "SOFTWARE\\Scenery Editor X" and sets the value 
 * to "Scenery Editor X". This allows the application to be recognized and referenced by its name.
 */
void RegisterApplication()
{
    spdlog::info("Registering Scenery Editor X");
    HKEY hKey;
    if (RegCreateKey(HKEY_LOCAL_MACHINE, "SOFTWARE\\Scenery Editor X", &hKey) == ERROR_SUCCESS)
    {
        const char *AppValue = "Scenery Editor X";
        RegSetValue(hKey, nullptr, REG_SZ, AppValue, static_cast<DWORD>(strlen(AppValue)));
        RegCloseKey(hKey);
    }
}

/**
 * @brief Registers the absolute path to the SceneryEditorX executable in the Windows registry.
 * 
 * This function creates a registry key under "SOFTWARE\\Scenery Editor X" and sets the value 
 * "AbsolutePath" to "C:\\Program Files\\Scenery Editor X\\SceneryEditorX.exe". This allows the 
 * application to be referenced by its absolute path.
 */
void RegisterAbsolutePath()
{
    spdlog::info("Registering Absolute Path");
    HKEY hKey;
    if (RegCreateKey(HKEY_LOCAL_MACHINE, "SOFTWARE\\Scenery Editor X", &hKey) == ERROR_SUCCESS)
    {
        const char *AbsPathValue = "C:\\Program Files\\Scenery Editor X\\SceneryEditorX.exe";
        RegSetValue(hKey, "AbsolutePath", REG_SZ, AbsPathValue, static_cast<DWORD>(strlen(AbsPathValue)));
        RegCloseKey(hKey);
    }
}

/**
 * @brief Registers the relative path to the SceneryEditorX executable in the Windows registry.
 * 
 * This function creates a registry key under "SOFTWARE\\Scenery Editor X" and sets the value 
 * "RelativePath" to "SceneryEditorX.exe". This allows the application to be referenced by its 
 * relative path.
 */
void RegisterRelativePath()
{
    spdlog::info("Registering Relative Path");
    HKEY hKey;
    if (RegCreateKey(HKEY_LOCAL_MACHINE, "SOFTWARE\\Scenery Editor X", &hKey) == ERROR_SUCCESS)
    {
        const char *RelPathValue = "SceneryEditorX.exe";
        RegSetValue(hKey, "RelativePath", REG_SZ, RelPathValue, static_cast<DWORD>(strlen(RelPathValue)));
        RegCloseKey(hKey);
    }
}
