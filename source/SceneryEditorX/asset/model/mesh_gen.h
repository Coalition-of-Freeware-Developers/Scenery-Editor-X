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
 * mesh_gen.h
 * -------------------------------------------------------
 * Created: 18/03/2026
 * -------------------------------------------------------
 */
#pragma once
#include <cstdint>
#include <vector>
#include <SceneryEditorX/renderer/vulkan/vertex.h>
#include <SceneryEditorX/utils/inheritance.h>

// -------------------------------------------------------

namespace SceneryEditorX
{
	/**
	 * @enum MeshType
	 * @brief 
	 */
	enum class MeshType: uint8_t
	{
		Cube,
		Quad,
		Box,
		Sphere,
		Cylinder,
		Cone,
		MaxEnum
	};

	/**
	 * @class MeshGenerator
	 * @brief A class responsible for generating various types of 3D meshes.
	 */
	class MeshGenerator : public SharedObject
	{
	public:
		using MeshVertex = ::SceneryEditorX::Vertex_PosTexNorTan;
		using MeshIndex = std::uint32_t;

		MeshGenerator(MeshType type);
		~MeshGenerator();
	
		Ref<MeshGenerator> Get() const { return m_Mesh; }

		static void GenerateCube(std::vector<MeshVertex>* vertices, std::vector<MeshIndex>* indices);
		static void GenerateQuad(std::vector<MeshVertex> *vertices, std::vector<MeshIndex> *indices);
		static void GenerateGrid(std::vector<MeshVertex>* vertices, std::vector<MeshIndex>* indices, MeshIndex grid_points_per_dimension, float extent);
		static void GenerateSphere(std::vector<MeshVertex>* vertices, std::vector<MeshIndex>* indices, float radius = 1.0f, int slices = 20, int stacks = 20);
		static void GenerateCylinder(std::vector<MeshVertex>* vertices, std::vector<MeshIndex>* indices, float radiusTop = 1.0f, float radiusBottom = 1.0f, float height = 1.0f, int slices = 15, int stacks = 15);
		static void GenerateCone(std::vector<MeshVertex>* vertices, std::vector<MeshIndex>* indices, float radius = 1.0f, float height = 2.0f);

	private:
		Ref<MeshGenerator> m_Mesh;
		MeshType m_Type = MeshType::MaxEnum;
		std::vector<MeshVertex> *m_Vertices = nullptr;
		std::vector<MeshIndex>* m_Indices = nullptr;
	};

	/*
	static void generate_foliage_grass_blade(std::vector<Vertex_PosTexNorTan>* vertices, std::vector<uint32_t>* indices, const uint32_t segment_count)
	{

		// constants
		const float grass_width    = 0.3f; // base width
		const float grass_height   = 1.2f; // blade height
		const float thinning_start = 0.4f; // thinning start (0=base, 1=top)
		const float thinning_power = 1.0f; // thinning sharpness

		// clear output vectors
		vertices->clear();
		indices->clear();

		// helper to compute width factor
		auto compute_width_factor = [=](float t) -> float
		{
			return (t <= thinning_start) ? 1.0f : std::pow(1.0f - ((t - thinning_start) / (1.0f - thinning_start)), thinning_power);
		};

		// helper to push vertex
		auto push_vertex = [&](const Vector3& pos, const Vector2& tex)
		{
			RHI_Vertex_PosTexNorTan v{};
			v.pos[0] = pos.x; v.pos[1] = pos.y; v.pos[2] = pos.z;
			v.tex[0] = tex.x; v.tex[1] = tex.y;
			v.nor[0] = 0.0f; v.nor[1] = 0.0f; v.nor[2] = 0.0f;
			v.tan[0] = 0.0f; v.tan[1] = 0.0f; v.tan[2] = 0.0f;
			vertices->push_back(v);
		};

		// total verts per face
		uint32_t verts_per_strip = (segment_count + 1) * 2 - 1;
		uint32_t total_vertices  = verts_per_strip;
		vertices->reserve(total_vertices);

		// generate vertices for front face
		for (uint32_t i = 0; i <= segment_count; ++i)
		{
			float t            = float(i) / segment_count;
			float y            = t * grass_height;
			float width_factor = compute_width_factor(t);
			if (i < segment_count)
			{
				// left vertex
				push_vertex(Vector3(-grass_width * 0.5f * width_factor, y, 0.0f), Vector2(0.0f, t));
				// right vertex
				push_vertex(Vector3(grass_width * 0.5f * width_factor, y, 0.0f), Vector2(1.0f, t));
			}
			else
			{
				// top vertex (single vertex at tip)
				push_vertex(Vector3(0.0f, y, 0.0f), Vector2(0.5f, t));
			}
		}

		// bend towards downwards (emulate gravity pulling the blade down)
		const float bend_amount = 0.25f;
		for (RHI_Vertex_PosTexNorTan& v : *vertices)
		{
			float uv_misc_z     = v.tex[1];
			float gravity_angle = bend_amount * uv_misc_z;
			float c             = std::cos(gravity_angle);
			float s             = std::sin(gravity_angle);
			float y             = v.pos[1];
			float z             = v.pos[2];
			v.pos[1]            = c * y - s * z;
			v.pos[2]            = s * y + c * z;
		}

		// build indices for front face
		uint32_t verts_per_face = verts_per_strip;
		uint32_t offset         = 0;
		bool invert_winding     = false;
		for (uint32_t i = 0; i < segment_count; ++i)
		{
			if (i < segment_count - 1)
			{
				if (!invert_winding)
				{
					indices->push_back(offset + i * 2);
					indices->push_back(offset + i * 2 + 1);
					indices->push_back(offset + i * 2 + 2);
					indices->push_back(offset + i * 2 + 2);
					indices->push_back(offset + i * 2 + 1);
					indices->push_back(offset + i * 2 + 3);
				}
				else
				{
					indices->push_back(offset + i * 2 + 2);
					indices->push_back(offset + i * 2 + 1);
					indices->push_back(offset + i * 2);
					indices->push_back(offset + i * 2 + 3);
					indices->push_back(offset + i * 2 + 1);
					indices->push_back(offset + i * 2 + 2);
				}
			}
			else
			{
				// last triangle at tip
				if (!invert_winding)
				{
					indices->push_back(offset + i * 2);
					indices->push_back(offset + i * 2 + 1);
					indices->push_back(offset + i * 2 + 2);
				}
				else
				{
					indices->push_back(offset + i * 2 + 2);
					indices->push_back(offset + i * 2 + 1);
					indices->push_back(offset + i * 2);
				}
			}
		}

		// compute normals and tangents
		for (size_t i = 0; i < indices->size(); i += 3)
		{
			uint32_t i0 = (*indices)[i];
			uint32_t i1 = (*indices)[i + 1];
			uint32_t i2 = (*indices)[i + 2];
			Vector3 p0((*vertices)[i0].pos[0], (*vertices)[i0].pos[1], (*vertices)[i0].pos[2]);
			Vector3 p1((*vertices)[i1].pos[0], (*vertices)[i1].pos[1], (*vertices)[i1].pos[2]);
			Vector3 p2((*vertices)[i2].pos[0], (*vertices)[i2].pos[1], (*vertices)[i2].pos[2]);
			Vector3 edge1 = p1 - p0;
			Vector3 edge2 = p2 - p0;
			Vector3 face_normal = Vector3::Normalize(Vector3::Cross(edge1, edge2));

			// add face normal to vertices
			(*vertices)[i0].nor[0] += face_normal.x;
			(*vertices)[i0].nor[1] += face_normal.y;
			(*vertices)[i0].nor[2] += face_normal.z;
			(*vertices)[i1].nor[0] += face_normal.x;
			(*vertices)[i1].nor[1] += face_normal.y;
			(*vertices)[i1].nor[2] += face_normal.z;
			(*vertices)[i2].nor[0] += face_normal.x;
			(*vertices)[i2].nor[1] += face_normal.y;
			(*vertices)[i2].nor[2] += face_normal.z;

			// approximate tangent as direction along width (x axis), assuming grass blade vertical along y
			Vector3 tangent = Vector3::Normalize(Vector3(edge1.x, 0.0f, edge1.z));
			(*vertices)[i0].tan[0] += tangent.x;
			(*vertices)[i0].tan[1] += tangent.y;
			(*vertices)[i0].tan[2] += tangent.z;
			(*vertices)[i1].tan[0] += tangent.x;
			(*vertices)[i1].tan[1] += tangent.y;
			(*vertices)[i1].tan[2] += tangent.z;
			(*vertices)[i2].tan[0] += tangent.x;
			(*vertices)[i2].tan[1] += tangent.y;
			(*vertices)[i2].tan[2] += tangent.z;
		}

		// normalize normals and tangents per vertex
		for (auto& v : *vertices)
		{
			Vector3 n(v.nor[0], v.nor[1], v.nor[2]);
			n = Vector3::Normalize(n);
			v.nor[0] = n.x; v.nor[1] = n.y; v.nor[2] = n.z;
			Vector3 t(v.tan[0], v.tan[1], v.tan[2]);
			t = Vector3::Normalize(t);
			v.tan[0] = t.x; v.tan[1] = t.y; v.tan[2] = t.z;
		}
	}

	static void generate_foliage_flower(std::vector<Vertex_PosTexNorTan>* vertices, std::vector<uint32_t>* indices, const uint32_t stem_segment_count, const uint32_t petal_count, const uint32_t petal_segment_count)
	{

		// constants
		const float stem_radius                  = 0.032f; // was width/2
		const float stem_height                  = 0.64f;
		const float stem_thinning_start          = 0.7f;
		const float stem_thinning_power          = 1.0f;
		const uint32_t stem_side_count           = 6; // new: for cylinder
		const float petal_width                  = 0.13f;
		const float petal_length                 = 0.26f;
		const float petal_thinning_power         = 1.5f; // for oval sharpness
		const float min_petal_tilt               = 20.0f * deg_to_rad; // outer open
		const float max_petal_tilt               = 70.0f * deg_to_rad; // inner upright
		const float min_petal_bend               = 0.0f; // inner straight
		const float max_petal_bend               = 0.3f; // outer droop
		const float small_petal_scale            = 0.5f; // inner small
		const float large_petal_scale            = 1.0f; // outer large
		const float min_petal_curvature          = 0.1f; // new: outer less cupped
		const float max_petal_curvature          = 0.4f; // inner more cupped
		const uint32_t petal_width_segment_count = 2; // new: >=1, allows cupping
		const float spiral_height                = 0.13f;
		const float spiral_radius                = 0.0f; // new: 0 to attach at center
		const float golden_angle                 = 137.5f * deg_to_rad;

		// clear output
		vertices->clear();
		indices->clear();

		// helper
		auto push_vertex = [&](const Vector3& pos, const Vector2& tex)
		{
			RHI_Vertex_PosTexNorTan v{};
			v.pos[0] = pos.x; v.pos[1] = pos.y; v.pos[2] = pos.z;
			v.tex[0] = tex.x; v.tex[1] = tex.y;
			v.nor[0] = 0.0f; v.nor[1] = 0.0f; v.nor[2] = 0.0f;
			v.tan[0] = 0.0f; v.tan[1] = 0.0f; v.tan[2] = 0.0f;
			vertices->push_back(v);
		};

		// stem: tapered cylinder
		auto stem_radius_factor = [=](float t) -> float
		{
			if (t <= stem_thinning_start) return 1.0f;
			float x = (t - stem_thinning_start) / (1.0f - stem_thinning_start);
			return std::pow(1.0f - std::clamp(x, 0.0f, 1.0f), stem_thinning_power);
		};
		uint32_t offset = 0;
		for (uint32_t i = 0; i <= stem_segment_count; ++i)
		{
			float t  = static_cast<float>(i) / stem_segment_count;
			float y  = t * stem_height;
			float rf = stem_radius_factor(t);
			float r  = stem_radius * rf;
			for (uint32_t j = 0; j < stem_side_count; ++j)
			{
				float a = static_cast<float>(j) / stem_side_count * math::pi_2;
				float x = std::cos(a) * r;
				float z = std::sin(a) * r;
				push_vertex(Vector3(x, y, z), Vector2(static_cast<float>(j) / stem_side_count, t));
			}
		}
		for (uint32_t i = 0; i < stem_segment_count; ++i)
		{
			for (uint32_t j = 0; j < stem_side_count; ++j)
			{
				uint32_t a = offset + i * stem_side_count + j;
				uint32_t b = offset + i * stem_side_count + (j + 1) % stem_side_count;
				uint32_t c = a + stem_side_count;
				uint32_t d = b + stem_side_count;
				indices->push_back(a); indices->push_back(c); indices->push_back(b);
				indices->push_back(b); indices->push_back(c); indices->push_back(d);
			}
		}
		uint32_t current_offset = (stem_segment_count + 1) * stem_side_count;

		// petals: spiral with oval taper, cup curvature
		auto petal_width_factor = [=](float t) -> float
		{
			float s = std::sin(t * math::pi);
			return std::pow(s, petal_thinning_power);
		};
		for (uint32_t p = 0; p < petal_count; ++p)
		{
			float frac           = petal_count > 1 ? static_cast<float>(p) / (petal_count - 1) : 0.0f;
			float this_tilt      = min_petal_tilt + frac * (max_petal_tilt - min_petal_tilt);
			float this_bend      = max_petal_bend - frac * (max_petal_bend - min_petal_bend);
			float this_scale     = large_petal_scale - frac * (large_petal_scale - small_petal_scale);
			float this_curvature = min_petal_curvature + frac * (max_petal_curvature - min_petal_curvature);
			float this_height    = stem_height + frac * spiral_height;
			float this_radius    = spiral_radius;
			float angle          = static_cast<float>(p) * golden_angle;
			float ca             = std::cos(angle);
			float sa             = std::sin(angle);
			float cos_t          = std::cos(this_tilt);
			float sin_t          = std::sin(this_tilt);
			uint32_t petal_start = static_cast<uint32_t>(vertices->size());
			for (uint32_t i = 0; i <= petal_segment_count; ++i)
			{
				float t = static_cast<float>(i) / petal_segment_count;
				float wf = petal_width_factor(t);
				Vector3 local_pos;
				Vector2 tex;
				if (i < petal_segment_count)
				{
					for (uint32_t j = 0; j <= petal_width_segment_count; ++j)
					{
						float frac_j   = static_cast<float>(j) / petal_width_segment_count;
						float local_x  = (frac_j - 0.5f) * petal_width * wf * this_scale;
						local_pos      = Vector3(local_x, 0.0f, t * petal_length * this_scale);
						tex            = Vector2(frac_j, t);
						local_pos.y   -= this_bend * t * t;

						// curvature: concave cup, stronger at base
						float norm_x  = 2.0f * (frac_j - 0.5f); // -1 to 1
						local_pos.y  += this_curvature * (norm_x * norm_x - 1.0f) * (1.0f - t);

						// tilt
						float new_y = cos_t * local_pos.y + sin_t * local_pos.z;
						float new_z = -sin_t * local_pos.y + cos_t * local_pos.z;
						local_pos.y = new_y;
						local_pos.z = new_z;

						// rotate
						float new_x = ca * local_pos.x - sa * local_pos.z;
						new_z       = sa * local_pos.x + ca * local_pos.z;
						local_pos.x = new_x;
						local_pos.z = new_z;

						// offset (minimal)
						local_pos.x += ca * this_radius;
						local_pos.z += sa * this_radius;
						local_pos.y += this_height;
						push_vertex(local_pos, tex);
					}
				}
				else
				{
					// tip
					local_pos    = Vector3(0.0f, 0.0f, t * petal_length * this_scale);
					tex          = Vector2(0.5f, t);
					local_pos.y -= this_bend * t * t;
					local_pos.y += this_curvature * (0.0f - 1.0f) * (1.0f - t); // center adjust
					float new_y  = cos_t * local_pos.y + sin_t * local_pos.z;
					float new_z  = -sin_t * local_pos.y + cos_t * local_pos.z;
					local_pos.y  = new_y;
					local_pos.z  = new_z;
					float new_x  = ca * local_pos.x - sa * local_pos.z;
					new_z        = sa * local_pos.x + ca * local_pos.z;
					local_pos.x  = new_x;
					local_pos.z  = new_z;
					local_pos.x += ca * this_radius;
					local_pos.z += sa * this_radius;
					local_pos.y += this_height;
					push_vertex(local_pos, tex);
				}
			}
			// petal indices (grid + fan to tip)
			uint32_t row_size = petal_width_segment_count + 1;
			for (uint32_t i = 0; i < petal_segment_count - 1; ++i)
			{
				for (uint32_t j = 0; j < petal_width_segment_count; ++j)
				{
					uint32_t a = current_offset + i * row_size + j;
					uint32_t b = a + 1;
					uint32_t c = a + row_size;
					uint32_t d = c + 1;
					indices->push_back(a); indices->push_back(c); indices->push_back(b);
					indices->push_back(b); indices->push_back(c); indices->push_back(d);
				}
			}
			// last row to tip
			uint32_t last_row_start = current_offset + (petal_segment_count - 1) * row_size;
			uint32_t tip_index = current_offset + petal_segment_count * row_size;
			for (uint32_t j = 0; j < petal_width_segment_count; ++j)
			{
				uint32_t a = last_row_start + j;
				uint32_t b = a + 1;
				indices->push_back(a); indices->push_back(tip_index); indices->push_back(b);
			}
			current_offset += petal_segment_count * row_size + 1;
		}

		// compute normals and tangents
		for (size_t i = 0; i < indices->size(); i += 3)
		{
			uint32_t i0 = (*indices)[i];
			uint32_t i1 = (*indices)[i + 1];
			uint32_t i2 = (*indices)[i + 2];
			Vector3 p0((*vertices)[i0].pos[0], (*vertices)[i0].pos[1], (*vertices)[i0].pos[2]);
			Vector3 p1((*vertices)[i1].pos[0], (*vertices)[i1].pos[1], (*vertices)[i1].pos[2]);
			Vector3 p2((*vertices)[i2].pos[0], (*vertices)[i2].pos[1], (*vertices)[i2].pos[2]);
			Vector3 edge1 = p1 - p0;
			Vector3 edge2 = p2 - p0;
			Vector3 face_normal = Vector3::Normalize(Vector3::Cross(edge1, edge2));
			(*vertices)[i0].nor[0] += face_normal.x; (*vertices)[i0].nor[1] += face_normal.y; (*vertices)[i0].nor[2] += face_normal.z;
			(*vertices)[i1].nor[0] += face_normal.x; (*vertices)[i1].nor[1] += face_normal.y; (*vertices)[i1].nor[2] += face_normal.z;
			(*vertices)[i2].nor[0] += face_normal.x; (*vertices)[i2].nor[1] += face_normal.y; (*vertices)[i2].nor[2] += face_normal.z;
			Vector3 tangent = Vector3::Normalize(Vector3(edge1.x, 0.0f, edge1.z));
			(*vertices)[i0].tan[0] += tangent.x; (*vertices)[i0].tan[1] += tangent.y; (*vertices)[i0].tan[2] += tangent.z;
			(*vertices)[i1].tan[0] += tangent.x; (*vertices)[i1].tan[1] += tangent.y; (*vertices)[i1].tan[2] += tangent.z;
			(*vertices)[i2].tan[0] += tangent.x; (*vertices)[i2].tan[1] += tangent.y; (*vertices)[i2].tan[2] += tangent.z;
		}
		for (auto& v : *vertices)
		{
			Vector3 n(v.nor[0], v.nor[1], v.nor[2]);
			n = Vector3::Normalize(n);
			v.nor[0] = n.x; v.nor[1] = n.y; v.nor[2] = n.z;
			Vector3 t(v.tan[0], v.tan[1], v.tan[2]);
			t = Vector3::Normalize(t);
			v.tan[0] = t.x; v.tan[1] = t.y; v.tan[2] = t.z;
		}
	}
	*/

}

// -------------------------------------------------------

