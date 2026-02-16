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
 * texture_image.cpp
 * -------------------------------------------------------
 * Created: 09/02/2026
 * -------------------------------------------------------
 */
#include "texture_image.h"
#include "render_context.h"
#include <ktx.h>
#include <ktxvulkan.h>
#include <volk/volk.h>

// -------------------------------------------------------

namespace SceneryEditorX
{

	Texture TextureImage::Load(VmaAllocator allocator, VkCommandPool oneTimeCmdPool, VkQueue queue, const std::string& path)
    {
		Texture out{};
	
		ktxTexture* ktxTexture = nullptr;
		KTX_error_code ktxres = ktxTexture_CreateFromNamedFile(path.c_str(), KTX_TEXTURE_CREATE_LOAD_IMAGE_DATA_BIT, &ktxTexture);

		Ref<Device> device = RenderContext::Get()->GetDevice();
		SEDX_CORE_ASSERT(ktxres == KTX_SUCCESS && ktxTexture != nullptr, "ktxTexture_CreateFromNamedFile failed for: {}", path);
	
		VkImageCreateInfo texImgCI{};
		texImgCI.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
		texImgCI.imageType = VK_IMAGE_TYPE_2D;
		texImgCI.format = ktxTexture_GetVkFormat(ktxTexture);
		texImgCI.extent.width = ktxTexture->baseWidth;
		texImgCI.extent.height = ktxTexture->baseHeight;
		texImgCI.extent.depth = 1;
		texImgCI.mipLevels = ktxTexture->numLevels;
		texImgCI.arrayLayers = 1;
		texImgCI.samples = VK_SAMPLE_COUNT_1_BIT;
		texImgCI.tiling = VK_IMAGE_TILING_OPTIMAL;
		texImgCI.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
		texImgCI.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	
		VmaAllocationCreateInfo texImageAllocCI{};
		texImageAllocCI.usage = VMA_MEMORY_USAGE_AUTO;
        if (VkResult r = vmaCreateImage(allocator, &texImgCI, &texImageAllocCI, &out.image, &out.allocation, nullptr); r != VK_SUCCESS)
		{
            SEDX_CORE_WARN_TAG("Texture", "vmaCreateImage failed: {}", r);
			ktxTexture_Destroy(ktxTexture);
			return out;
		}
	
		VkImageViewCreateInfo texVewCI{};
		texVewCI.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		texVewCI.image = out.image;
		texVewCI.viewType = VK_IMAGE_VIEW_TYPE_2D;
		texVewCI.format = texImgCI.format;
		texVewCI.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		texVewCI.subresourceRange.levelCount = texImgCI.mipLevels;
		texVewCI.subresourceRange.layerCount = 1;
        if (vkCreateImageView(device->GetLogicalDevice(), &texVewCI, nullptr, &out.view) != VK_SUCCESS)
		{
			SEDX_CORE_WARN_TAG("Texture", "vkCreateImageView failed");
			vmaDestroyImage(allocator, out.image, out.allocation);
			ktxTexture_Destroy(ktxTexture);
			out.image = VK_NULL_HANDLE;
			return out;
		}
	
		// Create staging buffer, copy data, and issue an upload command buffer
		VkBuffer imgSrcBuffer = VK_NULL_HANDLE;
		VmaAllocation imgSrcAllocation = VK_NULL_HANDLE;

		VkBufferCreateInfo imgSrcBufferCI{};
		imgSrcBufferCI.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
		imgSrcBufferCI.size = (VkDeviceSize)ktxTexture->dataSize;
		imgSrcBufferCI.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;

		VmaAllocationCreateInfo imgSrcAllocCI{};
		imgSrcAllocCI.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT | VMA_ALLOCATION_CREATE_MAPPED_BIT;
		imgSrcAllocCI.usage = VMA_MEMORY_USAGE_AUTO;

		if (vmaCreateBuffer(allocator, &imgSrcBufferCI, &imgSrcAllocCI, &imgSrcBuffer, &imgSrcAllocation, nullptr) != VK_SUCCESS)
		{
            SEDX_CORE_WARN_TAG("Texture", "vmaCreateBuffer (staging) failed");
            vkDestroyImageView(device->GetLogicalDevice(), out.view, nullptr);
			vmaDestroyImage(allocator, out.image, out.allocation);
			ktxTexture_Destroy(ktxTexture);
			out.image = VK_NULL_HANDLE;
			return out;
		}
	
		void* imgSrcBufferPtr = nullptr;
		vmaMapMemory(allocator, imgSrcAllocation, &imgSrcBufferPtr);
		memcpy(imgSrcBufferPtr, ktxTexture->pData, ktxTexture->dataSize);
	
		VkFence fenceOneTime = VK_NULL_HANDLE;
		VkFenceCreateInfo fenceOneTimeCI{};
		fenceOneTimeCI.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
		vkCreateFence(device->GetLogicalDevice(), &fenceOneTimeCI, nullptr, &fenceOneTime);
	
		VkCommandBuffer cbOneTime = VK_NULL_HANDLE;
		VkCommandBufferAllocateInfo cbOneTimeAI{};
		cbOneTimeAI.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
		cbOneTimeAI.commandPool = oneTimeCmdPool;
		cbOneTimeAI.commandBufferCount = 1;
        vkAllocateCommandBuffers(device->GetLogicalDevice(), &cbOneTimeAI, &cbOneTime);
	
		VkCommandBufferBeginInfo cbOneTimeBI{};
		cbOneTimeBI.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		cbOneTimeBI.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
		vkBeginCommandBuffer(cbOneTime, &cbOneTimeBI);
	
		VkImageMemoryBarrier2 barrierTexImage{};
		barrierTexImage.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2;
		barrierTexImage.srcStageMask = VK_PIPELINE_STAGE_2_NONE;
		barrierTexImage.srcAccessMask = VK_ACCESS_2_NONE;
		barrierTexImage.dstStageMask = VK_PIPELINE_STAGE_2_TRANSFER_BIT;
		barrierTexImage.dstAccessMask = VK_ACCESS_2_TRANSFER_WRITE_BIT;
		barrierTexImage.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		barrierTexImage.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
		barrierTexImage.image = out.image;
		barrierTexImage.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		barrierTexImage.subresourceRange.levelCount = texImgCI.mipLevels;
		barrierTexImage.subresourceRange.layerCount = 1;

		VkDependencyInfo barrierTexInfo{};
		barrierTexInfo.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO;
		barrierTexInfo.imageMemoryBarrierCount = 1;
		barrierTexInfo.pImageMemoryBarriers = &barrierTexImage;
		vkCmdPipelineBarrier2(cbOneTime, &barrierTexInfo);
	
		std::vector<VkBufferImageCopy> copyRegions;
		for (uint32_t j = 0; j < texImgCI.mipLevels; ++j)
		{
			ktx_size_t mipOffset = 0;
			ktxTexture_GetImageOffset(ktxTexture, j, 0, 0, &mipOffset);
			VkBufferImageCopy copyRegion{};
			copyRegion.bufferOffset = mipOffset;
			copyRegion.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			copyRegion.imageSubresource.mipLevel = j;
			copyRegion.imageSubresource.layerCount = 1;
			copyRegion.imageExtent.width = std::max(1u, texImgCI.extent.width >> j);
			copyRegion.imageExtent.height = std::max(1u, texImgCI.extent.height >> j);
			copyRegion.imageExtent.depth = 1;
			copyRegions.push_back(copyRegion);
		}
	
		vkCmdCopyBufferToImage(cbOneTime, imgSrcBuffer, out.image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, static_cast<uint32_t>(copyRegions.size()), copyRegions.data());
	
		VkImageMemoryBarrier2 barrierTexRead{};
		barrierTexRead.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2;
		barrierTexRead.srcStageMask = VK_PIPELINE_STAGE_TRANSFER_BIT;
		barrierTexRead.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
		barrierTexRead.dstStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
		barrierTexRead.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
		barrierTexRead.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
		barrierTexRead.newLayout = VK_IMAGE_LAYOUT_READ_ONLY_OPTIMAL;
		barrierTexRead.image = out.image;
		barrierTexRead.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		barrierTexRead.subresourceRange.levelCount = texImgCI.mipLevels;
		barrierTexRead.subresourceRange.layerCount = 1;
		barrierTexInfo.pImageMemoryBarriers = &barrierTexRead;
		vkCmdPipelineBarrier2(cbOneTime, &barrierTexInfo);
	
		vkEndCommandBuffer(cbOneTime);
	
		VkSubmitInfo oneTimeSI{};
		oneTimeSI.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
		oneTimeSI.commandBufferCount = 1;
		oneTimeSI.pCommandBuffers = &cbOneTime;
		vkQueueSubmit(queue, 1, &oneTimeSI, fenceOneTime);
		vkWaitForFences(device->GetLogicalDevice(), 1, &fenceOneTime, VK_TRUE, UINT64_MAX);
		vkDestroyFence(device->GetLogicalDevice(), fenceOneTime, nullptr);
	
		vmaUnmapMemory(allocator, imgSrcAllocation);
		vmaDestroyBuffer(allocator, imgSrcBuffer, imgSrcAllocation);
	
		VkSamplerCreateInfo samplerCI{};
		samplerCI.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
		samplerCI.magFilter = VK_FILTER_LINEAR;
		samplerCI.minFilter = VK_FILTER_LINEAR;
		samplerCI.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
		samplerCI.anisotropyEnable = VK_TRUE;
		samplerCI.maxAnisotropy = 8.0f;
		samplerCI.maxLod = (float)texImgCI.mipLevels;
		if (vkCreateSampler(device->GetLogicalDevice(), &samplerCI, nullptr, &out.sampler) != VK_SUCCESS)
		{
			SEDX_CORE_WARN_TAG("Texture", "vkCreateSampler failed");
			vkDestroyImageView(device->GetLogicalDevice(), out.view, nullptr);
			vmaDestroyImage(allocator, out.image, out.allocation);
			ktxTexture_Destroy(ktxTexture);
			out.image = VK_NULL_HANDLE;
			return out;
		}
	
		ktxTexture_Destroy(ktxTexture);
		return out;
	}
}

// -------------------------------------------------------
