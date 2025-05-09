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
#include <SceneryEditorX/vulkan/vk_core.h>
//#include <SceneryEditorX/scene/asset_manager.h>
#include <SceneryEditorX/ui/ui.h>
#include <SceneryEditorX/ui/ui_context.h>

// ---------------------------------------------------------

namespace SceneryEditorX
{
	/**
	 * @class EditorApplication
	 * @brief Main application class for the Scenery Editor X.
	 * 
	 * The EditorApplication class serves as the central controller for the editor,
	 * managing the lifecycle of the application and coordinating between the graphics engine,
	 * UI system, asset management, and viewport rendering. It handles initialization,
	 * main loop execution, frame rendering, and resource management.
	 */
	class EditorApplication
	{
	public:
        /**
         * @brief Constructor for the EditorApplication.
         * 
         * Initializes the application instance with default values.
         */
        EditorApplication();
        
        /**
         * @brief Destructor for the EditorApplication.
         * 
         * Cleans up resources and ensures proper shutdown of the application.
         */
        ~EditorApplication();

        /**
         * @brief Initializes the editor components.
         * 
         * Sets up the graphics engine, UI system, and other core components
         * required for the editor to function properly.
         */
        void InitEditor();
        
        /**
         * @brief Starts the editor application.
         * 
         * Launches the main application loop and begins processing events.
         */
		void Run();
        
        /**
         * @brief Updates the editor state.
         * 
         * Called each frame to update UI, process input, and manage the editor state.
         */
        void Update();
        
        /**
         * @brief Renders a single frame.
         * 
         * Coordinates the rendering of UI and scene elements for the current frame.
         */
        void DrawFrame();
        
        /**
         * @brief Creates necessary resources for the editor.
         * 
         * Initializes graphics resources, viewport, and other components
         * needed for editor operation.
         */
        void Create();
        
        /**
         * @brief Contains the main application loop.
         * 
         * Manages the continuous execution of the update and render cycle.
         */
        void MainLoop();

        /**
         * @brief Gets the application window instance.
         * 
         * @return A reference to the Window object managed by the graphics engine.
         */
        Ref<Window> GetWindow() { return gfxEngine.GetWindow(); }
	
	private:
        /**
         * @brief Graphics engine instance for rendering.
         * 
         * Manages Vulkan resources, rendering operations, and the window surface.
         */
        GraphicsEngine gfxEngine;
        
        /**
         * @brief Reference to the swap chain for rendering.
         * 
         * Manages the presentation of rendered frames to the display.
         */
        Ref<SwapChain> vkSwapChain;

        //AssetManager assetManager;
        
        /**
         * @brief UI system for the editor interface.
         * 
         * Handles drawing and interaction with the editor user interface.
         */
        UI::GUI ui;
        
        /**
         * @brief Context for UI rendering and interaction.
         * 
         * Provides state and resources needed for UI operations.
         */
        Ref<UI::UIContext> uiContext;

        /**
         * @brief Viewport configuration and state.
         * 
         * Contains settings related to the editor's main viewport.
         */
        Viewport viewportData;
        
        /**
         * @brief Core rendering configuration and state.
         * 
         * Holds settings and state information used throughout the rendering pipeline.
         */
        RenderData renderData;
        
        /**
         * @brief Vulkan device features enabled for the application.
         * 
         * Specifies which Vulkan hardware features are used by the editor.
         */
        VulkanDeviceFeatures vkDeviceFeatures;
        
        /**
         * @brief Index of the current frame being rendered.
         * 
         * Used to track frame-specific resources in the rendering cycle.
         */
        uint32_t currentFrame = 0;

		// ---------------------------------------------------------

        /**
         * @brief New size for viewport when resizing occurs.
         * 
         * Stores the target size for viewport recreation when dimensions change.
         */
	    glm::ivec2 newViewportSize = viewportData.viewportSize;
	    
	    /**
	     * @brief Counter for total frames rendered since application start.
	     */
	    uint32_t frameCount = 0;

        /**
         * @brief Handle to the logical Vulkan device.
         * 
         * Direct access to the Vulkan device for resource management operations.
         */
	    VkDevice device = VK_NULL_HANDLE;

        /**
         * @brief Creates resources needed for viewport rendering.
         * 
         * Allocates and initializes framebuffers, render targets, and other 
         * resources required for rendering to the viewport.
         */
        void CreateViewportResources();
        
        /**
         * @brief Cleans up viewport rendering resources.
         * 
         * Releases framebuffers, render targets, and other resources 
         * associated with viewport rendering.
         */
        void CleanupViewportResources();
        
        /**
         * @brief Handles surface resize events.
         * 
         * Called when the window or rendering surface changes size to update
         * viewport dimensions and recreate rendering resources as needed.
         * 
         * @param width New surface width in pixels
         * @param height New surface height in pixels
         */
        void OnSurfaceUpdate(uint32_t width, uint32_t height);
        
        /**
         * @brief Recreates frame-related resources.
         * 
         * Rebuilds swap chain images, framebuffers, and other resources
         * needed for rendering when the rendering context changes.
         */
        void RecreateFrameResources();
    };

} // namespace SceneryEditorX

// -------------------------------------------------------
