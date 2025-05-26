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


/// -------------------------------------------------------

namespace CrashHandler
{
	class CrashDialog
	{
	public:
	    bool showCrashDialog(const std::string &dumpPath);
        void uploadCrashReport(const std::string &dumpPath, const std::string &userComments);

	private:

	};

} // namespace CrashHandler

/// -------------------------------------------------------
