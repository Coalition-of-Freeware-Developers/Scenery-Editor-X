//Module:	XPLibrarySystem
//Author:	Connor Russell
//Date:		10/12/2024 2:32:10 PM
//Purpose:

#pragma once
#include "XPLibraryPath.h"
#include <filesystem>
#include <map>
#include <string>
#include <vector> 

namespace XPLibrary
{
	
	class VirtualFileSystem
	{
	private:
	    ///vPaths - A vector of VirtualPaths
	    std::vector<Definition> vctDefinitions;
	    std::map<std::string, Region> mRegions;
	
	public:
	    /**
	     * @brief LoadFileSystem - Loads the files from the Library.txt and real paths into the vPaths vector
		 *
		 * @param InXpRootPath = The root path of the X-Plane installation
		 * @param InCurrentPackagePath = A path to the current package. All files that exist here will be added as well.
		 * @param InCustomSceneryPacks = A vector of paths to custom scenery packs. These should be ordered based on the scenery_packs.ini, with the first element being the highest priority scenery
	     */
	    void LoadFileSystem(const std::filesystem::path &InXpRootPath,
                            const std::filesystem::path &InCurrentPackagePath, const std::vector<std::filesystem::path>
                            &InCustomSceneryPacks);
	
	    /**
	     * @brief GetDefinition - Returns the definition of a given path
		 *
		 * @param InPath = The path to get the definition of
		 * @returns The definition of the given path
	     */
	    Definition GetDefinition(const std::string &InPath);
	
	    /**
	     * @brief GetRegion - Returns the region of a given path
		 *
		 * @param InPath = The path to get the region of
		 * @returns Copy of the region of the given path. An empty region will be returned if the region does not exist
	     */
	    Region GetRegion(const std::string &InPath) const;
	};

}
