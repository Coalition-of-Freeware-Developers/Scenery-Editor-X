/**
* -------------------------------------------------------
* Scenery Editor X
* -------------------------------------------------------
* Copyright (c) 2025 Thomas Ray 
* Copyright (c) 2025 Coalition of Freeware Developers
* -------------------------------------------------------
* vk_enums.h
* -------------------------------------------------------
* Created: 10/6/2025
* -------------------------------------------------------
*/
#pragma once

/// -------------------------------------------------------

namespace SceneryEditorX
{
    /**
     * @enum ResourceAccessFlags
     * @brief Represents the access flags for Vulkan resources.
     *
     * This enum is used to specify the types of access that can be performed on Vulkan resources,
     * such as buffers and images.
     * It is used in synchronization operations to ensure proper access control and memory barriers.
     *
     * @note - The values in this enum are bitwise OR'd together to create a bitfield representing multiple access types.
     * @note - Identical to Vulkan's VkAccessFlagBits
     * @see @enum VkAccessFlagBits
     */
    enum class ResourceAccessFlags
    {
        None						= 0,
        IndirectCommandRead			= 0x00000001,
        IndexRead					= 0x00000002,
        VertexAttributeRead			= 0x00000004,
        UniformRead					= 0x00000008,
        InputAttachmentRead			= 0x00000010,
        ShaderRead					= 0x00000020,
        ShaderWrite					= 0x00000040,
        ColorAttachmentRead			= 0x00000080,
        ColorAttachmentWrite		= 0x00000100,
        DepthStencilAttachmentRead	= 0x00000200,
        DepthStencilAttachmentWrite = 0x00000400,
        TransferRead				= 0x00000800,
        TransferWrite				= 0x00001000,
        HostRead					= 0x00002000,
        HostWrite					= 0x00004000,
        MemoryRead					= 0x00008000,
        MemoryWrite					= 0x00010000,
    };

    /// -------------------------------------------------------

    /**
	 * @enum MemoryType
	 * @brief Represents the type of memory used in Vulkan.
	 *
	 * This enum is used to specify whether the memory is GPU or CPU accessible.
	 * It is used in resource allocation and management to determine the appropriate memory type.
	 *
	 * @note - The values in this enum are designed to be compatible with Vulkan's VkMemoryPropertyFlagBits.
	 */
	enum MemoryType : uint8_t
    {
        GPU = 0x00000001,
        CPU = 0x00000002 | 0x00000004,
    };

    /// -------------------------------------------------------

    namespace ShaderStage
    {
        enum class Stage
        {
            Vertex				= 0x00000001,
            TesselationControl	= 0x00000002,
            TesselationEval		= 0x00000004,
            Geometry			= 0x00000008,
            Fragment			= 0x00000010,
            Compute				= 0x00000020,
            AllGraphics			= 0x0000001F,
            Raygen				= 0x00000100,
            MaxEnum				= 0x7FFFFFFF,
        };
    }

    /// -------------------------------------------------------

    /**
     * @enum ShaderLanguage
     * @brief Represents the shader languages supported in Vulkan.
     *
     * This enum is used to specify the language of the shader code,
     * such as GLSL, HLSL, or SPIR-V.
     *
     * @note - The values in this enum are designed to be compatible with Vulkan's shader language specifications.
     */
    enum class ShaderLanguage : uint8_t
    {
        GLSL,
        HLSL,
        SPV,
    };

    /// -------------------------------------------------------

    /**
	 * @enum PipelineType
	 * @brief Represents the type of Vulkan pipeline.
	 *
	 * This enum is used to specify whether the pipeline is for graphics rendering or compute operations.
	 *
	 * @note - The values in this enum are designed to be compatible with Vulkan's VkPipelineBindPoint.
	 */
    enum class PipelineType : uint8_t
    {
        Graphics	= 0,
        Compute		= 1,
    };

    /// -------------------------------------------------------

	/**
	 * @enum PipelineStage
	 * @brief Represents the various stages of a Vulkan pipeline.
	 *
	 * This enum is used to specify the stages of the pipeline
	 * for synchronization purposes, such as when waiting for
	 * operations to complete or when setting up barriers.
	 *
	 * @note - The values in this enum are bitwise OR'd together
	 * to create a bitfield representing multiple stages.
	 * @note - Identical to Vulkan's VkPipelineStageFlagBits
	 * @see @enum VkPipelineStageFlagBits
	 */
    enum class PipelineStage
    {
        None						= 0,
        TopOfPipe					= 0x00000001,
        DrawIndirect				= 0x00000002,
        VertexInput					= 0x00000004,
        VertexShader				= 0x00000008,
        TesselationControlShader	= 0x00000010,
        TesselationEvaluationShader = 0x00000020,
        GeometryShader				= 0x00000040,
        FragmentShader				= 0x00000080,
        EarlyFragmentTests			= 0x00000100,
        LateFragmentTests			= 0x00000200,
        ColorAttachmentOutput		= 0x00000400,
        ComputeShader				= 0x00000800,
        Transfer					= 0x00001000,
        BottomOfPipe				= 0x00002000,
        Host						= 0x00004000,
        AllGraphics					= 0x00008000,
        AllCommands					= 0x00010000,
        TaskShader					= 0x00080000,
        MeshShader					= 0x00100000,
    };

	/// ---------------------------------------------------------

    /**
	 * @enum ColorSpace
	 * @brief Represents the color space used for textures in Vulkan.
	 *
	 * This enum is used to specify the color space of textures,
	 * such as whether they are in linear or nonlinear sRGB space.
	 *
	 * @note - The values in this enum are designed to be compatible with Vulkan's VkColorSpaceKHR.
	 * @see @enum VkColorSpaceKHR
	 */
    enum ColorSpace
    {
        SRGB_NonLinear		= 0,
        SRGB_Linear			= 1000104002,
        Adobe_RGB_Linear	= 1000104011,
        Adobe_RGB_NonLinear = 1000104012,
    };

    /// ----------------------------------------------------------

    /**
	 * @enum SamplerFilter
	 * @brief Defines the filtering modes for texture sampling in Vulkan.
	 *
	 * This enum specifies how textures should be sampled when accessed,
	 * such as whether to use nearest neighbor sampling, linear interpolation,
	 * or cubic interpolation.
	 *
	 * @note - The values in this enum are designed to be compatible with Vulkan's VkFilter.
	 * @see @enum VkFilter
	 */
    enum class SamplerFilter
    {
        Nearest = 0,
        Linear	= 1,
        Cubic	= 1000015000
    };

    /**
     * @enum SamplerMip
     * @brief Defines the mipmap filtering modes for texture sampling in Vulkan.
     *
     * This enum specifies how mipmaps should be sampled when textures are accessed,
     * such as whether to disable mipmapping, use nearest neighbor sampling, or linear interpolation.
     *
     * @note - The values in this enum are designed to be compatible with Vulkan's VkSamplerMipmapMode.
     * @see @enum VkSamplerMipmapMode
     */
    enum class SamplerMip : uint8_t
    {
        Nearest = 0,
        Linear	= 1
    };

    /**
	 * @enum SamplerWrap
	 * @brief Defines the wrapping modes for texture sampling in Vulkan.
	 *
	 * This enum specifies how textures should be wrapped when sampled,
	 * such as repeating, clamping, or mirroring.
	 *
	 * @note - The values in this enum are designed to be compatible with Vulkan's VkSamplerAddressMode.
	 * @see @enum VkSamplerAddressMode
	 */
    enum class SamplerWrap : uint8_t
    {
        Repeat			= 0,
        RepeatMirrored	= 1,
        ClampEdge		= 2,
        ClampBorder		= 3,
        MirrorOnce		= 4
    };

    /**
     * @enum PolygonMode
     * @brief Defines the polygon rendering modes in Vulkan.
     *
     * This enum specifies how polygons should be rendered,
     * such as filling them, drawing their outlines, or other modes.
     *
     * @note - The values in this enum are designed to be compatible with Vulkan's @enum VkPolygonMode.
     */
    enum class PolygonMode
    {
        Fill	= 0,
        Line	= 1,
        Point	= 2,
        MaxEnum = 0x7FFFFFFF
    };

    /**
	 * @enum TextureUsageBits
	 * @brief Defines the usage flags for textures in Vulkan.
	 *
	 * This enum specifies the various ways a texture can be used,
	 * such as being sampled, used for storage, or as an attachment.
	 *
	 * @note - The values in this enum are designed to be compatible with Vulkan's VkImageUsageFlagBits.
	 * @see @enum VkImageUsageFlagBits
	 */
    enum TextureUsageBits : uint8_t
    {
        Sampled	= 1 << 0,
        Storage	= 1 << 1,
        Attachment = 1 << 2,
    };

    /**
     * @enum Swizzle
     * @brief Defines the swizzle operations for texture channels.
     *
     * This enum specifies how the texture channels (R, G, B, A)
     * should be swizzled or remapped when sampling textures.
     *
     * @note - The values in this enum are designed to be compatible with Vulkan's @typedef VkComponentSwizzle.
     * @see @enum VkComponentSwizzle
     */
    enum Swizzle : uint8_t
    {
        Default			= 0,
        Swizzle_0		= 1,
        Swizzle_1		= 2,
        Swizzle_R		= 3,
        Swizzle_G		= 4,
        Swizzle_B		= 5,
        Swizzle_A		= 6,
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
        UniformSet,
        StorageBuffer,
        StorageSet,
        Texture2D,
        TextureCube,
        Image2D
    };

    /**
     * @enum ResourceInputType
     * @brief Defines the types of resources that can be used as inputs in shaders.
     *
     * This enum specifies the various types of resources that can be bound to shader stages,
     * such as uniform buffers, storage buffers, and image samplers.
     *
     * @note - The values in this enum are designed to be compatible with Vulkan's resource types.
     */
    enum class ResourceInputType : uint16_t  // NOLINT(performance-enum-size)
	{
        None = 0,
        UniformBuffer,
        StorageBuffer,
        ImageSampler1D,
        ImageSampler2D,
        ImageSampler3D, /// TODO: Clarify if this is a 3D sampler or Cube sampler
        StorageImage1D,
        StorageImage2D,
        StorageImage3D
	};

    /**
	 * @enum BlendMode
	 * @brief Defines the blending modes for framebuffer attachments.
	 *
	 * This enum specifies how colors from different framebuffer attachments
	 * should be blended together during rendering.
	 *
	 * @note - The values in this enum are designed to be compatible with Vulkan's @enum VkBlendFactor and @enum VkBlendOp.
	 */
	enum class BlendMode : uint8_t
    {
        None = 0,
        Alpha,
        Additive,
    };

    /**
	 * @enum AttachmentLoadOp
	 * @brief Defines the load operations for framebuffer attachments.
	 *
	 * This enum specifies how framebuffer attachments should be handled
	 * at the start of a render pass, such as whether to clear them or load
	 * existing data.
	 *
	 * @note - The values in this enum are designed to be compatible with Vulkan's @enum VkAttachmentLoadOp.
	 */
	enum class AttachmentLoadOp : uint8_t
    {
        Inherit = 0,
        Clear	= 1,
        Load	= 2
    };

    /**
     * @enum PrimitiveTopology
     * @brief Defines the primitive topology types used in Vulkan.
     *
     * This enum specifies the various primitive topologies
     * that can be used in rendering pipelines.
     *
     * @note - The values in this enum are designed to be compatible with Vulkan's @enum VkPrimitiveTopology.
     */
    enum class PrimitiveTopology
    {
        Points			= 0,
        Line			= 1,
        LineStrip		= 2,
        Triangles		= 3,
        TriangleStrip	= 4,
        TriangleFan		= 5,
        MaxEnum			= 0x7FFFFFFF
    };

    /**
     * @enum DepthCompareOperator
     * @brief Defines the depth compare operators used in Vulkan.
     *
     * This enum specifies the various depth comparison operations
     * that can be used in depth testing during rendering.
     *
     * @note - The values in this enum are designed to be compatible with Vulkan's VkCompareOp.
     * @see @enum VkCompareOp
     */
    enum class DepthCompareOperator
    {
        Never			= 0,
        Less			= 1,
        Equal			= 2,
        LessOrEqual		= 3,
        Greater			= 4,
        NotEqual		= 5,
        GreaterOrEqual	= 6,
        Always			= 7,
        MaxEnum			= 0x7FFFFFFF
    };

    /**
	 * @enum VertexBufferType
	 * @brief Defines the type and usage pattern of vertex buffer.
	 *
	 * This enum specifies how the vertex buffer is intended to be used,
	 * which can affect performance optimizations and memory management.
	 *
	 * @note - The values are designed to be compatible with Vulkan buffer usage flags.
	 * @see @enum VkBufferUsageFlagBits
	 */
    enum class VertexBufferType : uint8_t
    {
        None		= 0,    ///< No specific type defined.
        Static		= 1,    ///< Static data, rarely or never updated (GPU optimized).
        Dynamic		= 2,	///< Frequently changed data (CPU-GPU shared memory).
        Transient	= 3,	///< Single-use buffer that will be discarded after rendering.
        Streaming	= 4		///< Continuously streamed data (e.g. particles).
    };

    /**
	 * @enum VertexFormat
	 * @brief Standard vertex data formats.
	 *
	 * This enum defines various vertex formats that can be used in the vertex buffer.
	 * Each format specifies the components included in the vertex data,
	 * allowing for flexibility in rendering different types of geometry.
	 *
	 * @note - The values are designed to be compatible with Vulkan vertex input attribute descriptions.
	 */
    enum class VertexFormat : uint8_t
    {
        None								= 0,
        Position2D							= 1,    // Vec2 position
        Position3D							= 2,    // Vec3 position
        Position3D_Color3					= 3,    // Vec3 position + Vec3 color
        Position3D_Color4					= 4,    // Vec3 position + Vec4 color
        Position3D_Normal					= 5,    // Vec3 position + Vec3 normal
        Position3D_TexCoord					= 6,    // Vec3 position + Vec2 texcoord
        Position3D_Color4_TexCoord			= 7,    // Vec3 position + Vec4 color + Vec2 texcoord
        Position3D_Normal_TexCoord			= 8,    // Vec3 position + Vec3 normal + Vec2 texcoord
        Position3D_Normal_TexCoord_Tangent	= 9,	// Vec3 position + Vec3 normal + Vec2 texcoord + Vec4 tangent
        Custom								= 255	// Custom vertex format defined by user
    };

	/**
	 * @enum ShaderDataType
	 * @brief Represents the data types used in shaders.
	 *
	 * This enum defines the various data types that can be used in shader programs,
	 * such as floats, vectors, matrices, integers, and booleans.
	 *
	 * @note - The values in this enum are designed to be compatible with Vulkan's shader data types.
	 */
    enum class ShaderDataType : uint8_t
    {
        None = 0,
        Bool,
        Int,
        Float,
        Float2,
        Float3,
        Float4,
        Mat3,
        Mat4,
        Int2,
        Int3,
        Int4,
		IVec2,
        IVec3,
        IVec4,
		Vec2,
		Vec3,
		Vec4,
        Uint,
    };

    /**
	 * @enum Queue
	 * @brief Enumeration of Vulkan queue family types used in the rendering system
	 *
	 * Vulkan uses different queue families to execute different types of operations.
	 * This enum provides a type-safe way to identify and reference these queue families
	 * throughout the rendering system.
	 */
    enum Queue : uint8_t
    {
        Graphics = 0,  // Graphics queue family for rendering operations and drawing commands
        Compute  = 1,  // Compute queue family for compute shader and general computation operations
        Transfer = 2,  // Transfer queue family dedicated to memory transfer operations
        Count    = 3,  // Total number of queue families
        Present  = 4,  // Present queue family for presenting rendered images to the display surface
    };

    /**
     * @enum FrameSyncType
     * @brief Defines the synchronization primitive types used for frame synchronization in Vulkan.
     *
     * This enum specifies the types of synchronization primitives that can be used
     * to synchronize rendering operations between the CPU and GPU, such as fences and semaphores.
     *
     * @note - The values in this enum are designed to be compatible with Vulkan's VkSemaphoreType.
     * @see @enum VkSemaphoreType
     */
    enum class FrameSyncType
    {
        SyncFence             = 0,
        SyncSemaphore         = 1,
        SyncSemaphoreTimeline = 2,
        MaxEnum               = 0x7FFFFFFF
    };

	/**
	 * @enum CullMode
	 * @brief Represents the culling modes used in Vulkan.
	 *
	 * This enum specifies how polygons should be culled during rendering,
	 * such as whether to cull front-facing, back-facing, or both types of polygons.
	 *
	 * @note - The values in this enum are designed to be compatible with Vulkan's VkCullModeFlagBits.
	 * @see @enum VkCullModeFlagBits
	 */
    enum class CullMode
    {
        None			= 0,
        Front			= 0x00000001,
		Back			= 0x00000002,
		FrontAndBack	= 0x00000003,
        MaxEnum			= 0x7FFFFFFF,
    };

    /**
     * @enum FilterMode
     * @brief Defines the filtering modes for texture sampling.
     *
     * This enum specifies how textures should be filtered when sampled,
     * such as using nearest neighbor, linear interpolation, or cubic interpolation.
     *
     * @note - The values in this enum are designed to be compatible with Vulkan's VkFilter and VkSamplerMipmapMode.
     * @see @enum VkFilter and @enum VkSamplerMipmapMode
     */
    enum class FilterMode : uint8_t
	{
		Nearest = 0,
		Linear  = 1,
		Cubic   = 2
    };

    /**
	 * @enum RenderTarget
	 * @brief Defines the various render targets used in the rendering pipeline.
	 *
	 * This enum specifies the different types of render targets that can be used
	 * throughout the rendering process, such as G-buffers, lighting buffers, and post-processing effects.
	 *
	 * @note - The values in this enum are designed to be compatible with the rendering system's requirements.
     */
	enum class RenderTarget : uint8_t
    {
        gBuffer_Color,
        gBuffer_Normal,
        gBuffer_Material,
        gBuffer_Velocity,
        gBuffer_Depth,
        gBuffer_DepthOccluders,
        gBuffer_DepthOccluders_hiz,
        gBuffer_DepthOpaque_Output,
        LUT_Specular_BRDF,
        LUT_Atmosphere_Scatter,
        LightDiffuse,
        LightSpecular,
        LightShadow,
        LightVolumetric,
        FrameRender,
        FrameRenderOpaque,
        FrameOutput,
        FrameOutput2,
        SSAO,
        SSR,
        SSS,
        SkySphere,
        Bloom,
        Blur,
        Outline,
        ShadingRate,
        ShadowAtlas,
        MaxEnum
	};

    enum class RendererBufferId : uint8_t
    {
        ConstantFrame,
        SpdCounter,
        MaterialParameters,
        LightParameters,
        DummyInstance,
        AABBs,
        Visibility,
        VisibilityPrevious,
        MaxEnum
    };

	enum class RasterizerState : uint8_t
	{
	    Solid,
		Wireframe,
		PointSpot_Light,
		DirectionalSpot_Light,
		MaxEnum
	};

    enum class StandardTexture : uint8_t
    {
        Noise_blue_0,
        Noise_blue_1,
        Noise_blue_2,
        Noise_blue_3,
        Noise_blue_4,
        Noise_blue_5,
        Noise_blue_6,
        Noise_blue_7,
        Checkerboard,
        Gizmo_light_directional,
        Gizmo_light_point,
        Gizmo_light_spot,
        Black,
        White,
        MaxEnum
    };

    enum class Tonemapping : uint32_t
    {
        Aces,
        AgX,
        Reinhard,
        AcesNautilus,
        MaxEnum,
    };

    enum class Antialiasing : uint32_t
    {
        Disabled,
        Fxaa,
        Taa,
        TaaFxaa
    };

    enum class DepthStencilState : uint8_t
    {
        Off,
        ReadEqual,
        ReadGreaterEqual,
        ReadWrite,
        MaxEnum
    };

    enum class SamplerPreset : uint8_t
    {
        CompareDepth,
        PointClampEdge,
        PointClampBorder,
        PointWrap,
        BilinearClampEdge,
        BilinearClampBorder,
        BilinearWrap,
        TrilinearClamp,
        AnisotropicWrap,
        MaxEnum
    };

    enum class ShaderType : uint8_t
    {
        tessellation_h,
        tessellation_d,
        gbuffer_v,
        gbuffer_p,
        depth_prepass_v,
        depth_prepass_alpha_test_p,
        depth_light_v,
        depth_light_alpha_color_p,
        fxaa_c,
        film_grain_c,
        motion_blur_c,
        depth_of_field_c,
        chromatic_aberration_c,
        vhs_c,
        bloom_luminance_c,
        bloom_blend_frame_c,
        bloom_upsample_blend_mip_c,
        output_c,
        light_integration_brdf_specular_lut_c,
        light_integration_environment_filter_c,
        light_c,
        light_composition_c,
        light_image_based_c,
        line_v,
        line_p,
        grid_v,
        grid_p,
        outline_v,
        outline_p,
        outline_c,
        font_v,
        font_p,
        ssao_c,
        sss_c_bend,
        skysphere_c,
        skysphere_lut_c,
        blur_gaussian_c,
        blur_gaussian_bilaterial_c,
        variable_rate_shading_c,
        ffx_cas_c,
        ffx_spd_average_c,
        ffx_spd_min_c,
        ffx_spd_max_c,
        ffx_spd_luminance_c,
        blit_c,
        occlusion_c,
        icon_c,
        dithering_c,
        transparency_reflection_refraction_c,
        MaxEnum
    };

    enum class BlendSpec : uint8_t
    {
        Zero,
        One,
        Src_Color,
        Inv_Src_Color,
        Src_Alpha,
        Inv_Src_Alpha,
        Dest_Alpha,
        Inv_Dest_Alpha,
        Dest_Color,
        Inv_Dest_Color,
        Src_Alpha_Sat,
        Blend_Factor,
        Inv_Blend_Factor,
        Src1_Color,
        Inv_Src1_Color,
        Src1_Alpha,
        Inv_Src1_Alpha
    };

    enum class BlendOperation : uint8_t
    {
        Add,
        Subtract,
        Rev_Subtract,
        Min,
        Max,
        Undefined
    };

}

/// -------------------------------------------------------
