/**
* -------------------------------------------------------
* Scenery Editor X
* -------------------------------------------------------
* Copyright (c) 2025 Thomas Ray
* Copyright (c) 2025 Coalition of Freeware Developers
* -------------------------------------------------------
* time.cpp
* -------------------------------------------------------
* Created: 17/4/2025
* -------------------------------------------------------
*/
#include "time.h"
#include <algorithm>
#include <chrono>
#include <ctime>
#include <GLFW/glfw3.h>

/// -------------------------------------------------------

namespace SceneryEditorX
{
    float Time::GetTime() { return (float)glfwGetTime(); }

    void Time::Init()
    {
        s_ApplicationStartTime = GetTime();
        s_LastFrameTime = s_ApplicationStartTime;
        s_CurrentFPS = 0.0f;
        s_FrameTimes.clear();
        s_Timers.clear();
        DeltaTime();
    }

    void Time::Update(const DeltaTime dt)
    {
        /// Calculate delta time
        const float currentTime = GetTime();
        s_LastFrameTime = currentTime;

        /// Update FPS counter
        if (static_cast<float>(dt) > 0.0f)
		{
            s_FrameTimes.push_back(static_cast<float>(dt));

            /// Keep only the most recent samples
            while (s_FrameTimes.size() > s_MaxFrameTimesSamples)
                s_FrameTimes.pop_front();

            /// Calculate average FPS from samples
            float totalTime = 0.0f;
            for (const float frameTime : s_FrameTimes)
                totalTime += frameTime;

            s_CurrentFPS = s_FrameTimes.size() / totalTime;
        }
    }

    float Time::GetApplicationTime() { return GetTime() - s_ApplicationStartTime; }

    float Time::GetApplicationTimeMs() { return GetApplicationTime() * 1000.0f; }

    float Time::GetFPS() { return s_CurrentFPS; }

    uint32_t Time::CreateTimer(const float durationSeconds)
    {
        const uint32_t timerID = s_NextTimerID++;

        Timer newTimer;
        newTimer.duration = durationSeconds;
        newTimer.startTime = GetTime();
        newTimer.isActive = true;

        /// Find an available slot or add a new one
        bool timerAdded = false;
        for (auto& timer : s_Timers)
		{
            if (!timer.isActive)
			{
                timer = newTimer;
                timerAdded = true;
                break;
            }
        }

        if (!timerAdded)
            s_Timers.push_back(newTimer);

        return timerID;
    }

    bool Time::IsTimerComplete(const uint32_t timerID)
    {
        if (timerID >= s_Timers.size() || timerID == 0 || !s_Timers[timerID].isActive)
            return false;

        const Timer& timer = s_Timers[timerID];
        const float currentTime = GetTime();

        return currentTime - timer.startTime >= timer.duration;
    }

    void Time::ResetTimer(const uint32_t timerID, const float newDurationSeconds)
    {
        if (timerID >= s_Timers.size() || timerID == 0 || !s_Timers[timerID].isActive)
            return;

        Timer& timer = s_Timers[timerID];
        timer.startTime = GetTime();

        if (newDurationSeconds > 0.0f)
            timer.duration = newDurationSeconds;
    }

	uint64_t Time::GetCurrentDateTimeU64()
	{
		const std::string string = GetCurrentDateTimeString();
		return std::stoull(string);
	}

	std::string Time::GetCurrentDateTimeString()
	{
		const std::time_t currentTime = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
		const std::tm* localTime = std::localtime(&currentTime);

		int year = localTime->tm_year + 1900;
		int month = localTime->tm_mon + 1;
		int day = localTime->tm_mday + 1;
		int hour = localTime->tm_hour;
		int minute = localTime->tm_min;

		return std::format("{}{:02}{:02}{:02}{:02}", year, month, day, hour, minute);
		//return std::format("{:%Y%m%d%H%M}", *localTime);
	}

    /// -------------------------------------------------------

    DeltaTime::DeltaTime(const float time) : dt(time) {}

    /// -------------------------------------------------------

}

/// -------------------------------------------------------
