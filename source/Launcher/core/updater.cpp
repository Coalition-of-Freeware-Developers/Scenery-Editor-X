/**
* -------------------------------------------------------
* Scenery Editor X
* -------------------------------------------------------
* Copyright (c) 2025 Thomas Ray 
* Copyright (c) 2025 Coalition of Freeware Developers
* -------------------------------------------------------
* updater.cpp
* -------------------------------------------------------
* Created: 16/3/2025
* -------------------------------------------------------
*/

#include <curl/curl.h>
#include <Launcher/core/updater.h>
#include <SceneryEditorX/core/version.h>

// -------------------------------------------------------

/**
 * @brief Callback function for handling data received from a cURL request.
 *
 * This function is called by cURL as soon as there is data received that needs to be saved.
 * The data is appended to the string provided by the user through the userp parameter.
 *
 * @param contents Pointer to the delivered data.
 * @param size Size of a single data element.
 * @param nmemb Number of data elements.
 * @param userp Pointer to the user-defined string where the data will be appended.
 * @return The number of bytes actually taken care of.
 */
size_t Updater::WriteCallback(void *contents, const size_t size, const size_t nmemb, void *userp)
{
    static_cast<std::string *>(userp)->append(static_cast<char *>(contents), size * nmemb);
    return size * nmemb;
}
bool Updater::urlCheck()
{
    auto urlReachable = false;

    curl_global_init(CURL_GLOBAL_DEFAULT);
    if (CURL* curl = curl_easy_init())
    {
        curl_easy_setopt(curl, CURLOPT_URL, "https://api.github.com/The3dVehicleguy/Scenery-Editor-X/releases/latest");
        curl_easy_setopt(curl, CURLOPT_NOBODY, 1L); // We only need to check if the URL is reachable
        if (const CURLcode res = curl_easy_perform(curl); res == CURLE_OK)
        {
            urlReachable = true;
        }
        else
        {
            std::cerr << "Failed to reach the update URL: " << curl_easy_strerror(res) << '\n';
            std::cout << "Do you want to skip the update? (yes/no): ";
            std::string userInput;
            std::cin >> userInput;
            if (userInput == "yes")
            {
                Updater::skipUpdate();
            }
            else
            {
				LAUNCHER_LOG_INFO("Exiting application.");
                //std::cerr << "Exiting application." << std::endl;
                exit(EXIT_FAILURE);
            }
        }
        curl_easy_cleanup(curl);
    }
    curl_global_cleanup();

    return urlReachable;
}

/**
 * @brief Checks for updates by comparing the current version with the latest version available on GitHub.
 *
 * This function uses cURL to fetch the latest release information from the GitHub API.
 * It then parses the JSON response to extract the latest version tag and compares it with the current version.
 * If an update is available, it prints a message indicating the new version. Otherwise, it confirms that the user is using the latest version.
 */
void Updater::UpdateCheck()
{

    // Check if the application is running in debug mode
#ifdef SEDX_DEBUG
    LAUNCHER_LOG_INFO("Debug mode: Skipping update check. You are using the latest version.");
    return;
#endif

    // Perform URL check before continuing
    if (!urlCheck())
    {
		LAUNCHER_LOG_INFO("Update URL is not reachable. Skipping update check.");
        //std::cerr << "Update URL is not reachable. Skipping update check." << std::endl;
        return;
    }  

    std::string currentVersion = SEDX_VERSION_STRING();
    std::string latestVersion;
    CURL *curl;
    std::string readBuffer;

    curl_global_init(CURL_GLOBAL_DEFAULT);
    curl = curl_easy_init();
    if (curl)
    {
        CURLcode res;
        curl_easy_setopt(curl, CURLOPT_URL, "https://api.github.com/The3dVehicleguy/Scenery-Editor-X/releases/latest");
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &readBuffer);
        res = curl_easy_perform(curl);
        if (res != CURLE_OK)
        {
            std::cerr << "curl_easy_perform() failed: " << curl_easy_strerror(res) << std::endl;
        }
        else
        {
            // Parse the JSON response to get the latest version
            auto versionPos = readBuffer.find("\"tag_name\":\"");
            if (versionPos != std::string::npos)
            {
                versionPos += 12; // Move past the "tag_name":" part
                auto endPos = readBuffer.find("\"", versionPos);
                if (endPos != std::string::npos)
                {
                    latestVersion = readBuffer.substr(versionPos, endPos - versionPos);
                }
            }
        }
        curl_easy_cleanup(curl);
    }

    curl_global_cleanup();

    if (latestVersion.empty())
    {
        LAUNCHER_LOG_ERROR("Failed to get the latest version from GitHub.");
    }

    else if (currentVersion != latestVersion)
    {
        if (autoUpdate)
        {
            LAUNCHER_LOG_INFO("Auto-update is enabled. Updating to version: {}", latestVersion);
            startUpdate();
        }
        else
        {
            LAUNCHER_LOG_INFO("Update available! ");
            LAUNCHER_LOG_INFO("Current version: ",currentVersion);
            LAUNCHER_LOG_INFO("Would you like to update ? (yes / no)");
            std::string userInput;
            std::cin >> userInput;
            if (userInput == "yes")
            {
                startUpdate();
            }
            else
            {
                skipUpdate();
            }
        }
    }

    else
    {
        LAUNCHER_LOG_INFO("You are using the latest version: ",currentVersion);
    }
}

/**
 * @brief Starts the update process.
 *
 * This function simulates the update process by displaying a message indicating that the update process has started.
 * The actual update logic should be implemented here.
 */

void Updater::startUpdate()
{
    // Code to start the update process
	LAUNCHER_LOG_INFO("Starting the update process...");
    //std::cout << "Starting the update process..." << std::endl;
    // Implement the update logic here
}

void Updater::skipUpdate()
{
    // Code to bypass the update and continue with other functions
	LAUNCHER_LOG_INFO("Bypassing the update. Continuing with other functions...");
    //std::cout << "Bypassing the update. Continuing with other functions..." << std::endl;
    // Implement the logic to continue with other functions here
}

// TODO: Implement the cleanInstall function
void Updater::cleanInstall()
{
    return;
}

