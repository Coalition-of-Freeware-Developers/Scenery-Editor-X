#pragma once

#include <version.h>
#include <iostream>
#include <string>
#include <curl/curl.h>

size_t WriteCallback(void* contents, size_t size, size_t nmemb, void* userp)
{
    ((std::string*)userp)->append((char*)contents, size * nmemb);
    return size * nmemb;
}

void UpdateCheck()
{
    std::string currentVersion = SEDX_GET_VERSION();
    std::string latestVersion;
    CURL* curl;
    CURLcode res;
    std::string readBuffer;

    curl_global_init(CURL_GLOBAL_DEFAULT);
    curl = curl_easy_init();
    if(curl) {
        curl_easy_setopt(curl, CURLOPT_URL, "https://api.github.com/repos/your-repo/your-project/releases/latest");
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &readBuffer);
        res = curl_easy_perform(curl);
        if(res != CURLE_OK) {
            std::cerr << "curl_easy_perform() failed: " << curl_easy_strerror(res) << std::endl;
        } else {
            // Parse the JSON response to get the latest version
            auto versionPos = readBuffer.find("\"tag_name\":\"");
            if (versionPos != std::string::npos) {
                versionPos += 12; // Move past the "tag_name":" part
                auto endPos = readBuffer.find("\"", versionPos);
                if (endPos != std::string::npos) {
                    latestVersion = readBuffer.substr(versionPos, endPos - versionPos);
                }
            }
        }
        curl_easy_cleanup(curl);
    }
    curl_global_cleanup();

    if (latestVersion.empty()) {
        std::cerr << "Failed to get the latest version from GitHub." << std::endl;
    } else if (currentVersion != latestVersion) {
        std::cout << "Update available: " << latestVersion << " (current version: " << currentVersion << ")" << std::endl;
    } else {
        std::cout << "You are using the latest version: " << currentVersion << std::endl;
    }
}
