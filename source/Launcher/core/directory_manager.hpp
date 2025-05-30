/**
* -------------------------------------------------------
* Scenery Editor X
* -------------------------------------------------------
* Copyright (c) 2025 Thomas Ray 
* Copyright (c) 2025 Coalition of Freeware Developers
* -------------------------------------------------------
* directory_manager.hpp
* -------------------------------------------------------
* Created: 22/3/2025
* -------------------------------------------------------
*/
#pragma once
#include <string>
#include <vector>

/// ---------------------------------------------------------

namespace Launcher
{

	/// ---------------------------------------------------------

	class DirectoryInit
	{
	public:

	    /**
	    * Checks the directories based on the provided command-line arguments.
	    *
	    * @param argc The number of command-line arguments.
	    * @param argv An array of command-line argument strings.
	    * @return An integer indicating the result of the directory check.
	    */
	    int DirectoryCheck(int argc, char *argv[]);
	
	    /**
	    * Ensures that all directories in the provided list exist.
	    * If a directory does not exist, it will be created.
	    *
	    * @param directories A vector of directory paths to check and create if necessary.
	    */
	    void ensureDirectoriesExist(const std::vector<std::string> &directories);

	protected:
        static std::string absolutePath; /// The absolute path to the executable
        static std::string relativePath; /// The relative path to the executable
	};

	/// ---------------------------------------------------------

} // namespace Launcher

/// ---------------------------------------------------------
