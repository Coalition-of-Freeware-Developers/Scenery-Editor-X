//Module:	XPLibraryPath
//Author:	Connor Russell
//Date:		10/12/2024 2:32:01 PM
//Purpose:	Provides abstractions for the X-Plane library system's paths and conditions

//Compile once
#pragma once

//Include necessary headers
#include <filesystem>
#include <map>
#include <string>
#include <tuple>
#include <vector>


namespace XPLibrary
{
static constexpr char SEASON_DEFAULT = 'd';
static constexpr char SEASON_SUMMER = 's';
static constexpr char SEASON_WINTER = 'w';
static constexpr char SEASON_FALL = 'f';
static constexpr char SEASON_SPRING = 'p';

/**
    * @brief DefinitionPaths are the individual paths that make up a definition.
    * They consist of their relative path, and their base path (base path being the parent folder of the library.txt, or the scenery package they are from).
    * Concat to get the absolute path
	*/
class DefinitionPath
{
public:
    //Package path (i.e. the package)
    std::filesystem::path pPackagePath;
    //Full real path
    std::filesystem::path pRealPath;
    //Relative path
    std::filesystem::path pPath;

    inline void SetPath(std::filesystem::path InBasePath, std::filesystem::path InRelativePath)
    {
        pPackagePath = InBasePath;
        pRealPath = InBasePath / InRelativePath;
        pPath = InRelativePath;
    }

    //Whether it's from a library. If false it's local to that package.
    bool bFromLibrary{false};
};

/**
    * @brief DefinitionOptions is a container that holds all objects that are valid under certain circumstances,
    * and would be randomly intermixed based on a weighted system.
	*/
class DefinitionOptions
{
    //The total ratio of all the options
    double dblTotalRatio{0};

    //The options for the definition
    std::vector<std::pair<double, XPLibrary::DefinitionPath>> vctOptions;

public:
    /**
        * @brief Adds an option to the definition
		*
		* @param InPath
		* @param InRatio
        */
    inline void AddOption(XPLibrary::DefinitionPath InPath, double InRatio = 1)
    {
        vctOptions.push_back(std::make_pair(InRatio, InPath));
        dblTotalRatio += InRatio;
    }

    /**
        * @brief Gets a random option based on the ratios
		*/
    inline XPLibrary::DefinitionPath GetRandomOption()
    {
        if (vctOptions.empty())
        {
            return XPLibrary::DefinitionPath();
        }

        double dblRand = (double)rand() / RAND_MAX * dblTotalRatio;

        for (auto &Option : vctOptions)
        {
            dblRand -= Option.first;
            if (dblRand <= 0)
            {
                return Option.second;
            }
        }

        return vctOptions[0].second;
    }

    /**
        * @brief Resets the options. Useful for EXPORT_EXCLUDE where you're overwriting every other option
		*/
    inline void ResetOptions()
    {
        vctOptions.clear();
        dblTotalRatio = 0;
    }

    /**
        * @brief Gets the number of options
		*/
    inline size_t GetOptionCount() const
    {
        return vctOptions.size();
    }

    /**
        * @brief Returns the options, along with their weights
		*/
    inline std::vector<std::pair<double, XPLibrary::DefinitionPath>> &GetOptions()
    {
        return vctOptions;
    }
};

/**
    * @brief The region parameters. These are referenced by the definitions, and are used to determine if an object is compatible with a region. They have their own data structure so they can be shared. Should be used in a map with the name being the key
	*/
class Region
{
public:
    // Conditions for the region. Conditions are a comparison between two values by an operator, values are stored as strings here, as is the operator. value1, operator, value2
    std::vector<std::tuple<std::string, std::string, std::string>> Conditions;

    //Region coord bounds
    double dblNorth{91}, dblSouth{-91}, dblEast{-181}, dblWest{181};

    /**
        * @brief Checks if the given latitude and longitude (and in the future, other conditions) are compatible with the region
		*/
    inline bool CompatibleWith(double InLat, double InLon) const
    {
        bool bIsCompatible = InLat < dblNorth && InLat > dblSouth && InLon > dblWest && InLon < dblEast;

        return bIsCompatible;
    }
};

/**
    * @brief RegionDefinitions are the highest level of organization in the X-Plane library system.
    * They contain conditions, and seasonal variants of objects.
	*/
class RegionalDefinitions
{
public:
    //The region name
    std::string strRegionName;

    XPLibrary::DefinitionOptions dSummer;
    XPLibrary::DefinitionOptions dWinter;
    XPLibrary::DefinitionOptions dFall;
    XPLibrary::DefinitionOptions dSpring;
    XPLibrary::DefinitionOptions dDefault;
    XPLibrary::DefinitionOptions dBackup;

    /**
        * @brief Returns the path for the given season. If the season is not found, the default path is returned.
		*/
    inline XPLibrary::DefinitionPath GetVersion(char InSeason)
    {
        switch (InSeason)
        {
        case 's':
        case 'd':
            return dSummer.GetRandomOption();
        case 'w':
            return dWinter.GetRandomOption();
        case 'f':
            return dFall.GetRandomOption();
        case 'p':
            return dSpring.GetRandomOption();
        }

        if (dDefault.GetOptionCount() != 0)
        {
            return dDefault.GetRandomOption();
        }

        return dBackup.GetRandomOption();
    }
};

class Definition
{
public:
    //The virtual path
    std::filesystem::path pVirtual;

    //Vector of all the regions. .begin() is the highest priority region.
    std::vector<RegionalDefinitions> vctRegionalDefs;

    //Whether this is a private asset. A single instance of it being private will make *all* variants private.
    bool bIsPrivate{false};

    /**
        * @brief Returns the path for the given season. If the season is not found, the default path is returned.
		*
		* @param InRegionDefinitions = The region definitions to use
		* @param Inlat = The latitude of the object
		* @param InLon = The longitude of the object
		* @param InSeason = Optional, the season to get this asset for
		* @returns The absolute asset path
        */
    inline std::filesystem::path GetPath(const std::map<std::string, XPLibrary::Region> &InRegionDefinitions,
                                         double Inlat,
                                         double InLon,
                                         char InSeason = XPLibrary::SEASON_DEFAULT)
    {
        if (vctRegionalDefs.empty())
        {
            return "";
        }

        for (auto &r : vctRegionalDefs)
        {
            //Get the region
            auto ThisRegion = InRegionDefinitions.find(r.strRegionName);

            if (ThisRegion->second.CompatibleWith(Inlat, InLon))
            {
                auto DefPath = r.GetVersion(InSeason);
                return DefPath.pRealPath;
            }
        }

        return "";
    }

    /**
        * @brief Gets the index for the RegionalDefinition. If it doesn't exist, it is added. Always returns a valid index
		*/
    inline size_t GetRegionalDefinitionIdx(std::string InRegionName)
    {
        //Attempt to find the region
        for (size_t i = 0; i < vctRegionalDefs.size(); i++)
        {
            if (vctRegionalDefs[i].strRegionName == InRegionName)
            {
                return i;
            }
        }

        //Add the region since it doesn't exist
        RegionalDefinitions NewRegion;
        NewRegion.strRegionName = InRegionName;
        vctRegionalDefs.push_back(NewRegion);

        return vctRegionalDefs.size() - 1;
    }

    //< operator for sorting
    inline bool operator<(const Definition &InOther) const
    {
        return pVirtual < InOther.pVirtual;
    }

    //String comparison operators for find
    inline bool operator==(const std::string &InOther) const
    {
        return pVirtual == InOther;
    }
    inline bool operator<=(const std::string &InOther) const
    {
        return pVirtual <= InOther;
    }
    inline bool operator>=(const std::string &InOther) const
    {
        return pVirtual >= InOther;
    }
    inline bool operator>(const std::string &InOther) const
    {
        return pVirtual > InOther;
    }
    inline bool operator<(const std::string &InOther) const
    {
        return pVirtual < InOther;
    }
    inline bool operator!=(const std::string &InOther) const
    {
        return pVirtual != InOther;
    }
};
} // namespace XPLibrary
