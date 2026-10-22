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
 * xp_terrain_mesh.h
 * -------------------------------------------------------
 * Created: 22/10/2026
 * -------------------------------------------------------
 */
#pragma once
#include "xp_apt_geometry.h"
#include "xp_dsf.h"
#include "xp_overlay.h"
#include <cstdint>
#include <string>
#include <vector>

// -------------------------------------------------------

namespace SceneryEditorX::XPlane
{
	/**
	 * The base terrain around an airport in the scene's local frame (x east, y up, z south, meters; y = 0 at a datum
	 * elevation): meshes from DSF terrain patches, and the surface everything else stands on. The surface answers
	 * height and ray queries and drapes flat geometry over the terrain the way X-Plane drapes DSF polygons and apt.dat
	 * pavement: each triangle is cut along the terrain's triangle edges, so the pieces lie exactly on it. Standard
	 * library only.
	 */

	/* @brief Mean earth radius, meters (the drop of the horizon: d^2 / 2R). */
	inline constexpr double TERRAIN_EARTH_RADIUS = 6371008.8;

	/* @brief A terrain vertex: local meters, normal, and the patch's texture coordinates (border s / t, or water fetch / depth). */
	struct TerrainVertex
	{
		float x = 0.0f, y = 0.0f, z = 0.0f;
		float nx = 0.0f, ny = 1.0f, nz = 0.0f;
		float s = 0.0f, t = 0.0f;
	};

	/* @brief The triangles of one terrain definition in one tile, drawn as one part. */
	struct TerrainMeshPart
	{
		std::string definition;				// The DSF's terrain definition (a .ter virtual path, or "terrain_Water")
		bool overlay = false;				// Border patches: drawn over the base, faded by the .ter's BORDER_TEX
		bool water = false;					// DSF water mesh (no .ter)
		uint32_t order = 0;					// The DSF patch its first triangles came from (overlays draw in this order)
		std::vector<TerrainVertex> vertices;
		std::vector<uint32_t> indices;		// Counter-clockwise seen from above
	};

	/**
	 * @brief Where terrain is kept: within margin meters of some rings (an airport's boundary, say; holes aren't
	 * needed), in the local frame. No rings: everywhere.
	 */
	struct TerrainArea
	{
		std::vector<std::vector<Point2>> rings;
		double margin = 1609.344;			// One statute mile

		/* @brief True when the triangle reaches into the area. */
		[[nodiscard]] bool Overlaps(const Point2 &a, const Point2 &b, const Point2 &c) const;

		/* @brief The area's longitude / latitude box (for DsfReadOptions), through a projection. */
		void GetGeoBox(const GeoProjection &projection, double &west, double &south, double &east, double &north) const;
	};

	struct TerrainBuildOptions
	{
		double datumElevation = 0.0;		// Meters above sea level that y = 0 stands for
		bool earthCurvature = true;			// Lower points by d^2 / 2R with their distance from the datum point, as on X-Plane's round earth
		TerrainArea area;
		double waterCellSize = 0.0;			// Water meshes cut into cells this size (CutTerrainAlongGrid; 0: as the DSF has them)
		double waterDepthWithoutBathymetryM = 10.0;	// Floor depth at a depth ratio of 1 when the tile has no sea_level raster (ASSUMPTION)
	};

	/**
	 * @brief Height of the local frame's y = 0 above sea level for a point, so a point of elevation e (meters MSL)
	 * lands at y = e - datum - drop(distance): the drop of a round earth seen from the datum point (local origin).
	 */
	[[nodiscard]] double GetTerrainCurvatureDrop(double x, double z);

	/**
	 * @brief Converts the terrain patches of a DSF (read with DsfReadOptions::terrain, and its elevation raster) into
	 * parts in the local frame, appended to out: one part per definition and kind (base, overlay), triangles facing up.
	 * Points whose elevation is the raster flag take the raster's height; normals the patch doesn't carry come from the
	 * raster's slope (as X-Plane computes them), else from the mesh. Triangles outside options.area are left out.
	 * Water parts are cut into options.waterCellSize cells, and their vertices' t becomes the floor's depth in meters
	 * (WaterFloorDepth); s stays the DSF's fetch ratio.
	 * @return Triangles appended.
	 */
	uint32_t BuildTerrainMeshes(const DsfFile &dsf, const GeoProjection &projection, const TerrainBuildOptions &options, std::vector<TerrainMeshPart> &out);

	/**
	 * @brief Depth (m) of the water's floor below its surface at a water vertex. X-Plane 12's DSF water carries a depth
	 * ratio per vertex: 0 the ground's elevation (the shore), 1 the bathymetry (the tile's sea_level raster), so the floor
	 * lies ratio x (surface - bathymetry) down; never above the surface.
	 */
	[[nodiscard]] double WaterFloorDepth(double surfaceElevationM, double bathymetryElevationM, double depthRatio);

	/**
	 * @brief Cuts triangles along the lines x = k cell and z = k cell, so no piece is wider than a cell: each triangle is
	 * clipped to every cell it covers and the pieces fanned, then vertices within 5 mm are welded, so triangles that shared
	 * an edge share its new points too (no T-junctions: tessellated and displaced, the mesh has no cracks). Attributes
	 * interpolate linearly; winding is kept.
	 * @return Triangles after cutting.
	 */
	uint32_t CutTerrainAlongGrid(TerrainMeshPart &part, double cell);

	/**
	 * @class TerrainSurface
	 * @brief The physical terrain (base and water patches; not the border overlays) as one triangle soup with a grid
	 * over it, for height queries, ray casts and draping.
	 */
	class TerrainSurface
	{
	public:
		/* @brief Rebuilds from the physical parts (overlays are skipped). */
		void Build(const std::vector<TerrainMeshPart> &parts);
		void Clear();

		[[nodiscard]] bool IsEmpty() const { return m_Triangles.empty(); }
		[[nodiscard]] uint32_t GetTriangleCount() const { return static_cast<uint32_t>(m_Triangles.size() / 3); }
		[[nodiscard]] const Bounds2 &GetBounds() const { return m_Bounds; }

		/**
		 * @brief The terrain's y (and smooth normal) at a point of the ground plane.
		 * @return False outside the terrain.
		 */
		bool HeightAt(double x, double z, float &y, float normal[3] = nullptr) const;

		/* @brief HeightAt() or a fallback where there's no terrain. */
		[[nodiscard]] float HeightOr(double x, double z, float fallback = 0.0f) const;

		/**
		 * @brief The nearest hit of a ray with the terrain.
		 * @param direction Need not be normalized; distance comes back in its units.
		 */
		bool Raycast(const double origin[3], const double direction[3], double maxDistance, double &distance) const;

		/**
		 * @brief Drapes flat geometry: each triangle is cut along the terrain triangles under it and every piece is
		 * lifted by the terrain's height (the mesh's own y stays as the offset above the ground: layer heights), with the
		 * terrain's normal. Attributes interpolate over the original triangle. What has no terrain under it is kept as it
		 * was, or with clip left out (scenery that must end where the terrain does).
		 * @return Triangles after draping.
		 */
		uint32_t Drape(std::vector<OverlayVertex> &vertices, std::vector<uint32_t> &indices, bool clip = false) const;

		/* @brief Drape() for an overlay mesh, bounds kept. */
		uint32_t Drape(OverlayMesh &mesh, bool clip = false) const;

		/* @brief Lifts every vertex by the terrain's height under it (3D geometry: walls and roofs follow the ground). */
		void Lift(std::vector<OverlayVertex> &vertices) const;

	private:
		struct Cell
		{
			uint32_t first = 0;
			uint32_t count = 0;
		};

		/* @brief Terrain triangles whose box overlaps a box (each once). */
		void Gather(double minX, double minZ, double maxX, double maxZ, std::vector<uint32_t> &out) const;

		/* @brief The triangle containing a point (with its barycentrics), or UINT32_MAX. */
		uint32_t Locate(double x, double z, double bary[3]) const;

		std::vector<float> m_Positions;		// x, y, z per vertex
		std::vector<float> m_Normals;		// x, y, z per vertex
		std::vector<uint32_t> m_Triangles;
		std::vector<float> m_TriangleBoxes;	// min x, min z, max x, max z per triangle
		Bounds2 m_Bounds;
		double m_CellSize = 50.0;
		int m_CellsX = 0;
		int m_CellsZ = 0;
		std::vector<Cell> m_Cells;
		std::vector<uint32_t> m_CellTriangles;
		mutable std::vector<uint32_t> m_Stamp;	// Per triangle: the query that saw it last (Gather dedup)
		mutable uint32_t m_Query = 0;
	};
}

// -------------------------------------------------------
