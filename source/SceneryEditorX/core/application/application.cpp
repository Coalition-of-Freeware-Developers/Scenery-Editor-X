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
 * application.cpp
 * -------------------------------------------------------
 * Created: 25/5/2025
 * -------------------------------------------------------
 */
#include "application.h"
#include <SceneryEditorX/core/frame_profiler.h>
#include <SceneryEditorX/core/input/input.h>
#include <SceneryEditorX/core/platform/windows/crash_diagnostics.h>
#include <SceneryEditorX/core/resource/resource_cache.h>
#include <SceneryEditorX/core/threading/thread_pool.h>
#include <SceneryEditorX/core/time/fps_timer.h>
#include <SceneryEditorX/filesystem/file_system.h>
#include <SceneryEditorX/filesystem/vfs.h>
#include <SceneryEditorX/logging/logging.hpp>
#include <SceneryEditorX/logging/profiler.hpp>
#include <SceneryEditorX/project/project.h>
#include <SceneryEditorX/renderer/renderer.h>
#include <SceneryEditorX/renderer/ui/ui_layer.h>
#include <SceneryEditorX/renderer/vulkan/swapchain.h>
#include <SceneryEditorX/scene/scene.h>

// -------------------------------------------------------

bool appRunning = true; // Global variable to control the application loop

// -------------------------------------------------------

namespace SceneryEditorX
{
	Application *Application::s_AppInstance = nullptr;
	static std::thread::id s_MainThreadID;

	// -------------------------------------------------------

	/**
	 * @brief Helper function to initialize the application with common setup
	 * @param appData The application data configuration
	 */
	void Application::InitializeApplication(const AppData &appData)
	{
		m_AppData = appData;

		// Set working directory if specified
		if (!appData.workingDirectory.empty())
		{
			std::filesystem::current_path(appData.workingDirectory);
		}

		// Create window
		m_Window = CreateScope<Window>();
		SEDX_CORE_TRACE("Initializing Window");
		m_Window->Create();
		
		// Configure window properties BEFORE renderer init
		if (appData.splashScreen)
		{
			m_Window->SetResizable(false);
			m_Window->SetDecorated(false);
		}
		else
		{
			m_Window->SetResizable(appData.resizable);
			m_Window->SetDecorated(appData.decorated);
		}
		
		if (appData.startMaximized)
		{
			m_Window->Maximize();
		}
		else
		{
			m_Window->CenterWindow();
		}
		
		// Set event callback before renderer init (so it can handle any initialization events)
		m_Window->SetEventCallback([this](Event &e) { OnEvent(e); });
		m_IsMinimized = false;

		// The renderer's bootstrap uses the resource directories (standard meshes, textures, font), so they are
		// registered first (only path strings, built from the working directory set above)
		ResourceCache::Init();

		// Initialize RenderContext first so device/instance exist for Renderer
		RenderContext::Init();
		Renderer::Init();
		m_RenderThread.Run();

		// The UI layer needs the renderer's swapchain and the resource directories (fonts), so it attaches last.
		if (appData.enableUI)
		{
			m_UILayer = new UILayer(appData.enableUIViewports);
			PushOverlay(m_UILayer);
		}

		FPSTimer::Init();
		ThreadPool::Init();
		Scene::Init();

		m_IsRunning = true;
	}

	// -------------------------------------------------------

	void Application::SetUpAppLocations()
	{
		// Settings, data, caches, logs and temp files where the platform expects them (moving older versions' files once)
		for (const std::string &line : IO::FileSystem::InitAppLocations())
			SEDX_CORE_INFO_TAG("Files", "{}", line);
		const VFS::AppLayout layout = VFS::GetAppLayout();
		SEDX_CORE_INFO_TAG("Files", "Settings {} | data {} | cache {} | logs {}", VFS::ToUtf8(layout.settings), VFS::ToUtf8(layout.data),
			VFS::ToUtf8(layout.cache), VFS::ToUtf8(layout.logs));

		// Working folders: exchanges and autosaves stay on this machine, plugins travel with the settings
		for (const char *name : { "autosave", "downloads", "export", "import" })
			VFS::CreateFolders(layout.data / name);
		VFS::CreateFolders(layout.settings / "plugins");
	}

	Application::Application(const PlatformContext& context) : m_PlatformContext(&context), m_RenderThread(ThreadingPolicy::MultiThreaded)
	{
		Diagnostics::InstallCrashHandlers();
		s_AppInstance = this;
		s_MainThreadID = std::this_thread::get_id();

		SEDX_CORE_TRACE("Working directory set to: {}", std::filesystem::current_path().string());

		// -------------------------------------------------------

		SEDX_CORE_TRACE("=== Initializing Application with PlatformContext ===");
		SEDX_CORE_TRACE("  Working Directory: {}", context.GetWorkingDirectory());
		SEDX_CORE_TRACE("  Temp Directory: {}", context.GetTempDirectory());
		SEDX_CORE_TRACE("  Command Line Args: {}", context.GetCommandLineArgs().size());

		AppData specification;
		specification.coreThreadingPolicy = ThreadingPolicy::MultiThreaded;

		// Apply platform context settings to app data
		if (!context.GetWorkingDirectory().empty())
		{
			specification.workingDirectory = context.GetWorkingDirectory();
		}

		SetUpAppLocations();

		InitializeApplication(specification);
	}

	Application::Application(const PlatformContext& context, const AppData& appData) : m_PlatformContext(&context), m_RenderThread(appData.coreThreadingPolicy)
	{
		Diagnostics::InstallCrashHandlers();
		s_AppInstance = this;
		s_MainThreadID = std::this_thread::get_id();

		// Set working directory to application root (2 levels up from bin/Debug)
		std::filesystem::path exePath = std::filesystem::current_path();
		std::filesystem::path repoRoot = exePath.parent_path().parent_path();
		std::filesystem::current_path(repoRoot);

		SEDX_CORE_TRACE("Executable directory: {}", exePath.string());
		SEDX_CORE_TRACE("Repository root: {}", repoRoot.string());
		SEDX_CORE_TRACE("Working directory set to: {}", std::filesystem::current_path().string());

		// -------------------------------------------------------

		SEDX_CORE_TRACE("=== Initializing Application with PlatformContext ===");
		SEDX_CORE_TRACE("  Working Directory: {}", context.GetWorkingDirectory());
		SEDX_CORE_TRACE("  Temp Directory: {}", context.GetTempDirectory());
		SEDX_CORE_TRACE("  Command Line Args: {}", context.GetCommandLineArgs().size());

		SetUpAppLocations();

		AppData specification = appData;

		// Apply platform context settings if not already set in appData
		if (specification.workingDirectory.empty() && !context.GetWorkingDirectory().empty())
		{
			specification.workingDirectory = context.GetWorkingDirectory();
		}

		InitializeApplication(specification);
	}

	Application::~Application()
	{
		m_Window->SetEventCallback([](Event&) {});

		// Stop producing new work first
		m_RenderThread.Terminate();

		// Detach and destroy layers in reverse order (dependencies unwind correctly). The UI layer is an
		// overlay, so it goes first: client layers must not touch ImGui in OnDetach().
		for (size_t i = m_LayerStack.Size(); i > 0; --i)
		{
			Layer* layer = m_LayerStack[i - 1];
			layer->OnDetach();
			delete layer;
		}
		m_UILayer = nullptr;

		// Release shared resource owners/caches
		ResourceCache::UnloadDefaultResources();
		ResourceCache::Shutdown();
		Scene::Shutdown();

		// Shutdown systems that may hold/consume resources
		Renderer::Shutdown();
		ThreadPool::Shutdown();

		// Destroy the Vulkan device and instance while logging is still up (Log shuts down after this).
		RenderContext::Shutdown();
	}

	void Application::Tick()
	{
		Application* app = this;
		Input::Tick();
		Window::Tick();	// Per-frame housekeeping

		// Execute UI render calls directly for now to avoid relying on the
		// renderer submission system while that API is being refactored.
		app->RenderUI();
		m_RenderThread.Tick();
		FPSTimer::PostTick();
	}

	void Application::Run()
	{
		OnInit(); // Call user-defined initialization function

		static uint64_t frameCount = 0;

		// Main application loop
		while (m_IsRunning && !m_Window->GetShouldClose())
		{
			Timer timer;

			// Wait for render thread to finish frame
			{
				SEDX_PERF_SCOPE("Main: wait for render thread");
				m_RenderThread.BlockUntilRenderComplete();
				m_PerformanceTimers.MainThreadWaitTime = timer.ElapsedMillis();
			}
			Perf::EndFrame();

			if (!m_IsMinimized)
			{
				SEDX_PERF_SCOPE("Main: update + UI");

				// Poll events
				ProcessEvents();

				for (size_t i = 0; i < m_LayerStack.Size(); ++i)
				{
					m_LayerStack[i]->Tick();
				}

				OnUpdate();
				OnRender();
				Tick();

				// Start rendering previous frame
				m_RenderThread.Kick();
				m_CurrentFrameIndex = (m_CurrentFrameIndex + 1) % 2;
				m_PerformanceTimers.MainThreadWorkTime = timer.ElapsedMillis();

				frameCount++;
			}
		}

		Input::ClearReleasedKeys();
		m_RenderThread.BlockUntilRenderComplete();
		float time = GetTime();
		m_FrameTime = time - m_LastFrameTime;
		m_DeltaTime = xMath::Min<float>(static_cast<float>(m_FrameTime), 0.0333f);
		m_LastFrameTime = time;

		//SEDX_CORE_INFO_TAG("Application", "=== Exiting Application Main Loop (frames rendered: {}) ===", frameCount);
		OnShutdown();
	}

	void Application::Stop() { m_IsRunning = false; }

	void Application::OnRender()
	{
		SEDX_CORE_TRACE_TAG("Application", "OnRender() has been called");
	}

	void Application::RenderUI()
	{
		//SEDX_PROFILE_FUNC();
		//SEDX_SCOPE_PERF("Application::RenderUI");
		for (size_t i = 0; i < m_LayerStack.Size(); i++)
		{
			m_LayerStack[i]->OnRender();
		}

		if (!m_UILayer || !m_UILayer->IsInitialized())
			return;

		m_UILayer->BeginRendering();
		for (size_t i = 0; i < m_LayerStack.Size(); i++)
		{
			m_LayerStack[i]->OnUIRender();
		}
		m_UILayer->EndRendering();
	}

	void Application::OnShutdown()
	{
		SEDX_CORE_INFO_TAG("Application", "Application::OnShutdown()");
		SEDX_CORE_TRACE("Shutting down application");
		m_EventCallbacks.clear();
		appRunning = false;
	}

	void Application::PushLayer(Layer *layer)
	{
		m_LayerStack.PushLayer(layer);
		layer->OnAttach();
	}

	void Application::PushOverlay(Layer *layer)
	{
		m_LayerStack.PushOverlay(layer);
		layer->OnAttach();
	}

	void Application::PopLayer(Layer *layer)
	{
		m_LayerStack.PopLayer(layer);
		layer->OnDetach();
	}

	void Application::PopOverlay(Layer *layer)
	{
		m_LayerStack.PopOverlay(layer);
		layer->OnDetach();
	}

	void Application::SyncEvents()
	{
		std::scoped_lock lock(m_EventQueueMutex);
		for (auto &synced : m_EventQueue | std::views::keys)
		{
			synced = true;
		}
	}

	void Application::ProcessEvents()
	{
		Input::TransitionPressedKeys();
		Input::TransitionPressedButtons();
		Window::ProcessEvents();

		/*
		 * Process custom event queue up until we encounter an event that is not yet synced.
		 * If the application queues such events, it is the application's responsibility to call
		 * SyncEvents() at the appropriate time.
		 */
		while (true)
		{
			std::function<void()> func;
			{
				std::scoped_lock lock(m_EventQueueMutex);
				if (m_EventQueue.empty() || !m_EventQueue.front().first)
				{
					break;
				}

				func = std::move(m_EventQueue.front().second);
				m_EventQueue.pop_front();
			}

			if (func)
			{
				func();
			}
		}
	}

	void Application::OnEvent(Event& event)
	{
		EventDispatcher dispatcher(event);
		dispatcher.Dispatch<WindowResizeEvent>([this](const WindowResizeEvent& e) { return OnWindowResize(e); });
		dispatcher.Dispatch<WindowMinimizeEvent>([this](const WindowMinimizeEvent& e) { return OnWindowMinimize(e); });
		dispatcher.Dispatch<WindowCloseEvent>([this](WindowCloseEvent& e) { return OnWindowClose(e); });
		
		for (auto it = m_LayerStack.End(); it != m_LayerStack.Begin(); )
		{
			(*--it)->OnEvent(event);
			if (event.m_Handled) break;
		}

		if (event.m_Handled) return;

		/**
		 * TODO: Should these callbacks be called BEFORE the layers receive events?
		 * We may actually want that since most of these callbacks will be functions REQUIRED in order for the game
		 * to work, and if a layer has already handled the event we may end up with problems.
		 */
		for (auto& eventCallback : m_EventCallbacks)
		{
			eventCallback(event);
			if (event.m_Handled) break;
		}

	}

	bool Application::OnWindowResize(const WindowResizeEvent& e)
	{
		const uint32_t height = e.GetHeight();
		if (const uint32_t width = e.GetWidth(); width == 0 || height == 0)
		{
			//m_IsMinimized = true;
			return false;
		}
		//m_IsMinimized = false;

		return false;
	}

	bool Application::OnWindowMinimize(const WindowMinimizeEvent& e)
	{
		m_IsMinimized = e.IsMinimized();
		return false;
	}

	bool Application::OnWindowClose(WindowCloseEvent& e)
	{
		Stop();
		return false; // give other things a chance to react to window close
	}

	float Application::GetTime() { return Time::GetTime(); }
	const char* Application::GetConfigurationName() { return SEDX_BUILD_TYPE; }
	const char* Application::GetPlatformName() { return SEDX_PLATFORM_NAME; }
	std::thread::id Application::GetMainThreadID() { return s_MainThreadID; }
	bool Application::IsMainThread() { return std::this_thread::get_id() == s_MainThreadID; }

}

// -------------------------------------------------------------------------
