/**
* -------------------------------------------------------
* Scenery Editor X
* -------------------------------------------------------
* Copyright (c) 2025 Thomas Ray
* Copyright (c) 2025 Coalition of Freeware Developers
* -------------------------------------------------------
* texture.cpp
* -------------------------------------------------------
* Created: 8/7/2025
* -------------------------------------------------------
*/
#include <SceneryEditorX/renderer/renderer.h>
#include <SceneryEditorX/renderer/texture.h>
#include <SceneryEditorX/renderer/vulkan/vk_util.h>
#include <utility>

#include <SceneryEditorX/asset/texture_importer.h>

/// -------------------------------------------------------

namespace SceneryEditorX
{

	Ref<Texture2D> Texture2D::Create(const TextureSpecification &specification)
	{
        return CreateRef<Texture2D>(specification);
	}

	Ref<Texture2D> Texture2D::Create(const TextureSpecification &specification, const std::filesystem::path &filepath)
	{
        return CreateRef<Texture2D>(specification, filepath);
	}

	Ref<Texture2D> Texture2D::Create(const TextureSpecification &specification, const Buffer &imagedata)
	{
        return CreateRef<Texture2D>(specification, imagedata);
	}

    Ref<Texture2D> Texture2D::CreateFromSRGB(const Ref<Texture2D> &texture)
    {
        TextureSpecification spec;
        spec.Width = texture->GetWidth();
        spec.Height = texture->GetHeight();
        spec.Format = VK_FORMAT_R8G8B8A8_UNORM;
        Buffer buffer;
        texture->GetImage()->CopyToHostBuffer(buffer);
        auto srgbTexture = Create(spec, buffer);
        return srgbTexture;
    }

    void Texture2D::CreateFromFile(const TextureSpecification &specification, const std::filesystem::path &filepath)
    {
		Utils::ValidateSpecification(specification);

		m_ImageData = TextureImporter::ToBufferFromFile(filepath, m_Specification.Format, m_Specification.Width, m_Specification.Height);
		if (!m_ImageData)
		{
			SEDX_CORE_ERROR("Failed to load texture from file: {}", filepath);
			m_ImageData = TextureImporter::ToBufferFromFile("assets/textures/error_texture.png", m_Specification.Format, m_Specification.Width, m_Specification.Height);
		}

		ImageSpecification imageSpec;
		imageSpec.Format = m_Specification.Format;
		imageSpec.Width = m_Specification.Width;
		imageSpec.Height = m_Specification.Height;
		imageSpec.Mips = specification.GenerateMips ? GetMipLevelCount() : 1;
		imageSpec.DebugName = specification.DebugName;
		imageSpec.CreateSampler = false;
		m_Image = Image2D::Create(imageSpec);

		SEDX_CORE_ASSERT(m_Specification.Format != ImageFormat::None);

		Invalidate();
    }

    void Texture2D::ReplaceFromFile(const TextureSpecification &specification, const std::filesystem::path &filepath)
    {
	    Utils::ValidateSpecification(specification);

		m_ImageData = TextureImporter::ToBufferFromFile(filepath, m_Specification.Format, m_Specification.Width, m_Specification.Height);
		if (!m_ImageData)
		{
			SEDX_CORE_ERROR("Failed to load texture from file: {}", filepath);
			m_ImageData = TextureImporter::ToBufferFromFile("assets/textures/error_texture.png", m_Specification.Format, m_Specification.Width, m_Specification.Height);
		}

		ImageSpecification imageSpec;
		imageSpec.Format = m_Specification.Format;
		imageSpec.Width = m_Specification.Width;
		imageSpec.Height = m_Specification.Height;
		imageSpec.Mips = specification.GenerateMips ? GetMipLevelCount() : 1;
		imageSpec.DebugName = specification.DebugName;
		imageSpec.CreateSampler = false;
		m_Image = Image2D::Create(imageSpec);

		SEDX_CORE_ASSERT(m_Specification.Format != ImageFormat::None);

		Ref<Texture2D> instance(this);
		Renderer::Submit([instance]() mutable
		{
			instance->Invalidate();
		});
    }

    void Texture2D::CreateFromBuffer(const TextureSpecification &specification, Buffer data)
    {
	    if (m_Specification.Height == 0)
		{
			m_ImageData = TextureImporter::ToBufferFromMemory(Buffer(data.data, m_Specification.Width), m_Specification.Format, m_Specification.Width, m_Specification.Height);
			if (!m_ImageData)
			{
				m_ImageData = TextureImporter::ToBufferFromFile("assets/textures/error_texture.png", m_Specification.Format, m_Specification.Width, m_Specification.Height);
			}

			Utils::ValidateSpecification(m_Specification);
		}
		else if (data)
		{
			Utils::ValidateSpecification(m_Specification);
			auto size = (uint32_t)Utils::GetMemorySize(m_Specification.Format, m_Specification.Width, m_Specification.Height);
			m_ImageData = Buffer::Copy(data.data, size);
		}
		else
		{
			Utils::ValidateSpecification(m_Specification);
			auto size = (uint32_t)Utils::GetMemorySize(m_Specification.Format, m_Specification.Width, m_Specification.Height);
			m_ImageData.Allocate(size);
			m_ImageData.ZeroInitialize();
		}

		ImageSpecification imageSpec;
		imageSpec.Format = m_Specification.Format;
		imageSpec.Width = m_Specification.Width;
		imageSpec.Height = m_Specification.Height;
		imageSpec.Mips = specification.GenerateMips ? Texture2D::GetMipLevelCount() : 1;
		imageSpec.DebugName = specification.DebugName;
		imageSpec.CreateSampler = false;
		if (specification.Storage)
			imageSpec.Usage = ImageUsage::Storage;
		m_Image = Image2D::Create(imageSpec);

		Invalidate();
    }

    void Texture2D::Resize(const glm::uvec2 &size)
    {
        Resize(size.x, size.y);
    }

    void Texture2D::Resize(const uint32_t width, const uint32_t height)
    {
        m_Specification.Width = width;
        m_Specification.Height = height;


        Ref<Texture2D> instance(this);
        Renderer::Submit([instance]() mutable { instance->Invalidate(); });
    }

    void Texture2D::Invalidate()
    {
	    const auto device = RenderContext::GetCurrentDevice();
		const auto vulkanDevice = device->GetDevice();

		m_Image->Release();

		uint32_t mipCount = m_Specification.GenerateMips ? GetMipLevelCount() : 1;

		ImageSpecification& imageSpec = m_Image->GetSpecification();
		imageSpec.Format = m_Specification.Format;
		imageSpec.Width = m_Specification.Width;
		imageSpec.Height = m_Specification.Height;
		imageSpec.Mips = mipCount;
		imageSpec.CreateSampler = false;
		if (!m_ImageData) /// TODO: better management for this, probably from texture spec
			imageSpec.Usage = ImageUsage::Storage;

		const Ref<Image2D> image = m_Image.As<Image2D>();
		image->Invalidate_RenderThread();

		auto& info = image->GetImageInfo();

		if (m_ImageData)
		{
			SetData(m_ImageData);
		}
		else
		{
			VkCommandBuffer transitionCommandBuffer = device->GetCommandBuffer(true);
			VkImageSubresourceRange subresourceRange = {};
			subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			subresourceRange.layerCount = 1;
			subresourceRange.levelCount = GetMipLevelCount();
			SetImageLayout(transitionCommandBuffer, info.image, VK_IMAGE_LAYOUT_UNDEFINED, image->GetDescriptorInfoVulkan().imageLayout, subresourceRange);
			device->FlushCmdBuffer(transitionCommandBuffer);
		}

		////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		// CREATE TEXTURE SAMPLER (owned by Image)
		////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

		/// Create a texture sampler
		VkSamplerCreateInfo samplerInfo{};
		samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
		samplerInfo.maxAnisotropy = 1.0f;
		samplerInfo.magFilter = Utils::VulkanSamplerFilter(m_Specification.SamplerFilter);
		samplerInfo.minFilter = Utils::VulkanSamplerFilter(m_Specification.SamplerFilter);
		samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
		samplerInfo.addressModeU = Utils::VulkanSamplerWrap(m_Specification.SamplerWrap);
		samplerInfo.addressModeV = Utils::VulkanSamplerWrap(m_Specification.SamplerWrap);
		samplerInfo.addressModeW = Utils::VulkanSamplerWrap(m_Specification.SamplerWrap);
		samplerInfo.mipLodBias = 0.0f;
		samplerInfo.compareOp = VK_COMPARE_OP_NEVER;
		samplerInfo.minLod = 0.0f;
		samplerInfo.maxLod = (float)mipCount;
		/// Enable anisotropic filtering
		// This feature is optional, so we must check if it's supported on the device

		// TODO:
		/*if (vulkanDevice->features.samplerAnisotropy) {
				// Use max. level of anisotropy for this example
				sampler.maxAnisotropy = 1.0f;// vulkanDevice->properties.limits.maxSamplerAnisotropy;
				sampler.anisotropyEnable = VK_TRUE;
		}
		else {
				// The device does not support anisotropic filtering
				sampler.maxAnisotropy = 1.0;
				sampler.anisotropyEnable = VK_FALSE;
		}*/
		samplerInfo.maxAnisotropy = 1.0;
		samplerInfo.anisotropyEnable = VK_FALSE;
		samplerInfo.borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE;

		info.sampler = Vulkan::CreateSampler(samplerInfo);
		image->UpdateDescriptor();

		if (!m_Specification.Storage)
		{
			VkImageViewCreateInfo view{};
			view.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
			view.viewType = VK_IMAGE_VIEW_TYPE_2D;
			view.format = Utils::VulkanImageFormat(m_Specification.Format);
			view.components = {
			    .r = VK_COMPONENT_SWIZZLE_R,
			    .g = VK_COMPONENT_SWIZZLE_G,
			    .b = VK_COMPONENT_SWIZZLE_B,
			    .a = VK_COMPONENT_SWIZZLE_A
			};
			/// The subresource range describes the set of mip levels (and array layers) that can be accessed through this image view
			/// It's possible to create multiple image views for a single image referring to different (and/or overlapping) ranges of the image
			view.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			view.subresourceRange.baseMipLevel = 0;
			view.subresourceRange.baseArrayLayer = 0;
			view.subresourceRange.layerCount = 1;
			view.subresourceRange.levelCount = mipCount;
			view.image = info.image;
			VK_CHECK_RESULT(vkCreateImageView(vulkanDevice, &view, nullptr, &info.view))

			SetDebugUtilsObjectName(vulkanDevice, VK_OBJECT_TYPE_IMAGE_VIEW, std::format("Texture view: {}", m_Specification.DebugName), info.view);

			image->UpdateDescriptor();
		}

		/// TODO: Add option for local storage
		m_ImageData.Release();
		m_ImageData = Buffer();
    }

    void Texture2D::Bind(uint32_t slot) const
    {
    }

    void Texture2D::Lock()
    {
        if (!m_ImageData)
        {
            auto size = (uint32_t)GetMemorySize(m_Specification.Format, m_Specification.Width, m_Specification.Height);
            m_ImageData.Allocate(size);
        }
    }

    void Texture2D::Unlock()
    {
        SetData(m_ImageData);
    }

    Buffer Texture2D::GetWriteableBuffer()
    {
        return m_ImageData;
    }

    const std::filesystem::path & Texture2D::GetPath() const
    {
        return m_Path;
    }

    uint32_t Texture2D::GetMipLevelCount() const
    {
        return Utils::CalculateMipCount(m_Specification.Width, m_Specification.Height);
    }

    std::pair<uint32_t, uint32_t> Texture2D::GetMipSize(uint32_t mip) const
    {
        uint32_t width = m_Specification.Width;
        uint32_t height = m_Specification.Height;
        while (mip != 0)
        {
            width /= 2;
            height /= 2;
            mip--;
        }

        return {width, height};
    }

    void Texture2D::GenerateMips()
    {
	    const auto device = RenderContext::GetCurrentDevice();
		auto vulkanDevice = device->GetDevice();

		const Ref<Image2D> image = m_Image.As<Image2D>();
		const auto& info = image->GetImageInfo();

		const VkCommandBuffer blitCmd = RenderContext::GetCurrentDevice()->GetCommandBuffer(true);

		VkImageMemoryBarrier barrier = {};
		barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
		barrier.image = info.image;
		barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;

		const auto mipLevels = GetMipLevelCount();
		for (uint32_t i = 1; i < mipLevels; i++)
		{
			VkImageBlit imageBlit {};

			/// Source
			imageBlit.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			imageBlit.srcSubresource.layerCount = 1;
			imageBlit.srcSubresource.mipLevel = i - 1;
			imageBlit.srcOffsets[1].x = int32_t(m_Specification.Width >> (i - 1));
			imageBlit.srcOffsets[1].y = int32_t(m_Specification.Height >> (i - 1));
			imageBlit.srcOffsets[1].z = 1;

			/// Destination
			imageBlit.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			imageBlit.dstSubresource.layerCount = 1;
			imageBlit.dstSubresource.mipLevel = i;
			imageBlit.dstOffsets[1].x = int32_t(m_Specification.Width >> i);
			imageBlit.dstOffsets[1].y = int32_t(m_Specification.Height >> i);
			imageBlit.dstOffsets[1].z = 1;

			VkImageSubresourceRange mipSubRange = {};
			mipSubRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			mipSubRange.baseMipLevel = i;
			mipSubRange.levelCount = 1;
			mipSubRange.layerCount = 1;

			// Prepare current mip level as image blit destination
			InsertImageMemoryBarrier(blitCmd, info.image,
											0, VK_ACCESS_TRANSFER_WRITE_BIT,
											VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
											VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT,
											mipSubRange);

			/// Blit from previous level
			vkCmdBlitImage(
				blitCmd,
				info.image,
				VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
				info.image,
				VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
				1,
				&imageBlit,
				Utils::VulkanSamplerFilter(m_Specification.SamplerFilter));

			/// Prepare current mip level as image blit source for next level
			InsertImageMemoryBarrier(blitCmd, info.image,
											VK_ACCESS_TRANSFER_WRITE_BIT, VK_ACCESS_TRANSFER_READ_BIT,
											VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
											VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT,
											mipSubRange);
		}

		/// After the loop, all mip layers are in TRANSFER_SRC layout, so transition all to SHADER_READ
		VkImageSubresourceRange subresourceRange = {};
		subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		subresourceRange.layerCount = 1;
		subresourceRange.levelCount = mipLevels;

		InsertImageMemoryBarrier(blitCmd, info.image,
										VK_ACCESS_TRANSFER_READ_BIT, VK_ACCESS_SHADER_READ_BIT,
										VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
										VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
										subresourceRange);

		RenderContext::GetCurrentDevice()->FlushCmdBuffer(blitCmd);

#if 0
		VkImageMemoryBarrier barrier = {};
		barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
		barrier.image = m_Image;
		barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;

		VkImageSubresourceRange subresourceRange = {};
		subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		subresourceRange.levelCount = 1;
		subresourceRange.layerCount = 1;
		barrier.subresourceRange = subresourceRange;

		int32_t mipWidth = m_Specification.Width;
		int32_t mipHeight = m_Specification.Height;

		VkCommandBuffer commandBuffer = VulkanContext::GetCurrentDevice()->GetCommandBuffer(true);

		barrier.subresourceRange.baseMipLevel = 0;
		barrier.subresourceRange.baseArrayLayer = 0;
		barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
		barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
		barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
		barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;

		vkCmdPipelineBarrier(commandBuffer,
							 VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0,
							 0, nullptr,
							 0, nullptr,
							 1, &barrier);

		auto mipLevels = GetMipLevelCount();
		for (uint32_t i = 1; i < mipLevels; i++)
		{
			VkImageBlit blit = {};
			blit.srcOffsets[0] = { 0, 0, 0 };
			blit.srcOffsets[1] = { mipWidth, mipHeight, 1 };
			blit.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			blit.srcSubresource.mipLevel = i - 1;
			blit.srcSubresource.baseArrayLayer = 0;
			blit.srcSubresource.layerCount = 1;

			blit.dstOffsets[0] = { 0, 0, 0 };
			blit.dstOffsets[1] = { mipWidth / 2, mipHeight / 2, 1 };
			blit.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			blit.dstSubresource.mipLevel = i;
			blit.dstSubresource.baseArrayLayer = 0;
			blit.dstSubresource.layerCount = 1;

			vkCmdBlitImage(commandBuffer,
						   m_Image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
						   m_Image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
						   1, &blit,
						   VK_FILTER_LINEAR);

			barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
			barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
			barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
			barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
			barrier.subresourceRange.baseMipLevel = i;

			vkCmdPipelineBarrier(commandBuffer,
								 VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0,
								 0, nullptr,
								 0, nullptr,
								 1, &barrier);

			if (mipWidth > 1)
				mipWidth /= 2;
			if (mipHeight > 1)
				mipHeight /= 2;
		}

		// Transition all mips from transfer to shader read
		barrier.subresourceRange.baseMipLevel = 0;
		barrier.subresourceRange.levelCount = mipLevels;
		barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
		barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
		barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

		vkCmdPipelineBarrier(commandBuffer,
							 VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0,
							 0, nullptr,
							 0, nullptr,
							 1, &barrier);

		VulkanContext::GetCurrentDevice()->FlushCmdBuffer(commandBuffer);
#endif
    }

    void Texture2D::CopyToHostBuffer(Buffer &buffer) const
    {
        if (m_Image)
            m_Image.As<Image2D>()->CopyToHostBuffer(buffer);
    }

    void Texture2D::SetData(const Buffer &buffer)
    {
	    const auto device = RenderContext::GetCurrentDevice();
		const Ref<Image2D> image = m_Image.As<Image2D>();
		const auto& info = image->GetImageInfo();

		const VkDeviceSize size = m_ImageData.size;

		VkMemoryAllocateInfo memAllocInfo{};
		memAllocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;

		MemoryAllocator allocator("Texture2D");

		/// Create staging buffer
		VkBufferCreateInfo bufferCreateInfo{};
		bufferCreateInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
		bufferCreateInfo.size = size;
		bufferCreateInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
		bufferCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
		VkBuffer stagingBuffer;
		const VmaAllocation stagingBufferAllocation = allocator.AllocateBuffer(bufferCreateInfo, VMA_MEMORY_USAGE_CPU_TO_GPU, stagingBuffer);

		/// Copy data to staging buffer
		uint8_t* destData = allocator.MapMemory<uint8_t>(stagingBufferAllocation);
		SEDX_CORE_ASSERT(m_ImageData.data);
		memcpy(destData, m_ImageData.data, size);
        MemoryAllocator::UnmapMemory(stagingBufferAllocation);

		const VkCommandBuffer copyCmd = device->GetCommandBuffer(true);

		/// Image memory barriers for the texture image

		/// The sub resource range describes the regions of the image that will be transitioned using the memory barriers below
		VkImageSubresourceRange subresourceRange = {};
		/// Image only contains color data
		subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		/// Start at first mip level
		subresourceRange.baseMipLevel = 0;
		subresourceRange.levelCount = 1;
		subresourceRange.layerCount = 1;

		/// Transition the texture image layout to transfer target, so we can safely copy our buffer data to it.
		VkImageMemoryBarrier imageMemoryBarrier{};
		imageMemoryBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
		imageMemoryBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		imageMemoryBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		imageMemoryBarrier.image = info.image;
		imageMemoryBarrier.subresourceRange = subresourceRange;
		imageMemoryBarrier.srcAccessMask = 0;
		imageMemoryBarrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
		imageMemoryBarrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		imageMemoryBarrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;

		/// Insert a memory dependency at the proper pipeline stages that will execute the image layout transition
		/// Source pipeline stage is host write/read execution (VK_PIPELINE_STAGE_HOST_BIT)
		/// Destination pipeline stage is copy command execution (VK_PIPELINE_STAGE_TRANSFER_BIT)
		vkCmdPipelineBarrier(copyCmd,
			VK_PIPELINE_STAGE_HOST_BIT,
			VK_PIPELINE_STAGE_TRANSFER_BIT,
			0,
			0, nullptr,
			0, nullptr,
			1, &imageMemoryBarrier);

		VkBufferImageCopy bufferCopyRegion = {};
		bufferCopyRegion.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		bufferCopyRegion.imageSubresource.mipLevel = 0;
		bufferCopyRegion.imageSubresource.baseArrayLayer = 0;
		bufferCopyRegion.imageSubresource.layerCount = 1;
		bufferCopyRegion.imageExtent.width = m_Specification.Width;
		bufferCopyRegion.imageExtent.height = m_Specification.Height;
		bufferCopyRegion.imageExtent.depth = 1;
		bufferCopyRegion.bufferOffset = 0;

		/// Copy mip levels from staging buffer
		vkCmdCopyBufferToImage(
			copyCmd,
			stagingBuffer,
			info.image,
			VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
			1,
			&bufferCopyRegion);

#if 0
		/// Once the data has been uploaded we transfer to the texture image to the shader read layout, so it can be sampled from
		imageMemoryBarrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
		imageMemoryBarrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
		imageMemoryBarrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
		imageMemoryBarrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

		/// Insert a memory dependency at the proper pipeline stages that will execute the image layout transition
		/// Source pipeline stage, stage is copy command execution (VK_PIPELINE_STAGE_TRANSFER_BIT)
		/// Destination pipeline stage fragment shader access (VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT)
		vkCmdPipelineBarrier(
			copyCmd,
			VK_PIPELINE_STAGE_TRANSFER_BIT,
			VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
			0,
			0, nullptr,
			0, nullptr,
			1, &imageMemoryBarrier);

#endif
		const uint32_t mipCount = m_Specification.GenerateMips ? GetMipLevelCount() : 1;
		if (mipCount > 1) /// Mips to generate
		{
			InsertImageMemoryBarrier(copyCmd, info.image,
				VK_ACCESS_TRANSFER_WRITE_BIT, VK_ACCESS_TRANSFER_READ_BIT,
				VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
				VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT,
				subresourceRange);
		}
		else
		{
			InsertImageMemoryBarrier(copyCmd, info.image,
				VK_ACCESS_TRANSFER_READ_BIT, VK_ACCESS_SHADER_READ_BIT,
				VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, image->GetDescriptorInfoVulkan().imageLayout,
				VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
				subresourceRange);
		}


		device->FlushCmdBuffer(copyCmd);

		/// Clean up staging resources
		allocator.DestroyBuffer(stagingBuffer, stagingBufferAllocation);

		if (m_Specification.GenerateMips && mipCount > 1)
			GenerateMips();
    }

	//////////////////////////////////////////////////////////////////////////////////
    /// TextureCube
    //////////////////////////////////////////////////////////////////////////////////

	static std::map<VkImage, WeakRef<TextureCube>> s_TextureCubeReferences;

    TextureCube::TextureCube(TextureSpecification specification, const Buffer &data) : m_Specification(std::move(specification))
    {
        if (data)
        {
            const uint32_t size = m_Specification.Width * m_Specification.Height * 4 * 6; // six layers
            m_LocalStorage = Buffer::Copy(data.data, size);
        }

	    Invalidate();
    }

    void TextureCube::Release()
    {
        if (m_Image == nullptr)
            return;

        Renderer::SubmitResourceFree([image = m_Image, allocation = m_MemoryAlloc, texInfo = m_DescriptorImageInfo]() {
            SEDX_CORE_TRACE_TAG("Renderer", "Destroying TextureCube");
            auto vulkanDevice = RenderContext::GetCurrentDevice()->GetDevice();
            vkDestroyImageView(vulkanDevice, texInfo.imageView, nullptr);
            ImageSampler::DestroySampler(texInfo.sampler);

            MemoryAllocator allocator("TextureCube");
            allocator.DestroyImage(image, allocation);
            s_TextureCubeReferences.erase(image);
        });
        m_Image = nullptr;
        m_MemoryAlloc = nullptr;
        m_DescriptorImageInfo.imageView = nullptr;
        m_DescriptorImageInfo.sampler = nullptr;
    }

    TextureCube::~TextureCube()
    {
        Release();
    }

    void TextureCube::Invalidate()
	{
		auto device = RenderContext::GetCurrentDevice();
		auto vulkanDevice = device->GetDevice();

		Release();

		VkFormat format = Utils::VulkanImageFormat(m_Specification.Format);
		uint32_t mipCount = GetMipLevelCount();

		VkMemoryAllocateInfo memAllocInfo {};
		memAllocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;

		MemoryAllocator allocator("TextureCube");

		// Create optimal tiled target image on the device
		VkImageCreateInfo imageCreateInfo {};
		imageCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
		imageCreateInfo.imageType = VK_IMAGE_TYPE_2D;
		imageCreateInfo.format = format;
		imageCreateInfo.mipLevels = mipCount;
		imageCreateInfo.arrayLayers = 6;
		imageCreateInfo.samples = VK_SAMPLE_COUNT_1_BIT;
		imageCreateInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
		imageCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
		imageCreateInfo.extent = {
		    .width = m_Specification.Width,
		    .height = m_Specification.Height,
		    .depth = 1
		};
		imageCreateInfo.usage = VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_STORAGE_BIT;
		imageCreateInfo.flags = VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT;
		m_MemoryAlloc = allocator.AllocateImage(imageCreateInfo, VMA_MEMORY_USAGE_GPU_ONLY, m_Image, &m_GPUAllocationSize);
		SetDebugUtilsObjectName(vulkanDevice, VK_OBJECT_TYPE_IMAGE, m_Specification.DebugName, m_Image);
        s_TextureCubeReferences[m_Image] = CreateRef<TextureCube>(this);

		m_DescriptorImageInfo.imageLayout = VK_IMAGE_LAYOUT_GENERAL;

		/// Copy data if present
		if (m_LocalStorage)
		{
			/// Create staging buffer
			VkBufferCreateInfo bufferCreateInfo {};
			bufferCreateInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
			bufferCreateInfo.size = m_LocalStorage.size;
			bufferCreateInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
			bufferCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
			VkBuffer stagingBuffer;
			VmaAllocation stagingBufferAllocation = allocator.AllocateBuffer(bufferCreateInfo, VMA_MEMORY_USAGE_CPU_TO_GPU, stagingBuffer);

			/// Copy data to staging buffer
			uint8_t* destData = allocator.MapMemory<uint8_t>(stagingBufferAllocation);
			memcpy(destData, m_LocalStorage.data, m_LocalStorage.size);
            MemoryAllocator::UnmapMemory(stagingBufferAllocation);

			VkCommandBuffer copyCmd = device->GetCommandBuffer(true);

			/// Image memory barriers for the texture image

			/// The sub resource range describes the regions of the image that will be transitioned using the memory barriers below
			VkImageSubresourceRange subresourceRange = {};
			/// Image only contains color data
			subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			/// Start at first mip level
			subresourceRange.baseMipLevel = 0;
			subresourceRange.levelCount = 1;
			subresourceRange.layerCount = 6;

			/// Transition the texture image layout to transfer target, so we can safely copy our buffer data to it.
			VkImageMemoryBarrier imageMemoryBarrier {};
			imageMemoryBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
			imageMemoryBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
			imageMemoryBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
			imageMemoryBarrier.image = m_Image;
			imageMemoryBarrier.subresourceRange = subresourceRange;
			imageMemoryBarrier.srcAccessMask = 0;
			imageMemoryBarrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
			imageMemoryBarrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
			imageMemoryBarrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;

			/// Insert a memory dependency at the proper pipeline stages that will execute the image layout transition
			/// Source pipeline stage is host write/read execution (VK_PIPELINE_STAGE_HOST_BIT)
			/// Destination pipeline stage is copy command execution (VK_PIPELINE_STAGE_TRANSFER_BIT)
			vkCmdPipelineBarrier(
				copyCmd,
				VK_PIPELINE_STAGE_HOST_BIT,
				VK_PIPELINE_STAGE_TRANSFER_BIT,
				0,
				0, nullptr,
				0, nullptr,
				1, &imageMemoryBarrier);

			VkBufferImageCopy bufferCopyRegion = {};
			bufferCopyRegion.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			bufferCopyRegion.imageSubresource.mipLevel = 0;
			bufferCopyRegion.imageSubresource.baseArrayLayer = 0;
			bufferCopyRegion.imageSubresource.layerCount = 6;
			bufferCopyRegion.imageExtent.width = m_Specification.Width;
			bufferCopyRegion.imageExtent.height = m_Specification.Height;
			bufferCopyRegion.imageExtent.depth = 1;
			bufferCopyRegion.bufferOffset = 0;

			/// Copy mip levels from staging buffer
			vkCmdCopyBufferToImage(copyCmd, stagingBuffer, m_Image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,1, &bufferCopyRegion);
			InsertImageMemoryBarrier(copyCmd, m_Image,
											VK_ACCESS_TRANSFER_WRITE_BIT, VK_ACCESS_TRANSFER_READ_BIT,
											VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
											VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT,
											subresourceRange);

			device->FlushCmdBuffer(copyCmd);
			allocator.DestroyBuffer(stagingBuffer, stagingBufferAllocation);
		}

		VkCommandBuffer layoutCmd = device->GetCommandBuffer(true);

		VkImageSubresourceRange subresourceRange = {};
		subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		subresourceRange.baseMipLevel = 0;
		subresourceRange.levelCount = mipCount;
		subresourceRange.layerCount = 6;

		SetImageLayout(layoutCmd, m_Image, VK_IMAGE_LAYOUT_UNDEFINED, m_DescriptorImageInfo.imageLayout, subresourceRange);

		device->FlushCmdBuffer(layoutCmd);

		/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		/// CREATE TEXTURE SAMPLER
		////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		/// Create a texture sampler
		VkSamplerCreateInfo sampler {};
		sampler.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
		sampler.maxAnisotropy = 1.0f;
		sampler.magFilter = SamplerFilter(m_Specification.SamplerFilter);
		sampler.minFilter = SamplerFilter(m_Specification.SamplerFilter);
		sampler.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
		sampler.addressModeU = SamplerWrap(m_Specification.SamplerWrap);
		sampler.addressModeV = SamplerWrap(m_Specification.SamplerWrap);
		sampler.addressModeW = SamplerWrap(m_Specification.SamplerWrap);
		sampler.mipLodBias = 0.0f;
		sampler.compareOp = VK_COMPARE_OP_NEVER;
		sampler.minLod = 0.0f;
		/// Set max level-of-detail to mip level count of the texture
		sampler.maxLod = (float)mipCount;
		/// Enable anisotropic filtering
		/// This feature is optional, so we must check if it's supported on the device

		sampler.maxAnisotropy = 1.0;
		sampler.anisotropyEnable = VK_FALSE;
		sampler.borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE;
		m_DescriptorImageInfo.sampler = CreateSampler(sampler);

		/// Create image view
		/// Textures are not directly accessed by the shaders and
		/// are abstracted by image views containing additional
		/// information and sub resource ranges
		VkImageViewCreateInfo view {};
		view.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		view.viewType = VK_IMAGE_VIEW_TYPE_CUBE;
		view.format = format;
		view.components = {
		    .r = VK_COMPONENT_SWIZZLE_R,
		    .g = VK_COMPONENT_SWIZZLE_G,
		    .b = VK_COMPONENT_SWIZZLE_B,
		    .a = VK_COMPONENT_SWIZZLE_A
		};
		/// The subresource range describes the set of mip levels (and array layers) that can be accessed through this image view
		/// It's possible to create multiple image views for a single image referring to different (and/or overlapping) ranges of the image
		view.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		view.subresourceRange.baseMipLevel = 0;
		view.subresourceRange.baseArrayLayer = 0;
		view.subresourceRange.layerCount = 6;
		view.subresourceRange.levelCount = mipCount;
		view.image = m_Image;
		VK_CHECK_RESULT(vkCreateImageView(vulkanDevice, &view, nullptr, &m_DescriptorImageInfo.imageView));

		SetDebugUtilsObjectName(vulkanDevice, VK_OBJECT_TYPE_IMAGE_VIEW, std::format("Texture cube view: {}", m_Specification.DebugName), m_DescriptorImageInfo.imageView);
	}

	uint32_t TextureCube::GetMipLevelCount() const
	{
		return RenderData::CalculateMipCount(m_Specification.Width, m_Specification.Height);
	}

	std::pair<uint32_t, uint32_t> TextureCube::GetMipSize(uint32_t mip) const
	{
		uint32_t width = m_Specification.Width;
		uint32_t height = m_Specification.Height;
		while (mip != 0)
		{
			width /= 2;
			height /= 2;
			mip--;
		}

		return { width, height };
	}

	VkImageView TextureCube::CreateImageViewSingleMip(uint32_t mip)
	{
		// TODO: assert to check mip count
		const auto device = RenderContext::GetCurrentDevice();
		auto vulkanDevice = device->GetDevice();

		VkFormat format = Utils::VulkanImageFormat(m_Specification.Format);

		VkImageViewCreateInfo view {};
		view.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		view.viewType = VK_IMAGE_VIEW_TYPE_CUBE;
		view.format = format;
		view.components = {
		    .r = VK_COMPONENT_SWIZZLE_R,
		    .g = VK_COMPONENT_SWIZZLE_G,
		    .b = VK_COMPONENT_SWIZZLE_B,
		    .a = VK_COMPONENT_SWIZZLE_A
		};
		view.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		view.subresourceRange.baseMipLevel = mip;
		view.subresourceRange.baseArrayLayer = 0;
		view.subresourceRange.layerCount = 6;
		view.subresourceRange.levelCount = 1;
		view.image = m_Image;

		const VkImageView result = nullptr;
        VK_CHECK_RESULT(vkCreateImageView(vulkanDevice, &view, nullptr, &m_DescriptorImageInfo.imageView))
		SetDebugUtilsObjectName(vulkanDevice, VK_OBJECT_TYPE_IMAGE_VIEW, std::format("Texture cube mip: {}", mip), result);

		return result;
	}

	void TextureCube::GenerateMips(const bool readonly)
	{
		const auto device = RenderContext::GetCurrentDevice();
		auto vulkanDevice = device->GetDevice();

		VkCommandBuffer blitCmd = RenderContext::GetCurrentDevice()->GetCommandBuffer(true);

		const uint32_t mipLevels = GetMipLevelCount();
		for (uint32_t face = 0; face < 6; face++)
		{
			VkImageSubresourceRange mipSubRange = {};
			mipSubRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			mipSubRange.baseMipLevel = 0;
			mipSubRange.baseArrayLayer = face;
			mipSubRange.levelCount = 1;
			mipSubRange.layerCount = 1;

			/// Prepare current mip level as image blit destination
			InsertImageMemoryBarrier(blitCmd, m_Image,0, VK_ACCESS_TRANSFER_WRITE_BIT,
											VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
											VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT,
											mipSubRange);
		}

		for (uint32_t i = 1; i < mipLevels; i++)
		{
			for (uint32_t face = 0; face < 6; face++)
			{
				VkImageBlit imageBlit {};

				/// Source
				imageBlit.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
				imageBlit.srcSubresource.layerCount = 1;
				imageBlit.srcSubresource.mipLevel = i - 1;
				imageBlit.srcSubresource.baseArrayLayer = face;
				imageBlit.srcOffsets[1].x = int32_t(m_Specification.Width >> (i - 1));
				imageBlit.srcOffsets[1].y = int32_t(m_Specification.Height >> (i - 1));
				imageBlit.srcOffsets[1].z = 1;

				/// Destination
				imageBlit.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
				imageBlit.dstSubresource.layerCount = 1;
				imageBlit.dstSubresource.mipLevel = i;
				imageBlit.dstSubresource.baseArrayLayer = face;
				imageBlit.dstOffsets[1].x = int32_t(m_Specification.Width >> i);
				imageBlit.dstOffsets[1].y = int32_t(m_Specification.Height >> i);
				imageBlit.dstOffsets[1].z = 1;

				VkImageSubresourceRange mipSubRange = {};
				mipSubRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
				mipSubRange.baseMipLevel = i;
				mipSubRange.baseArrayLayer = face;
				mipSubRange.levelCount = 1;
				mipSubRange.layerCount = 1;

				/// Prepare current mip level as image blit destination
				InsertImageMemoryBarrier(blitCmd, m_Image,0, VK_ACCESS_TRANSFER_WRITE_BIT,
												VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
												VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT,
												mipSubRange);

				/// Blit from previous level
				vkCmdBlitImage(blitCmd,
				    m_Image,VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
					m_Image,VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
					1, &imageBlit, VK_FILTER_LINEAR);

				/// Prepare current mip level as image blit source for next level
				InsertImageMemoryBarrier(blitCmd, m_Image,
												VK_ACCESS_TRANSFER_WRITE_BIT, VK_ACCESS_TRANSFER_READ_BIT,
												VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
												VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT,
												mipSubRange);
			}
		}

		/// After the loop, all mip layers are in TRANSFER_SRC layout, so transition all to SHADER_READ
		VkImageSubresourceRange subresourceRange = {};
		subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		subresourceRange.layerCount = 6;
		subresourceRange.levelCount = mipLevels;

		InsertImageMemoryBarrier(blitCmd, m_Image,
										VK_ACCESS_TRANSFER_READ_BIT, VK_ACCESS_SHADER_READ_BIT,
										VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, readonly ? VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL : VK_IMAGE_LAYOUT_GENERAL,
										VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
										subresourceRange);

		RenderContext::GetCurrentDevice()->FlushCmdBuffer(blitCmd);

		m_MipsGenerated = true;
		m_DescriptorImageInfo.imageLayout = readonly ? VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL : VK_IMAGE_LAYOUT_GENERAL;
	}

#if 0
	void TextureCube::CopyToHostBuffer(Buffer& buffer)
	{
		auto device = RenderContext::GetCurrentDevice();
		auto vulkanDevice = device->GetDevice();
		MemoryAllocator allocator("TextureCube");

		// Create staging buffer
		VkBufferCreateInfo bufferCreateInfo{};
		bufferCreateInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
		bufferCreateInfo.size = m_GPUAllocationSize;
		bufferCreateInfo.usage = VK_BUFFER_USAGE_TRANSFER_DST_BIT;
		bufferCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

		VkBuffer stagingBuffer;
		VmaAllocation stagingBufferAllocation = allocator.AllocateBuffer(bufferCreateInfo, VMA_MEMORY_USAGE_GPU_TO_CPU, stagingBuffer);

		VkCommandBuffer copyCmd = device->GetCommandBufferAssetThread(true);

		// Image memory barriers for the texture image
		// The sub resource range describes the regions of the image that will be transitioned using the memory barriers below
		VkImageSubresourceRange subresourceRange = {};
		// Image only contains color data
		subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		// Start at first mip level
		subresourceRange.baseMipLevel = 0;
		subresourceRange.levelCount = GetMipLevelCount();
		subresourceRange.layerCount = 6;

		VkImageMemoryBarrier imageMemoryBarrier{};
		imageMemoryBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
		imageMemoryBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		imageMemoryBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		imageMemoryBarrier.image = m_Image;
		imageMemoryBarrier.subresourceRange = subresourceRange;
		imageMemoryBarrier.srcAccessMask = 0;
		imageMemoryBarrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
		imageMemoryBarrier.oldLayout = m_DescriptorImageInfo.imageLayout;
		imageMemoryBarrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;

		// Insert a memory dependency at the proper pipeline stages that will execute the image layout transition
		// Source pipeline stage is host write/read execution (VK_PIPELINE_STAGE_HOST_BIT)
		// Destination pipeline stage is copy command execution (VK_PIPELINE_STAGE_TRANSFER_BIT)
		vkCmdPipelineBarrier(
			copyCmd,
			VK_PIPELINE_STAGE_HOST_BIT,
			VK_PIPELINE_STAGE_TRANSFER_BIT,
			0,
			0, nullptr,
			0, nullptr,
			1, &imageMemoryBarrier);

		VkBufferImageCopy bufferCopyRegion = {};
		bufferCopyRegion.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		bufferCopyRegion.imageSubresource.mipLevel = 0;
		bufferCopyRegion.imageSubresource.baseArrayLayer = 0;
		bufferCopyRegion.imageSubresource.layerCount = 6;
		bufferCopyRegion.imageExtent.width = m_Specification.Width;
		bufferCopyRegion.imageExtent.height = m_Specification.Height;
		bufferCopyRegion.imageExtent.depth = 1;
		bufferCopyRegion.bufferOffset = 0;

		// Copy mip levels from staging buffer
		vkCmdCopyImageToBuffer(
			copyCmd,
			m_Image,
			VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
			stagingBuffer,
			1,
			&bufferCopyRegion);

		Utils::InsertImageMemoryBarrier(copyCmd, m_Image,
										VK_ACCESS_TRANSFER_WRITE_BIT, 0,
										VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, m_DescriptorImageInfo.imageLayout,
										VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
										subresourceRange);

		device->FlushCommandBufferAssetThread(copyCmd);

		// Copy data from staging buffer
		uint8_t* srcData = allocator.MapMemory<uint8_t>(stagingBufferAllocation);
		buffer.Allocate(m_GPUAllocationSize);
		memcpy(buffer.Data, srcData, m_GPUAllocationSize);
		allocator.UnmapMemory(stagingBufferAllocation);

		allocator.DestroyBuffer(stagingBuffer, stagingBufferAllocation);
	}
#endif

	void TextureCube::CopyToHostBuffer(Buffer& buffer) const
    {
        const auto device = RenderContext::GetCurrentDevice();
        auto vulkanDevice = device->GetDevice();
		MemoryAllocator allocator("TextureCube");

		const uint32_t mipCount = GetMipLevelCount();

        constexpr uint32_t bpp = sizeof(float) * 4;
		uint64_t bufferSize = 0;
		uint32_t w = m_Specification.Width, h = m_Specification.Height;

		for (int i = 0; std::cmp_less(i, mipCount); i++)
		{
            constexpr uint32_t faces = 6;
            bufferSize += w * h * bpp * faces;
			w /= 2;
			h /= 2;
		}

		/// Create staging buffer
		VkBufferCreateInfo bufferCreateInfo{};
		bufferCreateInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
		bufferCreateInfo.size = bufferSize;
		bufferCreateInfo.usage = VK_BUFFER_USAGE_TRANSFER_DST_BIT;
		bufferCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

		VkBuffer stagingBuffer;
		const VmaAllocation stagingBufferAllocation = allocator.AllocateBuffer(bufferCreateInfo, VMA_MEMORY_USAGE_GPU_TO_CPU, stagingBuffer);

		uint32_t mipWidth = m_Specification.Width, mipHeight = m_Specification.Height;

		VkCommandBuffer copyCmd = device->GetCommandBuffer(true);

		VkImageSubresourceRange subresourceRange = {};
		subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		subresourceRange.baseMipLevel = 0;
		subresourceRange.levelCount = mipCount;
		subresourceRange.layerCount = 6;

		InsertImageMemoryBarrier(copyCmd, m_Image,
										VK_ACCESS_TRANSFER_READ_BIT, 0,
										m_DescriptorImageInfo.imageLayout, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
										VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT,
										subresourceRange);

		uint64_t mipDataOffset = 0;
		for (uint32_t mip = 0; mip < mipCount; mip++)
		{
			VkBufferImageCopy bufferCopyRegion = {};
			bufferCopyRegion.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			bufferCopyRegion.imageSubresource.mipLevel = mip;
			bufferCopyRegion.imageSubresource.baseArrayLayer = 0;
			bufferCopyRegion.imageSubresource.layerCount = 6;
			bufferCopyRegion.imageExtent.width = mipWidth;
			bufferCopyRegion.imageExtent.height = mipHeight;
			bufferCopyRegion.imageExtent.depth = 1;
			bufferCopyRegion.bufferOffset = mipDataOffset;

			vkCmdCopyImageToBuffer(copyCmd,
				m_Image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
				stagingBuffer,
				1,
				&bufferCopyRegion);

			const uint64_t mipDataSize = mipWidth * mipHeight * sizeof(float) * 4 * 6;
			mipDataOffset += mipDataSize;
			mipWidth /= 2;
			mipHeight /= 2;
		}

		InsertImageMemoryBarrier(copyCmd, m_Image,
										VK_ACCESS_TRANSFER_READ_BIT, 0,
										VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, m_DescriptorImageInfo.imageLayout,
										VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
										subresourceRange);

		device->FlushCmdBuffer(copyCmd);

		/// Copy data from staging buffer
		const uint8_t* srcData = allocator.MapMemory<uint8_t>(stagingBufferAllocation);
		buffer.Allocate(bufferSize);
		memcpy(buffer.data, srcData, bufferSize);
		MemoryAllocator::UnmapMemory(stagingBufferAllocation);

		allocator.DestroyBuffer(stagingBuffer, stagingBufferAllocation);
	}

	void TextureCube::CopyFromBuffer(const Buffer& buffer, const uint32_t mips) const
    {
		const auto device = RenderContext::GetCurrentDevice();
		auto vulkanDevice = device->GetDevice();
		MemoryAllocator allocator("TextureCube");

		/// Create staging buffer
		VkBufferCreateInfo bufferCreateInfo{};
		bufferCreateInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
		bufferCreateInfo.size = buffer.size;
		bufferCreateInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
		bufferCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

		VkBuffer stagingBuffer;
		const VmaAllocation stagingBufferAllocation = allocator.AllocateBuffer(bufferCreateInfo, VMA_MEMORY_USAGE_CPU_TO_GPU, stagingBuffer);

		// Copy data from staging buffer
		uint8_t* dstData = allocator.MapMemory<uint8_t>(stagingBufferAllocation);
		memcpy(dstData, buffer.data, buffer.size);
        MemoryAllocator::UnmapMemory(stagingBufferAllocation);

		uint32_t mipWidth = m_Specification.Width, mipHeight = m_Specification.Height;

		VkCommandBuffer copyCmd = device->GetCommandBuffer(true);
		uint64_t mipDataOffset = 0;
		for (uint32_t mip = 0; mip < mips; mip++)
		{
			/// Image memory barriers for the texture image
			/// The sub resource range describes the regions of the image that will be transitioned using the memory barriers below
			VkImageSubresourceRange subresourceRange = {};
			/// Image only contains color data
			subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			/// Start at first mip level
			subresourceRange.baseMipLevel = mip;
			subresourceRange.levelCount = 1;
			subresourceRange.layerCount = 6;

			VkImageMemoryBarrier imageMemoryBarrier{};
			imageMemoryBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
			imageMemoryBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
			imageMemoryBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
			imageMemoryBarrier.image = m_Image;
			imageMemoryBarrier.subresourceRange = subresourceRange;
			imageMemoryBarrier.srcAccessMask = 0;
			imageMemoryBarrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
			imageMemoryBarrier.oldLayout = m_DescriptorImageInfo.imageLayout;
			imageMemoryBarrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;

			/// Insert a memory dependency at the proper pipeline stages that will execute the image layout transition
			/// Source pipeline stage is host write/read execution (VK_PIPELINE_STAGE_HOST_BIT)
			/// Destination pipeline stage is copy command execution (VK_PIPELINE_STAGE_TRANSFER_BIT)
			vkCmdPipelineBarrier(copyCmd,
				VK_PIPELINE_STAGE_HOST_BIT,
				VK_PIPELINE_STAGE_TRANSFER_BIT,
				0,
				0, nullptr,
				0, nullptr,
				1, &imageMemoryBarrier);

			VkBufferImageCopy bufferCopyRegion = {};
			bufferCopyRegion.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			bufferCopyRegion.imageSubresource.mipLevel = mip;
			bufferCopyRegion.imageSubresource.baseArrayLayer = 0;
			bufferCopyRegion.imageSubresource.layerCount = 6;
			bufferCopyRegion.imageExtent.width = mipWidth;
			bufferCopyRegion.imageExtent.height = mipHeight;
			bufferCopyRegion.imageExtent.depth = 1;
			bufferCopyRegion.bufferOffset = mipDataOffset;

			vkCmdCopyBufferToImage(
				copyCmd,
				stagingBuffer,
				m_Image,
				VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
				1,
				&bufferCopyRegion);

			const uint64_t mipDataSize = mipWidth * mipHeight * sizeof(float) * 4 * 6;
			mipDataOffset += mipDataSize;

			mipWidth /= 2;
			mipHeight /= 2;

			InsertImageMemoryBarrier(copyCmd, m_Image,
											VK_ACCESS_TRANSFER_WRITE_BIT, 0,
											VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, m_DescriptorImageInfo.imageLayout,
											VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
											subresourceRange);
		}

		device->FlushCmdBuffer(copyCmd);
		allocator.DestroyBuffer(stagingBuffer, stagingBufferAllocation);
	}

}

/// -------------------------------------------------------
