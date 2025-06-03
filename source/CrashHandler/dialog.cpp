/**
* -------------------------------------------------------
* Scenery Editor X | Crash Handler Service
* -------------------------------------------------------
* Copyright (c) 2025 Thomas Ray
* Copyright (c) 2025 Coalition of Freeware Developers
* -------------------------------------------------------
* dialog.cpp
* -------------------------------------------------------
* Created: 13/05/2025
* -------------------------------------------------------
*/
#include <CrashHandler/crash_handler.h>
#include <CrashHandler/dialog.h>
#include <curl/curl.h>
#include <filesystem>
#include <fstream>
#include <imgui/imgui.h>
#include <nlohmann/json.hpp>
#include <spdlog/spdlog.h>
#include <sstream>
#include <thread>
#include <Windows.h>

using json = nlohmann::json;

/// -------------------------------------------------------

/// CURL callback for writing responses
size_t WriteCallback(void *contents, size_t size, size_t nmemb, std::string *s)
{
    size_t newLength = size * nmemb;
    try
    {
        s->append((char *)contents, newLength);
        return newLength;
    }
    catch (std::bad_alloc &e)
    {
        return 0;
    }
}

/// -------------------------------------------------------

namespace CrashHandler
{
	bool CrashDialog::showCrashDialog(const std::string &dumpPath)
	{
	    /// Initialize ImGui window
	    /// Note: This is a placeholder for actual ImGui initialization
	
	    bool dialogOpen = true;
	    bool sendReport = false;
	    char userComments[1024] = "";
	
	    /// Extract crash information and system details
	    std::string systemInfo = collectSystemInfo();
	    std::string crashInfo = extractInfoFromDump(dumpPath);
	
	    /// Extract filename from dump path for display
	    std::string dumpFilename = std::filesystem::path(dumpPath).filename().string();
	
	    while (dialogOpen)
	    {
	        /// Start ImGui frame
	        /// Note: This is a placeholder for actual ImGui frame start
	
	        ImGui::Begin("Scenery Editor X - Application Crash",
	                     &dialogOpen,
	                     ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse);
	
	        ImGui::TextWrapped("Scenery Editor X encountered a problem and needs to close.");
	        ImGui::TextWrapped("We apologize for the inconvenience. A crash report has been generated.");
	
	        ImGui::Separator();
	
	        ImGui::TextWrapped("Would you like to send this report to help improve the application?");
	        ImGui::TextWrapped("The report contains technical information about the crash.");
	
	        ImGui::InputTextMultiline("Additional comments (optional)", userComments, IM_ARRAYSIZE(userComments));
	
	        /// Show collected data information
	        ImGui::Separator();
	        ImGui::Text("Crash dump: %s", dumpFilename.c_str());
	        ImGui::Text("Log files collected: %zu", m_collectedLogs.size());
	
	        if (!m_projectPath.empty())
	        {
	            ImGui::Text("Project: %s", std::filesystem::path(m_projectPath).filename().string().c_str());
	        }
	
	        /// Collapsible section for crash details
	        if (ImGui::CollapsingHeader("Crash Details"))
	        {
	            ImGui::TextWrapped("%s", crashInfo.c_str());
	            ImGui::TextWrapped("System Information:\n%s", systemInfo.c_str());
	
	            /// If we have logs, show a list of collected log files
	            if (!m_collectedLogs.empty())
	            {
	                if (ImGui::TreeNode("Collected Logs"))
	                {
	                    for (const auto &log : m_collectedLogs)
	                    {
	                        ImGui::BulletText("%s", std::filesystem::path(log).filename().string().c_str());
	                    }
	                    ImGui::TreePop();
	                }
	            }
	        }
	
	        /// GitHub integration explanation
	        ImGui::TextWrapped("Your crash report will be submitted as an issue to the GitHub repository:");
	        ImGui::TextWrapped("%s/%s", m_githubConfig.repoOwner.c_str(), m_githubConfig.repoName.c_str());
	
	        if (ImGui::Button("Send Report"))
	        {
	            sendReport = true;
	
	            /// Start a thread to upload the crash report
	            /// This avoids blocking the UI during upload
	            std::thread uploadThread(
	                [this, dumpPath, userComments]() { this->uploadCrashReport(dumpPath, userComments); });
	            uploadThread.detach();
	
	            dialogOpen = false;
	        }
	
	        ImGui::SameLine();
	
	        if (ImGui::Button("Don't Send"))
	        {
	            dialogOpen = false;
	        }
	
	        ImGui::End();
	
	        /// Render ImGui
	        /// Note: This is a placeholder for actual ImGui rendering
	    }
	
	    return sendReport;
	}
	
	void CrashDialog::uploadCrashReport(const std::string &dumpPath, const std::string &userComments)
	{
	    spdlog::info("Uploading crash report: {}", dumpPath);
	
	    /// First, prepare all files to upload
	    std::vector<std::string> filesToUpload;
	    filesToUpload.push_back(dumpPath);
	
	    /// Add collected logs
	    filesToUpload.insert(filesToUpload.end(), m_collectedLogs.begin(), m_collectedLogs.end());
	
	    /// Upload files to file sharing service
	    std::map<std::string, std::string> uploadedFiles = uploadFilesToFileService(filesToUpload);
	
	    /// Prepare issue title
	    std::string dumpFilename = std::filesystem::path(dumpPath).filename().string();
	    std::string issueTitle = "Crash Report: " + dumpFilename;
	
	    /// Prepare issue body
	    std::stringstream issueBody;
	    issueBody << "## Crash Report\n\n";
	
	    /// Add user comments if provided
	    if (!userComments.empty())
	    {
	        issueBody << "### User Comments\n" << userComments << "\n\n";
	    }
	
	    /// Add username of the person submitting
	    issueBody << "### Submitted by\n"
	              << "Username: The3dVehicleguy\n\n";
	
	    /// Add crash info
	    issueBody << "### Crash Information\n" << extractInfoFromDump(dumpPath) << "\n\n";
	
	    /// Add system info
	    issueBody << "### System Information\n" << collectSystemInfo() << "\n\n";
	
	    /// Add project info if available
	    if (!m_projectPath.empty())
	    {
	        issueBody << "### Project Information\n"
	                  << "Project: " << std::filesystem::path(m_projectPath).filename().string() << "\n\n";
	    }
	
	    /// Add links to uploaded files
	    issueBody << "### Attachments\n";
	    for (const auto &[filename, url] : uploadedFiles)
	    {
	        issueBody << "- [" << std::filesystem::path(filename).filename().string() << "](" << url << ")\n";
	    }
	
	    /// Add timestamp
	    issueBody << "\n\n_Report generated: 2025-06-03 02:58:41 UTC_\n";
	
	    /// Create the GitHub issue
	    std::vector<std::string> labels = {"crash", "bug", "automated-report"};
	    createGitHubIssue(issueTitle, issueBody.str(), labels, {});
	}
	
	std::map<std::string, std::string> CrashDialog::uploadFilesToFileService(const std::vector<std::string> &filePaths)
	{
	    std::map<std::string, std::string> results;
	
	    /// Initialize CURL
	    curl_global_init(CURL_GLOBAL_ALL);
	    CURL *curl = curl_easy_init();
	
	    if (!curl)
	    {
	        spdlog::error("Failed to initialize CURL");
	        return results;
	    }
	
	    for (const auto &filePath : filePaths)
	    {
	        /// Check if file exists
	        if (!std::filesystem::exists(filePath))
	        {
	            spdlog::error("File does not exist: {}", filePath);
	            continue;
	        }
	
	        /// Get file size
	        auto fileSize = std::filesystem::file_size(filePath);
	        if (fileSize == 0)
	        {
	            spdlog::warn("File is empty: {}", filePath);
	            continue;
	        }
	
	        /// In a real implementation, you would upload to a storage service
	        /// For demonstration, we'll create a simulated URL
	        /// In production, implement actual file upload logic here
	
	        std::string filename = std::filesystem::path(filePath).filename().string();
	        std::string uploadUrl = "https://storage.example.com/scenery-editor-x-crashes/" + filename;
	
	        /// Store the result
	        results[filePath] = uploadUrl;
	        spdlog::info("Uploaded file: {} -> {}", filePath, uploadUrl);
	    }
	
	    /// Clean up
	    curl_easy_cleanup(curl);
	    curl_global_cleanup();
	
	    return results;
	}
	
	std::string CrashDialog::extractInfoFromDump(const std::string &dumpPath)
	{
	    /// In a real implementation, you would use Windows DbgHelp to parse the minidump
	    /// and extract stack traces, exception information, etc.
	
	    std::stringstream info;
	    info << "Crash dump file: " << std::filesystem::path(dumpPath).filename().string() << "\n";
	
	    try
	    {
	        info << "Dump size: " << std::filesystem::file_size(dumpPath) << " bytes\n";
	        auto writeTime = std::filesystem::last_write_time(dumpPath);
	        auto systemTime = std::chrono::clock_cast<std::chrono::system_clock>(writeTime);
	        auto time_t_value = std::chrono::system_clock::to_time_t(systemTime);
	        info << "Created: " << std::ctime(&time_t_value);
	    }
	    catch (const std::exception &e)
	    {
	        info << "Error reading dump file properties: " << e.what() << "\n";
	    }
	
	    /// In a real implementation, you would use MiniDumpReadDumpStream to extract:
	    /// - Exception information (exception code, address, etc.)
	    /// - Module list (which DLLs were loaded)
	    /// - Thread information
	    /// - Stack trace of the crashing thread
	
	    return info.str();
	}
	
	std::string CrashDialog::collectSystemInfo()
	{
	    std::stringstream info;
	
	    /// Get OS version
	    OSVERSIONINFOEXA osvi;
	    ZeroMemory(&osvi, sizeof(OSVERSIONINFOEXA));
	    osvi.dwOSVersionInfoSize = sizeof(OSVERSIONINFOEXA);
	
	#pragma warning(disable : 4996)
	    GetVersionExA((OSVERSIONINFOA *)&osvi);
	#pragma warning(default : 4996)
	
	    info << "Operating System: Windows " << osvi.dwMajorVersion << "." << osvi.dwMinorVersion << " (Build "
	         << osvi.dwBuildNumber << ")\n";
	
	    /// Get system memory
	    MEMORYSTATUSEX memInfo;
	    memInfo.dwLength = sizeof(MEMORYSTATUSEX);
	    GlobalMemoryStatusEx(&memInfo);
	
	    DWORDLONG totalPhysMem = memInfo.ullTotalPhys;
	    DWORDLONG availPhysMem = memInfo.ullAvailPhys;
	
	    info << "Physical Memory: " << (totalPhysMem / (1024 * 1024)) << " MB Total, " << (availPhysMem / (1024 * 1024))
	         << " MB Available\n";
	
	    /// Get processor info
	    SYSTEM_INFO sysInfo;
	    GetSystemInfo(&sysInfo);
	
	    info << "Processor: " << sysInfo.dwNumberOfProcessors << " processor(s), Architecture: ";
	
	    switch (sysInfo.wProcessorArchitecture)
	    {
	    case PROCESSOR_ARCHITECTURE_AMD64:
	        info << "x64";
	        break;
	    case PROCESSOR_ARCHITECTURE_INTEL:
	        info << "x86";
	        break;
	    case PROCESSOR_ARCHITECTURE_ARM:
	        info << "ARM";
	        break;
	    default:
	        info << "Unknown";
	    }
	    info << "\n";
	
	    /// Get display info
	    HDC hdc = GetDC(NULL);
	    int screenWidth = GetDeviceCaps(hdc, HORZRES);
	    int screenHeight = GetDeviceCaps(hdc, VERTRES);
	    int colorDepth = GetDeviceCaps(hdc, BITSPIXEL);
	    ReleaseDC(NULL, hdc);
	
	    info << "Display: " << screenWidth << "x" << screenHeight << " at " << colorDepth << " bits per pixel\n";
	
	    /// Add application version info
	    /// In a real implementation, you would get this from your version resource
	    info << "Scenery Editor X Version: 1.0.0\n";
	
	    return info.str();
	}
	
	bool CrashDialog::createGitHubIssue(const std::string &title,
	                                    const std::string &body,
	                                    const std::vector<std::string> &labels,
	                                    const std::vector<std::string> &attachmentPaths)
	{
	    spdlog::info("Creating GitHub issue: {}", title);
	
	    /// Initialize CURL
	    curl_global_init(CURL_GLOBAL_ALL);
	    CURL *curl = curl_easy_init();
	
	    if (!curl)
	    {
	        spdlog::error("Failed to initialize CURL");
	        return false;
	    }
	
	    /// Prepare JSON payload
	    json issueJson;
	    issueJson["title"] = title;
	    issueJson["body"] = body;
	
	    if (!labels.empty())
	    {
	        issueJson["labels"] = labels;
	    }
	
	    std::string jsonPayload = issueJson.dump();
	
	    /// Set up GitHub API URL
	    std::string url =
	        "https://api.github.com/repos/" + m_githubConfig.repoOwner + "/" + m_githubConfig.repoName + "/issues";
	
	    /// Set up response buffer
	    std::string readBuffer;
	
	    /// Set up headers
	    struct curl_slist *headers = NULL;
	    headers = curl_slist_append(headers, "Content-Type: application/json");
	    headers = curl_slist_append(headers, "Accept: application/vnd.github.v3+json");
	
	    /// Add authorization if token is provided
	    if (!m_githubConfig.accessToken.empty())
	    {
	        std::string authHeader = "Authorization: token " + m_githubConfig.accessToken;
	        headers = curl_slist_append(headers, authHeader.c_str());
	    }
	
	    /// Set up CURL options
	    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
	    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, jsonPayload.c_str());
	    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
	    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
	    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &readBuffer);
	    curl_easy_setopt(curl, CURLOPT_USERAGENT, "SceneryEditorX-CrashReporter/1.0");
	
	    /// Perform the request
	    CURLcode res = curl_easy_perform(curl);
	
	    /// Check for errors
	    bool success = false;
	    if (res != CURLE_OK)
	    {
	        spdlog::error("CURL failed: {}", curl_easy_strerror(res));
	    }
	    else
	    {
	        long httpCode = 0;
	        curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &httpCode);
	
	        if (httpCode == 201)
	        {
	            spdlog::info("GitHub issue created successfully");
	            success = true;
	
	            /// Parse response to get issue URL
	            try
	            {
	                json responseJson = json::parse(readBuffer);
	                std::string issueUrl = responseJson["html_url"].get<std::string>();
	                spdlog::info("Issue URL: {}", issueUrl);
	
	                /// Open the issue URL in the default browser
	                ShellExecuteA(NULL, "open", issueUrl.c_str(), NULL, NULL, SW_SHOWNORMAL);
	            }
	            catch (const std::exception &e)
	            {
	                spdlog::error("Failed to parse GitHub response: {}", e.what());
	            }
	        }
	        else
	        {
	            spdlog::error("GitHub API returned code {}: {}", httpCode, readBuffer);
	        }
	    }
	
	    /// Clean up
	    curl_slist_free_all(headers);
	    curl_easy_cleanup(curl);
	    curl_global_cleanup();
	
	    return success;
	}
	
} // namespace CrashHandler

/// -------------------------------------------------------
