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
 * shader_input.h
 * -------------------------------------------------------
 * Created: 24/02/2026
 * -------------------------------------------------------
 */
#pragma once
#include <SceneryEditorX/renderer/vulkan/enums.h>

// -------------------------------------------------------

namespace SceneryEditorX
{
	
	#define MAXIMUM_NUMBER_OF_SETS_PER_STAGE 8
	#define MAXIMUM_ARRAY_ELEMENTS (1024 * 16)
	#define MAX_BINDLESS (1024 * 4)
	
	// -------------------------------------------------------

	/**
	 * @struct ShaderInput
	 * @brief Represents a single input variable of a Vulkan shader.
	 */
	struct ShaderInput
	{
		std::string debugName;
	
		uint32_t set;
		uint32_t count;
		uint32_t index = 0;
		uint32_t binding;
		StageType stage;
		ShaderInputType type;
	};

	/**
	 * @brief Converts a Stage enum to a VkShaderStageFlagBits value.
	 * @param stage The Stage enum value to convert.
	 * @return The corresponding VkShaderStageFlagBits value.
	 */
	inline VkShaderStageFlagBits GetStage(const StageType &stage)
	{
		switch (stage)
		{
			case StageType::Vertex:							return VK_SHADER_STAGE_VERTEX_BIT;
			case StageType::Geometry:						return VK_SHADER_STAGE_GEOMETRY_BIT;
			case StageType::TessellationControl:			return VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT;
			case StageType::TessellationEvaluation:			return VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT;
			case StageType::Fragment:						return VK_SHADER_STAGE_FRAGMENT_BIT;
			case StageType::Compute:						return VK_SHADER_STAGE_COMPUTE_BIT;
			case StageType::None:							SEDX_CORE_ASSERT(false, "Invalid shader stage: None"); break;
			case StageType::MaxEnum:
			default:
				return VK_SHADER_STAGE_FLAG_BITS_MAX_ENUM;
		}

		return VK_SHADER_STAGE_ALL;
	}

	/**
	 * @brief Converts a ShaderInputType enum to a VkDescriptorType value.
	 * @param type The ShaderInputType enum value to convert.
	 * @return The corresponding VkDescriptorType value.
	 */
	inline VkDescriptorType GetInputType(const ShaderInputType &type)
	{
		switch (type)
		{
			case ShaderInputType::UniformBuffer:			return VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
			case ShaderInputType::UniformBufferSet:			return VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
			case ShaderInputType::StorageBuffer:			return VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
			case ShaderInputType::StorageBufferSet:			return VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
			case ShaderInputType::CombinedImageSampler:		return VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
			case ShaderInputType::Texture:					return VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
			case ShaderInputType::Sampler:					return VK_DESCRIPTOR_TYPE_SAMPLER;
			case ShaderInputType::StorageImage:				return VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
		}

		return VK_DESCRIPTOR_TYPE_MAX_ENUM;
	}

}

// -------------------------------------------------------
