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
#include "vulkan/vk_util.h"

/// -------------------------------------------------------

namespace SceneryEditorX
{

	FrameSync::FrameSync(const FrameSyncType type, const std::string &debugName)
	{
        m_type = type;
        m_DebugName = debugName;

		if (type == FrameSyncType::SyncFence)
			CreateRef<Fence>(resource);
		else
            CreateRef<Semaphore>(type, resource);
	}
	
	FrameSync::~FrameSync()
	{
        if (!resource)
            return;

		resource = nullptr;
	}
	
	void FrameSync::Wait(const uint64_t timeout)
	{
        SEDX_ASSERT(m_type == FrameSyncType::SyncFence || m_type == FrameSyncType::SyncSemaphoreTimeline);

		if (m_type == FrameSyncType::SyncFence)
			Fence::WaitTime(timeout, resource);
		else
            Semaphore::WaitTime(signalValue, timeout, resource);
	}
	
	void FrameSync::Signal(const uint64_t value)
	{
        SEDX_ASSERT(m_type == FrameSyncType::SyncSemaphoreTimeline);

		Semaphore::Signal(value, resource);
	}
	
	bool FrameSync::IsSignaled()
	{
        SEDX_ASSERT(m_type != FrameSyncType::SyncSemaphore);

		if (m_type == FrameSyncType::SyncFence)
            return Fence::IsSignaled(resource);

	    return Semaphore::GetCounterValue(resource) == signalValue;
	}
	
	void FrameSync::Reset()
	{
		SEDX_ASSERT(m_type == FrameSyncType::SyncFence);

	    Fence::Reset(resource);
	}

}

/// -------------------------------------------------------
