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
#include <SceneryEditorX/scene/asset.h>
#include <SceneryEditorX/scene/texture.h>

/// -------------------------------------------------------

namespace SceneryEditorX
{
	struct Serializer;
	
	/// -------------------------------------------------------

    class MaterialAsset : public Asset
    {
    public:
        explicit MaterialAsset(const std::string & path);
        virtual ~MaterialAsset() override;
        virtual void Serialize(Serializer &ser) override;

        /// -------------------------------------------------------

        virtual void Load(const std::string &path) override;
        virtual void Unload() override;
        virtual void SetName(const std::string &name) override;
        //[[nodiscard]] virtual bool IsLoaded() const override;
        [[nodiscard]] virtual const std::string &GetPath() const;
        [[nodiscard]] virtual const std::string &GetName() const;

		/// -------------------------------------------------------

        Vec4 color = Vec4(1.0f);
        Vec3 emission = Vec3(0.0f);
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
	
} // namespace SceneryEditorX
