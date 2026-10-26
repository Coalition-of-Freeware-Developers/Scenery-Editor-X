/**
 * -------------------------------------------------------
 * Scenery Editor X
 * -------------------------------------------------------
 * Copyright (c) 2026 Thomas Ray 
 * Copyright (c) 2026 Coalition of Freeware Developers
 * -------------------------------------------------------
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 * -------------------------------------------------------
 * texture_importer.h
 * -------------------------------------------------------
 * Created: 19/03/2026
 * -------------------------------------------------------
 */
#pragma once
#include <SceneryEditorX/core/memory/buffer.h>

// -------------------------------------------------------

namespace SceneryEditorX
{
	class ImageResource;

	/**
	 * @class TextureImporter
	 * @brief A class responsible for importing and exporting texture resources.
	 */
	class TextureImporter
	{
	public:

		/**
		 * @brief 
		 */
		static void Init();

		/**
		 * @brief 
		 */
		static void Shutdown();

		/**
		 * @brief 
		 * @param filePath 
		 * @param sliceIndex 
		 * @param texture 
		 */
		static void Load(const std::string& filePath, const uint32_t sliceIndex, ImageResource* texture);

		/**
		 * @brief 
		 * @param filePath 
		 * @param width 
		 * @param height 
		 * @param channelCount 
		 * @param bitsPerChannel 
		 * @param data 
		 */
		static void Save(const std::string& filePath, const uint32_t width, const uint32_t height, const uint32_t channelCount, const uint32_t bitsPerChannel, void* data);

		/**
		 * @brief 
		 * @param filePath 
		 * @param width 
		 * @param height 
		 * @param channelCount 
		 * @param bitsPerChannel 
		 * @param data 
		 * @param isHdr 
		 */
		static void SaveSdr(const std::string& filePath, const uint32_t width, const uint32_t height, const uint32_t channelCount, const uint32_t bitsPerChannel, void* data, bool isHdr);

		/**
		 * @brief 
		 * @param path 
		 * @param outFormat 
		 * @param outWidth 
		 * @param outHeight 
		 * @return 
		 */
		static Memory::Buffer ToBufferFromFile(const std::filesystem::path& path, VkFormat& outFormat, uint32_t& outWidth, uint32_t& outHeight);

		/**
		 * @brief 
		 * @param buffer 
		 * @param outFormat 
		 * @param outWidth 
		 * @param outHeight 
		 * @return 
		 */
		static Memory::Buffer ToBufferFromMemory(Memory::Buffer buffer, VkFormat& outFormat, uint32_t& outWidth, uint32_t& outHeight);

	private:
		const std::filesystem::path m_Path; // for error messages
	};
}

// -------------------------------------------------------
