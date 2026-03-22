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
 * image_view.h
 * -------------------------------------------------------
 * Created: 05/03/2026
 * -------------------------------------------------------
 */
#pragma once
#include "image.h"
#include <SceneryEditorX/utils/inheritance.h>

// -------------------------------------------------------

namespace SceneryEditorX
{
	class Device;

	/**
	 * @struct ImageViewSpec
	 * @brief Specifies the properties of a Vulkan image view.
	 */
	struct ImageViewSpec
	{
		Ref<Image> image;
		VkFormat format = VK_FORMAT_UNDEFINED;
		uint32_t mip = 0;
		uint32_t mipCount = 0; // 0 means all mips
		uint32_t layer = 0;
		uint32_t layerCount = 0; // 0 means all layers
		VkImageAspectFlags aspect = VK_IMAGE_ASPECT_NONE;

		const char *name = nullptr;
	};

	/**
	 * @class ImageView
	 * @brief Represents a Vulkan image view, which is a view into an image resource that can be used for rendering or sampling.
	 */
	class ImageView : public SharedObject
	{
	public:
		ImageView(ImageViewSpec spec);
		virtual ~ImageView() override;

		ImageView(const ImageView&) = delete;
		void operator=(const ImageView&)  = delete;

		VkImageView GetImageView() const { return m_ImageView; }

	private:
		Ref<Device> m_Device;
		ImageViewSpec m_Spec;
		VkImageView m_ImageView = VK_NULL_HANDLE;
	};

}

// -------------------------------------------------------
