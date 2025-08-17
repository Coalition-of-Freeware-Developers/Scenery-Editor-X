/**
* -------------------------------------------------------
* Scenery Editor X
* -------------------------------------------------------
* Copyright (c) 2025 Thomas Ray 
* Copyright (c) 2025 Coalition of Freeware Developers
* -------------------------------------------------------
* application.h
* -------------------------------------------------------
* Created: 25/5/2025
* -------------------------------------------------------
*/
#pragma once
#include <deque>
#include "application_data.h"
#include "SceneryEditorX/core/events/application_events.h"
#include "SceneryEditorX/core/events/event_system.h"
#include "SceneryEditorX/core/modules/module_stage.h"
#include "SceneryEditorX/core/time/time.h"
#include "SceneryEditorX/core/time/timer.h"
#include "SceneryEditorX/core/window/window.h"
#include "SceneryEditorX/platform/settings/settings.h"
#include "SceneryEditorX/utils/pointers.h"
#include "SceneryEditorX/utils/static_states.h"

/// -------------------------------------------------------

namespace SceneryEditorX
{
	class Application
    {
    public:
	    using EventCallbackFn = std::function<void(Event&)>;

        /// -------------------------------------------------------

        struct PerformanceTimers
        {
            float MainThreadWorkTime = 0.0f;
            float MainThreadWaitTime = 0.0f;
            float RenderThreadWorkTime = 0.0f;
            float RenderThreadWaitTime = 0.0f;
            float RenderThreadGPUWaitTime = 0.0f;
            float ScriptUpdate = 0.0f;
        };

        /// -------------------------------------------------------

        Application(const AppData &appData);
        virtual ~Application();

		void Run();
        void Stop();

		virtual void OnInit() {}
        virtual void OnUpdate() {}
        virtual void OnShutdown();

		DeltaTime GetDeltaTime() const { return m_deltaTime; }
		DeltaTime GetFrametime() const { return m_frametime; }
		float GetTime() const; /// TODO: This should be in "Platform"

	    PerformanceProfiler* GetPerformanceProfiler() const { return m_profiler; }
        inline Window& GetWindow() { return *m_Window; }
        uint32_t GetCurrentFrameIndex() const { return currentFrameIndex; }
		const AppData &GetAppData() const { return m_AppData; }

        GLOBAL Application &Get() { return *appInstance; }
	    GLOBAL const char* GetConfigurationName();
        GLOBAL const char *GetPlatformName();
        GLOBAL std::thread::id GetMainThreadID();
        GLOBAL bool IsMainThread();

        ApplicationSettings &GetSettings() { return settings; }
        [[nodiscard]] const ApplicationSettings &GetSettings() const { return settings;}
        void RenderUI();
	    void SetShowStats(bool show) { m_ShowStats = show; }

	    void AddEventCallback(const EventCallbackFn& eventCallback) { m_EventCallbacks.push_back(eventCallback); }
        void SyncEvents();
        void ProcessEvents();
        void OnEvent(Event &event);
        bool OnWindowResize(const WindowResizeEvent &e);
        bool OnWindowMinimize(const WindowMinimizeEvent &e);
        bool OnWindowClose(WindowCloseEvent &e);

		template<typename Func>
		void QueueEvent(Func&& func)
		{
			std::scoped_lock<std::mutex> lock(m_EventQueueMutex);
			m_EventQueue.emplace_back(true, func);
		}

		// Creates & Dispatches an event either immediately, or adds it to an event queue which will be processed after the next call
		// to SyncEvents().
		// Waiting until after next sync gives the application some control over _when_ the events will be processed.
		// An example of where this is useful:
		// Suppose an asset thread is loading assets and dispatching "AssetReloaded" events.
		// We do not want those events to be processed until the asset thread has synced its assets back to the main thread.
		template<typename TEvent, bool DispatchImmediately = false, typename... TEventArgs>
		void DispatchEvent(TEventArgs&&... args)
		{
#ifndef SEDX_COMPILER_GCC
			// TODO: GCC causes this to fail for AnimationGraphCompiledEvent for some reason. Investigate.
			static_assert(std::is_assignable_v<Event, TEvent>);
#endif

			std::shared_ptr<TEvent> event = std::make_shared<TEvent>(std::forward<TEventArgs>(args)...);
			if constexpr (DispatchImmediately)
			{
				OnEvent(*event);
			}
			else
			{
				std::scoped_lock<std::mutex> lock(m_EventQueueMutex);
				m_EventQueue.emplace_back(false, [event](){ Application::Get().OnEvent(*event); });
			}
		}

	private:
        Scope<Window> m_Window;
        WindowData m_WindowData;
        AppData m_AppData;
        ModuleStage m_ModuleStage;

		DeltaTime m_deltaTime;
		DeltaTime m_frametime;
	    bool isRunning = true;
        bool isMinimized = false;
        bool m_ShowStats = true;

        ApplicationSettings settings = ApplicationSettings(std::filesystem::path("settings.cfg"));
        INTERNAL Application *appInstance;
        PerformanceProfiler *m_profiler = nullptr; /// TODO: Should be null in Dist
        std::unordered_map<const char *, PerformanceProfiler::PerFrameData> m_ProfilerPreviousFrameData;
        std::deque<std::pair<bool, std::function<void()>>> m_EventQueue;
        std::mutex m_EventQueueMutex;
        std::vector<EventCallbackFn> m_EventCallbacks;

		uint32_t currentFrameIndex = 0;
        //friend class RenderContext;
        //friend class Renderer;
    protected:
        inline LOCAL bool isRuntime = false;
    };

    Application *CreateApplication(int argc, char **argv);

}

/// -------------------------------------------------------
