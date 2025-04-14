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
#include <SceneryEditorX/renderer/vk_core.h>
#include <SceneryEditorX/scene/asset_manager.h>
#include <SceneryEditorX/ui/ui.h>

// ---------------------------------------------------------

namespace SceneryEditorX
{
	class EditorApplication
	{
	public:
        EditorApplication();
        ~EditorApplication();

	    void run()
	    {
	        InitEditor();
	        Create();
	        MainLoop();
	    }
	    //bool GetSwapChainDirty();
	
	private:
	    GLFWwindow *window = nullptr;
	    GraphicsEngine vkRenderer;
        //AssetManager assetManager;
        GUI ui;

		// ---------------------------------------------------------

	    uint32_t currentFrame = 0;
	    glm::ivec2 viewportSize = {64, 48};
	    glm::ivec2 newViewportSize = viewportSize;
	    bool viewportHovered = false;
	    bool viewportResized = false;
	    bool fullscreen = false;
	    bool swapChainDirty = true;
	    uint32_t frameCount = 0;

	
	    VkImage viewportImage = VK_NULL_HANDLE;
	    VkDeviceMemory viewportImageMemory = VK_NULL_HANDLE;
	    VkImageView viewportImageView = VK_NULL_HANDLE;
	    VkFramebuffer viewportFramebuffer = VK_NULL_HANDLE;
	    VkRenderPass viewportRenderPass = VK_NULL_HANDLE;

        void InitEditor();
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
