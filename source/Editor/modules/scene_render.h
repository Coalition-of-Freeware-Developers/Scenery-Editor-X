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
 * scene_render.h
 * -------------------------------------------------------
 * Created: 07/04/2026
 * -------------------------------------------------------
 */
#pragma once
#include <SceneryEditorX/scene/components/component.h>
#include <SceneryEditorX/scene/components/renderable.h>

// -------------------------------------------------------------------

namespace SceneryEditorX
{
	class Material;

	class Render : public Component
	{
	public:
		Render(Entity *entity);
		~Render();

		// mesh
		void SetMesh(Mesh* mesh, const uint32_t sub_mesh_index = 0);
		void SetMesh(const MeshType type);
		void GetGeometry(std::vector<uint32_t>* indices, std::vector<Vertex_PosTexNorTan>* vertices) const;
		uint32_t GetLodCount() const;
		uint32_t GetLodIndex() const { return m_lod_index; }
		uint32_t GetIndexOffset(const uint32_t lod = 0) const;
		uint32_t GetIndexCount(const uint32_t lod = 0) const;
		uint32_t GetVertexOffset(const uint32_t lod = 0) const;
		uint32_t GetVertexCount(const uint32_t lod = 0) const;
		Buffer* GetIndexBuffer() const;
		Buffer* GetVertexBuffer() const;
		const std::string& GetMeshName() const;
		void BuildAccelerationStructure(CommandList* cmd_list);
		void RefitAccelerationStructure(CommandList* cmd_list);
		bool HasAccelerationStructure() const;
		void InvalidateAccelerationStructure();
		uint64_t GetAccelerationStructureDeviceAddress() const;

		// blas refit (for deformable meshes like cloth)
		void SetNeedsBlasRefit(bool v)  { m_needs_blas_refit = v; }
		bool NeedsBlasRefit() const     { return m_needs_blas_refit; }
		void SetAllowBlasUpdate(bool v) { m_allow_blas_update = v; }
		bool GetAllowBlasUpdate() const { return m_allow_blas_update; }

		// bounding box
		const xMath::BoundingBox& GetBoundingBox() const     { return m_bounding_box; }
		const xMath::BoundingBox& GetBoundingBoxMesh() const { return m_bounding_box_mesh; }

		// material
		void SetMaterial(const Ref<MaterialAsset>& material);
		void SetMaterial(const std::string& file_path);
		void SetDefaultMaterial();
		std::string GetMaterialName() const;
		Material* GetMaterial() const { return m_Material; }
		MaterialAsset* GetMaterialAsset() const { return m_MaterialAsset; }

		// instancing
		bool HasInstancing() const { return !m_instances.empty(); }
		Buffer* GetInstanceBuffer() const { return m_instance_buffer.Get(); }
		uint32_t GetInstanceCount()  const { return m_instances.empty() ? 1 : static_cast<uint32_t>(m_instances.size()); }
		xMath::Matrix GetInstance(const uint32_t index, const bool to_world);
		void SetInstances(const std::vector<Instance>& instances);
		void SetInstances(const std::vector<xMath::Matrix>& transforms);

		// render distance
		float GetMaxRenderDistance() const { return m_max_distance_render; }
		void SetMaxRenderDistance(const float max_render_distance) { m_max_distance_render = max_render_distance; }

		// shadow distance
		float GetMaxShadowDistance() const { return m_max_distance_shadow; }
		void SetMaxShadowDistance(const float max_shadow_distance) { m_max_distance_shadow = max_shadow_distance; }

		// distance & visibility
		float GetDistanceSquared() const { return m_distance_squared; }
		bool IsVisible() const { return m_is_visible; }
		void SetVisible(const bool visible) { m_is_visible = visible; }

		// flags
		bool HasFlag(const RenderableFlags flag) const { return m_flags & flag; }
		void SetFlag(const RenderableFlags flag, const bool enable = true);

		// previous lights tracking
		uint64_t GetPreviousLights() const { return m_previous_lights; }
		void SetPreviousLights(uint64_t lights) { m_previous_lights = lights; }

	private:
		void UpdateAabb();
		void UpdateFrustumAndDistanceCulling();
		void UpdateLodIndices();

	public:
		void Tick() override;

	private:
		// geometry/mesh
		Mesh* m_mesh                          = nullptr;
		uint32_t m_sub_mesh_index             = 0;
		bool m_bounding_box_dirty             = true;
		xMath::BoundingBox m_bounding_box_mesh = xMath::BoundingBox::UNIT;
		xMath::BoundingBox m_bounding_box      = xMath::BoundingBox::UNIT;

		// material
		bool m_Material_Default = false;
		Material* m_Material = nullptr;
		MaterialAsset* m_MaterialAsset = nullptr;

		// instancing
		std::vector<Instance> m_instances;
		Ref<Buffer> m_instance_buffer;

		// blas refit
		bool m_needs_blas_refit  = false;
		bool m_allow_blas_update = false;

		// misc
		xMath::Matrix m_transform_previous = xMath::Matrix::IDENTITY;
		uint32_t m_flags = RenderableFlags::CastsShadows;

		// deferred default material assignment (renderer may not be ready during load)
		bool m_needs_default_material = false;

		// visibility & lods
		float m_max_distance_render = FLT_MAX;
		float m_max_distance_shadow = FLT_MAX;
		float m_distance_squared    = 0.0f;
		bool m_is_visible           = false;
		uint32_t m_lod_index        = 0;
		uint64_t m_previous_lights  = 0; // lights whose frustums this renderable was in last frame
	};


}

// -------------------------------------------------------------------
