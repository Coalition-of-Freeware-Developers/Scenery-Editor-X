/**
* -------------------------------------------------------
* Scenery Editor X
* -------------------------------------------------------
* Copyright (c) 2025 Thomas Ray
* Copyright (c) 2025 Coalition of Freeware Developers
* -------------------------------------------------------
* vk_buffers.h
* -------------------------------------------------------
* Created: 5/5/2025
* -------------------------------------------------------
*/
#pragma once
#include <SceneryEditorX/vulkan/buffers/buffer_data.h>
#include <SceneryEditorX/vulkan/vk_device.h>
#include <SceneryEditorX/vulkan/vk_swapchain.h>

/// --------------------------------------------

namespace SceneryEditorX
{
    /**
     * @brief Creates a Vulkan buffer with specified parameters
     * 
     * @param size Size of the buffer in bytes
     * @param usage Flags specifying how the buffer will be used (vertex, index, uniform, etc.)
     * @param memory Memory type flags indicating where the buffer should be allocated (GPU, CPU, etc.)
     * @param name Optional debug name for the buffer resource
     * @return Buffer A buffer object containing the Vulkan buffer handle and allocation information
     * 
     * This function creates a Vulkan buffer with the specified size and usage flags,
     * allocating it in the appropriate memory type based on the memory parameter.
     * It abstracts away the details of buffer creation and memory allocation in Vulkan.
     */
    Buffer CreateBuffer(uint32_t size, BufferUsageFlags usage, MemoryFlags memory = MemoryType::GPU, const std::string& name = "");
    
    /**
     * @brief Copies data between two Vulkan buffers
     * 
     * @param srcBuffer Source buffer to copy from
     * @param dstBuffer Destination buffer to copy to
     * @param size Number of bytes to copy
     * 
     * This function performs a buffer-to-buffer copy operation through a command buffer,
     * typically used to transfer data from a staging buffer to a device-local buffer.
     * The operation is completed synchronously by creating, executing and destroying
     * a single-use command buffer.
     */
    void CopyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size);
    
    /**
     * @brief Copies buffer data to an image
     * 
     * @param buffer Source buffer containing image data
     * @param image Destination image to copy data to
     * @param width Width of the image in pixels
     * @param height Height of the image in pixels
     * 
     * This function transfers data from a buffer to an image, typically used when
     * loading texture data. It handles the necessary image layout transitions and
     * submits a copy command to the graphics queue. The image should be in the
     * appropriate layout for a transfer destination operation before calling this function.
     */
    void CopyBufferToImage(VkBuffer buffer, VkImage image, uint32_t width, uint32_t height);

    void *MapBuffer(Buffer &buffer);

} // namespace SceneryEditorX

// ----------------------------------------------------------
