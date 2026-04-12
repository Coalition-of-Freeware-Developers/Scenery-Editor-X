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
 * timer.h
 * -------------------------------------------------------
 * Created: 14/7/2025
 * -------------------------------------------------------
 */
#pragma once

// -------------------------------------------------------

namespace SceneryEditorX
{
	class PerformanceProfiler;

	/**
	 * @class Timer
	 * @brief High-resolution timer class for measuring elapsed time in seconds and milliseconds.
	 */
	class Timer
	{
	public:
		/* @brief Constructs a Timer and resets it to the current time. */
		SEDX_FORCE_INLINE Timer() { Reset(); }
		SEDX_FORCE_INLINE ~Timer() = default;

		/* @brief Resets the timer to the current time. */
		SEDX_FORCE_INLINE void Reset() { m_Start = std::chrono::high_resolution_clock::now(); }

		/**
		 * @brief Returns the time elapsed in seconds since the last Reset() call.
		 * @return Time elapsed in seconds since the last Reset() call.
		 */
		SEDX_FORCE_INLINE float Elapsed() const { return std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::high_resolution_clock::now() - m_Start).count() * 0.001f * 0.001f; }

		/**
		 * @brief Returns the time elapsed in milliseconds since the last Reset() call.
		 * @return Time elapsed in milliseconds since the last Reset() call. 
		 */
		float ElapsedMillis() const;

	private:
		std::chrono::time_point<std::chrono::high_resolution_clock> m_Start; // Start time point for the timer
	};

	// -------------------------------------------------------

	/**
	 * @class PerformanceProfiler
	 * @brief Class for profiling performance by tracking timing data for various named sections of code on a per-frame basis. 
	 * Allows accumulation of timing data across multiple samples within the same frame and provides thread-safe access to the collected data.
	 */
	class PerformanceProfiler
	{
	public:
		/**
		 * @struct PerFrameData
		 * @brief Struct to hold timing data for a specific section of code within a single frame, including total time and sample count.
		 */
		struct PerFrameData
		{
			float time = 0.0f;
			uint32_t samples = 0;

			PerFrameData() = default;

			/**
			 * @brief Constructs a PerFrameData instance with a specified time value.
			 * @param time The initial time value for the PerFrameData instance.
			 */
			explicit PerFrameData(const float time) : time(time) {
			}

			/**
			 * @brief Converts the PerFrameData instance to a float representing the time value.
			 * @return The time value of the PerFrameData instance.
			 */
			explicit operator float() const { return time; }

			/**
			 * @brief Adds a specified time value to the PerFrameData instance.
			 * @param inTime The time value to add.
			 * @return A reference to the updated PerFrameData instance.
			 */
			PerFrameData& operator+=(const float inTime)
			{
				time += inTime;
				return *this;
			}

			/**
			 * @brief Assigns a specified time value to the PerFrameData instance.
			 * @param x The time value to assign.
			 * @return A reference to the updated PerFrameData instance.
			 */
			std::unordered_map<const char *,PerFrameData>::mapped_type &operator=(float x);
		};

		/**
		 * @brief Sets the timing data for a specific section of code within a single frame.
		 * @param name The name of the section of code.
		 * @param time The time value to set for the section of code.
		 */
		void SetPerFrameTiming(const char* name, const float time)
		{
			std::scoped_lock<std::mutex> lock(m_PerFrameDataMutex);

			if (!m_PerFrameData.contains(name))
				m_PerFrameData[name] = 0.0f;

			PerFrameData& data = m_PerFrameData[name];
			data.time += time;
			data.samples++;
		}

		/**
		 * @brief Clears all per-frame timing data.
		 */
		void Clear()
		{
			std::scoped_lock<std::mutex> lock(m_PerFrameDataMutex);
			m_PerFrameData.clear();
		}

		/**
		 * @brief Returns the per-frame timing data.
		 * @return A constant reference to the per-frame timing data.
		 */
		const std::unordered_map<const char*, PerFrameData>& GetPerFrameData() const { return m_PerFrameData; }
	private:
		std::unordered_map<const char*, PerFrameData> m_PerFrameData; // Map to hold timing data for each named section of code, indexed by the section name
		inline static std::mutex m_PerFrameDataMutex; // Mutex to protect access to the per-frame timing data map, ensuring thread safety when updating or clearing timing data
	};

	// -------------------------------------------------------

	/**
	 * @class ScopedTimer
	 * @brief RAII-style timer that automatically measures the time taken by a scope and logs it to the console and optionally to a PerformanceProfiler instance.
	 */
	class ScopedTimer
	{
	public:
		/**
		 * @brief Constructs a ScopedTimer instance with a specified name.
		 * @param name The name of the timed section, used for logging and profiling.
		 */
		explicit ScopedTimer(std::string name) : m_Name(std::move(name)) {}

		/**
		 * @brief Destroys the ScopedTimer instance and logs the elapsed time to the console and optionally to a PerformanceProfiler instance.
		 */
		~ScopedTimer()
		{
			const float time = m_Timer.ElapsedMillis();
			if (m_Profiler)
				m_Profiler->SetPerFrameTiming(m_Name.c_str(), time);
			SEDX_CORE_TRACE_TAG("Timer", "{0} - {1}ms", m_Name, time);
		}
	private:
		std::string m_Name; // Name of the timed section, used for logging and profiling
		Timer m_Timer;      // Timer instance to measure elapsed time for the scope
		PerformanceProfiler* m_Profiler = nullptr; // Optional pointer to a PerformanceProfiler instance for logging timing data; if nullptr, timing data will not be logged to the profiler
	};

	// -------------------------------------------------------

	/**
	 * @class ScopePerfTimer
	 * @brief RAII-style timer that automatically measures the time taken by a scope and logs it to a PerformanceProfiler instance for per-frame performance tracking.
	 */
	class ScopePerfTimer
	{
	public:
		/**
		 * @brief Constructs a ScopePerfTimer instance with a specified name and associated PerformanceProfiler.
		 * @param name The name of the timed section, used for logging and profiling.
		 * @param profiler Pointer to a PerformanceProfiler instance for logging timing data; must not be nullptr.
		 */
		ScopePerfTimer(const char* name, PerformanceProfiler* profiler) : m_Name(name), m_Profiler(profiler) {
		}

		/**
		 * @brief Destroys the ScopePerfTimer instance and logs the elapsed time to the associated PerformanceProfiler instance for per-frame performance tracking.
		 */
		~ScopePerfTimer()
		{
			const float time = m_Timer.ElapsedMillis();
			m_Profiler->SetPerFrameTiming(m_Name, time);
		}
	private:
		const char *m_Name; // Name of the timed section, used for logging and profiling
		PerformanceProfiler* m_Profiler; // Pointer to a PerformanceProfiler instance for logging timing data; must not be nullptr
		Timer m_Timer;   // Timer instance to measure elapsed time for the scope
	};

#if 1
	/**
	 * @brief Macro to create a ScopePerfTimer instance for profiling a specific section of code with a given name, 
	 * automatically logging the timing data to the PerformanceProfiler instance associated with the application.
	 * @param name The name of the timed section, used for logging and profiling.
	 */
	#define SEDX_SCOPE_PERF(name)\
		ScopePerfTimer timer__LINE__(name, Application::Get().GetPerformanceProfiler());

	/**
	 * @brief Macro to create a ScopedTimer instance for timing a specific section of code with a given name,
	 * automatically logging the timing data to the console.
	 * @param name The name of the timed section, used for logging.
	 */
	#define SEDX_SCOPE_TIMER(name)\
		ScopedTimer timer__LINE__(name);
#else
	#define SEDX_SCOPE_PERF(name)
	#define SEDX_SCOPE_TIMER(name)
#endif

}

// -------------------------------------------------------
