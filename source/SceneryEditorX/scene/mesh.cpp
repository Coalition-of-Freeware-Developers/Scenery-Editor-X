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
#include "mesh.h"
#include "entity.h"
#include "SceneryEditorX/core/time/timer.h"
#include <bounding_box.h>
#include <SceneryEditorX/logging/asserts.h>
#include <SceneryEditorX/renderer/gbuffer.h>
#include <algorithm>
#include <cctype>
#include <filesystem>
#include <limits>

// -------------------------------------------------------

namespace
{
	constexpr const char* MeshLogTag = "Mesh";

	bool IsNativeMeshFilePath(const std::string& file_path)
	{
		std::string extension = std::filesystem::path(file_path).extension().string();
		std::transform(extension.begin(), extension.end(), extension.begin(), [](const unsigned char c) { return static_cast<char>(std::tolower(c)); });
		return extension == ".mesh" || extension == ".sedxmesh";
	}

	xMath::BoundingBox ComputeBoundingBox(const std::vector<SceneryEditorX::Vertex_PosTexNorTan>& vertices)
	{
		if (vertices.empty())
		{
			return xMath::BoundingBox(Vec3(0.0f, 0.0f, 0.0f), Vec3(0.0f, 0.0f, 0.0f));
		}

		Vec3 min_point(std::numeric_limits<float>::max(), std::numeric_limits<float>::max(), std::numeric_limits<float>::max());
		Vec3 max_point(std::numeric_limits<float>::lowest(), std::numeric_limits<float>::lowest(), std::numeric_limits<float>::lowest());

		for (const auto& vertex : vertices)
		{
			const Vec3 position(vertex.pos[0], vertex.pos[1], vertex.pos[2]);
			min_point.x = std::min(min_point.x, position.x);
			min_point.y = std::min(min_point.y, position.y);
			min_point.z = std::min(min_point.z, position.z);
			max_point.x = std::max(max_point.x, position.x);
			max_point.y = std::max(max_point.y, position.y);
			max_point.z = std::max(max_point.z, position.z);
		}

		return xMath::BoundingBox(min_point, max_point);
	}
}

// -------------------------------------------------------

namespace SceneryEditorX
{
	Mesh::Mesh() : SharedResource(ResourceType::Mesh)
	{
		m_Flags = GetDefaultFlags();
	}

	void Mesh::Clear()
	{
		m_indices.clear();
		m_indices.shrink_to_fit();

		m_vertices.clear();
		m_vertices.shrink_to_fit();
	}

	void Mesh::SaveToFile(const std::string & file_path)
	{
		std::ofstream outfile(file_path, std::ios::binary);
		if (!outfile)
		{
			SEDX_CORE_ERROR_TAG("Mesh","Failed to open file for writing: %s", file_path.c_str());
			return;
		}

		uint32_t version = 1;
		outfile.write(reinterpret_cast<const char*>(&version), sizeof(uint32_t));

		uint32_t type = static_cast<uint32_t>(m_type);
		outfile.write(reinterpret_cast<const char*>(&type), sizeof(uint32_t));

		// legacy field for backward compatibility (previously stored lod curve type)
		uint32_t legacy_field = 0;
		outfile.write(reinterpret_cast<const char*>(&legacy_field), sizeof(uint32_t));

		outfile.write(reinterpret_cast<const char*>(&m_Flags), sizeof(uint32_t));

		uint32_t submesh_count = static_cast<uint32_t>(m_sub_meshes.size());
		outfile.write(reinterpret_cast<const char*>(&submesh_count), sizeof(uint32_t));

		for (uint32_t sub_idx = 0; sub_idx < submesh_count; sub_idx++)
		{
			const LodLevels& sub = m_sub_meshes[sub_idx];
			uint32_t lod_count = static_cast<uint32_t>(sub.lods.size());
			outfile.write(reinterpret_cast<const char*>(&lod_count), sizeof(uint32_t));
			SEDX_CORE_INFO_TAG("Mesh","Mesh '%s' sub-mesh %u: saving %u LODs", m_ObjectName.c_str(), sub_idx, lod_count);

			for (const auto& lod : sub.lods)
			{
				outfile.write(reinterpret_cast<const char*>(&lod.vertex_offset), sizeof(uint32_t));
				outfile.write(reinterpret_cast<const char*>(&lod.vertex_count), sizeof(uint32_t));
				outfile.write(reinterpret_cast<const char*>(&lod.index_offset), sizeof(uint32_t));
				outfile.write(reinterpret_cast<const char*>(&lod.index_count), sizeof(uint32_t));

				Vec3 min = lod.aabb.GetMin();
				Vec3 max = lod.aabb.GetMax();
				outfile.write(reinterpret_cast<const char*>(&min.x), sizeof(float));
				outfile.write(reinterpret_cast<const char*>(&min.y), sizeof(float));
				outfile.write(reinterpret_cast<const char*>(&min.z), sizeof(float));
				outfile.write(reinterpret_cast<const char*>(&max.x), sizeof(float));
				outfile.write(reinterpret_cast<const char*>(&max.y), sizeof(float));
				outfile.write(reinterpret_cast<const char*>(&max.z), sizeof(float));
			}
		}

		uint32_t vertex_count = static_cast<uint32_t>(m_vertices.size());
		outfile.write(reinterpret_cast<const char*>(&vertex_count), sizeof(uint32_t));
		outfile.write(reinterpret_cast<const char*>(m_vertices.data()), vertex_count * sizeof(Vertex_PosTexNorTan));

		uint32_t index_count = static_cast<uint32_t>(m_indices.size());
		outfile.write(reinterpret_cast<const char*>(&index_count), sizeof(uint32_t));
		outfile.write(reinterpret_cast<const char*>(m_indices.data()), index_count * sizeof(uint32_t));

		outfile.close();
	}

	void Mesh::LoadFromFile(const std::string & file_path)
	{
		Timer timer;
		SetResourceFilePath(file_path);

	    if (IO::FileManager::IsModel(file_path))
		{
		  SEDX_CORE_ERROR_TAG("Mesh", "Foreign model import is not available in Mesh::LoadFromFile yet: {}", file_path);
			return;
		}

		if (!IsNativeMeshFilePath(file_path))
		{
			SEDX_CORE_ERROR_TAG("Mesh", "Failed to load mesh {}: unsupported file extension", file_path);
			return;
		}

		std::ifstream infile(file_path, std::ios::binary);
		if (!infile)
		{
			SEDX_CORE_ERROR_TAG("Mesh", "Failed to open file: {}", file_path);
			return;
		}

		Clear();

		uint32_t version;
		infile.read(reinterpret_cast<char*>(&version), sizeof(uint32_t));
		if (version != 1)
		{
			SEDX_CORE_ERROR_TAG("Mesh", "Version mismatch for file: {}", file_path);
			return;
		}

		uint32_t type;
		infile.read(reinterpret_cast<char*>(&type), sizeof(uint32_t));
		m_type = static_cast<MeshType>(type);

		// legacy field for backward compatibility (skip)
		uint32_t legacy_field;
		infile.read(reinterpret_cast<char*>(&legacy_field), sizeof(uint32_t));

		infile.read(reinterpret_cast<char*>(&m_Flags), sizeof(uint32_t));

		uint32_t submesh_count;
		infile.read(reinterpret_cast<char*>(&submesh_count), sizeof(uint32_t));
		m_sub_meshes.resize(submesh_count);

		for (uint32_t sub_idx = 0; sub_idx < submesh_count; sub_idx++)
		{
			LodLevels& sub = m_sub_meshes[sub_idx];
			uint32_t lod_count;
			infile.read(reinterpret_cast<char*>(&lod_count), sizeof(uint32_t));
			sub.lods.resize(lod_count);
			SEDX_CORE_INFO("Mesh '{}' sub-mesh {}: loaded {} LODs", m_ObjectName, sub_idx, lod_count);

			for (auto& lod : sub.lods)
			{
				infile.read(reinterpret_cast<char*>(&lod.vertex_offset), sizeof(uint32_t));
				infile.read(reinterpret_cast<char*>(&lod.vertex_count), sizeof(uint32_t));
				infile.read(reinterpret_cast<char*>(&lod.index_offset), sizeof(uint32_t));
				infile.read(reinterpret_cast<char*>(&lod.index_count), sizeof(uint32_t));

				float min_x, min_y, min_z, max_x, max_y, max_z;
				infile.read(reinterpret_cast<char*>(&min_x), sizeof(float));
				infile.read(reinterpret_cast<char*>(&min_y), sizeof(float));
				infile.read(reinterpret_cast<char*>(&min_z), sizeof(float));
				infile.read(reinterpret_cast<char*>(&max_x), sizeof(float));
				infile.read(reinterpret_cast<char*>(&max_y), sizeof(float));
				infile.read(reinterpret_cast<char*>(&max_z), sizeof(float));

				lod.aabb = xMath::BoundingBox(Vec3(min_x, min_y, min_z), Vec3(max_x, max_y, max_z));
			}
		}

		uint32_t vertex_count;
		infile.read(reinterpret_cast<char*>(&vertex_count), sizeof(uint32_t));
		m_vertices.resize(vertex_count);
		infile.read(reinterpret_cast<char*>(m_vertices.data()), vertex_count * sizeof(Vertex_PosTexNorTan));

		uint32_t index_count;
		infile.read(reinterpret_cast<char*>(&index_count), sizeof(uint32_t));
		m_indices.resize(index_count);
		infile.read(reinterpret_cast<char*>(m_indices.data()), index_count * sizeof(uint32_t));

		infile.close();

		CreateGpuBuffers();

		// compute memory usage
		m_ObjectSize  = m_vertices.size() * sizeof(Vertex_PosTexNorTan);
		m_ObjectSize += m_indices.size() * sizeof(uint32_t);

		SEDX_CORE_INFO("Loading '{}' took {} ms", IO::FileSystem::GetFileNameFromFilePath(file_path), static_cast<int>(timer.ElapsedMillis()));
	}

	uint32_t Mesh::GetFlags() const
	{
		return m_Flags;
	}

	void Mesh::SetFlags(const uint32_t flags)
	{
		m_Flags = flags;
	}

	uint32_t Mesh::GetMemoryUsage() const
	{
		uint32_t size  = 0;
		size          += uint32_t(m_indices.size()  * sizeof(uint32_t));
		size          += uint32_t(m_vertices.size() * sizeof(Vertex_PosTexNorTan));

		return size;
	}

	void Mesh::GetGeometry(uint32_t sub_mesh_index, std::vector<uint32_t>* indices, std::vector<Vertex_PosTexNorTan>* vertices)
	{
		SEDX_CORE_ASSERT(indices != nullptr || vertices != nullptr, "Indices and vertices vectors can't both be null");

		// validate sub-mesh index
		if (sub_mesh_index >= m_sub_meshes.size())
		{
			SEDX_CORE_ERROR_TAG("Mesh", "GetGeometry: sub_mesh_index {} out of bounds (mesh has {} sub-meshes)", sub_mesh_index, m_sub_meshes.size());
			return;
		}

		const LodLevels& sub_mesh = m_sub_meshes[sub_mesh_index];
		if (sub_mesh.lods.empty())
		{
			SEDX_CORE_ERROR_TAG("Mesh", "GetGeometry: sub-mesh {} has no LODs", sub_mesh_index);
			return;
		}

		const MeshLod& lod = sub_mesh.lods[0];

		if (indices)
		{
			SEDX_CORE_ASSERT(lod.index_count != 0, "Index count can't be 0");

			indices->resize(lod.index_count); // allocate once (caller can reuse buffer)
			std::copy_n(m_indices.begin() + lod.index_offset, lod.index_count, indices->begin());
		}

		if (vertices)
		{
			SEDX_CORE_ASSERT(lod.vertex_count != 0, "Vertex count can't be 0");

			vertices->resize(lod.vertex_count); // allocate once (caller can reuse buffer)
		   std::copy_n(m_vertices.begin() + lod.vertex_offset, lod.vertex_count, vertices->begin());
		}
	}

	void Mesh::AddLod(std::vector<Vertex_PosTexNorTan>& vertices, std::vector<uint32_t>& indices, const uint32_t sub_mesh_index)
	{
		// build lod
		MeshLod lod;
		lod.vertex_offset = static_cast<uint32_t>(m_vertices.size());
		lod.vertex_count  = static_cast<uint32_t>(vertices.size());
		lod.index_offset  = static_cast<uint32_t>(m_indices.size());
		lod.index_count   = static_cast<uint32_t>(indices.size());
		lod.aabb          = ComputeBoundingBox(vertices);

		// append geometry
		{
			std::scoped_lock lock(m_mutex);

			// append geometry to mesh buffers
			m_vertices.insert(m_vertices.end(), vertices.begin(), vertices.end());
			m_indices.insert(m_indices.end(), indices.begin(), indices.end());

			// add lod to the specified sub-mesh
			m_sub_meshes[sub_mesh_index].lods.push_back(lod);
		}
	}

	void Mesh::AddGeometry(std::vector<Vertex_PosTexNorTan>& vertices, std::vector<uint32_t>& indices, const bool generateLods, uint32_t* sub_mesh_index)
	{
		// create a sub-mesh
		LodLevels sub_mesh;
		uint32_t current_sub_mesh_index = static_cast<uint32_t>(m_sub_meshes.size());
		m_sub_meshes.push_back(sub_mesh); // add it to the list so AddLod() can access it

		// lod 0: original geometry
		{
			// optimize original geometry if flagged
			if (m_Flags & static_cast<uint32_t>(MeshFlags::PostProcessOptimize))
			{
			   // Mesh optimization is currently not wired in this build.
			}

			// add the original geometry as lod 0
			AddLod(vertices, indices, current_sub_mesh_index);
		}

		// generate additional lods if requested
		if (generateLods && (m_Flags & static_cast<uint32_t>(MeshFlags::PostProcessGenerateLods)))
		{
			// screen coverage thresholds from renderable::update_lod_indices()
			// these define at what screen fraction each lod becomes active
			// lod generation targets are derived directly from these to ensure
			// simplification is matched to runtime selection - the user should never
			// see low quality geometry up close, yet we render minimum triangles
			static constexpr std::array<float, MESH_LOD_COUNT> SCREEN_THRESHOLDS =
			{
				0.05f,   // lod0: object covers >= 5% of screen height
				0.025f,  // lod1: object covers >= 2.5%
				0.012f,  // lod2: object covers >= 1.2%
				0.006f,  // lod3: object covers >= 0.6%
				0.003f   // lod4: object covers >= 0.3%
			};

			size_t original_index_count = indices.size();

			// start with lod0 geometry for progressive simplification
			std::vector<Vertex_PosTexNorTan> prev_vertices = vertices;
			std::vector<uint32_t> prev_indices                 = indices;

			/*
			for (uint32_t lod_level = 1; lod_level < MESH_LOD_COUNT; lod_level++)
			{
				// use previous lod as starting point for simplification
				std::vector<Vertex_PosTexNorTan> lod_vertices = prev_vertices;
				std::vector<uint32_t> lod_indices                 = prev_indices;

				// geometry too simple to benefit from further simplification
				if (lod_indices.size() <= 64)
					break;

				// compute optimal simplification target from screen coverage ratio
				// since visible detail scales with screen coverage, and we want
				// imperceptible quality loss, we use: target = coverage / base_coverage
				// this gives ~2x reduction per lod, matching the ~2x screen size steps
				float coverage      = SCREEN_THRESHOLDS[lod_level];
				float base_coverage = SCREEN_THRESHOLDS[0];
				float target_ratio  = coverage / base_coverage;

				// apply target relative to original mesh (not previous lod)
				// this ensures consistent quality targets regardless of actual achieved reduction
				size_t target_index_count = xMath::Max(static_cast<size_t>(64), static_cast<size_t>(original_index_count * target_ratio));

				// simplify geometry
				bool preserve_uvs   = true;
				bool preserve_edges = m_Flags & static_cast<uint32_t>(MeshFlags::PostProcessPreserveTerrainEdges);
				geometry_processing::simplify(lod_indices, lod_vertices, target_index_count, preserve_uvs, preserve_edges);

				// stop if simplification couldn't reduce complexity further
				if (lod_indices.size() >= prev_indices.size())
					break;

				// add simplified geometry as new lod
				AddLod(lod_vertices, lod_indices, current_sub_mesh_index);

				// update for next iteration
				prev_vertices = std::move(lod_vertices);
				prev_indices  = std::move(lod_indices);
			}
			*/
		}

		// return the sub-mesh index if requested
		if (sub_mesh_index)
		{
			*sub_mesh_index = current_sub_mesh_index;
		}
	}

	uint32_t Mesh::GetVertexCount() const
	{
		return static_cast<uint32_t>(m_vertices.size());
	}

	uint32_t Mesh::GetIndexCount() const
	{
		return static_cast<uint32_t>(m_indices.size());
	}

	uint32_t Mesh::GetDefaultFlags()
	{
		return
			static_cast<uint32_t>(MeshFlags::ImportRemoveRedundantData) |
			static_cast<uint32_t>(MeshFlags::PostProcessNormalizeScale) |
			static_cast<uint32_t>(MeshFlags::PostProcessOptimize)       |
			static_cast<uint32_t>(MeshFlags::PostProcessGenerateLods);
	}

	void Mesh::CreateGpuBuffers()
	{
		m_global_vertex_offset = 0;
		m_global_index_offset  = 0;

		// normalize scale
		if (m_Flags & static_cast<uint32_t>(MeshFlags::PostProcessNormalizeScale))
		{
			if (m_root_entity && !m_vertices.empty())
			{
				xMath::BoundingBox bounding_box = ComputeBoundingBox(m_vertices);
				float scale_offset     = Length(bounding_box.GetExtents());
				if (scale_offset > std::numeric_limits<float>::epsilon())
				{
					const float normalized_scale = 1.0f / scale_offset;
					m_root_entity->SetScale(Vec3(normalized_scale, normalized_scale, normalized_scale));
				}
			}
		}
	}

	void Mesh::BuildAccelerationStructure(CommandList* cmd_list, bool allow_update)
	{
		(void)cmd_list;
		(void)allow_update;
	}

	Buffer* Mesh::GetVertexBuffer()
	{
		return GeometryBuffer::GetVertexBuffer();
	}

	Buffer* Mesh::GetIndexBuffer()
	{
		return GeometryBuffer::GetIndexBuffer();
	}

}

// -------------------------------------------------------
