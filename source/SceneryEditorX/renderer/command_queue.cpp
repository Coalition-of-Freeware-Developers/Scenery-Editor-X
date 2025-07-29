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
#include <SceneryEditorX/core/memory/memory.h>
#include <SceneryEditorX/renderer/command_queue.h>
#include <SceneryEditorX/utils/math/math_utils.h>

/// -------------------------------------------------------

namespace SceneryEditorX
{
    CommandQueue::CommandQueue()
    {
        cmdBuffer = hnew uint8_t[10 * 1024 * 1024]; /// 10mb buffer
        cmdBufferPtr = cmdBuffer;
        memset(cmdBuffer, 0, 10 * 1024 * 1024);
    }

    CommandQueue::~CommandQueue()
    {
        delete[] cmdBuffer;
    }

    void* CommandQueue::Allocate(RenderCommandFn func, uint32_t size)
    {
        *(RenderCommandFn *)cmdBufferPtr = func;
        cmdBufferPtr += alignof(RenderCommandFn);

        *(uint32_t *)cmdBufferPtr = size;
        cmdBufferPtr += Utils::RoundUp(sizeof(uint32_t), alignof(RenderCommandFn));

        void *memory = cmdBufferPtr;
        cmdBufferPtr += Utils::RoundUp<size_t>(size, alignof(RenderCommandFn));

        cmdCount++;
        return memory;
    }

    void CommandQueue::Execute()
    {
        byte *buffer = cmdBuffer;

		for (uint32_t i = 0; i < cmdCount; i++)
        {
            RenderCommandFn function = *(RenderCommandFn *)buffer;
            buffer += sizeof(RenderCommandFn);

            uint32_t size = *(uint32_t *)buffer;
            buffer += Utils::RoundUp(sizeof(uint32_t), alignof(RenderCommandFn));

            function(buffer);
            buffer += Utils::RoundUp<size_t>(size, alignof(RenderCommandFn));
        }

        cmdBufferPtr = cmdBuffer;
        cmdCount = 0;
    }

}

/// -------------------------------------------------------
