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
#include <SceneryEditorX/core/threading/thread.h>

#define GLFW_EXPOSE_NATIVE_WIN32

/// -------------------------------------------------------

namespace SceneryEditorX
{

	Thread::Thread(const std::string &name)
	{
        this->name = name;
	}

    void Thread::SetName(const std::string &name)
	{
	    HANDLE threadHandle = mem_thread.native_handle();
		std::wstring str(name.begin(), name.end());
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
        std::wstring str(name.begin(), name.end());
        signalHandle = CreateEventW(nullptr, manualReset ? TRUE : FALSE, FALSE, str.c_str());
    }

    void ThreadSignal::Wait()
    {
        WaitForSingleObject(signalHandle, INFINITE);
    }
    
    void ThreadSignal::Signal()
    {
        SetEvent(signalHandle);
    }
    
    void ThreadSignal::Reset()
    {
        ResetEvent(signalHandle);
    }

    std::thread::id Thread::GetThreadID() const
	{
		return mem_thread.get_id();
    }

}

/// -------------------------------------------------------
