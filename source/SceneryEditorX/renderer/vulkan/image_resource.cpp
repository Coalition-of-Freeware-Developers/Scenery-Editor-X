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
 * image_resource.cpp
 * -------------------------------------------------------
 * Created: 05/03/2026
 * -------------------------------------------------------
 */
#include "image_resource.h"

#include "command_list.h"

// -------------------------------------------------------

namespace SceneryEditorX
{

	ImageResource::ImageResource(const ImgResourceSpec &spec) : SharedResource(ResourceType::Image), m_Spec(spec)
	{
	}
	
	ImageResource::ImageResource() : SharedResource(ResourceType::Image)
	{
	}

    void ImageResource::SetLayout(const Layout::ImageLayout newLayout, CommandList* cmdList, uint32_t mipIndex /*= all_mips*/, uint32_t mipRange /*= 0*/)
    {
        const bool mip_specified = mipIndex != ALL_MIPS;
        mipIndex                = mip_specified ? mipIndex : 0;
        mipRange                = mip_specified ? mipRange : m_MipCount;
    
        if (mip_specified)
        {
            SEDX_CORE_ASSERT(HasPerMipViews());
            SEDX_CORE_ASSERT(mipRange != 0);
            SEDX_CORE_ASSERT(mipIndex + mipRange <= m_MipCount);
        }

        cmdList->InsertBarrier(m_Image, m_Spec.format, mipIndex, mipRange, GetArrayLength(), newLayout);
    }

    void ImageResource::AllocateMip(uint32_t index)
    {
	    // ensure slices exist up to the requested index
        while (m_slices.size() <= index)
        { 
            m_slices.emplace_back();
        }

        MipBytes& mip = m_slices[index].mips.emplace_back();
        m_Depth              = static_cast<uint32_t>(m_slices.size());
        m_MipCount          = static_cast<uint32_t>(m_slices[index].mips.size());
        uint32_t mip_index   = static_cast<uint32_t>(m_slices[index].mips.size()) - 1;
        uint32_t width       = xMath::Max(1u, m_Spec.width >> mip_index);
        uint32_t height      = xMath::Max(1u, m_Spec.height >> mip_index);
        uint32_t depth       = (m_Spec.type == ImageType::Type3D) ? (m_Depth >> mip_index) : 1;
        size_t size_bytes    = CalculateMipSize(width, height, depth, m_Spec.format, m_BitsPerChannel, m_ChannelCount);
        mip.bytes.resize(size_bytes);
    }

    bool ImageResource::IsDepthFormat() const
    {
        return m_Spec.format == VK_FORMAT_D16_UNORM || m_Spec.format == VK_FORMAT_D32_SFLOAT ||
               m_Spec.format == VK_FORMAT_D32_SFLOAT_S8_UINT;
    }

    bool ImageResource::IsCompressedFormat(const VkFormat format)
    {
        return format == VK_FORMAT_BC1_RGBA_UNORM_BLOCK || format == VK_FORMAT_BC3_UNORM_BLOCK ||
               format == VK_FORMAT_BC5_UNORM_BLOCK || format == VK_FORMAT_BC7_UNORM_BLOCK || format == VK_FORMAT_ASTC_4x4_UNORM_BLOCK;
    }

    size_t ImageResource::CalculateMipSize(uint32_t width, uint32_t height, uint32_t depth, VkFormat format, uint32_t bitsPerChannel, uint32_t channelCount)
    {
        SEDX_CORE_ASSERT(width  > 0);
        SEDX_CORE_ASSERT(height > 0);
        SEDX_CORE_ASSERT(depth  > 0);

        if (IsCompressedFormat(format))
        {
            uint32_t blockSize;
            uint32_t blockWidth  = 4; // default block width  for BC formats
            uint32_t blockHeight = 4; // default block height for BC formats
            switch (format)
            {
            case VK_FORMAT_BC1_RGBA_UNORM_BLOCK:
                blockSize = 8;
                break;
            case VK_FORMAT_BC3_UNORM_BLOCK:
            case VK_FORMAT_BC5_UNORM_BLOCK:
            case VK_FORMAT_BC7_UNORM_BLOCK:
                blockSize = 16;
                break;
            case VK_FORMAT_ASTC_4x4_UNORM_BLOCK:
                blockWidth  = 4;
                blockHeight = 4;
                blockSize  = 16;
                break;
            default:
                SEDX_CORE_ASSERT(false);
                return 0;
            }
            uint32_t numBlocksWide = (width + blockWidth - 1) / blockWidth;
            uint32_t numBlocksHigh = (height + blockHeight - 1) / blockHeight;
            return static_cast<size_t>(numBlocksWide) * static_cast<size_t>(numBlocksHigh) * static_cast<size_t>(depth) * static_cast<size_t>(blockSize);
        }

        SEDX_CORE_ASSERT(channelCount > 0);
        SEDX_CORE_ASSERT(bitsPerChannel > 0);
        return static_cast<size_t>(width) * static_cast<size_t>(height) * static_cast<size_t>(depth) *
               static_cast<size_t>(channelCount) * static_cast<size_t>(bitsPerChannel / 8);
    }

} // namespace SceneryEditorX

// -------------------------------------------------------
