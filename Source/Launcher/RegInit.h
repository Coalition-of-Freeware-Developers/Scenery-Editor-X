#pragma once

#include "atlbase.h"
#include "resource.h"
#include "Logging.hpp"

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

// Function to register the EDX file association
void RegisterEDXAssociation()
{
    spdlog::info("Registering EDX file association.");
    HKEY hKey;

    // Create .edx association
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

    // Define SceneryEditorX file type
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

    // Define open command
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

// Function to register the EDX Library file association
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

// Function to register SceneryEditorX application
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
