/**
* -------------------------------------------------------
* Scenery Editor X
* -------------------------------------------------------
* Copyright (c) 2025 Thomas Ray 
* Copyright (c) 2025 Coalition of Freeware Developers
* -------------------------------------------------------
* timer.h
* -------------------------------------------------------
* Created: 14/7/2025
* -------------------------------------------------------
*/
#pragma once

/// -------------------------------------------------------

namespace SceneryEditorX
{
	
	class Timer
	{
	public:
		SEDX_FORCE_INLINE Timer() { Reset(); }
		SEDX_FORCE_INLINE void Reset() { m_Start = std::chrono::high_resolution_clock::now(); }
		SEDX_FORCE_INLINE float Elapsed() { return std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::high_resolution_clock::now() - m_Start).count() * 0.001f * 0.001f; }
		SEDX_FORCE_INLINE float ElapsedMillis() { return std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::high_resolution_clock::now() - m_Start).count() * 0.001f; }
	private:
		std::chrono::time_point<std::chrono::high_resolution_clock> m_Start;
	};
	
	class ScopedTimer
	{
	public:
        explicit ScopedTimer(std::string name) : m_Name(std::move(name)) {}
		~ScopedTimer()
		{
			float time = m_Timer.ElapsedMillis();
			SEDX_CORE_TRACE_TAG("Timer", "{0} - {1}ms", m_Name, time);
		}
	private:
		std::string m_Name;
		Timer m_Timer;
	};

	class PerformanceProfiler
	{
	public:
		struct PerFrameData
		{
			float Time = 0.0f;
			uint32_t Samples = 0;

			PerFrameData() = default;
			PerFrameData(float time) : Time(time) {}

			operator float() const { return Time; }
			inline PerFrameData& operator+=(float time)
			{
				Time += time;
			}
		};
	public:
		void SetPerFrameTiming(const char* name, float time)
		{
			std::scoped_lock<std::mutex> lock(m_PerFrameDataMutex);

			if (!m_PerFrameData.contains(name))
				m_PerFrameData[name] = 0.0f;

			PerFrameData& data = m_PerFrameData[name];
			data.Time += time;
			data.Samples++;
		}

		void Clear()
		{
			std::scoped_lock<std::mutex> lock(m_PerFrameDataMutex);
			m_PerFrameData.clear();
		}

		const std::unordered_map<const char*, PerFrameData>& GetPerFrameData() const { return m_PerFrameData; }
	private:
		std::unordered_map<const char*, PerFrameData> m_PerFrameData;
		inline static std::mutex m_PerFrameDataMutex;
	};

	class ScopePerfTimer
	{
	public:
		ScopePerfTimer(const char* name, PerformanceProfiler* profiler) : m_Name(name), m_Profiler(profiler) {}

		~ScopePerfTimer()
		{
			float time = m_Timer.ElapsedMillis();
			m_Profiler->SetPerFrameTiming(m_Name, time);
		}
	private:
		const char* m_Name;
		PerformanceProfiler* m_Profiler;
		Timer m_Timer;
	};

#if 1
	#define SEDX_SCOPE_PERF(name)\
	    ScopePerfTimer timer__LINE__(name, Application::Get().GetPerformanceProfiler());

    #define SEDX_SCOPE_TIMER(name)\
	    ScopedTimer timer__LINE__(name);
#else
	#define SEDX_SCOPE_PERF(name)
	#define SEDX_SCOPE_TIMER(name)
#endif
}

/// -------------------------------------------------------
