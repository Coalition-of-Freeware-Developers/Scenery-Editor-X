/**
* -------------------------------------------------------
* Scenery Editor X
* -------------------------------------------------------
* Copyright (c) 2025 Thomas Ray 
* Copyright (c) 2025 Coalition of Freeware Developers
* -------------------------------------------------------
* frame_sync.h
* -------------------------------------------------------
* Created: 28/8/2025
* -------------------------------------------------------
*/
#pragma once
#include "fence.h"
#include "semaphore.h"
#include "vulkan/vk_enums.h"
#include <vulkan/vulkan.h>

// -------------------------------------------------------

namespace SceneryEditorX
{
    /**
     * @class FrameSync
     * @brief Comprehensive synchronization primitive manager for a single frame in flight
     * 
     * Encapsulates all synchronization primitives (semaphores and fences) required for 
     * proper GPU-CPU synchronization in a multi-buffered rendering setup. Provides a 
     * unified interface for frame synchronization operations.
     * 
     * Key Features:
     *  - Manages image acquisition semaphore (GPU waits for swapchain image)
     *  - Manages render finished semaphore (presentation waits for rendering)
     *  - Manages in-flight fence (CPU waits for GPU frame completion)
     *  - Optional timeline semaphore support for advanced synchronization
     *  - Thread-safe operations with proper Vulkan synchronization
     * 
     * Usage Pattern:
     *  1. Create FrameSync for each frame in flight (typically 2-3 frames)
     *  2. WaitForFence() - CPU waits for previous frame to complete
     *  3. ResetFence() - After successful image acquisition
     *  4. GetImageAvailableSemaphore() - Use in vkAcquireNextImageKHR
     *  5. GetRenderFinishedSemaphore() - Use in queue submit and present
     *  6. GetInFlightFence() - Use in queue submit to signal frame completion
     * 
     * @note This class is designed to work with Vulkan's multi-buffering model
     *       where multiple frames can be "in-flight" (being processed) simultaneously.
     */
    class FrameSync : public RefCounted
	{
	public:
        /**
         * @brief Construct a FrameSync object with specified synchronization primitives
         * @param type Type of primary synchronization (fence, semaphore, or timeline semaphore)
         * @param debugName Human-readable name for debugging and profiling
         * @param createImageSemaphore Whether to create image available semaphore (default: true)
         * @param createRenderSemaphore Whether to create render finished semaphore (default: true)
         * @param createFence Whether to create in-flight fence (default: true)
         */
        FrameSync(FrameSyncType type, const std::string& debugName, bool createImageSemaphore = true, bool createRenderSemaphore = true, bool createFence = true);
        virtual ~FrameSync() override;

        /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
        /// Fence Operations (CPU-GPU Synchronization)
        /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
        
        /**
         * @brief Wait for the in-flight fence to be signaled (CPU waits for GPU)
         * @param timeout Timeout in nanoseconds (UINT64_MAX = wait indefinitely)
         */
		void WaitForFence(uint64_t timeout = UINT64_MAX);
        
        /**
         * @brief Check if the in-flight fence is currently signaled
         * @return True if fence is signaled (frame is complete), false otherwise
         */
        bool IsFenceSignaled();
        
        /**
         * @brief Reset the in-flight fence to unsignaled state
         * @note Must be called after WaitForFence() and before submitting new work
         */
        void ResetFence();

        /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
        /// Semaphore Operations (GPU-GPU Synchronization)
        /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
        
        /**
         * @brief Wait for a timeline semaphore value (if using timeline semaphores)
         * @param value Signal value to wait for
         * @param timeout Timeout in nanoseconds (UINT64_MAX = wait indefinitely)
         */
		void WaitForTimelineValue(uint64_t value, uint64_t timeout = UINT64_MAX);

        void SignalTimeline(uint64_t value);
        void SignalNextFrame();

        VkSemaphore GetImageAvailableSemaphore() const;
        VkSemaphore GetRenderFinishedSemaphore() const;
        VkSemaphore GetTimelineSemaphore() const;
        VkFence GetInFlightFence() const;

        /**
         * @brief Get the primary resource handle (for backward compatibility)
         * @return Void pointer to primary synchronization resource
         */
        void* GetResource() { return m_Resource; }

        /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
        /// State Queries
        /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

		uint64_t GetNextSignalValue() { return ++m_SignalValue; }

        uint64_t GetCurrentSignalValue() const { return m_SignalValue; }

        FrameSyncType GetType() const { return m_Type; }

        const std::string& GetDebugName() const { return m_DebugName; }

        /**
         * @brief Check if all requested synchronization primitives were created successfully
         * @return True if valid, false if creation failed
         */
        bool IsValid() const;

    private:

		void *m_Resource = nullptr;						// Primary synchronization resource (type-dependent)
		uint64_t m_SignalValue = 0;						// Current signal value for timeline semaphores
        FrameSyncType m_Type = FrameSyncType::MaxEnum;	// Type of primary synchronization primitive
        std::string m_DebugName;						// Debug name for identification and profiling

        Ref<Semaphore> m_TimelineSemaphore;				// Timeline semaphore for advanced synchronization patterns
        Ref<Semaphore> m_ImageAvailableSemaphore;		// Binary semaphore signaled when swapchain image is available
        Ref<Semaphore> m_RenderFinishedSemaphore;		// Binary semaphore signaled when rendering to image is complete
        Ref<Fence> m_InFlightFence;						// Fence for CPU-side synchronization (wait for frame completion)

        // Validation flags for created primitives
        bool m_HasImageSemaphore = false;
        bool m_HasRenderSemaphore = false;
        bool m_HasFence = false;
	};

}

// -------------------------------------------------------
