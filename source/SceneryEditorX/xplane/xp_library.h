/**
 * -------------------------------------------------------
 * Scenery Editor X
 * -------------------------------------------------------
 * Copyright (c) 2026 Thomas Ray
 * Copyright (c) 2026 Coalition of Freeware Developers
 * -------------------------------------------------------
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 * -------------------------------------------------------
 * xp_library.h
 * -------------------------------------------------------
 * Created: 17/10/2026
 * -------------------------------------------------------
 */
#pragma once
#include <array>
#include <cstdint>
#include <filesystem>
#include <functional>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

// -------------------------------------------------------

namespace SceneryEditorX::XPlane
{
	/**
	 * X-Plane library system: library.txt files (virtual path -> real file exports) and the scenery_packs.ini
	 * package order, per https://developer.x-plane.com/article/library-library-txt-file-format-specification/
	 *
	 * Standard library only, so the parsers and the index can be unit tested without the engine.
	 */

	/* @brief Art asset kinds, from the file extension of a virtual path. */
	enum class AssetType : uint8_t
	{
		Object,			// .obj
		Facade,			// .fac
		Forest,			// .for
		Line,			// .lin
		Polygon,		// .pol
		String,			// .str
		AutogenPoint,	// .agp
		AutogenBlock,	// .agb
		AutogenString,	// .ags
		Network,		// .net
		Terrain,		// .ter
		Beach,			// .bch
		Decal,			// .dcl
		Other,
		Count
	};

	/* @brief Asset type for a path's extension (case-insensitive). */
	AssetType GetAssetType(std::string_view path);

	/* @brief Display name for an asset type ("Object", "Facade", ...). */
	const char *GetAssetTypeName(AssetType type);

	/* @brief File extension for an asset type (".obj", ...; "" for Other). */
	const char *GetAssetTypeExtension(AssetType type);

	/* @brief Export flavors. Priority across packages: Export / Exclude block lower packages, Extend mixes, Backup is a last resort. */
	enum class ExportKind : uint8_t
	{
		Export,
		Extend,
		Exclude,
		Backup
	};

	const char *GetExportKindName(ExportKind kind);

	/* @brief Season bits for EXPORT_*_SEASON (0 = not seasonal, valid all year). */
	enum SeasonBits : uint8_t
	{
		SeasonSpring = 1 << 0,
		SeasonSummer = 1 << 1,
		SeasonFall   = 1 << 2,
		SeasonWinter = 1 << 3,
		SeasonAll    = SeasonSpring | SeasonSummer | SeasonFall | SeasonWinter
	};

	/* @brief "spr,sum" style list for a season mask ("all year" for 0). */
	std::string FormatSeasons(uint8_t seasons);

	/* @brief One EXPORT* line of a library.txt. */
	struct LibraryExport
	{
		std::string virtualPath;
		std::string realPath;			// As written, relative to the package folder
		ExportKind kind = ExportKind::Export;
		float ratio = 1.0f;				// EXPORT_RATIO weight
		uint8_t seasons = 0;			// SeasonBits, 0 = all year
		std::string region;				// Active REGION, empty = worldwide
		bool isPrivate = false;			// After PRIVATE (Laminar internal)
		bool deprecated = false;		// After DEPRECATED (hidden from WED, fails Gateway validation)
		bool semiDeprecated = false;	// After SEMI_DEPRECATED (hidden from WED, passes validation)
		uint32_t publicDate = 0;		// PUBLIC <yyyymmdd>, 0 when none
		uint32_t line = 0;
	};

	/* @brief A region (REGION_DEFINE + REGION_RECT / REGION_BITMAP / REGION_DREF / REGION_ALL). */
	struct LibraryRegion
	{
		struct Condition
		{
			std::string dataref;
			std::string comparison;
			std::string value;
		};

		std::string name;
		bool all = false;
		std::vector<std::array<int, 4>> rects;	// west, south, east, north (inclusive whole degrees)
		std::string bitmap;
		std::vector<Condition> conditions;

		// REGION_BITMAP: the image (resolved against its package by LibraryIndex::Build) and, once LoadRegionBitmaps ran,
		// one byte per pixel (non-zero inside) over the whole world, row 0 the northern edge (360 x 180: a pixel a tile)
		std::filesystem::path bitmapFile;
		std::vector<uint8_t> bitmapMask;
		uint32_t bitmapWidth = 0;
		uint32_t bitmapHeight = 0;

		/**
		 * @brief True when the 1x1 degree tile containing (lat, lon) is inside the region. A bitmap region whose mask
		 * isn't loaded counts as everywhere.
		 */
		[[nodiscard]] bool Contains(double latitude, double longitude) const;
	};

	/* @brief A parsed library.txt. */
	struct LibraryFile
	{
		uint32_t version = 0;
		std::vector<LibraryExport> exports;
		std::vector<LibraryRegion> regions;
		std::vector<std::string> warnings;
	};

	/**
	 * @brief Parses library.txt text. Real paths are the rest of the line, so they may contain spaces.
	 * @return False when the header isn't a LIBRARY header.
	 */
	bool ParseLibrary(std::string_view text, LibraryFile &out, std::string *error = nullptr);

	/* @brief Reads and parses a library.txt. */
	bool LoadLibraryFile(const std::filesystem::path &path, LibraryFile &out, std::string *error = nullptr);

	// -------------------------------------------------------

	/* @brief A SCENERY_PACK / SCENERY_PACK_DISABLED line of Custom Scenery/scenery_packs.ini. */
	struct SceneryPackEntry
	{
		std::string rawPath;					// As written ("Custom Scenery/Foo/", absolute paths are allowed)
		std::filesystem::path path;				// Resolved against the X-Plane folder
		bool enabled = true;
		bool isGlobalAirports = false;			// The *GLOBAL_AIRPORTS* placeholder
	};

	/**
	 * @brief Parses scenery_packs.ini. Entries come back in file order, which is X-Plane's priority order (first = highest).
	 * @param xplaneRoot X-Plane folder, for resolving relative entries.
	 */
	bool ParseSceneryPacks(std::string_view text, const std::filesystem::path &xplaneRoot, std::vector<SceneryPackEntry> &out, std::string *error = nullptr);

	// -------------------------------------------------------

	/* @brief Where a library package comes from. */
	enum class PackageSource : uint8_t
	{
		CustomScenery,	// Listed in scenery_packs.ini (or found in Custom Scenery)
		DefaultScenery,	// Resources/default scenery
		GlobalScenery,	// Global Scenery (Global Airports, ...)
		Tracked			// A folder the user added in the editor (work-in-progress libraries)
	};

	const char *GetPackageSourceName(PackageSource source);

	/* @brief A scenery package that may provide a library. */
	struct LibraryPackage
	{
		std::string name;					// Folder name
		std::filesystem::path path;			// Package folder
		PackageSource source = PackageSource::CustomScenery;
		bool enabledInSim = true;			// Not SCENERY_PACK_DISABLED
		bool hasLibrary = false;			// Has a library.txt
		bool tracked = true;				// Included in the editor's asset index
		int priority = 0;					// Lower = higher priority (custom scenery first, in ini order)

		// Filled in by LibraryIndex::Build
		uint32_t exportCount = 0;
		uint32_t assetCount = 0;			// Distinct virtual paths this package provides
		uint32_t missingFileCount = 0;		// Exports whose real file doesn't exist
		std::array<uint32_t, static_cast<size_t>(AssetType::Count)> typeCounts{};
		std::vector<std::string> warnings;
		std::string error;					// library.txt failed to parse
	};

	/* @brief One package's export of a virtual path. */
	struct AssetProvider
	{
		uint32_t package = 0;				// Index into LibraryIndex::GetPackages()
		ExportKind kind = ExportKind::Export;
		float ratio = 1.0f;
		uint8_t seasons = 0;
		std::string region;
		bool isPrivate = false;
		bool deprecated = false;
		bool semiDeprecated = false;
		std::filesystem::path realPath;		// Absolute path to the file
		bool fileExists = true;
	};

	/* @brief A virtual path and everything that exports it. */
	struct LibraryAsset
	{
		std::string virtualPath;
		AssetType type = AssetType::Other;
		std::vector<AssetProvider> providers;	// Sorted by package priority (highest first)
		int resolved = -1;						// Provider X-Plane would use with no region / season, -1 when none

		[[nodiscard]] bool IsPrivate() const;
		[[nodiscard]] bool IsDeprecated() const;
		[[nodiscard]] const AssetProvider *GetResolved() const { return resolved >= 0 ? &providers[resolved] : nullptr; }
	};

	/**
	 * @class LibraryIndex
	 * @brief Every virtual path exported by the tracked library packages, with X-Plane's priority resolution.
	 */
	class LibraryIndex
	{
	public:
		struct BuildOptions
		{
			bool checkFilesExist = true;	// Stat every real path (slow on cold disks; run off the main thread)
		};

		/**
		 * @brief Parses the library.txt of every tracked package and indexes the exports.
		 * @param packages Packages in priority order; their export counts and warnings are filled in.
		 */
		void Build(std::vector<LibraryPackage> packages, const BuildOptions &options);

		[[nodiscard]] const std::vector<LibraryPackage> &GetPackages() const { return m_Packages; }
		[[nodiscard]] const std::vector<LibraryAsset> &GetAssets() const { return m_Assets; }
		[[nodiscard]] const std::vector<LibraryRegion> &GetRegions() const { return m_Regions; }

		/* @brief The asset for a virtual path, or nullptr. */
		[[nodiscard]] const LibraryAsset *Find(std::string_view virtualPath) const;

		/* @brief The file X-Plane would load for a virtual path (no region / season), or an empty path. */
		[[nodiscard]] std::filesystem::path Resolve(std::string_view virtualPath) const;

		/* @brief Where and when a virtual path is resolved (Resolve with a context). */
		struct ResolveContext
		{
			bool hasLocation = false;		// Without one, regions don't narrow the choice (as Resolve() without a context)
			double latitude = 0.0;
			double longitude = 0.0;
			uint8_t season = 0;				// One SeasonBits value; 0: seasons don't narrow the choice
			uint64_t placementHash = 0;		// Picks among EXPORT_RATIO alternatives (0: the first)
		};

		/**
		 * @brief The export X-Plane would use at a place and season: the highest-priority package with an export whose
		 * REGION contains the place and whose seasons include the season (file order decides within a package, as the
		 * specific regions come before the catch-all one in X-Plane's libraries); EXPORT_RATIO alternatives in that
		 * package are picked by the placement hash weighted by ratio; an EXPORT_EXTEND-only package adds the next one's
		 * choices; EXPORT_BACKUP only fills gaps. Null when nothing applies.
		 */
		[[nodiscard]] const AssetProvider *ResolveProvider(std::string_view virtualPath, const ResolveContext &context) const;

		/* @brief ResolveProvider()'s file, or an empty path. */
		[[nodiscard]] std::filesystem::path Resolve(std::string_view virtualPath, const ResolveContext &context) const;

		/* @brief A region by name, or nullptr. */
		[[nodiscard]] const LibraryRegion *FindRegion(std::string_view name) const;

		/* @brief Decodes a REGION_BITMAP image: width, height and one byte per pixel (its red channel; non-zero inside). */
		using RegionBitmapLoader = std::function<bool(const std::filesystem::path &file, uint32_t &width, uint32_t &height, std::vector<uint8_t> &mask)>;

		/* @brief Loads the masks of bitmap regions, so they cover their pixels instead of the whole world. */
		void LoadRegionBitmaps(const RegionBitmapLoader &loader);

		[[nodiscard]] uint32_t GetExportCount() const { return m_ExportCount; }
		[[nodiscard]] uint32_t GetMissingFileCount() const { return m_MissingFileCount; }

	private:
		std::vector<LibraryPackage> m_Packages;
		std::vector<LibraryAsset> m_Assets;				// Sorted by virtual path
		std::vector<LibraryRegion> m_Regions;			// Region names are global across packages
		std::unordered_map<std::string, uint32_t> m_Lookup;
		uint32_t m_ExportCount = 0;
		uint32_t m_MissingFileCount = 0;
	};

}

// -------------------------------------------------------
