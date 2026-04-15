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
 * pipeline_graphics.h
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
	 * @class PipelineGraphics
	 * @brief Represents a graphics pipeline in the Vulkan renderer.
	 */
	class PipelineGraphics : public RefCounted
	{
	public:
		/**
		 * @brief Constructs a new PipelineGraphics object.
		 * @param graphicsSpec The specification for the graphics pipeline.
		 */
		PipelineGraphics(PipelineSpecification graphicsSpec);

		virtual ~PipelineGraphics() override;

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
		 * @brief Sets the push constants for the pipeline.
		 * @param constants The buffer containing the push constants.
		 */
		void SetPushConstants(Buffer constants) const;

		/**
		 * @brief Gets the shader associated with the pipeline.
		 * @return The shader associated with the pipeline.
		 */
		Ref<Shader> GetShader() const { return m_Spec.shader; }

		/**
		 * @brief Checks if the pipeline has dynamic line width enabled.
		 * @return True if dynamic line width is enabled, false otherwise.
		 */
		bool IsDynamicLineWidth() const;

	private:
		Ref<Shader> m_Shader;
		Ref<CommandList> m_CommandList;
		PipelineSpecification m_Spec;
	};

}

// --------------------------------------------------------------
