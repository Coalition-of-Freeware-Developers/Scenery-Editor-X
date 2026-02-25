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
#include "enums.h"
#include "shader_stage.h"

// -------------------------------------------------------

namespace SceneryEditorX
{

    class Shader : public RefCounted
	{
    public:
        Shader() = default;
        virtual ~Shader() override;

		/*
		void CreateDescriptorSetLayouts();
		void AddShaderStage(Stage stage, const std::string& filepath);
		Ref<ShaderStage> GetShaderStage(Stage stage);
		bool HasStage(Stage stage);
        std::map<uint32_t, std::vector<VkDescriptorSetLayoutBinding>> GetDescriptorSetLayoutBindings();
		const std::vector<VkDescriptorSetLayout>& GetDescriptorSetLayouts()
		{
			return m_DescriptorSetLayouts;
		}

		VkDescriptorSetLayout GetDescriptorSetLayout(uint32_t set) { return m_DescriptorSetLayouts[set]; }

		VkPipelineShaderStageCreateInfo const GetStageCreateInfo(ShaderStage stage);

	    const std::vector<ShaderInput> GetInputs(uint32_t set) { return m_Input[set]; }
	    uint32_t GetNumberOfSets() { return (uint32_t)m_DescriptorSetLayouts.size(); }
	    */

	private:

		/*
		std::unordered_map<Stage, Ref<ShaderStage>> m_Stages;
		std::map<uint32_t, std::vector<ShaderInput>> m_Input;
		std::set<uint32_t> m_BindlessSets;
		std::vector<VkDescriptorSetLayout> m_DescriptorSetLayouts;
		*/

	};

}

// -------------------------------------------------------
