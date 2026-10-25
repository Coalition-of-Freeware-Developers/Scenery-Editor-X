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
 * scenery_project.h
 * -------------------------------------------------------
 * Created: 18/10/2026
 * -------------------------------------------------------
 */
#pragma once
#include <SceneryEditorX/environment/environment_state.h>
#include <SceneryEditorX/xplane/xp_apt.h>
#include <SceneryEditorX/xplane/xp_library.h>
#include <cstdint>
#include <functional>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

// -------------------------------------------------------

namespace SceneryEditorX::ProjectFile
{
	/**
	 * A Scenery Editor X project: an X-Plane 12 scenery pack (airports and overlay placements) or a custom asset
	 * library (library.txt exports), held in memory the same way whichever file it came from:
	 *
	 *  - .edX  JSON text, deterministic and diff-friendly so projects live in git repositories (project_edx.h)
	 *  - .sex  binary serialization, checksummed chunks, the fast path and the edit cache's format (project_sex.h)
	 *  - earth.wed.xml  WorldEditor projects, converted both ways (project_wed.h)
	 *
	 * Positions are WGS84 latitude / longitude in decimal degrees (north and east positive), elevations meters,
	 * headings true degrees clockwise from north. Standard library only, so every format is unit tested without the engine.
	 */

	/* @brief Version of the project data model (both file formats carry it). */
	inline constexpr int FORMAT_VERSION = 1;

	inline constexpr const char *EDX_EXTENSION = ".edX";
	inline constexpr const char *SEX_EXTENSION = ".sex";
	inline constexpr const char *WED_FILE_NAME = "earth.wed.xml";

	/* @brief What the project is for. */
	enum class ProjectKind : uint8_t
	{
		Scenery,	// A scenery pack: airports, objects, polygons...
		Library		// A custom asset library: library.txt exports of the pack's art
	};

	const char *GetProjectKindName(ProjectKind kind);

	/* @brief The "Project" header. */
	struct ProjectInfo
	{
		std::string name;
		std::string uuid;					// Stable identity, set once when the project is created
		ProjectKind kind = ProjectKind::Scenery;
		std::string editorVersion;			// Editor that saved it last
		std::string xplaneVersion;			// X-Plane version the scenery targets ("12.1.4-r3")
		std::string created;				// ISO 8601 UTC ("2026-10-18T12:00:00Z")
		std::string modified;				// ISO 8601 UTC, updated only when a save changes the content
		std::string author;
		std::string description;
		std::string packageName;			// X-Plane scenery pack folder the project exports to
		int formatVersion = FORMAT_VERSION;
	};

	/* @brief The project's reference point (where the editor's local frame is centered). */
	struct GeoReference
	{
		double latitude = 0.0;
		double longitude = 0.0;
		double elevation = 0.0;				// Meters above mean sea level
		bool valid = false;
	};

	/* @brief A library the scenery uses (doc: "Libraries"); items point at it with "associated-library". */
	struct LibraryReference
	{
		std::string name;					// "Laminar Research Airport Scenery", "ZDP Library"
		std::string localPath;				// Its library.txt, relative to the X-Plane folder ("./Custom Scenery/ZDP_Library/library.txt")
		uint32_t entryCount = 0;			// Exports it had when the reference was recorded
		std::string uuid;
		std::string shortId;				// 8 hex digits, what items reference
	};

	/* @brief Draped item exclusion flags (DSF sim/exclude_*). */
	enum ExclusionBits : uint32_t
	{
		ExcludeObjects		= 1u << 0,
		ExcludeFacades		= 1u << 1,
		ExcludeForests		= 1u << 2,
		ExcludeBeaches		= 1u << 3,
		ExcludeRoads		= 1u << 4,
		ExcludeLines		= 1u << 5,
		ExcludePolygons		= 1u << 6,
		ExcludeStrings		= 1u << 7,
		ExcludeAll			= 0xFFu
	};

	/* @brief "objects", "facades", ... for each exclusion bit (in bit order). */
	const std::vector<std::pair<uint32_t, const char *>> &GetExclusionNames();

	/* @brief What a scene item is. */
	enum class ItemKind : uint8_t
	{
		Group,				// Folder in the hierarchy (WED_Group)
		Airport,			// Places a ProjectAirport in the hierarchy (its overlay children go below it)
		Object,				// .obj / .agp at a point
		Polygon,			// Draped .pol
		Orthophoto,			// Draped .pol with texture coordinates per node
		Line,				// .lin along a chain
		String,				// .str objects along a chain
		Forest,				// .for area, line or points
		Facade,				// .fac outline
		Autogen,			// .ags / .agb autogen string or block
		ExclusionZone,		// Rectangle (two corner nodes) excluding default scenery
		ExclusionPolygon,	// Polygon excluding default scenery
		Raw,				// WorldEditor objects this format doesn't model, kept verbatim for the round trip
		RoadNode,			// A junction of a road network (WED_RoadNode): where roads start, end and meet
		Road				// A .net road / railway / power line between two road nodes, along a bezier chain (WED_RoadEdge)
	};

	const char *GetItemKindName(ItemKind kind);

	/* @brief True for the kinds that are placed from an art asset (and carry "resource"). */
	bool HasResource(ItemKind kind);

	/* @brief True for the kinds made of node rings / chains. */
	bool HasNodes(ItemKind kind);

	/* @brief Object elevation: on the ground (draped), meters above sea level or above the ground. */
	enum class ElevationMode : uint8_t
	{
		Ground,
		Msl,
		Agl
	};

	/* @brief Forest fill: an area, a line of trees, one tree per node. */
	enum class ForestFill : uint8_t
	{
		Area,
		Line,
		Points
	};

	/**
	 * @brief A node of an item's ring or chain. Bezier handles are offsets in degrees from the node, as WorldEditor
	 * stores them: `lo` points back toward the previous node, `hi` toward the next. Unsplit nodes keep lo = -hi.
	 */
	struct GeoNode
	{
		double latitude = 0.0;
		double longitude = 0.0;
		double loLatitude = 0.0;
		double loLongitude = 0.0;
		double hiLatitude = 0.0;
		double hiLongitude = 0.0;
		bool split = false;					// The two handles are independent
		bool hasUv = false;					// Orthophoto texture coordinates
		float s = 0.0f, t = 0.0f;
		float sLo = 0.0f, tLo = 0.0f, sHi = 0.0f, tHi = 0.0f;
		int wallType = 0;					// Facade wall (0-based) of the segment starting here
		bool spawnTiles = true;				// Autogen node: tiles spawn along the segment starting here

		[[nodiscard]] bool HasLo() const { return loLatitude != 0.0 || loLongitude != 0.0; }
		[[nodiscard]] bool HasHi() const { return hiLatitude != 0.0 || hiLongitude != 0.0; }
		[[nodiscard]] bool HasBezier() const { return HasLo() || HasHi(); }

		bool operator==(const GeoNode &) const = default;
	};

	using GeoRing = std::vector<GeoNode>;

	/* @brief Free-form key / value data ("other-properties"). Values are JSON text, so any JSON value survives. */
	using PropertyList = std::vector<std::pair<std::string, std::string>>;

	/**
	 * @brief One entry of the scene hierarchy. One struct for every kind keeps the formats simple; each kind only
	 * reads and writes the fields that apply to it (see project_archive.h).
	 */
	struct SceneryItem
	{
		ItemKind kind = ItemKind::Object;
		std::string id;						// Unique and stable across saves (git diffs, references)
		std::string name;
		bool locked = false;
		bool hidden = false;

		// Art assets
		std::string resource;				// Library virtual path or a path relative to the pack ("lib/airport/...", "objects/hangar.obj")
		std::string library;				// LibraryReference::shortId providing it, empty when unknown

		// Point placements (Object)
		double latitude = 0.0;
		double longitude = 0.0;
		double altitude = 0.0;				// Meters, meaning set by elevationMode
		float heading = 0.0f;
		ElevationMode elevationMode = ElevationMode::Ground;
		int showLevel = 1;					// 1..6, X-Plane's object density setting it needs

		// Shapes: rings[0] is the outline (or the chain of a line / string, or the two exclusion zone corners), the rest holes
		std::vector<GeoRing> rings;
		bool closed = false;				// Lines and strings
		float height = 10.0f;				// Facade / autogen height (meters)
		float density = 1.0f;				// Forest density (0..1)
		ForestFill fill = ForestFill::Area;
		float spacing = 10.0f;				// String spacing (meters)
		float textureHeading = 0.0f;		// Polygon / orthophoto texture heading
		bool pickWalls = false;				// Facade: per-segment wall choice
		int spelling = 0;					// Autogen spelling
		std::string decal;					// Orthophoto decal
		float texTop = 1.0f, texBottom = 0.0f, texLeft = 0.0f, texRight = 1.0f;	// Orthophoto texture window (WorldEditor's)
		uint32_t exclusions = 0;			// ExclusionBits

		std::string airportId;				// Airport: ProjectAirport::airport.id
		std::vector<std::string> raw;		// Raw: the WorldEditor <object> elements of the subtree, verbatim

		// Roads (rings[0] runs from the start node's position to the end node's; RoadNode items use latitude / longitude)
		int roadSubtype = 1;				// Road type of the .net (its virtual types: "hwy_6/city/wet"...), 0..255
		std::string startNode;				// Ids of the RoadNode items the road starts and ends at
		std::string endNode;
		int startLayer = 0;					// Height level at each end: 0 on the ground, 1 and 2 raised (bridges)
		int endLayer = 0;

		PropertyList properties;			// "other-properties"
		std::vector<SceneryItem> children;	// Group and Airport

		[[nodiscard]] bool IsContainer() const { return kind == ItemKind::Group || kind == ItemKind::Airport; }
	};

	/* @brief An airport of the project: its apt.dat rows plus what WorldEditor keeps beside them. */
	struct ProjectAirport
	{
		XPlane::AptAirport airport;
		bool hasAtc = true;					// Header flag WorldEditor writes (X-Plane 9 era, still round-tripped)
		int sceneryId = -1;					// Scenery Gateway submission it came from, -1 when none
		PropertyList properties;			// "other-properties" (for example "MagVar")
	};

	/**
	 * @brief A named filter of the editor's X-Plane library browser, saved with the project so a team shares its views
	 * of the libraries (sidewalks, this airport's hangars, broken exports...).
	 */
	struct LibraryFilter
	{
		std::string name;
		std::string search;					// The search box's text (ImGuiTextFilter syntax: words, -excluded)
		std::vector<std::string> types;		// Asset types by file extension ("obj", "fac", "lin"...); empty: every type
		std::vector<std::string> libraries;	// Library packages by folder name; empty: every library
		bool showHidden = false;			// Private and deprecated assets too
		bool onlyMissing = false;			// Only assets whose file is missing

		bool operator==(const LibraryFilter &) const = default;
	};

	/* @brief A whole project. */
	struct SceneryProject
	{
		ProjectInfo info;
		GeoReference datum;
		std::vector<LibraryReference> libraries;
		std::vector<ProjectAirport> airports;
		std::vector<SceneryItem> items;					// The scene hierarchy ("Assets"); top level = WorldEditor's world
		std::optional<XPlane::LibraryFile> library;		// Library projects: the library.txt being authored
		std::optional<Environment::EnvironmentState> environment;	// The world the project was last viewed in (date, weather...)
		PropertyList wedPreferences;					// WorldEditor <prefs>, kept for the round trip
		PropertyList properties;						// "other-properties"
		std::vector<LibraryFilter> libraryFilters;		// The library browser's saved filters (not in WorldEditor files)

		[[nodiscard]] ProjectAirport *FindAirport(std::string_view id);
		[[nodiscard]] const ProjectAirport *FindAirport(std::string_view id) const;

		/* @brief Depth-first search of the hierarchy. */
		[[nodiscard]] SceneryItem *FindItem(std::string_view id);
		[[nodiscard]] const SceneryItem *FindItem(std::string_view id) const;

		/* @brief The children list holding an item (top level or a container's), and the item's position in it. */
		[[nodiscard]] std::vector<SceneryItem> *FindParentList(std::string_view id, size_t *index = nullptr);

		/* @brief Visits every item depth-first (parents before children). Return false to stop. */
		void ForEachItem(const std::function<bool(SceneryItem &item, int depth)> &visitor);
		void ForEachItem(const std::function<bool(const SceneryItem &item, int depth)> &visitor) const;

		[[nodiscard]] size_t CountItems() const;
		[[nodiscard]] size_t CountItems(ItemKind kind) const;

		/* @brief A LibraryReference by its short id, or nullptr. */
		[[nodiscard]] const LibraryReference *FindLibrary(std::string_view shortId) const;
	};

	// -------------------------------------------------------

	/* @brief A random identifier for an item: 12 lower-case hex digits ("3f9a0c41d2b7"). */
	std::string MakeItemId();

	/* @brief A random RFC 4122 version 4 UUID ("15da3863-07d1-462a-be65-3873058675f3"). */
	std::string MakeUuid();

	/* @brief Short id for a library name: 8 hex digits of its FNV-1a hash, so every machine derives the same one. */
	std::string MakeLibraryShortId(std::string_view libraryName);

	/* @brief Current UTC time, ISO 8601 with seconds ("2026-10-18T12:00:00Z"). */
	std::string GetUtcTimestamp();

	/* @brief FNV-1a 64-bit hash, for cache keys and change detection. */
	uint64_t HashFnv1a64(std::string_view bytes, uint64_t seed = 0xcbf29ce484222325ull);

	/* @brief CRC-32 (IEEE 802.3, as zlib computes it). */
	uint32_t Crc32(const void *data, size_t size, uint32_t crc = 0);

	/**
	 * @brief A new, empty project with an identity and timestamps.
	 * @param name Project name (also the default scenery pack name).
	 */
	SceneryProject MakeNewProject(std::string name, ProjectKind kind = ProjectKind::Scenery);

	/**
	 * @brief Repairs what a hand-edited or merged file can get wrong, so the rest of the editor can rely on it:
	 * items without an id (or repeating one) get a fresh id, Airport items whose airport is missing are dropped,
	 * airports no item places are added at the top level, libraries get missing short ids. Nothing else is filled in,
	 * so opening and saving an untouched project writes the same bytes.
	 * @return What was changed (empty when the project was already consistent).
	 */
	std::vector<std::string> NormalizeProject(SceneryProject &project);

	/**
	 * @brief Checks a project for problems X-Plane or the exporters would trip on: coordinates out of range, shapes
	 * with too few nodes, items without a resource, unknown library references, duplicate ids, and every airport's
	 * apt.dat problems (XPlane::ValidateAptAirport).
	 */
	std::vector<std::string> ValidateProject(const SceneryProject &project);

	/**
	 * @brief Where the project's local frame is centered: the datum when set, else the first airport's datum, else the
	 * middle of everything placed. False for an empty project.
	 */
	bool GetProjectCenter(const SceneryProject &project, double &latitude, double &longitude);

	/* @brief Bounding box of every coordinate in the project (airports and items). False when there is none. */
	bool GetProjectBounds(const SceneryProject &project, double &south, double &west, double &north, double &east);

	/* @brief Library virtual paths and pack-relative files the items use, sorted and unique. */
	std::vector<std::string> CollectResources(const SceneryProject &project);

	// -------------------------------------------------------
	// Road networks

	/* @brief The roads that start or end at a road node (by id), with whether it is their start. */
	std::vector<std::pair<SceneryItem *, bool>> FindRoadsAt(SceneryProject &project, std::string_view nodeId);

	/**
	 * @brief Puts each road's first and last node on the road nodes it names (their positions win), keeping the
	 * handles' offsets. A road whose ends already sit there is left alone.
	 * @return Ids of the roads that moved.
	 */
	std::vector<std::string> SnapRoadEnds(SceneryProject &project);
}

// -------------------------------------------------------
