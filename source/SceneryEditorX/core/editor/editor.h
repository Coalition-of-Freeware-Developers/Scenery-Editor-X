/**
* -------------------------------------------------------
* Scenery Editor X
* -------------------------------------------------------
* Copyright (c) 2025 Thomas Ray 
* Copyright (c) 2025 Coalition of Freeware Developers
* -------------------------------------------------------
* editor.h
* -------------------------------------------------------
* Created: 13/4/2025
* -------------------------------------------------------
*/
#pragma once
#include <SceneryEditorX/core/window.h>
#include <SceneryEditorX/renderer/render_data.h>
#include <SceneryEditorX/renderer/vk_core.h>
//#include <SceneryEditorX/scene/asset_manager.h>
#include <SceneryEditorX/ui/ui.h>
#include <SceneryEditorX/ui/ui_context.h>

// ---------------------------------------------------------

namespace SceneryEditorX
{
	class EditorApplication
	{
	public:
        EditorApplication() = default;
        ~EditorApplication();

	    void run()
	    {
	        InitEditor();
	        Create();
	        MainLoop();
	    }

        Ref<Window> GetWindow() { return vkRenderer.GetWindow(); }
	
	private:
        GraphicsEngine vkRenderer;
        Ref<SwapChain> vkSwapChain;

        //AssetManager assetManager;
        UI::GUI ui;
        Ref<UI::UIContext> uiContext;

        Viewport viewportData;
        RenderData renderData;
        VulkanDeviceFeatures vkDeviceFeatures;
        uint32_t currentFrame = 0;

		// ---------------------------------------------------------

	    glm::ivec2 newViewportSize = viewportData.viewportSize;
	    uint32_t frameCount = 0;

	    VkDevice device = VK_NULL_HANDLE;

        static void InitEditor();
        void Create();
        void MainLoop();
        void CreateViewportResources();
        void CleanupViewportResources();
        void OnSurfaceUpdate(uint32_t width, uint32_t height);
        void RecreateFrameResources();
        void DrawFrame();

    };


} // namespace SceneryEditorX

// -------------------------------------------------------
