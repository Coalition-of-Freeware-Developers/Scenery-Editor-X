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
 * xp_scene.h
 * -------------------------------------------------------
 * Created: 17/10/2026
 * -------------------------------------------------------
 */
#pragma once
#include "xp_object.h"
#include <filesystem>
#include <string>
#include <vector>
#include <entt/entity/fwd.hpp>

// -------------------------------------------------------

namespace SceneryEditorX
{
	class Entity;
}

namespace SceneryEditorX::XPlane
{
	/**
	 * @struct XPlaneObjectComponent
	 * @brief Registry component on the root entity of a placed X-Plane object.
	 *
	 * A placed OBJ is a small entity tree: the root carries the placement transform, every animation group is a
	 * child entity whose transform is re-evaluated from datarefs each frame, and every batch is a leaf entity with
	 * a Renderable. Batches are switched on and off for LODs and ANIM_hide / ANIM_show.
	 *
	 * Lights follow their animation group the same way: each spill is a leaf entity with a Light component, and
	 * billboards are evaluated every frame from the group's world transform and handed to the renderer. Both only
	 * show at night (or with DatarefEnvironment::forceLightsOn).
	 *
	 * Assets load in the background, so a fresh placement is only the root (a placeholder) until its asset is ready;
	 * SceneObjects::Tick() then builds the tree under it.
	 */
	struct XPlaneObjectComponent
	{
		/* @brief Which ObjectAsset light (and which of its spills) a spill entity draws. */
		struct SpillSource
		{
			uint32_t light = 0;
			uint32_t spill = 0;
		};

		Ref<ObjectAsset> asset;					// May still be loading (see loaded)
		std::string virtualPath;				// Library path when placed from a library ("lib/airport/...")
		bool loaded = false;					// The asset arrived and the entity tree below the root is built
		std::vector<uint64_t> nodeEntities;		// UUID per Obj8Data::nodes entry
		std::vector<uint64_t> batchEntities;	// UUID per ObjectAsset batch
		std::vector<uint64_t> spillEntities;	// UUID per spill light (Light component)
		std::vector<SpillSource> spillSources;	// Parallel to spillEntities

		// The scene-owned entities behind the UUIDs above, resolved again only after an entity was destroyed
		// (Scene::GetDestroyVersion): looking them up by UUID every frame was most of Tick()'s cost. Null when gone.
		Entity *rootStored = nullptr;
		std::vector<entt::entity> nodeHandles;	// Parallel to nodeEntities
		std::vector<Entity *> nodeStored;		// Parallel to nodeEntities
		std::vector<Entity *> batchStored;		// Parallel to batchEntities
		std::vector<Entity *> spillStored;		// Parallel to spillEntities
		uint64_t resolvedDestroyVersion = 0;
		bool spillsDark = false;				// Lights are off and every spill entity is switched off

		// What makes the placement's batch states change, worked out when its tree is built. A placement with
		// neither is evaluated once per change of visible / lodOverride and otherwise skipped by Tick().
		bool dynamicState = true;				// Animation groups or hide / show rules: evaluated every frame
		bool usesDistance = true;				// LODs (batches or lights): re-evaluated when the camera moves
		uint64_t evaluatedStamp = 0;			// SceneObjects' camera stamp it was last evaluated at
		bool evaluatedVisible = false;			// visible / lodOverride it was last evaluated with
		int evaluatedLodOverride = 0;


		bool logPlacement = true;				// Log placing and building it (off for the thousands an airport places)

		bool visible = true;
		bool animate = true;					// Off: groups hold their last pose
		int lodOverride = -1;					// -1 = by camera distance (like X-Plane), -2 = every LOD, n = only LOD n

		// Updated every frame for the UI
		float cameraDistance = 0.0f;
		uint32_t visibleBatches = 0;
		uint32_t visibleTriangles = 0;
		uint32_t visibleBillboards = 0;
		uint32_t litSpills = 0;
	};

	/**
	 * @class SceneObjects
	 * @brief Places X-Plane objects in the active scene and drives their animation, LODs and lighting.
	 * Main thread only. Objects load in the background (ObjectAsset::Request()); placing never waits for them.
	 */
	class SceneObjects
	{
	public:
		struct PlaceOptions
		{
			float position[3] = { 0.0f, 0.0f, 0.0f };
			float headingDegrees = 0.0f;	// Clockwise from north (-Z), like DSF placements
			std::string virtualPath;		// Shown in the UI and kept with the placement
			std::string name;				// Entity name; the file name when empty
			bool frameCamera = true;		// Move the camera so the object fills the view (once it has loaded)
		};

		/**
		 * @brief Places an OBJ8 file in the active scene without waiting for it to load.
		 * The root entity exists at once (placement transform, name); the meshes, animation groups and lights appear
		 * below it when the asset is ready. A file that fails to load logs the reason and its root is removed.
		 * @param file The .obj file.
		 * @param options Placement.
		 * @param error Receives the reason on failure (optional).
		 * @return The root entity's UUID, or 0 when nothing could be placed (no active scene).
		 */
		static uint64_t Place(const std::filesystem::path &file, const PlaceOptions &options, std::string *error = nullptr);

		/**
		 * @brief Finishes background loads (ObjectAsset::Update()) and builds the placements waiting for them, then
		 * advances the dataref clock and applies animation, LOD selection, hide / show rules and night lighting
		 * (LIT textures, light billboards, spill lights, the renderer's daylight) to every placed object.
		 * Call once per frame before the renderer collects draw calls.
		 */
		static void Tick(double deltaSeconds);

		/**
		 * @brief The placed object an entity belongs to (the entity itself or one of its ancestors).
		 * @param entityId Any entity UUID.
		 * @param rootId Receives the object's root UUID (optional).
		 * @return The object's component, or nullptr when the entity isn't part of an X-Plane object.
		 */
		static XPlaneObjectComponent *FindObject(uint64_t entityId, uint64_t *rootId = nullptr);

		/* @brief Moves the camera so a placed object fills the view (when it has loaded; the latest request wins). */
		static void FrameObject(uint64_t rootId);

		/* @brief Number of placed objects in the active scene. */
		static size_t GetObjectCount();

		/* @brief Number of placed objects still waiting for their asset. */
		static size_t GetLoadingObjectCount();

		/**
		 * @brief Removes every placed object and releases the object and texture caches. Call during shutdown, after the
		 * render thread stopped and before the renderer and device go away (their images would otherwise leak).
		 */
		static void Shutdown();
	};

}

// -------------------------------------------------------
