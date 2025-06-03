/**
* -------------------------------------------------------
* Scenery Editor X
* -------------------------------------------------------
* Copyright (c) 2025 Thomas Ray 
* Copyright (c) 2025 Coalition of Freeware Developers
* -------------------------------------------------------
* dialog.h
* -------------------------------------------------------
* Created: 13/5/2025
* -------------------------------------------------------
*/
#pragma once
#include <map>
#include <string>
#include <vector>

/// -------------------------------------------------------

namespace CrashHandler
{
	/// GitHub integration details
	struct GitHubConfig
	{
	    std::string repoOwner;
	    std::string repoName;
	    std::string accessToken; /// Optional: for authentication
	};
	
	class CrashDialog
	{
	public:
	    CrashDialog();
	    ~CrashDialog();
	
	    bool showCrashDialog(const std::string &dumpPath);
	    void uploadCrashReport(const std::string &dumpPath, const std::string &userComments);
	
	    /// GitHub issue creation
	    bool createGitHubIssue(const std::string &title,
	                           const std::string &body,
	                           const std::vector<std::string> &labels,
	                           const std::vector<std::string> &attachmentPaths);
	
	    /// Setters for configuration
	    void setGitHubConfig(const GitHubConfig &config)
	    {
	        m_githubConfig = config;
	    }
	    void setCollectedLogs(const std::vector<std::string> &logs)
	    {
	        m_collectedLogs = logs;
	    }
	    void setProjectPath(const std::string &projectPath)
	    {
	        m_projectPath = projectPath;
	    }
	
	private:
	    /// Upload files to a file sharing service and get URLs
	    std::map<std::string, std::string> uploadFilesToFileService(const std::vector<std::string> &filePaths);
	
	    /// Parse crash dump for useful information
	    std::string extractInfoFromDump(const std::string &dumpPath);
	
	    /// Collect system information
	    std::string collectSystemInfo();
	
	    /// GitHub configuration
	    GitHubConfig m_githubConfig{.repoOwner = R"(Coalition-of-Freeware-Developers)",.repoName = "Scenery-Editor-X",
                                     .accessToken = ""};
	
	    /// Collected data
	    std::vector<std::string> m_collectedLogs;
	    std::string m_projectPath;
	};
	
} // namespace CrashHandler

/// -------------------------------------------------------
