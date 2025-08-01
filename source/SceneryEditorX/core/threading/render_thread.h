/**
* -------------------------------------------------------
* Scenery Editor X
* -------------------------------------------------------
* Copyright (c) 2025 Thomas Ray 
* Copyright (c) 2025 Coalition of Freeware Developers
* -------------------------------------------------------
* render_thread.h
* -------------------------------------------------------
* Created: 14/7/2025
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
		None = 0,
	    SingleThreaded,
	    MultiThreaded /// MultiThreaded will create a Render Thread
	};

	class RenderThread
	{
	public:
        enum class State : uint8_t
		{
			Idle = 0,
			Busy,
			Kick
		};

        explicit RenderThread(ThreadingPolicy coreThreadingPolicy);
		~RenderThread();

		void Run();
		bool IsRunning() const { return m_IsRunning; }
		void Terminate();

		void Wait(State waitForState);
		void WaitAndSet(State waitForState, State setToState);
		void Set(State setToState);

		void NextFrame();
		void BlockUntilRenderComplete();
		void Kick();
		
		void Pump();

		static bool IsCurrentThreadRT();
	private:
		RenderThreadData* m_Data;
		ThreadingPolicy m_ThreadingPolicy;

		Thread m_RenderThread;

		bool m_IsRunning = false;

		std::atomic<uint32_t> m_AppThreadFrame = 0;
	};


}

/// -------------------------------------------------------
