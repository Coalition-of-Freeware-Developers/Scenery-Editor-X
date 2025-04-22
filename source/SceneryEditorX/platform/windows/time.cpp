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
#include <algorithm>
#include <GLFW/glfw3.h>
#include <SceneryEditorX/platform/windows/time.h>

// -------------------------------------------------------

namespace SceneryEditorX
{
    float Time::GetTime()
    {
        return static_cast<float>(glfwGetTime());
    }

    void Time::Initialize()
    {
        s_ApplicationStartTime = GetTime();
        s_LastFrameTime = s_ApplicationStartTime;
        s_DeltaTime = 0.0f;
        s_CurrentFPS = 0.0f;
        s_FrameTimes.clear();
        s_Timers.clear();
    }
    
    void Time::Update()
    {
        // Calculate delta time
        const float currentTime = GetTime();
        s_DeltaTime = currentTime - s_LastFrameTime;
        s_LastFrameTime = currentTime;
        
        // Update FPS counter
        if (s_DeltaTime > 0.0f)
		{
            s_FrameTimes.push_back(s_DeltaTime);
            
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
    
    float Time::GetDeltaTime()
    {
        return s_DeltaTime;
    }
    
    float Time::GetDeltaTimeMs()
    {
        return s_DeltaTime * 1000.0f;
    }
    
    float Time::GetApplicationTime()
    {
        return GetTime() - s_ApplicationStartTime;
    }
    
    float Time::GetApplicationTimeMs()
    {
        return GetApplicationTime() * 1000.0f;
    }
    
    float Time::GetFPS()
    {
        return s_CurrentFPS;
    }
    
    uint32_t Time::CreateTimer(const float durationSeconds)
    {
        uint32_t timerID = s_NextTimerID++;
        
        Timer newTimer;
        newTimer.duration = durationSeconds;
        newTimer.startTime = GetTime();
        newTimer.isActive = true;
        
        // Find an available slot or add a new one
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
		{
            s_Timers.push_back(newTimer);
        }
        
        return timerID;
    }
    
    bool Time::IsTimerComplete(uint32_t timerID)
    {
        if (timerID >= s_Timers.size() || timerID == 0 || !s_Timers[timerID].isActive)
		{
            return false;
        }
        
        const Timer& timer = s_Timers[timerID];
        float currentTime = GetTime();
        
        return (currentTime - timer.startTime) >= timer.duration;
    }
    
    void Time::ResetTimer(uint32_t timerID, float newDurationSeconds)
    {
        if (timerID >= s_Timers.size() || timerID == 0 || !s_Timers[timerID].isActive)
		{
            return;
        }
        
        Timer& timer = s_Timers[timerID];
        timer.startTime = GetTime();
        
        if (newDurationSeconds > 0.0f)
		{
            timer.duration = newDurationSeconds;
        }
    }

} // namespace SceneryEditorX

// -------------------------------------------------------
