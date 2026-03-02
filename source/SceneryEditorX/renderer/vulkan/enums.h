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
        AccelerationStructure,
		
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
     * @enum Stage
	 * @brief Enumeration of shader stages in Vulkan
	 *
	 * This enum provides a type-safe way to identify and reference
	 * the different stages of a shader in the rendering system.
	 *
	 * @note - The values in this enum are designed to be compatible with Vulkan's VkShaderStageFlagBits.
	 * @see @enum VkShaderStageFlagBits
	 */
	enum class Stage : uint32_t
	{
	    Vertex,
		Geometry,
        TessellationControl,
        TessellationEvaluation,
        Fragment,
		Compute,
		None,
		All,
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
		StorageImage
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
     * @enum RendererRenderTarget
     * @brief Enumeration of renderer render targets.
     *
     * This enum provides a type-safe way to identify and reference
     * the different render targets used in the rendering system.
     *
     * @note - The values in this enum are designed to be compatible with Vulkan's render target types.
     */
    enum class RendererRenderTarget : uint8_t
    {
        gbuffer_color,
        gbuffer_normal,
        gbuffer_material,
        gbuffer_velocity,
        gbuffer_depth,
        gbuffer_depth_occluders,
        gbuffer_depth_occluders_hiz,
        gbuffer_depth_opaque_output,
        lut_brdf_specular,
        lut_atmosphere_scatter,
        lut_atmosphere_transmittance,
        lut_atmosphere_multiscatter,
        light_diffuse,
        light_specular,
        light_volumetric,
        frame_render,
        frame_render_opaque,
        frame_output,
        frame_output_2,
        ssao,
        reflections,
        gbuffer_reflections_position,
        gbuffer_reflections_normal,
        gbuffer_reflections_albedo,
        sss,
        skysphere,
        bloom,
        blur,
        outline,
        shading_rate,
        shadow_atlas,
        auto_exposure,
        auto_exposure_previous,
        // restir reservoir buffers (current frame)
        restir_reservoir0,
        restir_reservoir1,
        restir_reservoir2,
        restir_reservoir3,
        restir_reservoir4,
        // restir reservoir buffers (previous frame for temporal)
        restir_reservoir_prev0,
        restir_reservoir_prev1,
        restir_reservoir_prev2,
        restir_reservoir_prev3,
        restir_reservoir_prev4,
        // restir reservoir buffers (spatial ping-pong)
        restir_reservoir_spatial0,
        restir_reservoir_spatial1,
        restir_reservoir_spatial2,
        restir_reservoir_spatial3,
        restir_reservoir_spatial4,
        // volumetric clouds
        cloud_noise_shape,
        cloud_noise_detail,
        cloud_shadow,
        // nrd denoiser textures
        nrd_viewz,
        nrd_normal_roughness,
        nrd_diff_radiance_hitdist,
        nrd_spec_radiance_hitdist,
        nrd_out_diff_radiance_hitdist,
        nrd_out_spec_radiance_hitdist,
        // debug
        debug_output,
        max_enum
    };

} // namespace SceneryEditorX

// -----------------------------------------------------------------
