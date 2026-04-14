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
#include <mutex>
#include <vector>
#include <SceneryEditorX/renderer/vulkan/enums.h>
#include <SceneryEditorX/renderer/vulkan/shader/shader_input.h>

// -------------------------------------------------------

namespace SceneryEditorX
{

	/**
	 * @class ShaderManager
	 * @brief Manages Vulkan shader modules and their corresponding stages.
	 * This class provides a convenient way to handle multiple shader stages within a single object.
	 * This allows Pipeline to accept a single object that may contain multiple stages (vertex, fragment, etc.).
	 */
	class ShaderManager : public RefCounted
	{
	public:
		ShaderManager();

		/* @brief Destroys the ShaderManager and releases all associated Vulkan shader modules. */
		~ShaderManager();

		/**
		 * @brief Retrieves the singleton instance of the ShaderManager.
		 * @return A reference to the singleton ShaderManager instance.
		 */
		static Ref<ShaderManager> Get();

		/**
		 * @brief Constructs a single shader with a single SPIR-V blob for both vertex and fragment stages.
		 * @param spirvCode Pointer to the SPIR-V bytecode.
		 * @param codeSize Size of the SPIR-V bytecode in bytes.
		 */
		void CreateSingleBlob(const void* spirvCode, size_t codeSize);

		/**
		 * @brief Creates Vulkan shader modules with multiple SPIR-V blobs for different shader stages.
		 * @param stages A vector of pairs, each containing a shader stage flag and a pair of SPIR-V bytecode pointer and size.
		 */
		void CreateShaderBlobs(const std::vector<std::pair<VkShaderStageFlagBits, std::pair<const void *, size_t>>> &stages);

		/**
		 * @brief Creates a new shader with the specified name.
		 * @param name name of the shader to create
		 * @return A reference to the created shader.
		 */
		static Ref<Shader>& CreateShader(const std::string& name);

		/**
		 * @brief Creates a new shader with the specified path and optional force compile flag.
		 * @param name name of the shader to create
		 * @param path Path to the shader file.
		 * @param forceCompile Whether to force compilation of the shader.
		 * @return A reference to the created shader.
		 */
		static Ref<Shader>& CreateShader(const std::string& name, const std::string &path, bool forceCompile);

		/**
		 * @brief Retrieves a shader with the specified name.
		 * @param name name of the shader to retrieve
		 * @return A reference to the retrieved shader.
		 */
		static Ref<Shader>& GetShader(const std::string& name);

		/**
		 * @brief Reloads a shader with the specified name and optional force compile flag.
		 * @param shaderName Name of the shader to reload.
		 * @param forceCompile Whether to force compilation of the shader.
		 */
		static void ReloadShader(const Ref<Shader> &shaderName, bool forceCompile = false);

		/**
		 * @brief Reloads a shader on the render thread with the specified name and optional force compile flag.
		 * @param shaderName Name of the shader to reload.
		 * @param forceCompile Whether to force compilation of the shader.
		 */
		static void RenderThread_Reload(const Ref<Shader> &shaderName, bool forceCompile);

		/**
		 * @brief Clears all shaders managed by the ShaderManager.
		 */
		static void ClearAll();

		/**
		 * @brief Clears a specific shader stage for the given shader.
		 * @param shader The shader whose stage is to be cleared.
		 * @param stage The stage to be cleared.
		 */
		static void ClearStage(const Ref<Shader> &shader, const StageType stage);

		/**
		 * @brief Retrieves the compilation state of the shaders.
		 * @return The current compilation state.
		 */
		[[nodiscard]] size_t StageCount() const { return m_Stages.size(); }

		/**
		 * @brief Retrieves the shader stage at the specified index.
		 * @param i The index of the shader stage to retrieve.
		 * @return The shader stage flag at the specified index.
		 */
		[[nodiscard]] VkShaderStageFlagBits StageAt(const size_t i) const { return m_Stages[i]; }

		/**
		 * @brief Retrieves the Vulkan shader module at the specified index.
		 * @param i The index of the shader module to retrieve.
		 * @return The Vulkan shader module at the specified index.
		 */
		[[nodiscard]] VkShaderModule ModuleAt(const size_t i) const { return m_Modules[i]; }

		/**
		 * @brief Compile a shader file to SPIR-V (delegates to ShaderCompiler)
		 * @param stage 
		 * @param filepath 
		 * @param optimize 
		 * @return 
		 */
		static std::vector<uint32_t> CompileToSpirv(StageType stage, const std::string& filepath, bool optimize = false);

		/**
		 * @brief Reflect shader inputs from SPIR-V (delegates to ShaderCompiler)
		 * @param stage 
		 * @param spirv 
		 * @return 
		 */
		static std::vector<ShaderInput> ReflectInputs(StageType stage, const std::vector<uint32_t>& spirv);

	private:
		std::vector<VkShaderModule> m_Modules{};		// Store the Vulkan shader modules for each stage
		std::vector<VkShaderStageFlagBits> m_Stages{};	// Store the corresponding shader stage flags (e.g., VK_SHADER_STAGE_VERTEX_BIT, VK_SHADER_STAGE_FRAGMENT_BIT)
		static std::mutex s_ShaderMutex; // protect m_Shaders and compile operations
		static std::unordered_map<std::string, Ref<Shader>> m_Shaders;	// Static map to manage shaders by name
	};


}

// -------------------------------------------------------
