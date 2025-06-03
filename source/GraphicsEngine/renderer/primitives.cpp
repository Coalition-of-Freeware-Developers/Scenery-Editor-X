/**
* -------------------------------------------------------
* Scenery Editor X
* -------------------------------------------------------
* Copyright (c) 2025 Thomas Ray 
* Copyright (c) 2025 Coalition of Freeware Developers
* -------------------------------------------------------
* primitives.cpp
* -------------------------------------------------------
* Created: 20/5/2025
* -------------------------------------------------------
*/
#include <glm/glm.hpp>
#include <GraphicsEngine/renderer/primitives.h>
#include <numbers>
#include <SceneryEditorX//scene/model_asset.h>

// -------------------------------------------------------

namespace SceneryEditorX
{
	
	/*
	ObjectType Primitives::CreateBox(const glm::vec3 &size)
	{
        std::vector<MeshVertex> vertices;
		vertices.resize(8);
		vertices[0].position = { -size.x / 2.0f, -size.y / 2.0f,  size.z / 2.0f };
		vertices[1].position = {  size.x / 2.0f, -size.y / 2.0f,  size.z / 2.0f };
		vertices[2].position = {  size.x / 2.0f,  size.y / 2.0f,  size.z / 2.0f };
		vertices[3].position = { -size.x / 2.0f,  size.y / 2.0f,  size.z / 2.0f };
		vertices[4].position = { -size.x / 2.0f, -size.y / 2.0f, -size.z / 2.0f };
		vertices[5].position = {  size.x / 2.0f, -size.y / 2.0f, -size.z / 2.0f };
		vertices[6].position = {  size.x / 2.0f,  size.y / 2.0f, -size.z / 2.0f };
		vertices[7].position = { -size.x / 2.0f,  size.y / 2.0f, -size.z / 2.0f };

		vertices[0].normal = { -1.0f, -1.0f,  1.0f };
		vertices[1].normal = {  1.0f, -1.0f,  1.0f };
		vertices[2].normal = {  1.0f,  1.0f,  1.0f };
		vertices[3].normal = { -1.0f,  1.0f,  1.0f };
		vertices[4].normal = { -1.0f, -1.0f, -1.0f };
		vertices[5].normal = {  1.0f, -1.0f, -1.0f };
		vertices[6].normal = {  1.0f,  1.0f, -1.0f };
		vertices[7].normal = { -1.0f,  1.0f, -1.0f };

		std::vector<ModelAsset::Index> indices;
		indices.resize(12);
		indices[0] =  {.V1 = 0,.V2 = 1,.V3 = 2 };
		indices[1] =  {.V1 = 2,.V2 = 3,.V3 = 0 };
		indices[2] =  {.V1 = 1,.V2 = 5,.V3 = 6 };
		indices[3] =  {.V1 = 6,.V2 = 2,.V3 = 1 };
		indices[4] =  {.V1 = 7,.V2 = 6,.V3 = 5 };
		indices[5] =  {.V1 = 5,.V2 = 4,.V3 = 7 };
		indices[6] =  {.V1 = 4,.V2 = 0,.V3 = 3 };
		indices[7] =  {.V1 = 3,.V2 = 7,.V3 = 4 };
		indices[8] =  {.V1 = 4,.V2 = 5,.V3 = 1 };
		indices[9] =  {.V1 = 1,.V2 = 0,.V3 = 4 };
		indices[10] = {.V1 = 3,.V2 = 2,.V3 = 6 };
		indices[11] = {.V1 = 6,.V2 = 7,.V3 = 3 };

		//ObjectType meshSource = Model::CreateMesh(vertices, indices, glm::mat4(1.0f));
	    return {};
	}
	
	ObjectType Primitives::CreateSphere(const float radius)
	{
        std::vector<MeshVertex> vertices;
        std::vector<ModelAsset::Index> indices;

		constexpr float latitudeBands = 30;
		constexpr float longitudeBands = 30;

		for (float latitude = 0.0f; latitude <= latitudeBands; latitude++)
		{
			const float theta = latitude * std::numbers::pi_v<float> / latitudeBands;
			const float sinTheta = glm::sin(theta);
			const float cosTheta = glm::cos(theta);

			for (float longitude = 0.0f; longitude <= longitudeBands; longitude++)
			{
				const float phi = longitude * 2.f * std::numbers::pi_v<float> / longitudeBands;
				const float sinPhi = glm::sin(phi);
				const float cosPhi = glm::cos(phi);

				MeshVertex vertex;
				vertex.normal = { cosPhi * sinTheta, cosTheta, sinPhi * sinTheta };
				vertex.position = { radius * vertex.normal.x, radius * vertex.normal.y, radius * vertex.normal.z };
				vertices.push_back(vertex);
			}
		}

		for (uint32_t latitude = 0; latitude < (uint32_t)latitudeBands; latitude++)
		{
			for (uint32_t longitude = 0; longitude < (uint32_t)longitudeBands; longitude++)
			{
				const uint32_t first = (latitude * ((uint32_t)longitudeBands + 1)) + longitude;
				const uint32_t second = first + (uint32_t)longitudeBands + 1;

				indices.push_back({.V1 = first,.V2 = second,.V3 = first + 1 });
				indices.push_back({.V1 = second,.V2 = second + 1,.V3 = first + 1 });
			}
		}

        return{};
	}
	
	ObjectType Primitives::CreateCylinder(float radius, float height)
	{
		std::vector<MeshVertex> vertices;
		std::vector<ModelAsset::Index> indices;

		MeshVertex topCenter = {
		    .position = {0, height / 2, 0},
		    .color = {0, 1, 0},
            .normal = {0, 1, 0},
            .tangent = {1.0f, 0.0f, 0.0f, 0.0f},
			.texCoord = {0.0f, 0.0f}
		};

		MeshVertex bottomCenter = {
			.position = {0, -height / 2, 0},
		    .color = {0, -1, 0},
			.normal = {0, -1, 0},
            .tangent = {1.0f, 0.0f, 0.0f, 0.0f},
			.texCoord = {0.0f, 0.0f}
		};

		vertices.push_back(topCenter);
		vertices.push_back(bottomCenter);

        constexpr int segments = 30;
		for (int i = 0; i <= segments; ++i)
		{
			float theta = (float)i / segments * 2.0f * std::numbers::pi_v<float>;
			float x = radius * cos(theta);
			float z = radius * sin(theta);
			vertices.push_back({
				.position = {x, height / 2, z},
				.color = {x, 0, z},
			    .normal = {x, 0, z},
				.tangent = {1.0f, 0.0f, 0.0f, 0.0f},
				.texCoord = {0.0f, 0.0f}
			});
			
            vertices.push_back({
                .position = {x, -height / 2, z},
                .color = {x, 0, z},
                .normal = {x, 0, z},
                .tangent = {1.0f, 0.0f, 0.0f, 0.0f},
                .texCoord = {0.0f, 0.0f}
            });
		}

		for (int i = 1; i < segments; ++i)
		{
			indices.push_back({(uint32_t)0, (uint32_t)(i * 2), (uint32_t)((i - 1) * 2)});
			indices.push_back({(uint32_t)1, (uint32_t)((i - 1) * 2 + 1), (uint32_t)(i * 2 + 1)});
			indices.push_back({(uint32_t)(i * 2), (uint32_t)((i - 1) * 2), (uint32_t)((i - 1) * 2 + 1)});
			indices.push_back({(uint32_t)(i * 2), (uint32_t)((i - 1) * 2 + 1), (uint32_t)(i * 2 + 1)});
		}

	    return {};
	}
	
	ObjectType Primitives::CreatePlane(const Vec2 &size)
	{
	    return {};
	}
	
	ObjectType Primitives::CreatePyramid(const Vec3 &baseSize, float height)
	{
	    return {};
	}
	*/

}

/// -------------------------------------------------------
