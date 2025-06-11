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
     * @note The values in this enum are bitwise OR'd together to create a bitfield representing multiple access types.
     * @note Identical to Vulkan's VkAccessFlagBits
     */
    enum class ResourceAccessFlags
    {
        None = 0,
        IndirectCommandRead = 0x00000001,
        IndexRead = 0x00000002,
        VertexAttributeRead = 0x00000004,
        UniformRead = 0x00000008,
        InputAttachmentRead = 0x00000010,
        ShaderRead = 0x00000020,
        ShaderWrite = 0x00000040,
        ColorAttachmentRead = 0x00000080,
        ColorAttachmentWrite = 0x00000100,
        DepthStencilAttachmentRead = 0x00000200,
        DepthStencilAttachmentWrite = 0x00000400,
        TransferRead = 0x00000800,
        TransferWrite = 0x00001000,
        HostRead = 0x00002000,
        HostWrite = 0x00004000,
        MemoryRead = 0x00008000,
        MemoryWrite = 0x00010000,
    };

	/// ---------------------------------------------------------

    enum ColorSpace : uint8_t
    {
        ColorSpace_SRGB_LINEAR,
        ColorSpace_SRGB_NONLINEAR,
    };

    /// ----------------------------------------------------------

    enum SamplerFilter : uint8_t
    {
        SamplerFilter_Nearest = 0,
        SamplerFilter_Linear
    };

    enum SamplerMip : uint8_t
    {
        SamplerMip_Disabled = 0,
        SamplerMip_Nearest,
        SamplerMip_Linear
    };

    enum SamplerWrap : uint8_t
    {
        SamplerWrap_Repeat = 0,
        SamplerWrap_Clamp,
        SamplerWrap_MirrorRepeat
    };

    enum PolygonMode : uint8_t
    {
        PolygonMode_Fill = 0,
        PolygonMode_Line = 1,
    };

}

/// -------------------------------------------------------
