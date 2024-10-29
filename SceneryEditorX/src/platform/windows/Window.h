#pragma once

#include "../src/core/Base.h"
#include "../src/xpeditorpch.h"

namespace SceneryEditorX
{
	struct WindowProperties
	{
        std::string Title = "Scenery Editor X";
        uint32_t Width = 1280;
        uint32_t Height = 720;
        bool Fullscreen = false;
        bool VSync = true;
	};

	class Window
	{
	public:
		using EventCallbackFn = std::function<void()>;

        Window(const WindowProperties& properties);
        virtual ~Window();

        virtual void Initialize();
        virtual void ProcessEvent();
        virtual void SwapBuffers();

		inline uint32_t GetWidth() const { return WinData.Width; }
        inline uint32_t GetHeight() const { return WinData.Height; }

		virtual void SetEventCallback(const EventCallbackFn& callback) = 0;
		virtual void SetVSync(bool enabled) = 0;
		virtual bool IsVSync() const = 0;

		virtual void* GetNativeWindow() const = 0;

	private:
        virtual void Shutdown();

    private:
        GLFWwindow *m_Window;
        GLFWcursor *m_ImGuiMouseCursors[9] = {0};
        WindowProperties m_Specification;
        struct WindowData
        {
            std::string Title;
            uint32_t Width, Height;

            EventCallbackFn EventCallback;
        };

        WindowData WinData;
	};
}
