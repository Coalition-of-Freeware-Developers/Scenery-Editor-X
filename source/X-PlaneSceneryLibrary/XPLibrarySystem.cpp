//Module:	XPLibrarySystem
//Author:	Connor Russell
//Date:		10/12/2024 2:32:18 PM
//Purpose:	Implements XPLibrarySystem.h
#include "XPLibrarySystem.h"
#include "TextUtils.h"
#include <algorithm>
#include <fstream>
#include <map>
#include <ranges>
#include <sstream>


namespace fs = std::filesystem; //I'm lazy, so less typing

/**
* @brief LoadFileSystem - Loads the files from the Library.txt and real paths into the vPaths vector
*
* @param InXpRootPath = The root path of the X-Plane installation<
* @param InCurrentPackagePath
* @param InCustomSceneryPacks = A vector of paths to custom scenery packs. These should be ordered based on the scenery_packs.ini, with the first element being the highest priority scenery
*/
void XPLibrary::VirtualFileSystem::LoadFileSystem(const std::filesystem::path &InXpRootPath, const std::filesystem::path &InCurrentPackagePath, const std::vector<std::filesystem::path> &InCustomSceneryPacks)
{
    ///< Define our seasons
    const std::string SUM = "sum";
    const std::string WIN = "win";
    const std::string SPR = "spr";
    const std::string FAL = "fal";

    std::map<std::string, Definition> mTempDefinitions;

    ///< Define a list of acceptable extensions to add to the library.txt
    std::vector<std::string> vctXPExtensions = {
        ".lin",
        ".pol",
        ".str",
        ".ter",
        ".net",
        ".obj",
        ".agb",
        ".ags",
        ".agp",
        ".bch"
    };
    std::ranges::sort(vctXPExtensions);

    ///< We will first add a new region, region_all, which contains everything that is not regionalized.
    Region Region_All;
    mRegions.insert(std::make_pair("region_all", Region_All));

    ///< Lambda to get an iterator to a definition, or add it if it doesn't exist
    auto GetIteratorToDefinition = [&](const std::string &InPath) -> std::map<std::string, Definition>::iterator {
        ///< Find the definition
        auto it = mTempDefinitions.find(InPath);
        if (it == mTempDefinitions.end())
        {
            ///< Create the definition
            Definition Def;
            Def.pVirtual = InPath;
            mTempDefinitions.insert(std::make_pair(InPath, Def));

            it = mTempDefinitions.find(InPath);
        }

        return it;
    };

    //First load all the real files from the Current Package
    for (const auto &p : fs::recursive_directory_iterator(InCurrentPackagePath))
    {
        if (std::ranges::binary_search(vctXPExtensions, p.path().extension().string()))
        {
            //Define a new DefinitionPath
            DefinitionPath DefPath;
            DefPath.SetPath(InCurrentPackagePath, p.path().lexically_relative(InCurrentPackagePath));

            //Get iterator to this def
            auto it = GetIteratorToDefinition(p.path().lexically_relative(InCurrentPackagePath).string());
            it->second.vctRegionalDefs[it->second.GetRegionalDefinitionIdx("region_all")].dDefault.AddOption(DefPath);
        }
    }

    //Get a list of all library.txt files in the custom scenery packs, and in the resources directory (XpRoot/Resources/default scenery)
    //First path is the package, second path is the library.txt
    std::vector<std::pair<fs::path, fs::path>> vctLibs;

    //Get the library.txts to process
    {
        for (auto &p : InCustomSceneryPacks)
        {
            for (auto &p2 : fs::recursive_directory_iterator(p, fs::directory_options::skip_permission_denied))
            {
                if (p2.path().filename() == "library.txt")
                    vctLibs.emplace_back(p2.path().parent_path(), p2.path());
            }
        }

        for (auto &p : fs::recursive_directory_iterator(InXpRootPath / "Resources" / "default scenery",
                                                        fs::directory_options::skip_permission_denied))
        {
            if (p.path().filename() == "library.txt")
                vctLibs.emplace_back(p.path().parent_path(), p.path());
        }
    }

    //Now we will process the library.txt files
    for (auto &[fst, snd] : vctLibs)
    {
        //Open the file
        std::ifstream ifsLib(snd);

        //Buffers
        std::string strBuffer;
        std::stringstream ssLineBuffer;
        Region CurrentRegion;
        std::string strCurrentRegionDefName;
        std::string strCurrentRegionName = "region_all";
        bool bInPrivate = false;

        //State buffers for multi-line commands
        bool bThisCommandWasRegion = false;
        bool bLastCommandWasRegion = false;

        //Read lines
        while (ifsLib.good())
        {
            //Get the line, put it into the string stream, and tokenize
            std::getline(ifsLib, strBuffer);
            //std::replace(strBuffer.begin(), strBuffer.end(), '\t', ' ');	//Replace tabs with spaces so the string stream properly delimits
            ssLineBuffer.clear();
            ssLineBuffer.str(strBuffer);
            auto tokens = TextUtils::TokenizeString(strBuffer, {' ', '\t', '\n', '\r'});

            //Skip non-commands
            if (strBuffer.starts_with("#"))
                continue;
            //Comments
            if (tokens.empty())
                continue;
            //Empty lines

            //Reset this command state
            bThisCommandWasRegion = false;

            //Check the command
            if ((tokens[0] == "EXPORT" || tokens[0] == "EXPORT_EXTEND") && tokens.size() >= 3) //EXPORT and EXPORT_EXTEND actually behave pretty much identically in sim, so we will save the complexity and treat them the same here.
            {
                //Create (or get) the definition for the virtual path
                auto it = GetIteratorToDefinition(tokens[1]);
                if (bInPrivate)
                    it->second.bIsPrivate = true;

                //Set the private flag if we are in a private block

                //Get the index of the current RegionalDefinition in this definition
                auto &RegionalDef = it->second.vctRegionalDefs[it->second.GetRegionalDefinitionIdx(strCurrentRegionName)];

                //Now we need to get the real path. We do this by removing the first 2 tokens from the stream, then we can getline
                ssLineBuffer >> strBuffer >> strBuffer;
                strBuffer.clear();
                std::getline(ssLineBuffer, strBuffer);
                strBuffer = TextUtils::TrimWhitespace(strBuffer);

                //Define our definition path
                DefinitionPath DefPath;
                DefPath.SetPath(fst, strBuffer);

                //This is a default path, so now we just need to add it as an option to the default definition
                RegionalDef.dDefault.AddOption(DefPath);
            }
            if (tokens[0] == "EXPORT_BACKUP" && tokens.size() >= 3)
            {
                //Create (or get) the definition for the virtual path
                auto it = GetIteratorToDefinition(tokens[1]);
                if (bInPrivate)
                    it->second.bIsPrivate = true;
                //Set the private flag if we are in a private block

                //Get the index of the current RegionalDefinition in this definition
                auto &RegionalDef = it->second.vctRegionalDefs[it->second.GetRegionalDefinitionIdx(strCurrentRegionName)];

                //Now we need to get the real path. We do this by removing the first 2 tokens from the stream, then we can getline
                ssLineBuffer >> strBuffer >> strBuffer;
                strBuffer.clear();
                std::getline(ssLineBuffer, strBuffer);
                strBuffer = TextUtils::TrimWhitespace(strBuffer);

                //Define our definition path
                DefinitionPath DefPath;
                DefPath.SetPath(fst, strBuffer);

                //This is a backup path, so now we just need to add it as an option to the default definition
                RegionalDef.dBackup.AddOption(DefPath);
            }
            else if (tokens[0] == "EXPORT_RATIO" && tokens.size() >= 4)
            {
                //Format: EXPORT_RATIO <ratio> <virtual path> <real path>
                //Create (or get) the definition for the virtual path
                auto it = GetIteratorToDefinition(tokens[2]);
                if (bInPrivate)
                    it->second.bIsPrivate = true;
                //Set the private flag if we are in a private block

                //Get the index of the current RegionalDefinition in this definition
                auto &RegionalDef = it->second.vctRegionalDefs[it->second.GetRegionalDefinitionIdx(strCurrentRegionName)];

                //Now we need to get the real path. We do this by removing the first 3 tokens from the stream, then we can getline
                ssLineBuffer >> strBuffer >> strBuffer >> strBuffer;
                strBuffer.clear();
                std::getline(ssLineBuffer, strBuffer);
                strBuffer = TextUtils::TrimWhitespace(strBuffer);

                //Define our definition path
                DefinitionPath DefPath;
                DefPath.SetPath(fst, strBuffer);

                //Get the ratio
                double dblRatio = 1;
                try
                {
                    dblRatio = std::stod(tokens[2]);
                }
                catch (...)
                {
                    //TODO: Log something here
                }

                //This is a default path, so now we just need to add it as an option to the default definition
                RegionalDef.dBackup.AddOption(DefPath, dblRatio);
            }
            else if (tokens[0] == "EXPORT_EXCLUDE")
            {
                //Create (or get) the definition for the virtual path
                auto it = GetIteratorToDefinition(tokens[1]);
                if (bInPrivate)
                    it->second.bIsPrivate = true;
                //Set the private flag if we are in a private block

                //Get the index of the current RegionalDefinition in this definition
                auto &RegionalDef = it->second.vctRegionalDefs[it->second.GetRegionalDefinitionIdx(strCurrentRegionName)];

                //Now we need to get the real path. We do this by removing the first 2 tokens from the stream, then we can getline
                ssLineBuffer >> strBuffer >> strBuffer;
                strBuffer.clear();
                std::getline(ssLineBuffer, strBuffer);
                strBuffer = TextUtils::TrimWhitespace(strBuffer);

                //Define our definition path
                DefinitionPath DefPath;
                DefPath.SetPath(fst, strBuffer);

                //Since this is an exclude, we need to reset the options first
                RegionalDef.dDefault.ResetOptions();

                //This is a default path, so now we just need to add it as an option to the default definition
                RegionalDef.dDefault.AddOption(DefPath);
            }
            else if (tokens[0] == "REGION_DEFINE" && tokens.size() == 2)
            {
                CurrentRegion = Region(); //Reset the region
                strCurrentRegionDefName = fst.string() + ":" + tokens[1];

                bLastCommandWasRegion = true;
                bThisCommandWasRegion = true;
            }
            else if (tokens[0] == "REGION_ALL")
            {
                //Nothing to do here - we leave the region with the default no conditions

                bLastCommandWasRegion = true;
                bThisCommandWasRegion = true;
            }
            else if (tokens[0] == "REGION_RECT" && tokens.size() == 5)
            {
                //Params here are w s e n. Save these in the region
                try
                {
                    CurrentRegion.dblWest = std::stod(tokens[1]);
                    CurrentRegion.dblSouth = std::stod(tokens[2]);
                    CurrentRegion.dblEast = std::stod(tokens[3]);
                    CurrentRegion.dblNorth = std::stod(tokens[4]);
                }
                catch (...)
                {
                    //TODO: Log something here
                }

                bLastCommandWasRegion = true;
                bThisCommandWasRegion = true;
            }
            else if (tokens[0] == "REGION_BITMAP" && tokens.size() >= 2)
            {
                //TODO: Implement a system that allows for REGION_BITMAPs to be used. We need to store the image data.

                bLastCommandWasRegion = true;
                bThisCommandWasRegion = true;
            }
            else if (tokens[0] == "REGION_DREF" &&
                     tokens.size() == 4) //I don't *think* datarefs can have spaces? So there should be exactly 4 tokens
            {
                CurrentRegion.Conditions.emplace_back(tokens[1], tokens[2], tokens[3]); //The conditions are a tuple with 3 strings
                bLastCommandWasRegion = true;
                bThisCommandWasRegion = true;
            }
            else if (tokens[0] == "REGION" && tokens.size() == 2)
            {
                //Check if there is an un-added region (we can tell by the name not being empty). If so, add it
                if (!strCurrentRegionDefName.empty())
                {
                    //Save the region and reset the name
                    mRegions.insert(std::make_pair(strCurrentRegionDefName, CurrentRegion));
                    strCurrentRegionDefName = "";
                }

                //Set the current region
                strCurrentRegionName = fst.string() + ":" + tokens[1];
            }
            else if ((tokens[0] == "EXPORT_SEASON" || tokens[0] == "EXPORT_EXTEND_SEASON") && tokens.size() >= 4)
            {
                //Format: EXPORT_SEASON <seasons (comma-delimited)> <virtual path> <real path>
                //Create (or get) the definition for the virtual path
                auto it = GetIteratorToDefinition(tokens[2]);
                if (bInPrivate)
                    it->second.bIsPrivate = true;
                //Set the private flag if we are in a private block

                //Get the index of the current RegionalDefinition in this definition
                auto &RegionalDef = it->second.vctRegionalDefs[it->second.GetRegionalDefinitionIdx(strCurrentRegionName)];

                //Now we need to get the real path. We do this by removing the first 3 tokens from the stream, then we can getline
                ssLineBuffer >> strBuffer >> strBuffer >> strBuffer;
                strBuffer.clear();
                std::getline(ssLineBuffer, strBuffer);
                strBuffer = TextUtils::TrimWhitespace(strBuffer);

                //Define our definition path
                DefinitionPath DefPath;
                DefPath.SetPath(fst, strBuffer);

                //Add this path to the options for the appropriate seasons
                if (tokens[1].find(SUM) != std::string::npos)
                    RegionalDef.dSummer.AddOption(DefPath);
                if (tokens[1].find(WIN) != std::string::npos)
                    RegionalDef.dWinter.AddOption(DefPath);
                if (tokens[1].find(SPR) != std::string::npos)
                    RegionalDef.dSpring.AddOption(DefPath);
                if (tokens[1].find(FAL) != std::string::npos)
                    RegionalDef.dFall.AddOption(DefPath);
            }
            else if (tokens[0] == "EXPORT_RATIO_SEASON" && tokens.size() >= 5)
            {
                //Format: EXPORT_RATIO_SEASON <seasons (comma-delimited)> <ratio> <virtual path> <real path>
                //Create (or get) the definition for the virtual path
                auto it = GetIteratorToDefinition(tokens[3]);
                if (bInPrivate)
                    it->second.bIsPrivate = true;
                //Set the private flag if we are in a private block

                //Get the index of the current RegionalDefinition in this definition
                auto &RegionalDef = it->second.vctRegionalDefs[it->second.GetRegionalDefinitionIdx(strCurrentRegionName)];

                //Now we need to get the real path. We do this by removing the first 4 tokens from the stream, then we can getline
                ssLineBuffer >> strBuffer >> strBuffer >> strBuffer >> strBuffer;
                strBuffer.clear();
                std::getline(ssLineBuffer, strBuffer);
                strBuffer = TextUtils::TrimWhitespace(strBuffer);

                //Define our definition path
                DefinitionPath DefPath;
                DefPath.SetPath(fst, strBuffer);

                //Get the ratio
                try
                {
                    double dblRatio = 1;
                    dblRatio = std::stod(tokens[2]);
                }
                catch (...)
                {
                    //TODO: Log something here
                }

                //Add this path to the options for the appropriate seasons
                if (tokens[1].find(SUM) != std::string::npos)
                    RegionalDef.dSummer.AddOption(DefPath);
                if (tokens[1].find(WIN) != std::string::npos)
                    RegionalDef.dWinter.AddOption(DefPath);
                if (tokens[1].find(SPR) != std::string::npos)
                    RegionalDef.dSpring.AddOption(DefPath);
                if (tokens[1].find(FAL) != std::string::npos)
                    RegionalDef.dFall.AddOption(DefPath);
            }
            else if (tokens[0] == "EXPORT_EXCLUDE_SEASON" && tokens.size() >= 4)
            {
                //Format: EXPORT_EXCLUDE <seasons (comma-delimited)> <virtual path> <real path>
                //Create (or get) the definition for the virtual path
                auto it = GetIteratorToDefinition(tokens[2]);
                if (bInPrivate)
                    it->second.bIsPrivate = true;
                //Set the private flag if we are in a private block

                //Get the index of the current RegionalDefinition in this definition
                auto &RegionalDef = it->second.vctRegionalDefs[it->second.GetRegionalDefinitionIdx(strCurrentRegionName)];

                //Now we need to get the real path. We do this by removing the first 3 tokens from the stream, then we can getline
                ssLineBuffer >> strBuffer >> strBuffer >> strBuffer;
                strBuffer.clear();
                std::getline(ssLineBuffer, strBuffer);
                strBuffer = TextUtils::TrimWhitespace(strBuffer);

                //Define our definition path
                DefinitionPath DefPath;
                DefPath.SetPath(fst, strBuffer);

                //Since this is an exclude, we need to reset the options first
                RegionalDef.dDefault.ResetOptions();

                //Add this path to the options for the appropriate seasons
                if (tokens[1].find(SUM) != std::string::npos)
                    RegionalDef.dSummer.AddOption(DefPath);
                if (tokens[1].find(WIN) != std::string::npos)
                    RegionalDef.dWinter.AddOption(DefPath);
                if (tokens[1].find(SPR) != std::string::npos)
                    RegionalDef.dSpring.AddOption(DefPath);
                if (tokens[1].find(FAL) != std::string::npos)
                    RegionalDef.dFall.AddOption(DefPath);
            }
            else if (tokens[0] == "PUBLIC")
            {
                bInPrivate = false;
            }
            else if (tokens[0] == "PRIVATE")
            {
                bInPrivate = true;
            }

            //Handle multi-line commands

            //The end of a region command
            if (bLastCommandWasRegion && !bThisCommandWasRegion)
            {
                //Save the region and reset the name
                mRegions.insert(std::make_pair(strCurrentRegionDefName, CurrentRegion));
                strCurrentRegionDefName = "";
            }


            //Peek to set flags
            ifsLib.peek();
        }
    }

    //Add the temp definitions to the main definitions
    vctDefinitions.clear();
    vctDefinitions.reserve(mTempDefinitions.size());
    for (auto &val : mTempDefinitions | std::views::values)
    {
        vctDefinitions.push_back(val);
    }
}

/**
* @brief GetDefinition - Returns the definition of a given path
*
* @param InPath = The path to get the definition of
* @return The definition of the given path
*/
XPLibrary::Definition XPLibrary::VirtualFileSystem::GetDefinition(const std::string &InPath)
{
    //Find the definition
    if (const auto itDef = std::lower_bound(vctDefinitions.begin(), vctDefinitions.end(), InPath); itDef->pVirtual == InPath)
        return *itDef;

    //Return an empty definition
    return {};
}

/**
* @brief GetRegion - Returns the region of a given path
*
* @param InPath = The path to get the region of
* @return Copy of the region of the given path. An empty region will be returned if the region does not exist
*/
XPLibrary::Region XPLibrary::VirtualFileSystem::GetRegion(const std::string &InPath) const
{
    //Find the definition
    auto itDef = mRegions.at(InPath);

    //Return an empty region
    return {};
}
