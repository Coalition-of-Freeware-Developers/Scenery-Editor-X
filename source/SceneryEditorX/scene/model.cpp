/**
* -------------------------------------------------------
* Scenery Editor X
* -------------------------------------------------------
* Copyright (c) 2025 Thomas Ray 
* Copyright (c) 2025 Coalition of Freeware Developers
* -------------------------------------------------------
* model.cpp
* -------------------------------------------------------
* Created: 15/4/2025
* -------------------------------------------------------
*/

//#include <SceneryEditorX/platform/windows/editor_config.hpp>
//#include <SceneryEditorX/scene/model.h>
//#include <string>
//#include <tiny_obj_loader.h>

// -------------------------------------------------------

namespace SceneryEditorX
{
	/*
	Mesh::Mesh()
	{
		// Constructor implementation
	}

	Mesh::~Mesh()
	{
		// Destructor implementation
	}

	void Mesh::Load(const std::string& path)
	{
        // Get editor configuration
        EditorConfig config;

        // Construct the model path using the modelFolder from config
        std::string modelPath = config.modelFolder + "/viking_room.obj";

        SEDX_CORE_INFO("Loading 3D model from: {}", modelPath);

        tinyobj::attrib_t attrib;
        std::vector<tinyobj::shape_t> shapes;
        std::vector<tinyobj::material_t> materials;
        std::string warn, err;

        if (!tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err, modelPath.c_str()))
        {
            SEDX_CORE_ERROR("Failed to load model: {}", modelPath);
            SEDX_CORE_ERROR("Error details: {} {}", warn, err);
            throw std::runtime_error(warn + err);
        }

        SEDX_CORE_INFO("Model loaded successfully: {} vertices, {} shapes", attrib.vertices.size() / 3, shapes.size());

        std::unordered_map<Vertex, uint32_t> uniqueVertices{};

        for (const auto &shape : shapes)
        {
            for (const auto &index : shape.mesh.indices)
            {
                Vertex vertex{};

                vertex.pos = {attrib.vertices[3 * index.vertex_index + 0],
                                attrib.vertices[3 * index.vertex_index + 1],
                                attrib.vertices[3 * index.vertex_index + 2]};

                // Check if the model has texture coordinates
                vertex.texCoord = {attrib.texcoords[2 * index.texcoord_index + 0], 1.0f - attrib.texcoords[2 * index.texcoord_index + 1]};

                vertex.color = {1.0f, 1.0f, 1.0f};

                if (!uniqueVertices.contains(vertex))
                {
                    // Convert Vertex to MeshVertex
                    MeshVertex meshVertex{};
                    meshVertex.position = {vertex.pos.x, vertex.pos.y, vertex.pos.z};
                    meshVertex.texCoord = {vertex.texCoord.x, vertex.texCoord.y};
                    meshVertex.color = {vertex.color.x, vertex.color.y, vertex.color.z};

                    vertices.push_back(meshVertex);
                }

                indices.push_back(uniqueVertices[vertex]);
            }
        }

        SEDX_CORE_INFO("Model processing complete: {} unique vertices, {} indices", vertices.size(), indices.size());
	}
	*/

} // namespace SceneryEditorX

// -------------------------------------------------------
