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
 * renderer_ssr.cpp
 * -------------------------------------------------------
 * Created: 21/10/2026
 * -------------------------------------------------------
 *
 * Screen-space reflections of the XP12 emulation (plan Phase 9; XP12: z_minmax, ssr_deferred, ssr_mesh). After the
 * g-buffer, the min / max depth pyramid of the finished depth (z_minmax.slang; the GPU scene's occlusion cull builds the
 * same target from the early depth); then every g-buffer pixel at least ssr/min_gloss glossy marches its mirror through
 * it and takes what it hits from the previous frame (ssr_trace.slang), into the reflections target the deferred lighting
 * blends over the probes. Blended surfaces trace their own in the forward pass. After fix_hdr the finished frame becomes
 * the next frame's history, with blurred mips for rough reflections. The CPU twin of the march, and the settings, are in
 * renderer/ssr.h.
 */
#include "renderer.h"
#include "renderer_declarations.h"
#include "vulkan/image_resource.h"
#include "vulkan/pipeline/pipeline_state.h"
#include "vulkan/shader/shader_manager.h"
#include <algorithm>

// -------------------------------------------------------

namespace SceneryEditorX
{
	namespace
	{
		Renderer::SsrReadout s_SsrReadout;
		bool s_SsrTraceAll = false;
		bool s_PyramidValid = false;		// The pyramid holds this frame's finished depth (Pass_ReflectionDepthPyramid)
		bool s_ReflectionsValid = false;	// The reflections target holds this frame's trace

		bool IsUsable(ImageResource *image)
		{
			return image && image->Get() && *image->Get() != VK_NULL_HANDLE && image->GetResourceState() == ResourceState::PreparedForGpu;
		}

		Shader *GetComputeShader(const Renderer_Shader id)
		{
			if (!ShaderManager::IsShaderAvailable(id))
				return nullptr;
			Shader *shader = ShaderManager::GetShader(id);
			return shader && shader->IsCompiled() ? shader : nullptr;
		}

		bool SetComputePipeline(CommandList *cmdList, const char *name, const Renderer_Shader id)
		{
			Shader *shader = GetComputeShader(id);
			if (!shader)
				return false;
			PipelineState pso;
			pso.name = name;
			pso.shaders[static_cast<uint32_t>(StageType::Compute)] = shader;
			cmdList->SetPipelineState(pso);
			return !cmdList->IsStickyInvalid();
		}

		void DispatchLevel(CommandList *cmdList, ImageResource *image, const uint32_t mip)
		{
			const uint32_t width = std::max(image->GetWidth() >> mip, 1u);
			const uint32_t height = std::max(image->GetHeight() >> mip, 1u);
			cmdList->Dispatch((width + 7) / 8, (height + 7) / 8, 1);
		}

		bool ReflectionShadersBuilt()
		{
			return GetComputeShader(Renderer_Shader::z_minmax_depth) && GetComputeShader(Renderer_Shader::z_minmax) &&
				   GetComputeShader(Renderer_Shader::ssr_trace) && GetComputeShader(Renderer_Shader::ssr_history_copy) &&
				   GetComputeShader(Renderer_Shader::ssr_history_mip);
		}
	}

	const Renderer::SsrReadout &Renderer::GetSsrReadout()
	{
		return s_SsrReadout;
	}

	void Renderer::SetSsrTraceAll(const bool all)
	{
		s_SsrTraceAll = all;
	}

	bool Renderer::GetSsrTraceAll()
	{
		return s_SsrTraceAll;
	}

	bool Renderer::Pass_DepthPyramid(CommandList *cmdList, ImageResource *depth)
	{
		ImageResource *pyramid = GetRenderTarget(Renderer_RenderTarget::gbuffer_depth_occluders_hiz);
		if (!cmdList || !IsUsable(pyramid) || !IsUsable(depth))
			return false;

		// Level 0 from the depth buffer, then each level from the one above (binding it moves it to shader-read: the
		// barrier after the dispatch that wrote it)
		depth->SetLayout(Layout::ImageLayout::ShaderRead, cmdList, 0, 0);
		if (!SetComputePipeline(cmdList, "z_minmax_depth", Renderer_Shader::z_minmax_depth))
			return false;
		cmdList->SetTexture(Renderer_BindingsSrv::tex, depth);
		cmdList->SetTexture(Renderer_BindingsUav::tex, pyramid, 0, 1);
		DispatchLevel(cmdList, pyramid, 0);

		if (!SetComputePipeline(cmdList, "z_minmax", Renderer_Shader::z_minmax))
			return false;
		for (uint32_t mip = 1; mip < pyramid->GetMipCount(); ++mip)
		{
			cmdList->SetTexture(Renderer_BindingsSrv::tex, pyramid, mip - 1, 1);
			cmdList->SetTexture(Renderer_BindingsUav::tex, pyramid, mip, 1);
			DispatchLevel(cmdList, pyramid, mip);
		}

		// Readers sample every level through one view: all of them shader-readable (the chain left them in different
		// layouts, and a whole-image transition assumes one)
		for (uint32_t mip = 0; mip < pyramid->GetMipCount(); ++mip)
			pyramid->SetLayout(Layout::ImageLayout::ShaderRead, cmdList, mip, 1);
		return !cmdList->IsStickyInvalid();
	}

	void Renderer::BeginReflectionsFrame()
	{
		s_PyramidValid = false;
		s_ReflectionsValid = false;
		if (!IsRenderingExtraView())
			s_SsrReadout.ran = false;
	}

	bool Renderer::AreReflectionsAvailable()
	{
		return GetFeatures().ssr && ReflectionShadersBuilt();
	}

	bool Renderer::Pass_ReflectionDepthPyramid(CommandList *cmdList)
	{
		if (!AreReflectionsAvailable() && !IsWaterVisible())	// Water reads the opaque depth under it from level 0
			return false;
		s_PyramidValid = Pass_DepthPyramid(cmdList, GetRenderTarget(Renderer_RenderTarget::gbuffer_depth));

		// The history is read in the forward pass too, inside its render pass: shader-readable from here on. Before its
		// first frame is written it is cleared, alpha 0 meaning no history (ssr_common.slang)
		if (ImageResource *history = GetRenderTarget(Renderer_RenderTarget::frame_render_previous); s_PyramidValid && IsUsable(history))
		{
			if (CommandList::GetImageLayout(history, 0) == Layout::ImageLayout::Undefined)
				cmdList->ClearTexture(history, Color::Clear());
			for (uint32_t mip = 0; mip < history->GetMipCount(); ++mip)
				history->SetLayout(Layout::ImageLayout::ShaderRead, cmdList, mip, 1);
		}
		return s_PyramidValid;
	}

	bool Renderer::Pass_ScreenSpaceReflections(CommandList *cmdList)
	{
		if (!s_PyramidValid || !GetFeatures().ssr)
			return false;
		ImageResource *output = GetRenderTarget(Renderer_RenderTarget::reflections);
		ImageResource *history = GetRenderTarget(Renderer_RenderTarget::frame_render_previous);
		ImageResource *pyramid = GetRenderTarget(Renderer_RenderTarget::gbuffer_depth_occluders_hiz);
		if (!IsUsable(output) || !IsUsable(history) || !IsUsable(pyramid))
			return false;

		if (!SetComputePipeline(cmdList, "ssr_trace", Renderer_Shader::ssr_trace))
			return false;
		cmdList->SetTexture(Renderer_BindingsSrv::gbuffer_depth, GetRenderTarget(Renderer_RenderTarget::gbuffer_depth));
		cmdList->SetTexture(Renderer_BindingsSrv::gbuffer_normal, GetRenderTarget(Renderer_RenderTarget::gbuffer_normal));
		cmdList->SetTexture(Renderer_BindingsSrv::gbuffer_material, GetRenderTarget(Renderer_RenderTarget::gbuffer_material));
		cmdList->SetTexture(Renderer_BindingsSrv::hiz, pyramid);
		cmdList->SetTexture(Renderer_BindingsSrv::ssr_history, history);
		cmdList->SetTexture(Renderer_BindingsSrv::ssr, GetStandardTexture(Renderer_StandardTexture::Black));	// ssr_common declares it; this pass writes it
		cmdList->SetTexture(Renderer_BindingsUav::tex, output);
		cmdList->Dispatch(output);
		if (cmdList->IsStickyInvalid())
			return false;
		output->SetLayout(Layout::ImageLayout::ShaderRead, cmdList, 0, 0);
		s_ReflectionsValid = true;

		if (!IsRenderingExtraView())
			s_SsrReadout = { true, s_SsrReadout.history, output->GetWidth(), output->GetHeight(), pyramid->GetMipCount(), history->GetMipCount() };
		return true;
	}

	void Renderer::Pass_ReflectionHistory(CommandList *cmdList, ImageResource *frame)
	{
		if (!GetFeatures().ssr || !s_PyramidValid)
			return;
		ImageResource *history = GetRenderTarget(Renderer_RenderTarget::frame_render_previous);
		if (!IsUsable(history) || !IsUsable(frame) || history->GetWidth() != frame->GetWidth() || history->GetHeight() != frame->GetHeight())
			return;

		// Level 0: the frame, its brightest texels clamped, alpha its pre-exposure
		frame->SetLayout(Layout::ImageLayout::ShaderRead, cmdList, 0, 0);
		if (!SetComputePipeline(cmdList, "ssr_history_copy", Renderer_Shader::ssr_history_copy))
			return;
		cmdList->SetTexture(Renderer_BindingsSrv::tex, frame);
		cmdList->SetTexture(Renderer_BindingsUav::tex, history, 0, 1);
		DispatchLevel(cmdList, history, 0);

		// The blurred levels rough reflections read
		if (!SetComputePipeline(cmdList, "ssr_history_mip", Renderer_Shader::ssr_history_mip))
			return;
		for (uint32_t mip = 1; mip < history->GetMipCount(); ++mip)
		{
			cmdList->SetTexture(Renderer_BindingsSrv::tex, history, mip - 1, 1);
			cmdList->SetTexture(Renderer_BindingsUav::tex, history, mip, 1);
			DispatchLevel(cmdList, history, mip);
		}
		for (uint32_t mip = 0; mip < history->GetMipCount(); ++mip)
			history->SetLayout(Layout::ImageLayout::ShaderRead, cmdList, mip, 1);
		if (!IsRenderingExtraView())
			s_SsrReadout.history = true;
	}

	void Renderer::BindReflections(CommandList *cmdList)
	{
		ImageResource *black = GetStandardTexture(Renderer_StandardTexture::Black);
		if (cmdList->UsesTexture(Renderer_BindingsSrv::ssr))
		{
			ImageResource *reflections = GetRenderTarget(Renderer_RenderTarget::reflections);
			cmdList->SetTexture(Renderer_BindingsSrv::ssr, s_ReflectionsValid && IsUsable(reflections) ? reflections : black);
		}
		if (cmdList->UsesTexture(Renderer_BindingsSrv::hiz))
		{
			ImageResource *pyramid = GetRenderTarget(Renderer_RenderTarget::gbuffer_depth_occluders_hiz);
			cmdList->SetTexture(Renderer_BindingsSrv::hiz, s_PyramidValid && IsUsable(pyramid) ? pyramid : black);
		}
		if (cmdList->UsesTexture(Renderer_BindingsSrv::ssr_history))
		{
			ImageResource *history = GetRenderTarget(Renderer_RenderTarget::frame_render_previous);
			cmdList->SetTexture(Renderer_BindingsSrv::ssr_history, s_PyramidValid && IsUsable(history) ? history : black);
		}
		if (cmdList->UsesTexture(Renderer_BindingsSrv::gbuffer_normal))
			cmdList->SetTexture(Renderer_BindingsSrv::gbuffer_normal, GetRenderTarget(Renderer_RenderTarget::gbuffer_normal));
	}
}

// -------------------------------------------------------
