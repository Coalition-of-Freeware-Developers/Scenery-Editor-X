/**
* -------------------------------------------------------
* Scenery Editor X
* -------------------------------------------------------
* Copyright (c) 2025 Thomas Ray
* Copyright (c) 2025 Coalition of Freeware Developers
* -------------------------------------------------------
* thread.cpp
* -------------------------------------------------------
* Created: 22/6/2025
* -------------------------------------------------------
*/
#ifndef GLFW_EXPOSE_NATIVE_WIN32
#define GLFW_EXPOSE_NATIVE_WIN32
#endif
#include "thread.h"

// -------------------------------------------------------

namespace SceneryEditorX
{
	Thread::Thread(const std::string &name) { this->m_Name = name; }

    void Thread::SetName(const std::string &name)
	{
	    const HANDLE threadHandle = m_Thread.native_handle();
		const std::wstring str(name.begin(), name.end());
		SetThreadDescription(threadHandle, str.c_str());
        SetThreadAffinityMask(threadHandle, 8);
        this->m_Name = name;
	}

    void Thread::Join()
    {
        if (m_Thread.joinable())
            m_Thread.join();
    }

    ThreadSignal::ThreadSignal(const std::string &name, const bool manualReset)
    {
        const std::wstring str(name.begin(), name.end());
        m_SignalHandle = CreateEventW(nullptr, manualReset ? TRUE : FALSE, FALSE, str.c_str());
    }

    void ThreadSignal::Wait() const { WaitForSingleObject(m_SignalHandle, INFINITE); }
    void ThreadSignal::Signal() const { SetEvent(m_SignalHandle); }
    void ThreadSignal::Reset() const { ResetEvent(m_SignalHandle); }
    std::thread::id Thread::GetThreadID() const { return m_Thread.get_id(); }

}

// -------------------------------------------------------
