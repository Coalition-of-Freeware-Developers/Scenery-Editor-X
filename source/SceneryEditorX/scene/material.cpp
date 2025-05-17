/**
* -------------------------------------------------------
* Scenery Editor X
* -------------------------------------------------------
* Copyright (c) 2025 Thomas Ray 
* Copyright (c) 2025 Coalition of Freeware Developers
* -------------------------------------------------------
* material.cpp
* -------------------------------------------------------
* Created: 16/4/2025
* -------------------------------------------------------
*/
#include <fstream>
#include <json.hpp>
#include <SceneryEditorX/logging/logging.hpp>
#include <SceneryEditorX/platform/file_manager.hpp>
#include <SceneryEditorX/scene/material.h>
#include <SceneryEditorX/scene/texture_manager.h>

// -------------------------------------------------------

namespace SceneryEditorX
{
    // Use nlohmann json for parsing material files
    using json = nlohmann::json;

    MaterialAsset::MaterialAsset(const std::string &path)
    {
        Load(path);
    }

    MaterialAsset::~MaterialAsset()
    {
        Unload();
    }

    void MaterialAsset::Serialize(Serializer &ser)
    {
        // TODO: Implement serialization
        // This would store material properties in a specific format
    }

    /*
    void MaterialAsset::Load(const std::string &path)
    {
        // Store the path
        materialPath = path;
        materialName = path.substr(path.find_last_of("/\\") + 1);
        
        // Default values
        color = Vec4(1.0f, 1.0f, 1.0f, 1.0f);
        emission = Vec3(0.0f);
        
        // Try to read material from file (assuming a simple JSON-based material format)

        try 
        {
            if (IO::FileManager::FileExists(path))
            {
                std::string jsonContent = IO::FileManager::ReadRawBytes(path);
                if (!jsonContent.empty()) 
                {
                    TextureManager textureManager;
                    json materialData = json::parse(jsonContent);
                    
                    // Parse base color
                    if (materialData.contains("color")) 
                    {
                        auto colorData = materialData["color"];
                        if (colorData.is_array() && colorData.size() >= 4) 
                        {
                            color.x = colorData[0];
                            color.y = colorData[1];
                            color.z = colorData[2];
                            color.w = colorData[3];
                        }
                    }
                    
                    // Parse emission
                    if (materialData.contains("emission")) 
                    {
                        auto emissionData = materialData["emission"];
                        if (emissionData.is_array() && emissionData.size() >= 3) 
                        {
                            emission.x = emissionData[0];
                            emission.y = emissionData[1];
                            emission.z = emissionData[2];
                        }
                    }
                    
                    // Get texture folder from editor config
                    EditorConfig config;
                    std::string textureBasePath = config.textureFolder;
                    
                    // Parse texture maps
                    // AO Map
                    if (materialData.contains("aoMap") && materialData["aoMap"].is_string()) 
                    {
                        std::string texPath = materialData["aoMap"];
                        aoMap = textureManager.LoadTexture(textureBasePath + "/" + texPath);
                    }
                    
                    // Color Map
                    if (materialData.contains("colorMap") && materialData["colorMap"].is_string()) 
                    {
                        std::string texPath = materialData["colorMap"];
                        colorMap = textureManager.LoadTexture(textureBasePath + "/" + texPath);
                    }
                    
                    // Normal Map
                    if (materialData.contains("normalMap") && materialData["normalMap"].is_string()) 
                    {
                        std::string texPath = materialData["normalMap"];
                        normalMap = textureManager.LoadTexture(textureBasePath + "/" + texPath);
                    }
                    
                    // Emission Map
                    if (materialData.contains("emissionMap") && materialData["emissionMap"].is_string()) 
                    {
                        std::string texPath = materialData["emissionMap"];
                        emissionMap = textureManager.LoadTexture(textureBasePath + "/" + texPath);
                    }
                    
                    // Metallic Roughness Map
                    if (materialData.contains("metallicRoughnessMap") && materialData["metallicRoughnessMap"].is_string()) 
                    {
                        std::string texPath = materialData["metallicRoughnessMap"];
                        metallicRoughnessMap = textureManager.LoadTexture(textureBasePath + "/" + texPath);
                    }
                    
                    SEDX_CORE_INFO("Material loaded successfully: {}", path);
                    return;
                }
            }
        }
        catch (const std::exception& e) 
        {
            SEDX_CORE_ERROR("Error loading material from {}: {}", path, e.what());
        }
        
        // If we reach here, use default values for the material
        SEDX_CORE_WARN("Using default values for material: {}", path);
    }
    */

    void MaterialAsset::Unload()
    {
        // Release texture references
        aoMap = nullptr;
        colorMap = nullptr;
        normalMap = nullptr;
        emissionMap = nullptr;
        metallicRoughnessMap = nullptr;
    }

    void MaterialAsset::SetName(const std::string &name)
    {
        materialName = name;
    }

    const std::string &MaterialAsset::GetPath() const
    {
        return materialPath;
    }

    const std::string &MaterialAsset::GetName() const
    {
        return materialName;
    }

} // namespace SceneryEditorX

// -------------------------------------------------------
