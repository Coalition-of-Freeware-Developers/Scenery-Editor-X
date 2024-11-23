#pragma once

#include <windows.h>
#include <shellapi.h>
#include "Logging.hpp"

static bool CheckRegistryKeyExists(HKEY hKeyRoot, const char* subKey)
{
    HKEY hKey;
    LONG result = RegOpenKeyEx(hKeyRoot, subKey, 0, KEY_READ, &hKey);
    if (result == ERROR_SUCCESS)
    {
        RegCloseKey(hKey);
        return true;
    }
    return false;
}

void RegistryCheck()
{
    struct RegistryKeyInfo
    {
        HKEY hKeyRoot;
        const char *subKey;
        void (*registerFunc)();
        const char *keyName;
    };

    RegistryKeyInfo registryKeys[] = {
        {HKEY_CLASSES_ROOT, ".edx", RegisterEDXAssociation, ".edx"},
        {HKEY_CLASSES_ROOT, ".edx.lib", RegisterLibraryAssociation, ".edx.lib"},
        {HKEY_LOCAL_MACHINE, "SOFTWARE\\SceneryEditorX", RegisterApplication, "SceneryEditorX"}
    };

    for (const auto &keyInfo : registryKeys)
    {
        if (!CheckRegistryKeyExists(keyInfo.hKeyRoot, keyInfo.subKey))
        {
            if (RunningAsAdmin)
            {
                spdlog::info("{} registry key not found. Creating...", keyInfo.keyName);
                keyInfo.registerFunc();
            }
            else
            {
                spdlog::error("{} registry key not found. Cannot create association without admin privileges.", keyInfo.keyName);
                RelaunchAsAdmin();
                return; // Exit the function to prevent further checks until admin privileges are obtained
            }
        }
        else
        {
            spdlog::info("{} registry key already exists.", keyInfo.keyName);
        }
    }
}
