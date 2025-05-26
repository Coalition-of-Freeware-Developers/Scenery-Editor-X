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
#include <GraphicsEngine/scene/material.h>
#include <GraphicsEngine/scene/texture_manager.h>

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

    void MaterialAsset::Load(const std::string &path)
    {
        // Store the path
        materialPath = path;
        materialName = path.substr(path.find_last_of("/\\") + 1);
        
        // Default values
        color = Vec4(1.0f, 1.0f, 1.0f, 1.0f);
        emission = Vec3(0.0f);
        metallic = 0.0f;
        roughness = 1.0f;
        
        // Clear any existing texture references
        aoMap = nullptr;
        colorMap = nullptr;
        normalMap = nullptr;
        emissionMap = nullptr;
        metallicRoughnessMap = nullptr;
        
        SEDX_CORE_INFO("Material initialized with default values: {}", path);
    }

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
