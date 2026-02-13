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
