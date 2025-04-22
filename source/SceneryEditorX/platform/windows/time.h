/**
* -------------------------------------------------------
* Scenery Editor X
* -------------------------------------------------------
* Copyright (c) 2025 Thomas Ray 
* Copyright (c) 2025 Coalition of Freeware Developers
* -------------------------------------------------------
* time.h
* -------------------------------------------------------
* Created: 17/4/2025
* -------------------------------------------------------
*/
#pragma once
#include <chrono>
#include <deque>
#include <string>

// -------------------------------------------------------

namespace SceneryEditorX
{
	/**
	 * @brief Time management class for tracking delta time, FPS, and providing timers.
	 *
	 * This class provides functionality for retrieving system time, application time,
	 * calculating delta time between frames, tracking frames per second,
	 * and implementing timers for game mechanics.
	 */
	class Time
	{
	public:
	    /**
	     * @brief Gets the current time in seconds since GLFW initialization.
	     * @return float The current system time in seconds.
	     */
	    static float GetTime();

        /**
         * @brief Logs the time taken for a specific operation.
         * @param title The title of the operation to log.
         */
        struct TimeLog
        {
            std::string title;
            TimeLog(const std::string &title,  bool logged = false)
            {
                this->title = title;
                start = std::chrono::high_resolution_clock::now();
                this->logged = logged;
            }

            ~TimeLog()
            {
                std::chrono::high_resolution_clock::time_point now = std::chrono::high_resolution_clock::now();
                float elapsed = std::chrono::duration_cast<std::chrono::microseconds>(now - start).count() / 1000.0f;
                if (logged)
                {
                    SEDX_CORE_INFO("{} took {} seconds", title.c_str(), elapsed / 1000.0f);
                }
                else
                {
                    if (auto it = _timings.find(title); it == _timings.end())
                    {
                        _timings[title] = elapsed;
                    }
                    else
                    {
                        it->second = (it->second + elapsed) / 2.0f;
                    }
                }
            }

			static const std::unordered_map<std::string, float> &GetCPUTimes() { return _timings; }
            static std::unordered_map<std::string, float> _timings;

        private:
            std::chrono::high_resolution_clock::time_point start;
            bool logged = false;
        };

	    /**
	     * @brief Initializes the Time system.
	     * Should be called at application startup.
	     */
	    static void Initialize();
	
	    /**
	     * @brief Updates the time-related variables each frame.
	     * Should be called once per frame.
	     */
	    static void Update();
	
	    /**
	     * @brief Get the time elapsed since the last frame in seconds.
	     * @return float Delta time in seconds.
	     */
	    static float GetDeltaTime();
	
	    /**
	     * @brief Get the time elapsed since the last frame in milliseconds.
	     * @return float Delta time in milliseconds.
	     */
	    static float GetDeltaTimeMs();
	
	    /**
	     * @brief Get the time elapsed since application start in seconds.
	     * @return float Application runtime in seconds.
	     */
	    static float GetApplicationTime();
	
	    /**
	     * @brief Get the time elapsed since application start in milliseconds.
	     * @return float Application runtime in milliseconds.
	     */
	    static float GetApplicationTimeMs();
	
	    /**
	     * @brief Get the current frames per second.
	     * @return float Current FPS value.
	     */
	    static float GetFPS();
	
	    /**
	     * @brief Create a new timer with a specific duration.
	     * @param durationSeconds Timer duration in seconds.
	     * @return uint32_t Timer ID that can be used to check the timer's status.
	     */
	    static uint32_t CreateTimer(float durationSeconds);
	
	    /**
	     * @brief Check if a timer has completed.
	     * @param timerID ID of the timer to check.
	     * @return bool True if timer has completed, false otherwise.
	     */
	    static bool IsTimerComplete(uint32_t timerID);
	
	    /**
	     * @brief Reset an existing timer.
	     * @param timerID ID of the timer to reset.
	     * @param newDurationSeconds Optional new duration in seconds.
	     */
	    static void ResetTimer(uint32_t timerID, float newDurationSeconds = -1.0f);
	
	private:
	    struct Timer
	    {
	        float duration;
	        float startTime;
	        bool isActive;
	    };
	
	    // Time tracking
	    static inline float s_DeltaTime = 0.0f;
	    static inline float s_LastFrameTime = 0.0f;
	    static inline float s_ApplicationStartTime = 0.0f;
	
	    // FPS tracking
	    static inline std::deque<float> s_FrameTimes;
	    static inline float s_CurrentFPS = 0.0f;
	    static constexpr size_t s_MaxFrameTimesSamples = 60;
	
	    // Timer system
	    static inline std::vector<Timer> s_Timers;
	    static inline uint32_t s_NextTimerID = 1;
    };

} // namespace SceneryEditorX

// -------------------------------------------------------
