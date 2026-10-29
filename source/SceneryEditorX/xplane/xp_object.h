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
 * xp_object.h
 * -------------------------------------------------------
 * Created: 17/10/2026
 * -------------------------------------------------------
 */
#pragma once
#include "xp_assets.h"
#include "xp_lights.h"
#include "xp_obj8.h"
#include "xp_texture_loader.h"
#include <filesystem>
#include <memory>
#include <vector>
#include <SceneryEditorX/scene/material.h>
#include <SceneryEditorX/scene/mesh.h>

// -------------------------------------------------------

namespace SceneryEditorX::XPlane
{
	/**
	 * @struct ObjectBatch
	 * @brief One drawable piece of an OBJ: a sub-mesh with its animation group, LOD, material and hide/show rules.
	 */
	struct ObjectBatch
	{
		uint32_t subMesh = 0;				// Sub-mesh in ObjectAsset::GetMesh()
		int node = -1;						// Obj8Data::nodes index, -1 when static
		int lod = -1;						// Obj8Data::lods index, -1 when the object has no LODs
		uint32_t material = 0;				// ObjectAsset::GetMaterial() index
		std::vector<uint32_t> visibility;	// Obj8Data::visibility rules, in order
		uint32_t triangleCount = 0;
		bool castShadow = true;
		bool draped = false;
	};

	/**
	 * @struct ObjectLight
	 * @brief An OBJ light resolved against lights.txt: where it sits and what it draws (billboards, spill).
	 */
	struct ObjectLight
	{
		uint32_t source = 0;				// Obj8Data::lights index
		int node = -1;						// Animation group, -1 when static
		int lod = -1;
		std::vector<uint32_t> visibility;	// Obj8Data::visibility rules, in order
		float position[3] = { 0.0f, 0.0f, 0.0f };
		ResolvedLight resolved;
	};

	/* @brief A detail or normal decal of an OBJ (its rows and DECAL_LIB's), with its texture. */
	struct ObjectDecal
	{
		PolygonDecal decal;					// scaleS / scaleT: repeats per albedo tile (per unit of the OBJ's UVs)
		Ref<ImageResource> image;
	};

	/* @brief The textures an OBJ resolved, with what the loader learned about them. */
	struct ObjectTextures
	{
		Ref<ImageResource> albedo;
		Ref<ImageResource> normal;
		Ref<ImageResource> lit;
		Ref<ImageResource> drapedAlbedo;
		Ref<ImageResource> drapedNormal;
		Ref<ImageResource> weather;			// WEATHER mask (weather.slang)
		Ref<ImageResource> materialMap;		// TEXTURE_MAP material_gloss, else gloss (X-Plane 12)
		TextureInfo albedoInfo;
		TextureInfo normalInfo;
		TextureInfo litInfo;
		TextureInfo drapedInfo;
		std::vector<ObjectDecal> decals;	// Up to two detail and two normal decals, in file order
	};

	/* @brief Where an object file is in its load. */
	enum class ObjectLoadState : uint8_t
	{
		NotLoaded,		// Never requested (or it failed and left the cache)
		Queued,			// Waiting for a worker thread
		Decoding,		// On a worker: parsing the OBJ, decoding textures, building vertices
		Uploading,		// On the main thread: uploading textures and geometry, creating materials
		Ready,
		Failed
	};

	/* @brief Display name of a load state ("Queued", "Decoding", ...). */
	const char *GetObjectLoadStateName(ObjectLoadState state);

	/* @brief An object load in flight, for progress displays. */
	struct ObjectLoadProgress
	{
		std::filesystem::path path;
		ObjectLoadState state = ObjectLoadState::Queued;
		float seconds = 0.0f;			// Since it was requested
	};

	/* @brief The background object loads, in request order, and how far the current burst of requests got. */
	struct ObjectLoadQueue
	{
		std::vector<ObjectLoadProgress> loads;
		uint32_t finished = 0;			// Loads finished since the queue was last empty
		uint32_t requested = 0;			// Loads requested since the queue was last empty
	};

	struct ObjectLoadJob;

	/* @brief Per-vertex tangents from UV derivatives (s right, t up), orthogonalized against the normal, as OBJ8s get them. */
	void ComputeVertexTangents(std::vector<Vertex_PosTexNorTan> &vertices, const std::vector<uint32_t> &indices);

	/**
	 * @class ObjectAsset
	 * @brief An OBJ8 file turned into GPU resources: one mesh (a sub-mesh per batch), its materials and textures.
	 * Shared by every placement of the file (one asset per file, cached by path).
	 *
	 * Loads run in the background: Request() returns at once with an empty asset and queues the CPU work (OBJ parsing,
	 * texture decoding and mip chains, vertex and tangent building) on the thread pool, a few loads at a time, in
	 * request order; Update() then uploads the result on the main thread, a few milliseconds per frame, and the asset
	 * becomes IsReady(). Only the main thread touches assets; the getters below describe the object once it is ready
	 * (they are empty before).
	 */
	class ObjectAsset : public RefCounted
	{
	public:
		/**
		 * @brief Starts loading an OBJ8 file in the background, or returns the asset cached or already in flight for it
		 * (two placements of one file share one load). Never blocks.
		 * @param path Absolute path to the .obj file.
		 * @return The asset; check IsReady() / HasFailed().
		 */
		static Ref<ObjectAsset> Request(const std::filesystem::path &path);

		/**
		 * @brief Loads an OBJ8 file and waits for it (or returns the cached asset for it). Blocks the main thread.
		 * @param path Absolute path to the .obj file.
		 * @param error Receives the reason on failure (optional).
		 * @return The asset, or nullptr when the file can't be parsed.
		 */
		static Ref<ObjectAsset> Load(const std::filesystem::path &path, std::string *error = nullptr);

		/**
		 * @brief Finishes background loads whose CPU half is done: uploads their textures and geometry and creates their
		 * materials, within a small per-frame time budget (at least one step a frame). Main thread, once per frame.
		 */
		static void Update();

		/* @brief The loads in flight, for progress displays. */
		static ObjectLoadQueue GetLoadQueue();

		/* @brief Load state of a file (NotLoaded when it was never requested). */
		static ObjectLoadState GetLoadState(const std::filesystem::path &path);

		/* @brief Every object loaded this session (ready ones only). */
		static std::vector<Ref<ObjectAsset>> GetLoaded();

		/**
		 * @brief Releases every cached object (meshes, materials, texture references).
		 * Only call once nothing renders them any more (no placed entities, render thread stopped).
		 */
		static void ClearCache();

		/**
		 * @brief X-Plane's lights.txt, loaded on first use from the X-Plane install (or, without one, from the
		 * install an OBJ lives in). Empty when neither has it. Thread-safe (workers resolve lights with it); the
		 * catalog itself never changes, another lights.txt replaces it.
		 * @param objectPath An OBJ to search upwards from when no install is configured (optional).
		 */
		static std::shared_ptr<const LightCatalog> GetLightCatalog(const std::filesystem::path &objectPath = {});

		/* @brief The lights.txt billboard texture (nullptr when missing); info receives its size. Main thread. */
		static Ref<ImageResource> GetLightTexture(TextureInfo *info = nullptr);

		ObjectAsset() = default;
		~ObjectAsset() override;

		[[nodiscard]] ObjectLoadState GetState() const;
		[[nodiscard]] bool IsReady() const { return m_State == ObjectLoadState::Ready; }
		[[nodiscard]] bool HasFailed() const { return m_State == ObjectLoadState::Failed; }
		[[nodiscard]] const std::string &GetError() const { return m_Error; }

		[[nodiscard]] const std::filesystem::path &GetPath() const { return m_Path; }
		/* @brief The parsed OBJ without its vertex tables (vertices, indices, line and light vertices): GetMesh() has the geometry. */
		[[nodiscard]] const Obj8Data &GetData() const { return m_Data; }
		[[nodiscard]] Mesh *GetMesh() const { return m_Mesh.Get(); }
		[[nodiscard]] const std::vector<ObjectBatch> &GetBatches() const { return m_Batches; }
		[[nodiscard]] uint32_t GetMaterialCount() const { return static_cast<uint32_t>(m_Materials.size()); }
		[[nodiscard]] MaterialAsset *GetMaterial(const uint32_t index) const { return index < m_Materials.size() ? m_Materials[index].Get() : nullptr; }
		[[nodiscard]] const ObjectTextures &GetTextures() const { return m_Textures; }

		/* @brief Lights resolved against lights.txt (unknown names are left out). */
		[[nodiscard]] const std::vector<ObjectLight> &GetLights() const { return m_Lights; }
		[[nodiscard]] uint32_t GetBillboardCount() const;
		[[nodiscard]] uint32_t GetSpillCount() const;

		/* @brief Static (unanimated) bounds of all geometry and lights, in object space. */
		[[nodiscard]] const float *GetBoundsMin() const { return m_BoundsMin; }
		[[nodiscard]] const float *GetBoundsMax() const { return m_BoundsMax; }

		/* @brief Time the load worked: on the worker plus uploading on the main thread (not the time it queued). */
		[[nodiscard]] float GetLoadMilliseconds() const { return m_LoadMilliseconds; }

	private:
		bool FinishStep();
		void FinishGeometry();
		void Fail();
		uint32_t GetOrCreateMaterial(const Obj8DrawState &state);

		std::filesystem::path m_Path;
		ObjectLoadState m_State = ObjectLoadState::NotLoaded;
		std::string m_Error;
		std::shared_ptr<ObjectLoadJob> m_Job;	// The load in flight (shared with its worker)
		Obj8Data m_Data;
		Ref<Mesh> m_Mesh;					// Null for objects that only carry lights
		std::vector<ObjectBatch> m_Batches;
		std::vector<ObjectLight> m_Lights;
		std::vector<Ref<MaterialAsset>> m_Materials;
		std::vector<Obj8DrawState> m_MaterialStates;	// State each material was made for
		ObjectTextures m_Textures;
		float m_BoundsMin[3] = { 0.0f, 0.0f, 0.0f };
		float m_BoundsMax[3] = { 0.0f, 0.0f, 0.0f };
		float m_LoadMilliseconds = 0.0f;
	};

}

// -------------------------------------------------------
