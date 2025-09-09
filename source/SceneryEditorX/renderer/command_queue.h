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
//#include <array>
//#include <atomic>
//#include <cstdint>
//#include <mutex>
//#include "vulkan/resource.h"
//#include "vulkan/vk_enums.h"

/// -------------------------------------------------------

/*
namespace SceneryEditorX
{

    class CommandQueue : public Resource
	{
    public:
		CommandQueue(Queue queueType, const std::string &debugName);
		~CommandQueue();

		/// Wait for any in-flight work (stub in refactor – no threaded queue).
		void Wait(const bool flush = false);
		/// Submit raw command buffer to underlying Vulkan queue (temporary no-op).
		void Submit(void *cmdBufferIn, uint32_t waitFlags);
		/// Execute enqueued lambdas (legacy). Now a no-op to keep interfaces compiling.
        void Execute();
        /// Legacy signature retained to satisfy older call sites expecting parameters.
        inline void Execute(void*, const uint32_t) { Execute(); }
        /// Get the number of commands submitted (legacy, preserved for stats).
        Queue GetQueueType() const { return qType; }

	private:
        uint8_t* cmdBuffer = nullptr;      ///< Legacy backing storage (unused in refactor)
		uint8_t* cmdBufferPtr = nullptr;     ///< Legacy write pointer
        std::atomic<uint32_t> cmdCount{0}; ///< Preserved for stats / future re-enable
        Queue qType;
	};

}
*/

/// -------------------------------------------------------
