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
#include "asset_manager.h"
#include "swapchain.h"
#include "uniform_buffer_set.h"
#include "SceneryEditorX/core/application/application.h"
#include "SceneryEditorX/utils/repeat_call_tracker.h"
#include "slang/slang-com-ptr.h"
#include "slang/slang.h"
#include <array>
#include <SDL3/SDL.h>
#include <glm/glm.hpp>
#include <tracy/Tracy.hpp>
#include <volk/volk.h>

// --------------------------------------------------------------

namespace SceneryEditorX
{

    struct RendererProperties
    {
        VkDescriptorSet activeRendererDescriptorSet = nullptr;
        std::vector<VkDescriptorPool> descriptorPools;
        VkDescriptorPool materialDescriptorPool = VK_NULL_HANDLE;
        std::vector<uint32_t> descriptorPoolAllocationCount;
    
        /** Default samplers */
        VkSampler samplerClamp = nullptr;
        VkSampler samplerPoint = nullptr;
    
        int32_t selectedDrawCall = -1;
        int32_t drawCallCount = 0;
    };

    // -------------------------------------------------------
    // Static Member Definitions
    // -------------------------------------------------------

    RendererProperties *Renderer::s_Data = nullptr;
    Ref<Swapchain> Renderer::s_SwapChain = nullptr;
    std::atomic<bool> Renderer::s_ResourcesInitialized = false;
    CommandList *Renderer::s_CurrentCmdList = nullptr;

    Scope<AssetManager> Renderer::s_AssetManager = nullptr;
    Scope<FrameSync> Renderer::s_FrameSync = nullptr;
    Scope<CommandPool> Renderer::s_CommandPool = nullptr;

    std::array<VkCommandBuffer, MAX_FRAMES_IN_FLIGHT> Renderer::s_CommandBuffers = {};
    uint32_t Renderer::s_CurrentFrameIndex = 0;
    uint64_t Renderer::s_FrameNumber = 0;
    uint32_t Renderer::s_SwapchainImageIndex = 0;
    bool Renderer::s_FrameInProgress = false;

    // Resolution & viewport (internal state)
    static xMath::Vec2 s_RendererResolution(0.0f, 0.0f);
    static xMath::Vec2 s_OutputResolution(0.0f, 0.0f);
    static Viewport s_Viewport = Viewport(0, 0, 0, 0);
    static bool s_OrthoProjection_Dirty = true;
    //static Scope<UniformBufferSet> s_UniformBuffers = nullptr;
    static VkSurfaceCapabilitiesKHR s_SurfaceCaps = {};

    // -------------------------------------------------------
    // Lifecycle Methods
    // -------------------------------------------------------

    void Renderer::Init()
    {
        //SEDX_TRACK_CALL("Renderer::Init");

        // Prevent double-initialization
        if (s_Data)
        {
            SEDX_CORE_INFO_TAG("Renderer", "Init called but renderer is already initialized — skipping");
            return;
        }

        SEDX_CORE_TRACE_TAG("Renderer", "=== Initializing Renderer ===");

        // Initialize volk loader
        volkInitialize();

        if (!RenderContext::IsInitialized())
        {
            RenderContext::Init();
        }

        if (!RenderContext::IsInitialized())
        {
            SEDX_CORE_FATAL_TAG("Renderer", "RenderContext failed to initialize — cannot proceed with renderer setup");
            return;
        }

        s_Data = new RendererProperties;

        /*
		if (Debugging::IsRenderdocEnabled())
        {
            RenderDoc::OnPreDeviceCreation();
        }
        */


        /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
        /// SwapChain                                                                                                     ///
        /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
        s_SwapChain = CreateRef<Swapchain>();
        if (Window::GetWindow())
        {
            // Verify surface was created
            if (s_SwapChain->GetSurface() == VK_NULL_HANDLE)
            {
                SEDX_CORE_ERROR_TAG("Renderer", "Failed to create Vulkan surface - surface is still VK_NULL_HANDLE");
                return;
            }
        }
        else
        {
            SEDX_CORE_ERROR_TAG("Renderer", "Failed to get SDL window for surface creation");
            return;
        }

        // Get window dimensions
        uint32_t width = Window::GetWidth();
        uint32_t height = Window::GetHeight();

        SetOutputResolution(width, height, false);
        SetRendererResolution(1920, 1080, false);
        SetViewport(static_cast<float>(width), static_cast<float>(height));

        // Create the swapchain now that render context is initialized
        if (RenderContext::Get() && s_SwapChain->GetSurface() != VK_NULL_HANDLE)
        {
            uint32_t queueFamily = RenderContext::Get()->GetDevice()->GetQueueManager()->GetFamilyIndexByType(Graphics);
            VmaAllocator allocator = RenderContext::Get()->GetDevice()->GetMemoryAllocator()->GetAllocator();

            VkSwapchainKHR swapchainHandle = s_SwapChain->Create(s_SwapChain->GetSurface(), queueFamily, allocator);
            if (swapchainHandle != VK_NULL_HANDLE)
            {
                SEDX_CORE_TRACE_TAG("Renderer", "Swapchain created successfully with {} images", s_SwapChain->Images().size());
            }
            else
            {
                SEDX_CORE_ERROR_TAG("Renderer", "Failed to create swapchain");
                return;
            }
        }
        else
        {
            SEDX_CORE_WARN_TAG("Renderer", "Surface not available - swapchain creation deferred");
            return;
        }

        /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
        /// Frame Resources                                                                                               ///
        /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
        
        s_AssetManager = CreateScope<AssetManager>();

        CreateFrameResources();
        CreateModels();
        CreateShaders();

        // Query surface capabilities
        VkPhysicalDevice physicalDevice = RenderContext::Get()->GetDevice()->GetPhysicalDevice();
        vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physicalDevice, s_SwapChain->GetSurface(), &s_SurfaceCaps);

        std::vector<RenderContext::Renderable> renderables;
        Asset asset;
        renderables.push_back({.descriptorSet = asset.GetDescriptorSet(),
                               .buffer = asset.GetModelBuffer(),
                               .vertexByteSize = asset.GetModelVertexSize(),
                               .indexByteSize = asset.GetModelIndexSize(),
                               .indexCount = asset.GetModelIndexCount()});

        RenderContext::Get()->renderables = &renderables;
        //RenderContext::Get()->s_ShaderDataBuffers = s_UniformBuffers ? &s_UniformBuffers->Buffers() : nullptr;
        RenderContext::Get()->s_CommandBuffers = &s_CommandBuffers;
        RenderContext::Get()->s_Fences = s_FrameSync ? &s_FrameSync->Fences() : nullptr;
        RenderContext::Get()->s_PresentSemaphores = s_FrameSync ? &s_FrameSync->PresentSemaphores() : nullptr;
        RenderContext::Get()->s_RenderSemaphores = s_FrameSync ? &s_FrameSync->RenderSemaphores() : nullptr;
        RenderContext::Get()->s_SurfaceCaps = &s_SurfaceCaps;

        s_ResourcesInitialized = true;
        SEDX_CORE_INFO_TAG("Renderer", "=== Renderer Initialization Complete ===");
    }

    void Renderer::Shutdown()
    {
        SEDX_CORE_INFO_TAG("Renderer", "=== Shutting Down Renderer ===");

        // Wait for all GPU work to complete
        if (RenderContext::Get())
        {
            RenderContext::Get()->GetDevice()->GetQueueManager()->WaitIdleAll();
        }

        // Destroy frame resources
        DestroyFrameResources();
        s_AssetManager->DestroyAll();
        s_AssetManager.reset();

        // Destroy swapchain
        if (s_SwapChain)
        {
            s_SwapChain->Destroy();
            s_SwapChain.Reset();
        }

        // Cleanup renderer data
        delete s_Data;
        s_Data = nullptr;

        s_ResourcesInitialized = false;
        SEDX_CORE_TRACE_TAG("Renderer", "=== Renderer Shutdown Complete ===");
    }

    void Renderer::Tick()
    {
        // Memory allocator housekeeping
        if (RenderContext::Get() && RenderContext::Get()->GetDevice()->GetMemoryAllocator())
        {
            RenderContext::Get()->GetDevice()->GetMemoryAllocator()->Tick(s_FrameNumber);
        }
    }

    // -------------------------------------------------------
    // Frame Resources
    // -------------------------------------------------------

    void Renderer::CreateFrameResources()
    {
        SEDX_CORE_TRACE_TAG("Renderer", "Creating frame resources for {} frames in flight", MAX_FRAMES_IN_FLIGHT);

        // Get queue family index from queue manager
        uint32_t queueFamily = RenderContext::Get()->GetDevice()->GetQueueManager()->GetFamilyIndexByType(Graphics);

        // Create command pool
        s_CommandPool = CreateScope<CommandPool>(queueFamily, CommandPoolType::Resettable);
        SEDX_CORE_TRACE_TAG("Renderer", "Created command pool");

        // Allocate command buffers
        auto allocatedBuffers = s_CommandPool->Allocate(MAX_FRAMES_IN_FLIGHT);
        for (size_t i = 0; i < allocatedBuffers.size() && i < s_CommandBuffers.size(); ++i)
        {
            s_CommandBuffers[i] = allocatedBuffers[i];
        }
        SEDX_CORE_TRACE_TAG("Renderer", "Allocated {} command buffers", MAX_FRAMES_IN_FLIGHT);

        // Create frame sync objects - use actual swapchain image count or fallback
        uint32_t swapchainImageCount = 2; // Default fallback
        if (s_SwapChain && !s_SwapChain->Images().empty())
        {
            swapchainImageCount = static_cast<uint32_t>(s_SwapChain->Images().size());
            SEDX_CORE_TRACE_TAG("Renderer", "Using swapchain image count: {}", swapchainImageCount);
        }
        else
        {
            SEDX_CORE_WARN_TAG("Renderer", "Swapchain not ready - using default image count: {}", swapchainImageCount);
        }

        s_FrameSync = CreateScope<FrameSync>(MAX_FRAMES_IN_FLIGHT, swapchainImageCount);
        SEDX_CORE_TRACE_TAG("Renderer", "Created frame sync objects (fences: {}, present semaphores: {}, render semaphores: {})", s_FrameSync->Fences().size(), s_FrameSync->PresentSemaphores().size(), s_FrameSync->RenderSemaphores().size());
    }

    void Renderer::DestroyFrameResources()
    {
        SEDX_CORE_TRACE_TAG("Renderer", "Destroying frame resources");

        // Destroy sync objects
        if (s_FrameSync)
        {
            s_FrameSync->Destroy();
            s_FrameSync.reset();
        }

        // Command buffers are freed when command pool is destroyed
        s_CommandBuffers.fill(VK_NULL_HANDLE);

        // Destroy command pool
        if (s_CommandPool)
        {
            s_CommandPool->Destroy();
            s_CommandPool.reset();
        }

        SEDX_CORE_TRACE_TAG("Renderer", " Frame resources destroyed");
    }

    // -------------------------------------------------------
    // Frame Rendering Methods
    // -------------------------------------------------------

    bool Renderer::BeginFrame()
    {
        // Check if we can render
        if (!s_ResourcesInitialized)
        {
            SEDX_CORE_WARN_TAG("Renderer", "BeginFrame called but renderer not initialized");
            return false;
        }

        // Skip if window is minimized
        const uint32_t minRenderDimension = 64;
        bool isValidResolution = s_RendererResolution.x >= minRenderDimension && s_RendererResolution.y >= minRenderDimension;

        if (Window::IsMinimized() || !isValidResolution)
        {
            return false;
        }

        // Check swapchain validity with detailed diagnostics
        if (!s_SwapChain)
        {
            SEDX_CORE_ERROR_TAG("Renderer", "BeginFrame: Swapchain is null - was Init() called successfully?");
            return false;
        }

        if (s_SwapChain->Images().empty())
        {
            SEDX_CORE_ERROR_TAG("Renderer", "BeginFrame: Swapchain has no images - VkSwapchainKHR handle: {}, surface valid: {}",
                                (void *)s_SwapChain->Get(), s_SwapChain->GetSurface() != VK_NULL_HANDLE);

            // Attempt to recreate swapchain if surface is available
            if (s_SwapChain->GetSurface() != VK_NULL_HANDLE)
            {
                SEDX_CORE_WARN_TAG("Renderer", "Attempting to recreate swapchain...");
                uint32_t queueFamily = RenderContext::Get()->GetDevice()->GetQueueManager()->GetFamilyIndexByType(Graphics);
                VmaAllocator allocator = RenderContext::Get()->GetDevice()->GetMemoryAllocator()->GetAllocator();
                VkSwapchainKHR handle = s_SwapChain->Recreate(s_SwapChain->GetSurface(), queueFamily, allocator);
                if (handle != VK_NULL_HANDLE && !s_SwapChain->Images().empty())
                {
                    SEDX_CORE_TRACE_TAG("Renderer", "Swapchain recreated successfully with {} images", s_SwapChain->Images().size());
                }
                else
                {
                    return false;
                }
            }
            else
            {
                return false;
            }
        }

        // Wait for the fence of the current frame-in-flight BEFORE acquiring the image
        auto &fences = s_FrameSync->Fences();
        if (s_CurrentFrameIndex < fences.size() && fences[s_CurrentFrameIndex] != VK_NULL_HANDLE)
        {
            VkDevice device = RenderContext::Get()->GetDevice()->GetLogicalDevice();
            vkWaitForFences(device, 1, &fences[s_CurrentFrameIndex], VK_TRUE, UINT64_MAX);
            vkResetFences(device, 1, &fences[s_CurrentFrameIndex]);
        }

        // Acquire next swapchain image with the present semaphore for synchronization
        auto &presentSemaphores = s_FrameSync->PresentSemaphores();
        VkSemaphore acquireSemaphore = (s_CurrentFrameIndex < presentSemaphores.size()) ? presentSemaphores[s_CurrentFrameIndex] : VK_NULL_HANDLE;

        if (!s_SwapChain->AcquireNextImage(acquireSemaphore))
        {
            // Acquisition failed (minimized, out-of-date handled internally)
            return false;
        }
        s_SwapchainImageIndex = s_SwapChain->GetImageIndex();

        // Begin command buffer recording
        VkCommandBuffer cb = s_CommandBuffers[s_CurrentFrameIndex];
        if (cb != VK_NULL_HANDLE)
        {
            vkResetCommandBuffer(cb, 0);

            VkCommandBufferBeginInfo beginInfo{.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
                                               .flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT};

            if (VkResult result = vkBeginCommandBuffer(cb, &beginInfo); result != VK_SUCCESS)
            {
                SEDX_CORE_ERROR_TAG("Renderer", "vkBeginCommandBuffer failed: {}", static_cast<int>(result));
                return false;
            }
        }

        s_FrameInProgress = true;
        return true;
    }

    void Renderer::EndFrame()
    {
        if (!s_FrameInProgress)
        {
            SEDX_CORE_WARN_TAG("Renderer", "EndFrame called but no frame in progress");
            return;
        }

        VkCommandBuffer cb = s_CommandBuffers[s_CurrentFrameIndex];
        if (cb != VK_NULL_HANDLE)
        {
            // Record the actual render commands for this frame
            RecordRenderCommands(cb, s_SwapchainImageIndex);

            // Transition swapchain image to present layout
            if (s_SwapChain)
            {
                auto &swapchainImages = s_SwapChain->Images();
                if (s_SwapchainImageIndex < swapchainImages.size())
                {
                    VkImageMemoryBarrier2 barrierPresent{
                        .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2,
                        .srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
                        .srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
                        .dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
                        .dstAccessMask = 0,
                        .oldLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
                        .newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
                        .image = swapchainImages[s_SwapchainImageIndex],
                        .subresourceRange{.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT, .levelCount = 1, .layerCount = 1}};

                    VkDependencyInfo dependencyInfo{.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO,
                                                    .imageMemoryBarrierCount = 1,
                                                    .pImageMemoryBarriers = &barrierPresent};

                    vkCmdPipelineBarrier2(cb, &dependencyInfo);
                }
            }

            // End command buffer recording
            if (VkResult result = vkEndCommandBuffer(cb); result != VK_SUCCESS)
            {
                SEDX_CORE_ERROR_TAG("Renderer", "vkEndCommandBuffer failed: {}", static_cast<int>(result));
            }
        }

        s_FrameInProgress = false;
    }

    void Renderer::SubmitAndPresent()
    {
        VkCommandBuffer cb = s_CommandBuffers[s_CurrentFrameIndex];
        if (cb == VK_NULL_HANDLE || !s_FrameSync)
        {
            return;
        }

        auto& fences = s_FrameSync->Fences();
        auto& presentSemaphores = s_FrameSync->PresentSemaphores();
        auto& renderSemaphores = s_FrameSync->RenderSemaphores();

        if (renderSemaphores.empty())
        {
            SEDX_CORE_ERROR_TAG("Renderer", "Cannot submit - renderSemaphores vector is empty. Swapchain may not be initialized.");
            return;
        }

        if (s_SwapchainImageIndex >= renderSemaphores.size())
        {
            SEDX_CORE_ERROR_TAG("Renderer", "Swapchain image index {} out of bounds (renderSemaphores size: {})", s_SwapchainImageIndex, renderSemaphores.size());
            return;
        }

        // Get graphics queue from context
        VkQueue graphicsQueue = VK_NULL_HANDLE;

        Ref<Device> device = RenderContext::Get()->GetDevice();
        SEDX_CORE_VERIFY(device.IsValid(), "Device is not valid during submit");

        if (device.IsValid())
        {
            Ref<QueueManager> queueManager = device->GetQueueManager();
            SEDX_CORE_VERIFY(queueManager.IsValid(), "QueueManager is not valid during submit");

            if (queueManager.IsValid())
            {
                if (Ref<Queue> *queueRef = queueManager->GetQueue(Graphics); queueRef && *queueRef)
                {
                    graphicsQueue = (*queueRef)->GetQueue();
                }
            }
        }
    
        SEDX_CORE_VERIFY(graphicsQueue != VK_NULL_HANDLE, "No graphics queue available for submission");
        if (graphicsQueue == VK_NULL_HANDLE)
        {
            return;
        }

        // Submit command buffer
        VkPipelineStageFlags waitStages = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        VkSubmitInfo submitInfo{
            .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
            .waitSemaphoreCount = 1,
            .pWaitSemaphores = &presentSemaphores[s_CurrentFrameIndex],
            .pWaitDstStageMask = &waitStages,
            .commandBufferCount = 1,
            .pCommandBuffers = &cb,
            .signalSemaphoreCount = 1,
            .pSignalSemaphores = &renderSemaphores[s_SwapchainImageIndex]
        };

        if (VkResult submitResult = vkQueueSubmit(graphicsQueue, 1, &submitInfo, fences[s_CurrentFrameIndex]); submitResult != VK_SUCCESS)
        {
            SEDX_CORE_ERROR_TAG("Renderer", "vkQueueSubmit failed: {}", static_cast<int>(submitResult));
        }

        // Present the rendered image
        VkSwapchainKHR swapchainHandle = s_SwapChain->Get();
        VkPresentInfoKHR presentInfo{};
        presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
        presentInfo.waitSemaphoreCount = 1;
        presentInfo.pWaitSemaphores = &renderSemaphores[s_SwapchainImageIndex];
        presentInfo.swapchainCount = 1;
        presentInfo.pSwapchains = &swapchainHandle;
        presentInfo.pImageIndices = &s_SwapchainImageIndex;

        VkResult presentResult = vkQueuePresentKHR(graphicsQueue, &presentInfo);
        if (presentResult == VK_ERROR_OUT_OF_DATE_KHR || presentResult == VK_SUBOPTIMAL_KHR)
        {
            // Swapchain needs recreation (e.g., window resize)
            uint32_t queueFamily = RenderContext::Get()->GetDevice()->GetQueueManager()->GetFamilyIndexByType(Graphics);
            VmaAllocator allocator = RenderContext::Get()->GetDevice()->GetMemoryAllocator()->GetAllocator();
            s_SwapChain->Recreate(s_SwapChain->GetSurface(), queueFamily, allocator);
            SEDX_CORE_TRACE_TAG("Renderer", "Swapchain recreated after present (result: {})", static_cast<int>(presentResult));
        }
        else if (presentResult != VK_SUCCESS)
        {
            SEDX_CORE_ERROR_TAG("Renderer", "vkQueuePresentKHR failed: {}", static_cast<int>(presentResult));
        }

        // Advance to next frame-in-flight
        s_CurrentFrameIndex = (s_CurrentFrameIndex + 1) % MAX_FRAMES_IN_FLIGHT;
        s_FrameNumber++;
    }

    void Renderer::DrawFrame(CommandList *cmdList, CommandList *computeCmdList)
    {
        // This method will be called by modules to record their draw commands
        // For now, placeholder implementation
        
        VkCommandBuffer cb = s_CommandBuffers[s_CurrentFrameIndex];
        if (cb == VK_NULL_HANDLE || !s_SwapChain)
        {
            return;
        }

        // Record render commands
        RecordRenderCommands(cb, s_SwapchainImageIndex);
    }

    void Renderer::RecordRenderCommands(VkCommandBuffer cb, uint32_t imageIndex)
    {
        if (!s_SwapChain)
        {
            return;
        }

        auto& swapchainImages = s_SwapChain->Images();
        auto& swapchainImageViews = s_SwapChain->ImageViews();
        VkImageView depthImageView = s_SwapChain->GetDepthView();

        if (imageIndex >= swapchainImages.size() || imageIndex >= swapchainImageViews.size())
        {
            return;
        }

        // Transition images to attachment optimal
        std::array<VkImageMemoryBarrier2, 2> outputBarriers{
            VkImageMemoryBarrier2{
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
            VkImageMemoryBarrier2{
                .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2,
                .srcStageMask = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT,
                .srcAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,
                .dstStageMask = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT,
                .dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,
                .oldLayout = VK_IMAGE_LAYOUT_UNDEFINED,
                .newLayout = VK_IMAGE_LAYOUT_ATTACHMENT_OPTIMAL,
                .image = s_SwapChain->GetDepthImage(),
                .subresourceRange{
                    .aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT,
                    .levelCount = 1,
                    .layerCount = 1
                }
            }
        };

        VkDependencyInfo barrierDependencyInfo{};
        barrierDependencyInfo.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO;
        barrierDependencyInfo.imageMemoryBarrierCount = 2;
        barrierDependencyInfo.pImageMemoryBarriers = outputBarriers.data();

        vkCmdPipelineBarrier2(cb, &barrierDependencyInfo);

		// -----------------------------------------------------------------

        // Begin dynamic rendering
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
            .clearValue = {.depthStencil = {1.0f, 0}}
        };

        VkExtent2D extent = s_SwapChain->GetExtent();
        VkRenderingInfo renderingInfo{
            .sType = VK_STRUCTURE_TYPE_RENDERING_INFO,
            .renderArea{
                .extent{
                    .width = extent.width,
                    .height = extent.height
                }
            },
            .layerCount = 1,
            .colorAttachmentCount = 1,
            .pColorAttachments = &colorAttachmentInfo,
            .pDepthAttachment = &depthAttachmentInfo
        };

        vkCmdBeginRendering(cb, &renderingInfo);

        // Set viewport and scissor
        VkViewport vp{
            .width = static_cast<float>(extent.width),
            .height = static_cast<float>(extent.height),
            .minDepth = 0.0f,
            .maxDepth = 1.0f
        };
        vkCmdSetViewport(cb, 0, 1, &vp);

        VkRect2D scissor{
            .extent{
                .width = extent.width,
                .height = extent.height
            }
        };
        vkCmdSetScissor(cb, 0, 1, &scissor);

        // TODO: Bind pipeline and draw renderables
        // This is where module-specific draw commands would be recorded

        vkCmdEndRendering(cb);
    }

    void Renderer::CreateModels()
    {
        VmaAllocationCreateInfo bufferAllocCI;
        bufferAllocCI.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT |
                              VMA_ALLOCATION_CREATE_HOST_ACCESS_ALLOW_TRANSFER_INSTEAD_BIT |
                              VMA_ALLOCATION_CREATE_MAPPED_BIT;
        bufferAllocCI.usage = VMA_MEMORY_USAGE_AUTO;

        VmaAllocator allocator = RenderContext::Get()->GetDevice()->GetMemoryAllocator()->GetAllocator();

        // GetQueue() returns Ref<Queue>*, so we need to get the pointer first
        Ref<Queue> *queuePtr = RenderContext::Get()->GetDevice()->GetQueueManager()->GetQueue(Graphics);
        SEDX_CORE_ASSERT(queuePtr && *queuePtr, "Graphics queue not available");
        // DIAGNOSTIC: Log current working directory
        std::filesystem::path cwd = std::filesystem::current_path();
        SEDX_CORE_ERROR_TAG("Renderer", "Current working directory: {}", cwd.string());

        // DIAGNOSTIC: Check if model file exists
        std::filesystem::path modelPath = "resources/models/suzanne.obj";
        SEDX_CORE_TRACE_TAG("Renderer", "Looking for model at: {}", std::filesystem::absolute(modelPath).string());
        SEDX_CORE_TRACE_TAG("Renderer", "Model file exists: {}", std::filesystem::exists(modelPath));

        std::vector<std::string> texFiles = {"resources/textures/suzanne0.ktx",
                                             "resources/textures/suzanne1.ktx",
                                             "resources/textures/suzanne2.ktx"};

        // Only proceed if file exists
        if (!std::filesystem::exists(modelPath))
        {
            SEDX_CORE_ERROR_TAG("Renderer", "Model file not found at expected path: {}", std::filesystem::absolute(modelPath).string());
            return; // Skip model loading instead of asserting
        }

        // Dereference the pointer to access the Ref, then call GetQueue()
        SEDX_CORE_ASSERT(s_AssetManager->AddAsset(allocator, s_CommandPool->GetPool(), (*queuePtr)->GetQueue(), modelPath.string(), texFiles, bufferAllocCI));

        const Asset &asset = s_AssetManager->GetAsset(0);
        VkBuffer vBuffer = asset.GetModelBuffer();
        VkDeviceSize vBufSize = asset.GetModelVertexSize();
        VkDeviceSize iBufSize = asset.GetModelIndexSize();
        VkDeviceSize indexCount = asset.GetModelIndexCount();

        // Create uniform buffers as static resource (per-frame) managed by UniformBufferSet RAII helper
        //s_UniformBuffers = CreateScope<UniformBufferSet>(allocator);
    }

    void Renderer::CreateShaders()
    {
        // Initialize Slang shader compiler
        Slang::ComPtr<slang::IGlobalSession> slangGlobalSession;
        slang::createGlobalSession(slangGlobalSession.writeRef());
        auto slangTargets{
            std::to_array<slang::TargetDesc>({{
                .format = SLANG_SPIRV, 
                .profile = slangGlobalSession->findProfile("spirv_1_4")
            }})
        };
        auto slangOptions{
            std::to_array<slang::CompilerOptionEntry>({{
                .name = slang::CompilerOptionName::EmitSpirvDirectly,
				.value = {.kind = slang::CompilerOptionValueKind::Int, .intValue0 = 1}}})
        };

        slang::SessionDesc slangSessionDesc = {};
        slangSessionDesc.targets = slangTargets.data();
        slangSessionDesc.targetCount = static_cast<SlangInt>(slangTargets.size());
        slangSessionDesc.defaultMatrixLayoutMode = SLANG_MATRIX_LAYOUT_COLUMN_MAJOR;
        slangSessionDesc.compilerOptionEntries = slangOptions.data();
        slangSessionDesc.compilerOptionEntryCount = static_cast<uint32_t>(slangOptions.size());

        // Load shader
        Slang::ComPtr<slang::ISession> slangSession;
        Slang::ComPtr<slang::IBlob> diagnosticsBlob; // Blob to capture any diagnostics from shader compilation
        slangGlobalSession->createSession(slangSessionDesc, slangSession.writeRef());
        Slang::ComPtr<slang::IModule> slangModule{slangSession->loadModuleFromSource("triangle", "resources/shaders/shader.slang", diagnosticsBlob, diagnosticsBlob.writeRef())};
		if (!slangModule)
		{
		    SEDX_CORE_ERROR_TAG("Renderer", "Failed to load shader module from source: resources/shaders/shader.slang");
		    if (diagnosticsBlob)
		    {
		        const char* errorMessage = static_cast<const char*>(diagnosticsBlob->getBufferPointer());
		        SEDX_CORE_ERROR_TAG("Renderer", "Slang diagnostics: {}", errorMessage);
		    }
		    else
		    {
		        SEDX_CORE_ERROR_TAG("Renderer", "No diagnostics available from Slang.");
		    }
		    return;
		}
        Slang::ComPtr<ISlangBlob> spirv;
        slangModule->getTargetCode(0, spirv.writeRef());

        // Create ShaderManager owning shader modules for the pipeline stages.
        //ShaderManager shaderManager(spirv->getBufferPointer(), spirv->getBufferSize());

    }

    // -------------------------------------------------------
    // Accessors
    // -------------------------------------------------------

    Ref<RenderContext> Renderer::GetContext()
    {
        return RenderContext::Get();
    }

    uint32_t Renderer::GetCurrentFrameIndex()
    {
        return s_CurrentFrameIndex;
    }

    uint64_t Renderer::GetFrameNumber()
    {
        return s_FrameNumber;
    }

    Swapchain* Renderer::GetSwapChain()
    {
        return s_SwapChain.Get();
    }

    uint32_t Renderer::GetSwapchainImageIndex()
    {
        return s_SwapchainImageIndex;
    }

    VkCommandBuffer Renderer::GetCurrentCommandBuffer()
    {
        return s_CommandBuffers[s_CurrentFrameIndex];
    }

    const Viewport& Renderer::GetViewport()
    {
        return s_Viewport;
    }

    void Renderer::SetViewport(float width, float height)
    {
        constexpr float epsilon = 1e-5f;

        // Check if absolute value is greater than epsilon (instead of != 0)
        SEDX_CORE_ASSERT(std::abs(width) > epsilon, "Width can't be zero");
        SEDX_CORE_ASSERT(std::abs(height) > epsilon, "Height can't be zero");

		// Check if the difference is greater than epsilon (instead of !=)
		if (std::abs(s_Viewport.width - width) > epsilon ||  std::abs(s_Viewport.height - height) > epsilon)
		{
		    s_Viewport.width = width;
		    s_Viewport.height = height;
		    s_OrthoProjection_Dirty = true;
		}
    }

    const Vec2& Renderer::GetRendererResolution()
    {
        return s_RendererResolution;
    }

    void Renderer::SetRendererResolution(uint32_t width, uint32_t height, const bool recreateResources)
    {
        
       // Check if the difference is smaller than epsilon (safe ==)
       if (constexpr float epsilon = 1e-5f;
           std::abs(s_RendererResolution.x - static_cast<float>(width)) < epsilon && 
           std::abs(s_RendererResolution.y - static_cast<float>(height)) < epsilon)
           return;

        s_RendererResolution.x = static_cast<float>(width);
        s_RendererResolution.y = static_cast<float>(height);

        if (recreateResources && s_ResourcesInitialized)
        {
            // Wait for GPU to finish before recreating resources
            if (RenderContext::Get()->GetDevice())
            {
                RenderContext::Get()->GetDevice()->GetQueueManager()->WaitIdleAll();
            }

            CreateRenderTargets(true, false, true);
        }

        SEDX_CORE_TRACE_TAG("Renderer", "Render resolution set to {}x{}", width, height);
    }

    const Vec2& Renderer::GetOutputResolution()
    {
        return s_OutputResolution;
    }

    void Renderer::SetOutputResolution(uint32_t width, uint32_t height, bool recreateResources)
    {

        // Check if the difference is smaller than epsilon (safe ==)
        if (constexpr float epsilon = 1e-5f; std::abs(s_OutputResolution.x - static_cast<float>(width)) < epsilon && 
            std::abs(s_OutputResolution.y - static_cast<float>(height)) < epsilon)
        {
            return;
        }

        s_OutputResolution.x = static_cast<float>(width);
        s_OutputResolution.y = static_cast<float>(height);

        if (recreateResources && s_ResourcesInitialized)
        {
            CreateRenderTargets(false, true, false);
        }

        SEDX_CORE_TRACE_TAG("Renderer", "Output resolution set to {}x{}", width, height);
    }

    void Renderer::CreateRenderTargets(const bool createRender, const bool createOutput, const bool createDynamic)
    {
        uint32_t renderWidth = static_cast<uint32_t>(GetRendererResolution().x);
        uint32_t renderHeight = static_cast<uint32_t>(GetRendererResolution().y);
        uint32_t outputWidth = static_cast<uint32_t>(GetOutputResolution().x);
        uint32_t outputHeight = static_cast<uint32_t>(GetOutputResolution().y);

        auto compute_mip_count = [](const uint32_t width, const uint32_t height, const uint32_t smallestDimension) 
        {
            uint32_t maxDimension = std::max(width, height);
            uint32_t mipCount = 1;

            while (maxDimension >= smallestDimension)
            {
                maxDimension /= 2;
                mipCount++;
            }
            return mipCount;
        };

        if (createRender)
        {
            // TODO: Create render targets
        }

        if (createOutput)
        {
            // TODO: Create output targets
        }
    }

} // namespace SceneryEditorX

// --------------------------------------------------------------
