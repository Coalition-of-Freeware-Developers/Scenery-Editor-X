/**
* -------------------------------------------------------
* Scenery Editor X
* -------------------------------------------------------
* Copyright (c) 2025 Thomas Ray 
* Copyright (c) 2025 Coalition of Freeware Developers
* -------------------------------------------------------
* application.h
* -------------------------------------------------------
* Created: 23/4/2025
* -------------------------------------------------------
*/
#pragma once
#include <imgui/imgui.h>
#include <queue>
#include <SceneryEditorX/core/application_data.h>
#include <SceneryEditorX/core/layer.h>
#include <vulkan/vulkan.h>

/// -------------------------------------------------------

void check_vk_result(VkResult err);

struct GLFWwindow;

/// -------------------------------------------------------

namespace Launcher
{

    /// -------------------------------------------------------

	class Application
	{
	public:
        Application(const SceneryEditorX::SoftwareStats &appData = SceneryEditorX::SoftwareStats());
        ~Application();

        static Application &Get();

		void Run();
		void SetMenubarCallback(const std::function<void()>& mainMenubarCallback) { menubarCallback = mainMenubarCallback; }
        void SetWindowUserData(void *userdata);
        void Close();

	    GLFWwindow* GetWindow() { return windowHandle; }
        float GetTime();
        static VkInstance GetInstance();

        static VkPhysicalDevice GetPhysicalDevice();
        static VkDevice GetDevice();

        static VkCommandBuffer GetCommandBuffer(bool begin);
        static void FlushCommandBuffer(VkCommandBuffer commandBuffer);

    private:
        void Init();
        void Shutdown();

        SceneryEditorX::SoftwareStats appSpecification;
        GLFWwindow *windowHandle = nullptr;
        bool isRunning = false;

        float deltaTime = 0.0f;
        float frameTime = 0.0f;
        float prevFrameTime = 0.0f;

        std::vector<std::shared_ptr<SceneryEditorX::Layer>> layerStack;
        std::function<void()> menubarCallback;
        std::queue<std::function<void()>> eventQueue;

        bool dockspace = false;
	};

	Application *CreateApplication(int argc, char **argv);

    /// -------------------------------------------------------


} // namespace Launcher

/// -------------------------------------------------------
