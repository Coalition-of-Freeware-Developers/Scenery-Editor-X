#pragma once
#include <string>


class Updater
{
public:

    bool isUpdate   = false;    ///< Flag indicating whether an update is available.
    bool isLatest   = false;    ///< Flag indicating whether the user is using the latest version.
    bool isBeta     = false;    ///< Flag indicating whether the user is using a beta version.
    bool autoUpdate = false;    ///< Flag indicating whether the application will automatically update.
    std::string currentVersion; ///< The current version of the application.
    std::string latestVersion;  ///< The latest version available on GitHub.

    
    /**
     * @brief Checks for updates by comparing the current version with the latest version available on GitHub.
     *
     * This function uses cURL to fetch the latest release information from the GitHub API.
     * It then parses the JSON response to extract the latest version tag and compares it with the current version.
     * If an update is available, it prints a message indicating the new version. Otherwise, it confirms that the user is using the latest version.
     */
    void UpdateCheck();

    void cleanInstall();

private:

    bool urlCheck();

    void startUpdate();

    void skipUpdate();

    bool isFirstRun = false; ///< Flag indicating whether this is the first time the application is run.

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
    static size_t WriteCallback(void *contents, size_t size, size_t nmemb, void *userp);

};
