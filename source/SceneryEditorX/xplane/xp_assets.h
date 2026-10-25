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
 * xp_assets.h
 * -------------------------------------------------------
 * Created: 18/10/2026
 * -------------------------------------------------------
 */
#pragma once
#include <cstdint>
#include <filesystem>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

// -------------------------------------------------------

namespace SceneryEditorX::XPlane
{
	/**
	 * The text art assets placed by DSF overlays besides objects: draped polygons (.pol), painted lines (.lin),
	 * object strings (.str), forests (.for) and facades (.fac, types 1 and 2), per the specs under
	 * https://developer.x-plane.com/docs/specs/. Read far enough to draw them; shader and weather attributes are
	 * kept where they matter for rendering. Standard library only.
	 */

	/* @brief The "I / A, version, TYPE" header every X-Plane text asset starts with. */
	struct AssetHeader
	{
		int version = 0;
		std::string type;				// "DRAPED_POLYGON", "LINE_PAINT", "OBJECT_STRING", "FOREST", "FACADE", "OBJ", ...
	};

	/* @brief Reads the header; returns the position of the first line after it (or npos when it isn't one). */
	size_t ParseAssetHeader(std::string_view text, AssetHeader &out);

	/**
	 * @brief A detail or normal decal of a .pol or .dcl: a second texture repeated over the surface at its own scale.
	 * Detail decals are grey-centred (0.5 = no change) and modulate the albedo; normal decals add their RG slopes to the
	 * surface normal. X-Plane keys a decal's strength from the albedo / control channels; here the strongest of its
	 * constant terms stands for it.
	 */
	struct PolygonDecal
	{
		bool normal = false;				// NORMAL_DECAL_PARAMS[_PROJ] (else DECAL_PARAMS[_PROJ], DECAL, DECAL_RGBA)
		float scaleS = 1.0f;				// Repeats per texture repeat of the polygon (the albedo's SCALE), along s and t
		float scaleT = 1.0f;
		float strength = 1.0f;				// 0..1: how much of the decal applies
		float gain = 1.0f;					// Normal decals: slope multiplier
		std::string texture;				// Relative to the file it came from (the .pol or the .dcl)
	};

	/* @brief .pol: a textured polygon draped on the ground. */
	struct PolygonAsset
	{
		std::string texture;				// Relative to the .pol file
		std::string textureNormal;
		bool wrap = true;					// TEXTURE (repeats) vs TEXTURE_NOWRAP (orthophoto, UVs from the DSF)
		float scaleS = 1000.0f;				// Meters per texture repeat
		float scaleT = 1000.0f;
		bool noAlpha = false;				// NO_ALPHA: ignore the alpha channel
		float noBlendCutoff = -1.0f;		// NO_BLEND <ratio>, -1 when blended
		std::string layerGroup;				// LAYER_GROUP name (terrain, beaches, shoulders, taxiways, runways, markings, airports, roads, objects, light_objects, cars)
		int layerOffset = 0;
		std::string surface;				// SURFACE asphalt / concrete / ... (physics only)
		std::string decal;					// DECAL_LIB: a .dcl library path (its decals are in addition to the ones below)
		std::vector<PolygonDecal> decals;	// DECAL_PARAMS[_PROJ] / NORMAL_DECAL_PARAMS[_PROJ] / DECAL / DECAL_RGBA rows
		bool tiled = false;					// TEXTURE_TILE / RUNWAY_TILE present
		int tilesX = 1;						// The texture is an atlas of tilesX x tilesY variants
		int tilesY = 1;

		/**
		 * RUNWAY_TILE <tiles x> <tiles y> <map width> <map height> <map.png> (X-Plane 12 runway and taxiway art): SCALE
		 * covers the whole atlas, so one tile is SCALE / tiles meters. Each tile-sized cell of the surface shows the tile
		 * its texel of the mapping texture names: R = column x tiles / 256, G = row x tiles / 256, the map repeating every
		 * map width x height cells. TEXTURE_TILE (no map, older art) keeps SCALE as one tile.
		 */
		bool runwayTile = false;
		std::string tileMap;				// Relative to the .pol; empty: cells pick tiles at random
		int tileMapWidth = 0;
		int tileMapHeight = 0;
		float noiseParams[5] = {};			// RUNWAY_NOISE_PARAMS (kept, not drawn)

		/**
		 * RUNWAY_MARKINGS <r> <g> <b> <a> <atlas>: the atlas runway markings are painted from when this polygon is a
		 * runway's surface. Its R channel is the paint, G (where it reaches past R) a dark border; r g b are the paint
		 * color, a the border's opacity.
		 */
		std::string runwayMarkings;			// Relative to the .pol; empty: none
		float runwayMarkingsColor[4] = { 1.0f, 1.0f, 1.0f, 1.0f };

		// Weather receptivity (weather.slang)
		std::string weather;				// WEATHER <mask>: R rain, G puddles, B snow; relative to the .pol
		bool weatherTransparent = false;	// WEATHER_TRANSPARENT: takes the weather of what lies under it
		bool hasSnowAlbedoLuma = false;		// SNOW_ALBEDO_LUMA <r> <g>: snow receptivity weights for the albedo
		float snowAlbedoLuma[2] = {};
	};

	bool ParsePolygonAsset(std::string_view text, PolygonAsset &out, std::string *error = nullptr);
	bool LoadPolygonAsset(const std::filesystem::path &path, PolygonAsset &out, std::string *error = nullptr);

	/* @brief .dcl: a decal library (A 1000 DECAL) a .pol / .obj names with DECAL_LIB. */
	struct DecalLibraryAsset
	{
		std::vector<PolygonDecal> decals;
	};

	bool ParseDecalLibraryAsset(std::string_view text, DecalLibraryAsset &out, std::string *error = nullptr);

	/**
	 * @brief Reads one decal row (DECAL, DECAL_RGBA, DECAL_PARAMS[_PROJ], NORMAL_DECAL_PARAMS[_PROJ]) of the standard
	 * shader formats (.pol, .dcl, .ter, ...), tokenized; false for other rows.
	 */
	bool ParsePolygonDecalRow(const std::vector<std::string_view> &tokens, PolygonDecal &out);
	bool LoadDecalLibraryAsset(const std::filesystem::path &path, DecalLibraryAsset &out, std::string *error = nullptr);

	/* @brief .lin: a painted line following a path. */
	struct LineAsset
	{
		struct Segment
		{
			int layer = 0;
			float s1 = 0.0f;				// Texture s range of this line style, normalized 0..1
			float sm = 0.0f;				// Center (where the path runs)
			float s2 = 1.0f;
		};

		struct Cap
		{
			float s1 = 0.0f, sm = 0.0f, s2 = 1.0f, t1 = 0.0f, t2 = 1.0f;
		};

		std::string texture;
		std::string textureNormal;
		float scaleS = 100.0f;				// Meters the whole texture width covers
		float scaleT = 100.0f;				// Meters per texture repeat along the line
		float textureWidth = 1024.0f;		// Pixels (S_OFFSET values are pixels)
		float textureHeight = 1024.0f;
		std::vector<Segment> segments;		// S_OFFSET rows; the first is the line as drawn
		std::vector<Cap> startCaps;
		std::vector<Cap> endCaps;
		bool mirror = false;				// Alternate texture direction per repeat
		int align = 0;						// ALIGN: snap length to whole repeats
		std::string layerGroup = "markings";
		int layerOffset = 0;
		std::string decal;

		/* @brief Painted width in meters (one transparent pixel margin on each side, like WorldEditor assumes). */
		[[nodiscard]] float GetWidthMeters(size_t segment = 0) const;
	};

	bool ParseLineAsset(std::string_view text, LineAsset &out, std::string *error = nullptr);
	bool LoadLineAsset(const std::filesystem::path &path, LineAsset &out, std::string *error = nullptr);

	/* @brief .str: objects repeated along a path. */
	struct ObjectStringAsset
	{
		struct Entry
		{
			float rotationMin = 0.0f;		// OBJECT <min rotation> <max rotation> <path>: extra heading relative to the path
			float rotationMax = 0.0f;
			std::string object;				// Relative to the .str (or a library path)
		};

		std::vector<Entry> objects;			// Cycled along the path
		float offset = 0.0f;				// OFFSET: lateral offset from the path, meters (positive right)
	};

	bool ParseObjectStringAsset(std::string_view text, ObjectStringAsset &out, std::string *error = nullptr);
	bool LoadObjectStringAsset(const std::filesystem::path &path, ObjectStringAsset &out, std::string *error = nullptr);

	/* @brief .for: trees scattered over a polygon. */
	struct ForestAsset
	{
		struct Tree
		{
			float s = 0.0f;					// Texture rectangle, pixels
			float t = 0.0f;
			float width = 0.0f;				// Pixels (negative flips)
			float height = 0.0f;
			float offset = 0.0f;			// Center line of the tree within the rectangle, pixels
			float percent = 100.0f;			// Relative frequency
			float minHeight = 10.0f;		// Meters
			float maxHeight = 20.0f;
			int quads = 2;					// Crossed quads making up the tree
			int layer = 0;
		};

		std::string texture;
		float scaleX = 256.0f;				// Texture size in pixels
		float scaleY = 256.0f;
		float spacingX = 30.0f;				// Grid spacing, meters
		float spacingY = 30.0f;
		float randomX = 0.0f;				// Jitter, meters
		float randomY = 0.0f;
		std::vector<Tree> trees;
		float maxHeight = 0.0f;
	};

	bool ParseForestAsset(std::string_view text, ForestAsset &out, std::string *error = nullptr);
	bool LoadForestAsset(const std::filesystem::path &path, ForestAsset &out, std::string *error = nullptr);

	/**
	 * @brief .bch: the strips drawn along coasts and lake shores. A DSF beach polygon gives each point a beach type (the
	 * type of the edge starting there, 0 none); a type's TILE rows are pieces of the atlas a strip is laid with, one
	 * after another along the shore: s across the piece's length, t across the shore from its water edge (t1) through
	 * the shoreline (t2, on the polygon's edge) to its land edge (t3). Pixels count in SCALE_X / SCALE_Y; the whole
	 * texture covers SIZE_X x SIZE_Y meters. Types are grouped (GROUP group type) for the transition pieces
	 * (XONL2 / XONR2 within a type, XONLG / XONRG between groups, to group 0 at dead ends).
	 */
	struct BeachAsset
	{
		struct Tile
		{
			int type = 0;
			float s1 = 0.0f, s2 = 0.0f;		// Pixels along the shore
			float t1 = 0.0f;				// Water edge (pixels)
			float t2 = 0.0f;				// Shoreline
			float t3 = 0.0f;				// Land edge
		};

		/* @brief A transition piece: its kind (XONL2, XONR2, XONLG, XONRG), the type or groups it joins, its rectangle. */
		struct Transition
		{
			std::string kind;
			int from = 0;					// Type (XONL2 / XONR2) or group the shore comes from
			int to = 0;						// Group it goes to (XONLG / XONRG; 0 a dead end)
			Tile tile;
		};

		std::string texture;				// TEXTURE (X-Plane 12) or BASE_TEX
		std::string textureNormal;
		float normalRatio = 1.0f;
		std::string decalLibrary;			// DECAL_LIB
		float scaleX = 1.0f;				// Pixels across the texture
		float scaleY = 1.0f;
		float sizeX = 1.0f;					// Meters the whole texture covers
		float sizeY = 1.0f;
		std::vector<Tile> tiles;
		std::vector<Transition> transitions;
		std::vector<std::pair<int, int>> groups;	// (group, type)

		/* @brief The TILE rows of a type, in file order. */
		[[nodiscard]] std::vector<const Tile *> GetTiles(int type) const;
	};

	bool ParseBeachAsset(std::string_view text, BeachAsset &out, std::string *error = nullptr);
	bool LoadBeachAsset(const std::filesystem::path &path, BeachAsset &out, std::string *error = nullptr);

	/* @brief .fac: a building (or fence) extruded along a polygon. */
	struct FacadeAsset
	{
		/**
		 * @brief When a wall may be used (WALL and each WALL_RULE): its width range in meters and its heading range in
		 * degrees relative to the facade's front (the first wall). Equal headings ("0 0") mean any heading.
		 */
		struct WallFilter
		{
			float minWidth = 0.0f;
			float maxWidth = 9999.0f;
			float minHeading = 0.0f;
			float maxHeading = 0.0f;

			[[nodiscard]] bool Accepts(double width, double relativeHeading) const;
		};

		/* Type 1: a wall is a grid of texture panels. */
		struct Wall
		{
			std::string name;
			float minWidth = 0.0f;
			float maxWidth = 9999.0f;
			std::vector<WallFilter> filters;	// The WALL row's own filter, then its WALL_RULE rows
			float scaleS = 1.0f;			// Meters the whole texture width / height cover (SCALE)
			float scaleT = 1.0f;
			float roofSlope = 0.0f;
			bool slant = false;				// ROOF_SLOPE ... SLANT: the sloped row shortens instead of stretching
			float basement = 0.0f;			// Normalized t below ground
			float textureWidth = 0.0f;		// TEX_SIZE (pixels) its rows are written in; 0 when they are fractions
			float textureHeight = 0.0f;
			std::vector<std::pair<float, float>> leftPanels;		// Normalized s ranges
			std::vector<std::pair<float, float>> centerPanels;
			std::vector<std::pair<float, float>> rightPanels;
			std::vector<std::pair<float, float>> bottomFloors;	// Normalized t ranges
			std::vector<std::pair<float, float>> middleFloors;
			std::vector<std::pair<float, float>> topFloors;
		};

		/* Type 2: walls are built from segment meshes picked by spellings. */
		struct Mesh
		{
			std::vector<float> positions;	// x (outward), y (up), z (-z along the wall), 3 per vertex
			std::vector<float> normals;
			std::vector<float> uvs;
			std::vector<uint32_t> indices;
			int group = 0;					// MESH <group> <far LOD> <cuts> ...
			float lodFar = 0.0f;
			int cuts = 1;					// Pieces a curved wall bends the mesh in
		};

		struct Attachment
		{
			int object = 0;					// Index into objects
			float x = 0.0f, y = 0.0f, z = 0.0f;
			float heading = 0.0f;
			bool draped = true;
			int showLo = 0;					// Object density range 1..6 ([lo hi]); 0 when the row gives none
			int showHi = 0;
		};

		struct Segment
		{
			std::vector<Mesh> meshes;
			std::vector<Attachment> attachments;
			float width = 0.0f;				// Along-wall extent of the meshes (x), computed after parsing
		};

		struct WallRule
		{
			std::string name;
			float minWidth = 0.0f;
			float maxWidth = 9999.0f;
			float minHeading = 0.0f;
			float maxHeading = 360.0f;
			std::vector<WallFilter> filters;			// The WALL row's own filter, then its WALL_RULE rows
			std::vector<std::vector<int>> spellings;	// Segment index sequences to fill a wall with
		};

		/* @brief An OBJ on a roof (ROOF_OBJ / ROOF_OBJ_HEADING): meters across the roof from the front wall. */
		struct RoofObject
		{
			int object = 0;
			float s = 0.0f;
			float t = 0.0f;
			float heading = 0.0f;			// Relative to the facade's front
			int showLo = 0;
			int showHi = 0;
		};

		struct Roof
		{
			float height = 0.0f;			// Meters; 0 drapes the roof on the ground
			bool twoSided = false;
			std::vector<RoofObject> objects;
		};

		struct Floor
		{
			std::string name;
			std::vector<Segment> segments;
			std::vector<Segment> curvedSegments;	// SEGMENT_CURVED: what a bezier wall bends (same indices as segments)
			std::vector<WallRule> walls;
			std::vector<float> roofHeights;	// ROOF_HEIGHT values (meters), one per allowed roof (mirrors roofs)
			std::vector<Roof> roofs;		// The same roofs with their options and objects (what a writer writes)
			std::string hardRoof;			// HARD_ROOF surface, empty when the roofs aren't solid
			float height = 0.0f;			// Its top roof, else its tallest segment (computed after parsing): what a DSF height matches
		};

		/* @brief FACADE_SCRAPER: above this height the facade becomes a skyscraper (base + tower models). */
		struct Scraper
		{
			float minAgl = 0.0f;
			float maxAgl = 0.0f;
			float step = 0.0f;
			int floors = 0;
			int models = 0;					// FACADE_SCRAPER_MODEL(_OFFSET) rows
			int pads = 0;					// FACADE_SCRAPER_PAD rows
			std::vector<std::string> rows;	// Those rows, verbatim (written back as they were)
		};

		int version = 800;					// 800 = type 1, 1000 = type 2
		bool ring = true;					// Closed polygon; false = open fence
		bool twoSided = false;
		bool graded = false;				// GRADED (vs DRAPED)
		std::string wallTexture;			// Relative to the .fac
		std::string roofTexture;
		std::string wallTextureNormal;
		float wallTextureNormalScale = 1.0f;	// TEXTURE_NORMAL <ratio> <file>
		std::string roofTextureNormal;
		float roofTextureNormalScale = 1.0f;
		float noBlendCutoff = -1.0f;		// The walls' NO_BLEND
		std::string layerGroup;				// LAYER_GROUP name and offset, empty for the default ("objects")
		int layerOffset = 0;
		std::string drapedLayerGroup;		// LAYER_GROUP_DRAPED
		int drapedLayerOffset = 0;
		std::vector<Scraper> scrapers;

		/**
		 * Rows of the SHADER_WALL / SHADER_ROOF sections the editor doesn't model (TEXTURE_LIT, TEXTURE_MODULATOR,
		 * NORMAL_METALNESS, DECAL_LIB, SNOW_ALBEDO_LUMA, the roof's NO_BLEND and TWO_SIDED...), verbatim, so a facade
		 * written back keeps its X-Plane 12 material. Rows before any SHADER_ line belong to the walls.
		 */
		std::vector<std::string> wallShaderRows;
		std::vector<std::string> roofShaderRows;

		// Type 1
		std::vector<Wall> walls;
		float lodFar = 10000.0f;			// The nearest LOD's far distance, meters (LOD 0 <far>)
		bool hasRoof = false;
		std::vector<std::pair<float, float>> roofUv;	// ROOF s t corners, normalized (ROOF_SCALE: its rectangle, counter-clockwise from s1 t1)
		bool roofScaled = false;			// ROOF_SCALE (a texture rectangle repeated by meters) instead of ROOF corners
		std::pair<float, float> roofCenter{ 0.0f, 0.0f };	// ROOF_SCALE's reference point, normalized
		float roofTextureWidth = 0.0f;		// TEX_SIZE (pixels) the roof rows are written in; 0 when fractions
		float roofTextureHeight = 0.0f;
		float roofScaleS = 1.0f;			// Meters per roof texture repeat (ROOF_SCALE), type 2 too
		float roofScaleT = 1.0f;
		int floorsMin = 1;
		int floorsMax = 999;
		bool doubled = false;				// DOUBLED: walls built inside and out
		std::string hardWall;				// HARD_WALL / HARD_ROOF surfaces
		std::string hardRoof;

		// Type 2
		std::vector<std::string> objects;	// OBJ paths relative to the .fac (or library)
		std::vector<Floor> floors;
		bool noRoofMesh = false;
		bool noWallMesh = false;

		[[nodiscard]] bool IsType2() const { return version >= 1000; }
	};

	bool ParseFacadeAsset(std::string_view text, FacadeAsset &out, std::string *error = nullptr);
	bool LoadFacadeAsset(const std::filesystem::path &path, FacadeAsset &out, std::string *error = nullptr);

	/* @brief .agp (AG_POINT): a ground tile with objects on it, placed like an object. */
	struct AutogenPointAsset
	{
		struct Object
		{
			int object = 0;					// Index into objects
			float x = 0.0f;					// Meters from the anchor, x east / y north in the tile's frame
			float y = 0.0f;
			float heading = 0.0f;
			bool draped = true;
		};

		std::string texture;				// Ground texture, relative to the .agp
		std::vector<std::string> objects;	// OBJECT rows, relative to the .agp (or library paths)
		std::vector<float> tile;			// Ground quad corners (x, y) meters and (s, t) UVs, 4 values per corner
		float anchorX = 0.0f;				// ANCHOR_PT in meters; the placement point
		float anchorY = 0.0f;
		float rotation = 0.0f;				// ROTATION steps (90 degree multiples allowed) - informational
		std::vector<Object> placedObjects;
		bool hideTiles = false;				// HIDE_TILES: no ground texture

		[[nodiscard]] bool HasTile() const { return tile.size() >= 16 && !hideTiles && !texture.empty(); }
	};

	bool ParseAutogenPointAsset(std::string_view text, AutogenPointAsset &out, std::string *error = nullptr);
	bool LoadAutogenPointAsset(const std::filesystem::path &path, AutogenPointAsset &out, std::string *error = nullptr);

	/**
	 * @brief .net: the road, railway and power line types of a network ("A 800 ROADS"). Read far enough to draw a
	 * road the way WorldEditor previews it: the flat strips of its road type, each a band of texture across the road.
	 * Junction meshes, graded (raised) parts, objects, wires and traffic aren't read.
	 *
	 * A DSF names a road by subtype. In X-Plane 10 and later networks the subtype is a virtual type (ROAD_DRAPED,
	 * labeled by a "#VROAD name" comment) that picks a road type by terrain grade; older networks use road type
	 * numbers directly.
	 */
	struct RoadNetworkAsset
	{
		/* @brief A flat band of texture across the road: lateral meters from the center line, right of travel positive. */
		struct Strip
		{
			int texture = 0;				// Index into textures
			float left = 0.0f;
			float right = 0.0f;
			float sLeft = 0.0f;				// Texture s at either edge (0..1)
			float sRight = 1.0f;
		};

		struct RoadType
		{
			int id = 0;
			float width = 0.0f;				// Meters
			float length = 100.0f;			// Meters per texture repeat along the road
			int texture = 0;
			float color[3] = { 0.6f, 0.6f, 0.6f };	// Map color (drawn when the type has no strips)
			std::vector<Strip> strips;
		};

		struct VirtualType
		{
			int id = 0;
			std::string description;		// "hwy_6/city/wet", empty when the file names none
			int roadType = 0;				// The first road type its drape choices name (what a preview draws)
		};

		std::vector<std::string> textures;	// TEXTURE rows in order, relative to the .net
		std::vector<RoadType> roadTypes;
		std::vector<VirtualType> virtualTypes;

		/* @brief The road type a DSF subtype draws as: its virtual type's choice, else the road type of that number. Null when neither exists. */
		[[nodiscard]] const RoadType *FindRoadType(int subtype) const;

		/* @brief A label for a subtype: the virtual type's description, else "Road type N". */
		[[nodiscard]] std::string GetSubtypeName(int subtype) const;
	};

	bool ParseRoadNetworkAsset(std::string_view text, RoadNetworkAsset &out, std::string *error = nullptr);
	bool LoadRoadNetworkAsset(const std::filesystem::path &path, RoadNetworkAsset &out, std::string *error = nullptr);

	/* @brief Draw order of LAYER_GROUP names, low (terrain) to high (cars). Unknown names count as "objects". */
	int GetLayerGroupOrder(std::string_view group);
}

// -------------------------------------------------------
