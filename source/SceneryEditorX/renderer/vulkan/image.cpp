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
 * image.cpp
 * -------------------------------------------------------
 * Created: 04/03/2026
 * -------------------------------------------------------
 */
#include "image.h"
#include "queue_manager.h"
#include "render_context.h"
#include <volk/volk.h>

// -------------------------------------------------------

namespace SceneryEditorX
{

/**
	 * @brief Converts an ImageType enum to the corresponding VkImageType.
	 * @param type The ImageType enum value.
	 * @return The corresponding VkImageType value.
	 */
	static VkImageType GetImageType(const ImageType type)
	{
		switch (type)
		{
			case ImageType::Type1D:			return VK_IMAGE_TYPE_1D;
			case ImageType::Type2D:			return VK_IMAGE_TYPE_2D;
			case ImageType::Type3D:			return VK_IMAGE_TYPE_3D;
			case ImageType::Type1DArray:
			case ImageType::Type2DArray:
			case ImageType::TypeCube:
			case ImageType::TypeCubeArray:
			case ImageType::MaxEnum:
				SEDX_CORE_ERROR_TAG("Image", "Invalid VkImageType type.");
				break;
		}

		SEDX_CORE_WARN_TAG("Image", "Unknown ImageType specified, defaulting to VK_IMAGE_TYPE_2D.");
		return VK_IMAGE_TYPE_2D; // Default fallback
	}

	/**
	 * @brief Finds a suitable memory type index based on the provided filter and flags.
	 * @param filter The memory type filter.
	 * @param flags The desired memory property flags.
	 * @return The index of a suitable memory type.
	 */
	static uint32_t FindMemoryType(uint32_t filter, VkMemoryPropertyFlags flags)
	{
		const Ref<Device> device = RenderContext::Get()->GetDevice();
		VkPhysicalDeviceMemoryProperties memProperties;
		vkGetPhysicalDeviceMemoryProperties(device->GetPhysicalDevice(), &memProperties);
	
		for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++)
		{
			if ((filter & 1) == 1 && (memProperties.memoryTypes[i].propertyFlags & flags) == flags)
			{
				return i;
			}
		}
	
		SEDX_CORE_ASSERT(false, "Failed to find suitable memory type for Vulkan Image!");
		return 0;
	}

	Image::Image(const ImageSpec &spec) : m_Spec(spec)
	{
		const Ref<Device> device = RenderContext::Get()->GetDevice();
		m_Device = device;
		m_ObjectSize = sizeof(VkImage);
		m_ObjectName = spec.name ? spec.name : "Unnamed Image";

		m_MemProperties = spec.memProperties;
	}

	Image::~Image()
	{
		if (m_Image != VK_NULL_HANDLE)
	    {
	        QueueManager::AddDeletionQueue(ResourceType::Image, m_Image);
	    }
	}

	void Image::CreateImage(const ImageSpec &spec, VkImage &outImage, VkDeviceMemory &outMemory)
	{
		const Ref<Device> device = RenderContext::Get()->GetDevice();
		
		VkImageCreateInfo imageCI;
		imageCI.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
		imageCI.pNext = nullptr;
		imageCI.flags = spec.createFlags;
		imageCI.imageType = GetImageType(spec.imageType);
		imageCI.format = spec.format;
		imageCI.extent.width = spec.width;
		imageCI.extent.height = spec.height;
		imageCI.extent.depth = 1; // Assuming 2D images for simplicity
		imageCI.mipLevels = spec.mipLevels;
		imageCI.arrayLayers = spec.arrayLayers;
		imageCI.samples = spec.samples;
		imageCI.tiling = spec.tiling;
		imageCI.usage = spec.usageFlags;
		imageCI.sharingMode = VK_SHARING_MODE_EXCLUSIVE; // Assuming exclusive mode for simplicity
		imageCI.queueFamilyIndexCount = 0; // Not used in exclusive mode
		imageCI.pQueueFamilyIndices = nullptr; // Not used in exclusive mode
		imageCI.initialLayout = spec.initialLayout;
		
		SEDX_VK_RESULT_ASSERT(vkCreateImage(device->GetLogicalDevice(), &imageCI, nullptr, &outImage), "Failed to create Vulkan Image!");
		
		VkMemoryRequirements memRequirements;
		vkGetImageMemoryRequirements(device->GetLogicalDevice(), outImage, &memRequirements);
		
		VkMemoryAllocateInfo allocInfo;
		allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
		allocInfo.pNext = nullptr;
		allocInfo.allocationSize = memRequirements.size;
		allocInfo.memoryTypeIndex = FindMemoryType(memRequirements.memoryTypeBits, spec.memProperties);

		SEDX_VK_RESULT_ASSERT(vkAllocateMemory(device->GetLogicalDevice(), &allocInfo, nullptr, &outMemory), "Failed to allocate memory for Vulkan Image!");
		SEDX_VK_RESULT_ASSERT(vkBindImageMemory(device->GetLogicalDevice(), outImage, outMemory, 0), "Failed to bind memory to Vulkan Image!");
	}

} // namespace SceneryEditorX

// -------------------------------------------------------
