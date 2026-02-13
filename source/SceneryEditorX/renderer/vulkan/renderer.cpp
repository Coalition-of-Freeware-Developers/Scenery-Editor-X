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
 * renderer.cpp
 * -------------------------------------------------------
 * Created: 09/02/2026
 * -------------------------------------------------------
 */
#include "renderer.h"
#include "swapchain.h"
#include "SceneryEditorX/core/application/application.h"
#include "SceneryEditorX/core/time/timer.h"
#include "SceneryEditorX/utils/repeat_call_tracker.h"
#include <array>
#include <cstdlib>
#include <iostream>
#include <optional>
#include <glm/glm.hpp>
#include <volk/volk.h>

// --------------------------------------------------------------

namespace SceneryEditorX
{

    struct RendererProperties
	{
	    VkDescriptorSet activeRendererDescriptorSet = nullptr;
	    std::vector<VkDescriptorPool> descriptorPools;
	    VkDescriptorPool materialDescriptorPool;
	    std::vector<uint32_t> descriptorPoolAllocationCount;
	
	    /** Default samplers */
	    VkSampler samplerClamp = nullptr;
	    VkSampler samplerPoint = nullptr;
	
	    int32_t selectedDrawCall = -1;
	    int32_t drawCallCount = 0;
	};

	static inline void chk(VkResult result) 
	{
	    if (result != VK_SUCCESS) {
	        std::cerr << "Vulkan call returned an error (" << result << ")\n";
	        exit(result);
	    }
	}

	static inline void chk(bool result) 
	{
	    if (!result) {
	        std::cerr << "Call returned an error\n";
	        exit(EXIT_FAILURE);
	    }
	}

    // -------------------------------------------------------

    static RendererProperties *s_Data = nullptr;
    static bool s_Initialized = false;
    static uint64_t s_FrameNumber = 0;
    static float s_NearPlane = 0.0f;
    static float s_FarPlane = 1.0f;
    static Ref<Swapchain> s_SwapChain = nullptr;
    const uint8_t SWAPCHAIN_IMAGE_COUNT = 2;
    std::atomic<bool> Renderer::m_ResourcesInitialized = false;
    CommandList* Renderer::m_CurrentCmdList = nullptr;

    // resolution & viewport
    static xMath::Vec2 s_RendererResolution(0.0f, 0.0f);
    static xMath::Vec2 s_OutputResolution(0.0f, 0.0f);
    static Viewport s_Viewport = Viewport(0, 0, 0, 0);
    static bool s_OrthoProjection_Dirty = true;

    // -------------------------------------------------------

    void Renderer::Init()
    {
        SEDX_TRACK_CALL("Renderer::Init");

        // Prevent double-initialization
        if (s_Data)
        {
            SEDX_CORE_INFO_TAG("Renderer", R"(Init() called but renderer is already initialized — skipping)");
            return;
        }

        // Initialize volk loader then create an instance.
        volkInitialize();

        m_Ctx = new RenderContext();

        // Get the render context (Initialize the context if needed)
        if (const Ref<RenderContext> ctx = GetContext())
        {
            if (!ctx->IsInitialized())
            {
                ctx->Init();
            }
        }

        s_Data = new RendererProperties;
        SEDX_CORE_INFO_TAG("Renderer", "Initialized new RenderProperties: {}");

        /*
		if (Debugging::IsRenderdocEnabled())
        {
            RenderDoc::OnPreDeviceCreation();
        }
        */

        m_Window = Window::Get();

        uint32_t width = Window::GetWidth();
        uint32_t height = Window::GetHeight();

        SetOutputResolution(width, height, false);
        SetRendererResolution(1920, 1080, false);
        SetViewport(static_cast<float>(width), static_cast<float>(height));

        /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
        /// SwapChain																									  ///
        /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
        s_SwapChain = CreateRef<Swapchain>();
        s_SwapChain->Create(m_Ctx->surface, m_Ctx->queueFamily, m_Ctx->allocator);
    }

    void Renderer::Shutdown()
    {
        RenderContext::GetDevice()->GetQueueManager()->WaitIdleAll(); // Ensure device is idle before destroying resources
        s_SwapChain->Destroy();
        delete s_Data;
        s_Data = nullptr;
    }

    void Renderer::Tick()
    {
		s_SwapChain->AcquireNextImage();
        RenderContext::GetDevice()->GetMemoryAllocator()->Tick(s_FrameNumber);

		const uint32_t minRenderDimension = 64;
		bool isValidResolution = s_RendererResolution.x >= minRenderDimension && s_RendererResolution.y >= minRenderDimension;
        bool canRender = !Window::IsMinimized() && m_ResourcesInitialized && isValidResolution;

		{
            Ref<Queue> *queue = RenderContext::GetDevice()->GetQueueManager()->GetQueue(Graphics);
		}

		if (canRender)
		{
            DrawFrame(m_CurrentCmdList, nullptr);
		}

        s_FrameNumber++;
    }

    void Renderer::DrawFrame(CommandList *cmdList, CommandList *computeCmdList)
    {
        /*
        // early exit if one or more shaders aren't ready
        for (const auto &shader : GetShaders())
        {
            if (!shader || !shader->IsCompiled())
                return;
        }

        // acquire render targets
        Texture *renderThread_render = GetRenderTarget(Renderer_RenderTarget::frame_render);
        Texture *renderThread_output = GetRenderTarget(Renderer_RenderTarget::frame_output);
        */

    }

    Ref<RenderContext> Renderer::GetContext() { return RenderContext::Get(); }

    uint64_t Renderer::GetCurrentFrameIndex()
    {
        return s_FrameNumber;
    }

    Swapchain *Renderer::GetSwapChain() { return s_SwapChain.Get(); }

    void Renderer::SubmitAndPresent()
    {

    }

    const Viewport &Renderer::GetViewport() { return s_Viewport; }

    void Renderer::SetViewport(float width, float height)
    {
        SEDX_CORE_ASSERT(width != 0, "Width can't be zero");
        SEDX_CORE_ASSERT(height != 0, "Height can't be zero");

        if (s_Viewport.width != width || s_Viewport.height != height)
        {
            s_Viewport.width = width;
            s_Viewport.height = height;
            s_OrthoProjection_Dirty = true;
        }
    }

    const Vec2 &Renderer::GetRendererResolution()
    {
    }

    void Renderer::SetRendererResolution(uint32_t width, uint32_t height, bool recreateResources)
    {
        /*if (!RHI_Device::IsValidResolution(width, height))
        {
            SEDX_CORE_WARN_TAG("Renderer","Can't set %dx% as it's an invalid resolution", width, height);
            return;
        }

        if (s_RendererResolution.x == width && s_RendererResolution.y == height)
            return;

        s_RendererResolution.x = static_cast<float>(width);
        s_RendererResolution.y = static_cast<float>(height);
        if (recreateResources)
        {
            // if frames are in-flight, wait for them to finish before resizing
            if (m_cb_frame_cpu.frame > 1)
            {
                bool flush = true;
                m_Ctx->GetDevice()->GetQueueManager()->WaitIdleAll(flush);
            }

            CreateRenderTargets(true, false, true);
            CreateSamplers();
        }

        SEDX_CORE_INFO_TAG("Renderer", "Render resolution has been set to %dx%d", width, height);*/

    }

    const Vec2 &Renderer::GetOutputResolution()
    {
    }

    void Renderer::SetOutputResolution(uint32_t width, uint32_t height, bool recreateResources)
    {
    }

    int Renderer::Run(const RenderContext& ctx)
	{
	    // Copied render loop from previous monolithic implementation and
	    // refactored to operate on the references passed in by the app.
	    Timer clock;
	    uint32_t imageIndex{ 0 };
	    uint32_t frameIndex{ 0 };
	    ShaderData shaderData{};
	    Vec3 camPos{ 0.0f, 0.0f, -6.0f };
	    Vec3 objectRotations[3]{};
	    Vec2 lastMousePos { 0,0};

	    auto& device = ctx.device;
	    auto& queue = ctx.queue;
	    auto& allocator = ctx.allocator;
	    auto& swapHelper = *ctx.swapchain;
	    auto& pipeline = ctx.pipeline;
	    auto& pipelineLayout = ctx.pipelineLayout;
	    auto& descriptorSetTex = ctx.descriptorSetTex;
	    auto& vBuffer = ctx.vBuffer;
	    auto vBufSize = ctx.vBufSize;
	    auto indexCount = ctx.indexCount;
	    auto& shaderDataBuffers = *ctx.shaderDataBuffers;
	    auto& commandBuffers = *ctx.commandBuffers;
	    auto& fences = *ctx.fences;
	    auto& presentSemaphores = *ctx.presentSemaphores;
	    auto& renderSemaphores = *ctx.renderSemaphores;
	    auto queueFamily = ctx.queueFamily;
	
	    while (window.IsOpen()) 
	    {

	        // Sync
	        chk(vkWaitForFences(device, 1, &fences[frameIndex], true, UINT64_MAX));
	        chk(vkResetFences(device, 1, &fences[frameIndex]));
	        VkSwapchainKHR swapchain = swapHelper.Get();
	        auto &swapchainImages = swapHelper.Images();
	        auto &swapchainImageViews = swapHelper.ImageViews();
	        VkImageView depthImageView = swapHelper.GetDepthView();
	        VkResult acquireRes = vkAcquireNextImageKHR(device, swapchain, UINT64_MAX, presentSemaphores[frameIndex], VK_NULL_HANDLE, &imageIndex);
	        if (acquireRes == VK_ERROR_OUT_OF_DATE_KHR || acquireRes == VK_SUBOPTIMAL_KHR) 
	        {
	            // Swapchain no longer compatible with window; recreate and skip this frame.
	            swapHelper.Recreate(ctx.surface, queueFamily, allocator);
	            continue;
	        }

            SEDX_CORE_ASSERT(acquireRes == VK_SUCCESS, "vkAcquireNextImageKHR failed with error code: {}", acquireRes);

            // --------------------------------------------------------------

	        // Update shader data
	        shaderData.projection = Perspective(glm::radians(45.0f), static_cast<float>(Window::GetWidth()) / static_cast<float>(Window::GetHeight()), 0.1f, 32.0f);
	        shaderData.view = Translate(Mat4(1.0f), camPos);
	        for (auto i = 0; i < 3; i++) 
	        {
	            auto instancePos = Vec3(static_cast<float>(i - 1) * 3.0f, 0.0f, 0.0f);
	            shaderData.model[i] = Translate(Mat4(1.0f), instancePos) * Mat4(Quat(objectRotations[i]));
	        }
	        memcpy(shaderDataBuffers[frameIndex].mapped, &shaderData, sizeof(ShaderData));
			
            // --------------------------------------------------------------

	        // Build command buffer
	        auto cb = commandBuffers[frameIndex];
	        vkResetCommandBuffer(cb, 0);
	        VkCommandBufferBeginInfo cbBI 
	        { 
	            .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO, 
	            .flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT 
	        };

	        vkBeginCommandBuffer(cb, &cbBI);
	        std::array<VkImageMemoryBarrier2, 2> outputBarriers
	        {
	            VkImageMemoryBarrier2
	            {
	                .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2,
	                .srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
	                .srcAccessMask = 0,
	                .dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
	                .dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
	                .oldLayout = VK_IMAGE_LAYOUT_UNDEFINED,
	                .newLayout = VK_IMAGE_LAYOUT_ATTACHMENT_OPTIMAL,
	                .image = swapchainImages[imageIndex],
	                .subresourceRange{
	                    .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT, 
	                    .levelCount = 1, 
	                    .layerCount = 1 
	                }
	            },

	            VkImageMemoryBarrier2
	            {
	                .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2,
	                .srcStageMask = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT,
	                .srcAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,
	                .dstStageMask = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT,
	                .dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,
	                .oldLayout = VK_IMAGE_LAYOUT_UNDEFINED,
	                .newLayout = VK_IMAGE_LAYOUT_ATTACHMENT_OPTIMAL,
	                .image = swapHelper.GetDepthImage(),
	                .subresourceRange{
	                    .aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT, 
	                    .levelCount = 1, 
	                    .layerCount = 1 
	                }
	            }
	        };

	        VkDependencyInfo barrierDependencyInfo{ 
	            .sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO, 
	            .imageMemoryBarrierCount = 2, 
	            .pImageMemoryBarriers = outputBarriers.data() 
	        };

	        vkCmdPipelineBarrier2(cb, &barrierDependencyInfo);
	        VkRenderingAttachmentInfo colorAttachmentInfo{
	            .sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO,
	            .imageView = swapchainImageViews[imageIndex],
	            .imageLayout = VK_IMAGE_LAYOUT_ATTACHMENT_OPTIMAL,
	            .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
	            .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
	            .clearValue{.color{{0.0f, 0.0f, 0.0f, 1.0f}}}
	        };

	        VkRenderingAttachmentInfo depthAttachmentInfo{
	            .sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO,
	            .imageView = depthImageView,
	            .imageLayout = VK_IMAGE_LAYOUT_ATTACHMENT_OPTIMAL,
	            .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
	            .storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
	            .clearValue = {.depthStencil = {1.0f,  0}}
	        };

	        VkRenderingInfo renderingInfo{
	            .sType = VK_STRUCTURE_TYPE_RENDERING_INFO,
	            .renderArea{
					.extent{
					.width = Window::GetWidth(),
					.height = Window::GetHeight() }},
	            .layerCount = 1,
	            .colorAttachmentCount = 1,
	            .pColorAttachments = &colorAttachmentInfo,
	            .pDepthAttachment = &depthAttachmentInfo
	        };

	        vkCmdBeginRendering(cb, &renderingInfo);
	        VkViewport vp{
	            .width = static_cast<float>(Window::GetWidth()), 
	            .height = static_cast<float>(Window::GetHeight()), 
	            .minDepth = 0.0f, 
	            .maxDepth = 1.0f
	        };

	        vkCmdSetViewport(cb, 0, 1, &vp);
	        VkRect2D scissor{ 
	            .extent{ 
	                .width = Window::GetWidth(), 
	                .height = Window::GetHeight() 
	            }
	        };

	        vkCmdBindPipeline(cb, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);
	        vkCmdSetScissor(cb, 0, 1, &scissor);

	        // If a list of renderables is provided in the context, draw each one.
	        if (ctx.renderables && !ctx.renderables->empty())
			{
	            for (const auto& r : *ctx.renderables) 
	            {
	                vkCmdBindDescriptorSets(cb, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, 1, &r.descriptorSet, 0, nullptr);
	                VkDeviceSize vOffset{ 0 };
	                vkCmdBindVertexBuffers(cb, 0, 1, &r.buffer, &vOffset);
	                vkCmdBindIndexBuffer(cb, r.buffer, r.vertexByteSize, VK_INDEX_TYPE_UINT16);
	                vkCmdPushConstants(cb, pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(VkDeviceAddress), &shaderDataBuffers[frameIndex].deviceAddress);
	                vkCmdDrawIndexed(cb, r.indexCount, 3, 0, 0, 0);
	            }
	        } 
	        else 
	        {
	            vkCmdBindDescriptorSets(cb, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, 1, &descriptorSetTex, 0, nullptr);
	            VkDeviceSize vOffset{ 0 };
	            vkCmdBindVertexBuffers(cb, 0, 1, &vBuffer, &vOffset);
	            vkCmdBindIndexBuffer(cb, vBuffer, vBufSize, VK_INDEX_TYPE_UINT16);
	            vkCmdPushConstants(cb, pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(VkDeviceAddress), &shaderDataBuffers[frameIndex].deviceAddress);
	            vkCmdDrawIndexed(cb, indexCount, 3, 0, 0, 0);
	        }
	        vkCmdEndRendering(cb);
	        VkImageMemoryBarrier2 barrierPresent
	        {
	            .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2,
	            .srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
	            .srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
	            .dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
	            .dstAccessMask = 0,
	            .oldLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
	            .newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
	            .image = swapchainImages[imageIndex],
	            .subresourceRange{
	                .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT, 
	                .levelCount = 1, 
	                .layerCount = 1 
	            }
	        };

	        VkDependencyInfo barrierPresentDependencyInfo{ 
	            .sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO, 
	            .imageMemoryBarrierCount = 1, 
	            .pImageMemoryBarriers = &barrierPresent 
	        };

	        vkCmdPipelineBarrier2(cb, &barrierPresentDependencyInfo);
	        vkEndCommandBuffer(cb);

            // --------------------------------------------------------------

	        // Submit to graphics queue
	        VkPipelineStageFlags waitStages = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	        VkSubmitInfo submitInfo{
	            .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
	            .waitSemaphoreCount = 1,
	            .pWaitSemaphores = &presentSemaphores[frameIndex],
	            .pWaitDstStageMask = &waitStages,
	            .commandBufferCount = 1,
	            .pCommandBuffers = &cb,
	            .signalSemaphoreCount = 1,
	            .pSignalSemaphores = &renderSemaphores[imageIndex],
	        };

	        chk(vkQueueSubmit(queue, 1, &submitInfo, fences[frameIndex]));
	        frameIndex = (frameIndex + 1) % VulkanApp::MAX_FRAMES_IN_FLIGHT;

	        VkPresentInfoKHR presentInfo
	        {
	            .sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
	            .waitSemaphoreCount = 1,
	            .pWaitSemaphores = &renderSemaphores[imageIndex],
	            .swapchainCount = 1,
	            .pSwapchains = &swapchain,
	            .pImageIndices = &imageIndex
	        };

	        VkResult presentRes = vkQueuePresentKHR(queue, &presentInfo);
	        if (presentRes == VK_ERROR_OUT_OF_DATE_KHR || presentRes == VK_SUBOPTIMAL_KHR) 
	        {
	            // Present situation requires swapchain recreation
	            swapHelper.Recreate(ctx.surface, queueFamily, allocator);
	            continue;
	        }

	        if (presentRes != VK_SUCCESS)
	        {
                SEDX_CORE_ERROR_TAG("Renderer", "vkQueuePresentKHR failed with error code: {}", presentRes);
	            return -1;
	        }
			
            // --------------------------------------------------------------

	        // Event polling
	        Time elapsed = clock.Reset();
	        while (const std::optional event = window.ProcessEvents()) 
	        {
	            if (event->is<Event::Closed>()) 
	            {
                    Window::GetShouldClose();
	            }
	            if (const auto* mouseMoved = event->getIf<Event::MouseMoved>()) 
	            {
	                if (Mouse::isButtonPressed(Mouse::Button::Left)) 
	                {
	                    auto delta = lastMousePos - mouseMoved->position;
	                    objectRotations[shaderData.selected].x += (float)delta.y * 0.0005f * (float)elapsed.asMilliseconds();
	                    objectRotations[shaderData.selected].y -= (float)delta.x * 0.0005f * (float)elapsed.asMilliseconds();
	                }
	                lastMousePos = mouseMoved->position;
	            }
	            if (const auto* mouseWheelScrolled = event->getIf<Event::MouseWheelScrolled>()) 
	            {
	                camPos.z += static_cast<float>(mouseWheelScrolled->delta) * 0.025f * static_cast<float>(elapsed.asMilliseconds());
	            }
	            if (const auto* keyPressed = event->getIf<Event::KeyPressed>()) 
	            {
	                if (keyPressed->code == Keyboard::Key::Add) 
	                {
	                    shaderData.selected = (shaderData.selected < 2) ? shaderData.selected + 1 : 0;
	                }
	                if (keyPressed->code == Keyboard::Key::Subtract) 
	                {
	                    shaderData.selected = (shaderData.selected > 0) ? shaderData.selected - 1 : 2;
	                }
	            }
	
	            // Window resize - recreate swapchain and depth image
	            if (const auto* resized = event->getIf<Event::Resized>()) 
	            {
	                // Delegate full recreation to Swapchain::recreate which handles
	                // device idle and surface capability refresh internally.
	                swapHelper.Recreate(ctx.surface, queueFamily, allocator);
	            }
	        }
	    }
	
	    return 0;
	}

}

// --------------------------------------------------------------
