/**
* -------------------------------------------------------
* Scenery Editor X
* -------------------------------------------------------
* Copyright (c) 2025 Thomas Ray
* Copyright (c) 2025 Coalition of Freeware Developers
* -------------------------------------------------------
* texture_importer.cpp
* -------------------------------------------------------
* Created: 11/7/2025
* -------------------------------------------------------
*/
#include <SceneryEditorX/asset/texture_importer.h>
#include <SceneryEditorX/platform/file_manager.hpp>
#include "stb_image.h"

/// -------------------------------------------------------

namespace SceneryEditorX
{
	Buffer TextureImporter::ToBufferFromFile(const std::filesystem::path& path, VkFormat& outFormat, uint32_t& outWidth, uint32_t& outHeight)
	{
        IO::FileStatus fileStatus = IO::FileSystem::TryOpenFileAndWait(path, 100);
		Buffer imageBuffer;
		std::string pathString = path.string();
        bool isSRGB = (outFormat == VkFormat::VK_FORMAT_R8G8B8_SRGB) || (outFormat == VkFormat::VK_FORMAT_R8G8B8A8_SRGB);

		int width, height, channels;
		void* tmp;
		size_t size = 0;

		if (stbi_is_hdr(pathString.c_str()))
		{
			tmp = stbi_loadf(pathString.c_str(), &width, &height, &channels, 4);
			if (tmp)
			{
				size = width * height * 4 * sizeof(float);
                outFormat = VkFormat::VK_FORMAT_R32G32B32A32_SFLOAT;
			}
		}
		else
		{
			//stbi_set_flip_vertically_on_load(1);
			tmp = stbi_load(pathString.c_str(), &width, &height, &channels, 4);
			if (tmp)
			{
				size = width * height * 4;
                outFormat = isSRGB ? VkFormat::VK_FORMAT_R8G8B8A8_SRGB : VkFormat::VK_FORMAT_R8G8B8A8_UINT;
			}
		}

		if (!tmp)
		{
			return {};
		}

		SEDX_CORE_ASSERT(size > 0);
		imageBuffer.data = new byte[size]; /// avoid `malloc+delete[]` mismatch.
		imageBuffer.size = size;
		memcpy(imageBuffer.data, tmp, size);
		stbi_image_free(tmp);

		outWidth = width;
		outHeight = height;
		return imageBuffer;
	}

	Buffer TextureImporter::ToBufferFromMemory(const Buffer &buffer, VkFormat& outFormat, uint32_t& outWidth, uint32_t& outHeight)
	{
		Buffer imageBuffer;

		bool isSRGB = (outFormat == VkFormat::VK_FORMAT_R8G8B8_SRGB) || (outFormat == VkFormat::VK_FORMAT_R8G8B8A8_SRGB);

		int width, height, channels;
		void* tmp;
		size_t size;

		if (stbi_is_hdr_from_memory((const stbi_uc*)buffer.data, (int)buffer.size))
		{
			tmp = (byte*)stbi_loadf_from_memory((const stbi_uc*)buffer.data, (int)buffer.size, &width, &height, &channels, STBI_rgb_alpha);
			size = width * height * 4 * sizeof(float);
            outFormat = VkFormat::VK_FORMAT_R32G32B32A32_SFLOAT;
		}
		else
		{
			/// stbi_set_flip_vertically_on_load(1);
			tmp = stbi_load_from_memory((const stbi_uc*)buffer.data, (int)buffer.size, &width, &height, &channels, STBI_rgb_alpha);
			size = width * height * 4;
            outFormat = isSRGB ? VkFormat::VK_FORMAT_R8G8B8A8_SRGB : VkFormat::VK_FORMAT_R8G8B8A8_UINT;
		}

		imageBuffer.data = new byte[size]; /// avoid `malloc+delete[]` mismatch.
		imageBuffer.size = size;
		memcpy(imageBuffer.data, tmp, size);
		stbi_image_free(tmp);

		if (!imageBuffer.data)
			return {};

		outWidth = width;
		outHeight = height;
		return imageBuffer;
	}
}

/// -------------------------------------------------------
