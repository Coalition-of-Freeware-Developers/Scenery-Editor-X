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
 * texture_image.h
 * -------------------------------------------------------
 * Created: 09/02/2026
 * -------------------------------------------------------
 */
#pragma once
#include "device.h"
#include <string>
#include <vma/vk_mem_alloc.h>

// -------------------------------------------------------

// Helper that loads a KTX texture, uploads it via a staging buffer and
// returns a filled `Texture` (defined in `VulkanApp.h`). This encapsulates
// the image, view, sampler and VMA allocation used for the image.
namespace SceneryEditorX
{

	struct Texture
	{
	    VmaAllocation allocation{VK_NULL_HANDLE};
	    VkImage image{VK_NULL_HANDLE};
	    VkImageView view{VK_NULL_HANDLE};
	    VkSampler sampler{VK_NULL_HANDLE};
	};

    class TextureImage : public SharedResource
    {
    public:
        TextureImage(const std::string &filePath);
		TextureImage();
		~TextureImage();

        void SaveToFile(const std::string& filePath) override;
        void LoadFromFile(const std::string& filePath) override;
        bool CanSaveToFile() const;

        uint32_t GetDepth() const				{ return m_Depth; }
        uint32_t GetMipCount() const			{ return m_MipCount; }

        uint32_t GetWidth() const				{ return m_Width; }
        void SetWidth(const uint32_t width)		{ m_Width = width; }

        uint32_t GetHeight() const				{ return m_Height; }
        void SetHeight(const uint32_t height)	{ m_Height = height; }

        uint32_t GetBitsPerChannel() const					{ return m_BitsPerChannel; }
        void SetBitsPerChannel(const uint32_t bits)			{ m_BitsPerChannel = bits; }
        uint32_t GetBytesPerChannel() const					{ return m_BitsPerChannel / 8; }
        uint32_t GetBytesPerPixel() const					{ return (m_BitsPerChannel / 8) * m_ChannelCount; }

        uint32_t GetChannelCount()  const					{ return m_ChannelCount; }
        void SetChannelCount(const uint32_t channelCount)	{ m_ChannelCount = channelCount; }

        // format type
        bool IsDepthFormat() const        { return m_Format == VkFormat::VK_FORMAT_D16_UNORM || m_Format == VkFormat::VK_FORMAT_D32_SFLOAT || m_Format == VkFormat::VK_FORMAT_D32_SFLOAT_S8_UINT; }
        bool IsStencilFormat() const      { return m_Format == VkFormat::VK_FORMAT_D32_SFLOAT_S8_UINT; }
        bool IsDepthStencilFormat() const { return IsDepthFormat() || IsStencilFormat(); }
        bool IsColorFormat() const        { return !IsDepthStencilFormat(); }

		// Load a texture from `path`. On failure the returned Texture will have `image == VK_NULL_HANDLE`.
        static Texture Load(VmaAllocator allocator, VkCommandPool oneTimeCmdPool, VkQueue queue, const std::string& path);

    private:
        Ref<Device> m_Device;
        uint32_t m_Width             = 0;
        uint32_t m_Height            = 0;
        uint32_t m_Depth             = 0; // array length and depth are both m_depth (for simplicity), in case of 3D textures we only have one layer though
        uint32_t m_MipCount			 = 0;
        uint32_t m_BitsPerChannel	 = 0;
        uint32_t m_ChannelCount		 = 0;

        VkFormat m_Format            = VkFormat::VK_FORMAT_UNDEFINED;
        VkFormat m_CompressionFormat = VkFormat::VK_FORMAT_UNDEFINED;
    };

} // namespace SceneryEditorX

// -------------------------------------------------------
