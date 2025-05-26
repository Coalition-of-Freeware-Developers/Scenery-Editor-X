/**
* -------------------------------------------------------
* Scenery Editor X
* -------------------------------------------------------
* Copyright (c) 2025 Thomas Ray
* Copyright (c) 2025 Coalition of Freeware Developers
* -------------------------------------------------------
* launcher_main.h
* -------------------------------------------------------
* Created: 17/3/2025
* -------------------------------------------------------
*/

#pragma once
#include <GraphicsEngine/vulkan/render_data.h>
#include <GraphicsEngine/vulkan/vk_core.h>
#include <Launcher/core/splash_handler.h>
#include <SceneryEditorX/core/window.h>
#include <SceneryEditorX/ui/ui.h>
#include <SceneryEditorX/ui/ui_context.h>
// -------------------------------------------------------

namespace Launcher
{
    class Loader
    {
    public:
		Loader() = default;
		~Loader() = default;

        void run();

		//Ref<Window> GetWindow() { return vkRenderer.GetWindow(); }

    private:
        SplashHandler *splashHandler;

        //GraphicsEngine vkRenderer;
        //SwapChain vkSwapChain;
        //Ref<VulkanDevice> vkDevice;

        //UI::GUI ui;
        //Ref<UI::UIContext> uiContext;

        //Viewport viewportData;
        //RenderData renderData;
        //VulkanDeviceFeatures vkDeviceFeatures;
        uint32_t currentFrame = 0;

        //Counter to make sure we wait for all threads to finish
        std::mutex mtThreadCounter;
        int intThreadCount{0};

		// ---------------------------------------------------------

		static void InitPreloader();
        void PerformPreloading();
        void CreateSplash();
        void MainLoop();
        void OperationThreads();
        void CleanUp() const;
		void CreateViewportResources();
        void CleanupViewportResources();
        void OnSurfaceUpdate(uint32_t width, uint32_t height);
        void RecreateFrameResources();
		void DrawFrame();
    };

    //GLOBAL void AdminCheck();

} // namespace Launcher
