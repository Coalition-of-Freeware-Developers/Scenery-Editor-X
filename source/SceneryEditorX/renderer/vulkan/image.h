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
 * image.h
 * -------------------------------------------------------
 * Created: 04/03/2026
 * -------------------------------------------------------
 */
#pragma once
#include <SceneryEditorX/utils/inheritance.h>

// -------------------------------------------------------

namespace SceneryEditorX
{
	class Device;

	/**
	 * @struct ImageSpec
	 * @brief Specifies the properties of an image, including dimensions, format, and usage flags.
	 */
	struct ImageSpec
	{		
		VkImageCreateFlags createFlags = 0;
		ImageType imageType = ImageType::Type2D;
		VkFormat format	= VK_FORMAT_UNDEFINED;
		uint32_t width	= 1;
		uint32_t height	= 1;
		uint32_t mips	= 1;
		uint32_t layers	= 1;
		std::uint32_t mipLevels		= 1;
		std::uint32_t arrayLayers	= 1;
		VkSampleCountFlagBits samples = VK_SAMPLE_COUNT_1_BIT;
		VkImageTiling tiling = VK_IMAGE_TILING_OPTIMAL;
		VkImageUsageFlags usageFlags = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
		VkImageLayout initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
				
		VkMemoryPropertyFlags memProperties;
		const char *name = nullptr;
	};

	/**
	 * @class Image
	 * @brief Represents a Vulkan image resource, encapsulating the VkImage handle and its associated properties. 
	 * Provides functionality for creating and managing images based on specified parameters.
	 */
	class Image : public SharedObject
	{
	public:
		Image() = default;

		/**
		 * @brief Constructor for the Image class, initializes an image resource based on the provided specifications.
		 * @param spec The specifications for the image, including dimensions, format, usage flags, and memory properties.
		 */
		Image(const ImageSpec& spec);

		/* @brief Destructor for the Image class, responsible for cleaning up. */
		virtual ~Image() override;

		/**
		 * @brief Initializes the image with the specified specifications.
		 * @param spec The specifications for the image.
		 */
		static void CreateImage(const ImageSpec &spec, VkImage &outImage, VkDeviceMemory &outMemory);

		/**
		 * @brief Gets the Vulkan image handle associated with this Image instance.
		 * @return VkImage handle of the image resource, or VK_NULL_HANDLE if not initialized.
		 */
		VkImage GetImage() const { return m_Image; }

		/**
		 * @brief Retrieves the specifications of this image resource. 
		 * @param outSpec Reference to an ImageSpec structure that will 
		 * be populated with the specifications of this image. 
		 */
		void GetImageSpec(ImageSpec &outSpec) const { outSpec = m_Spec; }

	private:
		Ref<Device> m_Device;
		ImageSpec m_Spec;

		std::vector<VkImageView> m_ImageViews;
		VkImage m_Image = VK_NULL_HANDLE;
		VkMemoryPropertyFlags m_MemProperties = 0;
	};
}

// -------------------------------------------------------
