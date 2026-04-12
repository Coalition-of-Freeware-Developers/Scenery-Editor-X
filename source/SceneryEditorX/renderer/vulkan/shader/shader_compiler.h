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
 * shader_compiler.h
 * -------------------------------------------------------
 * Created: 11/04/2026
 * -------------------------------------------------------
 */
#pragma once
#include "shader.h"
#include "SceneryEditorX/renderer/vulkan/enums.h"

#include <vector>

// -------------------------------------------------------

/**
 * @namespace ShaderCompiler
 * @brief A simple manager for Vulkan shader modules. 
 * This is not intended to be a full-featured shader management system, 
 * but rather a minimal wrapper around Vulkan shader modules that allows us to compile and reflect shaders at runtime.
 */
namespace ShaderCompiler
{
	/**
	 * @enum State
	 * @brief Represents the current state of shader compilation. 
	 * This can be used to track the progress of shader compilation and handle any errors that may occur during the process. 
	 */
	enum class State : uint8_t
	{
		Idle,
		Compiling,
		Succeeded,
		Failed
	};
	
	/**
	 * @brief Compiles a Vulkan shader from a file.
	 * @param stage The shader stage to compile.
	 * @param filepath The path to the shader file.
	 * @param optimize Whether to optimize the shader during compilation.
	 * @return A vector of uint32_t representing the compiled SPIR-V bytecode.
	 */
	std::vector<uint32_t> CompileShader(SceneryEditorX::StageType stage, const std::string& filepath, bool optimize = false);

	/**
	 * @brief Reflects the input variables of a Vulkan shader.
	 * @param stage The shader stage to reflect.
	 * @param shaderBytecode The SPIR-V bytecode of the shader.
	 * @return A vector of ShaderInput structures representing the input variables of the shader.
	 */
	std::vector<SceneryEditorX::ShaderInput> Reflect(SceneryEditorX::StageType stage, const std::vector<uint32_t>& shaderBytecode);

}

// --------------------------------------------------------------
