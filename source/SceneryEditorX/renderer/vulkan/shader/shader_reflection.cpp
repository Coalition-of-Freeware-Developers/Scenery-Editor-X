/**
 * -------------------------------------------------------
 * RailNex Train Simulator
 * -------------------------------------------------------
 * Copyright (c) 2026 Thomas Ray
 * Copyright (c) 2026 RailNex
 * -------------------------------------------------------
 * shader_reflection.cpp
 * -------------------------------------------------------
 * rnxForge.dll
 * -------------------------------------------------------
 * Created: 18/05/2026
 * -------------------------------------------------------
 */
#include "shader_reflection.h"
#include <filesystem>
#include <cstdint>
#include <utility>

// -------------------------------------------------------------

namespace SceneryEditorX 
{
	bool IsSingleShaderStage(ShaderType type) noexcept
	{
		const uint16_t bits = static_cast<std::underlying_type_t<ShaderType>>(type);
		return bits != 0u && (bits & (bits - 1u)) == 0u;
	}

	ShaderStage ShaderTypeToShaderStage(ShaderType type) noexcept
	{
		return ToShaderStage(type);
	}

	ShaderType ShaderStageToShaderType(ShaderStage stage) noexcept
	{
		return ToShaderType(stage);
	}

	VkShaderStageFlagBits ShaderTypeToVkStage(ShaderType type) noexcept
	{
		switch (type)
		{
			case ShaderType::Vertex:                 return VK_SHADER_STAGE_VERTEX_BIT;
			case ShaderType::TessellationControl:    return VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT;
			case ShaderType::TessellationEvaluation: return VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT;
			case ShaderType::Geometry:               return VK_SHADER_STAGE_GEOMETRY_BIT;
			case ShaderType::Fragment:               return VK_SHADER_STAGE_FRAGMENT_BIT;
			case ShaderType::Compute:                return VK_SHADER_STAGE_COMPUTE_BIT;
			case ShaderType::Task:                   return VK_SHADER_STAGE_TASK_BIT_EXT;
			case ShaderType::Mesh:                   return VK_SHADER_STAGE_MESH_BIT_EXT;
			case ShaderType::RayGeneration:          return VK_SHADER_STAGE_RAYGEN_BIT_KHR;
			case ShaderType::AnyHit:                 return VK_SHADER_STAGE_ANY_HIT_BIT_KHR;
			case ShaderType::ClosestHit:             return VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR;
			case ShaderType::Miss:                   return VK_SHADER_STAGE_MISS_BIT_KHR;
			case ShaderType::Intersection:           return VK_SHADER_STAGE_INTERSECTION_BIT_KHR;
			case ShaderType::Callable:               return VK_SHADER_STAGE_CALLABLE_BIT_KHR;
			default:                                 return VK_SHADER_STAGE_ALL;
		}
	}

	VkShaderStageFlags ShaderTypeToVkStageFlags(ShaderType type) noexcept
	{
		return static_cast<VkShaderStageFlags>(ShaderTypeToVkStage(type));
	}

	ShaderType InferShaderTypeFromPath(const std::filesystem::path& path) noexcept
	{
		const std::string full = path.generic_string();
		if (full.find(".vert.") != std::string::npos || path.stem().extension() == ".vert")
			return ShaderType::Vertex;
		if (full.find(".frag.") != std::string::npos || path.stem().extension() == ".frag")
			return ShaderType::Fragment;
		if (full.find(".comp.") != std::string::npos || path.stem().extension() == ".comp")
			return ShaderType::Compute;
		if (full.find(".mesh.") != std::string::npos)
			return ShaderType::Mesh;
		if (full.find(".task.") != std::string::npos)
			return ShaderType::Task;
		if (full.find(".rgen.") != std::string::npos)
			return ShaderType::RayGeneration;
		return ShaderType::Compute;
	}

}  // namespace SceneryEditorX 

// -------------------------------------------------------------
