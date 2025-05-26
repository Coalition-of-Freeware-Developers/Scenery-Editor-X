/**
* -------------------------------------------------------
* Scenery Editor X
* -------------------------------------------------------
* Copyright (c) 2025 Thomas Ray 
* Copyright (c) 2025 Coalition of Freeware Developers
* -------------------------------------------------------
* reg_check.h
* -------------------------------------------------------
* Created: 16/3/2025
* -------------------------------------------------------
*/

#pragma once

#include <Launcher/core/elevate_perms.h>
#include <Launcher/registry/reg_init.h>

// -------------------------------------------------------

/**
 * @brief Checks if a specified registry key exists.
 *
 * This function attempts to open a registry key with read access. If the key
 * is successfully opened, it exists and the function returns true. Otherwise,
 * it returns false.
 *
 * @param hKeyRoot The root key under which the subkey is located.
 * @param subKey The name of the subkey to check.
 * @return True if the registry key exists, false otherwise.
 */
static bool CheckRegistryKeyExists(const HKEY hKeyRoot, const char* subKey)
{
    HKEY hKey;
    if (const LONG result = RegOpenKeyEx(hKeyRoot, subKey, 0, KEY_READ, &hKey); result == ERROR_SUCCESS)
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
inline void RegistryCheck()
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
        HKEY hKeyRoot;          // The root key under which the subkey is located.
        const char *subKey;     // The name of the subkey to check.
        void (*registerFunc)(); // The function to call if the key needs to be registered.
        const char *keyName;    // A human-readable name for logging purposes.
    };

    /**
    * @brief List of registry keys to check and register if necessary.
    *
    * This array contains information about the registry keys that need to be checked.
    * Each entry includes the root key, the subkey name, the function to call if the key
    * needs to be registered, and a human-readable name for logging purposes.
    */
    RegistryKeyInfo registryKeys[] = {
        {.hKeyRoot = HKEY_CLASSES_ROOT,.subKey = ".edx",.registerFunc = RegisterEDXAssociation, .keyName = ".edx"},
        {.hKeyRoot = HKEY_CLASSES_ROOT,.subKey = ".edx.lib",.registerFunc = RegisterLibraryAssociation, .keyName = ".edx.lib"},
        {.hKeyRoot = HKEY_LOCAL_MACHINE,.subKey = "SOFTWARE\\SceneryEditorX",.registerFunc = RegisterApplication, .keyName = "SceneryEditorX"},
        {.hKeyRoot = HKEY_LOCAL_MACHINE,.subKey = "SOFTWARE\\SceneryEditorX",.registerFunc = RegisterAbsolutePath, .keyName = "SceneryEditorX"},
        {.hKeyRoot = HKEY_LOCAL_MACHINE,.subKey = "SOFTWARE\\SceneryEditorX",.registerFunc = RegisterRelativePath, .keyName = "SceneryEditorX"}
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
    for (const auto &[hKeyRoot, subKey, registerFunc, keyName] : registryKeys)
    {
        if (!CheckRegistryKeyExists(hKeyRoot, subKey))
        {
            if (RunningAsAdmin())
            {
                LAUNCHER_LOG_WARN("{} registry key not found. Creating...", keyName);
                
                registerFunc();
                // Verify if the key was created successfully
                if (CheckRegistryKeyExists(hKeyRoot, subKey))
                {
					LAUNCHER_LOG_INFO("{} registry key created successfully.",keyName);
                    //spdlog::info("{} registry key created successfully.", keyInfo.keyName);
                }
                else
                {
					LAUNCHER_LOG_WARN("Failed to create {} registry key.",keyName);
                    //spdlog::error("Failed to create {} registry key.", keyInfo.keyName);
                }
            }
            else
            {
				LAUNCHER_LOG_ERROR("{} registry key not found. Cannot create association without admin privileges.",keyName);
				//ErrMsg("Cannot create association without admin privileges.");
                //spdlog::error("{} registry key not found. Cannot create association without admin privileges.", keyInfo.keyName);
                RelaunchAsAdmin();
                return; // Exit the function to prevent further checks until admin privileges are obtained
            }
        }
        else
        {
            LAUNCHER_LOG_INFO("{} registry key already exists.", keyName);
        }
    }
}
