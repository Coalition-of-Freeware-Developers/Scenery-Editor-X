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
 * xp_terrain_scene.h
 * -------------------------------------------------------
 * Created: 22/10/2026
 * -------------------------------------------------------
 */
#pragma once
#include "xp_terrain_mesh.h"
#include <cstdint>
#include <string>

// -------------------------------------------------------

namespace SceneryEditorX::XPlane
{
	/**
	 * @class SceneTerrain
	 * @brief X-Plane's base terrain around the loaded airport (plan Phase 10): the DSF base mesh of the highest-priority
	 * scenery pack that has one for each tile (a custom mesh before the Global Scenery), cut down to an area (the airport
	 * property plus a mile), drawn with its .ter land classes (xp_terrain.slang), and the surface the airport, its
	 * overlays and objects are draped on. y = 0 is the terrain's height at the airport's datum. Main thread only; one
	 * terrain at a time.
	 */
	class SceneTerrain
	{
	public:
		struct Stats
		{
			uint32_t tiles = 0;				// DSF tiles read
			uint32_t triangles = 0;
			uint32_t parts = 0;				// Drawn parts (a land class's base or border patches, water)
			uint32_t terrainTypes = 0;		// .ter files loaded
			uint32_t missingTypes = 0;		// Land classes whose .ter couldn't be resolved or read (not drawn)
			uint32_t textures = 0;
			double datumElevation = 0.0;	// Meters above sea level of y = 0
			double loadMilliseconds = 0.0;
		};

		/**
		 * @brief Replaces the terrain with the one in an area of a local frame. Nothing changes when the same frame and
		 * area are loaded already.
		 * @param fallbackElevation Meters MSL of y = 0 when no elevation raster covers the frame's datum.
		 * @return True when terrain is shown.
		 */
		static bool Load(const GeoProjection &projection, const TerrainArea &area, const std::string &name, double fallbackElevation);

		/* @brief Removes the terrain from the scene (and the surface: drapes go flat). */
		static void Unload();

		[[nodiscard]] static bool IsLoaded();

		/* @brief The physical terrain, empty when none is loaded. */
		[[nodiscard]] static const TerrainSurface &GetSurface();

		/* @brief The terrain's y under a point of the ground plane (scene local frame), 0 where there is none. */
		[[nodiscard]] static float GetHeight(double x, double z);

		/* @brief Meters above sea level that y = 0 stands for (the loaded frame's). */
		[[nodiscard]] static double GetDatumElevation();

		/**
		 * @brief y of a point at an elevation above sea level (the earth's curvature included, as the terrain has it).
		 */
		[[nodiscard]] static float ElevationToY(double elevationMsl, double x, double z);

		[[nodiscard]] static uint64_t GetRootId();

		/* @brief Changes whenever the terrain (and so the surface) is loaded or unloaded: what stands on it re-seats. */
		[[nodiscard]] static uint64_t GetGeneration();

		/* @brief True when the terrain was built in this frame (a datum within a centimeter of the projection's). */
		[[nodiscard]] static bool IsInFrame(const GeoProjection &projection);

		/* @brief True for the terrain's root and its part entities. */
		[[nodiscard]] static bool Contains(uint64_t entityId);

		[[nodiscard]] static Stats GetStats();

		/* @brief Whether airports load terrain (--no-terrain / the RENDERER panel). Off unloads it. */
		static void SetEnabled(bool enabled);
		[[nodiscard]] static bool IsEnabled();

		/* @brief Releases GPU resources. Call before the renderer shuts down. */
		static void Shutdown();
	};
}

// -------------------------------------------------------
