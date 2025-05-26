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
#include <SceneryEditorX/platform/editor_config.hpp>
#include <GraphicsEngine/scene/asset.h>
#include <GraphicsEngine/vulkan/vk_device.h>

// -------------------------------------------------------

namespace SceneryEditorX
{
	struct Serializer;
	
	// -------------------------------------------------------

	class TextureAsset : public Asset
    {
    public:
        TextureAsset() : channels(0), width(0), height(0),
                         textureImage(VK_NULL_HANDLE), textureSampler(VK_NULL_HANDLE),
                         textureImageView(VK_NULL_HANDLE), textureImageMemory(VK_NULL_HANDLE) {}
        explicit TextureAsset(const std::string &path);
        virtual ~TextureAsset() override;
        virtual void Serialize(Serializer &ser) override;

        // -------------------------------------------------------

        virtual void Load(const std::string &path) override;
        virtual void Unload() override;

	    void LoadWithAllocator();
        void UnloadWithAllocator();
        //[[nodiscard]] virtual bool IsLoaded() const override;
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
		VkImageView CreateImageView(VkImage vkImage, VkFormat vkFormat, VkImageAspectFlagBits vkImageAspectFlagBits, int i);

		Ref<RenderData> renderData;
        WeakRef<EditorConfig> config;
        Ref<VulkanDevice> vkDevice = nullptr;
        Ref<VulkanPhysicalDevice> vkPhysDevice = nullptr;
        Ref<MemoryAllocator> allocator;
        std::string texturePath;
        std::string textureName;
		// -------------------------------------------------------

		VkImage textureImage = VK_NULL_HANDLE;
        VkSampler textureSampler = VK_NULL_HANDLE;
        VkImageView textureImageView = VK_NULL_HANDLE;
        VkDeviceMemory textureImageMemory = VK_NULL_HANDLE;

		// -------------------------------------------------------

        friend class AssetManager;
    };


} // namespace SceneryEditorX

// -------------------------------------------------------
