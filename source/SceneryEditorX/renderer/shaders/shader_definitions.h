/**
* -------------------------------------------------------
* Scenery Editor X
* -------------------------------------------------------
* Copyright (c) 2025 Thomas Ray
* Copyright (c) 2025 Coalition of Freeware Developers
* -------------------------------------------------------
* shader_definitions.h
* -------------------------------------------------------
* Created: 11/8/2025
* -------------------------------------------------------
*/
#pragma once
#include <SceneryEditorX/renderer/vulkan/vk_enums.h>

/// -----------------------------------

namespace SceneryEditorX::ShaderSpecs
{

	enum class AOMethod
	{
		None = 0,
	    GTAO = BIT(1)
	};

	constexpr static std::underlying_type_t<AOMethod> GetMethodIndex(const AOMethod method)
	{
		switch (method)
		{
			case AOMethod::None: return 0;
			case AOMethod::GTAO: return 1;
		}
		return 0;
	}
    constexpr static ShaderSpecs::AOMethod ROMETHODS[4] = {AOMethod::None, AOMethod::GTAO};

	constexpr static AOMethod GetAOMethod(const bool gtaoEnabled)
	{
		if (gtaoEnabled)
			return AOMethod::GTAO;

		return AOMethod::None;
	}

    static uint32_t ShaderStageToMask(ShaderStage::Stage type)
    {
        switch (type)
        {
			case ShaderStage::Stage::Vertex:				return 1 << 0;
			case ShaderStage::Stage::TesselationControl:	return 1 << 1;
			case ShaderStage::Stage::TesselationEval:		return 1 << 2;
			case ShaderStage::Stage::Fragment:				return 1 << 3;
			case ShaderStage::Stage::Compute:				return 1 << 4;
			default:										return 0;
        }
    }
    }

/// -------------------------------------------------------
