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
 * image_resource.h
 * -------------------------------------------------------
 * Created: 05/03/2026
 * -------------------------------------------------------
 */
#pragma once
#include "command_list.h"
#include "device.h"
#include <SceneryEditorX/utils/inheritance.h>

// -------------------------------------------------------

namespace SceneryEditorX
{
	class Device;

	enum ImageResourceFlags : uint32_t
	{
		ShaderViews				= BIT(0),
		UnorderedAccessView		= BIT(1),
		RenderTargetViews		= BIT(2),
		VariableRateShader		= BIT(3),
		DepthStencilAttachment	= BIT(4),
		BlitClear				= BIT(5),
		PerMipViews				= BIT(6),
		Greyscale				= BIT(7),
		Transparent				= BIT(8),
		SRGB					= BIT(9),
		Mappable				= BIT(10),
		QueueShare				= BIT(11), // Share between Graphics and Compute queue families (if not set, image will be owned by Graphics queue and Compute queue accesses will require ownership transfers)
		Compress				= BIT(12),
	};

	struct ImgResourceSpec
	{
		ImageType type				= ImageType::MaxEnum;
		uint32_t width				= 0;
		uint32_t height				= 0;
		uint32_t depth				= 0;
		uint32_t mipCount			= 0;
		VkFormat format				= VK_FORMAT_UNDEFINED;
		uint32_t flags				= 0;
		const char* name			= nullptr;
	};

	class ImageResource : public SharedResource
	{
	public:
		ImageResource(const ImgResourceSpec &spec);
		ImageResource();
		void SetLayout(Layout::ImageLayout newLayout, CommandList *cmdList, uint32_t mipIndex, uint32_t mipRange);
		virtual ~ImageResource() override;

		uint32_t GetWidth() const           { return m_Spec.width; }
		void SetWidth(const uint32_t width) { m_Spec.width = width; }

		uint32_t GetHeight() const            { return m_Spec.height; }
		void SetHeight(const uint32_t height) { m_Spec.height = height; }

		uint32_t GetArrayLength() const { return (m_Spec.type == ImageType::Type3D) ? 1 : m_Depth; }
		ImgResourceSpec GetImageSpec() const { return m_Spec; }

		/**
		 * @brief Stores the raw byte data for a single mip level.
		 */
		struct MipBytes
		{
			std::vector<std::byte> bytes;
		};

		/**
		 * @brief Stores all mip levels for a single array slice or 3D depth slice.
		 */
		struct Slice
		{
			std::vector<MipBytes> mips;
		};

		void AllocateMip(uint32_t index = 0);

		/**
		 * @brief Returns true if the resource was created with per-mip image views.
		 */
		bool HasPerMipViews() const { return (m_Spec.flags & PerMipViews) != 0; }

		/**
		 * @brief Returns a pointer to the internal VkImage handle.
		 * Used by barrier, clear, and layout-tracking operations that accept a raw image pointer.
		 * The pointer is stable for the lifetime of this object.
		 */
		[[nodiscard]] VkImage* Get() { return &m_Image; }
		[[nodiscard]] VkImageView GetImageView(uint32_t mip = 0) const
		{
			if (m_ImageViews.empty())
				return VK_NULL_HANDLE;

			const size_t idx = std::min<size_t>(mip, m_ImageViews.size() - 1);
			return m_ImageViews[idx];
		}

		// Format type
		bool IsDepthFormat() const;
		bool IsStencilFormat() const      { return m_Spec.format == VK_FORMAT_D32_SFLOAT_S8_UINT; }
		bool IsDepthStencilFormat() const { return IsDepthFormat() || IsStencilFormat(); }
		bool IsColorFormat() const        { return !IsDepthStencilFormat(); }

		static bool IsCompressedFormat(VkFormat format);
		static size_t CalculateMipSize(uint32_t width, uint32_t height, uint32_t depth, VkFormat format, uint32_t bitsPerChannel, uint32_t channelCount);

	private:
		Ref<Device> m_Device;
		ImgResourceSpec m_Spec;

		uint32_t m_Depth            = 0; // array length and depth are both m_depth (for simplicity), in case of 3D textures we only have one layer though
		uint32_t m_MipCount			= 0;
		uint32_t m_BitsPerChannel	= 0;
		uint32_t m_ChannelCount		= 0;
		VkImage m_Image = VK_NULL_HANDLE;
		std::vector<VkImageView> m_ImageViews;
		std::vector<Slice> m_slices;
		VmaAllocation m_Allocation = nullptr;
		VkDeviceMemory m_DeviceMemory = nullptr;
		VkFormat m_CompressionFormat = VK_FORMAT_UNDEFINED;
	};
}

// -------------------------------------------------------
