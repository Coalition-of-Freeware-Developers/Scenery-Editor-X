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
#include <SceneryEditorX/core/base.hpp>
#include <SceneryEditorX/core/initializer.h>
#include <SceneryEditorX/core/window.h>
#include <SceneryEditorX/platform/settings.h>

/// -------------------------------------------------------

namespace SceneryEditorX
{
    class Application
    {
    public:
        explicit Application(const WindowData &appData);
		virtual ~Application() = default;

		void Run();
        void Stop();

		virtual void OnInit() {}
        virtual void OnUpdate() {}
        virtual void OnShutdown();

        [[nodiscard]] Window &GetWindow() const { return *window; }
        GLOBAL Application &Get() { return *instance; }
        GLOBAL const char *GetConfigType();
        GLOBAL const char *GetPlatform();

		ApplicationSettings &GetSettings() { return settings; }
        const ApplicationSettings &GetSettings() const { return settings;}
    private:
        Scope<Window> window;
        bool isRunning = true;
        bool isMinimized = false;
        INTERNAL Application *instance;

		ApplicationSettings settings;

		friend class GraphicsEngine;
        friend class Renderer;
    protected:
        inline LOCAL bool isRuntime = false;
    };

    /// -------------------------------------------------------

    Application *CreateApplication(int argc, char **argv);

} // namespace SceneryEditorX

/// -------------------------------------------------------
