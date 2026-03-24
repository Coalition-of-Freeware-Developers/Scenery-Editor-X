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
 * application.h
 * -------------------------------------------------------
 * Created: 25/5/2025
 * -------------------------------------------------------
 */
#pragma once
#include "application_data.h"
#include <deque>
#include <Editor/ui/ui_layer.h>
#include <SceneryEditorX/core/events/application_events.h>
#include <SceneryEditorX/core/events/event_system.h>
#include <SceneryEditorX/core/layers/layer_stack.h>
#include <SceneryEditorX/core/platform/platform_context.h>
#include <SceneryEditorX/core/threading/render_thread.h>
#include <SceneryEditorX/core/time/time.h>
#include <SceneryEditorX/core/time/timer.h>
#include <SceneryEditorX/core/window/window.h>
#include <SceneryEditorX/settings/settings.h>
#include <SceneryEditorX/utils/pointers.h>

// -------------------------------------------------------

namespace SceneryEditorX
{
	class Application
	{
	public:
		typedef std::function<void(Event &)> EventCallbackFn;

		// -------------------------------------------------------

		struct PerformanceTimers
		{
			float MainThreadWorkTime = 0.0f;
			float MainThreadWaitTime = 0.0f;
			float RenderThreadWorkTime = 0.0f;
			float RenderThreadWaitTime = 0.0f;
			float RenderThreadGPUWaitTime = 0.0f;
			float ScriptUpdate = 0.0f;
		};

		// -------------------------------------------------------

		Application(const PlatformContext& context);
		Application(const PlatformContext& context, const AppData& appData);
		virtual ~Application();

		virtual void Run();
		virtual void OnRender();
		void RenderUI();
		virtual void Tick();
		virtual void Stop();

		virtual void OnInit() {}
		virtual void OnUpdate() {}
		virtual void OnShutdown();

		void PushLayer(Layer *layer);
		void PushOverlay(Layer *layer);
		void PopLayer(Layer *layer);
		void PopOverlay(Layer *layer);

		// -------------------------------------------------------

		DeltaTime GetDeltaTime() const { return m_DeltaTime; }
		DeltaTime GetFrameTime() const { return m_FrameTime; }
		static float GetTime(); // TODO: This should be in "Platform"

		// -------------------------------------------------------

		PerformanceProfiler* GetPerformanceProfiler() const { return m_Profiler; }
		inline Window& GetWindow() { return *m_Window; }
		uint32_t GetCurrentFrameIndex() const { return m_CurrentFrameIndex; }
		const AppData &GetAppData() const { return m_AppData; }
		const PlatformContext* GetPlatformContext() const { return m_PlatformContext; }
		LayerStack& GetLayerStack() { return m_LayerStack; }
		PerformanceTimers m_PerformanceTimers;

		// -------------------------------------------------------

		static Application &Get() { return *s_AppInstance; }
		static const char* GetConfigurationName();
		static const char *GetPlatformName();
		static std::thread::id GetMainThreadID();
		static bool IsMainThread();

		// -------------------------------------------------------

		// Settings accessors (single authoritative instance for the app lifetime)
		Settings& GetSettings() { return m_Settings; }
		const Settings& GetSettings() const { return m_Settings; }

		// -------------------------------------------------------

		void SetEventCallback(const EventCallbackFn& eventCallback) { m_EventCallbacks.push_back(eventCallback); }
		void SyncEvents();
		void ProcessEvents();
		void OnEvent(Event &event);
		static bool OnWindowResize(const WindowResizeEvent &e);
		bool OnWindowMinimize(const WindowMinimizeEvent &e);
		bool OnWindowClose(WindowCloseEvent &e);

		// -------------------------------------------------------

		/**
		 * @brief Queues an event to be processed later.
		 * @tparam Func The type of the function to be queued.
		 * @param func The function to be queued.
		 */
		template <typename Func>
		void QueueEvent(Func&& func)
		{
			std::scoped_lock lock(m_EventQueueMutex);
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
				std::scoped_lock lock(m_EventQueueMutex);
				m_EventQueue.emplace_back(false, [event](){ Application::Get().OnEvent(*event); });
			}
		}

	private:
		void InitializeApplication(const AppData& appData);

		Scope<Window> m_Window;
		AppData m_AppData;
		UILayer* m_UILayer;
		LayerStack m_LayerStack;
		DeltaTime m_DeltaTime;
		DeltaTime m_FrameTime;
		float m_LastFrameTime = 0.0f;
		bool m_IsRunning = true;
		bool m_IsMinimized = false;
		bool m_ShowStats = true;

		const PlatformContext* m_PlatformContext = nullptr;
		Settings m_Settings = Settings(std::filesystem::path("settings.cfg"));
		static Application *s_AppInstance;
		PerformanceProfiler *m_Profiler = nullptr; // TODO: Should be null in Dist
		std::unordered_map<const char *, PerformanceProfiler::PerFrameData> m_ProfilerPreviousFrameData;
		std::deque<std::pair<bool, std::function<void()>>> m_EventQueue;
		std::mutex m_EventQueueMutex;
		std::vector<EventCallbackFn> m_EventCallbacks;
		RenderThread m_RenderThread;
		uint32_t m_CurrentFrameIndex = 0;
	protected:
		inline static bool m_IsRunningTime = false;
	};

	// -------------------------------------------------------

	Application *CreateApplication(const std::vector<std::string> &args);
	Application *CreateApplication(const PlatformContext& context);

}

// -------------------------------------------------------
