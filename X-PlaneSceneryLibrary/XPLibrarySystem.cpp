//Module:	XPLibrarySystem
//Author:	Connor Russell
//Date:		10/12/2024 2:32:18 PM
//Purpose:	Implements XPLibrarySystem.h

//Compile once
#pragma once

//Include necessary headers
#include "XPLibrarySystem.h"

///	<summary>
///	LoadFileSystem - Loads the files from the Library.txt and real paths into the vPaths vector
///	</summary>
///	<param name="InXpRootPath">The root path of the X-Plane installation</param>
/// <param name="InCustomSceneryPacks">A vector of paths to custom scenery packs. These should be ordered based on the scenery_packs.ini, with the first element being the highest priority scenery</param>
void LoadFileSystem(std::filesystem::path InXpRootPath, std::vector<std::filesystem::path> InCustomSceneryPacks);
{
	/*
	//Define a vector of strings to hold the library.txt paths
	std::vector<std::filesystem::path> vctLibPaths;

	//Recursively scan all files using std::filesystem. Add all files to vPath, and also add all Library.txt (case insensitive) files to our library vector
	for (auto p : BasePaths)
	{
		for (const std::filesystem::path& p : recursive_directory_iterator(p))
		{
			//Check if the file is a Library.txt file
			if (p.filename().string() == "Library.txt" || p.filename().string() == "library.txt")
			{
				//Add the path to the library vector
				vctLibPaths.push_back(p);
			}
			else
			{
				//Add the path to the vPaths vector
				vPaths.push_back(VirtualPath(p.string()));
			}
		}
	}
	//Now sort the vPaths vector
	std::sort(vPaths.begin(), vPaths.end());

	//Define a binary tree to store our lib.txt paths. This allows us to rapidly find them without shifting data nonstop
	BinaryTree<VirtualPath> btLibTxtPaths;

	//Now we will read each Library.txt.
	//Library.txts contain commands in the form of EXPORT <vPath> <realPath>
	//vPaths may not have spaces, real paths can have spaces. So token 0 is export, token 1 is vPath, the rest of the line is realPath
	for (std::vector<std::filesystem::path>::reverse_iterator i = vctLibPaths.rbegin(); i != vctLibPaths.rend(); i++)
	{
		//Open the file
		std::ifstream file(i->wstring());

		//Check if the file is open
		if (file.is_open())
		{
			//Define a string to hold the current line
			std::string strLine;

			//Read each line
			while (std::getline(file, strLine))
			{
				//Define a vector of strings to hold the tokens
				stringstream sstrLine(strLine);

				//Define strings to hold tokens
				string strCmd;
				string strVPath;
				string strRealPath;

				//Read to first space
				sstrLine >> strCmd;

				//Just adds a new vPath entry, if one already exists, acts as EXPORT_EXTEND
				if (strCmd == "EXPORT")
				{
					//Read teh vPath
					sstrLine >> strVPath;

					//Read the real path
					getline(sstrLine, strRealPath);

					//Remove whitespace
					strRealPath = RemoveWhitespaceFromEnds(strRealPath);
					strVPath = RemoveWhitespaceFromEnds(strVPath);

					//Adjust the real path to include this library.txt's folder, plus the real path
					strRealPath = i->parent_path().string() + "\\" + strRealPath;

					//Now adjust the real and virtual paths to have all /s replaced with \\s
					strRealPath = ReplaceCharsInStr(strRealPath, '/', '\\');
					strVPath = ReplaceCharsInStr(strVPath, '/', '\\');

					//Try to find this vPath in the vPaths vector
					std::vector<VirtualPath>* vExistingPath = btLibTxtPaths.GetPointer(VirtualPath(strVPath));

					//If this path is present, add this real path. Otherwise insert it into the vector. We check if the index if < vPaths.size() cuz the index is unsigned, so we can't compare to -1
					if (vExistingPath != nullptr)
					{
						vExistingPath->at(0).AddRealPath(strRealPath);
					}
					else
					{
						btLibTxtPaths.Insert(VirtualPath(strVPath, strRealPath));
					}
				}

				//EXPORT_EXCLUDES override existing vPath entries
				else if (strCmd == "EXPORT_EXCLUDE")
				{
					//Read teh vPath
					sstrLine >> strVPath;

					//Read the real path
					getline(sstrLine, strRealPath);

					//Remove whitespace
					strRealPath = RemoveWhitespaceFromEnds(strRealPath);
					strVPath = RemoveWhitespaceFromEnds(strVPath);

					//Adjust the real path to include this library.txt's folder, plus the real path
					strRealPath = i->parent_path().string() + "\\" + strRealPath;

					//Now adjust the real and virtual paths to have all /s replaced with \\s
					strRealPath = ReplaceCharsInStr(strRealPath, '/', '\\');
					strVPath = ReplaceCharsInStr(strVPath, '/', '\\');

					//Try to find this vPath in the vPaths vector
					std::vector<VirtualPath>* vExistingPath = btLibTxtPaths.GetPointer(VirtualPath(strVPath));

					//If this path is present, add this real path. Otherwise insert it into the vector. We check if the index if < vPaths.size() cuz the index is unsigned, so we can't compare to -1
					if (vExistingPath != nullptr)
					{
						vExistingPath->at(0).ClearRealPaths();
						vExistingPath->at(0).AddRealPath(strRealPath);
					}
					else
					{
						btLibTxtPaths.Insert(VirtualPath(strVPath, strRealPath));
					}
				}

				//Creates a new vPath entry but with the real path provided here as a backup path. That backup path will only be used in resolving if the primary path is not found. Future EXPORT_BACKUPs will override
				else if (strCmd == "EXPORT_BACKUP")
				{
					//Read teh vPath
					sstrLine >> strVPath;

					//Read the real path
					getline(sstrLine, strRealPath);

					//Remove whitespace
					strRealPath = RemoveWhitespaceFromEnds(strRealPath);
					strVPath = RemoveWhitespaceFromEnds(strVPath);

					//Adjust the real path to include this library.txt's folder, plus the real path
					strRealPath = i->parent_path().string() + "\\" + strRealPath;

					//Now adjust the real and virtual paths to have all /s replaced with \\s
					strRealPath = ReplaceCharsInStr(strRealPath, '/', '\\');
					strVPath = ReplaceCharsInStr(strVPath, '/', '\\');

					//Try to find this vPath in the vPaths vector
					std::vector<VirtualPath>* vExistingPath = btLibTxtPaths.GetPointer(VirtualPath(strVPath));

					//If this path is present, add this real path. Otherwise insert it into the vector. We check if the index if < vPaths.size() cuz the index is unsigned, so we can't compare to -1
					if (vExistingPath != nullptr)
					{
						vExistingPath->at(0).AddBackupPath(strRealPath);
					}
					else
					{
						btLibTxtPaths.Insert(VirtualPath(strVPath, strRealPath));
						std::vector<VirtualPath>* vExistingPath = btLibTxtPaths.GetPointer(VirtualPath(strVPath));
						vExistingPath->at(0).ClearRealPaths();
						vExistingPath->at(0).AddBackupPath(strRealPath);
					}
				}

				//Same behavior as EXPORT, creates a new vPath entry, or extends it if it already exists
				else if (strCmd == "EXPORT_SEASON")
				{
					//Read the season part
					string strSeason;
					sstrLine >> strSeason;

					//Read teh vPath
					sstrLine >> strVPath;

					//Read the real path
					getline(sstrLine, strRealPath);

					//Remove whitespace
					strRealPath = RemoveWhitespaceFromEnds(strRealPath);
					strVPath = RemoveWhitespaceFromEnds(strVPath);

					//Adjust the real path to include this library.txt's folder, plus the real path
					strRealPath = i->parent_path().string() + "\\" + strRealPath;

					//Now adjust the real and virtual paths to have all /s replaced with \\s
					strRealPath = ReplaceCharsInStr(strRealPath, '/', '\\');
					strVPath = ReplaceCharsInStr(strVPath, '/', '\\');

					//Try to find this vPath in the vPaths vector
					std::vector<VirtualPath>* vExistingPath = btLibTxtPaths.GetPointer(VirtualPath(strVPath));

					//If this path is present, add this real path. Otherwise insert it into the vector. We check if the index if < vPaths.size() cuz the index is unsigned, so we can't compare to -1
					if (vExistingPath != nullptr && strSeason == "sum")
					{
						vExistingPath->at(0).ClearRealPaths();
						vExistingPath->at(0).AddRealPath(strRealPath);
					}
					else
					{
						btLibTxtPaths.Insert(VirtualPath(strVPath, strRealPath));
					}
				}

				//Same behavior as EXPORT, creates a new vPath entry, or extends it if it already exists
				else if (strCmd == "EXPORT_EXTEND")
				{
					//Read teh vPath
					sstrLine >> strVPath;

					//Read the real path
					getline(sstrLine, strRealPath);

					//Remove whitespace
					strRealPath = RemoveWhitespaceFromEnds(strRealPath);
					strVPath = RemoveWhitespaceFromEnds(strVPath);

					//Adjust the real path to include this library.txt's folder, plus the real path
					strRealPath = i->parent_path().string() + "\\" + strRealPath;

					//Now adjust the real and virtual paths to have all /s replaced with \\s
					strRealPath = ReplaceCharsInStr(strRealPath, '/', '\\');
					strVPath = ReplaceCharsInStr(strVPath, '/', '\\');

					//Try to find this vPath in the vPaths vector
					std::vector<VirtualPath>* vExistingPath = btLibTxtPaths.GetPointer(VirtualPath(strVPath));

					//If this path is present, add this real path. Otherwise insert it into the vector. We check if the index if < vPaths.size() cuz the index is unsigned, so we can't compare to -1
					if (vExistingPath != nullptr)
					{
						vExistingPath->at(0).AddRealPath(strRealPath);
					}
					else
					{
						btLibTxtPaths.Insert(VirtualPath(strVPath, strRealPath));
					}
				}
			}
		}
	}

	//Now get the data from the btree
	auto vctBtData = btLibTxtPaths.GetAllData();
	for (auto& v : vctBtData)
	{
		vPaths.push_back(v);
	}

	//Now sort the vPaths vector
	std::sort(vPaths.begin(), vPaths.end());
	*/
}
