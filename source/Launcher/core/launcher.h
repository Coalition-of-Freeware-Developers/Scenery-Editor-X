/**
* -------------------------------------------------------
* Scenery Editor X
* -------------------------------------------------------
* Copyright (c) 2025 Thomas Ray 
* Copyright (c) 2025 Coalition of Freeware Developers
* -------------------------------------------------------
* updater.h
* -------------------------------------------------------
* Created: 16/3/2025
* -------------------------------------------------------
*/
#pragma once
#include <SceneryEditorX/renderer/vulkan/vk_data.h>
//#include <SceneryEditorX/renderer/vulkan/vk_cmd_buffers.h>
#include <SceneryEditorX/core/window/window.h>
#include <SceneryEditorX/renderer/render_context.h>
//#include <SceneryEditorX/scene/asset_manager.h>
#include <SceneryEditorX/renderer/renderer.h>
#include <SceneryEditorX/ui/ui.h>
#include <SceneryEditorX/ui/ui_context.h>

/// -------------------------------------------------------

namespace SceneryEditorX
{

    class Launcher
    {
    public:

        Launcher();
        virtual ~Launcher() = default;

        /**
         * @brief Initializes the launcher components.
         * 
         * Sets up the graphics engine, UI system, and other core components
         * required for the editor to function properly.
         */
		void InitLauncher();

        /**
         * @brief Starts the launcher application.
         * 
         * Launches the main application loop and begins processing events.
         */
		void Run();
        
        /**
         * @brief Updates the launcher state.
         * 
         * Called each frame to update UI, process input, and manage the editor state.
         */
        void Update() const;
        
        /**
         * @brief Renders a single frame.
         * 
         * Coordinates the rendering of UI and scene elements for the current frame.
         */
        void DrawFrame();
        
        /**
         * @brief Creates necessary resources for the launcher.
         * 
         * Initializes graphics resources, viewport, and other components
         * needed for launcher operation.
         */
        void Create();
        
        /**
         * @brief Contains the main application loop.
         * 
         * Manages the continuous execution of the update and render cycle.
         */
        void MainLoop();

        bool isUpdate = false;      /// Flag indicating whether an update is available.
        bool isLatest = false;      /// Flag indicating whether the user is using the latest version.
        bool isBeta = false;        /// Flag indicating whether the user is using a beta version.
        bool autoUpdate = false;    /// Flag indicating whether the application will automatically update.
        std::string currentVersion; /// The current version of the application.
        std::string latestVersion;  /// The latest version available on GitHub.


        /**
		 * @brief Checks for updates by comparing the current version with the latest version available on GitHub.
		 *
		 * This function uses cURL to fetch the latest release information from the GitHub API.
		 * It then parses the JSON response to extract the latest version tag and compares it with the current version.
		 * If an update is available, it prints a message indicating the new version. Otherwise, it confirms that the user is using the latest version.
		 */
        void UpdateCheck() const;

        static void cleanInstall();

    private:
        using GraphicsEngine = Renderer;
        /**
         * @brief Graphics engine instance for rendering.
         * 
         * Manages Vulkan resources, rendering operations, and the window surface.
         */
        GraphicsEngine gfxEngine;

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

        /// ---------------------------------------------------------

        static bool urlCheck();
        static void startUpdate();
        static void skipUpdate();
        bool isFirstRun = false; /// Flag indicating whether this is the first time the application is run.

        /**
		 * @brief Callback function for handling data received from a cURL request.
		 *
		 * This function is called by cURL as soon as there is data received that needs to be saved.
		 * The data is appended to the string provided by the user through the userp parameter.
		 *
		 * @param contents Pointer to the delivered data.
		 * @param size Size of a single data element.
		 * @param nmemb Number of data elements.
		 * @param userp Pointer to the user-defined string where the data will be appended.
		 * @return The number of bytes actually taken care of.
		 */
        static size_t WriteCallback(void *contents, size_t size, size_t nmemb, void *userp);

        /**
         * @brief New size for viewport when resizing occurs.
         * 
         * Stores the target size for viewport recreation when dimensions change.
         */
        Viewport newViewportSize = viewportData.GetViewportSize();

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
        //void RecreateFrameResources();

    };

}

/// -------------------------------------------------------
