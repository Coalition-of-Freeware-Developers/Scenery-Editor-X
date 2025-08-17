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
            Vertex		= 0x00000001,
            Geometry	= 0x00000008,
            Fragment	= 0x00000010,
            Compute		= 0x00000020,
            AllGraphics = 0x0000001F,
            All			= 0x7FFFFFFF,
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
        Graphics = 0,
        Compute = 1,
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
        AllCommands					= 0x00010000
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
	 */
    enum ColorSpace : uint8_t
    {
        ColorSpace_SRGB_LINEAR,
        ColorSpace_SRGB_NONLINEAR,
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
	 */
    enum class SamplerFilter : uint8_t
    {
        Nearest,
        Linear,
		Cubic
    };

    /**
     * @enum SamplerMip
     * @brief Defines the mipmap filtering modes for texture sampling in Vulkan.
     *
     * This enum specifies how mipmaps should be sampled when textures are accessed,
     * such as whether to disable mipmapping, use nearest neighbor sampling, or linear interpolation.
     *
     * @note - The values in this enum are designed to be compatible with Vulkan's VkSamplerMipmapMode.
     */
    enum class SamplerMip : uint8_t
    {
        Disabled = 0,
        Nearest,
        Linear
    };

    /**
	 * @enum SamplerWrap
	 * @brief Defines the wrapping modes for texture sampling in Vulkan.
	 *
	 * This enum specifies how textures should be wrapped when sampled,
	 * such as repeating, clamping, or mirroring.
	 *
	 * @note - The values in this enum are designed to be compatible with Vulkan's VkSamplerAddressMode.
	 */
    enum class SamplerWrap : uint8_t
    {
        Repeat = 0,
        Clamp,
        MirrorRepeat
    };

    /**
     * @enum PolygonMode
     * @brief Defines the polygon rendering modes in Vulkan.
     *
     * This enum specifies how polygons should be rendered,
     * such as filling them, drawing their outlines, or other modes.
     *
     * @note - The values in this enum are designed to be compatible with Vulkan's VkPolygonMode.
     */
    enum PolygonMode : uint8_t
    {
        PolygonMode_Fill = 0,
        PolygonMode_Line = 1,
    };

    /**
	 * @enum TextureUsageBits
	 * @brief Defines the usage flags for textures in Vulkan.
	 *
	 * This enum specifies the various ways a texture can be used,
	 * such as being sampled, used for storage, or as an attachment.
	 *
	 * @note - The values in this enum are designed to be compatible with Vulkan's VkImageUsageFlagBits.
	 */
    enum TextureUsageBits : uint8_t
    {
        TextureUsageBits_Sampled = 1 << 0,
        TextureUsageBits_Storage = 1 << 1,
        TextureUsageBits_Attachment = 1 << 2,
    };

    /**
     * @enum Swizzle
     * @brief Defines the swizzle operations for texture channels.
     *
     * This enum specifies how the texture channels (R, G, B, A)
     * should be swizzled or remapped when sampling textures.
     *
     * @note - The values in this enum are designed to be compatible with Vulkan's @typedef VkComponentSwizzle.
     * @see VkComponentSwizzle
     */
    enum Swizzle : uint8_t
    {
        Swizzle_Default = 0,
        Swizzle_0,
        Swizzle_1,
        Swizzle_R,
        Swizzle_G,
        Swizzle_B,
        Swizzle_A,
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
	 * @enum FramebufferBlendMode
	 * @brief Defines the blending modes for framebuffer attachments.
	 *
	 * This enum specifies how colors from different framebuffer attachments
	 * should be blended together during rendering.
	 *
	 * @note - The values in this enum are designed to be compatible with Vulkan's VkBlendFactor and VkBlendOp.
	 */
	enum class FramebufferBlendMode : uint8_t
    {
        None = 0,
        OneZero,
        SrcAlphaOneMinusSrcAlpha,
        Additive,
        Zero_SrcColor
    };

    /**
	 * @enum AttachmentLoadOp
	 * @brief Defines the load operations for framebuffer attachments.
	 *
	 * This enum specifies how framebuffer attachments should be handled
	 * at the start of a render pass, such as whether to clear them or load
	 * existing data.
	 *
	 * @note - The values in this enum are designed to be compatible with Vulkan's VkAttachmentLoadOp.
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
     * @note - The values in this enum are designed to be compatible with Vulkan's VkPrimitiveTopology.
     */
    enum class PrimitiveTopology : uint8_t
    {
        None = 0,
        Points,
        Lines,
        Triangles,
        LineStrip,
        TriangleStrip,
        TriangleFan
    };

    /**
     * @enum DepthCompareOperator
     * @brief Defines the depth compare operators used in Vulkan.
     *
     * This enum specifies the various depth comparison operations
     * that can be used in depth testing during rendering.
     *
     * @note - The values in this enum are designed to be compatible with Vulkan's VkCompareOp.
     */
    enum class DepthCompareOperator : uint8_t
    {
        None = 0,
        Never,
        NotEqual,
        Less,
        LessOrEqual,
        Greater,
        GreaterOrEqual,
        Equal,
        Always,
    };

    /**
	 * @enum VertexBufferType
	 * @brief Defines the type and usage pattern of vertex buffer.
	 *
	 * This enum specifies how the vertex buffer is intended to be used,
	 * which can affect performance optimizations and memory management.
	 *
	 * @note - The values are designed to be compatible with Vulkan buffer usage flags.
	 */
    enum class VertexBufferType : uint8_t
    {
        None = 0,      ///< No specific type defined.
        Static = 1,    ///< Static data, rarely or never updated (GPU optimized).
        Dynamic = 2,   ///< Frequently changed data (CPU-GPU shared memory).
        Transient = 3, ///< Single-use buffer that will be discarded after rendering.
        Streaming = 4  ///< Continuously streamed data (e.g. particles).
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
        None = 0,
        Position2D = 1,                         ///< Vec2 position
        Position3D = 2,                         ///< Vec3 position
        Position3D_Color3 = 3,                  ///< Vec3 position + Vec3 color
        Position3D_Color4 = 4,                  ///< Vec3 position + Vec4 color
        Position3D_Normal = 5,                  ///< Vec3 position + Vec3 normal
        Position3D_TexCoord = 6,                ///< Vec3 position + Vec2 texcoord
        Position3D_Color4_TexCoord = 7,         ///< Vec3 position + Vec4 color + Vec2 texcoord
        Position3D_Normal_TexCoord = 8,         ///< Vec3 position + Vec3 normal + Vec2 texcoord
        Position3D_Normal_TexCoord_Tangent = 9, ///< Vec3 position + Vec3 normal + Vec2 texcoord + Vec4 tangent
        Custom = 255                            ///< Custom vertex format defined by user
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
        Float,
        Float2,
        Float3,
        Float4,
        Mat3,
        Mat4,
        Int,
        Int2,
        Int3,
        Int4,
        Bool
    };

}

/// -------------------------------------------------------
