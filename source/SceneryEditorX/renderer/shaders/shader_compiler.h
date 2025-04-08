/**
* -------------------------------------------------------
* Scenery Editor X
* -------------------------------------------------------
* Copyright (c) 2025 Thomas Ray 
* Copyright (c) 2025 Coalition of Freeware Developers
* -------------------------------------------------------
* shader_compiler.h
* -------------------------------------------------------
* Created: 5/4/2025
* -------------------------------------------------------
*/

#pragma once
#include <filesystem>
// -------------------------------------------------------

namespace SceneryEditorX
{
	std::vector<char> CompileShader(const std::filesystem::path &path);

} // namespace SceneryEditorX

// -------------------------------------------------------
