/**
* -------------------------------------------------------
* Scenery Editor X
* -------------------------------------------------------
* Copyright (c) 2025 Thomas Ray
* Copyright (c) 2025 Coalition of Freeware Developers
* -------------------------------------------------------
* shader_module.h
* -------------------------------------------------------
* Created: 11/6/2025
* -------------------------------------------------------
*/
#pragma once
#include <SceneryEditorX/logging/asserts.h>

/// -------------------------------------------------------

namespace SceneryEditorX
{
	/**
	 * @namespace ShaderStage
	 * @brief Contains shader stage type flags for Vulkan shader operations.
	 */
	namespace ShaderStage
	{
	    /**
	     * @enum Stage
	     * @brief Defines the various shader stages in the Vulkan graphics/compute pipeline.
	     *
	     * These values match Vulkan's VkShaderStageFlagBits and can be combined
	     * using bitwise operations to specify multiple stages.
	     */
	    enum Stage : uint8_t
	    {
	        Vertex		= 0x00000001,   ///< Vertex shader stage for processing each vertex
	        Geometry	= 0x00000008,   ///< Geometry shader stage for processing primitives
	        Fragment	= 0x00000010,   ///< Fragment shader stage for processing fragments/pixels
	        Compute		= 0x00000020,	///< Compute shader stage for general-purpose computation
	        AllGraphics = 0x0000001F,	///< Combination of all graphics pipeline stages
	        All			= 0x7FFFFFFF,	///< All possible shader stages
	    };
	};


	struct ShaderModule
	{
        ShaderStage::Stage stage = ShaderStage::Fragment; ///< The shader stage(s) this module is intended for
        const char *data = nullptr;
        size_t dataSize = 0; /// if `dataSize` is non-zero, interpret `data` as binary shader data
        const char *debugName = "";

		ShaderModule(const char *source, ShaderStage::Stage stage, const char *debugName) : stage(stage), data(source), debugName(debugName) {}

		ShaderModule(const void *data, size_t dataLength, ShaderStage::Stage stage, const char *debugName) : stage(stage), data(static_cast<const char *>(data)), dataSize(dataLength), debugName(debugName)
		{
            SEDX_ASSERT(dataSize);
		}
    };
}

/// -------------------------------------------------------
