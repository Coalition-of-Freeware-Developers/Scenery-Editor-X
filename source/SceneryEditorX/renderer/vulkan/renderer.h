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
 * renderer.h
 * -------------------------------------------------------
 * Created: 09/02/2026
 * -------------------------------------------------------
 */
#pragma once
#include "SceneryEditorX/core/window/window.h"
#include "render_context.h"
#include "VulkanApp.h" // for ShaderDataBuffer, Texture, Vertex types
#include "command_list.h"

#include <array>
#include <vector>
#include <vma/vk_mem_alloc.h>
#include <vulkan/vulkan.h>
#include "viewport.h"

// -------------------------------------------------------

namespace SceneryEditorX
{
	struct RendererProperties;
	class Swapchain; // forward
	
	class Renderer 
	{
	public:
	    Renderer() = default;
	    ~Renderer() = default;

        static void Init();
        static void Shutdown();
        static void Tick();

        static void BeginFrame();
        static void EndFrame();
        static void SubmitFrame();

		static void DrawFrame(CommandList *cmdList, CommandList *computeCmdList);

		/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		/// Render Context Management																					  ///
        /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
                
	    /**
		 * @brief Retrieve the global render context instance.
		 * @return Shared reference to RenderContext.
		 */
        static Ref<RenderContext> GetContext();

	    /**
         * @brief Get the current frame-in-flight index (ring buffer slot).
         * @return Frame index.
         */
        static uint64_t GetCurrentFrameIndex();

        /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
        /// Swapchain Management																						  ///
        /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	    /**
         * @brief Get the SwapChain instance managed by the Renderer.
         * @return Pointer to the active SwapChain, or nullptr if not initialized.
         */
        static Swapchain *GetSwapChain();
        //static void BlitToBackBuffer(CommandList *cmd_list, Image *texture);
        static void SubmitAndPresent();

	    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
        /// Viewport & Image Management																					  ///
        /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

        static const Viewport &GetViewport();
        static void SetViewport(float width, float height);

        // Resolution Render
        static const Vec2 &GetRendererResolution();
        static void SetRendererResolution(uint32_t width, uint32_t height, bool recreateResources = true);

        // Resolution Output
        static const Vec2 &GetOutputResolution();
        static void SetOutputResolution(uint32_t width, uint32_t height, bool recreateResources = true);

	    // Run the renderer loop using the provided RenderContext. The function
	    // returns when the window is closed; it does not own the resources in
	    // the context (ownership remains with the caller).
	    int Run(const RenderContext& ctx);

	private:
        static Ref<Window> m_Window;
        static RendererProperties *s_Data;
        static RenderContext *m_Ctx; // Optional: store a reference to the context if needed for internal use
        static Ref<Swapchain> s_SwapChain;

        static std::atomic<bool> m_ResourcesInitialized;
        static CommandList *m_CurrentCmdList;
	};

}

// -------------------------------------------------------
