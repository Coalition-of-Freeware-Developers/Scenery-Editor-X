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
#include "xp_obj8.h"
#include "xp_texture_loader.h"
#include <filesystem>
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

	/* @brief The textures an OBJ resolved, with what the loader learned about them. */
	struct ObjectTextures
	{
		Ref<ImageResource> albedo;
		Ref<ImageResource> normal;
		Ref<ImageResource> lit;
		Ref<ImageResource> packed;			// Occlusion / roughness / metalness built from the normal map
		Ref<ImageResource> drapedAlbedo;
		Ref<ImageResource> drapedNormal;
		TextureInfo albedoInfo;
		TextureInfo normalInfo;
		TextureInfo litInfo;
		TextureInfo drapedInfo;
	};

	/**
	 * @class ObjectAsset
	 * @brief An OBJ8 file turned into GPU resources: one mesh (a sub-mesh per batch), its materials and textures.
	 * Shared by every placement of the file; build and upload happen on the main thread.
	 */
	class ObjectAsset : public RefCounted
	{
	public:
		/**
		 * @brief Loads an OBJ8 file (or returns the cached asset for it).
		 * @param path Absolute path to the .obj file.
		 * @param error Receives the reason on failure (optional).
		 * @return The asset, or nullptr when the file can't be parsed.
		 */
		static Ref<ObjectAsset> Load(const std::filesystem::path &path, std::string *error = nullptr);

		/* @brief Every object loaded this session. */
		static std::vector<Ref<ObjectAsset>> GetLoaded();

		/**
		 * @brief Releases every cached object (meshes, materials, texture references).
		 * Only call once nothing renders them any more (no placed entities, render thread stopped).
		 */
		static void ClearCache();

		/* @brief Shows or hides LIT textures on every loaded object (night lighting). */
		static void SetLitTexturesVisible(bool visible);
		static bool AreLitTexturesVisible();

		ObjectAsset() = default;
		~ObjectAsset() override;

		[[nodiscard]] const std::filesystem::path &GetPath() const { return m_Path; }
		[[nodiscard]] const Obj8Data &GetData() const { return m_Data; }
		[[nodiscard]] Mesh *GetMesh() const { return m_Mesh.Get(); }
		[[nodiscard]] const std::vector<ObjectBatch> &GetBatches() const { return m_Batches; }
		[[nodiscard]] uint32_t GetMaterialCount() const { return static_cast<uint32_t>(m_Materials.size()); }
		[[nodiscard]] MaterialAsset *GetMaterial(const uint32_t index) const { return index < m_Materials.size() ? m_Materials[index].Get() : nullptr; }
		[[nodiscard]] const ObjectTextures &GetTextures() const { return m_Textures; }

		/* @brief Static (unanimated) bounds of all geometry, in object space. */
		[[nodiscard]] const float *GetBoundsMin() const { return m_BoundsMin; }
		[[nodiscard]] const float *GetBoundsMax() const { return m_BoundsMax; }

		[[nodiscard]] float GetLoadMilliseconds() const { return m_LoadMilliseconds; }

	private:
		bool Build(const std::filesystem::path &path, std::string *error);
		void LoadTextures();
		uint32_t GetOrCreateMaterial(const Obj8DrawState &state);
		void ApplyLit(bool visible);

		std::filesystem::path m_Path;
		Obj8Data m_Data;
		Ref<Mesh> m_Mesh;
		std::vector<ObjectBatch> m_Batches;
		std::vector<Ref<MaterialAsset>> m_Materials;
		std::vector<Obj8DrawState> m_MaterialStates;	// State each material was made for
		ObjectTextures m_Textures;
		float m_BoundsMin[3] = { 0.0f, 0.0f, 0.0f };
		float m_BoundsMax[3] = { 0.0f, 0.0f, 0.0f };
		float m_LoadMilliseconds = 0.0f;
	};

}

// -------------------------------------------------------
