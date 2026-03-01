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
#include "asset_manager.h"
#include "command_list.h"
#include "command_pool.h"
#include "frame_sync.h"
#include "render_context.h"
#include "viewport.h"
#include "SceneryEditorX/core/window/window.h"
#include <array>

// -------------------------------------------------------

namespace SceneryEditorX
{
    struct RendererProperties;
    class Swapchain;

    /**
     * @brief Static renderer class managing Vulkan rendering lifecycle.
     * 
     * The Renderer integrates with the Application main loop, providing:
     * - Frame synchronization (fences, semaphores)
     * - Command buffer management
     * - Swapchain presentation
     * 
     * Usage pattern (called by Application::Run):
     *   Renderer::BeginFrame();  // Acquire swapchain image, wait for fence
     *   // ... module Tick() calls and rendering ...
     *   Renderer::EndFrame();    // End command recording
     *   Renderer::SubmitAndPresent(); // Submit to GPU and present
     */
    class Renderer 
    {
    public:
        Renderer() = default;
        ~Renderer() = default;

        /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
        /// Lifecycle Methods - Called by Application                                                                     ///
        /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

        /**
         * @brief Initialize the renderer subsystem.
         * 
         * Creates Vulkan instance, device, swapchain, command pools, sync objects,
         * and all per-frame resources. Must be called once before any other Renderer method.
         */
        static void Init();

        /**
         * @brief Shutdown the renderer and release all resources.
         * 
         * Waits for GPU idle, destroys all Vulkan objects in correct order.
         * After calling Shutdown, Init must be called again before rendering.
         */
        static void Shutdown();

        /**
         * @brief Per-frame tick for memory/resource management.
         * 
         * Performs housekeeping tasks like memory allocator updates.
         * Called once per frame by Application.
         */
        static void Tick();

        /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
        /// Frame Rendering Methods - Called each frame in sequence                                                       ///
        /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

        /**
         * @brief Begin a new frame.
         * 
         * Acquires the next swapchain image, waits for the previous frame's fence,
         * resets the fence, and begins command buffer recording. This must be called
         * at the start of each frame before any draw calls.
         * 
         * @return true if frame can proceed, false if rendering should be skipped
         *         (e.g., window minimized or swapchain out of date)
         */
        static bool BeginFrame();

        /**
         * @brief End command buffer recording for the current frame.
         * 
         * Finalizes command buffer recording and transitions swapchain image
         * to present layout. Must be called after all draw commands are recorded.
         */
        static void EndFrame();

        /**
         * @brief Submit command buffers and present the swapchain image.
         * 
         * Submits the recorded command buffer to the graphics queue with proper
         * synchronization, then presents the image. Handles swapchain recreation
         * if VK_ERROR_OUT_OF_DATE_KHR is returned.
         */
        static void SubmitAndPresent();

        /**
         * @brief Record draw commands to the current frame's command buffer.
         * @param cmdList Graphics command list for 3D rendering
         * @param computeCmdList Compute command list (optional, may be nullptr)
         */
        static void DrawFrame(CommandList *cmdList, CommandList *computeCmdList);

        /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
        /// Render Context Management                                                                                     ///
        /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

        /**
         * @brief Retrieve the global render context instance.
         * @return Shared reference to RenderContext.
         */
        static Ref<RenderContext> GetContext();

        /**
         * @brief Get the current frame-in-flight index (ring buffer slot).
         * @return Frame index (0 to MAX_FRAMES_IN_FLIGHT-1).
         */
        static uint32_t GetCurrentFrameIndex();

        /**
         * @brief Get the total frame number since renderer initialization.
         * @return Total frame count.
         */
        static uint64_t GetFrameNumber();

        /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
        /// Swapchain Management                                                                                          ///
        /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

        /**
         * @brief Get the SwapChain instance managed by the Renderer.
         * @return Pointer to the active SwapChain, or nullptr if not initialized.
         */
        static Swapchain *GetSwapChain();

        /**
         * @brief Get the current swapchain image index.
         * @return Index of the acquired swapchain image.
         */
        static uint32_t GetSwapchainImageIndex();

        /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
        /// Viewport & Image Management                                                                                   ///
        /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

        static const Viewport &GetViewport();
        static void SetViewport(float width, float height);

        // Resolution Render
        static const Vec2 &GetRendererResolution();
        static void SetRendererResolution(uint32_t width, uint32_t height, bool recreateResources = true);

        // Resolution Output
        static const Vec2 &GetOutputResolution();
        static void SetOutputResolution(uint32_t width, uint32_t height, bool recreateResources = true);

        /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
        /// Command Buffer Access                                                                                         ///
        /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

        /**
         * @brief Get the command buffer for the current frame.
         * @return VkCommandBuffer for recording draw commands.
         */
        static VkCommandBuffer GetCurrentCommandBuffer();
		
		/**
		 * @brief Create models and upload to GPU. This is separate from shader creation to allow for better error handling and resource management.
		 */
        static void CreateModels();

        /**
         * @brief Create shader modules and pipelines.
         */
        static void CreateShaders();

    private:

        /**
         * @brief Create render targets (swapchain images, depth buffer) based on current swapchain configuration.
         * @param createRender Whether to create render targets (color/depth) for the swapchain images.
         * @param createOutput Whether to create output render targets.
         * @param createDynamic Whether to create dynamic render targets.
         */
        static void CreateRenderTargets(const bool createRender, const bool createOutput, const bool createDynamic);

        /**
         * @brief Create per-frame resources such as command buffers and synchronization objects.
         */
        static void CreateFrameResources();

        /**
         * @brief Destroy per-frame resources such as command buffers and synchronization objects.
         */
        static void DestroyFrameResources();

        /**
         * @brief Record draw commands for the current frame using the provided command lists.
         * @param cb Command buffer to record into (retrieved via GetCurrentCommandBuffer())
         * @param imageIndex Index of the swapchain image being rendered to (for resource binding)
         */
        static void RecordRenderCommands(VkCommandBuffer cb, uint32_t imageIndex);

        /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
        /// Static State                                                                                                  ///
        /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

        static RendererProperties *s_Data;
        static std::atomic<bool> s_ResourcesInitialized;
        CommandList *m_CurrentCmdList;

		// CPU-Side draw data staging
		//static std::array<Sb_DrawData, renderer_max_draw_calls> m_DrawData_CPU;
        static uint32_t m_DrawDataCount;

        static CommandList *m_CmdList_Compute;
        static CommandList *m_CmdList_Present;
        static Scope<AssetManager> s_AssetManager;
        static uint32_t m_ResourceIndex;

        // Frame synchronization
        static Scope<FrameSync> s_FrameSync;
        static Scope<CommandPool> s_CommandPool;
        static std::array<VkCommandBuffer, MAX_FRAMES_IN_FLIGHT> s_CommandBuffers;

        // Frame tracking
        static uint32_t s_CurrentFrameIndex;     // Ring buffer index (0 to MAX_FRAMES_IN_FLIGHT-1)
        static uint64_t s_FrameNumber;           // Total frames rendered
        static uint32_t s_SwapchainImageIndex;   // Current swapchain image
        static bool s_FrameInProgress;           // True between BeginFrame and EndFrame
    };

}

// -------------------------------------------------------
