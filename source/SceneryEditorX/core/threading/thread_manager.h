/**
* -------------------------------------------------------
* Scenery Editor X
* -------------------------------------------------------
* Copyright (c) 2025 Thomas Ray 
* Copyright (c) 2025 Coalition of Freeware Developers
* -------------------------------------------------------
* thread_manager.h
* -------------------------------------------------------
* Created: 22/6/2025
* -------------------------------------------------------
*/
#pragma once
#include <SceneryEditorX/core/threading/thread.h>

/// -------------------------------------------------------

namespace SceneryEditorX
{
    struct RenderThreadData;

	enum class ThreadingPolicy : uint8_t
	{
	    None = 0, 		/// None will run on the main thread
        SingleThreaded, /// SingleThreaded will run on the main thread
	    MultiThreaded	/// MultiThreaded will run on a separate thread
	};

    class ThreadManager
    {
    public:
        enum class State : uint8_t
        {
            Idle = 0,
            Busy,
            Kick
        };
        ThreadManager(ThreadingPolicy policy);
        ~ThreadManager();

        GLOBAL bool checkRenderThread();
		
		void Run();
        bool isRunning() const { return m_isRunning; }
        void Terminate();

        void Wait(State waitForState);
        void WaitAndSet(State waitForState, State setToState);
        void Set(State setToState);

        void NextFrame();
        void BlockUntilRenderComplete();
        void Kick();
        void Pump();

    private:
        RenderThreadData *m_Data;
        bool m_isRunning = false;
        ThreadingPolicy m_policy;
        Thread renderThread;
        std::atomic<uint32_t> appThreadFrame = 0;
    };
}

/// -------------------------------------------------------
