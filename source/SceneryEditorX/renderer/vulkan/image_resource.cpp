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
#include "render_context.h"

// -------------------------------------------------------

namespace SceneryEditorX
{

	/**
	 * @brief Resolve Vulkan aspect mask from image format.
	 */
	static VkImageAspectFlags GetAspectMask(const VkFormat format)
	{
		switch (format)
		{
		case VK_FORMAT_D16_UNORM:
		case VK_FORMAT_D32_SFLOAT:
		case VK_FORMAT_X8_D24_UNORM_PACK32:
			return VK_IMAGE_ASPECT_DEPTH_BIT;
		case VK_FORMAT_D16_UNORM_S8_UINT:
		case VK_FORMAT_D24_UNORM_S8_UINT:
		case VK_FORMAT_D32_SFLOAT_S8_UINT:
			return VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT;
		case VK_FORMAT_S8_UINT:
			return VK_IMAGE_ASPECT_STENCIL_BIT;
		default:
			return VK_IMAGE_ASPECT_COLOR_BIT;
		}
	}

	/**
	 * @brief Perform bilinear downsampling on the input image data to produce a smaller output image.
	 * @param input The input image data.
	 * @param output The output image data.
	 * @param width The width of the input image.
	 * @param height The height of the input image.
	 */
	static void DownsampleBilinear(const std::vector<std::byte>& input, std::vector<std::byte>& output, uint32_t width, uint32_t height)
	{
		constexpr uint32_t channels = 4; // RGBA32 - engine standard
  
		// calculate new dimensions (halving both width and height)
		uint32_t newWidth  = width  >> 1;
		uint32_t newHeight = height >> 1;
		
		// ensure minimum size
		if (newWidth < 1) newWidth = 1;
		if (newHeight < 1) newHeight = 1;
		
		 // perform bilinear downsampling
		for (uint32_t y = 0; y < newHeight; y++)
		{
			for (uint32_t x = 0; x < newWidth; x++)
			{
				// calculate base indices for this 2x2 block
				uint32_t srcIdx              = (y * 2 * width + x * 2) * channels;
				uint32_t srcIdx_Right        = srcIdx + channels;                     // right pixel
				uint32_t srcIdx_Bottom       = srcIdx + (width * channels);           // bottom pixel
				uint32_t srcIdx_BottomRight = srcIdx + (width * channels) + channels; // bottom-right pixel
				uint32_t dstIdx              = (y * newWidth + x) * channels;

				// process all 4 channels (RGBA)
				for (uint32_t c = 0; c < channels; c++)
				{
					uint32_t sum = std::to_integer<uint32_t>(input[srcIdx + c]);
					uint32_t count = 1;
		
					// right pixel
					if (x * 2 + 1 < width)
					{
						sum += std::to_integer<uint32_t>(input[srcIdx_Right + c]);
						count++;
					}
		
					// bottom pixel
					if (y * 2 + 1 < height)
					{
						sum += std::to_integer<uint32_t>(input[srcIdx_Bottom + c]);
						count++;
					}
		
					// bottom-right pixel
					if ((x * 2 + 1 < width) && (y * 2 + 1 < height))
					{
						sum += std::to_integer<uint32_t>(input[srcIdx_BottomRight + c]);
						count++;
					}
		
					// assign the averaged result to the output
					output[dstIdx + c] = static_cast<std::byte>(sum / count);
				}
			}
		}
	}

	/**
	 * @brief Compute the number of mip levels for a given image size.
	 * @param width The width of the image.
	 * @param height The height of the image.
	 * @return The number of mip levels.
	 */
	static uint32_t ComputeCount(uint32_t width, uint32_t height)
	{
		uint32_t mipCount = 1; // base level counts
		while (width > 1 || height > 1)
		{
			width  = xMath::Max(1u, width >> 1);
			height = xMath::Max(1u, height >> 1);
			mipCount++;
		}
		return mipCount;
	}

	ImageResource::ImageResource(const ImgResourceSpec &spec) : SharedResource(ResourceType::Image), m_Spec(spec)
	{
		m_Device = RenderContext::Get()->GetDevice();
		SEDX_CORE_ASSERT(m_Device.IsValid(), "ImageResource requires a valid Device");

		if (m_Spec.name)
			m_ObjectName = m_Spec.name;

		m_Depth = xMath::Max(1u, m_Spec.depth);
		m_Spec.mipCount = xMath::Max(1u, m_Spec.mipCount);

		VkImageUsageFlags usage = 0;
		if ((m_Spec.flags & ShaderViews) != 0)
			usage |= VK_IMAGE_USAGE_SAMPLED_BIT;

		if ((m_Spec.flags & UnorderedAccessView) != 0)
			usage |= VK_IMAGE_USAGE_STORAGE_BIT;

		if ((m_Spec.flags & RenderTargetViews) != 0)
			usage |=
				IsDepthFormat() ? VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT : VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

		if ((m_Spec.flags & BlitClear) != 0)
			usage |= VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;

		// Temporarily avoid adding fragment shading-rate attachment usage here.
		// Device feature probing may report support while extension wiring is still incomplete,
		// which triggers validation errors at vkCreateImage.
		if (usage == 0)
			usage = VK_IMAGE_USAGE_SAMPLED_BIT;

		VkImageCreateInfo imageCI = {};
		imageCI.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
		imageCI.imageType = (m_Spec.type == ImageType::Type3D) ? VK_IMAGE_TYPE_3D : VK_IMAGE_TYPE_2D;
		imageCI.format = m_Spec.format;
		imageCI.extent.width = m_Spec.width;
		imageCI.extent.height = m_Spec.height;
		imageCI.extent.depth = (m_Spec.type == ImageType::Type3D) ? m_Depth : 1u;
		imageCI.mipLevels = m_Spec.mipCount;
		imageCI.arrayLayers = (m_Spec.type == ImageType::Type3D) ? 1u : m_Depth;
		imageCI.samples = VK_SAMPLE_COUNT_1_BIT;
		imageCI.tiling = VK_IMAGE_TILING_OPTIMAL;
		imageCI.usage = usage;
		imageCI.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

		uint32_t queueFamilies[2] = {};
		if ((m_Spec.flags & QueueShare) != 0)
		{
			auto queueManager = m_Device->GetQueueManager();
			const uint32_t graphicsFamily = queueManager->GetFamilyIndexByType(QueueType::Graphics);
			const uint32_t computeFamily = queueManager->GetFamilyIndexByType(QueueType::Compute);
			if (graphicsFamily != computeFamily)
			{
				queueFamilies[0] = graphicsFamily;
				queueFamilies[1] = computeFamily;
				imageCI.sharingMode = VK_SHARING_MODE_CONCURRENT;
				imageCI.queueFamilyIndexCount = 2;
				imageCI.pQueueFamilyIndices = queueFamilies;
			}
			else
			{
				imageCI.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
			}
		}
		else
		{
			imageCI.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
		}

		m_Allocation = m_Device->GetMemoryAllocator().AllocateImage(imageCI, VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE, m_Image, nullptr);
		SEDX_CORE_ASSERT(m_Allocation != nullptr && m_Image != VK_NULL_HANDLE, "Failed to allocate VkImage for ImageResource");

		VkImageViewCreateInfo viewCI{};
		viewCI.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		viewCI.image = m_Image;
		viewCI.format = m_Spec.format;
		viewCI.viewType = (m_Spec.type == ImageType::Type3D) ? VK_IMAGE_VIEW_TYPE_3D : ((m_Depth > 1) ? VK_IMAGE_VIEW_TYPE_2D_ARRAY : VK_IMAGE_VIEW_TYPE_2D);
		viewCI.subresourceRange.aspectMask = GetAspectMask(m_Spec.format);
		viewCI.subresourceRange.baseMipLevel = 0;
		viewCI.subresourceRange.levelCount = m_Spec.mipCount;
		viewCI.subresourceRange.baseArrayLayer = 0;
		viewCI.subresourceRange.layerCount = (m_Spec.type == ImageType::Type3D) ? 1u : m_Depth;

		VkImageView imageView = VK_NULL_HANDLE;
		SEDX_VK_RESULT_ASSERT(vkCreateImageView(m_Device->GetLogicalDevice(), &viewCI, nullptr, &imageView), "Failed to create ImageResource image view");
		m_ImageViews.push_back(imageView);

		if (HasPerMipViews() && m_Spec.mipCount > 1)
		{
			for (uint32_t mip = 0; mip < m_Spec.mipCount; ++mip)
			{
				VkImageViewCreateInfo mipViewCI = viewCI;
				mipViewCI.subresourceRange.baseMipLevel = mip;
				mipViewCI.subresourceRange.levelCount = 1;
				VkImageView mipView = VK_NULL_HANDLE;
				SEDX_VK_RESULT_ASSERT(vkCreateImageView(m_Device->GetLogicalDevice(), &mipViewCI, nullptr, &mipView), "Failed to create per-mip ImageResource view");
				m_ImageViews.push_back(mipView);
			}
		}

		// First-use policy: keep newly-created images in UNDEFINED. We transition at the
		// first real use site (UAV write -> GENERAL, SRV read -> SHADER_READ_ONLY_OPTIMAL,
		// attachment use -> ATTACHMENT/DEPTH_ATTACHMENT). This avoids speculative startup
		// transitions and keeps layout tracking aligned with actual usage.

		m_ResourceState = ResourceState::PreparedForGpu;
	}

	ImageResource::ImageResource(const ImgResourceSpec &spec, std::vector<Slice> data) : InheritanceBundle<RefCounted, IResource>(ResourceType::Image), m_Spec(spec), m_Slices(std::move(data))
	{
		m_Device = RenderContext::Get()->GetDevice();
		SEDX_CORE_ASSERT(m_Device.IsValid(), "ImageResource requires a valid Device");

		if (m_Spec.name)
			m_ObjectName = m_Spec.name;

		m_Depth = xMath::Max(1u, m_Spec.depth);
		m_Spec.mipCount = xMath::Max(1u, m_Spec.mipCount);
		m_Slices = data;
		VkImageUsageFlags usage = 0;
		if ((m_Spec.flags & ShaderViews) != 0)
		{
			usage |= VK_IMAGE_USAGE_SAMPLED_BIT;
		}

		if ((m_Spec.flags & UnorderedAccessView) != 0)
		{
			usage |= VK_IMAGE_USAGE_STORAGE_BIT;
		}

		if ((m_Spec.flags & RenderTargetViews) != 0)
		{
			usage |= IsDepthFormat() ? VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT : VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
		}

		if ((m_Spec.flags & BlitClear) != 0)
		{
			usage |= VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
		}

		// Temporarily avoid adding fragment shading-rate attachment usage here.
		// Device feature probing may report support while extension wiring is still incomplete,
		// which triggers validation errors at vkCreateImage.
		if (usage == 0)
			usage = VK_IMAGE_USAGE_SAMPLED_BIT;

		VkImageCreateInfo imageCI{};
		imageCI.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
		imageCI.imageType = (m_Spec.type == ImageType::Type3D) ? VK_IMAGE_TYPE_3D : VK_IMAGE_TYPE_2D;
		imageCI.format = m_Spec.format;
		imageCI.extent.width = m_Spec.width;
		imageCI.extent.height = m_Spec.height;
		imageCI.extent.depth = (m_Spec.type == ImageType::Type3D) ? m_Depth : 1u;
		imageCI.mipLevels = m_Spec.mipCount;
		imageCI.arrayLayers = (m_Spec.type == ImageType::Type3D) ? 1u : m_Depth;
		imageCI.samples = VK_SAMPLE_COUNT_1_BIT;
		imageCI.tiling = VK_IMAGE_TILING_OPTIMAL;
		imageCI.usage = usage;
		imageCI.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

		uint32_t queueFamilies[2] = {};
		if ((m_Spec.flags & QueueShare) != 0)
		{
			auto queueManager = m_Device->GetQueueManager();
			const uint32_t graphicsFamily = queueManager->GetFamilyIndexByType(QueueType::Graphics);
			const uint32_t computeFamily = queueManager->GetFamilyIndexByType(QueueType::Compute);
			if (graphicsFamily != computeFamily)
			{
				queueFamilies[0] = graphicsFamily;
				queueFamilies[1] = computeFamily;
				imageCI.sharingMode = VK_SHARING_MODE_CONCURRENT;
				imageCI.queueFamilyIndexCount = 2;
				imageCI.pQueueFamilyIndices = queueFamilies;
			}
			else
			{
				imageCI.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
			}
		}
		else
		{
			imageCI.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
		}

		m_Allocation = m_Device->GetMemoryAllocator().AllocateImage(imageCI, VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE, m_Image, nullptr);
		SEDX_CORE_ASSERT(m_Allocation != nullptr && m_Image != VK_NULL_HANDLE, "Failed to allocate VkImage for ImageResource");

		VkImageViewCreateInfo viewCI{};
		viewCI.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		viewCI.image = m_Image;
		viewCI.format = m_Spec.format;
		viewCI.viewType = (m_Spec.type == ImageType::Type3D) ? VK_IMAGE_VIEW_TYPE_3D : ((m_Depth > 1) ? VK_IMAGE_VIEW_TYPE_2D_ARRAY : VK_IMAGE_VIEW_TYPE_2D);
		viewCI.subresourceRange.aspectMask = GetAspectMask(m_Spec.format);
		viewCI.subresourceRange.baseMipLevel = 0;
		viewCI.subresourceRange.levelCount = m_Spec.mipCount;
		viewCI.subresourceRange.baseArrayLayer = 0;
		viewCI.subresourceRange.layerCount = (m_Spec.type == ImageType::Type3D) ? 1u : m_Depth;

		VkImageView imageView = VK_NULL_HANDLE;
		SEDX_VK_RESULT_ASSERT(vkCreateImageView(m_Device->GetLogicalDevice(), &viewCI, nullptr, &imageView),
							  "Failed to create ImageResource image view");
		m_ImageViews.push_back(imageView);

		if (HasPerMipViews() && m_Spec.mipCount > 1)
		{
			for (uint32_t mip = 0; mip < m_Spec.mipCount; ++mip)
			{
				VkImageViewCreateInfo mipViewCI = viewCI;
				mipViewCI.subresourceRange.baseMipLevel = mip;
				mipViewCI.subresourceRange.levelCount = 1;
				VkImageView mipView = VK_NULL_HANDLE;
				SEDX_VK_RESULT_ASSERT(vkCreateImageView(m_Device->GetLogicalDevice(), &mipViewCI, nullptr, &mipView),
									  "Failed to create per-mip ImageResource view");
				m_ImageViews.push_back(mipView);
			}
		}

		// First-use policy: keep newly-created images in UNDEFINED. We transition at the
		// first real use site (UAV write -> GENERAL, SRV read -> SHADER_READ_ONLY_OPTIMAL,
		// attachment use -> ATTACHMENT/DEPTH_ATTACHMENT). This avoids speculative startup
		// transitions and keeps layout tracking aligned with actual usage.

		m_ResourceState = ResourceState::PreparedForGpu;
	}

	ImageResource::ImageResource() : SharedResource(ResourceType::Image)
	{
	}

	void ImageResource::SetLayout(const Layout::ImageLayout newLayout, CommandList *cmdList, uint32_t mipIndex /*= all_mips*/, uint32_t mipRange /*= 0*/)
	{
		// Treat (mipIndex=0, mipRange=0) as "all mips" for compatibility with
		// existing call sites that use 0/0 to mean full-range transitions.
		const bool fullRangeRequested = (mipIndex == 0 && mipRange == 0);
		const bool mip_specified = (mipIndex != ALL_MIPS) && !fullRangeRequested;
		mipIndex = mip_specified ? mipIndex : 0;
		mipRange = mip_specified ? mipRange : m_Spec.mipCount;

		if (mip_specified && mipRange == 0)
			mipRange = 1;

		if (m_Spec.mipCount == 0)
			m_Spec.mipCount = 1;

		if (mip_specified)
		{
			SEDX_CORE_ASSERT(HasPerMipViews());
			SEDX_CORE_ASSERT(mipIndex + mipRange <= m_Spec.mipCount);
		}

		// Defensive: callers sometimes pass a command list that is not in the
		// `Recording` state (or nullptr). Instead of crashing inside the driver
		// we perform the transition via an immediate command list when the
		// provided command list isn't usable. This preserves correctness while
		// we continue to root-cause why non-recording lists are used.
		if (cmdList == nullptr || cmdList->GetState() != CommandState::Recording)
		{
			// Use a short-lived immediate command list on the graphics queue so
			// this transition is applied safely on the GPU.
			if (CommandList *temp = CommandList::BeginImmediateExecution(QueueType::Graphics))
			{
				temp->InsertBarrier(m_Image, m_Spec.format, mipIndex, mipRange, GetArrayLength(), newLayout);
				CommandList::EndImmediateExecution(temp);
			}
			else
			{
				SEDX_CORE_WARN_TAG("VULKAN", "Failed to acquire temporary command list for image layout transition on '{}'", m_ObjectName.c_str());
			}
			return;
		}

		cmdList->InsertBarrier(m_Image, m_Spec.format, mipIndex, mipRange, GetArrayLength(), newLayout);
	}

	Layout::ImageLayout ImageResource::GetLayout(const uint32_t mip) const
	{
		return m_Image ? CommandList::GetImageLayout(m_Image, mip) : Layout::ImageLayout::MaxEnum;
	}

	std::array<Layout::ImageLayout, MAX_MIP_COUNT> ImageResource::GetLayouts()
	{
		std::array<Layout::ImageLayout, MAX_MIP_COUNT> layouts;
		for (uint32_t i = 0; i < MAX_MIP_COUNT; i++)
		{
			layouts[i] = GetLayout(i);
		}

		return layouts;
	}

	ImageResource::~ImageResource()
	{
		if (!m_Device.IsValid())
			return;

		VkDevice device = m_Device->GetLogicalDevice();
		for (VkImageView view : m_ImageViews)
		{
			if (view != VK_NULL_HANDLE)
				QueueManager::AddDeletionQueue(ResourceType::ImageView, view);
		}

		m_ImageViews.clear();

		if (m_Image != VK_NULL_HANDLE)
		{
			// Enqueue image + its allocation so the deletion path has the allocation handle
			QueueManager::AddDeletionQueue(ResourceType::Image, m_Image, m_Allocation);
			//m_Device->GetMemoryAllocator().DestroyImage(m_Image, m_Allocation);
			m_Image = VK_NULL_HANDLE;
			m_Allocation = nullptr;
		}
	}

	void ImageResource::AllocateMip(uint32_t index)
	{
		// ensure slices exist up to the requested index
		while (m_Slices.size() <= index)
		{
			m_Slices.emplace_back();
		}

		MipBytes &mip = m_Slices[index].mips.emplace_back();
		m_Depth = static_cast<uint32_t>(m_Slices.size());
		m_Spec.mipCount = static_cast<uint32_t>(m_Slices[index].mips.size());
		uint32_t mipIndex = static_cast<uint32_t>(m_Slices[index].mips.size()) - 1;
		uint32_t width = xMath::Max(1u, m_Spec.width >> mipIndex);
		uint32_t height = xMath::Max(1u, m_Spec.height >> mipIndex);
		uint32_t depth = (m_Spec.type == ImageType::Type3D) ? (m_Depth >> mipIndex) : 1;
		size_t sizeBytes = CalculateMipSize(width, height, depth, m_Spec.format, m_BitsPerChannel, m_ChannelCount);
		mip.bytes.resize(sizeBytes);
	}

	bool ImageResource::IsDepthFormat() const
	{
		return m_Spec.format == VK_FORMAT_D16_UNORM || m_Spec.format == VK_FORMAT_D32_SFLOAT ||
			   m_Spec.format == VK_FORMAT_D32_SFLOAT_S8_UINT;
	}

	bool ImageResource::IsCompressedFormat(const VkFormat format)
	{
		return format == VK_FORMAT_BC1_RGBA_UNORM_BLOCK || format == VK_FORMAT_BC3_UNORM_BLOCK ||
			   format == VK_FORMAT_BC5_UNORM_BLOCK || format == VK_FORMAT_BC7_UNORM_BLOCK ||
			   format == VK_FORMAT_ASTC_4x4_UNORM_BLOCK;
	}

	size_t ImageResource::CalculateMipSize(uint32_t width, uint32_t height, uint32_t depth, VkFormat format, uint32_t bitsPerChannel, uint32_t channelCount)
	{
		SEDX_CORE_ASSERT(width > 0);
		SEDX_CORE_ASSERT(height > 0);
		SEDX_CORE_ASSERT(depth > 0);

		if (IsCompressedFormat(format))
		{
			uint32_t blockSize;
			uint32_t blockWidth = 4;  // default block width  for BC formats
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
				blockWidth = 4;
				blockHeight = 4;
				blockSize = 16;
				break;
			default:
				SEDX_CORE_ASSERT(false);
				return 0;
			}
			uint32_t numBlocksWide = (width + blockWidth - 1) / blockWidth;
			uint32_t numBlocksHigh = (height + blockHeight - 1) / blockHeight;
			return static_cast<size_t>(numBlocksWide) * static_cast<size_t>(numBlocksHigh) *
				   static_cast<size_t>(depth) * static_cast<size_t>(blockSize);
		}

		SEDX_CORE_ASSERT(channelCount > 0);
		SEDX_CORE_ASSERT(bitsPerChannel > 0);
		return static_cast<size_t>(width) * static_cast<size_t>(height) * static_cast<size_t>(depth) *
			   static_cast<size_t>(channelCount) * static_cast<size_t>(bitsPerChannel / 8);
	}

	MipBytes* ImageResource::GetMip(const uint32_t arrayIndex, const uint32_t mipIndex)
	{
		if (arrayIndex >= m_Slices.size())
			return nullptr;

		if (mipIndex >= m_Slices[arrayIndex].mips.size())
			return nullptr;

		return &m_Slices[arrayIndex].mips[mipIndex];
	}

	Slice* ImageResource::GetSlice(const uint32_t arrayIndex)
	{
		if (arrayIndex >= m_Slices.size())
			return nullptr;

		return &m_Slices[arrayIndex];
	}

	void ImageResource::PrepareForGpu()
	{
		// atomically transition from idle to preparing so only one thread can enter
		ResourceState expected = ResourceState::MaxEnum;
		if (!m_ResourceState.compare_exchange_strong(expected, ResourceState::PreparingForGpu))
			return;

		// skip textures with invalid dimensions (failed to load)
		if (m_Spec.width == 0 || m_Spec.height == 0)
		{
			SEDX_CORE_ERROR_TAG("ImageResource","Texture '{}' has invalid dimensions (%dx%d), skipping preparation", m_Spec.name ? m_Spec.name : m_ObjectName.c_str(), m_Spec.width, m_Spec.height);
			m_ResourceState = ResourceState::MaxEnum;
			return;
		}

		{
			char marker[128];
			snprintf(marker, sizeof(marker), "texture_prepare_gpu: {}", m_Spec.name ? m_Spec.name : m_ObjectName.c_str());
		}

		bool isNotCompressed   = !IsCompressedFormat(m_Spec.format); // the bistro world loads pre-compressed textures
		bool isMaterialTexture = IsMaterialTexture() && !m_Slices.empty(); // render targets or textures which are written to in compute passes, don't need mip and compression

		if (isNotCompressed && isMaterialTexture)
		{
			// generate mip chain for all slices
			uint32_t mipCount = ComputeCount(m_Spec.width, m_Spec.height);
			for (uint32_t sliceIndex = 0; sliceIndex < static_cast<uint32_t>(m_Slices.size()); sliceIndex++)
			{
				for (uint32_t mipIndex = 1; mipIndex < mipCount; mipIndex++)
				{
					AllocateMip(sliceIndex);

					DownsampleBilinear(m_Slices[sliceIndex].mips[mipIndex - 1].bytes, // larger
						m_Slices[sliceIndex].mips[mipIndex].bytes, // smaller
						xMath::Max(1u, m_Spec.width  >> (mipIndex - 1)), // larger width
						xMath::Max(1u, m_Spec.height >> (mipIndex - 1)) // larger height
					);
				}
			}

			// compress - format is chosen per-texture (bc3 for packed, bc1 for color, bc5 for normal, etc.)
			bool compress       = m_Spec.flags & ImageResourceFlags::Compress;
			bool notCompressed = !IsCompressedFormat(m_Spec.format);
			if (compress && notCompressed)
			{
				VkFormat target = m_CompressionFormat != VK_FORMAT_MAX_ENUM ? m_CompressionFormat : VK_FORMAT_BC3_UNORM_BLOCK;

				// TODO: integrate compression into the mip generation loop with AMD's Compressonator library.
				//compressonator::compress(this);
			}
		}
		
		// upload to gpu
		if (!Device::IsDeviceLost())
		{
			SEDX_CORE_ASSERT(CreateRef<ImageResource>(), "Failed to create image resource");
		}

		ComputeMemoryUsage();

		if (m_Image)
		{
			m_ResourceState = ResourceState::PreparedForGpu;
		}
		else
		{
			m_ResourceState = ResourceState::MaxEnum;
		}

	}

	void ImageResource::ComputeMemoryUsage()
	{
		m_ObjectSize = 0;

		uint32_t arrayLength = (m_Spec.type == ImageType::Type3D) ? 1 : m_Spec.depth;
		for (uint32_t arrayIndex = 0; arrayIndex < arrayLength; arrayIndex++)
		{
			for (uint32_t mipIndex = 0; mipIndex <m_Spec.mipCount; mipIndex++)
			{
				const uint32_t mipWidth  = xMath::Max(1u, m_Spec.width >> mipIndex);
				const uint32_t mipHeight = xMath::Max(1u, m_Spec.height >> mipIndex);
				const uint32_t mipDepth  = (m_Spec.type == ImageType::Type3D) ? xMath::Max(1u, m_Spec.depth >> mipIndex) : 1;
				m_ObjectSize += CalculateMipSize(mipWidth, mipHeight, mipDepth, m_Spec.format, m_BitsPerChannel, m_ChannelCount);
			}
		}
	}

} // namespace SceneryEditorX

// -------------------------------------------------------
