#pragma once
#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <GLFW/glfw3.h>
#include "../src/platform/windows/WindowProperties.h"

namespace SceneryEditorX
{
    GLFWwindow *createWindow(int width, int height, const char *title);

    class MainWindow
    {
    public:

        MainWindow(const WindowProperties &properties);
        virtual ~MainWindow();

        void OnUpdate();
        void SetVSync(bool enabled);
        bool IsVSync() const;

        void SetEventCallback(const EventCallbackFn &callback)
        {
            m_Data.EventCallback = callback;
        }

        void *GetNativeWindow() const
        {
            return m_Window;
        }

        unsigned int GetWidth() const
        {
            return m_Data.Width;
        }

        unsigned int GetHeight() const
        {
            return m_Data.Height;
        }

        static Scope<MainWindow> Create(const WindowProperties &properties);

    private:
        virtual void Init(const WindowProperties &properties);
        virtual void Shutdown();
    
    private:
        GLFWwindow *m_Window;
        Scope<GraphicsContext> m_Contex;
    
        struct WindowData
        {
            std::string Title;
            unsigned int Width, Height;
            bool VSync;
    
            EventCallbackFn EventCallback;
        };
    
        WindowData m_Data;
    };
}

#endif // MAINWINDOW_H
