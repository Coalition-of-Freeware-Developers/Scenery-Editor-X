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

/// -----------------------------------

namespace SceneryEditorX::ShaderSpecs
{
	
	enum class AOMethod
	{
		None = 0,
	    GTAO = BIT(1)
	};

	constexpr GLOBAL std::underlying_type_t<AOMethod> GetMethodIndex(const AOMethod method)
	{
		switch (method)
		{
			case AOMethod::None: return 0;
			case AOMethod::GTAO: return 1;
		}
		return 0;
	}
    constexpr GLOBAL ShaderSpecs::AOMethod ROMETHODS[4] = {AOMethod::None, AOMethod::GTAO};

	constexpr GLOBAL AOMethod GetAOMethod(const bool gtaoEnabled)
	{
		if (gtaoEnabled)
			return AOMethod::GTAO;

		return AOMethod::None;
	}

}

/// -------------------------------------------------------
