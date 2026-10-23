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

}

// -------------------------------------------------------

