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
 * render_thread.h
 * -------------------------------------------------------
 * Created: 14/7/2025
 * -------------------------------------------------------
 */
#pragma once
#include "thread.h"

// -------------------------------------------------------

namespace SceneryEditorX
{
	struct RenderThreadData;

	enum class ThreadingPolicy : uint8_t
	{
		None = 0,
	    SingleThreaded,
	    MultiThreaded // MultiThreaded will create a Render Thread
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

	    RenderThread(ThreadingPolicy policy);
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

// -------------------------------------------------------
