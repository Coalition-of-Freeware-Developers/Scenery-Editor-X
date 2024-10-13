//Module:	XPLibrarySystem
//Author:	Connor Russell
//Date:		10/12/2024 2:32:10 PM
//Purpose:	

//Compile once
#pragma once

//Include necessary headers
#include <string>
#include <vector>
#include "XPLibraryPath.h"
#include <filesystem>

namespace XPLibrary
{

	class VirtualFileSystem
	{
	private:
		//vPaths - A vector of VirtualPaths
		std::vector<XPLibrary::Definition> vctDefinitions;
	public:
		///	<summary>
		///	LoadFileSystem - Loads the files from the Library.txt and real paths into the vPaths vector
		///	</summary>
		///	<param name="InXpRootPath">The root path of the X-Plane installation</param>
		/// <param name="InCustomSceneryPacks">A vector of paths to custom scenery packs. These should be ordered based on the scenery_packs.ini, with the first element being the highest priority scenery</param>
		void LoadFileSystem(std::filesystem::path InXpRootPath, std::vector<std::filesystem::path> InCustomSceneryPacks);

		///	<summary>
		///	GetDefinition - Returns the definition of a given path
		/// </summary>
		/// <param name="InPath">The path to get the definition of</param>
		/// <returns>The definition of the given path</returns>
		XPLibrary::Definition GetDefinition(const std::string& InPath);
	};


}
