/**
* -------------------------------------------------------
* Scenery Editor X
* -------------------------------------------------------
* Copyright (c) 2025 Thomas Ray 
* Copyright (c) 2025 Coalition of Freeware Developers
* -------------------------------------------------------
* semaphore.h
* -------------------------------------------------------
* Created: 28/8/2025
* -------------------------------------------------------
*/
#pragma once
#include "vulkan/vk_enums.h"

/// -------------------------------------------------------

namespace SceneryEditorX
{
	class Semaphore : public RefCounted
	{
	public:
        explicit Semaphore(FrameSyncType type, void *&resource);
        virtual ~Semaphore() override = default;

	    static void Create(FrameSyncType type, void *&resource);
        static void WaitTime(uint64_t value, uint64_t timeout, void *&resource);
        static void Signal(uint64_t value, void *&resource);
        static uint64_t GetCounterValue(void *&resource);
    };

}

/// -------------------------------------------------------
