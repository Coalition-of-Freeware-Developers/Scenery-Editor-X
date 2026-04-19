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
* shader_stage.h
* -------------------------------------------------------
* Created: 24/02/2026
* -------------------------------------------------------
*/
#pragma once
#include "shader_input.h"
#include <SceneryEditorX/renderer/vulkan/enums.h>

// -------------------------------------------------------

namespace SceneryEditorX
{

	/**
	 * @class ShaderStage
	 * @brief ShaderStage represents a single stage in the graphics pipeline, encapsulating a Vulkan shader module and its associated inputs.
	 */
	class ShaderStage : public RefCounted
	{
	public:
		/**
		 * @brief Constructs a ShaderStage object.
		 * @param stage the shader stage (e.g., vertex, fragment)
		 * @param filepath the file path to the SPIR-V binary for this shader stage 
		 */
		ShaderStage(StageType stage, const std::string& filepath);

		/* 
		 * @brief Destroys the ShaderStage object and releases its resources. 
		 */
		virtual ~ShaderStage() override;

		/* 
		 * @brief Recompile the shader stage from its source file. 
		 */
		void Recompile();

		/**
		 * @brief Gets the Vulkan shader module handle.
		 * @return The Vulkan shader module handle.
		 */
		[[nodiscard]] inline VkShaderModule GetHandle() const { return m_ShaderModule; }

		/**
		 * @brief Gets the list of shader inputs for this stage.
		 * @return A reference to the vector of shader inputs.
		 */
		const std::vector<ShaderInput>& GetInput() const { return m_Input; }

		/**
		 * @brief Gets the Vulkan pipeline shader stage create info.
		 * @return The Vulkan pipeline shader stage create info.
		 */
		VkPipelineShaderStageCreateInfo GetStageCreateInfo() const;
	
	private:
		/**
		 * @brief Builds or rebuilds the Vulkan shader module for this stage by compiling the shader source file to SPIR-V and creating the shader module.
		 * @param optimize Whether to optimize the shader during compilation. This may enable additional compiler optimizations for better performance at the cost of longer compilation times.
		 * @param forceCompile When true, bypasses cache checks and forces recompilation.
		 * @return True if the shader module was successfully built or rebuilt, false otherwise.
		 */
		bool BuildOrRebuildModule(bool optimize = false, bool forceCompile = false);

		/**
		 * @brief Destroys the Vulkan shader module for this stage.
		 */
		void DestroyModule();

		StageType m_Stage;					// The shader stage (e.g., vertex, fragment)
		std::string m_Filepath;				// Path to the shader source file
		std::string m_EntryPoint;			// Stage entry point name used for pipeline creation
	    VkShaderModule m_ShaderModule = VK_NULL_HANDLE;		// Vulkan shader module handle
		std::vector<ShaderInput> m_Input;	// List of shader inputs (uniforms, samplers, etc.)
	};

}

// -------------------------------------------------------
