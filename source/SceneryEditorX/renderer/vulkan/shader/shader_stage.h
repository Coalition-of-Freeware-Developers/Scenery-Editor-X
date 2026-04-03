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
		 * @brief 
		 * @param stage 
		 * @param filepath 
		 */
		ShaderStage(Stage stage, const std::string& filepath);

		/**
		 * @brief 
		 */
		virtual ~ShaderStage() override;

		/**
		 * @brief 
		 */
		void Recompile();

		/**
		 * @brief 
		 * @return 
		 */
		[[nodiscard]] inline VkShaderModule GetHandle() const { return m_ShaderModule; };

		/**
		 * @brief 
		 * @return 
		 */
		const std::vector<ShaderInput>& GetInput() { return m_Input; }

		/**
		 * @brief 
		 * @return 
		 */
		const VkPipelineShaderStageCreateInfo GetStageCreateInfo();
	
	private:
		Stage m_Stage;						// The shader stage (e.g., vertex, fragment)
		std::string m_Filepath;				// Path to the shader source file
		VkShaderModule m_ShaderModule;		// Vulkan shader module handle
		std::vector<ShaderInput> m_Input;	// List of shader inputs (uniforms, samplers, etc.)
	};

}

// -------------------------------------------------------
