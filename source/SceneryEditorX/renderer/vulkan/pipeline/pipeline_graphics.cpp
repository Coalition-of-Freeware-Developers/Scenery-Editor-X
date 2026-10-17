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
<<<<<<<< HEAD:source/SceneryEditorX/renderer/vulkan/pipeline/pipeline_graphics.cpp
 * pipeline_graphics.cpp
 * -------------------------------------------------------
 * Created: 14/04/2026
 * -------------------------------------------------------
 */
#include "pipeline_graphics.h"
#include <SceneryEditorX/renderer/vulkan/render_context.h>

// --------------------------------------------------------------

namespace SceneryEditorX
{
	
	PipelineGraphics::PipelineGraphics(PipelineSpecification graphicsSpec) : m_Spec(std::move(graphicsSpec))
	{

	}

	PipelineGraphics::~PipelineGraphics()
	{
		const Ref<Device> device = RenderContext::Get()->GetDevice();
		QueueManager::AddDeletionQueue(ResourceType::Pipeline, m_Pipeline);


		/*if (m_Pipeline)
			QueueManager::AddDeletionQueue(ResourceType::Pipeline, m_Pipeline);*/
	}

	void PipelineGraphics::SetPushConstants(Buffer constants) const
	{

	}

	bool PipelineGraphics::IsDynamicLineWidth() const
	{
		return m_Spec.topology == VkPrimitiveTopology::VK_PRIMITIVE_TOPOLOGY_LINE_LIST ||
			   m_Spec.topology == VkPrimitiveTopology::VK_PRIMITIVE_TOPOLOGY_LINE_STRIP || m_Spec.wireframe;
	}

} // namespace SceneryEditorX

// --------------------------------------------------------------
========
 * ui_renderer.cpp
 * -------------------------------------------------------
 * Created: 09/04/2026
 * -------------------------------------------------------
 */
#include "ui_renderer.h"

// -------------------------------------------------------

namespace SceneryEditorX
{

	void UIRenderer::Init()
	{
	}

	/*
	bool UIRenderer::UpdateFontTexture()
	{
		ImGuiIO& io = ImGui::GetIO();
		io.BackendRendererName = "HazelImGuiRenderer";
		io.BackendFlags |= ImGuiBackendFlags_RendererHasVtxOffset;  // We can honor the ImDrawCmd::VtxOffset field, allowing for large meshes.
		io.BackendFlags |= ImGuiBackendFlags_RendererHasViewports;  // We can create multi-viewports on the Renderer side (optional)
		// If the font texture exists and is bound to ImGui, we're done.
		// Note: ImGui_Renderer will reset io.Fonts->TexID when new fonts are added.
		if (m_FontTexture && io.Fonts->TexID)
			return true;

		unsigned char* pixels;
		int width, height;

		io.Fonts->GetTexDataAsRGBA32(&pixels, &width, &height);
		if (!pixels)
			return false;
	}
	*/

	bool UIRenderer::UpdateGeometry(ImDrawData *drawData)
	{
		return true;
	}

	} // namespace SceneryEditorX

// -------------------------------------------------------
>>>>>>>> e8126a5968df90e6d0729cf1257081d7862db7d6:source/SceneryEditorX/renderer/ui/ui_renderer.cpp
