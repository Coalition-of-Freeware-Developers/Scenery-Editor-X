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
 * scene_render.cpp
 * -------------------------------------------------------
 * Created: 07/04/2026
 * -------------------------------------------------------
 */
#include "scene_render.h"
#include <SceneryEditorX/logging/asserts.h>
#include <SceneryEditorX/logging/logging.hpp>
#include <SceneryEditorX/renderer/renderer.h>
#include <SceneryEditorX/scene/entity.h>
#include <SceneryEditorX/scene/material.h>
#include <SceneryEditorX/scene/scene.h>

// -------------------------------------------------------------------

namespace SceneryEditorX
{

	Render::Render(Entity* entity) : Component(entity)
	{

	}

	Render::~Render()
	{
		m_mesh = nullptr;
	}

	void Render::Tick()
	{
		// deferred default material assignment (renderer may not be ready during load)
		if (m_needs_default_material)
		{
			if (Renderer::GetStandardMaterial())
			{
				SetDefaultMaterial();
				m_needs_default_material = false;
			}
		}

		UpdateAabb();
		UpdateFrustumAndDistanceCulling();
		UpdateLodIndices();
	}

	void Render::SetMesh(Mesh* mesh, const uint32_t sub_mesh_index)
	{
		if (!mesh)
		{
			EDITOR_ERROR_TAG("Render","Renderable::SetMesh called with null mesh");
			return;
		}

		// set mesh
		m_mesh = mesh;
		m_sub_mesh_index = sub_mesh_index;

		// compute and set bounding box (GetGeometry validates bounds internally)
		std::vector<Vertex_PosTexNorTan> vertices;
		mesh->GetGeometry(sub_mesh_index, nullptr, &vertices);
		if (!vertices.empty())
		{
			std::vector<Vec3> positions;
			positions.reserve(vertices.size());
			for (const auto& v : vertices)
				positions.emplace_back(v.pos[0], v.pos[1], v.pos[2]);
			m_bounding_box_mesh = BoundingBox(positions.data(), static_cast<uint32_t>(positions.size()));
		}

		Tick(); // update bounding boxes, frustum and distance culling
	}

	void Render::SetMesh(const MeshType type)
	{
		SetMesh(Renderer::GetStandardMesh(type)->Get());
	}

	void Render::GetGeometry(std::vector<uint32_t>* indices, std::vector<Vertex_PosTexNorTan>* vertices) const
	{
		if (!m_mesh)
		{
			EDITOR_ERROR_TAG("Render","Renderable::GetGeometry called with null mesh");
			return;
		}
		m_mesh->GetGeometry(m_sub_mesh_index, indices, vertices);
	}

	void Render::SetMaterial(const Ref<MaterialAsset>& material)
	{
		SEDX_ASSERT(material != nullptr, "Material is null");

		m_Material_Default = false;

		// store the material asset reference directly
		m_MaterialAsset = material.Get();
		if (m_MaterialAsset == nullptr)
		{
			EDITOR_ERROR_TAG("Render", "Material was unable to be set.");
			return;
		}

		// pack textures, generate mips, compress, upload to GPU
		if (m_MaterialAsset->GetResourceState() == ResourceState::MaxEnum)
		{
			m_MaterialAsset->PrepareForGPU();
		}

		// compute world dimensions (skip if no mesh is available yet, e.g. procedural meshes like roads)
		{
			std::vector<Vertex_PosTexNorTan> vertices;
			GetGeometry(nullptr, &vertices);

			if (!vertices.empty())
			{
				float height_min = FLT_MAX;
				float max_height = -FLT_MAX;
				float min_width  = FLT_MAX;
				float max_width  = -FLT_MAX;

				Matrix transform = HasInstancing() ? GetInstance(0, true) : GetEntity()->GetMatrix();
				for (const Vertex_PosTexNorTan& vertex : vertices)
				{
					Vec3 position = Vec3(vertex.pos[0], vertex.pos[1], vertex.pos[2]);
					height_min       = xMath::Min(height_min, position.y);
					max_height       = xMath::Max(max_height, position.y);
					min_width        = xMath::Min(min_width, position.x);
					max_width        = xMath::Max(max_width, position.x);
				}

				material->SetProperty(MaterialClass::SceneWidth,  max_width - min_width);
				material->SetProperty(MaterialClass::SceneHeight, max_height - height_min);
			}
		}
	}

	void Render::SetMaterial(const std::string & file_path)
	{
		auto material = CreateRef<MaterialAsset>();

		material->LoadFromFile(file_path);

		SetMaterial(material);
	}

	void Render::SetDefaultMaterial()
	{
		SetMaterial(Renderer::GetStandardMaterial());
		m_Material_Default = true;
	}

	std::string Render::GetMaterialName() const
	{
		return m_MaterialAsset ? m_MaterialAsset->GetObjectName() : "";
	}

	uint32_t Render::GetIndexOffset(const uint32_t lod) const
	{
		// global base offset + lod-relative offset within the mesh
		return m_mesh->GetGlobalIndexOffset() + m_mesh->GetSubMesh(m_sub_mesh_index).lods[lod].index_offset;
	}

	uint32_t Render::GetIndexCount(const uint32_t lod) const
	{
		return m_mesh->GetSubMesh(m_sub_mesh_index).lods[lod].index_count;
	}

	uint32_t Render::GetVertexOffset(const uint32_t lod) const
	{
		// global base offset + lod-relative offset within the mesh
		return m_mesh->GetGlobalVertexOffset() + m_mesh->GetSubMesh(m_sub_mesh_index).lods[lod].vertex_offset;
	}

	uint32_t Render::GetVertexCount(const uint32_t lod) const
	{
		return m_mesh->GetSubMesh(m_sub_mesh_index).lods[lod].vertex_count;
	}

	Buffer* Render::GetIndexBuffer() const
	{
		if (!m_mesh)
			return nullptr;

		return m_mesh->GetIndexBuffer();
	}

	Buffer* Render::GetVertexBuffer() const
	{
		if (!m_mesh)
			return nullptr;

		return m_mesh->GetVertexBuffer();
	}

	const std::string & Render::GetMeshName() const
	{
		static std::string no_mesh = "N/A";
		if (!m_mesh)
			return no_mesh;

		return m_mesh->GetObjectName();
	}

	Matrix Render::GetInstance(const uint32_t index, const bool to_world)
	{
		return to_world ? m_instances[index].GetMatrix() * GetEntity()->GetMatrix() : m_instances[index].GetMatrix();
	}

	void Render::SetInstances(const std::vector<Instance>& instances)
	{
		if (instances.empty())
		{
			m_instances.clear();
			m_instance_buffer    = nullptr;
			m_bounding_box_dirty = true;
			return;
		}

		// store instance data
		m_instances = instances;
		m_instance_buffer = CreateRef<Buffer>(
			sizeof(Instance),
			static_cast<uint32_t>(instances.size()),
			static_cast<const void*>(instances.data()),
			false,
			("instance_buffer_" + GetObjectName()).c_str()
		);

		m_bounding_box_dirty = true;
		Tick(); // update bounding boxes, frustum and distance culling
	}

	void Render::SetInstances(const std::vector<Matrix>& transforms)
	{
		if (transforms.empty())
		{
			SetInstances(std::vector<Instance>{});
			return;
		}

		// convert matrices to instances
		std::vector<Instance> instances;
		instances.reserve(transforms.size());
		for (const auto& transform : transforms)
		{
			Instance instance;
			instance.SetMatrix(transform);
			instances.emplace_back(instance);
		}

		// call instance overload
		SetInstances(instances);
	}

	uint32_t Render::GetLodCount() const
	{
		if (!m_mesh)
			return 0;

		return static_cast<uint32_t>(m_mesh->GetSubMesh(m_sub_mesh_index).lods.size());
	}

	void Render::SetFlag(const RenderableFlags flag, const bool enable /*= true*/)
	{
		bool enabled      = false;
		bool disabled     = false;
		bool flag_present = m_flags & flag;

		if (enable && !flag_present)
		{
			m_flags |= static_cast<uint32_t>(flag);
			enabled  = true;

		}
		else if (!enable && flag_present)
		{
			m_flags  &= ~static_cast<uint32_t>(flag);
			disabled  = true;
		}
	}

	void Render::UpdateAabb()
	{
		const Matrix transform = (GetEntity() && GetEntity()->GetActive()) ? GetEntity()->GetMatrix() : Matrix::IDENTITY;
		if (m_bounding_box_dirty || m_transform_previous != transform)
		{
			if (m_instances.empty()) // non-instanced
			{
				m_bounding_box = m_bounding_box_mesh * transform;
			}
			else // instanced
			{
				m_bounding_box = BoundingBox(Vec3::VEC3_INFINITY<float>, Vec3::INFINITY_NEG<float>);
				for (const Instance& instance : m_instances)
				{
					Matrix world_instance = instance.GetMatrix() * transform;
					m_bounding_box.Merge(m_bounding_box_mesh * world_instance);
				}
			}
			m_transform_previous = transform;
			m_bounding_box_dirty = false;
		}
	}

	void Render::UpdateFrustumAndDistanceCulling()
	{
		if (Camera* camera = Scene::GetCamera())
		{
			Vec3 camera_position = camera->GetEntity()->GetPosition();

			const BoundingBox& bounding_box = GetBoundingBox();

			// first, check if the bounding box is in the frustum
			if (camera->IsInViewFrustum(bounding_box))
			{
				// only if in frustum, calculate distance
				m_distance_squared = xMath::Length2(bounding_box.GetClosestPoint(camera_position) - camera_position);
				m_is_visible       = m_distance_squared <= m_max_distance_render * m_max_distance_render;
			}
			else
			{
				// outside frustum, no need for distance check
				m_is_visible = false;
			}
		}
		else
		{
			m_distance_squared = 0.0f;
			m_is_visible       = true;
		}
	}

	void Render::UpdateLodIndices()
	{
		// screen-space coverage based lod selection
		// this approach (used by unreal, unity, cryengine, frostbite) naturally handles:
		// - distance: farther objects appear smaller
		// - object size: larger objects maintain detail longer
		// - fov: wider fov = everything smaller on screen
		// - works uniformly for all object types (no special cases needed)

		const uint32_t lod_count = GetLodCount();
		if (lod_count == 0)
		{
			m_lod_index = 0;
			return;
		}

		Camera* camera = Scene::GetCamera();
		if (!camera)
		{
			m_lod_index = lod_count - 1;
			return;
		}

		const BoundingBox& box = GetBoundingBox();
		const Vec3 camera_position = camera->GetEntity()->GetPosition();

		// camera inside bounding box = maximum detail
		if (box.Contains(camera_position))
		{
			m_lod_index = 0;
			return;
		}

		// distance from camera to closest point on bounding box
		Vec3 closest_point = box.GetClosestPoint(camera_position);
		float distance = xMath::Max(xMath::Length(closest_point - camera_position), 0.001f);

		// compute screen-space coverage: fraction of vertical screen space the object covers
		// screen_fraction = (object_diameter) / (visible_height_at_distance)
		// visible_height_at_distance = 2 * distance * tan(fov_v / 2)
		float bounding_diameter = xMath::Length(box.GetExtents() * 2.0f);
		float tan_half_fov = tan(camera->GetFovVerticalRad() * 0.5f);
		float screen_fraction = bounding_diameter / (2.0f * distance * tan_half_fov);

		// lod thresholds as percentage of screen height coverage
		// calibrated so transitions remain imperceptible to the user
		// higher threshold = object must cover more screen to qualify for that lod
		static constexpr std::array<float, 5> SCREEN_THRESHOLDS =
		{
			0.05f,   // lod0: object covers >= 5% of screen height
			0.025f,  // lod1: object covers >= 2.5% of screen height
			0.012f,  // lod2: object covers >= 1.2% of screen height
			0.006f,  // lod3: object covers >= 0.6% of screen height
			0.003f   // lod4: object covers >= 0.3% of screen height
		};

		// hysteresis prevents lod popping at threshold boundaries
		// upgrading to higher detail requires exceeding threshold by 10%
		// downgrading to lower detail requires dropping 10% below threshold
		constexpr float hysteresis = 1.1f;

		uint32_t new_lod = lod_count - 1;
		for (uint32_t i = 0; i < xMath::Min(lod_count, static_cast<uint32_t>(SCREEN_THRESHOLDS.size())); i++)
		{
			float threshold = SCREEN_THRESHOLDS[i];

			// apply hysteresis based on relationship to current lod
			if (i < m_lod_index)
			{
				// upgrading to higher detail: raise the bar
				threshold *= hysteresis;
			}
			else if (i == m_lod_index)
			{
				// staying at current lod: lower the bar (easier to stay)
				threshold /= hysteresis;
			}

			if (screen_fraction >= threshold)
			{
				new_lod = i;
				break;
			}
		}

		m_lod_index = std::clamp(new_lod, 0u, lod_count - 1);
	}
}

// -------------------------------------------------------------------
