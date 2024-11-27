#pragma once

#include <windows.h>
#include <shellapi.h>
#include "Logging.hpp"
#include "RegInit.h"
#include "ElevatePerms.h"

/**
 * @brief Checks if a specified registry key exists.
 *
 * This function attempts to open a registry key with read access. If the key
 * is successfully opened, it exists and the function returns true. Otherwise,
 * it returns false.
 *
 * @param hKeyRoot The root key under which the subkey is located.
 * @param subKey The name of the subkey to check.
 * @return true if the registry key exists, false otherwise.
 */
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

/**
 * @brief Checks and registers necessary registry keys.
 *
 * This function iterates through a list of registry keys and checks if they exist.
 * If a key does not exist and the application is running with administrative privileges,
 * it attempts to create the key by calling the associated registration function.
 * If the application is not running with administrative privileges, it logs an error
 * and attempts to relaunch the application with administrative privileges.
 */
void RegistryCheck()
{
    /**
    * @brief Structure to hold information about a registry key.
    *
    * This structure contains details about a registry key that needs to be checked
    * and potentially registered. It includes the root key, the subkey name, the function
    * to call if the key needs to be registered, and a human-readable name for logging purposes.
    */
    struct RegistryKeyInfo
    {
        HKEY hKeyRoot;          ///< The root key under which the subkey is located.
        const char *subKey;     ///< The name of the subkey to check.
        void (*registerFunc)(); ///< The function to call if the key needs to be registered.
        const char *keyName;    ///< A human-readable name for logging purposes.
    };

    /**
    * @brief List of registry keys to check and register if necessary.
    *
    * This array contains information about the registry keys that need to be checked.
    * Each entry includes the root key, the subkey name, the function to call if the key
    * needs to be registered, and a human-readable name for logging purposes.
    */
    RegistryKeyInfo registryKeys[] = {
        {HKEY_CLASSES_ROOT,     ".edx",                        RegisterEDXAssociation,        ".edx"},
        {HKEY_CLASSES_ROOT,     ".edx.lib",                    RegisterLibraryAssociation,    ".edx.lib"},
        {HKEY_LOCAL_MACHINE,    "SOFTWARE\\SceneryEditorX",    RegisterApplication,           "SceneryEditorX"},
        {HKEY_LOCAL_MACHINE,    "SOFTWARE\\SceneryEditorX",    RegisterAbsolutePath,          "SceneryEditorX"},
        {HKEY_LOCAL_MACHINE,    "SOFTWARE\\SceneryEditorX",    RegisterRelativePath,          "SceneryEditorX"}
    };

    /**
    * @brief Iterates through the list of registry keys and checks if they exist.
    *
    * For each registry key in the list, this loop checks if the key exists by calling
    * CheckRegistryKeyExists(). If the key does not exist and the application is running
    * with administrative privileges, it attempts to create the key by calling the associated
    * registration function. If the application is not running with administrative privileges,
    * it logs an error and attempts to relaunch the application with administrative privileges.
    */
    for (const auto &keyInfo : registryKeys)
    {
        if (!CheckRegistryKeyExists(keyInfo.hKeyRoot, keyInfo.subKey))
        {
            if (RunningAsAdmin())
            {
                spdlog::info("{} registry key not found. Creating...", keyInfo.keyName);
                keyInfo.registerFunc();
                // Verify if the key was created successfully
                if (CheckRegistryKeyExists(keyInfo.hKeyRoot, keyInfo.subKey))
                {
                    spdlog::info("{} registry key created successfully.", keyInfo.keyName);
                }
                else
                {
                    spdlog::error("Failed to create {} registry key.", keyInfo.keyName);
                }
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
