/**
* -------------------------------------------------------
* Scenery Editor X
* -------------------------------------------------------
* Copyright (c) 2025 Thomas Ray 
* Copyright (c) 2025 Coalition of Freeware Developers
* -------------------------------------------------------
* thread.h
* -------------------------------------------------------
* Created: 22/6/2025
* -------------------------------------------------------
*/
#pragma once
#include <string>
#include <thread>

/// -------------------------------------------------------

namespace SceneryEditorX
{
	class Thread
	{
	public:
        explicit Thread(const std::string &name);

		template<typename Func, typename... Args>
        void Dispatch(Func &&func, Args &&...args)
		{
            mem_thread = std::thread(func, std::forward<Args>(args)...);
            SetName(name);
		}

		void SetName(const std::string &name);
		void Join();

		std::thread::id GetThreadID() const;
	private:
        std::string name;
        std::thread mem_thread;
	};

    /// -------------------------------------------------------

    class ThreadSignal
    {
    public:
        explicit ThreadSignal(const std::string &name, bool manualReset = false);

		void Wait() const;
        void Signal() const;
        void Reset() const;
    private:
        void *signalHandle = nullptr;
    };

}

/// -------------------------------------------------------
