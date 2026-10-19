# Bug description:
• SceneryEditorX starts, but window stays black.
• No UI from SceneryEditorX::UILayer.
• Forced swapchain clear path does not produce visible output.
• Validation errors show sync/layout problems:
        • vkCmdPipelineBarrier2 from UNDEFINED -> SHADER_READ_ONLY_OPTIMAL
        • SubmitAndPresent called while a frame is still in progress
        • vkQueuePresentKHR ... acquired semaphore ... not waited on
        • vkAcquireNextImageKHR ... Semaphore must not be currently signaled

Project/app in scope: SceneryEditorX desktop editor app, specifically source/SceneryEditorX/renderer/* and renderer/vulkan/*.

## Repro-step analysis
1. Action: App enters render loop early (OnRender -> BeginFrame)
        • Likely code areas:
                • source/SceneryEditorX/renderer/renderer.cpp (Renderer::BeginFrame path; warning source)
                • app render loop caller (Application/Editor layer render path)
        • Key findings:
                • Logs repeatedly show BeginFrame called but renderer not initialized.
                • Renderer resource init is async (ThreadPool::Submit(...)) while render loop continues.
        • Failures/hypotheses:
                • Render loop allowed before renderer ready flag is set.
                • Initialization state transitions are non-atomic or checked inconsistently.
2. Action: Renderer initialization continues while frames are attempted
        • Likely code areas:
                • renderer.cpp Init(), async resource block, CreateCameraResources, shader/pipeline setup
                • ui_layer.cpp ImGui backend setup timing
        • Key findings:
                • m_ResourcesInitialized set false inside async job, then true at end of Init() too.
                • CreateCameraResources appears later in logs, interleaved with frame attempts.
        • Failures/hypotheses:
                • Initialization readiness is declared too early (or toggled incorrectly), causing partial frame lifecycle execution.
                • UI renderer depends on resources not yet fully valid.
3. Action: Frame lifecycle execution (BeginFrame/Record/Submit/EndFrame)
        • Likely code areas:
                • renderer.cpp BeginFrame, SubmitAndPresent, EndFrame, RecordRenderCommands
        • Key findings:
                • Explicit error: SubmitAndPresent called while a frame is still in progress (EndFrame must be called before submit).
        • Failures/hypotheses:
                • Call-order bug in render loop or re-entrant submit path.
                • Guard/state machine around m_FrameInProgress is violated by UI/render threading.
4. Action: Swapchain acquire/present synchronization
        • Likely code areas:
                • renderer/vulkan/swapchain.cpp (AcquireNextImage, Present, semaphore indexing)
                • renderer.cpp submit wait/signal semaphore selection
        • Key findings:
                • Present validation: acquired semaphore not waited on.
                • Next acquire validation: semaphore still signaled.
        • Failures/hypotheses:
                • Acquire semaphore used in vkAcquireNextImageKHR is not the same one waited in vkQueueSubmit.
                • Per-image and per-frame semaphore indexing are mixed (m_LastAcquiredSemaphoreIndex vs frame index vs image index).
5. Action: Forced clear test and image layout transitions
        • Likely code areas:
                • renderer.cpp forced clear block (lines ~2522+)
                • post-clear transition to present
        • Key findings:
                • Forced clear uses oldLayout = UNDEFINED every frame.
                • Another validation warning shows invalid layout/access assumptions elsewhere.
        • Failures/hypotheses:
                • Transition assumptions are wrong for already-presented swapchain images; can invalidate synchronization correctness even if clear runs.

## Coverage check
        • Strongest hypotheses: Steps 3 and 4 (direct runtime errors + Vulkan validation failures).
        • Moderate: Step 1/2 (startup race and readiness gating).
        • Weakest: Step 5 alone (layout misuse is likely contributory, but semaphore misuse is more decisive for black screen/hang behavior).

## Top hypotheses (cross-step)
1. Primary hypothesis: Acquire/wait/signal semaphore mismatch between Swapchain::AcquireNextImage, renderer submit, and Present causes invalid present/acquire cycle.
2. Render frame state machine is out of order (SubmitAndPresent before EndFrame), causing synchronization contract breakage.
3. Renderer readiness flagging allows frame execution before initialization is complete.
4. Forced clear and/or transition path uses incorrect old layouts, compounding validation issues and visibility.
5. UI frame execution occurs when graphics resources are not fully created, so no drawable output path is active.

## Problem summary
This is most likely a frame lifecycle + semaphore ownership mismatch issue in the Vulkan present pipeline, with a secondary initialization/race gating problem causing repeated invalid early frames.

## Primary hypothesis
The semaphore returned/used by vkAcquireNextImageKHR is not consistently the semaphore waited by submit before present, leading to:
• present on an image without proper wait,
• then acquire reusing a still-signaled semaphore,
• resulting in black output and eventual timeout/assert path.

## Repro steps
1. Launch Debug build.
2. Let editor start rendering immediately.
3. Observe repeated BeginFrame called but renderer not initialized.
4. Observe SubmitAndPresent while frame in progress, then semaphore validation errors.
5. Screen remains black/no UI.
