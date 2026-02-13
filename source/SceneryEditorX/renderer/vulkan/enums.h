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
 * enums.h
 * -------------------------------------------------------
 * Created: 10/02/2026
 * -------------------------------------------------------
 */
#pragma once
#include <utility>

// -----------------------------------------------------------------

namespace SceneryEditorX
{

    /**
	 * @enum DeviceType
	 * @brief Enumeration of physical device types in Vulkan
	 *
	 * This enum provides a type-safe way to identify and reference
	 * the type of physical m_Device being used in the rendering system.
	 *
	 * @note - The values in this enum are designed to be compatible with Vulkan's VkPhysicalDeviceType.
	 * @see @enum VkPhysicalDeviceType
	 */
	enum class DeviceType
	{
        Other		= 0,
	    Integrated	= 1,
	    Discrete	= 2,
	    External	= 3,
	    Virtual		= 4,
	    MaxEnum		= 255
	};

    /**
	 * @enum QueueType
	 * @brief Enumeration of Vulkan queue family types used in the rendering system
	 *
	 * Vulkan uses different queue families to execute different types of operations.
	 * This enum provides a type-safe way to identify and reference these queue families
	 * throughout the rendering system.
	 */
    enum QueueType : int32_t
    {
        Graphics	= 0,  // Graphics queue family for rendering operations and drawing commands
        Compute		= 1,  // Compute queue family for compute shader and general computation operations
        Transfer	= 2,  // Transfer queue family dedicated to memory transfer operations
        Count		= 3,  // Total number of queue families
        Present		= 4,  // Present queue family for presenting rendered images to the display surface
        Unknown		= static_cast<uint8_t>(-1),
    };

    /**     
	 * @enum PoolFlags
	 * @brief Flags for command pool behavior in Vulkan.
	 *
	 * This enum defines various flags that can be used to configure
	 * the behavior of command pools in Vulkan, such as whether
	 * to use transient command buffers or allow individual resets.
	 */
    enum PoolFlags : uint8_t
    {
        Transient	= 0,  // Use transient command buffers optimized for short-lived usage
        Resettable	= 1,  // Allow command buffers to be reset to the initial state to begin recording again
        Protected	= 2   // Allows command buffer to access protected memory (if supported by m_Device)
    };

    /**
	 * @enum ResourceType
	 * @brief Defines the types of resources used in Vulkan.
	 *
	 * This enum specifies the various types of resources that can be created and managed
	 * in a Vulkan application, such as buffers and textures.
	 *
	 * @note - The values in this enum are designed to be compatible with Vulkan's resource types.
	 */
    enum class ResourceType : uint16_t // NOLINT(performance-enum-size)
    {
        None = 0,
        UniformBuffer,
        UniformBufferSet,
        StorageBuffer,
        StorageBufferSet,
        Texture2D,
        TextureCube,
        Image2D,
        Fence,
        Semaphore,
        Shader,
        Sampler,
        QueryPool,
        DeviceMemory,
        Buffer,
        CommandList,
        CommandPool,
        Image,
        ImageView,
        DescriptorSet,
        DescriptorSetLayout,
        Pipeline,
        PipelineLayout,
        Queue,
        AccelerationStructure,
        Unknown,
        MaxEnum
    };


} // namespace SceneryEditorX

// -----------------------------------------------------------------
