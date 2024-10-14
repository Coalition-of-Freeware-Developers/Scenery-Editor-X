
#include "../pch.h"
#include "MainWindow.h"

#include "../src/platform/windows/Window.h"
#include "../src/platform/windows/WindowProperties.h"

namespace SceneryEditorX 
{

	MainWindow::MainWindow(const WindowProperties& properties)
	{
		SceneryEditorX::Log::Init();

		Init(properties);
	}

	void MainWindow::Init(const WindowProperties& properties)
	{
		Window = glfwCreateWindow((int)properties.Width, (int)properties.Height, m_Data.Title.c_str(), nullptr, nullptr);
        if (window == nullptr)
            return nullptr;
        return window;
    }

	const unsigned int width = 1400;
	const unsigned int height = 810;
}
