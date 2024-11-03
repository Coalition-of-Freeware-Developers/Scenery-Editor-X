#pragma once

#include <filesystem>
#include <mutex>

#include "../scene/Scene.hpp"
#include "../renderer/VK_Wrapper.h"

/**
 * @brief Structure to describe a texture.
 * 
 * This structure holds the necessary information to describe a texture,
 * including its file path, raw data, width, and height.
 */
struct TextureDesc
{
    std::filesystem::path path = ""; ///< The file path to the texture.
    void *data = nullptr;            ///< Pointer to the raw texture data.
    uint32_t width = 0;              ///< The width of the texture in pixels.
    uint32_t height = 0;             ///< The height of the texture in pixels.
};

/**
 * @brief Structure to describe a mesh vertex.
 * 
 * This structure holds the necessary information to describe a mesh vertex,
 * including its position, normal, tangent, and texture coordinates.
 */
struct MeshVertex
{
    glm::vec3 pos;      ///< The position of the vertex.
    glm::vec3 normal;   ///< The normal vector at the vertex.
    glm::vec4 tangent;  ///< The tangent vector at the vertex.
    glm::vec2 texCoord; ///< The texture coordinates of the vertex.

    /**
     * @brief Equality operator for MeshVertex.
     * 
     * Compares two MeshVertex objects for equality based on their position,
     * normal, and texture coordinates.
     * 
     * @param other The other MeshVertex to compare with.
     * @return true if the vertices are equal, false otherwise.
     */
    bool operator==(const MeshVertex &other) const
    {
        return pos == other.pos && normal == other.normal && texCoord == other.texCoord; 
    }
};

namespace std
{
    /**
     * @brief Hash function specialization for MeshVertex.
     * 
     * This specialization of the std::hash template provides a hash function
     * for MeshVertex objects. It combines the hashes of the position, normal,
     * and texture coordinates of the vertex to produce a unique hash value.
     */
    template <> struct hash<MeshVertex>
    {
        size_t operator()(MeshVertex const &vertex) const
        {
            return ((hash<glm::vec3>()(vertex.pos) ^ (hash<glm::vec3>()(vertex.normal) << 1)) >> 1) ^
                   (hash<glm::vec2>()(vertex.texCoord) << 1);
        }
    };
}

/**
 * @brief Structure to describe a mesh.
 * 
 * This structure holds the necessary information to describe a mesh,
 * including its vertices, indices, file path, name, and center position.
 */
struct MeshDesc
{
    std::vector<MeshVertex> vertices; ///< The vertices of the mesh.
    std::vector<uint32_t> indices;    ///< The indices of the mesh.
    std::filesystem::path path;       ///< The file path to the mesh.
    std::string name;                 ///< The name of the mesh.
    glm::vec3 center = glm::vec3(0, 0, 0); ///< The center position of the mesh.
};

/**
 * @brief Structure to describe a mesh resource.
 * 
 * This structure holds the necessary information to describe a mesh resource,
 * including its vertex buffer, index buffer, vertex count, index count, and BLAS.
 */
struct MeshResource
{
    vkw::Buffer vertexBuffer; ///< The buffer containing the vertices of the mesh.
    vkw::Buffer indexBuffer;  ///< The buffer containing the indices of the mesh.
    u32 vertexCount;          ///< The number of vertices in the mesh.
    u32 indexCount;           ///< The number of indices in the mesh.
    vkw::BLAS blas;           ///< The bottom-level acceleration structure for the mesh.
};

/// @brief Maximum number of meshes that can be managed.
#define MAX_MESHES 2048 

/// @brief Maximum number of textures that can be managed.
#define MAX_TEXTURES 2048

class AssetManager
{
    /**
     * @brief Vector to store uninitialized mesh resource IDs.
     * 
     * This vector holds the resource IDs of meshes that have been created but not yet initialized.
     */
    static inline std::vector<RID> unintializedMeshes;

    /**
     * @brief Mutex to protect access to the uninitialized meshes vector.
     * 
     * This mutex ensures that access to the unintializedMeshes vector is thread-safe.
     */
    static inline std::mutex meshesLock;

    /**
     * @brief The next available texture resource ID.
     * 
     * This variable holds the next available resource ID for textures.
     */
    static inline RID nextTextureRID = 0;

    /**
     * @brief Vector to store uninitialized texture resource IDs.
     * 
     * This vector holds the resource IDs of textures that have been created but not yet initialized.
     */
    static inline std::vector<RID> unintializedTextures;

    /**
     * @brief Mutex to protect access to the uninitialized textures vector.
     * 
     * This mutex ensures that access to the unintializedTextures vector is thread-safe.
     */
    static inline std::mutex texturesLock;

    /**
     * @brief Vector to store loaded models.
     * 
     * This vector holds the models that have been loaded into memory.
     */
    static inline std::vector<Model> loadedModels;

    /**
     * @brief Mutex to protect access to the loaded models vector.
     * 
     * This mutex ensures that access to the loadedModels vector is thread-safe.
     */
    static inline std::mutex loadedModelsLock;

    /**
     * @brief Create a new mesh resource ID.
     * 
     * This function generates a new resource ID for a mesh.
     * 
     * @return The new mesh resource ID.
     */
    static RID NewMesh();

    /**
     * @brief Initialize a mesh with the given resource ID.
     * 
     * This function initializes a mesh using the provided resource ID.
     * 
     * @param id The resource ID of the mesh to initialize.
     */
    static void InitializeMesh(RID id);

    /**
     * @brief Recenter a mesh with the given resource ID.
     * 
     * This function recenters a mesh using the provided resource ID.
     * 
     * @param id The resource ID of the mesh to recenter.
     */
    static void RecenterMesh(RID id);

    /**
     * @brief Create a new texture resource ID.
     * 
     * This function generates a new resource ID for a texture.
     * 
     * @return The new texture resource ID.
     */
    static RID NewTexture();

    /**
     * @brief Initialize a texture with the given resource ID.
     * 
     * This function initializes a texture using the provided resource ID.
     * 
     * @param id The resource ID of the texture to initialize.
     */
    static void InitializeTexture(RID id);

    /**
     * @brief Update the textures descriptor with the given resource IDs.
     * 
     * This function updates the textures descriptor using the provided resource IDs.
     * 
     * @param rids The vector of resource IDs to update the textures descriptor with.
     */
    static void UpdateTexturesDescriptor(std::vector<RID> &rids);

    /**
     * @brief Load an OBJ file from the given path.
     * 
     * This function loads an OBJ file from the specified file path.
     * 
     * @param path The file path to the OBJ file.
     */
    static void LoadOBJ(std::filesystem::path path);

    /**
     * @brief Load a GLTF file from the given path.
     * 
     * This function loads a GLTF file from the specified file path.
     * 
     * @param path The file path to the GLTF file.
     */
    static void LoadGLTF(std::filesystem::path path);

    /**
     * @brief Check if the given path is an OBJ file.
     * 
     * This function checks if the specified file path points to an OBJ file.
     * 
     * @param path The file path to check.
     * @return true if the path points to an OBJ file, false otherwise.
     */
    static bool IsOBJ(std::filesystem::path path);

    /**
     * @brief Check if the given path is a GLTF file.
     * 
     * This function checks if the specified file path points to a GLTF file.
     * 
     * @param path The file path to check.
     * @return true if the path points to a GLTF file, false otherwise.
     */
    static bool IsGLTF(std::filesystem::path path);

public:

    /**
     * @brief The next available mesh resource ID.
     * 
     * This variable holds the next available resource ID for meshes.
     */
    static inline RID nextMeshRID = 0;

    /**
     * @brief Array to store mesh descriptions.
     * 
     * This array holds the descriptions of all managed meshes.
     */
    static inline MeshDesc meshDescs[MAX_MESHES];

    /**
     * @brief Array to store mesh resources.
     * 
     * This array holds the resources of all managed meshes.
     */
    static inline MeshResource meshes[MAX_MESHES];

    /**
     * @brief Array to store texture descriptions.
     * 
     * This array holds the descriptions of all managed textures.
     */
    static inline TextureDesc textureDescs[MAX_TEXTURES];

    /**
     * @brief Array to store images.
     * 
     * This array holds the images of all managed textures.
     */
    static inline vkw::Image images[MAX_TEXTURES];

    /**
     * @brief Setup the asset manager.
     * 
     * This function sets up the asset manager.
     */
    static void Setup();

    /**
     * @brief Create assets.
     * 
     * This function creates assets managed by the asset manager.
     */
    static void Create();

    /**
     * @brief Destroy assets.
     * 
     * This function destroys assets managed by the asset manager.
     */
    static void Destroy();

    /**
     * @brief Finish asset management.
     * 
     * This function finalizes the asset management process.
     */
    static void Finish();

    /**
     * @brief Render ImGui interface for the asset manager.
     * 
     * This function renders the ImGui interface for the asset manager.
     */
    static void OnImgui();

    /**
     * @brief Update resources managed by the asset manager.
     * 
     * This function updates the resources managed by the asset manager.
     */
    static void UpdateResources();

    /**
     * @brief Check if the given path is a model file.
     * 
     * This function checks if the specified file path points to a model file.
     * 
     * @param path The file path to check.
     * @return true if the path points to a model file, false otherwise.
     */
    static bool IsModel(std::filesystem::path path);

    /**
     * @brief Check if the given path is a texture file.
     * 
     * This function checks if the specified file path points to a texture file.
     * 
     * @param path The file path to check.
     * @return true if the path points to a texture file, false otherwise.
     */
    static bool IsTexture(std::filesystem::path path);

    /**
     * @brief Create a texture with the given parameters.
     * 
     * This function creates a texture using the specified name, data, width, and height.
     * 
     * @param name The name of the texture.
     * @param data Pointer to the texture data.
     * @param width The width of the texture.
     * @param height The height of the texture.
     * @return The resource ID of the created texture.
     */
    static RID CreateTexture(std::string name, u8 *data, u32 width, u32 height);

    /**
     * @brief Load a texture from the given path.
     * 
     * This function loads a texture from the specified file path.
     * 
     * @param path The file path to the texture.
     * @return The resource ID of the loaded texture.
     */
    static RID LoadTexture(std::filesystem::path path);

    /**
     * @brief Load a model from the given path.
     * 
     * This function loads a model from the specified file path.
     * 
     * @param path The file path to the model.
     * @return Pointer to the loaded model.
     */
    static Model *LoadModel(std::filesystem::path path);

    /**
     * @brief Asynchronously load models from the given path.
     * 
     * This function asynchronously loads models from the specified file path.
     * 
     * @param path The file path to the models.
     */
    static void AsyncLoadModels(std::filesystem::path path);

    /**
     * @brief Load models from the given path.
     * 
     * This function loads models from the specified file path.
     * 
     * @param path The file path to the models.
     * @return Vector of pointers to the loaded models.
     */
    static std::vector<Model *> LoadModels(std::filesystem::path path);

    /**
     * @brief Get the loaded models.
     * 
     * This function returns a vector of pointers to the loaded models.
     * 
     * @return Vector of pointers to the loaded models.
     */
    static std::vector<Model *> GetLoadedModels();
};
