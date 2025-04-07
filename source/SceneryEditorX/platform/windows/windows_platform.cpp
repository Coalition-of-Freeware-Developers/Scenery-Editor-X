/**
* -------------------------------------------------------
* Scenery Editor X
* -------------------------------------------------------
* Copyright (c) 2025 Thomas Ray 
* Copyright (c) 2025 Coalition of Freeware Developers
* -------------------------------------------------------
* windows_platform.cpp
* -------------------------------------------------------
* Created: 5/4/2025
* -------------------------------------------------------
*/
#include <SceneryEditorX/platform/platform_utils.h>

// -------------------------------------------------------

namespace SceneryEditorX
{
	struct PlatformData
	{
		uint64_t TimerOffset = 0;
		uint64_t TimerFrequency = 0;
	};

	PlatformData g_PlatformData;

	// -------------------------------------------------------

    namespace IO
    {

		void Platform::Init()
		{
			QueryPerformanceFrequency((LARGE_INTEGER*)&g_PlatformData.TimerFrequency);

			uint64_t value;
			QueryPerformanceCounter((LARGE_INTEGER*)&value);
			g_PlatformData.TimerOffset = value;
		}

		double Time::GetTime()
		{
			uint64_t value;
			QueryPerformanceCounter((LARGE_INTEGER*)&value);

			return (double)(value - g_PlatformData.TimerOffset) / g_PlatformData.TimerFrequency;
		}

    } // namespace IO

} // namespace SceneryEditorX

// -------------------------------------------------------

