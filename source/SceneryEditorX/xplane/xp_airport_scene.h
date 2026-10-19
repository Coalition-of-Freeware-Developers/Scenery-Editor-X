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
 * xp_airport_scene.h
 * -------------------------------------------------------
 * Created: 18/10/2026
 * -------------------------------------------------------
 */
#pragma once
#include "xp_airport_document.h"
#include "xp_apt_geometry.h"
#include <cstdint>
#include <string>
#include <string_view>

// -------------------------------------------------------

namespace SceneryEditorX::XPlane
{
	/* @brief What an airport entity stands for. */
	enum class AirportElementKind : uint8_t
	{
		Airport,			// The root entity
		Group,				// A folder entity ("Runways", "Pavement", ...)
		Runway,
		WaterRunway,
		Helipad,
		Pavement,
		LinearFeature,
		Boundary,
		Viewpoint,
		Beacon,
		Windsock,
		Sign,
		LightFixture,
		Startup,
		LegacyStartup,
		TaxiRoutes,			// Every taxi edge, as one entity
		TaxiNodes,			// Every taxi node, as one entity
		TruckParking,
		TruckDestination,
		Jetway,

		// The scenery pack's DSF overlay around the airport (one entity per asset, merged across placements)
		DrapedPolygon,		// .pol (and .agp ground tiles)
		LineString,			// .lin
		Forest,				// .for
		Facade,				// .fac
		SceneryObject,		// Group of the placed OBJ8s (DSF objects, .str strings, facade attachments)
		AirportLight,		// Group of the apt.dat light strings' library light objects
		Count
	};

	const char *GetAirportElementKindName(AirportElementKind kind);

	/**
	 * @struct AirportElementComponent
	 * @brief Registry component on every entity SceneAirports creates: which apt.dat row the entity draws.
	 * `index` is the position in the matching AptAirport vector; `subIndex` tells a feature's painted lines (0) from
	 * its light strings (1). For overlay kinds `index` is the first DSF placement merged into the entity and `subIndex`
	 * how many there are.
	 */
	struct AirportElementComponent
	{
		AirportElementKind kind = AirportElementKind::Airport;
		uint32_t index = 0;
		uint32_t subIndex = 0;
		uint64_t airportRoot = 0;		// UUID of the airport's root entity
		uint32_t triangles = 0;
		Bounds2 bounds;					// Local-space footprint, for framing
	};

	/**
	 * @class SceneAirports
	 * @brief Draws an airport document in the active scene: geometry in X-Plane's local frame (meters, +X east,
	 * -Z north) around the airport datum, one entity per element under group entities under one root. Main thread
	 * only; one airport is loaded at a time.
	 *
	 * Pavement is triangulated from the bezier outlines (later rows draw underneath, like X-Plane layers them) and,
	 * like runways and painted lines, textured with the X-Plane library's art for its surface / line code when the
	 * library is available (flat colors otherwise); light strings become the library's light objects. Point features
	 * (startup locations, signs, windsocks, ...) are small markers pointing along their heading. The scenery pack's
	 * DSF overlay around the airport is drawn too: draped polygons, lines, forests and facades as textured meshes,
	 * objects and object strings through the OBJ8 loader.
	 */
	class SceneAirports
	{
	public:
		struct Stats
		{
			uint32_t entities = 0;
			uint32_t triangles = 0;
			uint32_t parts = 0;
			uint32_t objects = 0;			// OBJ8 placements (DSF objects, strings, light fixtures)
			uint32_t overlayMeshes = 0;		// DSF polygon placements drawn (before merging per asset)
			uint32_t warnings = 0;			// Assets that couldn't be found or read (see the log)
			double buildMilliseconds = 0.0;
		};

		/**
		 * @brief Replaces the loaded airport (if any) with the document's and frames the camera on it.
		 * @return The root entity's UUID, or 0 when there is no active scene.
		 */
		static uint64_t Load(const AirportDocument &document, bool frameCamera = true);

		/* @brief Rebuilds the geometry after edits, keeping the root entity (and its selection) in place. */
		static void Rebuild(const AirportDocument &document);

		/**
		 * @brief Moves the point features (startup locations, signs, windsocks, ...) to where the document now puts
		 * them, without rebuilding meshes. Enough for position / heading edits; anything else needs Rebuild().
		 */
		static void UpdatePlacements(const AirportDocument &document);

		/* @brief True for kinds drawn as a marker placed by the entity transform (UpdatePlacements() moves them). */
		static bool IsPlacedKind(AirportElementKind kind);

		/* @brief True for the DSF overlay kinds (not apt.dat rows: nothing to edit or delete in the document). */
		static bool IsOverlayKind(AirportElementKind kind);

		/* @brief Removes the loaded airport from the scene. */
		static void Unload();

		static bool IsLoaded();
		static uint64_t GetRootId();

		/* @brief The document revision the scene reflects (UpdatePlacements / Rebuild when the document moved past it). */
		static uint64_t GetBuiltRevision();

		/* @brief The document geometry revision the meshes were built from (Rebuild when it changed). */
		static uint64_t GetBuiltGeometryRevision();

		/**
		 * @brief LibraryManager::GetGeneration() when the scene was built. Library art (pavement textures, painted
		 * lines, light objects, lib/ objects of the DSF) resolves through that index, so a newer one means a rebuild.
		 */
		static uint64_t GetBuiltLibraryGeneration();

		/**
		 * @brief The airport element an entity stands for (the entity itself or, for the root's descendants, itself).
		 * @return The component, or nullptr when the entity isn't part of the loaded airport.
		 */
		static const AirportElementComponent *FindElement(uint64_t entityId);

		/* @brief Entity drawing an element, or 0. */
		static uint64_t FindEntity(AirportElementKind kind, uint32_t index, uint32_t subIndex = 0);

		/* @brief Shows or hides every entity of a kind (taxi routes clutter a 3D view, for instance). */
		static void SetKindVisible(AirportElementKind kind, bool visible);
		static bool IsKindVisible(AirportElementKind kind);

		/* @brief Moves the camera to see the whole airport. */
		static void Frame();

		/* @brief Moves the camera to see one element. */
		static void FrameEntity(uint64_t entityId);

		/**
		 * @brief Puts the camera at an offset from the airport's center (on the ground plane) looking along a heading,
		 * for repeatable views such as a ground-level one (--xp-airport-camera).
		 * @param x, y, z Meters from the center (+X east, +Y up, +Z south).
		 * @param yawDegrees Heading of the view, the convention Frame() uses (0 looks along +Z).
		 * @param pitchDegrees Above (> 0) or below the horizon.
		 */
		static void LookFrom(float x, float y, float z, float yawDegrees, float pitchDegrees);

		/* @brief The projection the loaded airport was built with (datum at the origin). */
		static const GeoProjection &GetProjection();

		/**
		 * @brief Height above the ground for draped art of a LAYER_GROUP (name, offset), slotted between the loaded
		 * airport's own layers (pavement, runways, painted lines) the way its pack's DSF art is; usable as
		 * OverlayBuilder::Options::layerHeight for other overlays drawn with the airport. Without an airport, the
		 * heights an airport without pavement would use.
		 */
		static float GetOverlayLayerHeight(std::string_view group, int offset);

		static Stats GetStats();

		/* @brief Unloads and releases GPU resources. Call before the renderer shuts down. */
		static void Shutdown();
	};

}

// -------------------------------------------------------
