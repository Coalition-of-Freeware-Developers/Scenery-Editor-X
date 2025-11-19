# Integration Flow & Usage Patterns

Utilizing RenderDispatcher, CommandManager, ThreadCommandPools, Renderer, and the RenderContext classes.

---

## Initialization Sequence

```cpp
// Application startup
void Application::InitializeRenderer()
{
    SEDX_CORE_INFO_TAG("INIT", "=== Initializing Renderer ===");
  
    // Step 1: Initialize RenderContext (creates Vulkan instance, device)
    auto renderContext = RenderContext::Get(); // Singleton creation
    renderContext->Init();
  
    // Step 2: Initialize RenderDispatcher (spawns worker thread)
    RenderDispatcher::Init(); // Must be called before async submissions
  
    // Step 3: Initialize Renderer (creates swapchain, pipelines, resources)
    Renderer::Init(); // Uses RenderContext internally
  
    SEDX_CORE_INFO_TAG("INIT", "✓ Renderer initialization complete");
}
```

## Per-Frame Execution Flow

```cpp
void Application::OnUpdate()
{
    SEDX_PROFILE_SCOPE("Application::OnUpdate");
  
    // ========== CPU/Game Thread Work ==========
  
    // Step 1: Begin frame (acquire swapchain image, reset per-frame resources)
    Renderer::BeginFrame();
  
    // Step 2: Submit async render preparation work
    Renderer::Submit([this]()
    {
        SEDX_PROFILE_SCOPE("Async Render Prep");
      
        // This executes on RenderDispatcher worker thread
        PrepareSceneData();
        UpdateMaterialBuffers();
        CullVisibleObjects();
    });
  
    // Step 3: Record rendering commands (can be multi-threaded)
    std::vector<std::thread> recordingThreads;
    for (uint32_t i = 0; i < numThreads; ++i)
    {
        recordingThreads.emplace_back([this, i]()
        {
            RecordRenderCommandsForChunk(i);
        });
    }
  
    for (auto& thread : recordingThreads)
        thread.join();
  
    // Step 4: End frame (submit command buffers, present)
    Renderer::EndFrame();
  
    // Step 5: Advance resource free ring (execute safe deletions)
    RenderDispatcher::NextFrame(Renderer::GetCurrentFrameIndex());
}
```

## Multi-threaded Command Recording Pattern

```cpp
void Application::RecordRenderCommandsForChunk(uint32_t chunkIndex)
{
    SEDX_PROFILE_SCOPE("Record Chunk");
  
    // Step 1: Get thread-local command pool (automatically creates if first use)
    auto device = RenderContext::GetCurrentDevice();
    auto cmdPool = ThreadCommandPools::Get(device, Queue::Graphics);
  
    // Step 2: Create CommandManager for this recording session
    auto cmdManager = CreateRef<CommandManager>(
        device->GetQueue(Queue::Graphics),
        cmdPool->GetCmdPool(),
        fmt::format("ChunkRecorder_{}", chunkIndex)
    );
  
    // Step 3: Begin recording
    cmdManager->Begin();
    cmdManager->BeginTimeBlock("RenderChunk", true, true);
  
    // Step 4: Record draw calls
    for (const auto& drawable : GetDrawablesForChunk(chunkIndex))
    {
        cmdManager->SetVertexBuffer(drawable->GetVertexBuffer().Get());
        cmdManager->SetIndexBuffer(drawable->GetIndexBuffer().Get());
        cmdManager->PushConstants(drawable->GetPushConstants());
        cmdManager->DrawIndexed(drawable->GetIndexCount());
    }
  
    cmdManager->EndTimeBlock();
  
    // Step 5: Submit to GPU (with synchronization)
    FrameSync* waitSemaphore = GetPreviousFrameSemaphore();
    cmdManager->Submit(waitSemaphore, false);
  
    // Step 6: Store completion semaphore for next frame dependency
    StoreChunkCompletionSemaphore(chunkIndex, cmdManager->GetRenderingCompleteSemaphore());
}
```

## Resource Deletion with Frame Safety

```cpp
void AssetManager::UnloadTexture(Ref<Texture2D> texture)
{
    SEDX_CORE_INFO_TAG("ASSET", "Scheduling texture deletion: {}", texture->GetName());
  
    // Capture texture by value (ref count keeps it alive)
    Renderer::SubmitResourceFree([texture]()
    {
        SEDX_PROFILE_SCOPE("Texture Destruction");
      
        // This executes N frames later (when GPU is done using it)
        VkDevice device = RenderContext::GetCurrentDevice()->GetDevice();
      
        if (texture->GetImageView() != VK_NULL_HANDLE)
        {
            vkDestroyImageView(device, texture->GetImageView(), nullptr);
        }
      
        if (texture->GetImage() != VK_NULL_HANDLE)
        {
            vkDestroyImage(device, texture->GetImage(), nullptr);
        }
      
        if (texture->GetMemory() != VK_NULL_HANDLE)
        {
            vkFreeMemory(device, texture->GetMemory(), nullptr);
        }
      
        SEDX_CORE_INFO_TAG("ASSET", "✓ Texture destroyed: {}", texture->GetName());
      
        // texture goes out of scope here, ref count decrements
    });
}
```

## Render Pass Workflow

```cpp
void SceneRenderer::RenderScene(Ref<Scene> scene, Ref<Camera> camera)
{
    SEDX_PROFILE_SCOPE("SceneRenderer::RenderScene");
  
    // Get main graphics command manager
    auto cmdManager = GetGraphicsCommandManager();
  
    // ===== Shadow Pass =====
    cmdManager->BeginTimeBlock("ShadowPass");
    {
        auto shadowPipeline = GetShadowPipeline();
        cmdManager->PipelineState(); // Binds active pipeline
      
        cmdManager->SetViewport(m_ShadowViewport);
        cmdManager->SetCullMode(CullMode::Front); // Reverse culling for shadows
      
        for (const auto& mesh : scene->GetVisibleMeshes())
        {
            cmdManager->SetVertexBuffer(mesh->GetVertexBuffer().Get());
            cmdManager->SetIndexBuffer(mesh->GetIndexBuffer().Get());
            cmdManager->DrawIndexed(mesh->GetIndexCount());
        }
      
        cmdManager->RenderPassEnd();
    }
    cmdManager->EndTimeBlock();
  
    // ===== Depth Pre-Pass =====
    cmdManager->BeginTimeBlock("DepthPrePass");
    {
        cmdManager->SetCullMode(CullMode::Back);
        cmdManager->SetViewport(m_MainViewport);
      
        // Record depth-only draws...
      
        cmdManager->RenderPassEnd();
    }
    cmdManager->EndTimeBlock();
  
    // ===== Main Geometry Pass =====
    cmdManager->BeginTimeBlock("GeometryPass");
    {
        auto geometryPipeline = GetGeometryPipeline();
      
        // Insert barrier for depth texture read
        cmdManager->InsertBarrier(
            m_DepthTexture->GetImage(),
            VK_FORMAT_D32_SFLOAT,
            0, 1, 1,
            Layout::ImageLayout::DepthStencilReadOnlyOptimal
        );
      
        // Bind resources
        cmdManager->SetConstantBuffer(0, m_SceneDataBuffer.Get());
        cmdManager->SetConstantBuffer(1, camera->GetCameraBuffer().Get());
      
        for (const auto& mesh : scene->GetVisibleMeshes())
        {
            // Bind per-draw resources
            cmdManager->PushConstants(mesh->GetTransform());
            cmdManager->SetVertexBuffer(mesh->GetVertexBuffer().Get());
            cmdManager->SetIndexBuffer(mesh->GetIndexBuffer().Get());
          
            cmdManager->DrawIndexed(mesh->GetIndexCount());
        }
      
        cmdManager->RenderPassEnd();
    }
    cmdManager->EndTimeBlock();
  
    // ===== Post-Processing Compute Pass =====
    cmdManager->BeginTimeBlock("PostProcess");
    {
        // Transition render target for compute shader read
        cmdManager->InsertBarrier(
            m_GeometryRT->GetImage(),
            VK_FORMAT_R16G16B16A16_SFLOAT,
            0, 1, 1,
            Layout::ImageLayout::General
        );
      
        cmdManager->InsertPendingBarrierGroup(); // Execute all pending barriers
      
        // Dispatch post-process compute shader
        cmdManager->Dispatch(
            (m_RenderWidth + 15) / 16,
            (m_RenderHeight + 15) / 16,
            1
        );
    }
    cmdManager->EndTimeBlock();
}
```

## Shutdown Sequence

```cpp
void Application::ShutdownRenderer()
{
    SEDX_CORE_INFO_TAG("SHUTDOWN", "=== Shutting Down Renderer ===");
  
    // Step 1: Flush all pending async work
    RenderDispatcher::Flush();
    SEDX_CORE_INFO_TAG("SHUTDOWN", "✓ Async work flushed");
  
    // Step 2: Wait for GPU to finish all submitted work
    auto device = RenderContext::GetCurrentDevice();
    vkDeviceWaitIdle(device->GetDevice());
    SEDX_CORE_INFO_TAG("SHUTDOWN", "✓ GPU idle");
  
    // Step 3: Shutdown renderer (destroys pipelines, swapchain, etc.)
    Renderer::Shutdown();
    SEDX_CORE_INFO_TAG("SHUTDOWN", "✓ Renderer resources destroyed");
  
    // Step 4: Shutdown thread command pools
    ThreadCommandPools::Shutdown();
    SEDX_CORE_INFO_TAG("SHUTDOWN", "✓ Thread command pools destroyed");
  
    // Step 5: Shutdown render dispatcher (executes all deferred frees, joins worker)
    RenderDispatcher::Shutdown();
    SEDX_CORE_INFO_TAG("SHUTDOWN", "✓ Render dispatcher shutdown");
  
    // Step 6: RenderContext cleanup (device, instance destruction)
    // Happens automatically when RenderContext ref count reaches 0
  
    SEDX_CORE_INFO_TAG("SHUTDOWN", "✓ Renderer shutdown complete");
}
```



---



## Key Integration Points

### RenderContext ↔ Everything

•	Singleton providing Vulkan device access to all subsystems
•	RenderContext::Get() used by Renderer, ThreadCommandPools, CommandManager
•	Owns the fundamental Vulkan objects (instance, physical device, logical device)

### Renderer ↔ RenderDispatcher

•	Renderer::Submit() is a thin wrapper around RenderDispatcher::Enqueue()
•	Renderer::SubmitResourceFree() wraps RenderDispatcher::EnqueueResourceFree()
•	Renderer::BeginFrame()/EndFrame() calls RenderDispatcher::NextFrame()

### ThreadCommandPools ↔ CommandManager

•	Each thread gets its own CommandPool per queue type (Graphics/Compute/Transfer)
•	CommandManager constructor receives a VkCommandPool from ThreadCommandPools::Get()
•	Enables safe multi-threaded command buffer recording

### CommandManager ↔ GPU Resources

•	Primary interface for recording Vulkan commands
•	Manages render passes, barriers, queries, and draw calls
•	Uses FrameSync for inter-command buffer synchronization

### RenderDispatcher ↔ Resource Lifetime

•	The resource free ring ensures GPU resources aren't destroyed while in use
•	NextFrame() advances the ring and executes safe bucket
•	Frame count = framesInFlight (typically 2-3)

# Threading Model Summary

| Thread                  | Responsibilities                                | Key Classes Used                   |
| ----------------------- | ----------------------------------------------- | ---------------------------------- |
| Main/Game Thread        | Frame orchestration, module updates, user input | Renderer, RenderContext            |
| RenderDispatcher Worker | Async render prep (culling, buffer updates)     | RenderDispatcher, JobQueue         |
| Recording Threads (N)   | Parallel command buffer recording               | ThreadCommandPools, CommandManager |
| GPU                     | Command buffer execution, rendering             | All Vulkan resources               |

---
