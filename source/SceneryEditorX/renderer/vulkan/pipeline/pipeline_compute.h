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
 * pipeline_compute.h
 * -------------------------------------------------------
 * Created: 14/04/2026
 * -------------------------------------------------------
 */
#pragma once
#include "pipeline_spec.h"


#include <SceneryEditorX/renderer/vulkan/buffer.h>
#include <SceneryEditorX/renderer/vulkan/shader/shader.h>

// --------------------------------------------------------------

namespace SceneryEditorX
{
	class CommandList;

	/**
	 * @class PipelineCompute
	 * @brief Represents a compute pipeline in the Vulkan renderer.
	 */
	class PipelineCompute : public RefCounted
	{
	public:
		/**
		 * @brief Construct a compute pipeline with the given compute specification.
		 * @param computeSpec The specification for the compute pipeline.
		 */
		PipelineCompute(PipelineSpecification computeSpec);

        virtual ~PipelineCompute() override;

		/**
		 * @brief Create a compute pipeline with the given compute specification.
		 * @param computeSpec The specification for the compute pipeline.
		 * @return A reference to the created compute pipeline.
		 */
		static Ref<PipelineCompute> Create(const PipelineSpecification &computeSpec);
		
		/**
		 * @brief Gets the pipeline specification.
		 * @return The pipeline specification.
		 */
		PipelineSpecification& GetPipelineSpec() { return m_Spec; }

		/**
		 * @brief Gets the pipeline specification.
		 * @return The pipeline specification.
		 */
		const PipelineSpecification& GetPipelineSpec() const { return m_Spec; }

		/**
		 * @brief Set the push constants for the compute pipeline.
		 * @param constants The buffer containing the push constants to set.
		 */
		void SetPushConstants(Buffer constants) const;

		/**
		 * @brief Get the compute shader associated with this pipeline.
		 * @return A reference to the compute shader.
		 */
		Ref<Shader> GetShader() const { return m_Spec.shader; }
		
	private:
	    PipelineSpecification m_Spec;
		Ref<CommandList> m_CommandList;
	    VkPipeline m_Pipeline = VK_NULL_HANDLE;
	};
}

// --------------------------------------------------------------
