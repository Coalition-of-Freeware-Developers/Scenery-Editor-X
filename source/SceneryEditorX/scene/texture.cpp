/**
* -------------------------------------------------------
* Scenery Editor X
* -------------------------------------------------------
* Copyright (c) 2025 Thomas Ray 
* Copyright (c) 2025 Coalition of Freeware Developers
* -------------------------------------------------------
* texture.cpp
* -------------------------------------------------------
* Created: 16/4/2025
* -------------------------------------------------------
*/
#include <SceneryEditorX/renderer/render_data.h>
#include <SceneryEditorX/scene/texture.h>
#include <SceneryEditorX/vulkan/vk_allocator.h>

// -------------------------------------------------------

namespace SceneryEditorX
{

	void TextureAsset::Load(const std::string &path)
	{

	}

    void TextureAsset::Unload()
    {
        vkDestroyImage(vkDevice->GetDevice(), textureImage, nullptr);
        vkFreeMemory(vkDevice->GetDevice(), textureImageMemory, nullptr);
    }

    /*
    const std::string & TextureAsset::GetPath() const
    {
        return Asset::Load()
    }
    */

    void TextureAsset::CreateTextureSampler()
	{
	    VkPhysicalDeviceProperties properties{};
        vkGetPhysicalDeviceProperties(vkPhysDevice->GetGPUDevice(), &properties);
	
	    VkSamplerCreateInfo samplerInfo{};
	    samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
	    samplerInfo.magFilter = VK_FILTER_LINEAR;
	    samplerInfo.minFilter = VK_FILTER_LINEAR;
	    samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
	    samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
	    samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
	    samplerInfo.anisotropyEnable = VK_TRUE;
	    samplerInfo.maxAnisotropy = properties.limits.maxSamplerAnisotropy;
	    samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
	    samplerInfo.unnormalizedCoordinates = VK_FALSE;
	    samplerInfo.compareEnable = VK_FALSE;
	    samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;
	    samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
	
	    if (vkCreateSampler(vkDevice->GetDevice(), &samplerInfo, nullptr, &textureSampler) != VK_SUCCESS)
	    {
	        ErrMsg("failed to create texture sampler!");
	    }
	}

    void TextureAsset::CreateTextureImageView()
    {
        textureImageView = CreateImageView(textureImage, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_ASPECT_COLOR_BIT, 1);
    }

	void TextureAsset::CreateTextureImage()
    {
        std::string texturePath = config.textureFolder + "/texture.png";

        SEDX_CORE_INFO("Loading texture shader from: {}", texturePath);

        int texWidth, texHeight, texChannels;

        stbi_uc *pixels = stbi_load(texturePath.c_str(), &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);
        VkDeviceSize imageSize = texWidth * texHeight * 4;
        renderData.mipLevels = static_cast<uint32_t>(std::floor(std::log2(std::max(texWidth, texHeight)))) + 1;

        if (!pixels)
        {
            ErrMsg("Failed to load texture image!");
        }

        VkBuffer stagingBuffer;
        VkDeviceMemory stagingBufferMemory;
        CreateBuffer(imageSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer, stagingBufferMemory);

        void *data;

        vkMapMemory(vkDevice->GetDevice(), stagingBufferMemory, 0, imageSize, 0, &data);
        if (pixels != nullptr)
        {
            memcpy(data, pixels, imageSize);
        }
        else
        {
            ErrMsg("Failed to load texture image: pixels is null");
        }

        vkUnmapMemory(vkDevice->GetDevice(), stagingBufferMemory);

        stbi_image_free(pixels);

        CreateImage(texWidth, texHeight, renderData.mipLevels, VK_SAMPLE_COUNT_1_BIT, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_TILING_OPTIMAL,
					VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, textureImage, textureImageMemory);

        transitionImageLayout(textureImage, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, renderData.mipLevels);
        copyBufferToImage(stagingBuffer, textureImage,  static_cast<uint32_t>(texWidth), static_cast<uint32_t>(texHeight));
        transitionImageLayout(textureImage, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, renderData.mipLevels);

        vkDestroyBuffer(vkDevice->GetDevice(), stagingBuffer, nullptr);
        vkFreeMemory(vkDevice->GetDevice(), stagingBufferMemory, nullptr);
    }

} // namespace SceneryEditorX

// ---------------------------------------------------------
