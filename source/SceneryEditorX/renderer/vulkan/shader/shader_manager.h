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
	enum class State : uint8_t
	{
		Idle,
		Compiling,
		Succeeded,
		Failed
	};

	/**
	 * @struct ShaderCompilationResult
	 * @brief Bundles the SPIR-V bytecode and reflected shader inputs produced by a single
	 *        compile-and-reflect pass so both can be obtained in one Slang session without
	 *        loading the module twice.
	 */
	struct ShaderCompilationResult
	{
		std::vector<uint32_t>                        spirv;   ///< Compiled SPIR-V words ready for vkCreateShaderModule.
		std::vector<SceneryEditorX::ShaderInput>     inputs;  ///< Reflected descriptor bindings for this stage.
	};

	/**
	 * @brief Compiles a Vulkan shader from a file and produces SPIR-V bytecode.
	 *
	 * The function checks for a cached `.slang-module` alongside the cached `.spv` file.
	 * When the cache is fresh the front-end compilation step is skipped and the pre-built
	 * Slang IR blob is used directly, making subsequent loads significantly faster.
	 *
	 * @param stage    The shader stage to compile.
	 * @param filepath The path to the Slang source file.
	 * @param optimize Whether to apply Slang/SPIR-V optimisation passes.
	 * @return Compiled SPIR-V words, or an empty vector on failure.
	 */
	std::vector<uint32_t> CompileVulkanShader(SceneryEditorX::Stage stage, const std::string& filepath, bool optimize = false);

	/**
	 * @brief Reflects the descriptor inputs of a Slang shader using the Slang native reflection API.
	 *
	 * Loads the Slang module from source (or from the `.slang-module` cache if up-to-date),
	 * links the given entry point, and iterates the program layout to produce ShaderInput
	 * descriptors without requiring a separate SPIRV-Cross pass.
	 *
	 * @param stage    The shader stage whose entry point should be reflected.
	 * @param filepath The path to the Slang source file (used to locate the module).
	 * @return A vector of ShaderInput structures representing the bindable resources.
	 */
	std::vector<SceneryEditorX::ShaderInput> Reflect(SceneryEditorX::Stage stage, const std::string& filepath);

	/**
	 * @brief Compiles and reflects a Slang shader stage in a single Slang session.
	 *
	 * Combines the work of CompileVulkanShader and Reflect so the Slang module is only
	 * loaded once.  The `.slang-module` cache is checked before invoking the front-end
	 * compiler and is written on first compile for faster future loads.
	 *
	 * @param stage    The shader stage to compile and reflect.
	 * @param filepath The path to the Slang source file.
	 * @param optimize Whether to apply Slang/SPIR-V optimisation passes.
	 * @return A ShaderCompilationResult containing SPIR-V words and reflected inputs.
	 */
	ShaderCompilationResult CompileAndReflect(SceneryEditorX::Stage stage, const std::string& filepath, bool optimize = false);

}


// -------------------------------------------------------

namespace SceneryEditorX
{

	/**
	 * @class ShaderManager
	 * @brief Manages Vulkan shader modules and their corresponding stages.
	 * This class provides a convenient way to handle multiple shader stages within a single object.
	 * This allows Pipeline to accept a single object that may contain multiple stages (vertex, fragment, etc.).
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
		 * @brief Creates a new shader with the specified name.
		 * @param name name of the shader to create
		 * @return A reference to the created shader.
		 */
		static Ref<Shader>& CreateShader(const std::string& name);

		/**
		 * @brief Retrieves a shader with the specified name.
		 * @param name name of the shader to retrieve
		 * @return A reference to the retrieved shader.
		 */
		static Ref<Shader>& GetShader(const std::string& name);

		/**
		 * @brief Clears all shaders managed by the ShaderManager.
		 */
		static void Clear();

		/**
		 * @brief Retrieves the compilation state of the shaders.
		 * @return The current compilation state.
		 */
		size_t StageCount() const { return m_Stages.size(); }

		/**
		 * @brief Retrieves the shader stage at the specified index.
		 * @param i The index of the shader stage to retrieve.
		 * @return The shader stage flag at the specified index.
		 */
		VkShaderStageFlagBits StageAt(const size_t i) const { return m_Stages[i]; }

		/**
		 * @brief Retrieves the Vulkan shader module at the specified index.
		 * @param i The index of the shader module to retrieve.
		 * @return The Vulkan shader module at the specified index.
		 */
		VkShaderModule ModuleAt(const size_t i) const { return m_Modules[i]; }

		/**
		 * @brief Retrieves the compilation state of the shaders.
		 * @return The current compilation state.
		 */
		ShaderCompiler::State GetCompilationState() const { return m_CompilationState; }

		/**
		 * @brief Checks if the shaders have been successfully compiled.
		 * @return True if the shaders are compiled successfully, false otherwise.
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
