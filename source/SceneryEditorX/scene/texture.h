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

#include <SceneryEditorX/renderer/vk_device.h>
#include <SceneryEditorX/scene/asset.h>
#include <vector>

// -------------------------------------------------------

namespace SceneryEditorX
{
	struct Serializer;
	
	// -------------------------------------------------------

	class TextureAsset : public Asset
    {
    public:
        TextureAsset();
        virtual void Serialize(Serializer &ser);

        // -------------------------------------------------------

        //virtual void Load(const std::string &path) override;
        //virtual void Unload() override;
        //[[nodiscard]] virtual bool IsLoaded() const override;
        //[[nodiscard]] virtual const std::string &GetPath() const override;
        //[[nodiscard]] virtual const std::string &GetName() const override;
        virtual void SetName(const std::string &name);

        std::vector<uint8_t> data;
        int channels = 0;
        int width = 0;
        int height = 0;

	private:
        void CreateTextureSampler();
        VkImageView CreateImageView(VkImage vkImage, VkFormat vkFormat, VkImageAspectFlagBits vkImageAspectFlagBits, int i);
        void CreateTextureImageView();
        void CreateTextureImage();

		RenderData renderData;
        //EditorConfig config;
		Ref<VulkanDevice> device;

		// -------------------------------------------------------

        VkDevice vkDevice = device->GetDevice();
        VkPhysicalDevice vkPhysDevice = device->GetPhysicalDevice()->GetGPUDevice();

        // -------------------------------------------------------

		VkImage textureImage;
        VkSampler textureSampler;
        VkImageView textureImageView;
        VkDeviceMemory textureImageMemory;

		// -------------------------------------------------------

        friend class AssetManager;
    };


} // namespace SceneryEditorX

// -------------------------------------------------------
