/**
* -------------------------------------------------------
* Scenery Editor X
* -------------------------------------------------------
* Copyright (c) 2025 Thomas Ray 
* Copyright (c) 2025 Coalition of Freeware Developers
* -------------------------------------------------------
* command_queue.h
* -------------------------------------------------------
* Created: 22/6/2025
* -------------------------------------------------------
*/
#pragma once

/// -------------------------------------------------------

namespace SceneryEditorX
{

    class CommandQueue
	{
    public:
        typedef void (*RenderCommandFn)(void *);

		CommandQueue();
		~CommandQueue();

        void *Allocate(RenderCommandFn func, uint32_t size);

        void Execute();
	private:
        uint8_t* cmdBuffer;
		uint8_t* cmdBufferPtr;
		uint32_t cmdCount = 0;
	};

}

/// -------------------------------------------------------
