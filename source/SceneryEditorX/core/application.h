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
#include <SceneryEditorX/core/pointers.h>
#include <SceneryEditorX/core/time.h>
#include <SceneryEditorX/core/window/window.h>
#include <SceneryEditorX/platform/platform_states.h>
#include <SceneryEditorX/platform/settings.h>

/// -------------------------------------------------------

namespace SceneryEditorX
{
	class Application
    {
    public:
        Application(const AppData &appData);
		virtual ~Application() = default;

		void Run();
        void Stop();

		virtual void OnInit() {}
        virtual void OnUpdate() {}
        virtual void OnShutdown();

        [[nodiscard]] Window &GetWindow() const { return *m_Window; }

        GLOBAL Application &Get() { return *appInstance; }
        GLOBAL const char *GetConfigType();
        GLOBAL const char *GetPlatform();
        void RenderUI();
        float GetTime();

        ApplicationSettings &GetSettings() { return settings; }
        [[nodiscard]] const ApplicationSettings &GetSettings() const { return settings;}
    private:
        Scope<Window> m_Window;
        std::deque<std::pair<bool, std::function<void()>>> m_EventQueue;

	    bool isRunning = true;
        bool isMinimized = false;

        ApplicationSettings settings = ApplicationSettings(std::filesystem::path("default_config_path.cfg"));
        INTERNAL Application *appInstance;

        friend class RenderContext;
        friend class Renderer;
    protected:
        inline LOCAL bool isRuntime = false;
    };


    Application *CreateApplication(int argc, char **argv);

} // namespace SceneryEditorX

/// -------------------------------------------------------
