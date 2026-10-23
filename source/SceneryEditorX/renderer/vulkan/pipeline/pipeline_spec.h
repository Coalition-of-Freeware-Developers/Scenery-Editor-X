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
* pipeline_spec.h
* -------------------------------------------------------
* Created: 11/04/2026
* -------------------------------------------------------
*/
#pragma once
#include <SceneryEditorX/renderer/vulkan/input_layout.h>
#include <SceneryEditorX/renderer/vulkan/shader/shader.h>

// -------------------------------------------------------

namespace SceneryEditorX
{
	/**
	 * @struct PipelineSpecification
	 * @brief Specifies the configuration for a Vulkan pipeline.
	 */
	struct PipelineSpecification
	{
		Ref<Shader> shader = nullptr;
		InputLayout layout = {};
		InputLayout instanceLayout = {};
		InputLayout boneInfluenceLayout = {};
		VkPrimitiveTopology topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
		VkCompareOp depthOperator = VK_COMPARE_OP_GREATER_OR_EQUAL;
		bool backfaceCulling = true;
		bool depthTest = true;
		bool depthWrite = true;
		bool wireframe = false;
		float lineWidth = 1.0f;

		const char* debugName = "";
	};

}

// -------------------------------------------------------
