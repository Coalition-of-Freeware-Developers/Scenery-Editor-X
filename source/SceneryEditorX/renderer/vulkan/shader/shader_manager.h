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
 * shader_manager.h
 * -------------------------------------------------------
 * Created: 09/02/2026
 * -------------------------------------------------------
 */
#pragma once
#include "shader.h"
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
	enum class State
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
	std::vector<uint32_t> CompileVulkanShader(SceneryEditorX::Stage stage, const std::string& filepath, bool optimize = false);

	/**
	 * @brief Reflects the input variables of a Vulkan shader.
	 * @param stage The shader stage to reflect.
	 * @param shaderBytecode The SPIR-V bytecode of the shader.
	 * @return A vector of ShaderInput structures representing the input variables of the shader.
	 */
	std::vector<SceneryEditorX::ShaderInput> Reflect(SceneryEditorX::Stage stage, const std::vector<uint32_t>& shaderBytecode);

}


// -------------------------------------------------------

namespace SceneryEditorX
{

	/**
	 * @class ShaderManager
	 * @brief Manages Vulkan shader modules and their corresponding stages.
	 * This class provides a convenient way to handle multiple shader stages within a single object.
	 * This allows Pipeline to accept a single object that may contain multiple stages (vertex, fragment, etc.).
	 *
	 */
	class ShaderManager 
	{
	public:
		ShaderManager() = default;

		/**
		 * Convenience ctor: use the same SPIR-V blob for both vertex and fragment
		 * stages (matches the original sample behavior).
		 */

		/**
		 * @brief Constructs a ShaderManager with a single SPIR-V blob for both vertex and fragment stages.
		 * @param spirvCode Pointer to the SPIR-V bytecode.
		 * @param codeSize Size of the SPIR-V bytecode in bytes.
		 */
		ShaderManager(const void* spirvCode, size_t codeSize);

		/**
		 * @brief Constructs a ShaderManager with multiple SPIR-V blobs for different shader stages.
		 * @param stages A vector of pairs, each containing a shader stage flag and a pair of SPIR-V bytecode pointer and size.
		 */
		ShaderManager(const std::vector<std::pair<VkShaderStageFlagBits, std::pair<const void*, size_t>>>& stages);

		/* @brief Destroys the ShaderManager and releases all associated Vulkan shader modules. */
		~ShaderManager();

		

		/**
		 * @brief 
		 * @param name 
		 * @return  
		 */
		static Ref<Shader>& CreateShader(const std::string& name);

		/**
		 * @brief 
		 * @param name 
		 * @return 
		 */
		static Ref<Shader>& GetShader(const std::string& name);

		/**
		 * @brief 
		 */
		static void Clear();

		/**
		 * @brief 
		 * @return 
		 */
		size_t StageCount() const { return m_Stages.size(); }

		/**
		 * @brief 
		 * @param i 
		 * @return 
		 */
		VkShaderStageFlagBits StageAt(size_t i) const { return m_Stages[i]; }

		/**
		 * @brief 
		 * @param i 
		 * @return 
		 */
		VkShaderModule ModuleAt(size_t i) const { return m_Modules[i]; }

		/**
		 * @brief 
		 * @return 
		 */
		ShaderCompiler::State GetCompilationState() const { return m_CompilationState; }

		/**
		 * @brief 
		 * @return 
		 */
		bool IsCompiled() const { return m_CompilationState == ShaderCompiler::State::Succeeded; }

	private:
		std::vector<VkShaderModule> m_Modules{};							// Store the Vulkan shader modules for each stage
		std::vector<VkShaderStageFlagBits> m_Stages{};						// Store the corresponding shader stage flags (e.g., VK_SHADER_STAGE_VERTEX_BIT, VK_SHADER_STAGE_FRAGMENT_BIT)
		static std::unordered_map<std::string, Ref<Shader>> m_Shaders;						 // Static map to manage shaders by name
		std::atomic<ShaderCompiler::State> m_CompilationState = ShaderCompiler::State::Idle; // Track the compilation state of the shaders
	};


}

// -------------------------------------------------------
