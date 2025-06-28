/**
* -------------------------------------------------------
* Scenery Editor X
* -------------------------------------------------------
* Copyright (c) 2025 Thomas Ray 
* Copyright (c) 2025 Coalition of Freeware Developers
* -------------------------------------------------------
* renderer.cpp
* -------------------------------------------------------
* Created: 22/6/2025
* -------------------------------------------------------
*/
#include <SceneryEditorX/core/application.h>
#include <SceneryEditorX/logging/profiler.hpp>
#include <SceneryEditorX/renderer/render_context.h>
#include <SceneryEditorX/renderer/renderer.h>
#include <SceneryEditorX/renderer/vulkan/vk_swapchain.h>

/// -------------------------------------------------------

namespace SceneryEditorX
{
    /// Static variables
    LOCAL uint32_t s_RenderQueueIndex = 0;
    LOCAL uint32_t s_RenderQueueSubmissionIndex = 0;
    LOCAL uint32_t s_CurrentFrameIndex = 0;
    constexpr LOCAL uint32_t s_cmdQueueCount = 2;
    LOCAL CommandQueue *s_cmdQueue[s_cmdQueueCount];
    LOCAL std::atomic<uint32_t> s_cmdQueueSubmissionIdx = 0;
    LOCAL RenderData m_renderData;

    /// -------------------------------------------------------

    Ref<RenderContext> Renderer::GetContext()
    {
        return RenderContext::Get();
    }

    void Renderer::Init()
    {
        /// Initialize the rendering system. This includes setting up the render context, command buffers, etc.

        /// Get the render context

        /// Initialize the context if needed
        if (const auto context = GetContext())
            context->Init();

        /// Initialize render queue indices
        s_RenderQueueIndex = 0;
        s_RenderQueueSubmissionIndex = 0;
        s_CurrentFrameIndex = 0;
    }

    void Renderer::Shutdown()
    {
        // Shutdown the rendering system
        // Clean up resources, destroy Vulkan objects, etc.
    }

    void Renderer::BeginFrame()
    {
        // Begin a new frame
        // This would typically involve waiting for fences, acquiring swapchain images, etc.
    }

    void Renderer::EndFrame()
    {
        // End the current frame
        // This would typically involve submitting command buffers, presenting images, etc.
    }

    void Renderer::SubmitFrame()
    {
        // Submit the current frame to the GPU
        // This would involve submitting command buffers to the appropriate queues
    }

    CommandQueue &Renderer::GetRenderResourceReleaseQueue(uint32_t index)
    {
        return resourceFreeQueue[index];
    }

    uint32_t Renderer::GetRenderQueueIndex()
    {
        return s_RenderQueueIndex;
    }

    uint32_t Renderer::GetRenderQueueSubmissionIndex()
    {
        return s_RenderQueueSubmissionIndex;
    }

    uint32_t Renderer::GetCurrentFrameIndex()
    {
        return s_CurrentFrameIndex;
    }

    RenderData &Renderer::GetRenderData()
    {
        return m_renderData;
    }

	void Renderer::SetRenderData(const RenderData &renderData)
    {
        m_renderData = renderData;
    }

    void Renderer::RenderThreadFunc(ThreadManager *renderThread)
    {
        SEDX_PROFILE_THREAD("Render Thread");
        while (renderThread->isRunning())
            WaitAndRender(renderThread);
    }

    void Renderer::WaitAndRender(ThreadManager *renderThread)
    {
        renderThread->WaitAndSet(ThreadManager::State::Kick, ThreadManager::State::Busy);
        s_cmdQueue[GetRenderQueueIndex()]->Execute();

		/// Rendering has completed, set state to idle
        renderThread->Set(ThreadManager::State::Idle);

        SubmitFrame();
    }

    void Renderer::SwapQueues()
    {
        s_cmdQueueSubmissionIdx = (s_cmdQueueSubmissionIdx + 1) % s_cmdQueueCount;
    }

    uint32_t Renderer::GetCurrentRenderThreadFrameIndex()
    {
        /// Swapchain owns the Render Thread frame index
        return Application::Get().GetWindow().GetSwapChain().GetCurrentBufferIndex();
    }

    uint32_t Renderer::GetDescriptorAllocationCount(uint32_t frameIndex)
    {
        return m_renderData.DescriptorPoolAllocationCount[frameIndex];
    }

    /// -------------------------------------------------------

} // namespace SceneryEditorX

/// -------------------------------------------------------
