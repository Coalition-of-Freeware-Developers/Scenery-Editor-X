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
#define GLFW_EXPOSE_NATIVE_WIN32
#include <SceneryEditorX/core/threading/thread.h>

/// -------------------------------------------------------

namespace SceneryEditorX
{

	Thread::Thread(const std::string &name)
	{
        this->name = name;
	}

    void Thread::SetName(const std::string &name)
	{
	    const HANDLE threadHandle = mem_thread.native_handle();
		const std::wstring str(name.begin(), name.end());
		SetThreadDescription(threadHandle, str.c_str());
        SetThreadAffinityMask(threadHandle, 8);
        this->name = name;
	}

    void Thread::Join()
    {
        if (mem_thread.joinable())
            mem_thread.join();
    }

    ThreadSignal::ThreadSignal(const std::string &name, const bool manualReset)
    {
        const std::wstring str(name.begin(), name.end());
        signalHandle = CreateEventW(nullptr, manualReset ? TRUE : FALSE, FALSE, str.c_str());
    }

    void ThreadSignal::Wait() const
    {
        WaitForSingleObject(signalHandle, INFINITE);
    }

    void ThreadSignal::Signal() const
    {
        SetEvent(signalHandle);
    }

    void ThreadSignal::Reset() const
    {
        ResetEvent(signalHandle);
    }

    std::thread::id Thread::GetThreadID() const
	{
		return mem_thread.get_id();
    }

}

/// -------------------------------------------------------
