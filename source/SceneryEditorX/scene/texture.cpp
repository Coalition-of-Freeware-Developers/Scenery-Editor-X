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
#include <SceneryEditorX/logging/logging.hpp>
#include <SceneryEditorX/renderer/vulkan/vk_core.h>
#include <SceneryEditorX/scene/texture.h>
#include <stb_image.h>

/// -------------------------------------------------------

namespace SceneryEditorX
{

	TextureAsset::TextureAsset(const std::string &path)
	{
        texturePath = path;
        textureName = path.substr(path.find_last_of("/\\") + 1);
        TextureAsset::Load(path);
	}
	
	TextureAsset::~TextureAsset()
	{
        TextureAsset::Unload();
	}

    /*
    void TextureAsset::Serialize(Serializer &ser)
    {
        // TODO: Implement serialization
    }
    */

	void TextureAsset::Load(const std::string &path)
	{
        texturePath = path;
        
        /// Get Vulkan resources
        RenderData renderData;
        vkDevice = RenderContext::GetCurrentDevice();
        vkPhysDevice = vkDevice->GetPhysicalDevice();

        if (auto configPtr = config.Lock())
        {
            std::string actualPath = path;
            if (!path.empty() && path[0] != '/' && path[0] != '\\' && (path.size() < 2 || path[1] != ':'))
            {
                /// Path is relative, prepend texture folder
                actualPath = configPtr->textureFolder + "/" + path;
            }
            
            SEDX_CORE_INFO("Loading texture from: {}", actualPath);
            
            /// Load the image
            int texWidth, texHeight, texChannels;

            if (stbi_uc *pixels = stbi_load(actualPath.c_str(), &texWidth, &texHeight, &texChannels, STBI_rgb_alpha))
            {
                width = texWidth;
                height = texHeight;
                channels = texChannels;
                
                /// Store the image data
                data.resize(width * height * 4);
                memcpy(data.data(), pixels, data.size());
                
                /// Create the texture image and related resources
                CreateTextureImage();
                CreateTextureImageView();
                CreateTextureSampler();
                
                stbi_image_free(pixels);
            }
            else
                SEDX_CORE_ERROR("Failed to load texture: {}", actualPath);
        }
        else
            SEDX_CORE_ERROR("Failed to access EditorConfig: config is expired or null");
    }

    void TextureAsset::Unload()
    {
        if (vkDevice)
        {
            if (textureSampler != VK_NULL_HANDLE)
            {
                vkDestroySampler(vkDevice->GetDevice(), textureSampler, nullptr);
                textureSampler = VK_NULL_HANDLE;
            }
            
            if (textureImageView != VK_NULL_HANDLE)
            {
                vkDestroyImageView(vkDevice->GetDevice(), textureImageView, nullptr);
                textureImageView = VK_NULL_HANDLE;
            }
            
            if (textureImage != VK_NULL_HANDLE)
            {
                vkDestroyImage(vkDevice->GetDevice(), textureImage, nullptr);
                textureImage = VK_NULL_HANDLE;
            }
            
            if (textureImageMemory != VK_NULL_HANDLE)
            {
                vkFreeMemory(vkDevice->GetDevice(), textureImageMemory, nullptr);
                textureImageMemory = VK_NULL_HANDLE;
            }
        }
        
        /// Clear data
        data.clear();
        width = 0;
        height = 0;
        channels = 0;
    }

    void TextureAsset::SetName(const std::string &name)
    {
        textureName = name;
    }

    const std::string &TextureAsset::GetPath() const
    {
        return texturePath;
    }

    const std::string &TextureAsset::GetName() const
    {
        return textureName;
    }

    void TextureAsset::LoadWithAllocator()
    {
        // Load texture using the memory allocator
        // This is a placeholder implementation
        // Actual implementation will depend on the specific requirements and libraries used
    }

    void TextureAsset::UnloadWithAllocator()
    {
        // Unload texture using the memory allocator
        // This is a placeholder implementation
        // Actual implementation will depend on the specific requirements and libraries used
    }

    void TextureAsset::CreateTextureSampler()
	{
	    VkPhysicalDeviceProperties properties{};
        vkGetPhysicalDeviceProperties(vkPhysDevice->GetGPUDevices(), &properties);
	
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
            SEDX_CORE_ERROR("Failed to create texture sampler!");
    }

    void TextureAsset::CreateTextureImageView()
    {
        textureImageView = CreateImageView(textureImage, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_ASPECT_COLOR_BIT, 1);
    }

	void TextureAsset::CreateTextureImage()
    {
        if (data.empty())
        {
            SEDX_CORE_ERROR("No texture data to create image from");
            return;
        }
        
        VkDeviceSize imageSize = data.size();
        renderData->mipLevels = static_cast<uint32_t>(std::floor(std::log2(std::max(width, height)))) + 1;
        
        /// Create a staging buffer
        VkBuffer stagingBuffer = nullptr;
        VkDeviceMemory stagingBufferMemory = nullptr;
        
        /// Create staging buffer to transfer data to GPU
        VkBufferCreateInfo bufferInfo{};
        bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        bufferInfo.size = imageSize;
        bufferInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
        bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
        
        if (vkCreateBuffer(vkDevice->GetDevice(), &bufferInfo, nullptr, &stagingBuffer) != VK_SUCCESS)
        {
            SEDX_CORE_ERROR("Failed to create staging buffer for texture");
            return;
        }
        
        /// Get memory requirements
        VkMemoryRequirements memRequirements;
        vkGetBufferMemoryRequirements(vkDevice->GetDevice(), stagingBuffer, &memRequirements);
        
        VkMemoryAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        allocInfo.allocationSize = memRequirements.size;
        allocInfo.memoryTypeIndex = vkDevice->FindMemoryType(
            memRequirements.memoryTypeBits, 
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT
        );
        
        if (vkAllocateMemory(vkDevice->GetDevice(), &allocInfo, nullptr, &stagingBufferMemory) != VK_SUCCESS)
        {
            vkDestroyBuffer(vkDevice->GetDevice(), stagingBuffer, nullptr);
            SEDX_CORE_ERROR("Failed to allocate staging buffer memory for texture");
            return;
        }
        
        vkBindBufferMemory(vkDevice->GetDevice(), stagingBuffer, stagingBufferMemory, 0);
        
        /// Copy data to staging buffer
        void *mappedData;
        vkMapMemory(vkDevice->GetDevice(), stagingBufferMemory, 0, imageSize, 0, &mappedData);
        memcpy(mappedData, data.data(), imageSize);
        vkUnmapMemory(vkDevice->GetDevice(), stagingBufferMemory);
        
        /// Create the texture image
        VkImageCreateInfo imageInfo{};
        imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
        imageInfo.imageType = VK_IMAGE_TYPE_2D;
        imageInfo.extent.width = static_cast<uint32_t>(width);
        imageInfo.extent.height = static_cast<uint32_t>(height);
        imageInfo.extent.depth = 1;
        imageInfo.mipLevels = renderData->mipLevels;
        imageInfo.arrayLayers = 1;
        imageInfo.format = VK_FORMAT_R8G8B8A8_SRGB;
        imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
        imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        imageInfo.usage = VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
        imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
        imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
        
        /// Create the image
        if (vkCreateImage(vkDevice->GetDevice(), &imageInfo, nullptr, &textureImage) != VK_SUCCESS)
        {
            vkDestroyBuffer(vkDevice->GetDevice(), stagingBuffer, nullptr);
            vkFreeMemory(vkDevice->GetDevice(), stagingBufferMemory, nullptr);
            SEDX_CORE_ERROR("Failed to create texture image");
            return;
        }
        
        /// Allocate memory for the image
        vkGetImageMemoryRequirements(vkDevice->GetDevice(), textureImage, &memRequirements);
        
        VkMemoryAllocateInfo imageAllocInfo{};
        imageAllocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        imageAllocInfo.allocationSize = memRequirements.size;
        imageAllocInfo.memoryTypeIndex = vkDevice->FindMemoryType(
            memRequirements.memoryTypeBits,
            VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT
        );
        
        if (vkAllocateMemory(vkDevice->GetDevice(), &imageAllocInfo, nullptr, &textureImageMemory) != VK_SUCCESS)
        {
            vkDestroyImage(vkDevice->GetDevice(), textureImage, nullptr);
            vkDestroyBuffer(vkDevice->GetDevice(), stagingBuffer, nullptr);
            vkFreeMemory(vkDevice->GetDevice(), stagingBufferMemory, nullptr);
            SEDX_CORE_ERROR("Failed to allocate texture image memory");
            return;
        }
        
        vkBindImageMemory(vkDevice->GetDevice(), textureImage, textureImageMemory, 0);
        
        // TODO: Transition image layout, copy buffer to image, and generate mipmaps
        // For now this is a simplified version without proper layout transitions
        
        /// Clean up staging resources
        vkDestroyBuffer(vkDevice->GetDevice(), stagingBuffer, nullptr);
        vkFreeMemory(vkDevice->GetDevice(), stagingBufferMemory, nullptr);
	}
	
	VkImageView TextureAsset::CreateImageView(VkImage image, VkFormat format, VkImageAspectFlagBits aspectFlags, int mipLevels) const
    {
        VkImageViewCreateInfo viewInfo{};
        viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        viewInfo.image = image;
        viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
        viewInfo.format = format;
        viewInfo.subresourceRange.aspectMask = aspectFlags;
        viewInfo.subresourceRange.baseMipLevel = 0;
        viewInfo.subresourceRange.levelCount = mipLevels;
        viewInfo.subresourceRange.baseArrayLayer = 0;
        viewInfo.subresourceRange.layerCount = 1;

        VkImageView imageView;
        if (vkCreateImageView(vkDevice->GetDevice(), &viewInfo, nullptr, &imageView) != VK_SUCCESS)
        {
            SEDX_CORE_ERROR("Failed to create texture image view!");
            return VK_NULL_HANDLE;
        }

        return imageView;
    }

} // namespace SceneryEditorX

/// ---------------------------------------------------------
