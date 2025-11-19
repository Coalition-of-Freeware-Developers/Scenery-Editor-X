/**
* -------------------------------------------------------
* Scenery Editor X
* -------------------------------------------------------
* Copyright (c) 2025 Thomas Ray 
* Copyright (c) 2025 Coalition of Freeware Developers
* -------------------------------------------------------
* frame_sync.cpp
* -------------------------------------------------------
* Created: 28/8/2025
* -------------------------------------------------------
*/
#include "frame_sync.h"
#include "fence.h"
#include "render_context.h"
#include "semaphore.h"
#include "SceneryEditorX/logging/logging.hpp"
#include "SceneryEditorX/logging/profiler.hpp"

// -------------------------------------------------------

namespace SceneryEditorX
{
	FrameSync::FrameSync(const FrameSyncType type,const std::string& debugName,
        bool createImageSemaphore,bool createRenderSemaphore,bool createFence) : m_Type(type), m_DebugName(debugName)
	{
        SEDX_CORE_INFO_TAG("FRAME-SYNC", "Creating FrameSync: {} (Type: {})", debugName, static_cast<int>(type));

        // Create primary resource based on type
		if (type == FrameSyncType::SyncFence)
        {
            Fence::Create(m_Resource);
            SEDX_CORE_INFO_TAG("FRAME-SYNC", "✓ Primary fence created for {}", debugName);
        }
		else if (type == FrameSyncType::SyncSemaphore)
        {
            Semaphore::Create(FrameSyncType::SyncSemaphore, m_Resource);
            SEDX_CORE_INFO_TAG("FRAME-SYNC", "✓ Primary binary semaphore created for {}", debugName);
        }
        else if (type == FrameSyncType::SyncSemaphoreTimeline)
        {
            void* timelineResource = nullptr;
            Semaphore::Create(FrameSyncType::SyncSemaphoreTimeline, timelineResource);
            m_TimelineSemaphore = CreateRef<Semaphore>(FrameSyncType::SyncSemaphoreTimeline, timelineResource);
            m_Resource = timelineResource;
            SEDX_CORE_INFO_TAG("FRAME-SYNC", "✓ Timeline semaphore created for {}", debugName);
        }

        // Create image available semaphore (for swapchain image acquisition)
        if (createImageSemaphore)
        {
            void* imageAvailableResource = nullptr;
            Semaphore::Create(FrameSyncType::SyncSemaphore, imageAvailableResource);
            m_ImageAvailableSemaphore = CreateRef<Semaphore>(FrameSyncType::SyncSemaphore, imageAvailableResource);
            m_HasImageSemaphore = true;
            SEDX_CORE_INFO_TAG("FRAME-SYNC", "✓ Image available semaphore created for {}", debugName);
        }

        // Create render finished semaphore (for presentation synchronization)
        if (createRenderSemaphore)
        {
            void* renderFinishedResource = nullptr;
            Semaphore::Create(FrameSyncType::SyncSemaphore, renderFinishedResource);
            m_RenderFinishedSemaphore = CreateRef<Semaphore>(FrameSyncType::SyncSemaphore, renderFinishedResource);
            m_HasRenderSemaphore = true;
            SEDX_CORE_INFO_TAG("FRAME-SYNC", "✓ Render finished semaphore created for {}", debugName);
        }

        // Create in-flight fence (for CPU-GPU synchronization)
        if (createFence)
        {
            void* fenceResource = nullptr;
            Fence::Create(fenceResource);
            m_InFlightFence = CreateRef<Fence>(fenceResource);
            m_HasFence = true;
            SEDX_CORE_INFO_TAG("FRAME-SYNC", "✓ In-flight fence created for {}", debugName);
        }

        SEDX_CORE_INFO_TAG("FRAME-SYNC", "✓ FrameSync {} fully initialized", debugName);
	}
	
	FrameSync::~FrameSync()
	{
        SEDX_CORE_INFO_TAG("FRAME-SYNC", "Destroying FrameSync: {}", m_DebugName);

        // Smart pointers will handle cleanup automatically
        m_TimelineSemaphore.Reset();
        m_ImageAvailableSemaphore.Reset();
        m_RenderFinishedSemaphore.Reset();
        m_InFlightFence.Reset();

        if (m_Resource)
            m_Resource = nullptr;

        SEDX_CORE_INFO_TAG("FRAME-SYNC", "✓ FrameSync {} destroyed", m_DebugName);
	}

    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// Fence Operations
    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	
	void FrameSync::WaitForFence(const uint64_t timeout)
	{
        SEDX_CORE_ASSERT(m_InFlightFence != nullptr, "In-flight fence not created for FrameSync: {}", m_DebugName);
        SEDX_PROFILE_SCOPE("FrameSync::WaitForFence");

        void* fenceResource = m_InFlightFence->GetRhiResource();
        Fence::WaitTime(timeout, fenceResource);
	}

    bool FrameSync::IsFenceSignaled()
    {
        SEDX_CORE_ASSERT(m_InFlightFence != nullptr, "In-flight fence not created for FrameSync: {}", m_DebugName);

        void* fenceResource = m_InFlightFence->GetRhiResource();
        return Fence::IsSignaled(fenceResource);
    }

    void FrameSync::ResetFence()
    {
        SEDX_CORE_ASSERT(m_InFlightFence != nullptr, "In-flight fence not created for FrameSync: {}", m_DebugName);

        void* fenceResource = m_InFlightFence->GetRhiResource();
        Fence::Reset(fenceResource);
    }

    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// Timeline Semaphore Operations
    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	
	void FrameSync::WaitForTimelineValue(const uint64_t value, const uint64_t timeout)
	{
        SEDX_CORE_ASSERT(m_Type == FrameSyncType::SyncSemaphoreTimeline, "WaitForTimelineValue can only be called on timeline semaphores");
        SEDX_CORE_ASSERT(m_TimelineSemaphore != nullptr, "Timeline semaphore not created");
        SEDX_PROFILE_SCOPE("FrameSync::WaitForTimelineValue")

        void* semaphoreResource = m_TimelineSemaphore->GetResource();
        Semaphore::WaitTime(value, timeout, semaphoreResource);
	}
	
	void FrameSync::SignalTimeline(const uint64_t value)
	{
        SEDX_CORE_ASSERT(m_Type == FrameSyncType::SyncSemaphoreTimeline, "SignalTimeline can only be called on timeline semaphores");
        SEDX_CORE_ASSERT(m_TimelineSemaphore != nullptr, "Timeline semaphore not created");

        void* semaphoreResource = m_TimelineSemaphore->GetResource();
        Semaphore::Signal(value, semaphoreResource);
        m_SignalValue = value;
	}

    void FrameSync::SignalNextFrame()
    {
        SEDX_CORE_ASSERT(m_Type == FrameSyncType::SyncSemaphoreTimeline, "SignalNextFrame can only be called on timeline semaphores");
        SEDX_CORE_ASSERT(m_TimelineSemaphore != nullptr, "Timeline semaphore not created");

        m_SignalValue++;
        void* semaphoreResource = m_TimelineSemaphore->GetResource();
        Semaphore::Signal(m_SignalValue, semaphoreResource);

        SEDX_CORE_INFO_TAG("FRAME-SYNC", "Timeline {} signaled with value {}", m_DebugName, m_SignalValue);
    }

    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// Resource Accessors
    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

    VkSemaphore FrameSync::GetImageAvailableSemaphore() const
    {
        if (m_ImageAvailableSemaphore)
            return static_cast<VkSemaphore>(m_ImageAvailableSemaphore->GetResource());

        return VK_NULL_HANDLE;
    }

    VkSemaphore FrameSync::GetRenderFinishedSemaphore() const
    {
        if (m_RenderFinishedSemaphore)
            return static_cast<VkSemaphore>(m_RenderFinishedSemaphore->GetResource());

        return VK_NULL_HANDLE;
    }

    VkFence FrameSync::GetInFlightFence() const
    {
        if (m_InFlightFence)
            return static_cast<VkFence>(m_InFlightFence->GetRhiResource());

        return VK_NULL_HANDLE;
    }

    VkSemaphore FrameSync::GetTimelineSemaphore() const
    {
        if (m_TimelineSemaphore)
            return static_cast<VkSemaphore>(m_TimelineSemaphore->GetResource());

        return VK_NULL_HANDLE;
    }

    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// Validation
    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

    bool FrameSync::IsValid() const
    {
        bool valid = true;

        if (m_HasImageSemaphore && !m_ImageAvailableSemaphore)
        {
            SEDX_CORE_ERROR_TAG("FRAME-SYNC", "Image available semaphore creation failed for {}", m_DebugName);
            valid = false;
        }

        if (m_HasRenderSemaphore && !m_RenderFinishedSemaphore)
        {
            SEDX_CORE_ERROR_TAG("FRAME-SYNC", "Render finished semaphore creation failed for {}", m_DebugName);
            valid = false;
        }

        if (m_HasFence && !m_InFlightFence)
        {
            SEDX_CORE_ERROR_TAG("FRAME-SYNC", "In-flight fence creation failed for {}", m_DebugName);
            valid = false;
        }

        return valid;
    }

}

// -------------------------------------------------------
