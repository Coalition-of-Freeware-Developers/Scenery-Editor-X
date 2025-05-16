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
#include <imgui/imgui.h>
#include <thread>

/// -------------------------------------------------------

namespace CrashHandler
{
    bool CrashDialog::showCrashDialog(const std::string &dumpPath)
    {
	    /// Initialize ImGui window
	    /// ...

	    bool dialogOpen = true;
	    bool sendReport = false;
	    char userComments[1024] = "";

	    while (dialogOpen)
		{
	        /// Start ImGui frame
	        /// ...

	        ImGui::Begin("Scenery Editor X - Application Crash", &dialogOpen, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse);

	        ImGui::TextWrapped("Scenery Editor X encountered a problem and needs to close.");
	        ImGui::TextWrapped("We apologize for the inconvenience. A crash report has been generated.");

	        ImGui::Separator();

	        ImGui::TextWrapped("Would you like to send this report to help improve the application?");
	        ImGui::TextWrapped("The report contains technical information about the crash.");

	        ImGui::InputTextMultiline("Additional comments (optional)", userComments, IM_ARRAYSIZE(userComments));

	        if (ImGui::Button("Send Report"))
			{
	            sendReport = true;
	            uploadCrashReport(dumpPath, userComments);
	            dialogOpen = false;
	        }

	        ImGui::SameLine();

	        if (ImGui::Button("Don't Send"))
			{
	            dialogOpen = false;
	        }

	        ImGui::Separator();

	        std::string dumpInfo = "Crash dump location: " + dumpPath;
	        ImGui::TextWrapped("%s", dumpInfo.c_str());

	        ImGui::End();

	        /// Render ImGui
	        /// ...
	    }

	    return sendReport;
	}

	void CrashDialog::uploadCrashReport(const std::string &dumpPath, const std::string &userComments)
    {
	    /// Use libcurl to upload the crash report to your server
	    /// ...
	}

} // namespace CrashHandler

/// -------------------------------------------------------
