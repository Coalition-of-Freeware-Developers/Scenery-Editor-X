/**
* -------------------------------------------------------
* Scenery Editor X
* -------------------------------------------------------
* Copyright (c) 2025 Thomas Ray 
* Copyright (c) 2025 Coalition of Freeware Developers
* -------------------------------------------------------
* ui.h
* -------------------------------------------------------
* Created: 25/3/2025
* -------------------------------------------------------
*/
#pragma once
#include <IconsFontAwesome5.h>
#include <imgui/imgui.h>
#include "ui_manager.h"
#include "SceneryEditorX/renderer/image_data.h"
#include "SceneryEditorX/renderer/render_context.h"

/// -------------------------------------------------------

struct Image
{
    //std::shared_ptr<ImageResource> resource;
    uint32_t width = 0;
    uint32_t height = 0;
    //ImageUsageFlags usage;
    //Format format;
    //Layout::ImageLayout layout;
    SceneryEditorX::AspectFlags aspect;
    uint32_t layers = 1;
    uint32_t RID();
    ImTextureID ImGuiRID();
    ImTextureID ImGuiRID(uint32_t layer);
};

//struct Font
//{
//  /**
//	 * @brief Constructor
//	 * @param name The name of the font file that exists within 'assets/fonts' (without extension)
//	 * @param size The font size, scaled by DPI
//	 */
//    Font(const std::string &name, float size)
//        : name{name}, data{SceneryEditorX::fs::read_asset("fonts/" + name + ".ttf")}, size{size}
//    {
//        // Keep ownership of the font data to avoid a double delete
//        ImFontConfig font_config{};
//        font_config.FontDataOwnedByAtlas = false;
//
//        if (size < 1.0f)
//        {
//            size = 20.0f;
//        }
//
//        ImGuiIO &io = ImGui::GetIO();
//        handle = io.Fonts->AddFontFromMemoryTTF(data.data(), static_cast<int>(data.size()), size, &font_config);
//    }
//
//    ImFont *handle{nullptr};
//
//    std::string name;
//
//    std::vector<uint8_t> data;
//
//    float size{};
//};

// -------------------------------------------------------
namespace SceneryEditorX::UI
{
    /**
     * @brief Main GUI class responsible for ImGui integration with Vulkan
     * 
     * This class manages the lifecycle of ImGui resources and provides
     * utilities for common UI operations in the editor.
     */
	class GUI
	{
	public:
	    GUI();
	    ~GUI();

		/**
         * @brief Initialize ImGui with the provided Vulkan renderer and window
         * @param window GLFW window handle
         * @param renderer Graphics engine reference
         * @return True if initialization was successful
         */
        bool InitGUI();
	
        /**
         * @brief Set the command buffer for rendering ImGui
         * @param cmdBuffer Vulkan command buffer to render into
         */
        void SetActiveCommandBuffer(const VkCommandBuffer cmdBuffer) { activeCommandBuffer = cmdBuffer; }
        [[nodiscard]] VkCommandBuffer GetActiveCommandBuffer() const { return activeCommandBuffer; }
	
	    /**
	     * @brief Handles resizing of the window
	     * @param width New width of the window
	     * @param height New height of the window
	     */
	    void Resize(uint32_t width, uint32_t height);
	
        /**
         * @brief Begin a new ImGui frame
         * 
         * This should be called at the start of each frame before any ImGui drawing
         */
        void BeginFrame() const;

        /**
         * @brief End the ImGui frame and render it to the active command buffer
         * 
         * This should be called after all ImGui drawing is complete
         */
        void EndFrame() const;
	
        /**
         * @brief Clean up ImGui resources
         * 
         * This should be called during application shutdown
         */
        void CleanUp();

        /**
         * @brief Update the GUI state
         * @param deltaTime Time elapsed since last update
         */
        void Update(float deltaTime) const;
	
        /**
         * @brief Show the ImGui demo window
         * @param open Pointer to boolean controlling window visibility
         */
        void ShowDemoWindow(bool *open = nullptr) const;

        /**
         * @brief Shows application info in an ImGui window
         * @param appName Application name
         */
        void ShowAppInfo(std::string &appName) const;

        /**
         * @brief Initialize a viewport window for rendering
         * @param size Initial viewport size
         * @param imageView Vulkan image view to render into
         * @return True if initialization was successful
         */
        bool InitViewport(Viewport &size, VkImageView imageView);

        /**
         * @brief Display the viewport window with the scene rendering
         * @param size Reference that will be updated with new viewport size
         * @param hovered Will be set to true if mouse is hovering the viewport
         * @param imageView Vulkan image view containing the rendered scene
         */
        void ViewportWindow(ImVec2 &size, bool &hovered, VkImageView imageView) const;

        /**
         * @brief Set ImGui style (colors, sizes, etc.)
         */
        INTERNAL void SetStyle();

        /**
         * @brief Configure and load fonts for ImGui
         */
        void SetFonts() const;

        /**
         * @brief Get a descriptor set for an image to use in ImGui
         * @param imageView Vulkan image view
         * @param sampler Vulkan sampler
         * @param layout Image layout
         * @return ImTextureID that can be used with ImGui::Image functions
         */
        ImTextureID GetTextureID(VkImageView imageView, VkSampler sampler, VkImageLayout layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) const;

        GLOBAL bool visible;                 /// Used to show/hide the GUI
        LOCAL const std::string defaultFont; /// Default font name

	private:
        VkDescriptorPool imguiPool = VK_NULL_HANDLE;
        VkCommandBuffer activeCommandBuffer = VK_NULL_HANDLE;

	    /// State tracking
	    bool initialized = false;
        bool viewportInitialized = false;
		float contentScaleFactor = 1.0f;	///  Scale factor to apply due to a difference between the window and GL pixel sizes
        float dpiFactor = 1.0f;				/// Scale factor to apply to the size of gui elements (expressed in dp)
        float time = 0.0f;					/// Time elapsed since the last frame

		/// ImGui window flags
        const ImGuiWindowFlags commonFlags = ImGuiWindowFlags_NoCollapse;
        const ImGuiWindowFlags optionsFlags = ImGuiWindowFlags_NoResize;
        const ImGuiWindowFlags infoFlags = ImGuiWindowFlags_NoMove;

        /// Helper methods
        bool CreateDescriptorPool();
        void UpdateDpiScale();

	};

    /**
     * @brief Initialize custom ImGui extensions
     */
	void initImGuiExtensions();

}

/// -------------------------------------------------------
