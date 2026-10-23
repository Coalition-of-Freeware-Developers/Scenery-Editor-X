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
 * mesh_gen.cpp
 * -------------------------------------------------------
 * Created: 18/03/2026
 * -------------------------------------------------------
 */
#include "mesh_gen.h"
#include <cmath>
#include <math_utils.h>
#include <SceneryEditorX/renderer/vulkan/vertex.h>

// -------------------------------------------------------

namespace SceneryEditorX
{
	MeshGenerator::MeshGenerator(MeshType type)
	{
		this->m_Type = type;
	    this->m_Vertices = new std::vector<MeshVertex>();
		this->m_Indices = new std::vector<MeshIndex>();
		switch (type)
		{
			case MeshType::Cube:
				GenerateCube(m_Vertices, m_Indices);
				break;
		  case MeshType::Quad:
				GenerateQuad(m_Vertices, m_Indices);
				break;
			case MeshType::Sphere:
				GenerateSphere(m_Vertices, m_Indices);
				break;
			case MeshType::Cylinder:
				GenerateCylinder(m_Vertices, m_Indices);
				break;
			case MeshType::Cone:
				GenerateCone(m_Vertices, m_Indices);
				break;
			default:
				SEDX_CORE_ERROR_TAG("MeshGenerator","Unsupported mesh type");
				break;
		}
	}

	MeshGenerator::~MeshGenerator()
	{
		delete m_Vertices;
		m_Vertices = nullptr;

		delete m_Indices;
		m_Indices = nullptr;
	}

	void MeshGenerator::GenerateCube(std::vector<MeshGenerator::MeshVertex>* vertices, std::vector<MeshGenerator::MeshIndex>* indices)
	{
		if (vertices == nullptr || indices == nullptr)
			return;

		vertices->clear();
		indices->clear();

		// front
		vertices->emplace_back(Vec3(-0.5f, -0.5f, -0.5f), Vec2(0, 1), Vec3(0, 0, -1), Vec3(0, 1, 0));
		vertices->emplace_back(Vec3(-0.5f, 0.5f, -0.5f), Vec2(0, 0), Vec3(0, 0, -1), Vec3(0, 1, 0));
		vertices->emplace_back(Vec3(0.5f, -0.5f, -0.5f), Vec2(1, 1), Vec3(0, 0, -1), Vec3(0, 1, 0));
		vertices->emplace_back(Vec3(0.5f, 0.5f, -0.5f), Vec2(1, 0), Vec3(0, 0, -1), Vec3(0, 1, 0));

		// bottom
		vertices->emplace_back(Vec3(-0.5f, -0.5f, 0.5f), Vec2(0, 1), Vec3(0, -1, 0), Vec3(1, 0, 0));
		vertices->emplace_back(Vec3(-0.5f, -0.5f, -0.5f), Vec2(0, 0), Vec3(0, -1, 0), Vec3(1, 0, 0));
		vertices->emplace_back(Vec3(0.5f, -0.5f, 0.5f), Vec2(1, 1), Vec3(0, -1, 0), Vec3(1, 0, 0));
		vertices->emplace_back(Vec3(0.5f, -0.5f, -0.5f), Vec2(1, 0), Vec3(0, -1, 0), Vec3(1, 0, 0));

		// back
		vertices->emplace_back(Vec3(-0.5f, -0.5f, 0.5f), Vec2(1, 1), Vec3(0, 0, 1), Vec3(0, 1, 0));
		vertices->emplace_back(Vec3(-0.5f, 0.5f, 0.5f), Vec2(1, 0), Vec3(0, 0, 1), Vec3(0, 1, 0));
		vertices->emplace_back(Vec3(0.5f, -0.5f, 0.5f), Vec2(0, 1), Vec3(0, 0, 1), Vec3(0, 1, 0));
		vertices->emplace_back(Vec3(0.5f, 0.5f, 0.5f), Vec2(0, 0), Vec3(0, 0, 1), Vec3(0, 1, 0));

		// top
		vertices->emplace_back(Vec3(-0.5f, 0.5f, 0.5f), Vec2(0, 0), Vec3(0, 1, 0), Vec3(1, 0, 0));
		vertices->emplace_back(Vec3(-0.5f, 0.5f, -0.5f), Vec2(0, 1), Vec3(0, 1, 0), Vec3(1, 0, 0));
		vertices->emplace_back(Vec3(0.5f, 0.5f, 0.5f), Vec2(1, 0), Vec3(0, 1, 0), Vec3(1, 0, 0));
		vertices->emplace_back(Vec3(0.5f, 0.5f, -0.5f), Vec2(1, 1), Vec3(0, 1, 0), Vec3(1, 0, 0));

		// left
		vertices->emplace_back(Vec3(-0.5f, -0.5f, 0.5f), Vec2(0, 1), Vec3(-1, 0, 0), Vec3(0, 1, 0));
		vertices->emplace_back(Vec3(-0.5f, 0.5f, 0.5f), Vec2(0, 0), Vec3(-1, 0, 0), Vec3(0, 1, 0));
		vertices->emplace_back(Vec3(-0.5f, -0.5f, -0.5f), Vec2(1, 1), Vec3(-1, 0, 0), Vec3(0, 1, 0));
		vertices->emplace_back(Vec3(-0.5f, 0.5f, -0.5f), Vec2(1, 0), Vec3(-1, 0, 0), Vec3(0, 1, 0));

		// right
		vertices->emplace_back(Vec3(0.5f, -0.5f, 0.5f), Vec2(1, 1), Vec3(1, 0, 0), Vec3(0, 1, 0));
		vertices->emplace_back(Vec3(0.5f, 0.5f, 0.5f), Vec2(1, 0), Vec3(1, 0, 0), Vec3(0, 1, 0));
		vertices->emplace_back(Vec3(0.5f, -0.5f, -0.5f), Vec2(0, 1), Vec3(1, 0, 0), Vec3(0, 1, 0));
		vertices->emplace_back(Vec3(0.5f, 0.5f, -0.5f), Vec2(0, 0), Vec3(1, 0, 0), Vec3(0, 1, 0));

		// front
		indices->emplace_back(0); indices->emplace_back(1); indices->emplace_back(2);
		indices->emplace_back(2); indices->emplace_back(1); indices->emplace_back(3);

		// bottom
		indices->emplace_back(4); indices->emplace_back(5); indices->emplace_back(6);
		indices->emplace_back(6); indices->emplace_back(5); indices->emplace_back(7);

		// back
		indices->emplace_back(10); indices->emplace_back(9); indices->emplace_back(8);
		indices->emplace_back(11); indices->emplace_back(9); indices->emplace_back(10);

		// top
		indices->emplace_back(14); indices->emplace_back(13); indices->emplace_back(12);
		indices->emplace_back(15); indices->emplace_back(13); indices->emplace_back(14);

		// left
		indices->emplace_back(16); indices->emplace_back(17); indices->emplace_back(18);
		indices->emplace_back(18); indices->emplace_back(17); indices->emplace_back(19);

		// right
		indices->emplace_back(22); indices->emplace_back(21); indices->emplace_back(20);
		indices->emplace_back(23); indices->emplace_back(21); indices->emplace_back(22);
	}

	void MeshGenerator::GenerateCone(std::vector<MeshGenerator::MeshVertex>* vertices, std::vector<MeshGenerator::MeshIndex>* indices, float radius, float height)
	{
		GenerateCylinder(vertices, indices, 0.0f, radius, height);
	}

 void MeshGenerator::GenerateCylinder(std::vector<MeshGenerator::MeshVertex>* vertices, std::vector<MeshGenerator::MeshIndex>* indices, float radiusTop, float radiusBottom, float height, int slices, int stacks)
	{
		if (vertices == nullptr || indices == nullptr)
			return;

		if (slices < 3 || stacks < 1)
			return;

		vertices->clear();
		indices->clear();

		const float stackHeight = height / stacks;
		const float radiusStep = (radiusTop - radiusBottom) / stacks;
		const int ringCount = stacks + 1;

		for (int i = 0; i < ringCount; i++)
		{
			const float y = -0.5f * height + i * stackHeight;
			const float r = radiusBottom + i * radiusStep;
			const float dTheta = 2.0f * xMath::PI / slices;
			for (int j = 0; j <= slices; j++)
			{
				const float c = std::cos(j * dTheta);
				const float s = std::sin(j * dTheta);

				Vec3 v = Vec3(r*c, y, r*s);
				Vec2 uv = Vec2((float)j / slices, 1.0f - (float)i / stacks);
				Vec3 t = Vec3(-s, 0.0f, c);

				const float dr = radiusBottom - radiusTop;
				Vec3 bitangent = Vec3(dr*c, -height, dr*s);

				Vec3 n = xMath::Normalize(xMath::Cross(t, bitangent));
				vertices->emplace_back(v, uv, n, t);

			}
		}

		const int ringVertexCount = slices + 1;
		for (int i = 0; i < stacks; i++)
		{
			for (int j = 0; j < slices; j++)
			{
				indices->push_back(i * ringVertexCount + j);
				indices->push_back((i + 1) * ringVertexCount + j);
				indices->push_back((i + 1) * ringVertexCount + j + 1);

				indices->push_back(i * ringVertexCount + j);
				indices->push_back((i + 1) * ringVertexCount + j + 1);
				indices->push_back(i * ringVertexCount + j + 1);
			}
		}

		// build top cap
		int baseIndex = (int)vertices->size();
		float y = 0.5f * height;
		const float dTheta = 2.0f * xMath::PI / slices;

		Vec3 normal;
		Vec3 tangent;

		for (int i = 0; i <= slices; i++)
		{
		  const float x = radiusTop * std::cos(i * dTheta);
			const float z = radiusTop * std::sin(i * dTheta);
			const float u = x / height + 0.5f;
			const float v = z / height + 0.5f;

			normal = Vec3(0, 1, 0);
			tangent = Vec3(1, 0, 0);
			vertices->emplace_back(Vec3(x, y, z), Vec2(u, v), normal, tangent);
		}

		normal = Vec3(0, 1, 0);
		tangent = Vec3(1, 0, 0);
		vertices->emplace_back(Vec3(0, y, 0), Vec2(0.5f, 0.5f), normal, tangent);

		int centerIndex = (int)vertices->size() - 1;
		for (int i = 0; i < slices; i++)
		{
			indices->push_back(centerIndex);
			indices->push_back(baseIndex + i + 1);
			indices->push_back(baseIndex + i);
		}

		// build bottom cap
		baseIndex = (int)vertices->size();
		y = -0.5f * height;

		for (int i = 0; i <= slices; i++)
		{
		 const float x = radiusBottom * std::cos(i * dTheta);
			const float z = radiusBottom * std::sin(i * dTheta);
			const float u = x / height + 0.5f;
			const float v = z / height + 0.5f;

			normal  = Vec3(0, -1, 0);
			tangent = Vec3(1, 0, 0);
			vertices->emplace_back(Vec3(x, y, z), Vec2(u, v), normal, tangent);
		}

		normal  = Vec3(0, -1, 0);
		tangent = Vec3(1, 0, 0);
		vertices->emplace_back(Vec3(0, y, 0), Vec2(0.5f, 0.5f), normal, tangent);

		centerIndex = (int)vertices->size() - 1;
		for (int i = 0; i < slices; i++)
		{
			indices->push_back(centerIndex);
			indices->push_back(baseIndex + i);
			indices->push_back(baseIndex + i + 1);
		}
	}

	void MeshGenerator::GenerateSphere(std::vector<MeshGenerator::MeshVertex>* vertices, std::vector<MeshGenerator::MeshIndex>* indices, float radius, int slices, int stacks)
	{
		if (vertices == nullptr || indices == nullptr)
			return;

		if (slices < 3 || stacks < 2)
			return;

		vertices->clear();
		indices->clear();

		Vec3 normal = Vec3(0, 1, 0);
		Vec3 tangent = Vec3(1, 0, 0);
		vertices->emplace_back(Vec3(0, radius, 0), Vec2::ZERO, normal, tangent);

		const float phiStep   = xMath::PI / stacks;
		const float thetaStep = 2.0f * xMath::PI / slices;

		for (int i = 1; i <= stacks - 1; i++)
		{
			const float phi = i * phiStep;
			for (int j = 0; j <= slices; j++)
			{
				const float theta = j * thetaStep;
			  Vec3 p = Vec3(
					(radius * std::sin(phi) * std::cos(theta)),
					(radius * std::cos(phi)),
					(radius * std::sin(phi) * std::sin(theta))
				);

			 Vec3 t = xMath::Normalize(Vec3(-radius * std::sin(phi) * std::sin(theta), 0, radius * std::sin(phi) * std::cos(theta)));
				Vec3 n = xMath::Normalize(p);
				Vec2 uv = Vec2(theta / (xMath::PI * 2), phi / xMath::PI);
				vertices->emplace_back(p, uv, n, t);
			}
		}

		normal = Vec3(0, -1, 0);
		tangent = Vec3(1, 0, 0);
		vertices->emplace_back(Vec3(0, -radius, 0), Vec2(0, 1), normal, tangent);

		for (int i = 1; i <= slices; i++)
		{
			indices->emplace_back(0);
			indices->emplace_back(i + 1);
			indices->emplace_back(i);
		}
		int baseIndex = 1;
		const int ringVertexCount = slices + 1;
		for (int i = 0; i < stacks - 2; i++)
		{
			for (int j = 0; j < slices; j++)
			{
				indices->emplace_back(baseIndex + i * ringVertexCount + j);
				indices->emplace_back(baseIndex + i * ringVertexCount + j + 1);
				indices->emplace_back(baseIndex + (i + 1) * ringVertexCount + j);

				indices->emplace_back(baseIndex + (i + 1) * ringVertexCount + j);
				indices->emplace_back(baseIndex + i * ringVertexCount + j + 1);
				indices->emplace_back(baseIndex + (i + 1) * ringVertexCount + j + 1);
			}
		}
		int southPoleIndex = (int)vertices->size() - 1;
		baseIndex = southPoleIndex - ringVertexCount;
		for (int i = 0; i < slices; i++)
		{
			indices->emplace_back(southPoleIndex);
			indices->emplace_back(baseIndex + i);
			indices->emplace_back(baseIndex + i + 1);
		}
	}

  void MeshGenerator::GenerateGrid(std::vector<MeshGenerator::MeshVertex>* vertices, std::vector<MeshGenerator::MeshIndex>* indices, MeshGenerator::MeshIndex grid_points_per_dimension, float extent)
	{
		if (vertices == nullptr || indices == nullptr)
			return;

		if (grid_points_per_dimension < 2)
			return;

		vertices->clear();
		indices->clear();
	
		const float spacing = extent / static_cast<float>(grid_points_per_dimension - 1); // scale spacing based on extent
		const Vec3 normal(0, 1, 0);  // upward-facing normal (Y-axis)
		const Vec3 tangent(1, 0, 0); // tangent along X-axis
	
		// generate vertices
		for (uint32_t i = 0; i < grid_points_per_dimension; ++i)
		{
			for (uint32_t j = 0; j < grid_points_per_dimension; ++j)
			{
				const float x = static_cast<float>(i) * spacing - (extent / 2.0f); // center the grid around origin
				const float z = static_cast<float>(j) * spacing - (extent / 2.0f); // center the grid around origin
				const Vec2 texCoord(static_cast<float>(i) / (grid_points_per_dimension - 1), static_cast<float>(j) / (grid_points_per_dimension - 1)); // normalized UVs [0,1]
				vertices->emplace_back(Vec3(x, 0.0f, z), texCoord, normal, tangent);
			}
		}
	
		// generate indices (clockwise winding order for DirectX with back-face culling)
		for (uint32_t i = 0; i < grid_points_per_dimension - 1; ++i)
		{
			for (uint32_t j = 0; j < grid_points_per_dimension - 1; ++j)
			{
				int topLeft     = i * grid_points_per_dimension + j;
				int topRight    = i * grid_points_per_dimension + j + 1;
				int bottomLeft  = (i + 1) * grid_points_per_dimension + j;
				int bottomRight = (i + 1) * grid_points_per_dimension + j + 1;
	
				// triangle 1 (top-left, bottom-right, bottom-left) - clockwise when viewed from above
				indices->emplace_back(topLeft);
				indices->emplace_back(bottomRight);
				indices->emplace_back(bottomLeft);
	
				// triangle 2 (top-left, top-right, bottom-right) - clockwise when viewed from above
				indices->emplace_back(topLeft);
				indices->emplace_back(topRight);
				indices->emplace_back(bottomRight);
			}
		}
	}

	void MeshGenerator::GenerateQuad(std::vector<MeshGenerator::MeshVertex>* vertices, std::vector<MeshGenerator::MeshIndex>* indices)
	{
		if (vertices == nullptr || indices == nullptr)
			return;

		vertices->clear();
		indices->clear();

		vertices->emplace_back(Vec3(-0.5f, 0.0f, 0.5f),  Vec2(0, 0), Vec3(0, 1, 0), Vec3(1, 0, 0)); // 0 top-left
		vertices->emplace_back(Vec3(0.5f,  0.0f, 0.5f),  Vec2(1, 0), Vec3(0, 1, 0), Vec3(1, 0, 0)); // 1 top-right
		vertices->emplace_back(Vec3(-0.5f, 0.0f, -0.5f), Vec2(0, 1), Vec3(0, 1, 0), Vec3(1, 0, 0)); // 2 bottom-left
		vertices->emplace_back(Vec3(0.5f,  0.0f, -0.5f), Vec2(1, 1), Vec3(0, 1, 0), Vec3(1, 0, 0)); // 3 bottom-right

		indices->emplace_back(3);
		indices->emplace_back(2);
		indices->emplace_back(0);
		indices->emplace_back(3);
		indices->emplace_back(0);
		indices->emplace_back(1);
	}


}


// -------------------------------------------------------

