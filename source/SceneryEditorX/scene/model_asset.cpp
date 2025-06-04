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
#include <SceneryEditorX/platform/editor_config.hpp>
#include <SceneryEditorX/scene/model_asset.h>
#include <tiny_gltf.h>
#include <tiny_obj_loader.h>

/// -------------------------------------------------------

/*
template <>
struct std::hash<SceneryEditorX::MeshVertex>
{
    size_t operator()(const SceneryEditorX::MeshVertex &vertex) const noexcept
    {
        size_t h1 = hash<Vec3>()(vertex.position);
        size_t h2 = hash<Vec3>()(vertex.color);
        size_t h3 = hash<Vec2>()(vertex.texCoord);
        return h1 ^ (h2 << 1) ^ (h3 << 2); /// Combine the hashes
    }
};
*/

/// -------------------------------------------------------

namespace SceneryEditorX
{

	/*
	ModelAsset::ModelAsset()
	{
	    /// Initialize the model with default values
	    vertices.clear();
	    indices.clear();
        loaded = false;
        modelPath = "";
        modelName = "Unnamed Model";
	}

    ModelAsset::~ModelAsset() = default;

    /// -------------------------------------------------------

	void ModelAsset::Load(const std::string &path)
	{
        /// Get editor configuration
        EditorConfig config;

		///TODO: Setup the model loading process to use the editor config and EDX format
        ///TODO: Add loading X-Plane models dynamically from X-Plane 12 librarys

        /// Store the path
        modelPath = path;

        /// Construct the model path using the modelFolder from config
        std::string fullModelPath = config.modelFolder + "/" + path;

        SEDX_CORE_INFO("Loading 3D model from: {}", fullModelPath);

        tinyobj::attrib_t attrib;
        std::vector<tinyobj::shape_t> shapes;
        std::vector<tinyobj::material_t> materials;
        std::string warn, err;

        if (!tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err, fullModelPath.c_str()))
        {
            SEDX_CORE_ERROR("Failed to load model: {}", fullModelPath);
            SEDX_CORE_ERROR("Error details: {} {}", warn, err);
            loaded = false;
            return;
        }

        SEDX_CORE_INFO("Model loaded successfully: {} vertices, {} shapes", attrib.vertices.size() / 3, shapes.size());

        std::unordered_map<MeshVertex, uint32_t> uniqueVertices{};

        for (const auto &shape : shapes)
        {
            for (const auto &index : shape.mesh.indices)
            {
                MeshVertex vertex{};
                vertex.position = {
                    attrib.vertices[3 * index.vertex_index + 0],
                    attrib.vertices[3 * index.vertex_index + 1],
                    attrib.vertices[3 * index.vertex_index + 2]
                };

                /// Check if the model has texture coordinates
                vertex.texCoord = {attrib.texcoords[2 * index.texcoord_index + 0], 1.0f - attrib.texcoords[2 * index.texcoord_index + 1]};
                vertex.color = {1.0f, 1.0f, 1.0f};
                if (!uniqueVertices.contains(vertex))
                {
                    /// Convert Vertex to MeshVertex
                    MeshVertex meshVertex{};
                    meshVertex.position = {vertex.position.x, vertex.position.y, vertex.position.z};
                    meshVertex.texCoord = {vertex.texCoord.x, vertex.texCoord.y};
                    meshVertex.color = {vertex.color.x, vertex.color.y, vertex.color.z};

                    vertices.push_back(meshVertex);
                }
                indices.push_back(uniqueVertices[vertex]);
            }
        }

        SEDX_CORE_INFO("Model processing complete: {} unique vertices, {} indices", vertices.size(), indices.size());
        loaded = true;
	}
    
    void ModelAsset::Unload()
    {
        vertices.clear();
        indices.clear();
        loaded = false;
        SEDX_CORE_INFO("Model unloaded: {}", modelPath);
    }
    
    void ModelAsset::SetName(const std::string &name)
    {
        modelName = name;
        this->name = name; // Update the base Object::name
    }
    
    bool ModelAsset::IsLoaded() const
    {
        return loaded;
    }
    
    const std::string &ModelAsset::GetPath() const
    {
        return modelPath;
    }
    
    const std::string &ModelAsset::GetName() const
    {
        return modelName;
    }
    
    void ModelAsset::Serialize(Serializer &ser)
    {
        // Implement serialization logic for ModelAsset
        // This will depend on the exact implementation of the Serializer class
        // For now, providing a minimal implementation
    }
    */

} // namespace SceneryEditorX

/// -------------------------------------------------------
