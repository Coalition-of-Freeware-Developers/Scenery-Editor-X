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
#include "SceneryEditorX/asset/asset.h"
#include "SceneryEditorX/platform/config/editor_config.hpp"

/// -------------------------------------------------------

namespace SceneryEditorX
{
    /**
     * @brief Represents a texture asset managed by the editor.
     *
     * All texture assets must inherit from RefCounted to support reference counting.
     */
    class TextureAsset : public Object
    {
    public:
        TextureAsset();
        explicit TextureAsset(const std::string &path);
        virtual ~TextureAsset() override;
        //virtual void Serialize(Serializer &ser);

        /// -------------------------------------------------------

        virtual void Load(const std::string &path);
        virtual void Unload();

		///TODO: Add these texture allocation functions
        static void LoadWithAllocator();
        static void UnloadWithAllocator();
        [[nodiscard]] virtual bool IsLoaded() const;
        [[nodiscard]] virtual const std::string &GetPath() const;
        [[nodiscard]] virtual const std::string &GetName() const;
        virtual void SetName(const std::string &name);

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
