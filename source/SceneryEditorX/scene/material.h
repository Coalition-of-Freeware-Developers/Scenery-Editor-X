/**
* -------------------------------------------------------
* Scenery Editor X
* -------------------------------------------------------
* Copyright (c) 2025 Thomas Ray 
* Copyright (c) 2025 Coalition of Freeware Developers
* -------------------------------------------------------
* material.h
* -------------------------------------------------------
* Created: 15/4/2025
* -------------------------------------------------------
*/
#pragma once
#include <SceneryEditorX/asset/asset.h>
#include <SceneryEditorX/renderer/texture.h>
#include <SceneryEditorX/scene/texture.h>

/// -------------------------------------------------------

namespace SceneryEditorX
{
	struct Serializer;
	
	/// -------------------------------------------------------

    class MaterialAsset : public Asset
    {
    public:
        MaterialAsset() : color(1.0f), emission(0.0f), metallic(0.0f), roughness(1.0f) {}
        explicit MaterialAsset(const std::string & path);
        virtual ~MaterialAsset() override;
        virtual void Serialize(Serializer &ser);

        /// -------------------------------------------------------

        virtual void Load(const std::string &path) override;
        virtual void Unload() override;
        virtual void SetName(const std::string &name) override;
        [[nodiscard]] virtual bool IsLoaded() const;
        [[nodiscard]] virtual const std::string &GetPath() const;
        [[nodiscard]] virtual const std::string &GetName() const;
        Ref<Texture2D> GetAlbedoMap();
        Ref<Texture2D> GetRoughnessMap();
        Ref<Texture2D> GetMetalnessMap();
        Ref<Texture2D> GetNormalMap();

        /// -------------------------------------------------------

        Vec4 color = Vec4(1.0f);
        Vec3 emission = Vec3(0.0f);
        float metallic = 0.0f;
        float roughness = 1.0f;
        Ref<TextureAsset> aoMap;
        Ref<TextureAsset> colorMap;
        Ref<TextureAsset> normalMap;
        Ref<TextureAsset> emissionMap;
        Ref<TextureAsset> metallicRoughnessMap;

    private:
        friend class AssetManager;
        std::string materialPath;
        std::string materialName;
    };
	
}

/// -------------------------------------------------------
