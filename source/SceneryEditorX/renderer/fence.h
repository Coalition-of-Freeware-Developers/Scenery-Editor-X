/**
* -------------------------------------------------------
* Scenery Editor X
* -------------------------------------------------------
* Copyright (c) 2025 Thomas Ray 
* Copyright (c) 2025 Coalition of Freeware Developers
* -------------------------------------------------------
* fence.h
* -------------------------------------------------------
* Created: 28/8/2025
* -------------------------------------------------------
*/
#pragma once

/// -------------------------------------------------------

namespace SceneryEditorX
{

    class Fence : public RefCounted
	{
	public:
	    explicit Fence(void *&resource);
        virtual ~Fence() override = default;

	    static void Create(void* &resource);
	    static bool IsSignaled(void* &resource);
	    static void WaitTime(uint64_t timeout, void *&resource);
	    static void Reset(void *&resource);
	};

}

/// -------------------------------------------------------
