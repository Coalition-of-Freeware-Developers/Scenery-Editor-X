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
 * mesh.h
 * -------------------------------------------------------
 * Created: 18/03/2026
 * -------------------------------------------------------
 */
#pragma once
#include "SceneryEditorX/asset/model/mesh_gen.h"


#include <SceneryEditorX/renderer/vulkan/vertex.h>
#include <SceneryEditorX/utils/inheritance.h>

// -------------------------------------------------------

namespace SceneryEditorX
{
	class Entity;
	class CommandList;
	class Buffer;

	/**
	 * @struct MeshLod
	 * @brief 
	 */
	struct MeshLod
	{
		uint32_t vertex_offset;  // starting offset in m_vertices
		uint32_t vertex_count;   // number of vertices for this LOD
		uint32_t index_offset;   // starting offset in m_indices
		uint32_t index_count;    // number of indices for this LOD
		xMath::BoundingBox aabb; // bounding box of this LOD
	};

	static const uint32_t MESH_LOD_COUNT = 5;

	/**
	 * @struct LodLevels
	 * @brief 
	 */
	struct LodLevels
	{
		std::vector<MeshLod> lods; // list of LOD levels for this sub-mesh
	};

	/**
	 * @enum MeshFlags
	 * @brief 
	 */
	enum class MeshFlags : uint32_t
	{
		ImportRemoveRedundantData       = BIT(0),
		ImportLights                    = BIT(1),
		ImportCombineMeshes             = BIT(2),
		PostProcessNormalizeScale       = BIT(3),
		PostProcessOptimize             = BIT(4),
		PostProcessGenerateLods         = BIT(5),
		PostProcessPreserveTerrainEdges = BIT(6),
	};

	/**
	 * @class Mesh
	 * @brief Stub mesh class providing the interface consumed by renderer passes.
	 */
	class Mesh : public SharedResource
	{
	public:
		Mesh();
		virtual ~Mesh() = default;
		static Mesh *Get() { return {}; }

		// geometry
		void Clear();
		void GetGeometry(uint32_t sub_mesh_index, std::vector<uint32_t>* indices, std::vector<Vertex_PosTexNorTan>* vertices);
		uint32_t GetMemoryUsage() const;
		void AddLod(std::vector<Vertex_PosTexNorTan>& vertices, std::vector<uint32_t>& indices, const uint32_t sub_mesh_index);
		void AddGeometry(std::vector<Vertex_PosTexNorTan>& vertices, std::vector<uint32_t>& indices, const bool generateLods, uint32_t* sub_mesh_index = nullptr);
		std::vector<Vertex_PosTexNorTan>& GetVertices()   { return m_vertices; }
		std::vector<uint32_t>& GetIndices()                   { return m_indices; }
		const LodLevels& GetSubMesh(const uint32_t index) const { return m_sub_meshes[index]; }

		// get counts
		virtual uint32_t GetVertexCount() const;
		virtual uint32_t GetIndexCount() const;

		// gpu buffers
		void CreateGpuBuffers();
		void BuildAccelerationStructure(CommandList* cmd_list, bool allow_update = false);
		virtual Buffer* GetIndexBuffer();
		virtual Buffer* GetVertexBuffer();

		// global geometry buffer offsets
		uint32_t GetGlobalVertexOffset() const { return m_global_vertex_offset; }
		uint32_t GetGlobalIndexOffset() const  { return m_global_index_offset; }

		// root entity
		Entity* GetRootEntity() { return m_root_entity; }
		void SetRootEntity(Entity* entity) { m_root_entity = entity; }

		// mesh type
		MeshType GetType() const          { return m_type; }
		void SetType(const MeshType type) { m_type = type; }

		uint32_t GetFlags() const override;
		void SetFlags(const uint32_t flags) override;

		void SaveToFile(const std::string &filePath) override;
		void LoadFromFile(const std::string &filePath) override;
		static uint32_t GetDefaultFlags();

	private:
		// geometry
		std::vector<Vertex_PosTexNorTan> m_vertices;	// all vertices of a model file
		std::vector<uint32_t> m_indices;                // all indices of a model file
		std::vector<LodLevels> m_sub_meshes;              // tracks sub-meshes and lods within the above vectors

		// global geometry buffer offsets (base offsets into the shared vertex/index buffers)
		uint32_t m_global_vertex_offset = 0;
		uint32_t m_global_index_offset  = 0;

		// misc
		std::mutex m_mutex;
		Entity* m_root_entity = nullptr;
		MeshType m_type       = MeshType::MaxEnum;
	};

}

// -------------------------------------------------------
