/**
* -------------------------------------------------------
* Scenery Editor X
* -------------------------------------------------------
* Copyright (c) 2025 Thomas Ray
* Copyright (c) 2025 Coalition of Freeware Developers
* -------------------------------------------------------
* thread_pool.h
* -------------------------------------------------------
* Created: 26/9/2025
* -------------------------------------------------------
*/
#pragma once
#include <future>
#include <functional>

// -------------------------------------------------------

namespace SceneryEditorX
{

    class ThreadPool
    {
    public:
        ThreadPool() = default;
        ~ThreadPool() = default;

		static void Init();
		static void Shutdown();




    };


}

// -------------------------------------------------------
