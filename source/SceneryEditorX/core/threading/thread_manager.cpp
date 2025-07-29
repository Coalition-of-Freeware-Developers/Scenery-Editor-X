/**
* -------------------------------------------------------
* Scenery Editor X
* -------------------------------------------------------
* Copyright (c) 2025 Thomas Ray
* Copyright (c) 2025 Coalition of Freeware Developers
* -------------------------------------------------------
* thread_manager.cpp
* -------------------------------------------------------
* Created: 22/6/2025
* -------------------------------------------------------
*/
#include <SceneryEditorX/core/threading/thread.h>
#include <SceneryEditorX/core/threading/thread_manager.h>
#include <SceneryEditorX/renderer/renderer.h>

/// -------------------------------------------------------

namespace SceneryEditorX
{

    /// -------------------------------------------------------

	struct RenderThreadData
	{
	    CRITICAL_SECTION m_CriticalSection;
	    CONDITION_VARIABLE m_ConditionVariable;

	    ThreadManager::State m_State = ThreadManager::State::Idle;
	};

	LOCAL std::thread::id s_threadID;

    /// -------------------------------------------------------

	ThreadManager::ThreadManager(const ThreadingPolicy policy) : m_policy(policy), renderThread("Render Thread")
    {
        m_Data = new RenderThreadData();

		if (m_policy == ThreadingPolicy::MultiThreaded)
        {
            InitializeCriticalSection(&m_Data->m_CriticalSection);
            InitializeConditionVariable(&m_Data->m_ConditionVariable);
        }
    }

    /// -------------------------------------------------------

    ThreadManager::~ThreadManager()
	{
        if (m_policy == ThreadingPolicy::MultiThreaded)
            DeleteCriticalSection(&m_Data->m_CriticalSection);

        s_threadID = std::thread::id();
	}

	bool ThreadManager::checkRenderThread()
	{
        return s_threadID == std::this_thread::get_id();
	}

	void ThreadManager::Run()
	{
        m_isRunning = true;
        if (m_policy == ThreadingPolicy::MultiThreaded)
            renderThread.Dispatch(Renderer::RenderThreadFunc, this);

        s_threadID = renderThread.GetThreadID();
	}

	void ThreadManager::Terminate()
	{
        m_isRunning = false;
        Pump();

        if (m_policy == ThreadingPolicy::MultiThreaded)
            renderThread.Join();

        s_threadID = std::thread::id();
	}

	void ThreadManager::Wait(const State waitForState) const
    {
        if (m_policy == ThreadingPolicy::SingleThreaded)
            return;

        EnterCriticalSection(&m_Data->m_CriticalSection);
        while (m_Data->m_State != waitForState) // This releases the CS so that another thread can wake it
            SleepConditionVariableCS(&m_Data->m_ConditionVariable, &m_Data->m_CriticalSection, INFINITY);

        LeaveCriticalSection(&m_Data->m_CriticalSection);
	}

	void ThreadManager::WaitAndSet(const State waitForState, const State setToState) const
    {
        if (m_policy == ThreadingPolicy::SingleThreaded)
            return;

        EnterCriticalSection(&m_Data->m_CriticalSection);
        while (m_Data->m_State != waitForState)
            SleepConditionVariableCS(&m_Data->m_ConditionVariable, &m_Data->m_CriticalSection, INFINITY);

	    m_Data->m_State = setToState;
        WakeAllConditionVariable(&m_Data->m_ConditionVariable);
        LeaveCriticalSection(&m_Data->m_CriticalSection);
	}

	void ThreadManager::Set(const State setToState) const
    {
        if (m_policy == ThreadingPolicy::SingleThreaded)
            return;

        EnterCriticalSection(&m_Data->m_CriticalSection);
        m_Data->m_State = setToState;
        WakeAllConditionVariable(&m_Data->m_ConditionVariable);
        LeaveCriticalSection(&m_Data->m_CriticalSection);
	}

	void ThreadManager::NextFrame()
	{
        ++appThreadFrame;
        Renderer::SwapQueues();
	}

	void ThreadManager::BlockUntilRenderComplete() const
    {
        if (m_policy == ThreadingPolicy::SingleThreaded)
            return;

        Wait(State::Idle);
	}

	void ThreadManager::Kick() const
    {
        if (m_policy == ThreadingPolicy::MultiThreaded)
            Set(State::Kick);
        else
            Renderer::WaitAndRender(this);
    }

	void ThreadManager::Pump()
	{
        NextFrame();
        Kick();
        BlockUntilRenderComplete();
	}

	std::thread::id Thread::GetThreadID() const { return mem_thread.get_id(); }

}

/// -------------------------------------------------------
