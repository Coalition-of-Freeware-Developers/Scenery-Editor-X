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
 * renderable.cpp
 * -------------------------------------------------------
 * Created: 20/03/2026
 * -------------------------------------------------------
 */
#include "renderable.h"
#include "SceneryEditorX/core/resource/resource_cache.h"
#include "SceneryEditorX/renderer/renderer.h"
#include "SceneryEditorX/scene/entity.h"
#include "SceneryEditorX/scene/material.h"
#include "SceneryEditorX/scene/scene.h"

// -------------------------------------------------------

namespace SceneryEditorX
{
	Renderable::Renderable(Entity* entity) : Component(entity)
	{
		/*SP_REGISTER_ATTRIBUTE_VALUE_VALUE(m_material_default, bool);
		SP_REGISTER_ATTRIBUTE_VALUE_VALUE(m_material, Material*);
		SP_REGISTER_ATTRIBUTE_VALUE_VALUE(m_flags, uint32_t);
		SP_REGISTER_ATTRIBUTE_VALUE_VALUE(m_mesh, Mesh*);
		SP_REGISTER_ATTRIBUTE_VALUE_VALUE(m_bounding_box, BoundingBox);
		SP_REGISTER_ATTRIBUTE_VALUE_VALUE(m_bounding_box_mesh, BoundingBox);
		SP_REGISTER_ATTRIBUTE_VALUE_VALUE(m_sub_mesh_index, uint32_t);
		SP_REGISTER_ATTRIBUTE_VALUE_VALUE(m_bounding_box_dirty, bool);
		SP_REGISTER_ATTRIBUTE_VALUE_VALUE(m_instances, vector<Instance>);
		SP_REGISTER_ATTRIBUTE_VALUE_VALUE(m_instance_buffer, shared_ptr<RHI_Buffer>);
		SP_REGISTER_ATTRIBUTE_VALUE_VALUE(m_transform_previous, Matrix);
		SP_REGISTER_ATTRIBUTE_VALUE_VALUE(m_max_distance_render, float);
		SP_REGISTER_ATTRIBUTE_VALUE_VALUE(m_max_distance_shadow, float);
		SP_REGISTER_ATTRIBUTE_VALUE_VALUE(m_distance_squared, float);
		SP_REGISTER_ATTRIBUTE_VALUE_VALUE(m_is_visible, bool);
		SP_REGISTER_ATTRIBUTE_VALUE_VALUE(m_lod_index, uint32_t);
		SP_REGISTER_ATTRIBUTE_VALUE_VALUE(m_previous_lights, uint64_t);*/
	}

	Renderable::~Renderable()
	{
		m_mesh = nullptr;
	}

	/*
	void Render::Save(pugi::xml_node& node)
	{
		// mesh
		node.append_attribute("mesh_name")      = m_mesh ? m_mesh->GetObjectName().c_str() : "";
		node.append_attribute("sub_mesh_index") = m_sub_mesh_index;

		// material
		node.append_attribute("material_name")    = m_material && !m_material_default ? m_material->GetObjectName().c_str() : "";
		node.append_attribute("material_default") = m_material_default;

		// flags
		node.append_attribute("flags") = m_flags;

		// distances
		node.append_attribute("max_render_distance") = m_max_distance_render;
		node.append_attribute("max_shadow_distance") = m_max_distance_shadow;

		// instances
		pugi::xml_node instances_node = node.append_child("Instances");
		for (const auto& instance : m_instances)
		{
			pugi::xml_node t_node = instances_node.append_child("Transform");
			math::Matrix matrix = instance.GetMatrix();
			std::stringstream ss;
			ss << matrix.m00 << " " << matrix.m01 << " " << matrix.m02 << " " << matrix.m03 << " "
			   << matrix.m10 << " " << matrix.m11 << " " << matrix.m12 << " " << matrix.m13 << " "
			   << matrix.m20 << " " << matrix.m21 << " " << matrix.m22 << " " << matrix.m23 << " "
			   << matrix.m30 << " " << matrix.m31 << " " << matrix.m32 << " " << matrix.m33;
			t_node.append_attribute("matrix") = ss.str().c_str();
		}
	}

	void Render::Load(pugi::xml_node& node)
	{
		// mesh
		const string mesh_name = node.attribute("mesh_name").as_string();
		m_sub_mesh_index       = node.attribute("sub_mesh_index").as_uint();
		if (!mesh_name.empty())
		{
			// check for standard meshes first (owned by Renderer, not ResourceCache)
			if (mesh_name == "standard_cube")
			{
				m_mesh = Renderer::GetStandardMesh(MeshType::Cube).get();
			}
			else if (mesh_name == "standard_quad")
			{
				m_mesh = Renderer::GetStandardMesh(MeshType::Quad).get();
			}
			else if (mesh_name == "standard_sphere")
			{
				m_mesh = Renderer::GetStandardMesh(MeshType::Sphere).get();
			}
			else if (mesh_name == "standard_cylinder")
			{
				m_mesh = Renderer::GetStandardMesh(MeshType::Cylinder).get();
			}
			else if (mesh_name == "standard_cone")
			{
				m_mesh = Renderer::GetStandardMesh(MeshType::Cone).get();
			}
			else
			{
				// look up in ResourceCache for custom meshes
				shared_ptr<Mesh> mesh = ResourceCache::GetByName<Mesh>(mesh_name);
				if (mesh)
				{
					m_mesh = mesh.get();
				}
				else
				{
					SP_LOG_WARNING("Renderable::Load - mesh '{}' not found in cache", mesh_name.c_str());
				}
			}
		}

		// material
		m_material_default         = node.attribute("material_default").as_bool(true);
		const string material_name = node.attribute("material_name").as_string();
		if (!material_name.empty() && !m_material_default)
		{
			shared_ptr<Material> material = ResourceCache::GetByName<Material>(material_name);
			if (material)
			{
				SetMaterial(material);
			}
		}
		else if (m_material_default)
		{
			// defer default material assignment - renderer may not be ready during load
			m_needs_default_material = true;
		}

		// flags
		m_flags = node.attribute("flags").as_uint();

		// distances
		m_max_distance_render = node.attribute("max_render_distance").as_float(FLT_MAX);
		m_max_distance_shadow = node.attribute("max_shadow_distance").as_float(FLT_MAX);

		// instances
		m_instances.clear();
		pugi::xml_node instances_node = node.child("Instances");
		if (instances_node)
		{
			for (pugi::xml_node t_node : instances_node.children("Transform"))
			{
				std::stringstream ss(t_node.attribute("matrix").as_string());
				math::Matrix matrix;
				float m[16];
				for (int i = 0; i < 16; ++i)
				{
					ss >> m[i];
				}
				if (!ss.fail())
				{
					matrix = math::Matrix(m[0], m[1], m[2], m[3],
						m[4], m[5], m[6], m[7],
						m[8], m[9], m[10], m[11],
						m[12], m[13], m[14], m[15]);
					Instance instance;
					instance.SetMatrix(matrix);
					m_instances.emplace_back(instance);
				}
			}
		}

		// compute mesh bounding box (needed for culling and LOD)
		if (m_mesh)
		{
			vector<RHI_Vertex_PosTexNorTan> vertices;
			m_mesh->GetGeometry(m_sub_mesh_index, nullptr, &vertices);
			if (!vertices.empty())
			{
				m_bounding_box_mesh = BoundingBox(vertices.data(), static_cast<uint32_t>(vertices.size()));
			}
		}

		// update instance buffer and bounding boxes
		if (!m_instances.empty())
		{
			SetInstances(m_instances);
		}
		else if (m_mesh)
		{
			Tick();
		}
	}
	*/

	void Renderable::Tick()
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

	/*
	void Render::RegisterForScripting(sol::state_view State)
	{
		State.new_usertype<Render>("Renderable",
		sol::base_classes,              sol::bases<Component>()
		);

	}

	sol::reference Render::AsLua(sol::state_view state)
	{
		return sol::make_reference(state, this);
	}
	*/

	void Renderable::SetMesh(Mesh* mesh, const uint32_t sub_mesh_index)
	{
		if (!mesh)
		{
			SEDX_CORE_WARN_TAG("Renderable", "SetMesh called with null mesh");
			return;
		}

		// set mesh
		m_mesh           = mesh;
		m_sub_mesh_index = sub_mesh_index;

		// compute and set bounding box (GetGeometry validates bounds internally)
		std::vector<Vertex_PosTexNorTan> vertices;
		mesh->GetGeometry(sub_mesh_index, nullptr, &vertices);
		if (!vertices.empty())
		{
		 std::vector<Vec3> positions;
			positions.reserve(vertices.size());
			for (const Vertex_PosTexNorTan& vertex : vertices)
			{
				positions.emplace_back(vertex.pos[0], vertex.pos[1], vertex.pos[2]);
			}

			m_bounding_box_mesh = BoundingBox(positions.data(), static_cast<uint32_t>(positions.size()));
		}

		Tick(); // update bounding boxes, frustum and distance culling
	}

	void Renderable::SetMesh(const MeshType type)
	{
	 SetMesh(Renderer::GetStandardMesh(type));
	}

	void Renderable::GetGeometry(std::vector<uint32_t>* indices, std::vector<Vertex_PosTexNorTan>* vertices) const
	{
		if (!m_mesh)
		{
			SEDX_CORE_WARN_TAG("Renderable", "GetGeometry called with null mesh");
			return;
		}
		m_mesh->GetGeometry(m_sub_mesh_index, indices, vertices);
	}

	void Renderable::SetMaterial(const Ref<MaterialAsset>& material)
	{
		SEDX_CORE_ASSERT(material != nullptr, "Material is null");

		m_material_default = false;

		m_material = material.Get();
		if (m_material == nullptr)
		{
		   SEDX_CORE_ERROR_TAG("Renderable", "Material was null and failed to be set.");
			return;
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

				for (const Vertex_PosTexNorTan& vertex : vertices)
				{
					Vec3 position = Vec3(vertex.pos[0], vertex.pos[1], vertex.pos[2]);
					height_min       = xMath::Min(height_min, position.y);
					max_height       = xMath::Max(max_height, position.y);
					min_width        = xMath::Min(min_width, position.x);
					max_width        = xMath::Max(max_width, position.x);
				}

				static_cast<void>(max_width - min_width);
				static_cast<void>(max_height - height_min);
			}
		}
	}

	void Renderable::SetMaterial(const std::string & file_path)
	{
		auto material = CreateRef<MaterialAsset>();

		material->Load(file_path);

		SetMaterial(material);
	}

	void Renderable::SetDefaultMaterial()
	{
		SetMaterial(Renderer::GetStandardMaterial());
		m_material_default = true;
	}

	std::string Renderable::GetMaterialName() const
	{
		return m_material ? m_material->GetName() : "";
	}

	uint32_t Renderable::GetIndexOffset(const uint32_t lod) const
	{
		// global base offset + lod-relative offset within the mesh
		return m_mesh->GetGlobalIndexOffset() + m_mesh->GetSubMesh(m_sub_mesh_index).lods[lod].index_offset;
	}

	uint32_t Renderable::GetIndexCount(const uint32_t lod) const
	{
		return m_mesh->GetSubMesh(m_sub_mesh_index).lods[lod].index_count;
	}

	uint32_t Renderable::GetVertexOffset(const uint32_t lod) const
	{
		// global base offset + lod-relative offset within the mesh
		return m_mesh->GetGlobalVertexOffset() + m_mesh->GetSubMesh(m_sub_mesh_index).lods[lod].vertex_offset;
	}

	uint32_t Renderable::GetVertexCount(const uint32_t lod) const
	{
		return m_mesh->GetSubMesh(m_sub_mesh_index).lods[lod].vertex_count;
	}

	Buffer* Renderable::GetIndexBuffer() const
	{
		if (!m_mesh)
			return nullptr;

		return m_mesh->GetIndexBuffer();
	}

	Buffer* Renderable::GetVertexBuffer() const
	{
		if (!m_mesh)
			return nullptr;

		return m_mesh->GetVertexBuffer();
	}

	const std::string & Renderable::GetMeshName() const
	{
		static std::string noMesh = "N/A";
		if (!m_mesh)
			return noMesh;

		return m_mesh->GetObjectName();
	}

	Matrix Renderable::GetInstance(const uint32_t index, const bool to_world)
	{
		return to_world ? m_instances[index].GetMatrix() * GetEntity()->GetMatrix() : m_instances[index].GetMatrix();
	}

	void Renderable::SetInstances(const std::vector<Instance>& instances)
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

	void Renderable::SetInstances(const std::vector<Matrix>& transforms)
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

	uint32_t Renderable::GetLodCount() const
	{
		if (!m_mesh)
			return 0;

		return static_cast<uint32_t>(m_mesh->GetSubMesh(m_sub_mesh_index).lods.size());
	}

	void Renderable::SetFlag(const RenderableFlags flag, const bool enable /*= true*/)
	{
		bool enabled      = false;
		bool disabled     = false;
		bool flagPresent  = m_flags & flag;

		if (enable && !flagPresent)
		{
			m_flags |= static_cast<uint32_t>(flag);
			enabled  = true;

		}
		else if (!enable && flagPresent)
		{
			m_flags  &= ~static_cast<uint32_t>(flag);
			disabled  = true;
		}
	}

	void Renderable::UpdateAabb()
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
					Matrix worldInstance = instance.GetMatrix() * transform;
					m_bounding_box.Merge(m_bounding_box_mesh * worldInstance);
				}
			}
			m_transform_previous = transform;
			m_bounding_box_dirty = false;
		}
	}

	void Renderable::UpdateFrustumAndDistanceCulling()
	{
		if (Camera* camera = Scene::GetCamera())
		{
			Vec3 cameraPosition = camera->GetEntity()->GetPosition();

			const BoundingBox& boundingBox = GetBoundingBox();

			// first, check if the bounding box is in the frustum
			if (camera->IsInViewFrustum(boundingBox))
			{
				// only if in frustum, calculate distance
				const Vec3 closestPoint = boundingBox.GetClosestPoint(cameraPosition);
				m_distance_squared = xMath::Length2(cameraPosition - closestPoint);
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

	void Renderable::UpdateLodIndices()
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

		const BoundingBox& box        = GetBoundingBox();
		const Vec3 cameraPosition = camera->GetEntity()->GetPosition();

		// camera inside bounding box = maximum detail
		if (box.Contains(cameraPosition))
		{
			m_lod_index = 0;
			return;
		}

		// distance from camera to closest point on bounding box
		Vec3 closestPoint = box.GetClosestPoint(cameraPosition);
		float distance     = xMath::Max(Length((closestPoint - cameraPosition)), 0.001f);

		// compute screen-space coverage: fraction of vertical screen space the object covers
		// screen_fraction = (object_diameter) / (visible_height_at_distance)
		// visible_height_at_distance = 2 * distance * tan(fov_v / 2)
		float boundingDiameter = Length(box.GetExtents()) * 2.0f;
		float tanHalfFov      = tan(camera->GetFovVerticalRad() * 0.5f);
		float screenFraction   = boundingDiameter / (2.0f * distance * tanHalfFov);

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

			if (screenFraction >= threshold)
			{
				new_lod = i;
				break;
			}
		}

		m_lod_index = xMath::Clamp(new_lod, 0u, lod_count - 1);
	}
}

// -------------------------------------------------------
