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
#include <imgui/imgui.h>
#include <iostream>
#include <Launcher/core/launcher.h>
#include <SceneryEditorX/core/application.h>
#include <SceneryEditorX/core/version.h>
#include <SceneryEditorX/core/window/window.h>
#include <SceneryEditorX/logging/logging.hpp>
#include <SceneryEditorX/platform/settings.h>
#include <SceneryEditorX/project/project.h>
#include <SceneryEditorX/renderer/render_context.h>
#include <SceneryEditorX/ui/ui.h>
#include <SceneryEditorX/ui/ui_context.h>

/// -------------------------------------------------------

namespace SceneryEditorX
{
	namespace UI
	{
	    class UIContextImpl;
	}
	
	/**
	 * -------------------------------------------------------
	 * PreLoader Global Variables
	 * -------------------------------------------------------
	 */
	
	//GLOBAL Scope<Window> g_Window;
	
	/**
	 * -------------------------------------------------------
	 * FORWARD FUNCTION DECLARATIONS
	 * -------------------------------------------------------
	 */

	
	/// -------------------------------------------------------
	
	Launcher::Launcher() = default;

	void Launcher::InitLauncher() 
	{
        /// Log header information immediately after init and flush to ensure it's written
        LAUNCHER_CORE_TRACE("Scenery Editor X Graphics Engine is starting...");

        GraphicsEngine::Init();

	}

    void Launcher::Run()
    {
        //const Ref<Window> &window = GetWindow();
        const auto start = std::chrono::high_resolution_clock::now();

        InitLauncher();

        const auto end = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();

        MainLoop();
    }

    void Launcher::Update() const
    {
        /// Update the viewport size if it has changed
        if (viewportData.viewportResized)
            return;
        //RecreateFrameResources();
    }

    void Launcher::DrawFrame()
    {

    }

    void Launcher::Create()
    {
        /// Initialize UI components
        ui.InitGUI();
    }

    void Launcher::MainLoop()
    {
        /// Perform the operations in separate threads
        //OperationThreads();

        while (true)
        {
			/**
            Sleep(1000);
            mtThreadCounter.lock();
            if (intThreadCount == 0)
            {
                mtThreadCounter.unlock();
                break;
            }
            mtThreadCounter.unlock();
			*/
            Update();
            DrawFrame();
        }

        vkDeviceWaitIdle(device);
    }
	
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
	size_t Launcher::WriteCallback(void *contents, const size_t size, const size_t nmemb, void *userp)
	{
	    static_cast<std::string *>(userp)->append(static_cast<char *>(contents), size * nmemb);
	    return size * nmemb;
	}
	
	bool Launcher::urlCheck()
	{
	    auto urlReachable = false;
	
	    curl_global_init(CURL_GLOBAL_DEFAULT);
	    if (CURL* curl = curl_easy_init())
	    {
	        curl_easy_setopt(curl, CURLOPT_URL, "https://api.github.com/The3dVehicleguy/Scenery-Editor-X/releases/latest");
	        curl_easy_setopt(curl, CURLOPT_NOBODY, 1L); // We only need to check if the URL is reachable
	        if (const CURLcode res = curl_easy_perform(curl); res == CURLE_OK)
                urlReachable = true;
            else
	        {
	            std::cerr << "Failed to reach the update URL: " << curl_easy_strerror(res) << '\n';
	            std::cout << "Do you want to skip the update? (yes/no): ";
	            std::string userInput;
	            std::cin >> userInput;
	            if (userInput == "yes")
                    skipUpdate();
                else
	            {
					LAUNCHER_CORE_TRACE("Exiting application.");
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
	void Launcher::UpdateCheck() const
    {
	    /// Check if the application is running in debug mode
	#ifdef SEDX_DEBUG
	    LAUNCHER_CORE_TRACE("Debug mode: Skipping update check. You are using the latest version.");
    #elif defined SEDX_RELEASE
	
	    /// Perform URL check before continuing
	    if (!urlCheck())
	    {
			LAUNCHER_CORE_TRACE("Update URL is not reachable. Skipping update check.");
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
                std::cerr << "curl_easy_perform() failed: " << curl_easy_strerror(res) << std::endl;
            else
	        {
	            /// Parse the JSON response to get the latest version
                if (auto versionPos = readBuffer.find("\"tag_name\":\""); versionPos != std::string::npos)
	            {
	                versionPos += 12; /// Move past the "tag_name":" part
                    if (auto endPos = readBuffer.find("\"", versionPos); endPos != std::string::npos)
                        latestVersion = readBuffer.substr(versionPos, endPos - versionPos);
                }
	        }
	        curl_easy_cleanup(curl);
	    }
	
	    curl_global_cleanup();
	
	    if (latestVersion.empty())
            LAUNCHER_CORE_ERROR("Failed to get the latest version from GitHub.");

        else if (currentVersion != latestVersion)
	    {
	        if (autoUpdate)
	        {
	            LAUNCHER_CORE_TRACE("Auto-update is enabled. Updating to version: {}", latestVersion);
	            startUpdate();
	        }
	        else
	        {
	            LAUNCHER_CORE_TRACE("Update available! ");
	            LAUNCHER_CORE_TRACE("Current version: ",currentVersion);
	            LAUNCHER_CORE_TRACE("Would you like to update ? (yes / no)");
	            std::string userInput;
	            std::cin >> userInput;
	            if (userInput == "yes")
                    startUpdate();
                else
                    skipUpdate();
            }
	    }
	
	    else
            LAUNCHER_CORE_TRACE("You are using the latest version: ", currentVersion);
    #endif
    }
	
	/**
	 * @brief Starts the update process.
	 *
	 * This function simulates the update process by displaying a message indicating that the update process has started.
	 * The actual update logic should be implemented here.
	 */
	void Launcher::startUpdate()
	{
	    /// Code to start the update process
		LAUNCHER_CORE_TRACE("Starting the update process...");
	    ///TODO: Implement the update logic here
	}
	
	void Launcher::skipUpdate()
	{
	    /// Code to bypass the update and continue with other functions
		LAUNCHER_CORE_TRACE("Bypassing the update. Continuing with other functions...");
	    ///TODO: Implement the logic to continue with other functions here
	}
	

	void Launcher::cleanInstall()
	{
        /// TODO: Implement the cleanInstall function
	}

}

/// ---------------------------------------------------------
