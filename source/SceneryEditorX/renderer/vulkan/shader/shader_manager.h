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
#include <SceneryEditorX/renderer/renderer_declarations.h>
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
		/**
		 * @struct ShaderStageDescriptor
		 * @brief Describes one stage to register for a renderer shader slot.
		 */
		struct ShaderStageDescriptor
		{
			StageType stage = StageType::MaxEnum;
			std::string filepath{};
			VertexType vertexType = VertexType::MaxEnum;
		};

		/**
		 * @struct ShaderRegistration
		 * @brief Data-driven shader registration record used by SetShaderAvailable.
		 */
		struct ShaderRegistration
		{
			Renderer_Shader id = Renderer_Shader::MaxEnum;
			const char* debugName = nullptr;
			bool asyncCompile = false;
			std::vector<ShaderStageDescriptor> stages{};
		};

		/**
		 * @brief Constructs a new ShaderManager.
		 */
		ShaderManager();

		/**
		 * @brief Destroys the ShaderManager and releases all associated Vulkan shader modules. 
		 */
		virtual ~ShaderManager() override;

		/**
		 * @brief Retrieves the singleton instance of the ShaderManager.
		 * @return A reference to the singleton ShaderManager instance.
		 */
		static Ref<ShaderManager> Get();

		/**
		 * @brief Creates all shaders managed by the ShaderManager.
		 */
		static void CreateShaders();

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
		static void ClearShaderStage(const Ref<Shader> &shader, const StageType stage);

		/**
		 * @brief Get a shader by type.
		 * @param type The type of shader to retrieve.
		 * @return Pointer to the requested shader.
		 */
		static Shader *GetShader(Renderer_Shader type);

		/**
		 * @brief Marks a shader as available, creating it if it doesn't already exist.
		 * @param type The type of shader to mark as available.
		 */
		static void SetShaderAvailable(Renderer_Shader type);

		/**
		 * @brief Checks if a shader of the specified type is available.
		 * @param type The type of shader to check.
		 * @return True if the shader is available, false otherwise.
		 */
		static bool IsShaderAvailable(Renderer_Shader type);

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
		 * @param stage The shader stage to compile.
		 * @param filepath The path to the shader file.
		 * @param optimize Whether to optimize the shader during compilation.
		 * @return A vector of uint32_t representing the compiled SPIR-V bytecode.
		 */
		static std::vector<uint32_t> CompileToSpirv(StageType stage, const std::string& filepath, bool optimize = false);

		/**
		 * @brief Reflect shader inputs from SPIR-V (delegates to ShaderCompiler)
		 * @param stage The shader stage for which to reflect inputs.
		 * @param spirv The SPIR-V bytecode from which to reflect shader inputs.
		 * @return A vector of ShaderInput structures representing the reflected shader inputs. 
		 */
		static std::vector<ShaderInput> ReflectInputs(StageType stage, const std::vector<uint32_t>& spirv);

		/**
		 * @brief Get all shaders managed by the ShaderManager.
		 * @return Reference to the array of shaders.
		 */
		static std::array<Ref<Shader>,  static_cast<uint32_t>(Renderer_Shader::MaxEnum)>& GetShaders();

	private:
		/**
		 * @brief Returns the shader registration metadata for all known startup shaders.
		 */
		static const std::vector<ShaderRegistration>& GetShaderRegistrations();

		/**
		 * @brief Finds a shader registration by renderer shader enum.
		 * @param type The shader enum to find the registration for.
		 * @return Pointer to the shader registration, or nullptr if not found.
		 */
		static const ShaderRegistration* FindRegistration(Renderer_Shader type);

		/**
		 * @brief Applies one shader registration into s_Shaders.
		 * @param registration The shader registration to apply.
		 */
		static void ApplyRegistration(const ShaderRegistration& registration);

		/**
		 * @brief Maps a renderer shader enum to its registration record.
		 * @return Reference to the shader registration map.
		 */
		static const std::unordered_map<Renderer_Shader, ShaderRegistration>& GetShaderRegistrationMap();

		/**
		 * @brief Returns true if this renderer shader should share one Shader object with another enum entry.
		 * @param type Enum slot being requested.
		 * @param outOwnerType Canonical enum slot owning the Shader object.
		 * @return True if the requested slot should share a Shader with another slot, false if it should have its own Shader.
		 */
		static bool TryGetSharedOwner(Renderer_Shader type, Renderer_Shader& outOwnerType);

		std::vector<VkShaderModule> m_Modules{};		// Store the Vulkan shader modules for each stage
		std::vector<VkShaderStageFlagBits> m_Stages{};	// Store the corresponding shader stage flags (e.g., VK_SHADER_STAGE_VERTEX_BIT, VK_SHADER_STAGE_FRAGMENT_BIT)
		static std::mutex s_ShaderMutex; // protect m_Shaders and compile operations
		static std::unordered_map<std::string, Ref<Shader>> m_Shaders;	// Static map to manage shaders by name
	};


}

// -------------------------------------------------------
