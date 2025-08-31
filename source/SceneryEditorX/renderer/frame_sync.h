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
#include "vulkan/vk_enums.h"

/// -------------------------------------------------------

namespace SceneryEditorX
{

    class FrameSync : public RefCounted
	{
	public:
        FrameSync(FrameSyncType type, const std::string &debugName);
        ~FrameSync();

		void Wait(const uint64_t timeout);
        void Signal(const uint64_t value);
        bool IsSignaled();
        void Reset();

		uint64_t GetNextSignalValue() { return ++signalValue; }
        void *GetResource() { return resource; }

    private:
		void *resource = nullptr;
		uint64_t signalValue = 0;
        FrameSyncType m_type = FrameSyncType::MaxEnum;
        std::string m_DebugName;
	};

}

/// -------------------------------------------------------
