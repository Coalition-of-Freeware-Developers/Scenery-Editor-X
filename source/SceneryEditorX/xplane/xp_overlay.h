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
 * xp_overlay.h
 * -------------------------------------------------------
 * Created: 18/10/2026
 * -------------------------------------------------------
 */
#pragma once
#include "xp_apt.h"
#include "xp_apt_geometry.h"
#include "xp_assets.h"
#include "xp_dsf.h"
#include <cstdint>
#include <filesystem>
#include <functional>
#include <map>
#include <memory>
#include <string>
#include <vector>

// -------------------------------------------------------

namespace SceneryEditorX::XPlane
{
	/**
	 * Turns DSF overlay placements (and the textured parts of an apt.dat) into geometry in the airport's local frame:
	 * draped polygons, painted lines, forests and facades become textured meshes; objects and object strings become
	 * placements for the OBJ8 loader. Standard library only, so it can run (and be tested) without the engine; the
	 * scene layer uploads the meshes and loads the textures.
	 */

	/* @brief What an overlay mesh came from. */
	enum class OverlayKind : uint8_t
	{
		DrapedPolygon,	// .pol
		Line,			// .lin
		Forest,			// .for
		Facade,			// .fac
		Autogen,		// .agp ground tile
		Pavement,		// apt.dat taxiway / apron with a library texture (placement = pavement index)
		Runway,			// apt.dat runway with a library texture (placement = runway index)
		Helipad,		// apt.dat helipad with a library texture (placement = helipad index)
		PaintedLine,	// apt.dat linear feature with a library .lin (placement = feature index)
		PavementLine,	// apt.dat pavement edge line with a library .lin (placement = pavement index)
		Road			// .net road segment (placement = index into DsfFile::roads)
	};

	const char *GetOverlayKindName(OverlayKind kind);

	struct OverlayVertex
	{
		float x = 0.0f, y = 0.0f, z = 0.0f;
		float u = 0.0f, v = 0.0f;
		float nx = 0.0f, ny = 1.0f, nz = 0.0f;
	};

	/* @brief How an overlay mesh's texture is read (the draped-polygon shader paths). */
	enum class OverlayShading : uint8_t
	{
		Plain,			// The texture as mapped
		Tiled,			// A RUNWAY_TILE atlas: each tile-sized cell shows the tile its mapping texel names (UVs span the whole atlas)
		Markings		// A RUNWAY_MARKINGS atlas: R paint in the mesh color, G a border darkened by color[3]
	};

	/* @brief A decal drawn with an overlay mesh (PolygonDecal with its texture resolved). */
	struct OverlayDecal
	{
		bool normal = false;
		float scaleS = 1.0f;					// Repeats per unit of the mesh's UVs
		float scaleT = 1.0f;
		float strength = 1.0f;
		float gain = 1.0f;
		std::filesystem::path texture;
	};

	/* @brief A textured mesh ready for upload. */
	struct OverlayMesh
	{
		OverlayKind kind = OverlayKind::DrapedPolygon;
		std::string name;
		std::string resource;					// Virtual / relative path of the asset it came from
		uint32_t placement = 0;					// Index of the DSF polygon it came from (or the apt.dat row)
		std::vector<OverlayVertex> vertices;
		std::vector<uint32_t> indices;			// Counter-clockwise seen from outside / above
		std::filesystem::path texture;			// Empty: draw with color
		float color[4] = { 0.6f, 0.6f, 0.6f, 1.0f };	// Used without a texture, or as a tint
		bool cutout = false;					// Alpha test at cutoff (trees, fences, lines)
		float cutoff = 0.5f;
		bool blended = false;					// Alpha blend (translucent polygons)
		bool twoSided = true;
		Bounds2 bounds;

		// Draped-polygon shading (runway / taxiway art)
		OverlayShading shading = OverlayShading::Plain;
		int tilesX = 1;							// Tiled: the atlas's tiles
		int tilesY = 1;
		std::filesystem::path tileMap;			// Tiled: mapping texture (empty: cells pick tiles at random)
		int tileMapWidth = 0;					// Cells the mapping texture covers before it repeats
		int tileMapHeight = 0;
		float metersPerU = 1.0f;				// Meters one unit of u / v covers (decals keep square texels on non-square atlases)
		float metersPerV = 1.0f;
		float tangentX = 1.0f;					// Ground direction u increases along (x, z), for normal decals
		float tangentZ = 0.0f;
		std::vector<OverlayDecal> decals;
	};

	/* @brief An OBJ8 to place through the object loader. */
	struct OverlayObject
	{
		enum class Source : uint8_t
		{
			Dsf,			// OBJECT placement in the DSF
			String,			// Along a .str
			Facade,			// Attached to a facade segment
			Autogen,		// On an .agp tile
			AirportLights	// apt.dat light string, through the library's .str
		};

		Source source = Source::Dsf;
		std::filesystem::path file;
		std::string resource;					// As named by the DSF / string / autogen
		std::string name;
		double x = 0.0, y = 0.0, z = 0.0;		// Local meters
		float heading = 0.0f;					// True degrees
		uint32_t placement = 0;
	};

	struct OverlayBuild
	{
		std::vector<OverlayMesh> meshes;
		std::vector<OverlayObject> objects;
		std::vector<std::string> warnings;		// Assets that couldn't be resolved or parsed (deduplicated)
		uint32_t culled = 0;					// Placements outside the area of interest
		uint32_t unsupported = 0;				// Placements of asset kinds not drawn (.ags, .agb, roads)
	};

	/* @brief Finds the file behind an asset path: a library virtual path or a path relative to the scenery pack. */
	using AssetResolver = std::function<std::filesystem::path(std::string_view path)>;

	/**
	 * @class OverlayBuilder
	 * @brief Builds overlay geometry. Parsed assets are cached per path for the builder's lifetime, so one builder
	 * should serve a whole airport.
	 */
	class OverlayBuilder
	{
	public:
		struct Options
		{
			Bounds2 area;						// Local-space area of interest; placements outside it (plus margin) are culled when valid
			double areaMargin = 1500.0;			// Meters
			double maxObjects = 20000;
			double bezierSegment = 2.0;			// Meters per straight piece of a curve
			bool drawForests = true;
			bool drawFacades = true;
			bool drawLines = true;
			bool drawPolygons = true;
			bool drawRoads = true;
			bool placeObjects = true;
			bool placeStrings = true;
			uint32_t forestTreeLimit = 40000;	// Trees per forest polygon

			/**
			 * Height above the ground for a draped LAYER_GROUP (name, offset), so the scene can slot DSF polygons
			 * between its own apt.dat layers. Null: a built-in spacing.
			 */
			std::function<float(std::string_view group, int offset)> layerHeight;

			/**
			 * BuildAirportSurfaces only: which apt.dat rows to build, by the kind and index their meshes carry (Pavement and
			 * PavementLine: pavement index, Runway, Helipad, PaintedLine: feature index). Null: every row. Lets an editor
			 * rebuild just the rows it changed.
			 */
			std::function<bool(OverlayKind kind, uint32_t index)> elementFilter;
		};

		OverlayBuilder(AssetResolver resolver, GeoProjection projection);

		/* @brief Converts every placement of a tile. */
		void Build(const DsfFile &dsf, const Options &options, OverlayBuild &out);

		/**
		 * @brief Textured apt.dat surfaces: pavement and runways with the library's pavement .pol textures (by surface
		 * code), painted lines and light strings with the library's .lin / .str assets.
		 * @param pavementHeights Y of each pavement (file order), runwayHeight the runways', lineHeight the lines'.
		 */
		void BuildAirportSurfaces(const AptAirport &airport, const std::vector<float> &pavementHeights, float runwayHeight, float lineHeight, const Options &options, OverlayBuild &out);

		/* @brief Objects along a path at a spacing (the .str asset's objects, cycled), with its lateral offset. */
		void PlaceObjectString(const ObjectStringAsset &asset, const std::filesystem::path &assetFile, std::string_view resource, const std::vector<Point2> &path, bool closed, double spacing, float y, uint32_t placement, OverlayBuild &out, OverlayObject::Source source = OverlayObject::Source::String);

		// Asset caches (nullptr when the asset couldn't be loaded; the warning is recorded once)
		const PolygonAsset *GetPolygon(std::string_view path, std::filesystem::path *file, OverlayBuild &out);
		const LineAsset *GetLine(std::string_view path, std::filesystem::path *file, OverlayBuild &out);
		const ObjectStringAsset *GetString(std::string_view path, std::filesystem::path *file, OverlayBuild &out);
		const ForestAsset *GetForest(std::string_view path, std::filesystem::path *file, OverlayBuild &out);
		const FacadeAsset *GetFacade(std::string_view path, std::filesystem::path *file, OverlayBuild &out);
		const AutogenPointAsset *GetAutogen(std::string_view path, std::filesystem::path *file, OverlayBuild &out);
		const RoadNetworkAsset *GetRoadNetwork(std::string_view path, std::filesystem::path *file, OverlayBuild &out);
		const DecalLibraryAsset *GetDecalLibrary(std::string_view path, std::filesystem::path *file, OverlayBuild &out);

		/**
		 * @brief A draped mesh's shading from its .pol: RUNWAY_TILE atlas and mapping texture, the decals of its rows
		 * and DECAL_LIB library. Call after the geometry is appended (UVs must already use GetPolygonUvScale's scale).
		 */
		void ApplyPolygonShading(OverlayMesh &mesh, const PolygonAsset &asset, const std::filesystem::path &assetFile, OverlayBuild &out);

		/* @brief Meters one texture repeat covers along s and t: SCALE, or SCALE x tiles for TEXTURE_TILE atlases (one tile per SCALE). */
		static void GetPolygonUvScale(const PolygonAsset &asset, float &scaleS, float &scaleT);

		/* @brief A road segment's path in local meters: its curves (one control point: quadratic, two: cubic) flattened. */
		void FlattenRoad(const DsfRoadSegment &segment, std::vector<Point2> &out, const Options &options) const;

		/* @brief Library .pol WorldEditor pairs with an apt.dat surface code ("lib/airport/ground/pavement/asphalt/strips.pol"), empty when the family has none. */
		static std::string GetSurfaceLibraryPolygon(int surface);

		/**
		 * @brief Library .pol X-Plane 12 draws an apt.dat surface with ("lib/airport/default_runways/asphalt/runway.pol"):
		 * kind Runway, Pavement (taxiway) or Helipad; empty for other kinds and for families it has no polygon for here.
		 * Scenery packs replace these (AEP does), so drawing through them shows what the user's X-Plane shows.
		 */
		static std::string GetRunwayLibraryPolygon(int surface, OverlayKind kind);

		/* @brief Library .lin for an apt.dat painted line code (1..59), empty when unknown. */
		static std::string GetLineLibraryPath(int lineType);

		/* @brief Library .str for an apt.dat light string code (101..108), empty when unknown. */
		static std::string GetLightStringLibraryPath(int lightType);

		/* @brief Spacing in meters X-Plane uses for an apt.dat light string code. */
		static double GetLightStringSpacing(int lightType);

	private:
		template<typename Asset>
		struct Cached
		{
			std::unique_ptr<Asset> asset;		// Null when loading failed
			std::filesystem::path file;
		};

		template<typename Asset>
		const Asset *GetCached(std::map<std::string, Cached<Asset>> &cache, std::string_view path, std::filesystem::path *file, OverlayBuild &out,
			bool (*load)(const std::filesystem::path &, Asset &, std::string *), const char *what);

		void Warn(OverlayBuild &out, const std::string &message);
		float Height(const Options &options, std::string_view group, int offset) const;

		/* @brief The resolver, with its answers remembered per path (thousands of placements share a few objects). */
		std::filesystem::path ResolveFile(std::string_view path);
		bool InArea(const Options &options, const Point2 &point) const;
		bool PolygonInArea(const Options &options, const std::vector<std::vector<Point2>> &rings) const;
		/* @brief Local polyline of a winding; with curves, each point's control is planes controlPlane, controlPlane + 1 (lon, lat). */
		void FlattenWinding(const std::vector<DsfPoint> &winding, bool closed, bool bezier, std::vector<Point2> &out, const Options &options, size_t controlPlane = 2) const;

		void BuildDrapedPolygon(const DsfPolygonPlacement &placement, uint32_t index, const std::string &resource, const Options &options, OverlayBuild &out);
		void BuildLine(const DsfPolygonPlacement &placement, uint32_t index, const std::string &resource, const Options &options, OverlayBuild &out);
		void BuildForest(const DsfPolygonPlacement &placement, uint32_t index, const std::string &resource, const Options &options, OverlayBuild &out);
		void BuildFacade(const DsfPolygonPlacement &placement, uint32_t index, const std::string &resource, const Options &options, OverlayBuild &out);
		void BuildString(const DsfPolygonPlacement &placement, uint32_t index, const std::string &resource, const Options &options, OverlayBuild &out);
		void BuildAutogen(const DsfObjectPlacement &placement, uint32_t index, const std::string &resource, const Options &options, OverlayBuild &out);
		void BuildRoad(const DsfRoadSegment &segment, uint32_t index, const std::string &resource, const Options &options, OverlayBuild &out);

		AssetResolver m_Resolver;
		GeoProjection m_Projection;
		std::map<std::string, Cached<PolygonAsset>> m_Polygons;
		std::map<std::string, Cached<LineAsset>> m_Lines;
		std::map<std::string, Cached<ObjectStringAsset>> m_Strings;
		std::map<std::string, Cached<ForestAsset>> m_Forests;
		std::map<std::string, Cached<FacadeAsset>> m_Facades;
		std::map<std::string, Cached<AutogenPointAsset>> m_Autogens;
		std::map<std::string, Cached<RoadNetworkAsset>> m_Roads;
		std::map<std::string, Cached<DecalLibraryAsset>> m_DecalLibraries;
		std::map<std::string, bool> m_Warned;
		std::map<std::string, std::filesystem::path> m_Resolved;
		uint32_t m_Seed = 12345;
	};

	// -------------------------------------------------------
	// Geometry helpers shared with the airport scene

	/**
	 * @brief Appends a flat textured polygon (rings[0] outline, holes after) at height y. UVs: local meters from origin / scale,
	 * rotated by headingDegrees (v runs along the heading, u to its right). Records the u direction as the mesh's tangent.
	 */
	void AppendTexturedPolygon(OverlayMesh &mesh, const std::vector<std::vector<Point2>> &rings, float y, float scaleS, float scaleT, double headingDegrees, Point2 origin = Point2{});

	/* @brief Where a RUNWAY_MARKINGS atlas keeps one marking: texture rectangle (X-Plane UVs, v up) and its paint inside it. */
	struct RunwayMarkingSprite
	{
		float u0 = 0.0f, v0 = 0.0f, u1 = 1.0f, v1 = 1.0f;		// The quad's texture rectangle (paint plus its border)
		float paintWidth = 0.0f, paintLength = 0.0f;			// Fraction of the rectangle the paint covers across / along
	};

	enum class RunwayMarking : uint8_t
	{
		ThresholdBar, TouchdownBar, AimingPoint, Centerline, EdgeStripe, Glyph
	};

	/**
	 * @brief The atlas rectangle of a marking. The layout is shared by X-Plane 12's RWY_markings_*.dds and add-on atlases
	 * such as AEP's (same proportions at any size): 1-9, 0, L, R, C across the top, threshold / touchdown / aiming point
	 * bars and centerline / edge stripes below. glyph: '0'..'9', 'L', 'R', 'C' (Glyph only).
	 */
	RunwayMarkingSprite GetRunwayMarkingSprite(RunwayMarking marking, char glyph = '1');

	/**
	 * @brief Appends the painted markings of an apt.dat runway (per end, by its 100-row marking code): threshold bars,
	 * designator, touchdown zone and aiming points, centerline dashes and edge stripes, as quads at height y with UVs
	 * into a RUNWAY_MARKINGS atlas. FAA / ICAO dimensions. False when the runway has no markings.
	 */
	bool AppendRunwayMarkings(OverlayMesh &mesh, const AptRunway &runway, const GeoProjection &projection, float y);

	/* @brief Appends a textured ribbon along a path: u spans [s1, s2] across, v runs along the path (meters / scaleT). */
	void AppendTexturedRibbon(OverlayMesh &mesh, const std::vector<Point2> &path, bool closed, float y, float width, float s1, float s2, float scaleT, bool mirror);

	/**
	 * @brief Appends a textured band along an open path between two lateral offsets (meters, right of travel positive):
	 * u runs from sLeft to sRight across, v along the path (meters / scaleT). Corners are mitered.
	 */
	void AppendOffsetRibbon(OverlayMesh &mesh, const std::vector<Point2> &path, float y, float left, float right, float sLeft, float sRight, float scaleT);

	/* @brief Resamples a polyline (open or closed) at a spacing: positions and headings (degrees) along it. */
	void ResamplePath(const std::vector<Point2> &path, bool closed, double spacing, double startOffset, std::vector<Point2> &outPoints, std::vector<double> &outHeadings);
}

// -------------------------------------------------------
