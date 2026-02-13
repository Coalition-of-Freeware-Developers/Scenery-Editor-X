/*
/**
 * -------------------------------------------------------
 * Scenery Editor X
 * -------------------------------------------------------
 * Copyright (c) 2026 Thomas Ray 
 * Copyright (c) 2026 Coalition of Freeware Developers
 * -------------------------------------------------------
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 * -------------------------------------------------------
 * render_thread.cpp
 * -------------------------------------------------------
 * Created: 21/12/2025
 * -------------------------------------------------------
 #1#
#include "render_thread.h"
#include <SceneryEditorX/renderer/vulkan/renderer.h>

// -------------------------------------------------------

namespace SceneryEditorX
{

	static std::thread::id s_RenderThreadID;

    // -------------------------------------------------------

	struct RenderThreadData
	{
#ifdef SEDX_PLATFORM_WINDOWS
    #include <Windows.h>

	    CRITICAL_SECTION m_CriticalSection;
	    CONDITION_VARIABLE m_ConditionVariable;
	
	    RenderThread::State m_State = RenderThread::State::Idle;
	
#else
	
#endif
    };

    // -------------------------------------------------------


    RenderThread::RenderThread(ThreadingPolicy policy) : m_ThreadingPolicy(policy), m_RenderThread("Render Thread")
    {
        m_Data = new RenderThreadData();

		if (m_ThreadingPolicy == ThreadingPolicy::MultiThreaded)
        {
            InitializeCriticalSection(&m_Data->m_CriticalSection);
            InitializeConditionVariable(&m_Data->m_ConditionVariable);
        }
    }

    RenderThread::~RenderThread()
    {
        if (m_ThreadingPolicy == ThreadingPolicy::MultiThreaded)
            DeleteCriticalSection(&m_Data->m_CriticalSection);

        s_RenderThreadID = std::thread::id();
    }

    void RenderThread::Run()
    {
        m_IsRunning = true;
        if (m_ThreadingPolicy == ThreadingPolicy::MultiThreaded)
            m_RenderThread.Dispatch(Renderer::RenderThreadFunc, this);

        s_RenderThreadID = m_RenderThread.GetThreadID();
    }

    void RenderThread::Terminate()
    {
        m_IsRunning = false;
        Pump();

        if (m_ThreadingPolicy == ThreadingPolicy::MultiThreaded)
            m_RenderThread.Join();

        s_RenderThreadID = std::thread::id();
    }

    void RenderThread::Wait(State waitForState)
    {
        if (m_ThreadingPolicy == ThreadingPolicy::SingleThreaded)
            return;

        EnterCriticalSection(&m_Data->m_CriticalSection);
        while (m_Data->m_State != waitForState)
        {
            // This releases the CS so that another thread can wake it
            SleepConditionVariableCS(&m_Data->m_ConditionVariable, &m_Data->m_CriticalSection, INFINITE);
        }
        LeaveCriticalSection(&m_Data->m_CriticalSection);
    }

    void RenderThread::WaitAndSet(State waitForState, State setToState)
    {
        if (m_ThreadingPolicy == ThreadingPolicy::SingleThreaded)
            return;

        EnterCriticalSection(&m_Data->m_CriticalSection);
        while (m_Data->m_State != waitForState)
        {
            SleepConditionVariableCS(&m_Data->m_ConditionVariable, &m_Data->m_CriticalSection, INFINITE);
        }
        m_Data->m_State = setToState;
        WakeAllConditionVariable(&m_Data->m_ConditionVariable);
        LeaveCriticalSection(&m_Data->m_CriticalSection);
    }

    void RenderThread::Set(State setToState)
    {
        if (m_ThreadingPolicy == ThreadingPolicy::SingleThreaded)
            return;

        EnterCriticalSection(&m_Data->m_CriticalSection);
        m_Data->m_State = setToState;
        WakeAllConditionVariable(&m_Data->m_ConditionVariable);
        LeaveCriticalSection(&m_Data->m_CriticalSection);
    }

    void RenderThread::NextFrame()
    {
        ++m_AppThreadFrame;
        Renderer::Tick(m_AppThreadFrame);
    }

    void RenderThread::BlockUntilRenderComplete()
    {
        if (m_ThreadingPolicy == ThreadingPolicy::SingleThreaded)
            return;

        Wait(State::Idle);
    }

    void RenderThread::Kick()
    {
        if (m_ThreadingPolicy == ThreadingPolicy::MultiThreaded)
        {
            Set(State::Kick);
        }
        else
        {
            Renderer::WaitAndRender(this);
        }
    }

    void RenderThread::Pump()
    {
        NextFrame();
        Kick();
        BlockUntilRenderComplete();
    }

    bool RenderThread::IsCurrentThreadRT()
    {
        //SEDX_CORE_VERIFY(s_RenderThreadID != std::thread::id());
        return s_RenderThreadID == std::this_thread::get_id();
    }

} // namespace SceneryEditorX

// -------------------------------------------------------
*/
