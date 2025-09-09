/**
* -------------------------------------------------------
* Scenery Editor X
* -------------------------------------------------------
* Copyright (c) 2025 Thomas Ray
* Copyright (c) 2025 Coalition of Freeware Developers
* -------------------------------------------------------
* command_queue.cpp
* -------------------------------------------------------
* Created: 22/6/2025
* -------------------------------------------------------
*/
//#include "command_queue.h"
//#include "SceneryEditorX/core/memory/memory.h"
//#include <mutex>

/// -------------------------------------------------------

/*
namespace SceneryEditorX
{
    namespace
    {
        std::array<std::mutex, 3> mutexes; // One mutex for each queue type (Graphics, Compute, Transfer)

	    /// Get the mutex corresponding to the queue type
		std::mutex &GetMutex(const CommandQueue *queue)
		{
            return mutexes[static_cast<uint32_t>(queue->GetQueueType())];
		}
    }

    CommandQueue::CommandQueue(Queue queueType, const std::string &debugName) : qType(queueType)
    {
        // Legacy allocation (kept to avoid touching broader lifetime assumptions)
        cmdBuffer = new uint8_t[1];
        cmdBufferPtr = cmdBuffer;
    }

    CommandQueue::~CommandQueue()
    {
        delete[] cmdBuffer;
    }

    void CommandQueue::Wait(const bool flush)
    {
        std::lock_guard<std::mutex> lock(GetMutex(this)); (void)flush;
    }

    void CommandQueue::Submit(void *cmdBufferIn, uint32_t waitFlags) { (void)cmdBufferIn; (void)waitFlags; /* no-op in refactor #1# }

    void CommandQueue::Execute() { /* legacy queue removed #1# }

    /*
    void* CommandQueue::Allocate(RenderCommandFn func, uint32_t size)
    {
        *(RenderCommandFn *)cmdBufferPtr = func;
        cmdBufferPtr += alignof(RenderCommandFn);

        *(uint32_t *)cmdBufferPtr = size;
        cmdBufferPtr += RoundUp(sizeof(uint32_t), alignof(RenderCommandFn));

        void *memory = cmdBufferPtr;
        cmdBufferPtr += RoundUp<size_t>(size, alignof(RenderCommandFn));

        cmdCount++;
        return memory;
    }
    #1#

    /*
    void CommandQueue::Execute()
    {
        byte *buffer = cmdBuffer;

		for (uint32_t i = 0; i < cmdCount; i++)
        {
            RenderCommandFn function = *(RenderCommandFn *)buffer;
            buffer += sizeof(RenderCommandFn);

            uint32_t size = *(uint32_t *)buffer;
            buffer += RoundUp(sizeof(uint32_t), alignof(RenderCommandFn));

            function(buffer);
            buffer += RoundUp<size_t>(size, alignof(RenderCommandFn));
        }

        cmdBufferPtr = cmdBuffer;
        cmdCount = 0;
    }
    #1#

}
*/

/// -------------------------------------------------------
