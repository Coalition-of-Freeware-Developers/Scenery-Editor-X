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
 * thread.h
 * -------------------------------------------------------
 * Created: 22/6/2025
 * -------------------------------------------------------
 */
#pragma once
#include <string>
#include <thread>

// -------------------------------------------------------

namespace SceneryEditorX
{

	class Thread
	{
	public:
        explicit Thread(const std::string &name);

		template<typename Func, typename... Args>
        void Dispatch(Func &&func, Args &&...args)
		{
            m_Thread = std::thread(func, std::forward<Args>(args)...);
            SetName(m_Name);
		}

		void SetName(const std::string &name);
		void Join();

        [[nodiscard]] std::thread::id GetThreadID() const;
	private:
        std::string m_Name;
        std::thread m_Thread;
	};

    // -------------------------------------------------------

    class ThreadSignal
    {
    public:
        explicit ThreadSignal(const std::string &name, bool manualReset = false);

		void Wait() const;
        void Signal() const;
        void Reset() const;
    private:
        void *m_SignalHandle = nullptr;
    };

}

// -------------------------------------------------------
