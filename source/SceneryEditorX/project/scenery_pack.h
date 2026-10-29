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
 * scenery_pack.h
 * -------------------------------------------------------
 * Created: 19/10/2026
 * -------------------------------------------------------
 */
#pragma once
#include "scenery_project.h"
#include <filesystem>
#include <functional>
#include <string>
#include <string_view>
#include <vector>

// -------------------------------------------------------

namespace SceneryEditorX::ProjectFile
{
	/**
	 * A project as an X-Plane 12 scenery pack: creating a new airport project (the pack folder laid out like the others
	 * in Custom Scenery, the project file inside it, the way WorldEditor keeps earth.wed.xml in its pack) and exporting
	 * a project into a pack X-Plane loads:
	 *
	 *   <pack>/Earth nav data/apt.dat                  the project's airports
	 *   <pack>/Earth nav data/+40-090/+41-084.dsf      the overlay placements, one DSF per 1x1 degree tile
	 *   <pack>/objects, textures, ...                  the pack's own art, copied when exporting somewhere else
	 *   <pack>/library.txt                             library projects
	 *
	 * Coordinates are decimal degrees (north and east positive, south and west negative) with up to 8 decimals,
	 * headings true degrees. File work goes through the VFS (atomic writes, case-insensitive lookups). Standard library
	 * only, so creation and export are unit tested on every platform.
	 */

	inline constexpr const char *PACK_NAV_DATA = "Earth nav data";

	/* @brief The folders a new pack starts with: "Earth nav data", "objects", "textures". */
	const std::vector<std::string> &GetPackFolders();

	// -------------------------------------------------------
	// Coordinates

	/* @brief Decimal places the new-project form keeps (and apt.dat datum metadata carries). */
	inline constexpr int COORDINATE_DECIMALS = 8;

	/**
	 * @brief Reads a coordinate typed as decimal degrees: an optional sign, digits, and at most 8 decimals
	 * ("-50.50000000" is 50 degrees 30 minutes south). A trailing hemisphere letter works in place of the sign
	 * ("50.5 S", "122.31 W"). Latitudes must be within +-90, longitudes within +-180.
	 */
	bool ParseCoordinate(std::string_view text, bool latitude, double &value, std::string *error = nullptr);

	/* @brief A coordinate as decimal degrees with 8 decimals ("-50.50000000"). */
	std::string FormatCoordinate(double value);

	// -------------------------------------------------------
	// New airport projects

	/* @brief What the New Project form asks for. */
	struct NewAirportProject
	{
		std::string icao;						// Airport identifier (apt.dat row 1): 3 to 7 letters and digits
		std::string name;						// Airport name
		std::string country;					// 1302 country
		std::string city;						// 1302 city (optional)
		std::string state;						// 1302 state: state, province or county (optional)
		double latitude = 0.0;					// Airport reference point, decimal degrees, north positive
		double longitude = 0.0;					// East positive
		double elevationFeet = 0.0;				// Above mean sea level (apt.dat stores whole feet)
		XPlane::AirportType type = XPlane::AirportType::Land;
		bool starterLayout = true;				// Start with a runway (helipad for heliports), a windsock and a beacon
		std::string projectName;				// Pack folder and project file name; empty: "<ICAO> - <Name>"
		std::filesystem::path location;			// Folder the pack folder is created in (Custom Scenery by default)
		std::string author;
	};

	/* @brief A form field's problem; field is "icao", "name", "country", "city", "state", "latitude", "longitude", "elevation", "projectName" or "location". */
	struct FieldProblem
	{
		std::string field;
		std::string message;
	};

	/* @brief Highest airport elevation the form accepts, in feet (Everest is 29032). */
	inline constexpr double MAX_ELEVATION_FEET = 30000.0;
	/* @brief Lowest (the Dead Sea shore is about -1412 ft). */
	inline constexpr double MIN_ELEVATION_FEET = -1500.0;

	/* @brief Everything the form's values get wrong, field by field (empty: the project can be created). */
	std::vector<FieldProblem> ValidateNewAirportProject(const NewAirportProject &spec);

	/* @brief "<ICAO> - <Name>" made portable as a folder name. */
	std::string MakeDefaultProjectName(std::string_view icao, std::string_view name);

	/* @brief The pack folder the form creates: location / (project name or the default one). */
	std::filesystem::path GetNewProjectFolder(const NewAirportProject &spec);

	/**
	 * @brief The project the form describes: its info (name, author, pack name), the datum (reference point and
	 * elevation in meters) and the airport, with metadata city / country / datum_lat / datum_lon (8 decimals) /
	 * gui_label / icao_code / state in key order, placed at the top level of the hierarchy.
	 */
	SceneryProject MakeAirportProject(const NewAirportProject &spec);

	/**
	 * @brief Creates the pack folder (a folder that already has files in it is refused), its folders, the project file
	 * "<project name>.edX" inside it and the git helper files.
	 * @param projectFile Receives the project file's path.
	 */
	bool CreateAirportProject(const NewAirportProject &spec, std::filesystem::path &projectFile, std::string *error = nullptr);

	// -------------------------------------------------------
	// New scenery and library projects (no airport)

	/* @brief What the New Project form asks for a project that isn't tied to an airport. */
	struct NewSceneryProject
	{
		ProjectKind kind = ProjectKind::Scenery;	// Scenery (objects, polygons, facades... anywhere) or Library (library.txt exports)
		std::string name;							// Pack folder and project file name
		std::filesystem::path location;				// Folder the pack folder is created in
		std::string author;
		std::string description;
		bool hasReferencePoint = true;				// A datum the scene centers on (it can also come later, from what is placed)
		double latitude = 0.0;						// Decimal degrees, north positive
		double longitude = 0.0;						// East positive
		double elevationMeters = 0.0;
	};

	/* @brief Every problem with a NewSceneryProject's values: fields "name", "latitude", "longitude", "elevation", "location". */
	std::vector<FieldProblem> ValidateNewSceneryProject(const NewSceneryProject &spec);

	/* @brief The pack folder the form creates: location / name. */
	std::filesystem::path GetNewProjectFolder(const NewSceneryProject &spec);

	/* @brief The project: its info, the datum when it has a reference point, an empty library.txt for library projects. */
	SceneryProject MakeSceneryProject(const NewSceneryProject &spec);

	/**
	 * @brief Creates the pack folder (refused when it already holds files), its folders, "<name>.edX" and the git helper
	 * files.
	 * @param projectFile Receives the project file's path.
	 */
	bool CreateSceneryProject(const NewSceneryProject &spec, std::filesystem::path &projectFile, std::string *error = nullptr);

	// -------------------------------------------------------
	// Export

	struct ExportOptions
	{
		bool aptDat = true;						// Earth nav data/apt.dat (when the project has airports)
		bool dsf = true;						// Earth nav data/<tiles>.dsf (when the project places anything)
		bool copyAssets = true;					// Pack-relative art from the project folder (when exporting elsewhere), with the textures it names
		bool library = true;					// library.txt (library projects)
		bool wed = false;						// earth.wed.xml, for opening the pack in WorldEditor
		bool removeStaleTiles = false;			// Delete .dsf tiles in Earth nav data the project no longer writes
		bool includeHidden = false;				// Export hidden items too
		std::string creationAgent = "Scenery Editor X";
		std::vector<std::filesystem::path> protectedFolders;	// Never written into (X-Plane's Global Scenery, Resources)
		std::function<bool(std::string_view)> isLibraryPath;	// True for a virtual path a library exports (the editor's library index)
	};

	struct ExportReport
	{
		std::vector<std::filesystem::path> written;		// Files written or copied
		std::vector<std::filesystem::path> removed;		// Stale tiles removed
		std::vector<std::string> warnings;				// Left out or suspicious (nothing failed)
		std::vector<std::string> problems;				// ValidateProject's findings
		size_t airports = 0;
		size_t tiles = 0;
		size_t objects = 0;
		size_t polygons = 0;
		size_t assets = 0;							// Pack art files copied
		size_t libraryResources = 0;				// Resources taken from libraries (not copied)
	};

	/**
	 * @brief Writes a project as an X-Plane scenery pack into @p packFolder (usually the project's own folder).
	 * Problems ValidateProject finds are reported, not fatal; a write that fails stops the export (files already
	 * written stay, each one complete).
	 * @param projectFolder Where the project's pack-relative art lives (its file's folder).
	 */
	bool ExportSceneryPack(const SceneryProject &project, const std::filesystem::path &projectFolder, const std::filesystem::path &packFolder,
		const ExportOptions &options, ExportReport &report, std::string *error = nullptr);
}

// -------------------------------------------------------
