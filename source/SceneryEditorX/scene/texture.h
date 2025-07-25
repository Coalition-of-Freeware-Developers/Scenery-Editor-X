/**
* -------------------------------------------------------
* Scenery Editor X
* -------------------------------------------------------
* Copyright (c) 2025 Thomas Ray 
* Copyright (c) 2025 Coalition of Freeware Developers
* -------------------------------------------------------
* texture.h
* -------------------------------------------------------
* Created: 15/4/2025
* -------------------------------------------------------
*/
#pragma once
#include <SceneryEditorX/asset/asset.h>
#include <SceneryEditorX/platform/editor_config.hpp>
#include <SceneryEditorX/serialization/graph_serializer.h>
//#include <SceneryEditorX/serialization/serializer.hpp>

/// -------------------------------------------------------

namespace SceneryEditorX
{

	class TextureAsset : public Asset
    {
    public:
        TextureAsset() = default;
        explicit TextureAsset(const std::string &path);
        virtual ~TextureAsset() override;
        //virtual void Serialize(Serializer &ser);

        /// -------------------------------------------------------

        virtual void Load(const std::string &path) override;
        virtual void Unload() override;

		///TODO: Add these texture allocation functions
        GLOBAL void LoadWithAllocator();
        GLOBAL void UnloadWithAllocator();
        [[nodiscard]] virtual bool IsLoaded() const;

        [[nodiscard]] virtual const std::string &GetPath() const;
        [[nodiscard]] virtual const std::string &GetName() const;
        virtual void SetName(const std::string &name) override;

        std::vector<uint8_t> data;
        int channels = 0;
        int width = 0;
        int height = 0;

	private:
        void CreateTextureImage();
        void CreateTextureImageView();
        void CreateTextureSampler();
		VkImageView CreateImageView(VkImage image, VkFormat format, VkImageAspectFlagBits aspectFlags, int mipLevels) const;
        WeakRef<EditorConfig> config;

        /// -------------------------------------------------------

        std::string texturePath;
        std::string textureName;

		/// -------------------------------------------------------

		VkImage textureImage = VK_NULL_HANDLE;
        VkSampler textureSampler = VK_NULL_HANDLE;
        VkImageView textureImageView = VK_NULL_HANDLE;
        VkDeviceMemory textureImageMemory = VK_NULL_HANDLE;

		/// -------------------------------------------------------

        friend class AssetManager;
    };

}

/// -------------------------------------------------------
