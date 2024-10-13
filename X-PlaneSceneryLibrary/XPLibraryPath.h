//Module:	XPLibraryPath
//Author:	Connor Russell
//Date:		10/12/2024 2:32:01 PM
//Purpose:	Provides abstractions for the X-Plane library system's paths and conditions

//Compile once
#pragma once

//Include necessary headers
#include <vector>
#include <string>
#include <tuple>
#include <filesystem>

namespace XPLibrary
{
	static constexpr char SEASON_DEFAULT = 'd';
	static constexpr char SEASON_SUMMER = 's';
	static constexpr char SEASON_WINTER = 'w';
	static constexpr char SEASON_FALL = 'f';
	static constexpr char SEASON_SPRING = 'p';

	/// <summary>
	/// DefinitionPaths are the individual paths that make up a definition. They consist of their relative path, and their base path (base path being the parent folder of the library.txt, or the scenery package they are from). Concat to get the absolute path
	/// </summary>
	class DefinitionPath
	{
	public:
		std::filesystem::path pBase;
		std::filesystem::path pPath;
	};

	/// <summary>
	/// DefinitionOptions is a container that holds all objects that are valid under certain circumstances, and would be randomly intermixed based on a weighted system.
	/// </summary>
	class DefinitionOptions
	{
		//The total ratio of all the options
		double dblTotalRatio{ 0 };

		//The options for the definition
		std::vector<std::pair<double, XPLibrary::DefinitionPath>> vctOptions;
	public:
		/// <summary>
		/// Adds an option to the definition
		/// </summary>
		/// <param name="InPath"></param>
		/// <param name="InRatio"></param>
		void AddOption(XPLibrary::DefinitionPath InPath, double InRatio = 1)
		{
			vctOptions.push_back(std::make_pair(InRatio, InPath));
			dblTotalRatio += InRatio;
		}

		/// <summary>
		/// Gets a random option based on the ratios
		/// </summary>
		/// <returns></returns>
		XPLibrary::DefinitionPath GetRandomOption()
		{
			double dblRand = (double)rand() / RAND_MAX * dblTotalRatio;

			for (auto& Option : vctOptions)
			{
				dblRand -= Option.first;
				if (dblRand <= 0)
				{
					return Option.second;
				}
			}

			return vctOptions[0].second;
		}
	};

	/// <summary>
	/// RegionDefinitions are the highest level of organization in the X-Plane library system. They contain conditions, and seasonalzied variants of objects.
	/// </summary>
	class DefinitionsForRegion
	{
	public:
		// Conditions for the region. Conditions are a comparison between two values by an operator, values are stored as strings here, as is the operator. value1, operator, value2
		std::vector<std::tuple<std::string, std::string, std::string>> Conditions;
		double dblNorth{ 91 }, dblSouth{ -91 }, dblEast{ -181 }, dblWest{ 181 };

		XPLibrary::DefinitionPath pSummer;
		XPLibrary::DefinitionPath pWinter;
		XPLibrary::DefinitionPath pFall;
		XPLibrary::DefinitionPath pSpring;
		XPLibrary::DefinitionPath pDefault;
		XPLibrary::DefinitionPath pBackup;

		/// <summary>
		/// Checks if the given latitude and longitude (and in the future, other conditions) are compatible with the region
		/// </summary>
		inline bool CompatibleWith(double InLat, double InLon)
		{
			bool bIsCompatible = InLat < dblNorth && InLat > dblSouth && InLon > dblWest && InLon < dblEast;

			return bIsCompatible;
		}

		/// <summary>
		/// Returns the path for the given season. If the season is not found, the default path is returned.
		/// </summary>
		inline XPLibrary::DefinitionPath GetVersion(char InSeason)
		{
			switch (InSeason)
			{
			case 's':
			case 'd':
				return pSummer;
			case 'w':
				return pWinter;
			case 'f':
				return pFall;
			case 'p':
				return pSpring;
			}

			if (!pDefault.pPath.empty())
			{
				return pDefault;
			}

			return pBackup;
		}
	};

	class Definition
	{
	public:
		//The virtual path
		std::filesystem::path pVirtual;

		//Vector of all the regions. .begin() is the highest priority region.
		std::vector<DefinitionsForRegion> vctRegionsDefs;
		size_t idxDefaultRegion = UINT64_MAX;

		/// <summary>
		/// Sets the default region for the definition. Otherwise first region is used.
		/// </summary>
		/// <param name="InRegionIdx">Index of the new default region. UINT64_MAX for no region.</param>
		inline void SetDefaultRegion(size_t InRegionIdx = UINT64_MAX)
		{
			idxDefaultRegion = InRegionIdx;
		}

		/// <summary>
		/// Returns the path for the given season. If the season is not found, the default path is returned.
		/// </summary>
		/// <param name="InSeason">Optional, the season to get this asset for</param>
		/// <returns>The absolute asset path</returns>
		inline std::filesystem::path GetPath(char InSeason = XPLibrary::SEASON_DEFAULT, double Inlat, double InLon)
		{
			if (vctRegionsDefs.empty())
			{
				return "";
			}

			if (idxDefaultRegion < vctRegionsDefs.size())
			{
				auto DefPath = vctRegionsDefs[idxDefaultRegion].GetVersion(InSeason);
				return DefPath.pBase / DefPath.pPath;
			}

			for (auto& r : vctRegionsDefs)
			{
				if (r.CompatibleWith(Inlat, InLon))
				{
					auto DefPath = r.GetVersion(InSeason);
					return DefPath.pBase / DefPath.pPath;
				}
			}

			return "";

		}
	};
}
