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
 * shader.h
 * -------------------------------------------------------
 * Created: 24/02/2026
 * -------------------------------------------------------
 */
#pragma once
#include "shader_stage.h"
#include <SceneryEditorX/renderer/vulkan/descriptor.h>
#include <SceneryEditorX/renderer/vulkan/enums.h>

// -------------------------------------------------------

namespace SceneryEditorX
{
	/**
	 * @class Shader
	 * @brief Represents a Vulkan shader composed of multiple shader stages.
	 */
	class Shader : public SharedObject
	{
	public:
		Shader() = default;
		virtual ~Shader() override;

		/**
		 * @brief Adds a shader stage to the shader, loading the SPIR-V binary from the specified file path.
		 * @param stage The shader stage to add.
		 * @param filepath The file path to the SPIR-V binary.
		 */
		void AddShaderStage(Stage stage, const std::string& filepath);

		/**
		 * @brief Retrieves the shader stage for the specified stage.
		 * @param stage The shader stage to retrieve.
		 * @return A reference to the shader stage.
		 */
		Ref<ShaderStage> GetShaderStage(Stage stage);

		/**
		 * @brief Checks if the shader has the specified stage.
		 * @param stage The shader stage to check.
		 * @return True if the shader has the stage, false otherwise.
		 */
		bool HasStage(Stage stage);

		/* @brief Creates the descriptor set layouts for the shader. */
		void CreateDescriptorSetLayouts();

		/**
		 * @brief Retrieves the descriptor set layout bindings for the shader.
		 * @return A map of descriptor set layout bindings.
		 */
		std::map<uint32_t, std::vector<VkDescriptorSetLayoutBinding>> GetDescriptorSetLayoutBindings();

		/**
		 * @brief Retrieves the descriptor set layouts for the shader.
		 * @return A vector of descriptor set layouts.
		 */
		const std::vector<VkDescriptorSetLayout>& GetDescriptorSetLayouts() { return m_DescriptorSetLayouts; }

		/**
		 * @brief Retrieves the descriptor set layout for the specified set.
		 * @param set The descriptor set index.
		 * @return The descriptor set layout.
		 */
		VkDescriptorSetLayout GetDescriptorSetLayout(uint32_t set) { return m_DescriptorSetLayouts[set]; }

		/**
		 * @brief Retrieves the shader inputs for the specified descriptor set.
		 * @param set The descriptor set index.
		 * @return A vector of shader inputs.
		 */
		const std::vector<ShaderInput> GetInputs(uint32_t set) { return m_Input[set]; }

		/**
		 * @brief Retrieves the number of descriptor sets for the shader.
		 * @return The number of descriptor sets.
		 */
		uint32_t GetNumberOfSets() { return static_cast<uint32_t>(m_DescriptorSetLayouts.size()); }

		/**
		 * @brief Collects all shader inputs across all descriptor sets and converts them to Descriptor objects.
		 * @return A vector of Descriptor objects representing all bindable resources in this shader.
		 */
		std::vector<Descriptor> GetDescriptors();

		/**
		 * @brief Returns true if all shader stages have been compiled successfully.
		 * @return True if the shader has at least one stage and all stages have valid modules.
		 */
		bool IsCompiled() const { return !m_Stages.empty(); }

	private:
		std::unordered_map<Stage, Ref<ShaderStage>> m_Stages;		// Map of shader stages by stage type
		std::map<uint32_t, std::vector<ShaderInput>> m_Input;		// Map of shader inputs by descriptor set index
		std::set<uint32_t> m_BindlessSets;							// Set of bindless descriptor sets
		std::vector<VkDescriptorSetLayout> m_DescriptorSetLayouts;  // Vector of descriptor set layouts
	};

}

// -------------------------------------------------------
