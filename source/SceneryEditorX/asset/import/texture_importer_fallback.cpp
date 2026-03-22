#include "texture_importer.h"

#include "SceneryEditorX/logging/asserts.h"
#include "SceneryEditorX/renderer/vulkan/image_resource.h"

#include <stb_image.h>
#include <stb_image_write.h>
#include <math_utils.h>
#include <SceneryEditorX/filesystem/file_manager.hpp>

namespace SceneryEditorX
{
    static float HalfToFloat(uint16_t half)
    {
        uint32_t mant = half & 0x3FFu;
        uint32_t exp  = (half >> 10) & 0x1Fu;
        uint32_t sign = (half >> 15) & 0x1u;

        uint32_t f;
        if (exp == 0)
        {
            f = (sign << 31) | (mant ? ((127 - 15) << 23) | (mant << 13) : 0);
        }
        else if (exp == 0x1F)
        {
            f = (sign << 31) | (0x7F800000) | (mant << 13);
        }
        else
        {
            f = (sign << 31) | ((exp + (127 - 15)) << 23) | (mant << 13);
        }

        return *reinterpret_cast<float*>(&f);
    }

    void TextureImporter::Init() {}

    void TextureImporter::Shutdown() {}

    void TextureImporter::Load(const std::string& filePath, const uint32_t /*sliceIndex*/, ImageResource* texture)
    {
        SEDX_CORE_ASSERT(texture != nullptr);
        if (!IO::FileSystem::Exists(filePath))
        {
            return;
        }

        int width = 0;
        int height = 0;
        int channels = 0;
        stbi_uc* pixels = stbi_load(filePath.c_str(), &width, &height, &channels, STBI_rgb_alpha);
        if (!pixels)
        {
            return;
        }

        texture->SetWidth(static_cast<uint32_t>(width));
        texture->SetHeight(static_cast<uint32_t>(height));
        texture->SetChannelCount(4);
        texture->SetBitsPerChannel(8);
        texture->SetFormat(VkFormat::VK_FORMAT_R8G8B8A8_UNORM);
        texture->AllocateMip();

        MipBytes* mip = texture->GetMip(0, 0);
        if (mip)
        {
            const size_t size = static_cast<size_t>(width) * static_cast<size_t>(height) * 4;
            mip->bytes.resize(size);
            memcpy(mip->bytes.data(), pixels, size);
        }

        stbi_image_free(pixels);
    }

    void TextureImporter::Save(const std::string& filePath, const uint32_t width, const uint32_t height, const uint32_t /*channelCount*/, const uint32_t /*bitsPerChannel*/, void* data)
    {
        if (!data || width == 0 || height == 0)
        {
            return;
        }

        const uint16_t* srcHalf = static_cast<const uint16_t*>(data);
        std::vector<uint8_t> rgba8(static_cast<size_t>(width) * static_cast<size_t>(height) * 4);

        for (uint32_t i = 0; i < width * height; ++i)
        {
            rgba8[i * 4 + 0] = static_cast<uint8_t>(xMath::Min(xMath::Max(HalfToFloat(srcHalf[i * 4 + 0]), 0.0f), 1.0f) * 255.0f);
            rgba8[i * 4 + 1] = static_cast<uint8_t>(xMath::Min(xMath::Max(HalfToFloat(srcHalf[i * 4 + 1]), 0.0f), 1.0f) * 255.0f);
            rgba8[i * 4 + 2] = static_cast<uint8_t>(xMath::Min(xMath::Max(HalfToFloat(srcHalf[i * 4 + 2]), 0.0f), 1.0f) * 255.0f);
            rgba8[i * 4 + 3] = static_cast<uint8_t>(xMath::Min(xMath::Max(HalfToFloat(srcHalf[i * 4 + 3]), 0.0f), 1.0f) * 255.0f);
        }

        stbi_write_png(filePath.c_str(), static_cast<int>(width), static_cast<int>(height), 4, rgba8.data(), static_cast<int>(width * 4));
    }

    void TextureImporter::SaveSdr(const std::string& filePath, const uint32_t width, const uint32_t height, const uint32_t channelCount, const uint32_t bitsPerChannel, void* data, bool /*isHdr*/)
    {
        Save(filePath, width, height, channelCount, bitsPerChannel, data);
    }

    Memory::Buffer TextureImporter::ToBufferFromFile(const std::filesystem::path& path, VkFormat& outFormat, uint32_t& outWidth, uint32_t& outHeight)
    {
        Memory::Buffer imageBuffer;
        std::string pathString = path.string();
        bool isSRGB = (outFormat == VkFormat::VK_FORMAT_R8G8B8_SRGB) || (outFormat == VkFormat::VK_FORMAT_R8G8B8A8_SRGB);

        int width = 0;
        int height = 0;
        int channels = 0;
        void* tmp = nullptr;
        size_t size = 0;

        if (stbi_is_hdr(pathString.c_str()))
        {
            tmp = stbi_loadf(pathString.c_str(), &width, &height, &channels, 4);
            if (tmp)
            {
                size = static_cast<size_t>(width) * static_cast<size_t>(height) * 4 * sizeof(float);
                outFormat = VkFormat::VK_FORMAT_R32G32B32A32_SFLOAT;
            }
        }
        else
        {
            tmp = stbi_load(pathString.c_str(), &width, &height, &channels, 4);
            if (tmp)
            {
                size = static_cast<size_t>(width) * static_cast<size_t>(height) * 4;
                outFormat = isSRGB ? VkFormat::VK_FORMAT_R8G8B8A8_SRGB : VkFormat::VK_FORMAT_R8G8B8A8_UNORM;
            }
        }

        if (!tmp)
        {
            return {};
        }

        imageBuffer.data = new byte[size];
        imageBuffer.size = size;
        memcpy(imageBuffer.data, tmp, size);
        stbi_image_free(tmp);

        outWidth = static_cast<uint32_t>(width);
        outHeight = static_cast<uint32_t>(height);
        return imageBuffer;
    }

    Memory::Buffer TextureImporter::ToBufferFromMemory(Memory::Buffer buffer, VkFormat& outFormat, uint32_t& outWidth, uint32_t& outHeight)
    {
        Memory::Buffer imageBuffer;
        bool isSRGB = (outFormat == VkFormat::VK_FORMAT_R8G8B8_SRGB) || (outFormat == VkFormat::VK_FORMAT_R8G8B8A8_SRGB);

        int width = 0;
        int height = 0;
        int channels = 0;
        void* tmp = nullptr;
        size_t size = 0;

        if (stbi_is_hdr_from_memory(static_cast<const stbi_uc*>(buffer.data), static_cast<int>(buffer.size)))
        {
            tmp = stbi_loadf_from_memory(static_cast<const stbi_uc*>(buffer.data), static_cast<int>(buffer.size), &width, &height, &channels, STBI_rgb_alpha);
            size = static_cast<size_t>(width) * static_cast<size_t>(height) * 4 * sizeof(float);
            outFormat = VkFormat::VK_FORMAT_R32G32B32A32_SFLOAT;
        }
        else
        {
            tmp = stbi_load_from_memory(static_cast<const stbi_uc*>(buffer.data), static_cast<int>(buffer.size), &width, &height, &channels, STBI_rgb_alpha);
            size = static_cast<size_t>(width) * static_cast<size_t>(height) * 4;
            outFormat = isSRGB ? VkFormat::VK_FORMAT_R8G8B8A8_SRGB : VkFormat::VK_FORMAT_R8G8B8A8_UNORM;
        }

        if (!tmp)
        {
            return {};
        }

        imageBuffer.data = new byte[size];
        imageBuffer.size = size;
        memcpy(imageBuffer.data, tmp, size);
        stbi_image_free(tmp);

        outWidth = static_cast<uint32_t>(width);
        outHeight = static_cast<uint32_t>(height);
        return imageBuffer;
    }
}
