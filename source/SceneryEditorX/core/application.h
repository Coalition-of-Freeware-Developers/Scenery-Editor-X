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
#include <SceneryEditorX/core/time.h>
#include <SceneryEditorX/core/window/window.h>
#include <SceneryEditorX/platform/settings.h>
#include <deque>

/// -------------------------------------------------------

namespace SceneryEditorX
{
    class Application
    {
    public:
        Application(const WindowData &appData);
		virtual ~Application() = default;

		void Run();
        void Stop();

		virtual void OnInit() {}
        virtual void OnUpdate() {}
        virtual void OnShutdown();

        inline Window &GetWindow() { return *m_Window; }
        GLOBAL inline Application &Get() { return *instance; }
        GLOBAL const char *GetConfigType();
        GLOBAL const char *GetPlatform();
        void RenderUI();

		ApplicationSettings &GetSettings() { return settings; }
        const ApplicationSettings &GetSettings() const { return settings;}
    private:
        bool isRunning = true;
        bool isMinimized = false;

        std::unique_ptr<Window> m_Window;
        std::deque<std::pair<bool, std::function<void()>>> m_EventQueue;

        INTERNAL Application *instance;
        INTERNAL Application *s_Instance;

		ApplicationSettings settings;

		friend class RenderContext;
        friend class Renderer;

    protected:
        inline LOCAL bool isRuntime = false;

    };

    /// -------------------------------------------------------

    Application *CreateApplication(int argc, char **argv);

    /// -------------------------------------------------------

} // namespace SceneryEditorX

/// -------------------------------------------------------
