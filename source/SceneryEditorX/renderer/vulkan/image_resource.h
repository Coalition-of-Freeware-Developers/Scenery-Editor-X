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
#include <vma/vk_mem_alloc.h>
#include <vulkan/vulkan.h>

// -------------------------------------------------------

namespace SceneryEditorX
{
	class Device;

	/**
	 * @enum ImageResourceFlags
	 * @brief Flags that specify the properties and usage of an image resource.
	 */
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

	/**
	 * @struct ImgResourceSpec
	 * @brief Specifies the properties of an image resource, including dimensions, format, and usage flags.
	 */
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
			
	/* 
	 * @struct MipBytes
	 * @brief Stores the raw byte data for a single mip level. 
	 */
	struct MipBytes
	{
		std::vector<std::byte> bytes;
	};

	/* 
	 * @struct Slice
	 * @brief Stores all mip levels for a single array slice or 3D depth slice. 
	 */
	struct Slice
	{
		std::vector<MipBytes> mips;
	};

	/**
	 * @class ImageResource
	 * @brief Represents a Vulkan image resource, encapsulating the VkImage handle, its memory allocation, and associated properties.
	 */
	class ImageResource : public SharedResource
	{
	public:
		/* @brief Default constructor for the ImageResource class. */
		ImageResource();

		/**
		 * @brief Creates an image resource with the specified properties.
		 * @param spec The specification of the image resource.
		 */
		ImageResource(const ImgResourceSpec &spec);

		/**
		 * @brief Creates an image resource with the specified properties.
		 * @param spec The specification of the image resource.
		 * @param data The initial data for the image resource.
		 */
		ImageResource(const ImgResourceSpec &spec, std::vector<Slice> data);

		/**
		 * @brief Destroys the image resource.
		 */
		virtual ~ImageResource() override;

		/**
		 * @brief Returns the width of the image resource.
		 * @return The width of the image resource in pixels.
		 */
		uint32_t GetWidth() const { return m_Spec.width; }

		/**
		 * @brief Sets the width of the image resource.
		 * @param width width of the image resource in pixels.
		 */
		void SetWidth(const uint32_t width) { m_Spec.width = width; }

		/**
		 * @brief Returns the height of the image resource.
		 * @return The height of the image resource in pixels.
		 */
		uint32_t GetHeight() const { return m_Spec.height; }

		/**
		 * @brief Sets the height of the image resource.
		 * @param height height of the image resource in pixels. 
		 * @note For 3D images, this represents the height of each slice. 
		 * @note For 2D array images, this represents the height of each layer.
		 */
		void SetHeight(const uint32_t height) { m_Spec.height = height; }

		/**
		 * @brief Returns the length of the image array.
		 * @return The number of layers in the image array. For 3D images, this is always 1.
		 */
		uint32_t GetArrayLength() const { return (m_Spec.type == ImageType::Type3D) ? 1 : m_Depth; }

		/**
		 * @brief Returns the image resource specification.
		 * @return The image resource specification.
		 */
		ImgResourceSpec GetImageSpec() const { return m_Spec; }

		/**
		 * @brief Returns the flags of the image resource.
		 * @return The flags of the image resource.
		 */
		virtual uint32_t GetFlags() const override { return m_Spec.flags; }

		/**
		 * @brief Sets the flags of the image resource.
		 * @param flags The flags to set for the image resource.
		 */
		virtual void SetFlags(const uint32_t flags) override { m_Spec.flags = flags; IResource::SetFlags(flags); }

		/**
		 * @brief Sets the format of the image resource.
		 * @param format The format to set for the image resource.
		 */
		void SetFormat(const VkFormat format) { m_Spec.format = format; }

		/**
		 * @brief Returns the format of the image resource.
		 * @return The format of the image resource.
		 */
		VkFormat GetFormat() const { return m_Spec.format; }

		/**
		 * @brief Allocates a mip level for the image resource.
		 * @param index The index of the mip level to allocate.
		 */
		void AllocateMip(uint32_t index = 0);

		/**
		 * @brief Returns the number of bits per channel for the image resource.
		 * @return The number of bits per channel.
		 */
		uint32_t GetBitsPerChannel() const { return m_BitsPerChannel; }

		/**
		 * @brief Sets the number of bits per channel for the image resource.
		 * @param bits The number of bits per channel.
		 */
		void SetBitsPerChannel(const uint32_t bits) { m_BitsPerChannel = bits; }

		/**
		 * @brief Returns the number of bytes per channel for the image resource.
		 * @return The number of bytes per channel.
		 */
		uint32_t GetBytesPerChannel() const { return m_BitsPerChannel / 8; }

		/**
		 * @brief Returns the number of bytes per pixel for the image resource.
		 * @return The number of bytes per pixel.
		 */
		uint32_t GetBytesPerPixel() const { return (m_BitsPerChannel / 8) * m_ChannelCount; }

		/**
		 * @brief Returns the number of channels in the image resource.
		 * @return The number of channels.
		 */
		uint32_t GetChannelCount() const { return m_ChannelCount; }

		/**
		 * @brief Sets the number of channels in the image resource.
		 * @param channel_count The number of channels to set.
		 */
		void SetChannelCount(const uint32_t channel_count) { m_ChannelCount = channel_count; }

		/**
		 * @brief Sets the layout of the image resource.
		 * @param newLayout The new layout to set for the image resource.
		 * @param cmdList The command list to use for the layout transition.
		 * @param mipIndex The index of the mip level to transition. Defaults to all mips.
		 * @param mipRange The range of mip levels to transition. Defaults to 0.
		 */
		void SetLayout(Layout::ImageLayout newLayout, CommandList *cmdList, uint32_t mipIndex = ALL_MIPS, uint32_t mipRange = 0);

		/**
		 * @brief Returns the layout of the image resource for a specific mip level.
		 * @param mip The index of the mip level.
		 * @return The layout of the specified mip level.
		 */
		Layout::ImageLayout GetLayout(uint32_t mip) const;

		/**
		 * @brief Returns the layouts of all mip levels for the image resource.
		 * @return An array containing the layouts of all mip levels.
		 */
		std::array<Layout::ImageLayout, MAX_MIP_COUNT> GetLayouts();

		/* @brief Returns true if the resource was created with per-mip image views. */
		bool HasPerMipViews() const { return (m_Spec.flags & PerMipViews) != 0; }

		/**
		 * @brief Returns a pointer to the internal VkImage handle.
		 * Used by barrier, clear, and layout-tracking operations that accept a raw image pointer.
		 * The pointer is stable for the lifetime of this object.
		 */
		[[nodiscard]] VkImage *Get() { return &m_Image; }

		/**
		 * @brief Returns the image view for a specific mip level.
		 * @param mip The index of the mip level.
		 * @return The image view of the specified mip level.
		 */
		[[nodiscard]] VkImageView GetImageView(uint32_t mip = 0) const
		{
			if (m_ImageViews.empty())
				return VK_NULL_HANDLE;

			const size_t idx = std::min<size_t>(mip, m_ImageViews.size() - 1);
			return m_ImageViews[idx];
		}

		/**
		 * @brief Returns true if the image resource has a depth format.
		 * @return True if the image resource has a depth format, false otherwise.
		 */
		bool IsDepthFormat() const;

		/**
		 * @brief Returns true if the image resource has a stencil format.
		 * @return True if the image resource has a stencil format, false otherwise.
		 */
		bool IsShaderResourceView() const { return m_Spec.flags & ShaderViews; }

		/**
		 * @brief Returns true if the image resource has variable rate shader usage.
		 * @return True if the image resource has variable rate shader usage, false otherwise. 
		 */
		bool IsVariableRateShader() const { return m_Spec.flags & VariableRateShader; }

		/**
		 * @brief Returns true if the image resource has an unordered access view.
		 * @return True if the image resource has an unordered access view, false otherwise.
		 */
		bool IsUnorderedAccessView() const { return m_Spec.flags & UnorderedAccessView; }

		/**
		 * @brief Returns true if the image resource is a render target.
		 * @return True if the image resource is a render target, false otherwise.
		 */
		bool IsRenderTarget() const { return m_Spec.flags & RenderTargetViews; }

		/**
		 * @brief Returns true if the image resource has a stencil format.
		 * @return True if the image resource has a stencil format, false otherwise.
		 */
		bool IsStencilFormat() const { return m_Spec.format == VK_FORMAT_D32_SFLOAT_S8_UINT; }

		/**
		 * @brief Returns true if the image resource has a depth-stencil format.
		 * @return True if the image resource has a depth-stencil format, false otherwise.
		 */
		bool IsDepthStencilFormat() const { return IsDepthFormat() || IsStencilFormat(); }

		/**
		 * @brief Returns true if the image resource is a depth-stencil view.
		 * @return True if the image resource is a depth-stencil view, false otherwise.
		 */
		bool IsDepthStencilView() const { return IsRenderTarget() && IsDepthStencilFormat(); }

		/**
		 * @brief Returns true if the image resource is a color format.
		 * @return True if the image resource is a color format, false otherwise.
		 */
		bool IsColorFormat() const { return IsRenderTarget() && !IsDepthStencilFormat(); }

		/**
		 * @brief Returns true if the image resource is semi-transparent (has the Transparent flag set).
		 * @return True if the image resource is semi-transparent, false otherwise.
		 */
		bool IsSemiTransparent() const { return m_Spec.flags & Transparent; }

		/**
		 * @brief Returns true if the specified format is a compressed format.
		 * @param format The format to check.
		 * @return True if the format is compressed, false otherwise.
		 */
		static bool IsCompressedFormat(VkFormat format);

		/**
		 * @brief Returns true if the image resource is a material texture.
		 * @return True if the image resource is a material texture, false otherwise.
		 */
		bool IsMaterialTexture() const
		{
			return m_Spec.format == VkFormat::VK_FORMAT_R8G8B8A8_UNORM &&
				   m_ChannelCount == 4 && m_BitsPerChannel == 8 && !(IsRenderTarget() || IsDepthStencilView());
		}

		/**
		 * @brief Calculates the size of a mip level.
		 * @param width The width of the mip level.
		 * @param height The height of the mip level.
		 * @param depth The depth of the mip level.
		 * @param format The format of the mip level.
		 * @param bitsPerChannel The number of bits per channel.
		 * @param channelCount The number of channels.
		 * @return The size of the mip level in bytes.
		 */
		static size_t CalculateMipSize(uint32_t width, uint32_t height, uint32_t depth, VkFormat format, uint32_t bitsPerChannel, uint32_t channelCount);

		/**
		 * @brief Returns the mip level data for a specific array index and mip level.
		 * @param arrayIndex The index of the array.
		 * @param mipIndex The index of the mip level.
		 * @return A pointer to the mip level data.
		 */
		MipBytes *GetMip(const uint32_t arrayIndex, const uint32_t mipIndex);

		/**
		 * @brief Returns the slice data for a specific array index.
		 * @param arrayIndex The index of the array.
		 * @return A pointer to the slice data.
		 */
		Slice *GetSlice(const uint32_t arrayIndex);

		/**
		 * @brief Returns the number of mip levels for the image resource.
		 * @return The number of mip levels.
		 */
		uint32_t GetMipCount() const { return m_MipCount; }

		/**
		 * @brief Returns the shader resource view for the image resource.
		 * @return The shader resource view.
		 */
		VkImageView GetShaderResourceView() const { return m_ShaderResourceView; }

		/**
		 * @brief Returns the shader resource view for a specific mip level.
		 * @param i The index of the mip level.
		 * @return The shader resource view for the specified mip level.
		 */
		VkImageView GetShaderView_Mip(const uint32_t i) const { return m_ShaderResourceView_mips[i]; }

		/**
		 * @brief Returns the shader resource view for a specific layer.
		 * @param i The index of the layer.
		 * @return The shader resource view for the specified layer.
		 */
		VkImageView GetShaderView_Layer(const uint32_t i) const { return m_ShaderResourceView_Layers[i]; }

		/**
		 * @brief Returns the depth stencil view for a specific index.
		 * @param i The index of the depth stencil view.
		 * @return The depth stencil view for the specified index.
		 */
		VkImageView GetDepthStencilView(const uint32_t i = 0) const { return m_DepthStencilView[i]; }

		/**
		 * @brief Returns the render target view for a specific index.
		 * @param i The index of the render target view.
		 * @return The render target view for the specified index.
		 */
		VkImageView GetRenderTargetView(const uint32_t i = 0) const { return m_RenderTargetView[i]; }

		/**
		 * @brief Returns the render target view for multi-view rendering.
		 * @return The render target view for multi-view rendering.
		 */
		VkImageView GetRenderTargetView_MultiView() const { return m_RenderTargetView_MultiView; }

		/**
		 * @brief Returns the depth stencil view for multi-view rendering.
		 * @return The depth stencil view for multi-view rendering.
		 */
		VkImageView GetDepthStencilView_MultiView() const { return m_DepthStencilView_MultiView; }

		/**
		 * @brief Prepares the image resource for use on the GPU by creating the Vulkan image, 
		 * allocating memory, and creating image views based on the specified properties and usage flags.
		 */
		void PrepareForGpu();

	private:
		/**
		 * @brief Computes the memory usage of the image resource.
		 */
		void ComputeMemoryUsage();

		Ref<Device> m_Device;
		ImgResourceSpec m_Spec;

		uint32_t m_Width            = 0;
		uint32_t m_Height           = 0;
		uint32_t m_Depth            = 0; // array length and depth are both m_depth (for simplicity), in case of 3D textures we only have one layer though
		uint32_t m_MipCount			= 0;
		uint32_t m_BitsPerChannel	= 0;
		uint32_t m_ChannelCount		= 0;
		VkImage m_Image = VK_NULL_HANDLE;
		std::vector<VkImageView> m_ImageViews;
		std::vector<Slice> m_Slices;
		VmaAllocation m_Allocation = nullptr;
		VkDeviceMemory m_DeviceMemory = nullptr;
		VkFormat m_CompressionFormat = VK_FORMAT_UNDEFINED;

		VkImageView m_ShaderResourceView												= nullptr;     // an srv with all mips
		std::array<VkImageView, MAX_MIP_COUNT> m_ShaderResourceView_mips				= { nullptr }; // an srv for each mip
		std::array<VkImageView, MAX_RENDER_TARGET_COUNT> m_ShaderResourceView_Layers	= { nullptr }; // per-layer srvs for array textures
		std::array<VkImageView, MAX_RENDER_TARGET_COUNT> m_RenderTargetView				= { nullptr };
		std::array<VkImageView, MAX_RENDER_TARGET_COUNT> m_DepthStencilView				= { nullptr };
		VkImageView m_RenderTargetView_MultiView										= nullptr;
		VkImageView m_DepthStencilView_MultiView										= nullptr;
	};
}

// -------------------------------------------------------
