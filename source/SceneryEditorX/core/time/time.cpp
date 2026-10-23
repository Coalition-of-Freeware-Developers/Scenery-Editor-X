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
 * time.cpp
 * -------------------------------------------------------
 * Created: 17/4/2025
 * -------------------------------------------------------
 */
#include <SceneryEditorX/core/time/time.h>
#include <algorithm>
#include <chrono>
#include <ctime>
#include <SDL3/SDL_timer.h>

// -------------------------------------------------------

namespace SceneryEditorX
{
    
    float Time::GetTime() 
    {         
        // SDL_GetTicks() returns milliseconds since SDL initialization as Uint64
        // Convert to seconds as float
        return static_cast<float>(SDL_GetTicks()) / 1000.0f;
    }

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
        // Calculate delta time
        const float currentTime = GetTime();
        s_LastFrameTime = currentTime;

        // Update FPS counter
        if (static_cast<float>(dt) > 0.0f)
		{
            s_FrameTimes.push_back(static_cast<float>(dt));

            // Keep only the most recent samples
            while (s_FrameTimes.size() > s_MaxFrameTimesSamples)
            {
                s_FrameTimes.pop_front();
            }

            // Calculate average FPS from samples
            float totalTime = 0.0f;
            for (const float frameTime : s_FrameTimes)
            {
                totalTime += frameTime;
            }

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

    // -------------------------------------------------------

    DeltaTime::DeltaTime(const float time) : dt(time) {}

    auto DeltaTime::operator<=>(float x) const
    {
        if (dt < x)
            return std::strong_ordering::less;
        if (dt > x)
            return std::strong_ordering::greater;

        return std::strong_ordering::equal;
    }

    // -------------------------------------------------------

}

// -------------------------------------------------------
