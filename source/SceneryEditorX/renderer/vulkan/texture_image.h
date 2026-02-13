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

    class TextureImage
    {
    public:
		TextureImage() = default;
		~TextureImage() = default;

		// Load a texture from `path`. On failure the returned Texture will have `image == VK_NULL_HANDLE`.
        static Texture Load(VmaAllocator allocator, VkCommandPool oneTimeCmdPool, VkQueue queue, const std::string& path);

    private:
        Ref<Device> m_Device;
    };

} // namespace SceneryEditorX

// -------------------------------------------------------
