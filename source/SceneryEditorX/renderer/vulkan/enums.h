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
		MaxEnum		= 255
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
		Unknown = 0,
		PhysicalDevice,
		Device,
		Queue,
		Semaphore,
		CommandBuffer,
		Fence,
		DeviceMemory,
		Buffer,
		Image,
		Event,
		QueryPool,
		ImageView,
		Shader,
		PipelineCache,
		PipelineLayout,
		RenderPass,
		Pipeline,
		DescriptorSetLayout,
		Sampler,
		DescriptorPool,
		DescriptorSet,
		CommandPool,
		DebugCallback,
		//AccelerationStructure,
		Mesh,
		Material,
		Font,
		UniformBuffer,
		UniformBufferSet,
		StorageBuffer,
		StorageBufferSet,
		Texture2D,
		TextureCube,
		Image2D,
		CommandList,
		MaxEnum
	};

	/**
	 * @enum StageType
	 * @brief Enumeration of shader stages in Vulkan
	 *
	 * This enum provides a type-safe way to identify and reference
	 * the different stages of a shader in the rendering system.
	 *
	 * @note - The values in this enum are designed to be compatible with Vulkan's VkShaderStageFlagBits.
	 * @see @enum VkShaderStageFlagBits
	 */
	enum class StageType : uint32_t
	{
		Vertex,
		TessellationControl,
		TessellationEvaluation,
	    Geometry,
		Fragment,
		Compute,
	    Graphics,
		All,
		None,
		MaxEnum
	};

	/**
	 * @enum ShaderInputType
	 * @brief Enumeration of shader input types in Vulkan
	 *
	 * This enum provides a type-safe way to identify and reference
	 * the different types of inputs that a shader can accept in the rendering system.
	 *
	 * @note - The values in this enum are designed to be compatible with Vulkan's shader input types.
	 * @see @enum VkDescriptorType
	 */
	enum class ShaderInputType : uint32_t
	{
		UniformBuffer,
		UniformBufferSet,
		StorageBuffer,
		StorageBufferSet,
		CombinedImageSampler,
		Sampler,
		Texture,
		StorageImage,
		PushConstant,
		MaxEnum
	};

	/**
	 * @enum SyncType
	 * @brief Types of synchronization objects.
	 *
	 * This enum can be used to specify which type of synchronization object is being referred to in various contexts (e.g., when waiting or signaling). 
	 * The FrameSync class manages fences and semaphores, so this enum helps identify them.
	 */
	enum class SyncType : uint8_t
	{
		Fence,
		Semaphore,
		SemaphoreTimeline,
		MaxEnum
	};

	/**
	 * @brief Namespace for image layout definitions in Vulkan.
	 *
	 * This namespace contains an enumeration of possible image layouts
	 * that can be used in Vulkan applications. Image layouts define
	 * how the image data is organized and accessed by the GPU.
	 */
	namespace Layout
	{
		/**
		 * @enum ImageLayout
		 * @brief Enumeration of Vulkan image layouts.
		 */
		enum ImageLayout : uint8_t
		{
			Undefined = 0,
			General,
			ColorAttachment,
			Preinitialized,
			DepthStencilAttachment,
			DepthStencilRead,
			ShaderRead,
			TransferSrc,
			TransferDst,
			DepthReadStencilAttachment,
			DepthAttachmentStencilRead,
			FragmentShadingRate,
			DepthAttachment,
			DepthRead,
			StencilAttachment,
			StencilRead,
			ShadingRateAttachment,
			Read,
			Attachment,
			Present,
			MaxEnum
		};
	}

	/**
	 * @enum PolygonMode
	 * @brief Vulkan polygon fill mode, exposed on RasterizerState objects.
	 */
	enum class PolygonMode : uint8_t
	{
		Solid,
		Wireframe,
		Point,
		MaxEnum
	};

	/**
	 * @enum CullMode
	 * @brief Enumeration of culling modes in Vulkan.
	 * 
	 * This enum provides a type-safe way to specify which faces of a polygon
	 * should be culled (not rendered) in the Vulkan rendering pipeline.
	 */
	enum class CullMode : uint8_t
	{
		None,
		Front,
		Back,
		All,
		MaxEnum
	};

	/**
	 * @enum ImageType
	 * @brief Enumeration of Vulkan image types.
	 *
	 * This enum provides a type-safe way to specify the type of an image in Vulkan.
	 * The image type determines how the image can be used and accessed in the rendering pipeline.
	 */
	enum class ImageType : uint8_t
	{
		Type1D,
		Type1DArray,
		Type2D,
		Type2DArray,
		Type3D,
		TypeCube,
		TypeCubeArray,
		MaxEnum
	};

	/**
	 * @enum BarrierScope
	 * @brief Enumeration of barrier scopes for Vulkan synchronization.
	 * Allows specifying barrier scope instead of conservative auto-deduction
	 */
	enum class BarrierScope : uint8_t
	{
		Auto,     // deduce from layout/usage (default, conservative)
		Graphics, // vertex/fragment/tessellation stages
		Compute,  // compute stage only
		Transfer, // transfer stage only
		Fragment, // fragment stage only
		All       // all commands (most conservative, explicit)
	};

	/**
	 * @enum VertexType
	 * @brief Enumeration of vertex types for input layouts in Vulkan. 
	 */
	enum class VertexType : uint8_t
	{
		Position,
		PositionColor,
		PositionUv,
		PositionUvNormalTangent,
		Position2dUvColor8,
		MaxEnum
	};

}

// -----------------------------------------------------------------
