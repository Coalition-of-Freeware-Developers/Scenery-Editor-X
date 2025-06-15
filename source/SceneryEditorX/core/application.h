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
#include <SceneryEditorX/core/application_data.h>
#include <SceneryEditorX/core/window/window.h>
#include <SceneryEditorX/core/pointers.h>
#include <SceneryEditorX/core/time.h>
#include <SceneryEditorX/platform/platform_states.h>
#include <SceneryEditorX/platform/settings.h>

/// -------------------------------------------------------

namespace SceneryEditorX
{
	class Application
    {
    public:
        Application(const AppData &appData);
		virtual ~Application();

		void Run();
        void Stop();

		virtual void OnInit() {}
        virtual void OnUpdate() {}
        virtual void OnShutdown();

        inline Window GetWindow() const { return *m_Window; }
        uint32_t GetCurrentFrameIndex() const { return currentFrameIndex; }
		const AppData &GetAppData() const { return m_AppData; }
        GLOBAL Application &Get() { return *appInstance; }
	    GLOBAL const char* GetConfigurationName();
		GLOBAL const char* GetPlatformName();
        ApplicationSettings &GetSettings() { return settings; }
        [[nodiscard]] const ApplicationSettings &GetSettings() const { return settings;}
        void RenderUI();
        float GetTime();
	private:
        Scope<Window> m_Window;
        WindowData m_WindowData;
        AppData m_AppData;
        std::deque<std::pair<bool, std::function<void()>>> m_EventQueue;
	    bool isRunning = true;
        bool isMinimized = false;

        ApplicationSettings settings = ApplicationSettings(std::filesystem::path("default_config_path.cfg"));
        INTERNAL Application *appInstance;

		uint32_t currentFrameIndex = 0;

        friend class RenderContext;
        friend class Renderer;
    protected:
        inline LOCAL bool isRuntime = false;
    };

    Application *CreateApplication(int argc, char **argv);

} // namespace SceneryEditorX

/// -------------------------------------------------------
